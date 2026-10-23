/*
    FunshiEngineGL - Motor de juegos 3D con OpenGL e ImGui
    Copyright 2026 Gianfranco Ivan Enrique

    Licensed under the Apache License, Version 2.0 (the "License");
    ...

    SPDX-License-Identifier: Apache-2.0
*/
// Pruebas headless del camino completo del componente Model (drag&drop de .obj):
//
//   AssetManager -> resolverDibujoModelo -> MeshRenderer::intentarRenderMesh
//   -> regiones (materiales por cara).
//
// Comprueba que un GameObject con un componente Model que apunta a un .obj con
// dos materiales se dibuja region por region con el rango de indices exacto.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "../FunshiEngineGL/src/Assets/AssetManager.h"
#include "../FunshiEngineGL/src/Assets/AssimpMeshLoader.h"
#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Model.h"
#include "../FunshiEngineGL/src/Rendering/Backend/IRenderBackend.h"
#include "../FunshiEngineGL/src/Rendering/DibujoModelo.h"
#include "../FunshiEngineGL/src/Rendering/MeshRenderer.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/GestorShaders.h"
#include "../tests/TempPruebas.h"

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

class BackendFalso : public IRenderBackend {
public:
    std::vector<LlamadaDibujo> drawCalls;

    bool init() override { return true; }
    bool available() const override { return true; }

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
    void bindDepthTargetCuboFace(Handle, int, Handle) override {}
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

// Escribe un .obj con dos materiales, dos regiones y normales por vertiente.
bool escribirModelo(const std::string& rutaObj, const std::string& rutaMtl) {
    {
        std::ofstream mtl(rutaMtl.c_str());
        if (!mtl) return false;
        mtl << "newmtl Rojo\nKd 0.8 0.1 0.1\n";
        mtl << "newmtl Azul\nKd 0.1 0.1 0.8\n";
    }
    std::ofstream obj(rutaObj.c_str());
    if (!obj) return false;
    obj << "mtllib dosMateriales.mtl\n";
    obj << "v -1.0 -1.0  1.0\n";
    obj << "v  1.0 -1.0  1.0\n";
    obj << "v  1.0  1.0  1.0\n";
    obj << "v -1.0  1.0  1.0\n";
    obj << "v -1.0 -1.0 -1.0\n";
    obj << "v  1.0 -1.0 -1.0\n";
    obj << "v  1.0  1.0 -1.0\n";
    obj << "v -1.0  1.0 -1.0\n";
    obj << "vn 0.0 0.0 1.0\n";
    obj << "vn 0.0 0.0 1.0\n";
    obj << "vn 0.0 0.0 1.0\n";
    obj << "vn 0.0 0.0 1.0\n";
    obj << "vn 0.0 0.0 -1.0\n";
    obj << "vn 0.0 0.0 -1.0\n";
    obj << "vn 0.0 0.0 -1.0\n";
    obj << "vn 0.0 0.0 -1.0\n";
    obj << "usemtl Rojo\n";
    obj << "f 1//1 2//2 3//3\n";
    obj << "f 1//1 3//3 4//4\n";
    obj << "usemtl Azul\n";
    obj << "f 5//5 6//6 7//7\n";
    obj << "f 5//5 7//7 8//8\n";
    return true;
}

float identidad[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

void modeloComponenteDibujaPorRegiones() {
    TempPruebas::CarpetaPrueba carpeta("funshi_modelo_component_render_tests");
    const std::string rutaObj = (carpeta.ruta() / "modelo.obj").string();
    const std::string rutaMtl = (carpeta.ruta() / "modelo.mtl").string();
    CHECK(escribirModelo(rutaObj, rutaMtl), "se escribe el modelo de prueba");

    AssetManager assets(std::make_unique<AssimpMeshLoader>());

    // GameObject con componente Model (el camino del drag&drop del editor).
    GameObject objeto;
    auto model = std::make_unique<Model>();
    model->setPath(rutaObj);
    objeto.addComponent(std::move(model));

    // resolverDibujoModelo debe resolver la malla desde el AssetManager.
    DibujoModelo dibujo = resolverDibujoModelo(&objeto, assets);
    CHECK(dibujo.valido, "resolverDibujoModelo resuelve el modelo");
    CHECK(dibujo.malla != nullptr, "la malla resuelta no es nula");
    auto* m = dibujo.malla;
    if (!m) return;
    CHECK(!m->isEmpty(), "la malla no esta vacia");
    CHECK(m->hasNormals(), "la malla tiene normales por vertiente");
    CHECK(m->subMallas.size() == 2,
          "el .obj con dos materiales produce dos regiones");

    // Dibujar con el renderer: cada region con su rango de indices.
    BackendFalso backend;
    GestorShaders gestor(backend);
    ::MeshRenderer renderer;
    renderer.setGestorShaders(&gestor);
    renderer.setBackend(backend);

    bool ok = renderer.intentarRenderMesh(&objeto, dibujo.malla, dibujo.modelo,
                                          identidad, identidad, 0.016f);
    CHECK(ok, "intentarRenderMesh dibuja el modelo del componente Model");

    CHECK(backend.drawCalls.size() == 2,
          "el modelo con dos regiones genera dos llamadas de dibujado");
    if (backend.drawCalls.size() == 2) {
        CHECK(backend.drawCalls[0].firstIndex == 0 &&
                  backend.drawCalls[0].indexCount == 6,
              "region Rojo: [0,6)");
        CHECK(backend.drawCalls[1].firstIndex == 6 &&
                  backend.drawCalls[1].indexCount == 6,
              "region Azul: [6,12)");
    }
}

int main() {
    modeloComponenteDibujaPorRegiones();
    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
