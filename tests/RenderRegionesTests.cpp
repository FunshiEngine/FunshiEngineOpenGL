/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    ...

    SPDX-License-Identifier: Apache-2.0
*/
// Pruebas headless del dibujado de mallas por regiones (materiales por cara).
//
// Un MeshRenderer con un backend falso (sin GPU) registra cada llamada a
// drawMesh(handle, firstIndex, indexCount). La suite comprueba que:
//   - una malla con regiones se dibuja region por region, con el rango de
//     indices exacto de cada una,
//   - una malla sin regiones se dibuja entera de una vez,
//   - una region con indiceCantidad == 0 se salta sin dibujar.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <iostream>
#include <vector>

#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../FunshiEngineGL/src/Objetos/SimpleObject.h"
#include "../FunshiEngineGL/src/Rendering/Backend/IRenderBackend.h"
#include "../FunshiEngineGL/src/Rendering/MeshRenderer.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/GestorShaders.h"

using Rendering::Backend::Handle;
using Rendering::Backend::IRenderBackend;
using Rendering::Backend::kInvalidHandle;
using Rendering::Backend::MeshData;
using Rendering::Backend::Image2D;
using Rendering::Backend::BlendMode;
using Rendering::Backend::DepthFunc;
using Rendering::GestorShaders;

struct LlamadaDibujo {
    Handle handle = kInvalidHandle;
    unsigned int firstIndex = 0;
    unsigned int indexCount = 0;
};

// Backend falso: sin GPU, solo registra las llamadas de dibujado de malla y
// cumple el contrato de IRenderBackend con no-ops.
class BackendFalso : public IRenderBackend {
public:
    std::vector<LlamadaDibujo> drawCalls;
    bool initCalled = false;
    bool disponible = true;

    bool init() override { initCalled = true; return true; }
    bool available() const override { return disponible; }

    Handle createMesh(const MeshData&) override { return 1; }
    void destroyMesh(Handle) override {}
    void drawMesh(Handle h, unsigned int firstIndex,
                  unsigned int indexCount) override {
        drawCalls.push_back({h, firstIndex, indexCount});
    }

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

    Handle createTextureCube(const ImageCube&) override { return 4; }
    void destroyTextureCube(Handle) override {}
    void bindTextureCube(Handle, int) override {}

    Handle createProgram(const char*, const char*) override { return 5; }
    void destroyProgram(Handle) override {}
    void useProgram(Handle) override {}
    int uniformLocation(Handle, const char*) override { return 0; }
    void setUniformMat4(int, const glm::mat4&) override {}
    void setUniformMat3(int, const glm::mat3&) override {}
    void setUniformVec3(int, const glm::vec3&) override {}
    void setUniformVec4(int, const float*) override {}
    void setUniformFloat(int, float) override {}
    void setUniformInt(int, int) override {}

    Handle createRenderTarget(int, int) override { return 6; }
    void destroyRenderTarget(Handle) override {}
    void bindRenderTarget(Handle) override {}
    void bindDefaultFramebuffer() override {}
    Handle renderTargetColorTexture(Handle) const override { return 6; }

    Handle createDepthTarget(int) override { return 7; }
    Handle createDepthTargetCubo(int) override { return 8; }
    void bindDepthTargetCuboFace(Handle, int) override {}
    void destroyDepthTarget(Handle) override {}
    void bindDepthTarget(Handle) override {}
    Handle depthTargetTexture(Handle) const override { return 7; }

