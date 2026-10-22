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
// Pruebas headless de la vista previa de un material.
//
// La vista previa es la esfera de muestra que el panel del material dibuja:
// un servicio con su propio render target y su propio renderer (con luces
// fijas). La suite comprueba su contrato sin GPU, con un backend falso:
//   - cada material editado tiene su vista propia (la clave la distingue),
//   - la vista se dibuja en un render target del tamano de la vista previa,
//     con su viewport, y vuelve al framebuffer de la ventana,
//   - el material editado llega al shader (el color difuso se sube),
//   - al invalidarla queda sucia y se vuelve a dibujar.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../FunshiEngineGL/src/Objetos/Componentes/Material.h"
#include "../FunshiEngineGL/src/Rendering/Preview/PreviewMaterial.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/GestorShaders.h"

using Rendering::Backend::BlendMode;
using Rendering::Backend::DepthFunc;
using Rendering::Backend::Handle;
using Rendering::Backend::Image2D;
using Rendering::Backend::IRenderBackend;
using Rendering::Backend::MeshData;

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

// Backend falso: no hay GPU, asi que solo se registran las llamadas que
// importan al contrato de la vista previa (render target, viewport, limpieza,
// dibujo de la malla y los uniforms del material).
class BackendFalso : public IRenderBackend {
public:
    int renderTargetsCreados = 0;
    int ultimoTamanoRenderTarget = 0;
    int mallasDibujadas = 0;
    int uniformsLocados = 0;
    bool vistaPreviaActiva = false;
    bool volvioAlFramebufferPrincipal = false;
    int ultimoViewport[4] = {0, 0, 0, 0};
    bool limpio = false;
    // Ultimo vec4 que se subio por cada uniform (los del material).
    std::unordered_map<std::string, std::vector<float>> uniformesVec4;

    bool init() override { return true; }
    bool available() const override { return true; }

    Handle createMesh(const MeshData&) override { return 1; }
    void destroyMesh(Handle) override {}
    void drawMesh(Handle, unsigned int, unsigned int) override { ++mallasDibujadas; }

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

    Handle createProgram(const char*, const char*) override { return 5; }
    void destroyProgram(Handle) override {}
    void useProgram(Handle) override {}
    // Cada uniform recibe un identificador propio: asi la suite puede
    // comprobar que el material editado llega al shader.
    int uniformLocation(Handle, const char* nombre) override {
        auto it = ids_.find(nombre);
        if (it != ids_.end()) return it->second;
        const int id = ++uniformsLocados;
        ids_[nombre] = id;
        return id;
    }
    void setUniformMat4(int, const glm::mat4&) override {}
    void setUniformMat3(int, const glm::mat3&) override {}
    void setUniformVec3(int, const glm::vec3&) override {}
    void setUniformVec4(int id, const float* value) override {
        for (const auto& par : ids_) {
            if (par.second != id) continue;
            uniformesVec4[par.first] =
                std::vector<float>(value, value + 4);
            return;
        }
    }
    void setUniformFloat(int, float) override {}
    void setUniformInt(int, int) override {}

    Handle createRenderTarget(int width, int height) override {
        ++renderTargetsCreados;
        ultimoTamanoRenderTarget = width;
        return 100 + static_cast<Handle>(renderTargetsCreados);
    }
    void destroyRenderTarget(Handle) override {}
    void bindRenderTarget(Handle) override { vistaPreviaActiva = true; }
    void bindDefaultFramebuffer() override {
        vistaPreviaActiva = false;
        volvioAlFramebufferPrincipal = true;
    }
    Handle renderTargetColorTexture(Handle target) const override {
        return target + 1000;
    }

    Handle createDepthTarget(int) override { return 8; }
    Handle createDepthTargetCubo(int) override { return 9; }
    void bindDepthTargetCuboFace(Handle, int, Handle) override {}
    void destroyDepthTarget(Handle) override {}
    void bindDepthTarget(Handle) override {}
    Handle depthTargetTexture(Handle target) const override { return target + 2000; }

    // Cada textura tiene su descriptor: la GUI los distingue.
    void* imguiTextureId(Handle texture) const override {
        return reinterpret_cast<void*>(texture * 8);
    }

