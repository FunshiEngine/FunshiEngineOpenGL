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

#include "IconosGUI.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace {

// Directorio del ejecutable (sin el nombre del binario, con separador final).
// Refleja donde quedo el build y NO depende del directorio desde el que se
// lance el engine: hasta ahora los iconos se resolvian 100% relativo al cwd y
// segun quien corriera el exe se veian las imagenes de una carpeta u otra.
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

} // namespace

IconosGUI::IconosGUI() {}

IconosGUI::~IconosGUI() {
    // Todos los iconos cargados se liberan igual; conviene recorrer el array
    // en vez de repetir el bloque destroyTexture2D para cada miembro.
    Rendering::Backend::Handle* iconos[] = {
        &iconoCarpeta, &iconoArchivo, &iconoCpp, &iconoHpp, &iconoJava,
        &iconoGameObject, &iconoDepuracion, &iconoPlay, &iconoPausa,
        &iconoReset, &iconoStop, &iconoLogo, &iconoBlend, &iconoCsv, &iconoExr,
        &iconoFbx, &iconoHdr, &iconoJpeg, &iconoJpg, &iconoJson, &iconoMax,
        &iconoMaya, &iconoMp3, &iconoObj, &iconoPrefab, &iconoOgg, &iconoOtf, &iconoPng,
        &iconoPsd, &iconoRs, &iconoTga, &iconoTtf, &iconoWav, &iconoXml,
        &iconoDb, &iconoMtl, &iconoRar, &iconoZip,
    };
    for (Rendering::Backend::Handle* icono : iconos) {
        if (*icono != Rendering::Backend::kInvalidHandle) {
            Rendering::Backend::activeBackend().destroyTexture2D(*icono);
            *icono = Rendering::Backend::kInvalidHandle;
        }
    }
}

void IconosGUI::init() {
    if (inicializado) return;
    iconoCarpeta = cargarPNG("File-Image/folder.png");
    iconoArchivo = cargarPNG("File-Image/file.png");
    iconoCpp = cargarPNG("File-Image/cpp.png");
    iconoHpp = cargarPNG("File-Image/hpp.png");
    iconoJava = cargarPNG("File-Image/java.png");
    iconoGameObject = cargarPNG("GUI-Image/Inspector/cubo.png");
    iconoDepuracion = cargarPNG("GUI-Image/MenuBar/debug-button-white.png");
    iconoPlay = cargarPNG("GUI-Image/MenuBar/play-button-triangle-white.png");
    iconoPausa = cargarPNG("GUI-Image/MenuBar/pause-button-white.png");
    iconoReset = cargarPNG("GUI-Image/MenuBar/restart-button-white.png");
    iconoStop = cargarPNG("GUI-Image/MenuBar/stop-button-white.png");
    // El logo guarda sus dimensiones para que los consumidores respeten la
    // proporcion del asset a cualquier alto de pantalla.
    iconoLogo = cargarPNG("Logo/FunshiEngineGL_Isotipo_Blanco.png", &anchoLogo,
                          &altoLogo);
    if (altoLogo < 1) altoLogo = 1;
    // Iconos por extension de asset/formato (Imagenes/File-Image/ + nombre.png).
    iconoBlend = cargarPNG("File-Image/blend.png");
    iconoCsv = cargarPNG("File-Image/csv.png");
    iconoExr = cargarPNG("File-Image/exr.png");
    iconoFbx = cargarPNG("File-Image/fbx.png");
    iconoHdr = cargarPNG("File-Image/hdr.png");
    iconoJpeg = cargarPNG("File-Image/jpeg.png");
    iconoJpg = cargarPNG("File-Image/jpg.png");
    iconoJson = cargarPNG("File-Image/json.png");
    iconoMax = cargarPNG("File-Image/max.png");
    iconoMaya = cargarPNG("File-Image/maya.png");
    iconoMp3 = cargarPNG("File-Image/mp3.png");
    iconoObj = cargarPNG("File-Image/obj.png");
    iconoPrefab = cargarPNG("File-Image/prefab.png");
    iconoOgg = cargarPNG("File-Image/ogg.png");
    iconoOtf = cargarPNG("File-Image/otf.png");
    iconoPng = cargarPNG("File-Image/png.png");
    iconoPsd = cargarPNG("File-Image/psd.png");
    iconoRs = cargarPNG("File-Image/rs.png");
    iconoTga = cargarPNG("File-Image/tga.png");
    iconoTtf = cargarPNG("File-Image/ttf.png");
    iconoWav = cargarPNG("File-Image/wav.png");
    iconoXml = cargarPNG("File-Image/xml.png");
    iconoDb = cargarPNG("File-Image/db.png");
    iconoMtl = cargarPNG("File-Image/mtl.png");
    iconoRar = cargarPNG("File-Image/rar.png");
    iconoZip = cargarPNG("File-Image/zip.png");
    inicializado = true;
}

ImTextureID IconosGUI::aImTexture(Rendering::Backend::Handle handle) {
    if (handle == Rendering::Backend::kInvalidHandle) return ImTextureID_Invalid;
    const void* descriptor =
        Rendering::Backend::activeBackend().imguiTextureId(handle);
    return descriptor
               ? static_cast<ImTextureID>(reinterpret_cast<std::intptr_t>(
                     descriptor))
               : ImTextureID_Invalid;
}