    void* imguiTextureId(Handle) const override { return nullptr; }

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

// Una malla con dos regiones: Rojo [0,6) y Azul [6,12).
Mesh mallaConDosRegiones() {
    Mesh m;
    m.name = "cubo_dos_materiales";
    // 8 esquinas de un cubo.
    m.vertices = {
        {-1.f, -1.f,  1.f}, { 1.f, -1.f,  1.f}, { 1.f,  1.f,  1.f}, {-1.f,  1.f,  1.f},
        {-1.f, -1.f, -1.f}, { 1.f, -1.f, -1.f}, { 1.f,  1.f, -1.f}, {-1.f,  1.f, -1.f},
    };
    m.normals = {
        {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f},
        {0.f, 0.f, -1.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, -1.f}, {0.f, 0.f, -1.f},
    };
    m.indices = {
        0, 1, 2, 0, 2, 3,   // region Rojo: 6 indices
        4, 5, 6, 4, 6, 7,   // region Azul: 6 indices
    };
    m.subMallas = {
        {"Rojo", 0, 6},
        {"Azul", 6, 6},
    };
    return m;
}

// Matriz identidad como float[16].
float identidad[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

void dibujaPorRegiones() {
    BackendFalso backend;
    GestorShaders gestor(backend);
    ::MeshRenderer renderer;
    renderer.setGestorShaders(&gestor);
    renderer.setBackend(backend);

    Mesh m = mallaConDosRegiones();
    SimpleObject contexto;

    bool ok = renderer.intentarRenderMesh(&contexto, &m, identidad, identidad,
                                          identidad, 0.016f);
    CHECK(ok, "intentarRenderMesh devuelve true para una malla con regiones");

    CHECK(backend.drawCalls.size() == 2,
          "una malla con dos regiones genera dos llamadas de dibujado");
    if (backend.drawCalls.size() == 2) {
        CHECK(backend.drawCalls[0].handle == 1,
              "la primera llamada usa el handle de la malla GPU");
        CHECK(backend.drawCalls[0].firstIndex == 0 &&
                  backend.drawCalls[0].indexCount == 6,
              "region 0: rango [0,6)");
        CHECK(backend.drawCalls[1].firstIndex == 6 &&
                  backend.drawCalls[1].indexCount == 6,
              "region 1: rango [6,12)");
    }
}

void dibujaEnteraSinRegiones() {
    BackendFalso backend;
    GestorShaders gestor(backend);
    ::MeshRenderer renderer;
    renderer.setGestorShaders(&gestor);
    renderer.setBackend(backend);

    Mesh m = mallaConDosRegiones();
    m.subMallas.clear(); // sin regiones: se dibuja entera

    SimpleObject contexto;
    bool ok = renderer.intentarRenderMesh(&contexto, &m, identidad, identidad,
                                          identidad, 0.016f);
    CHECK(ok, "intentarRenderMesh devuelve true para una malla sin regiones");

    CHECK(backend.drawCalls.size() == 1,
          "una malla sin regiones genera una llamada de dibujado");
    if (!backend.drawCalls.empty()) {
        CHECK(backend.drawCalls[0].firstIndex == 0 &&
                  backend.drawCalls[0].indexCount == m.indices.size(),
              "dibuja toda la malla: [0, total_indices)");
    }
}

void saltaRegionVacia() {
    BackendFalso backend;
    GestorShaders gestor(backend);
    ::MeshRenderer renderer;
    renderer.setGestorShaders(&gestor);
    renderer.setBackend(backend);

    Mesh m = mallaConDosRegiones();
    // La segunda region queda vacia: indiceCantidad == 0.
    m.subMallas[1].indiceCantidad = 0;

    SimpleObject contexto;
    bool ok = renderer.intentarRenderMesh(&contexto, &m, identidad, identidad,
                                          identidad, 0.016f);
    CHECK(ok, "intentarRenderMesh devuelve true con una region vacia");

    CHECK(backend.drawCalls.size() == 1,
          "una region vacia se salta: solo se dibuja la region no vacia");
    if (!backend.drawCalls.empty()) {
        CHECK(backend.drawCalls[0].firstIndex == 0 &&
                  backend.drawCalls[0].indexCount == 6,
              "se dibuja solo la region no vacia [0,6)");
    }
}

int main() {
    dibujaPorRegiones();
    dibujaEnteraSinRegiones();
    saltaRegionVacia();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