    void setViewport(int x, int y, int width, int height) override {
        ultimoViewport[0] = x;
        ultimoViewport[1] = y;
        ultimoViewport[2] = width;
        ultimoViewport[3] = height;
    }
    void clearScreen(const float[3]) override { limpio = true; }
    void applyBaseState() override {}
    void setClearColor(const float[3]) override {}
    const char* diagnosticoGPU() const override { return "backend falso"; }
    void setBlendEnabled(bool) override {}
    void setBlendMode(BlendMode) override {}
    void setDepthTestEnabled(bool) override {}
    void setDepthMask(bool) override {}
    void setDepthFunc(DepthFunc) override {}
    void drawFullscreenTriangle() override {}

private:
    std::unordered_map<std::string, int> ids_;
};

int main() {
    BackendFalso backend;
    Rendering::GestorShaders gestor(backend);
    PreviewMaterial preview(backend, &gestor, nullptr);

    CHECK(preview.disponible(), "la vista previa esta disponible con backend");
    CHECK(preview.lado() > 0, "la vista previa declara su tamano");

    // Una vista que todavia no se ha dibujado: esta sucia y no tiene textura.
    CHECK(preview.sucia("material"), "una vista nueva esta sucia");
    CHECK(preview.descriptorTextura("material") == nullptr,
          "una vista nueva todavia no tiene textura");

    Material material;
    material.setDiffuse(0.25f, 0.5f, 0.75f);
    material.setShininess(48.0f);

    CHECK(preview.renderizar("material", material), "la vista previa se dibuja");
    CHECK(backend.renderTargetsCreados == 1,
          "la vista crea su propio render target");
    CHECK(backend.ultimoTamanoRenderTarget == preview.lado(),
          "el render target es del tamano de la vista previa");
    CHECK(backend.ultimoViewport[2] == preview.lado() &&
              backend.ultimoViewport[3] == preview.lado(),
          "el viewport se ajusta al tamano de la vista previa");
    CHECK(backend.limpio, "la vista se limpia antes de dibujar");
    CHECK(backend.mallasDibujadas > 0, "la esfera de muestra se dibuja");
    CHECK(backend.volvioAlFramebufferPrincipal,
          "despues de la vista previa se vuelve al framebuffer de la ventana");

    // El material editado llega al shader: sin esto la esfera mostraria el
    // material por defecto y el usuario no veria su cambio.
    const auto difuso = backend.uniformesVec4.find("uMaterialDiffuse");
    CHECK(difuso != backend.uniformesVec4.end(),
          "el color difuso del material se sube al shader");
    if (difuso != backend.uniformesVec4.end()) {
        CHECK(std::fabs(difuso->second[0] - 0.25f) < 1e-4f &&
                  std::fabs(difuso->second[1] - 0.5f) < 1e-4f &&
                  std::fabs(difuso->second[2] - 0.75f) < 1e-4f,
              "el color difuso es el del material que se esta editando");
    }
    const auto brillo = backend.uniformesVec4.find("uMaterialShininess");
    CHECK(brillo == backend.uniformesVec4.end(), "el brillo no es un vec4");

    // Despues de dibujar, la vista esta limpia y tiene textura.
    CHECK(!preview.sucia("material"), "despues de dibujar la vista esta limpia");
    CHECK(preview.descriptorTextura("material") != nullptr,
          "la vista dibujada tiene textura");

    // Invalidar: el material cambio, hay que redibujar.
    preview.invalidar("material");
    CHECK(preview.sucia("material"), "invalidar deja la vista sucia");

    // Cada material editado tiene su vista propia: la clave las distingue.
    CHECK(preview.renderizar("region:Cuero", material),
          "una segunda vista se dibuja");
    CHECK(backend.renderTargetsCreados == 2,
          "la segunda vista tiene su propio render target");
    CHECK(preview.descriptorTextura("material") !=
              preview.descriptorTextura("region:Cuero"),
          "las vistas tienen texturas distintas");

    // Sin programa (sin gestor de shaders) la vista no se puede dibujar, y
    // sigue sucia para reintentarlo en el siguiente frame.
    BackendFalso backendSinPrograma;
    PreviewMaterial sinPrograma(backendSinPrograma, nullptr, nullptr);
    CHECK(!sinPrograma.renderizar("material", material),
          "sin programa la vista previa no se dibuja");
    CHECK(sinPrograma.sucia("material"),
          "sin programa la vista sigue sucia (se reintenta)");

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
