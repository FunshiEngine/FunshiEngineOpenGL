/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

    SPDX-License-Identifier: Apache-2.0
*/

// Windows.h ANTES del header propio y de la stdlib, para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte.
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "Ventana.h"

#include <fstream>
#include <iostream>
#include <vector>

#include "Rendering/Backend/IRenderBackend.h"
#include "Herramientas/IconosGUI/stb_image.h"

namespace {

// Directorio del ejecutable (mismo criterio que IconosGUI: no depende del cwd
// desde el que se lance el engine).
std::string directorioEjecutable() {
#ifdef _WIN32
    char exe[4096] = {};
    DWORD n = GetModuleFileNameA(nullptr, exe, sizeof(exe));
    if (n == 0 || n >= sizeof(exe)) return "";
    const std::string path(exe, static_cast<std::size_t>(n));
    const std::size_t sep = path.find_last_of("\\/");
    return (sep == std::string::npos) ? "" : path.substr(0, sep + 1);
#else
    char link[4096] = {};
    const ssize_t n = readlink("/proc/self/exe", link, sizeof(link) - 1);
    if (n <= 0) return "";
    link[n] = '\0';
    const std::string path(link);
    const std::size_t sep = path.find_last_of('/');
    return (sep == std::string::npos) ? "" : path.substr(0, sep + 1);
#endif
}

// Ruta del logo del motor (busqueda identica a IconosGUI: junto al ejecutable
// o un nivel arriba, y fallback relativo al cwd).
std::string ubicarLogoVentana() {
    const char* nombre = "FunshiEngineGL_Isotipo_Blanco.png";
    const std::string exeDir = directorioEjecutable();
    std::vector<std::string> carpetas;
    if (!exeDir.empty()) {
        carpetas.push_back(exeDir + "Imagenes/");
        carpetas.push_back(exeDir + "../Imagenes/");
        carpetas.push_back(exeDir + "../FunshiEngineGL/Imagenes/");
        carpetas.push_back(exeDir + "../../Imagenes/");
        carpetas.push_back(exeDir + "../../../Imagenes/");
    }
    const char* rutasCwd[] = {
        "Imagenes/", "../Imagenes/", "../../Imagenes/", "../../../Imagenes/"
    };
    for (const char* carpeta : rutasCwd) carpetas.emplace_back(carpeta);
    for (const std::string& carpeta : carpetas) {
        const std::string ruta = carpeta + nombre;
        std::ifstream archivo(ruta.c_str(), std::ios::binary);
        if (archivo.good()) return ruta;
    }
    return "";
}

// Logo en la barra de titulo de la ventana (el icono que muestra el sistema
// junto al nombre de la aplicacion). GLFW no soporta iconos en macOS, en el
// resto el WM lo escala solo; si el PNG no aparece (build sin Imagenes/) se
// omite sin romper el arranque.
void aplicarIconoVentana(GLFWwindow* window) {
#ifdef __APPLE__
    (void)window;
    return;
#else
    const std::string ruta = ubicarLogoVentana();
    if (ruta.empty()) return;
    int ancho = 0, alto = 0, canales = 0;
    unsigned char* pixeles =
        stbi_load(ruta.c_str(), &ancho, &alto, &canales, 4);
    if (!pixeles || ancho < 1 || alto < 1) {
        if (pixeles) stbi_image_free(pixeles);
        return;
    }
    GLFWimage icono = {ancho, alto, pixeles};
    glfwSetWindowIcon(window, 1, &icono);
    stbi_image_free(pixeles);
#endif
}

} // namespace

Ventana* Ventana::instance = nullptr;

Ventana::Ventana() {
    instance = this;
}

Ventana& Ventana::getInstance() {
    return *instance;
}

GLFWwindow* Ventana::getWindow() {
    return window;
}

int Ventana::getWidth() const {
    return width;
}

int Ventana::getHeight() const {
    return height;
}

int Ventana::initVentana() {
    if (!glfwInit()) return -1;

    // El engine pide un contexto 3.3 core: todo el render es moderno (shaders,
    // VBO/VAO, FBO) y no queda ninguna llamada a estado fijo, asi que el perfil
    // de compatibilidad no aporta nada y solo suma funciones deprecated que
    // podrian tapar un camino moderno mal migrado. 3.3 es el piso que exige
    // GLSL 330 (layout in/out). Si el driver no puede darlo, glfwCreateWindow
    // devuelve null y se aborta con un mensaje claro en vez de seguir hasta que
    // falle el primer dibujo.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Forward compatible: descarta el codigo deprecated que un driver podria
    // exponer por extensions y mantiene el contexto en 3.3 core puro.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    // Antialiasing por multisampling (4x): el contexto por defecto de GLFW se
    // crea sin samples y las lineas (grilla, ejes, wireframes) se ven
    // escalonadas ("a dientes"). Con MSAA el framebuffer se suaviza entero.
    glfwWindowHint(GLFW_SAMPLES, 4);
    window = glfwCreateWindow(width, height, "FunshiEngineGL", nullptr, nullptr);
    if (!window) {
        // Driver sin framebuffer multisampleado: se reintenta sin MSAA antes
        // de abortar (la escena se dibuja igual, solo sin suavizado de lineas).
        glfwWindowHint(GLFW_SAMPLES, 0);
        window = glfwCreateWindow(width, height, "FunshiEngineGL", nullptr, nullptr);
        if (!window) {
            std::cerr << "[Ventana] No se pudo crear un contexto OpenGL 3.3 core."
                      << " El driver de la GPU no lo soporta o esta actualizado."
                      << std::endl;
            glfwTerminate();
            return -1;
        }
    }
    glfwMakeContextCurrent(window);
    // El icono de la barra de titulo (junto al nombre de la aplicacion) se
    // puede fijar con o sin contexto actual; se aplica apenas la ventana
    // existe para que el WM lo muestre desde el primer frame.
    aplicarIconoVentana(window);
    // Carga de las funciones modernas de GL por puntero. Se hace aqui, con el
    // contexto ya activo, y no es opcional: si falta alguna, el engine no sabe
    // dibujar. Los renderers igual la consultan por available() antes de su
    // primer uso, pero llegar hasta aca sin las funciones seria un fallo
    // silencioso en pantalla negra.
    if (!Rendering::Backend::activeBackend().init()) {
        std::cerr << "[Ventana] El driver no expone las funciones de OpenGL 3.3 "
                     "que el engine necesita (shaders, VBO/VAO, uniforms)."
                  << std::endl;
        glfwDestroyWindow(window);
        window = nullptr;
        glfwTerminate();
        return -1;
    }
    // Info del contexto (GPU, versiones, perfil): lo reporta el backend; aca
    // solo se imprime para los logs de arranque.
    std::cout << Rendering::Backend::activeBackend().diagnosticoGPU()
              << std::endl;
    return 0;
}

void Ventana::redimension(int ventanaWidthEjeX, int ventanaHeightEjeY) {
    width = ventanaWidthEjeX;
    height = ventanaHeightEjeY;
    // El viewport del framebuffer lo fija el backend; las matrices de proyec-
    // cion se establecen cada frame en la pasada de la escena (no viven aca).
    Rendering::Backend::activeBackend().setViewport(0, 0, width, height);
}
