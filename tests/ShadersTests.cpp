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
// Pruebas headless del sistema de shaders:
//
//  - el preprocesador de #include (resuelto en CPU, porque OpenGL 3.3 no
//    tiene ARB_shading_language_include): inserta el fragmento, resuelve
//    includes anidados, y rechaza un include que no existe, uno ciclico y un
//    anidamiento excesivo,
//  - el gestor de shaders: el programa se compila una sola vez y se comparte
//    entre quien lo pide, el fuente que llega al backend ya viene
//    preprocesado y un nombre desconocido es un error claro,
//  - los shaders de sombra: el programa de profundidad del mapa y la consulta
//    que hace el shader de mallas.
//
// El backend es falso (no-op): sin pila grafica ni driver.

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

#include "../FunshiEngineGL/src/Rendering/Backend/IRenderBackend.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/GestorShaders.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/PreprocesadorShaders.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/ShaderException.h"

using Rendering::Backend::BlendMode;
using Rendering::Backend::DepthFunc;
using Rendering::Backend::Handle;
using Rendering::Backend::IRenderBackend;
using Rendering::Backend::Image2D;
using Rendering::Backend::MeshData;
using Rendering::Backend::kInvalidHandle;

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Backend falso: cuenta los programas que se crean y guarda los fuentes que
// le llegan (para comprobar que el include ya viene resuelto).
class BackendFalso : public IRenderBackend {
public:
    int programasCreados = 0;
    int programasDestruidos = 0;
    std::string ultimoVertex;
    std::string ultimoFragment;

    bool init() override { return true; }
    bool available() const override { return true; }

    Handle createMesh(const MeshData&) override { return 1; }
    void destroyMesh(Handle) override {}
    void drawMesh(Handle, unsigned int, unsigned int) override {}

    Handle createLineBatch(const float*, std::size_t) override { return 2; }
    void destroyLineBatch(Handle) override {}
    void updateLineBatch(Handle, const float*, std::size_t) override {}
    void drawLineBatch(Handle, unsigned int) override {}

    Handle createParticulasBatch(const float*, std::size_t) override { return 5; }
    void destroyParticulasBatch(Handle) override {}
    void updateParticulasBatch(Handle, const float*, std::size_t) override {}
    void drawParticulasBatch(Handle, unsigned int) override {}

    Handle createTexture2D(const Image2D&) override { return 3; }
    void destroyTexture2D(Handle) override {}
    void bindTexture2D(Handle, int) override {}
    void updateTexture2D(Handle, const Image2D&) override {}

    Handle createTextureCube(const IRenderBackend::ImageCube&) override {
        return 4;
    }
    void destroyTextureCube(Handle) override {}
    void bindTextureCube(Handle, int) override {}

    Handle createProgram(const char* vertexSource,
                         const char* fragmentSource) override {
        ++programasCreados;
        ultimoVertex = vertexSource ? vertexSource : "";
        ultimoFragment = fragmentSource ? fragmentSource : "";
        return static_cast<Handle>(100 + programasCreados);
    }
    void destroyProgram(Handle) override { ++programasDestruidos; }
    void useProgram(Handle) override {}
    int uniformLocation(Handle, const char*) override { return -1; }
    void setUniformMat4(int, const glm::mat4&) override {}
    void setUniformMat3(int, const glm::mat3&) override {}
    void setUniformVec3(int, const glm::vec3&) override {}
    void setUniformVec4(int, const float*) override {}
    void setUniformFloat(int, float) override {}
    void setUniformInt(int, int) override {}

    Handle createRenderTarget(int, int) override { return 5; }
    void destroyRenderTarget(Handle) override {}
    void bindRenderTarget(Handle) override {}
    void bindDefaultFramebuffer() override {}
    Handle renderTargetColorTexture(Handle) const override { return 6; }
    void* imguiTextureId(Handle) const override { return nullptr; }

    Handle createDepthTarget(int) override { return 7; }
    Handle createDepthTargetCubo(int) override { return 8; }
    void bindDepthTargetCuboFace(Handle, int, Handle) override {}
    void destroyDepthTarget(Handle) override {}
    void bindDepthTarget(Handle) override {}
    Handle depthTargetTexture(Handle) const override { return 8; }

    void setViewport(int, int, int, int) override {}
    void clearScreen(const float[3]) override {}
    void applyBaseState() override {}
    void setClearColor(const float[3]) override {}
    const char* diagnosticoGPU() const override { return "backend falso"; }
    void setBlendEnabled(bool) override {}
    void setBlendMode(BlendMode) override {}
    void setDepthTestEnabled(bool) override {}
    void setDepthMask(bool) override {}
    void setDepthFunc(DepthFunc) override {}
    void drawFullscreenTriangle() override {}
};

