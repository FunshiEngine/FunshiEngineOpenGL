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

#include <cmath>

namespace Rendering {

static_assert(kCarasCubemap == kCarasCuboLuz,
              "las caras del mapa y las de la matriz de luz deben coincidir");

namespace {

// Tamano de un texel en el mundo para una proyeccion en perspectiva de campo
// 'campoGrados', medido a mitad del alcance (donde suelen estar los objetos
// que reciben la sombra).
float texelPerspectiva(float campoGrados, float alcance, int resolucion) {
    const float mitad = std::tan(campoGrados * 0.5f * 3.14159265f / 180.f);
    return (2.f * mitad * alcance * 0.5f) / static_cast<float>(resolucion);
}

}  // namespace

ResultadoSeleccion seleccionarProyectores(const CandidatoProyector* candidatos,
                                           int cantidad, int limite,
                                           const glm::vec3& centroEscena,
                                           ProyectorSombra* salida) {
    ResultadoSeleccion resultado;
    if (!candidatos || !salida || cantidad <= 0) return resultado;

    for (int i = 0; i < cantidad; ++i) {
        const CandidatoProyector& candidato = candidatos[i];
        if (resultado.elegidos >= limite) {
            ++resultado.porLimite;
            continue;
        }

        ProyectorSombra& proyector = salida[resultado.elegidos];
        proyector = ProyectorSombra();
        proyector.indiceLuz = candidato.indiceLuz;
        proyector.resolucion = candidato.resolucion > 0 ? candidato.resolucion : 1;
        proyector.sesgo = candidato.sesgo;
        proyector.fuerza = candidato.fuerza;
        proyector.distancia = candidato.distancia;
        proyector.posicionLuz = candidato.posicion;

        switch (candidato.tipo) {
        case TipoLuzProyector::Puntual: {
            proyector.esCubemap = true;
            glm::mat4 caras[kCarasCubemap];
            matricesCaraCubo(candidato.posicion, candidato.distancia, caras);
            for (int cara = 0; cara < kCarasCubemap; ++cara)
                proyector.matricesCara[cara] = caras[cara];
            proyector.matrizLuz = caras[0];
            proyector.tamanoTexelMundo = texelPerspectiva(
                90.f, candidato.distancia, proyector.resolucion);
            break;
        }
        case TipoLuzProyector::Direccional:
            // La luz direccional no esta en ningun sitio: su frustum se
            // encuadra alrededor del centro y se alinea a la grilla de texeles.
            proyector.matrizLuz = matrizDireccionalEstable(
                centroEscena, candidato.direccion, candidato.radio,
                candidato.distancia, proyector.resolucion);
            proyector.tamanoTexelMundo =
                (2.f * candidato.radio) / static_cast<float>(proyector.resolucion);
            break;
        case TipoLuzProyector::Spot:
            proyector.matrizLuz = matrizLuzSpot(
                candidato.posicion, candidato.direccion,
                candidato.anguloSpotGrados, candidato.distancia);
            proyector.tamanoTexelMundo = texelPerspectiva(
                2.f * candidato.anguloSpotGrados + 4.f, candidato.distancia,
                proyector.resolucion);
            break;
        }

        ++resultado.elegidos;
    }
    return resultado;
}

MapaSombras::MapaSombras(Backend::IRenderBackend& backend)
    : backend_(backend) {}

MapaSombras::~MapaSombras() {
    for (Mapa& entrada : entradas_) liberar(entrada);
    entradas_.clear();
}

void MapaSombras::liberar(Mapa& entrada) {
    // destroyDepthTarget destruye el FBO y la textura del objetivo, sea 2D o
    // cubemap: un solo handle basta para los dos casos.
    if (entrada.target != Backend::kInvalidHandle)
        backend_.destroyDepthTarget(entrada.target);
    entrada = Mapa();
}

bool MapaSombras::preparar(int indice, int resolucion, bool esCubemap) {
    if (indice < 0) return false;
    if (indice < static_cast<int>(entradas_.size())) {
        Mapa& entrada = entradas_[static_cast<std::size_t>(indice)];
        if (entrada.target != Backend::kInvalidHandle &&
            entrada.resolucion == resolucion && entrada.esCubemap == esCubemap) {
            return true;
        }
        liberar(entrada);
    } else {
        entradas_.resize(static_cast<std::size_t>(indice) + 1);
    }

    if (resolucion <= 0) return false;

    Mapa& entrada = entradas_[static_cast<std::size_t>(indice)];
    const Backend::Handle target = esCubemap
                                       ? backend_.createDepthTargetCubo(resolucion)
                                       : backend_.createDepthTarget(resolucion);
    if (target == Backend::kInvalidHandle) return false;
    entrada.target = target;
    entrada.textura = backend_.depthTargetTexture(target);
    entrada.esCubemap = esCubemap;
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
    const std::size_t desde = static_cast<std::size_t>(indice);
    for (std::size_t i = desde; i < entradas_.size(); ++i) liberar(entradas_[i]);
    if (desde < entradas_.size()) entradas_.resize(desde);
}

}  // namespace Rendering