Rendering::Backend::Handle IconosGUI::cargarPNG(const char* nombrePNG) {
    int ancho = 0, alto = 0;
    return cargarPNG(nombrePNG, &ancho, &alto);
}

Rendering::Backend::Handle IconosGUI::cargarPNG(const char* nombrePNG,
                                                int* ancho, int* alto) {
    // Orden de busqueda:
    //  1) Relativo al directorio del ejecutable (determinista): los vectores
    //     de build activo suelen quedar junto al binario o un nivel arriba
    //     (build/ -> ../Imagenes, build-java/ -> ../FunshiEngineGL/Imagenes).
    //  2) Relativo al cwd (fallback historico): si el exe se corre desde
    //     build/, la raiz del proyecto o la raiz del proyecto del usuario.
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

    std::string rutaEncontrada;
    for (const std::string& carpeta : carpetas) {
        const std::string ruta = carpeta + nombrePNG;
        std::ifstream archivo(ruta.c_str(), std::ios::binary);
        if (archivo.good()) {
            rutaEncontrada = ruta;
            break;
        }
    }
    if (rutaEncontrada.empty()) {
        std::cerr << "[IconosGUI] No se encontro la imagen '" << nombrePNG
                  << "' (se probaron varias rutas relativas al cwd).\n";
        return Rendering::Backend::kInvalidHandle;
    }

    int anchoLocal = 0, altoLocal = 0, canales = 0;
    unsigned char* pixeles =
        stbi_load(rutaEncontrada.c_str(), &anchoLocal, &altoLocal, &canales, 4);
    if (!pixeles) {
        std::cerr << "[IconosGUI] stbi_load fallo en: " << rutaEncontrada << "\n";
        return Rendering::Backend::kInvalidHandle;
    }
    if (ancho) *ancho = anchoLocal;
    if (alto) *alto = altoLocal;

    // La textura se crea por el backend (con mipmaps trilineales para que la
    // minificacion a tamanos chicos no produzca "dientes"/alias) y la GUI
    // recibe el descriptor opaco cuando la pinta. Ningun GL aca.
    Rendering::Backend::Image2D gpuImage;
    gpuImage.width = anchoLocal;
    gpuImage.height = altoLocal;
    gpuImage.pixels = pixeles;
    gpuImage.generateMipmaps = true;
    const Rendering::Backend::Handle textura =
        Rendering::Backend::activeBackend().createTexture2D(gpuImage);

    stbi_image_free(pixeles);
    if (textura == Rendering::Backend::kInvalidHandle) {
        std::cerr << "[IconosGUI] createTexture2D fallo en: " << rutaEncontrada
                  << "\n";
        return Rendering::Backend::kInvalidHandle;
    }
    std::cout << "[IconosGUI] Textura cargada: " << rutaEncontrada
              << " (" << anchoLocal << "x" << altoLocal << ")\n";
    return textura;
}

ImTextureID IconosGUI::getIconoPorExtension(const std::string& extension) const {
    std::string ext = extension;
    for (auto& c : ext) c = (char)tolower((unsigned char)c);

    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c") return aImTexture(iconoCpp);
    if (ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx") return aImTexture(iconoHpp);
    if (ext == ".java") return aImTexture(iconoJava);

    // Assets y formatos: modelos, imagenes, audio, fuentes y datos.
    if (ext == ".blend" || ext == ".blend1") return aImTexture(iconoBlend);      // Blender
    if (ext == ".max" || ext == ".3ds") return aImTexture(iconoMax);             // 3ds Max
    if (ext == ".ma" || ext == ".mb") return aImTexture(iconoMaya);              // Maya
    if (ext == ".fbx") return aImTexture(iconoFbx);
    if (ext == ".obj") return aImTexture(iconoObj);
    if (ext == ".prefab") return aImTexture(iconoPrefab);
    if (ext == ".png") return aImTexture(iconoPng);
    if (ext == ".jpg" || ext == ".jpe") return aImTexture(iconoJpg);
    if (ext == ".jpeg") return aImTexture(iconoJpeg);
    if (ext == ".tga") return aImTexture(iconoTga);
    if (ext == ".hdr") return aImTexture(iconoHdr);                              // HDR/OpenEXR legacy
    if (ext == ".exr") return aImTexture(iconoExr);
    if (ext == ".psd") return aImTexture(iconoPsd);
    if (ext == ".mp3") return aImTexture(iconoMp3);
    if (ext == ".wav") return aImTexture(iconoWav);
    if (ext == ".ogg") return aImTexture(iconoOgg);
    if (ext == ".ttf") return aImTexture(iconoTtf);
    if (ext == ".otf") return aImTexture(iconoOtf);
    if (ext == ".json") return aImTexture(iconoJson);
    if (ext == ".xml") return aImTexture(iconoXml);
    if (ext == ".csv") return aImTexture(iconoCsv);
    if (ext == ".rs") return aImTexture(iconoRs);                                // Rust
    if (ext == ".db" || ext == ".sqlite" || ext == ".sqlite3") return aImTexture(iconoDb); // Bases de datos
    if (ext == ".mtl") return aImTexture(iconoMtl);                              // Material Wavefront junto a .obj
    if (ext == ".rar" || ext == ".7z" || ext == ".tar" || ext == ".gz") return aImTexture(iconoRar);
    if (ext == ".zip") return aImTexture(iconoZip);
    return aImTexture(iconoArchivo);
}