// --- Preprocesador de includes ------------------------------------------------

static void testPreprocesadorIncluye() {
    std::cout << "-- preprocesador: el include se inserta en su sitio"
              << std::endl;

    Rendering::PreprocesadorShaders::Fragmentos fragmentos;
    fragmentos["comun"] = "float escala = 2.0;\n";

    const std::string fuente =
        "#version 330 core\n"
        "#include \"comun\"\n"
        "void main() { escala = 1.0; }\n";

    const std::string resultado =
        Rendering::PreprocesadorShaders::preprocesar(fuente, fragmentos);

    CHECK(resultado.find("float escala = 2.0;") != std::string::npos,
          "el fragmento incluido aparece en el fuente compuesto");
    CHECK(resultado.find("#include \"comun\"") == std::string::npos,
          "el include desaparece del fuente compuesto");
    CHECK(resultado.find("void main() { escala = 1.0; }") != std::string::npos,
          "el resto del fuente se conserva");
    CHECK(resultado.find("#line 1 \"comun\"") != std::string::npos,
          "el fragmento incluido lleva su #line (los errores del driver "
          "señalan el fragmento)");
    CHECK(resultado.find("#line 3 \"shader\"") != std::string::npos,
          "la numeracion del fuente origen se restaura tras el include");
}

static void testPreprocesadorAnidados() {
    std::cout << "-- preprocesador: los includes anidados se resuelven"
              << std::endl;

    Rendering::PreprocesadorShaders::Fragmentos fragmentos;
    fragmentos["base"] = "float a = 1.0;\n#include \"extra\"\n";
    fragmentos["extra"] = "float b = 2.0;\n";

    const std::string resultado = Rendering::PreprocesadorShaders::preprocesar(
        "#version 330 core\n#include \"base\"\n", fragmentos);

    CHECK(resultado.find("float a = 1.0;") != std::string::npos,
          "el fragmento exterior aparece");
    CHECK(resultado.find("float b = 2.0;") != std::string::npos,
          "el fragmento anidado aparece");
    CHECK(resultado.find("#include") == std::string::npos,
          "no queda ningun include sin resolver");
}

static void testPreprocesadorErrores() {
    std::cout << "-- preprocesador: un include invalido es un error claro"
              << std::endl;

    Rendering::PreprocesadorShaders::Fragmentos fragmentos;
    fragmentos["ciclo"] = "#include \"ciclo\"\n";

    // 1. Include que no existe.
    bool lanzo = false;
    try {
        Rendering::PreprocesadorShaders::preprocesar(
            "#version 330 core\n#include \"noExiste\"\n", fragmentos);
    } catch (const ShaderIncludeException& e) {
        lanzo = true;
        CHECK(std::string(e.what()).find("noExiste") != std::string::npos,
              "el error nombra el fragmento que falta");
    }
    CHECK(lanzo, "un include que no existe lanza ShaderIncludeException");

    // 2. Include ciclico: no puede colgarse el motor.
    lanzo = false;
    try {
        Rendering::PreprocesadorShaders::preprocesar(
            "#version 330 core\n#include \"ciclo\"\n", fragmentos);
    } catch (const ShaderIncludeException&) {
        lanzo = true;
    }
    CHECK(lanzo, "un include ciclico lanza ShaderIncludeException");

    // 3. Anidamiento excesivo: se corta, no se cuelga.
    Rendering::PreprocesadorShaders::Fragmentos profundos;
    for (int i = 0; i < 40; ++i)
        profundos["f" + std::to_string(i)] =
            "#include \"f" + std::to_string(i + 1) + "\"\n";
    profundos["f40"] = "float fin = 1.0;\n";
    lanzo = false;
    try {
        Rendering::PreprocesadorShaders::preprocesar(
            "#version 330 core\n#include \"f0\"\n", profundos);
    } catch (const ShaderIncludeException&) {
        lanzo = true;
    }
    CHECK(lanzo, "un anidamiento excesivo lanza ShaderIncludeException");

    // 4. Un include de sistema (<...>) se deja intacto: lo resuelve el driver.
    const std::string resultado = Rendering::PreprocesadorShaders::preprocesar(
        "#version 330 core\n#include <algo>\n", fragmentos);
    CHECK(resultado.find("#include <algo>") != std::string::npos,
          "un include de sistema se deja intacto");
}

// --- Gestor de shaders --------------------------------------------------------

