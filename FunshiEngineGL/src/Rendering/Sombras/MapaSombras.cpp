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
#include "MapaSombras.h"

#include "../../Iluminacion/MatrizLuz.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rendering {

// Direcciones de vista y vectores up para las 6 caras de un cubemap, en el
// orden que OpenGL espera: +X, -X, +Y, -Y, +Z, -Z.
constexpr std::array<std::pair<glm::vec3, glm::vec3>, kCarasCubemap> kCarasCubemapDir{{
    {glm::vec3(1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)},  // +X
    {glm::vec3(-1.f, 0.f, 0.f), glm::vec3(0.f, 1.f, 0.f)}, // -X
    {glm::vec3(0.f, 1.f, 0.f), glm::vec3(0.f, 0.f, -1.f)}, // +Y
    {glm::vec3(0.f, -1.f, 0.f), glm::vec3(0.f, 0.f, 1.f)}, // -Y
    {glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 1.f, 0.f)},  // +Z
    {glm::vec3(0.f, 0.f, -1.f), glm::vec3(0.f, 1.f, 0.f)}, // -Z
}};

ResultadoSeleccion seleccionarProyectores(const CandidatoProyector* candidatos,
                                           int cantidad, int limite,
                                           const glm::vec3& centroEscena,
                                           ProyectorSombra* salida) {
    ResultadoSeleccion resultado;
    // Sin candidatos no hay nada que elegir.
    if (!candidatos || !salida || cantidad <= 0) return resultado;

    for (int i = 0; i < cantidad; ++i) {
        const CandidatoProyector& candidato = candidatos[i];

        if (resultado.elegidos >= limite) {
            ++resultado.porLimite;
            continue;
        }

        ProyectorSombra& proyector = salida[resultado.elegidos];
        proyector.indiceLuz = candidato.indiceLuz;
        proyector.resolucion = candidato.resolucion;
        proyector.sesgo = candidato.sesgo;
        proyector.fuerza = candidato.fuerza;
        proyector.distancia = candidato.distancia;
        proyector.textura = Backend::kInvalidHandle;

        if (candidato.tipo == TipoLuzProyector::Puntual) {
            // Luz puntual: proyecta en todas las direcciones, necesita un
            // cubemap de 6 caras. Una entrada ocupa un slot del limite y
            // almacena las 6 matrices (una por cara).
            proyector.esCubemap = true;
            for (int cara = 0; cara < kCarasCubemap; ++cara) {
                const auto& [dir, up] = kCarasCubemapDir[cara];
                glm::mat4 vista = glm::lookAt(
                    candidato.posicion,
                    candidato.posicion + dir,
                    up);
                glm::mat4 proj = glm::perspective(
                    glm::radians(90.0f), 1.0f,
                    0.05f, candidato.distancia);
                proyector.matricesLuz[cara] = proj * vista;
            }
            proyector.matrizLuz = proyector.matricesLuz[0];
        } else if (candidato.tipo == TipoLuzProyector::Direccional) {
            // La luz direccional no esta en ningun sitio: su frustum se
            // coloca alrededor del centro de la escena.
            FocoLuz foco = focoDireccional(centroEscena, candidato.direccion,
                                           candidato.radio, candidato.distancia);
            proyector.esCubemap = false;
            proyector.matrizLuz = matrizLuz(foco);
            proyector.matricesLuz[0] = proyector.matrizLuz;
        } else {
            // Spot: luz posicional con un solo frustum.
            FocoLuz foco = focoPosicional(candidato.posicion, candidato.direccion,
                                          candidato.radio, candidato.distancia);
            proyector.esCubemap = false;
            proyector.matrizLuz = matrizLuz(foco);
            proyector.matricesLuz[0] = proyector.matrizLuz;
        }

        ++resultado.elegidos;
    }

    return resultado;
}

MapaSombras::MapaSombras(Backend::IRenderBackend& backend)
    : backend_(backend) {}

MapaSombras::~MapaSombras() {
    for (Mapa& entrada : entradas_) {
        if (entrada.target != Backend::kInvalidHandle) {
            backend_.destroyDepthTarget(entrada.target);
        }
        // Un cubemap comparte el FBO de targetsCubo[0] con el target 2D;
        // si es cubemap, entrada.target ya fue destruido arriba, pero las
        // demas entradas de targetsCubo (si hubiera sido necesario separarlas)
        // no existen: usamos un solo FBO para las 6 caras.
    }
    entradas_.clear();
}

bool MapaSombras::preparar(int indice, int resolucion, bool esCubemap) {
    if (indice < 0) return false;
    if (indice < static_cast<int>(entradas_.size())) {
        Mapa& entrada = entradas_[indice];
        if (entrada.resolucion == resolucion &&
            entrada.esCubemap == esCubemap &&
            entrada.target != Backend::kInvalidHandle) {
            return true;
        }
        // La resolucion o el tipo cambio: el mapa viejo ya no sirve.
        backend_.destroyDepthTarget(entrada.target);
        entrada.target = Backend::kInvalidHandle;
        entrada.textura = Backend::kInvalidHandle;
        entrada.esCubemap = false;
        entrada.resolucion = 0;
        for (auto& t : entrada.targetsCubo) t = Backend::kInvalidHandle;
    } else {
        entradas_.resize(static_cast<std::size_t>(indice) + 1);
    }

    if (resolucion <= 0) return false;

    Mapa& entrada = entradas_[static_cast<std::size_t>(indice)];
    entrada.esCubemap = esCubemap;

    if (esCubemap) {
        const Backend::Handle fbo =
            backend_.createDepthTargetCubo(resolucion);
        if (fbo == Backend::kInvalidHandle) return false;
        entrada.target = fbo;
        entrada.targetsCubo[0] = fbo;
        // La textura de cubemap se consulta por depthTargetTexture del FBO.
        entrada.textura = backend_.depthTargetTexture(fbo);
    } else {
        const Backend::Handle target = backend_.createDepthTarget(resolucion);
        if (target == Backend::kInvalidHandle) return false;
        entrada.target = target;
        entrada.textura = backend_.depthTargetTexture(target);
    }
    entrada.resolucion = resolucion;
    return true;
}

const MapaSombras::Mapa& MapaSombras::mapa(int indice) const {
    static const Mapa vacio;
    if (indice < 0 || indice >= static_cast<int>(entradas_.size())) return vacio;
    return entradas_[static_cast<std::size_t>(indice)];
}

void MapaSombras::liberarDesde(int indice) {
    if (indice < 0) indice = 0;
    for (std::size_t i = static_cast<std::size_t>(indice);
         i < entradas_.size(); ++i) {
        if (entradas_[i].target != Backend::kInvalidHandle) {
            backend_.destroyDepthTarget(entradas_[i].target);
        }
    }
    if (static_cast<std::size_t>(indice) < entradas_.size()) {
        entradas_.resize(static_cast<std::size_t>(indice));
    }
}

}  // namespace Rendering
