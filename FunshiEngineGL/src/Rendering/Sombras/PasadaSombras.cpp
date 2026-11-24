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
#include "PasadaSombras.h"

#include <algorithm>
#include <iostream>

#include "../../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../../Iluminacion/LightSystem.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/Componentes/Sombras/Sombras.h"
#include "../../Objetos/GameObject.h"
#include "../../Assets/Mallas/Mesh.h"
#include "../Mallas/MeshRenderer.h"

namespace Rendering {

namespace {

// Tipo de luz en LightData: 0 direccional, 1 punto, 2 spot.
TipoLuzProyector tipoDe(int tipoLuz) {
    if (tipoLuz == 0) return TipoLuzProyector::Direccional;
    if (tipoLuz == 2) return TipoLuzProyector::Spot;
    return TipoLuzProyector::Puntual;
}

}  // namespace

PasadaSombras::PasadaSombras(Backend::IRenderBackend& backend)
    : backend_(backend), mapas_(backend) {}

void PasadaSombras::recogerCandidatos(const Entrada& entrada) {
    candidatos_.clear();
    const int cantidad = (entrada.luces && entrada.duenos)
                             ? std::min(entrada.cantidadLuces, LightSystem::kMaxLights)
                             : 0;
    for (int i = 0; i < cantidad; ++i) {
        GameObject* dueno = entrada.duenos[i];
        if (!dueno) continue;
        Sombras* ajustes = dueno->getComponent<Sombras>();
        if (!ajustes || !ajustes->estaActivado()) continue;

        const LightData& luz = entrada.luces[i];
        CandidatoProyector candidato;
        candidato.indiceLuz = i;
        candidato.posicion = glm::vec3(luz.worldPos[0], luz.worldPos[1], luz.worldPos[2]);
        candidato.direccion =
            glm::vec3(luz.direction[0], luz.direction[1], luz.direction[2]);
        candidato.tipo = tipoDe(luz.type);
        candidato.anguloSpotGrados = luz.spotCutoffDegrees;
        candidato.resolucion = ajustes->getResolucion();
        candidato.sesgo = ajustes->getSesgo();
        candidato.fuerza = ajustes->getFuerza();
        candidato.radio = ajustes->getRadio();
        candidato.distancia = ajustes->getDistancia();
        candidatos_.push_back(candidato);
    }
}

void PasadaSombras::ejecutar(const Entrada& entrada, MeshRenderer& meshRenderer) {
    // Aunque no haya candidatos se sigue: los mapas anteriores se liberan y el
    // shader deja de consultar sombras de un frame que ya no existe.
    recogerCandidatos(entrada);

    // La direccional se encuadra delante de la camara (lo que el jugador ve):
    // el centro queda a medio radio por delante de ella.
    float radioDireccional = 0.f;
    for (const CandidatoProyector& c : candidatos_)
        if (c.tipo == TipoLuzProyector::Direccional)
            radioDireccional = std::max(radioDireccional, c.radio);
    glm::vec3 centro(0.f);
    if (entrada.hayCamara)
        centro = entrada.posicionCamara + entrada.frenteCamara * (radioDireccional * 0.5f);

    elegidos_.assign(static_cast<std::size_t>(kMaxProyectoresSombra), ProyectorSombra());
    const ResultadoSeleccion seleccion = seleccionarProyectores(
        candidatos_.data(), static_cast<int>(candidatos_.size()),
        kMaxProyectoresSombra, centro, elegidos_.data());
    if (seleccion.porLimite > 0) {
        std::cerr << "[Sombras] " << seleccion.porLimite
                  << " luz(es) con sombra quedaron fuera por el limite de mapas ("
                  << kMaxProyectoresSombra << ").\n";
    }

    // Un proyector cuyo mapa no se pudo crear no ocupa slot: si lo ocupara, el
    // shader leeria en ese slot el mapa de otra luz.
    conMapa_.clear();
    for (int i = 0; i < seleccion.elegidos; ++i) {
        ProyectorSombra& proyector = elegidos_[static_cast<std::size_t>(i)];
        const int slot = static_cast<int>(conMapa_.size());
        if (!mapas_.preparar(slot, proyector.resolucion, proyector.esCubemap)) {
            std::cerr << "[Sombras] No se pudo crear el mapa de sombra ("
                      << proyector.resolucion << "px).\n";
            continue;
        }
        const MapaSombras::Mapa& mapa = mapas_.mapa(slot);
        proyector.textura = mapa.textura;

        const int caras = proyector.esCubemap ? kCarasCubemap : 1;
        for (int cara = 0; cara < caras; ++cara) {
            if (proyector.esCubemap)
                backend_.bindDepthTargetCuboFace(mapa.target, cara);
            else
                backend_.bindDepthTarget(mapa.target);
            backend_.setDepthTestEnabled(true);
            backend_.setDepthMask(true);
            backend_.setDepthFunc(Backend::kDepthFuncLess);
            // El destino no tiene color: limpiar solo deja la profundidad en 1.
            backend_.clearScreen(nullptr);
            dibujarGeometria(entrada, meshRenderer,
                             proyector.esCubemap ? proyector.matricesCara[cara]
                                                 : proyector.matrizLuz,
                             proyector);
        }
        conMapa_.push_back(proyector);
    }
    mapas_.liberarDesde(static_cast<int>(conMapa_.size()));

    // Vuelta al framebuffer de la ventana: el mapa es cuadrado y dejo el
    // viewport en su lado.
    backend_.bindDefaultFramebuffer();
    if (entrada.framebufferAncho > 0 && entrada.framebufferAlto > 0)
        backend_.setViewport(0, 0, entrada.framebufferAncho, entrada.framebufferAlto);

    meshRenderer.setSombras(conMapa_.data(), static_cast<int>(conMapa_.size()));
}

void PasadaSombras::dibujarGeometria(const Entrada& entrada,
                                     MeshRenderer& meshRenderer,
                                     const glm::mat4& matriz,
                                     const ProyectorSombra& proyector) {
    if (!entrada.objetos || entrada.objetos->isEmpty()) return;

    MeshRenderer::ParametrosProfundidad pase;
    pase.matrizLuz = matriz;
    pase.esCubo = proyector.esCubemap;
    pase.posicionLuz = proyector.posicionLuz;
    pase.alcance = proyector.distancia;

    Position<GameObject*>* pos = entrada.objetos->first();
    while (pos && pos->getElement()) {
        GameObject* objeto = pos->getElement();
        Model* modelo = objeto->getComponent<Model>();
        Transform* transform = modelo ? objeto->getGlobalTransform() : nullptr;
        const Mesh* malla = modelo ? modelo->getMesh() : nullptr;
        if (malla && !malla->isEmpty() && transform) {
            float matrizModelo[16];
            buildMatrixFromTransform(transform, matrizModelo);
            meshRenderer.intentarRenderSombra(malla, matrizModelo, pase);
        }
        pos = (pos != entrada.objetos->last()) ? entrada.objetos->next(pos) : nullptr;
    }
}

}  // namespace Rendering