static void testGestorComparteProgramas() {
    std::cout << "-- gestor: el programa se compila una vez y se comparte"
              << std::endl;

    BackendFalso backend;
    Rendering::GestorShaders gestor(backend);

    CHECK(gestor.tiene("malla"), "el gestor conoce los shaders incorporados");
    CHECK(gestor.tiene("sprite"), "el gestor conoce el shader de sprite");
    CHECK(!gestor.tiene("inexistente"), "un shader desconocido no existe");

    auto primero = gestor.obtener("sprite");
    auto segundo = gestor.obtener("sprite");
    CHECK(primero.get() == segundo.get(),
          "dos pedidos del mismo shader devuelven el mismo programa");
    CHECK(backend.programasCreados == 1,
          "el shader se compila una sola vez, no una por pedido");
    CHECK(gestor.cantidadProgramas() == 1, "el gestor lleva un programa");

    // El fragmento compartido no se vuelve a compilar para otro dibujante.
    gestor.obtener("malla");
    CHECK(backend.programasCreados == 2,
          "un shader distinto se compila aparte");

    // Un nombre desconocido es un error claro.
    bool lanzo = false;
    try {
        gestor.obtener("noExiste");
    } catch (const RuntimeException&) {
        lanzo = true;
    }
    CHECK(lanzo, "pedir un shader que no existe es un error");
}

static void testGestorPreprocesaElFuente() {
    std::cout << "-- gestor: el fuente que llega al backend ya viene "
                 "preprocesado"
              << std::endl;

    BackendFalso backend;
    Rendering::GestorShaders gestor(backend);
    gestor.registrarFragmento("luces", "uniform int uLuces;\n");
    gestor.registrarShader("prueba", "#version 330 core\n#include \"luces\"\n",
                            "#version 330 core\nvoid main() {}\n");

    gestor.obtener("prueba");
    CHECK(backend.ultimoVertex.find("uniform int uLuces;") != std::string::npos,
          "el fragmento incluido llega al backend resuelto");
    CHECK(backend.ultimoVertex.find("#include") == std::string::npos,
          "al backend no le llega ningun include");
}

static void testGestorInvalidar() {
    std::cout << "-- gestor: invalidar libera los programas" << std::endl;

    BackendFalso backend;
    Rendering::GestorShaders gestor(backend);
    gestor.obtener("linea");
    CHECK(gestor.cantidadProgramas() == 1, "hay un programa compilado");

    gestor.invalidar();
    CHECK(gestor.cantidadProgramas() == 0, "invalidar deja el gestor vacio");
    CHECK(backend.programasDestruidos == 1, "el programa se libera en el backend");

    // El que lo tenia sigue vivo (puntero compartido): no queda colgando.
    gestor.obtener("linea");
    CHECK(backend.programasCreados == 2, "el shader se vuelve a compilar");
}

static void testShadersDeSombras() {
    std::cout << "-- shaders: el mapa de sombras tiene su programa y el shader "
                 "de mallas lo consulta"
              << std::endl;

    BackendFalso backend;
    Rendering::GestorShaders gestor(backend);

    CHECK(gestor.tiene("profundidad"),
          "el gestor conoce el programa de profundidad");

    gestor.obtener("profundidad");
    CHECK(backend.ultimoVertex.find("uLightMatrix") != std::string::npos,
          "el programa de profundidad proyecta con la matriz de la luz");
    CHECK(backend.ultimoVertex.find("uModel") != std::string::npos,
          "el programa de profundidad aplica la matriz del modelo");
    CHECK(backend.ultimoFragment.find("void main") != std::string::npos,
          "el programa de profundidad tiene su propio fragment shader");

    // El shader de mallas es el que consulta el mapa.
    gestor.obtener("malla");
    CHECK(backend.ultimoFragment.find("sampler2DShadow") != std::string::npos,
          "el shader de mallas lee los mapas como sampler2DShadow");
    CHECK(backend.ultimoFragment.find("uShadowMatrix") != std::string::npos,
          "el shader de mallas recibe la matriz de cada mapa");
    CHECK(backend.ultimoFragment.find("uShadowLightIndex") != std::string::npos,
          "el shader de mallas sabe a que luz pertenece cada mapa");
    CHECK(backend.ultimoFragment.find("uShadowCount") != std::string::npos,
          "el shader de mallas sabe cuantos mapas hay");
}

int main() {
    testPreprocesadorIncluye();
    testPreprocesadorAnidados();
    testPreprocesadorErrores();
    testGestorComparteProgramas();
    testGestorPreprocesaElFuente();
    testGestorInvalidar();
    testShadersDeSombras();

    std::cout << "ShadersTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}
