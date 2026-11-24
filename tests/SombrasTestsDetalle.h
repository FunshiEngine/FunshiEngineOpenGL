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
// Preambulo compartido por las unidades de SombrasTests.cpp (generado al dividirlo).
#ifndef SOMBRASTESTSDETALLE_H
#define SOMBRASTESTSDETALLE_H

// Pruebas headless del sistema de sombras:
//
//  - la matriz de luz (CPU puro): el frustum ortografico abarca la caja de
//    lado 2*radio alrededor del foco, el plano lejano queda delante del
//    cercano, la matriz es finita y la direccion de la luz manda,
//  - el componente de sombras: sus valores por defecto, el acotado de los
//    valores imposibles y el viaje de ida y vuelta por el archivo binario,
//  - la seleccion de proyectores y el mapa de sombras: que solo projecute las
//    luces que pueden, que el limite se respete y que los objetivos de
//    profundidad se rehagan solo cuando cambia la resolucion,
//  - el límite de mapas: el shader de mallas declara cuantos mapas puede
//    consultar y tiene que ser el mismo que el motor.
//
// El backend es falso (no-op): sin pila grafica ni driver.

#include <cmath>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <glm/glm.hpp>
#include "../FunshiEngineGL/src/Iluminacion/MatrizLuz.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Sombras/Sombras.h"
#include "../FunshiEngineGL/src/Rendering/Shaders/ShaderSources.h"
#include "../FunshiEngineGL/src/Rendering/Sombras/MapaSombras.h"
#include "TempPruebas.h"

using Rendering::Backend::BlendMode;

using Rendering::Backend::DepthFunc;

using Rendering::Backend::Handle;

using Rendering::Backend::Image2D;

using Rendering::Backend::MeshData;

using Rendering::CandidatoProyector;

using Rendering::MapaSombras;

using Rendering::ProyectorSombra;

using Rendering::ResultadoSeleccion;

using Rendering::TipoLuzProyector;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// --- Seleccion de proyectores y mapas de sombra ------------------------------

// Backend falso: solo se usa el objetivo de profundidad, asi que no hace
// falta ni pila grafica ni driver.
class BackendFalso : public Rendering::Backend::IRenderBackend {
public:
    int creados = 0;
    int destruidos = 0;
    int ultimoTamano = 0;

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

    Handle createProgram(const char*, const char*) override { return 5; }
    void destroyProgram(Handle) override {}
    void useProgram(Handle) override {}
    int uniformLocation(Handle, const char*) override { return -1; }
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
    Handle renderTargetColorTexture(Handle) const override { return 7; }
    void* imguiTextureId(Handle) const override { return nullptr; }

    Handle createDepthTarget(int size) override {
        ++creados;
        ultimoTamano = size;
        return 100 + static_cast<Handle>(creados);
    }
    Handle createDepthTargetCubo(int size) override {
        ++creados;
        ultimoTamano = size;
        return 200 + static_cast<Handle>(creados);
    }
    void bindDepthTargetCuboFace(Handle, int) override {}
    void destroyDepthTarget(Handle) override { ++destruidos; }
    void bindDepthTarget(Handle) override {}
    Handle depthTargetTexture(Handle target) const override {
        return target + 1000;
    }

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

// Estado de la unidad original: una sola instancia (se define en SombrasTests.cpp).

namespace SombrasTestsDetalle {

extern int total;
extern int fallos;

}  // namespace SombrasTestsDetalle
using namespace SombrasTestsDetalle;

// Funciones repartidas entre las unidades.

glm::vec3 aNdc(const glm::mat4& m, const glm::vec3& p);
glm::vec3 adelanteDeVista(const glm::mat4& vista);
bool esFinita(const glm::mat4& m);
bool cercaDe(float a, float b, float tol = 1e-3f);
bool cercaDe(const glm::vec3& a, const glm::vec3& b, float tol = 1e-3f);
void testVistaMiraEnLaDireccion();
void testVistaVerticalNoEsNaN();
void testVistaDireccionNula();
void testProyeccionAbragaLaCaja();
void testProyeccionVolumenValido();
void testMatrizLuzEnfocaElFoco();
void testMatrizCubreLaEscena();
void testLaDireccionManda();
void testValoresPorDefecto();
void testValoresAcotados();
void testViajeDeIdaYVuelta();
void testLecturaAcotaValores();
void testSeleccionDeProyectores();
void testSeleccionSinCandidatos();
void testMapaDeSombras();
void testMapaCubemap();
void testElProyectorConservaElIndiceDeSuLuz();
void testElLimiteCoincideConElShader();
void testCarasDelCuboMiranSuDireccion();
void testSpotEnPerspectiva();
void testDireccionalEstable();

#endif
