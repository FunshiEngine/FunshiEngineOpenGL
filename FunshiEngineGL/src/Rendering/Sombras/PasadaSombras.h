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
#ifndef PASADASOMBRAS_H
#define PASADASOMBRAS_H

#include <glm/glm.hpp>
#include <vector>

#include "MapaSombras.h"

class GameObject;
class MeshRenderer;
struct LightData;
template <typename T>
class ListaDE;

namespace Rendering {

// Pasada de sombras de un frame: elige las luces que proyectan sombra (las que
// tienen el componente Sombras activo), prepara sus mapas, dibuja la geometria
// desde cada luz y deja los proyectores listos para el shader de mallas.
//
// Se hace una vez por frame (el mapa depende de la luz, no de la camara) y
// antes de las vistas previas, para que todas las pasadas vean las mismas
// sombras. La geometria que proyecta es la de los objetos con Model.
class PasadaSombras {
public:
    struct Entrada {
        const LightData* luces = nullptr;
        // Objeto duenio de cada luz (mismo indice que luces).
        GameObject* const* duenos = nullptr;
        int cantidadLuces = 0;
        ListaDE<GameObject*>* objetos = nullptr;
        // Camara activa: la luz direccional se encuadra delante de ella.
        bool hayCamara = false;
        glm::vec3 posicionCamara{0.f};
        glm::vec3 frenteCamara{0.f, 0.f, -1.f};
        // Framebuffer de la ventana: se restaura al terminar.
        int framebufferAncho = 0;
        int framebufferAlto = 0;
    };

    explicit PasadaSombras(Backend::IRenderBackend& backend);

    // Ejecuta la pasada y entrega los proyectores al MeshRenderer.
    void ejecutar(const Entrada& entrada, MeshRenderer& meshRenderer);

    // Proyectores con mapa del ultimo frame (los que consulta el shader).
    const std::vector<ProyectorSombra>& proyectores() const { return conMapa_; }

private:
    void recogerCandidatos(const Entrada& entrada);
    void dibujarGeometria(const Entrada& entrada, MeshRenderer& meshRenderer,
                          const glm::mat4& matriz, const ProyectorSombra& proyector);

    Backend::IRenderBackend& backend_;
    MapaSombras mapas_;
    std::vector<CandidatoProyector> candidatos_;
    std::vector<ProyectorSombra> elegidos_;
    std::vector<ProyectorSombra> conMapa_;
};

}  // namespace Rendering

#endif
