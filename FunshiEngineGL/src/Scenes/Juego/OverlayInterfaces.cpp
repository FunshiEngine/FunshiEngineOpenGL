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
#include "OverlayInterfaces.h"

#include <cstdint>

#include <imgui.h>

#include "InterfacesJuego.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../../WebUI/WebUIRuntime.h"
#include "../../Localizacion/Localizacion.h"

using Localizacion::Texto;

namespace {

ImTextureID aTextura(void* handle) {
    return static_cast<ImTextureID>(reinterpret_cast<std::intptr_t>(handle));
}

// Reenvia los bordes de los tres botones y la rueda (coordenadas ya locales).
void reenviarRaton(VistaWebUI& vista, int x, int y) {
    WebUIRuntime::ratonMover(vista, x, y);
    for (int boton = 0; boton < 3; ++boton) {
        if (ImGui::IsMouseClicked(boton)) WebUIRuntime::ratonBoton(vista, boton, true);
        if (ImGui::IsMouseReleased(boton)) WebUIRuntime::ratonBoton(vista, boton, false);
    }
    const float rueda = ImGui::GetIO().MouseWheel;
    if (rueda != 0.0f) WebUIRuntime::ratonRueda(vista, rueda);
}

}  // namespace

void OverlayInterfaces::soltarFoco() {
    if (vistaFoco_) WebUIRuntime::foco(*vistaFoco_, false);
    foco_ = nullptr;
    vistaFoco_ = nullptr;
}

void OverlayInterfaces::pintarHud(ListaDE<GameObject*>* objetos, EstadoInterfaces* estado,
                                  float deltaTime, bool recibeInput) {
    const std::vector<InterfaceComponent*> interfaces =
        InterfacesJuego::recolectarDeEscena(objetos);
    const ImGuiIO& io = ImGui::GetIO();
    hud_.sincronizar(interfaces, static_cast<int>(io.DisplaySize.x),
                     static_cast<int>(io.DisplaySize.y), deltaTime, estado);
    // La vista con foco pudo descargarse (su interfaz se borro o se apago):
    // el puntero ya no es valido y no hay a quien avisar.
    if (foco_ && !hud_.buscar(foco_)) {
        foco_ = nullptr;
        vistaFoco_ = nullptr;
    }

    rects_.assign(interfaces.size(), RectInterfaz());
    ImDrawList* dibujo = ImGui::GetForegroundDrawList();
    for (std::size_t i = 0; i < interfaces.size(); ++i) {
        const InterfaceComponent* interfaz = interfaces[i];
        const VistaWebUI* vista = hud_.buscar(interfaz);
        if (!vista || vista->ancho <= 0 || vista->alto <= 0) continue;
        const glm::vec2 d = interfaz->getDesplazamiento();
        rects_[i] = ColocacionInterfaz::calcular(
            interfaz->getAncla(), d.x, d.y, static_cast<float>(vista->ancho),
            static_cast<float>(vista->alto), io.DisplaySize.x, io.DisplaySize.y);
        if (!vista->handleImGui) continue;
        const RectInterfaz& r = rects_[i];
        dibujo->AddImage(aTextura(vista->handleImGui), ImVec2(r.x, r.y),
                         ImVec2(r.x + r.ancho, r.y + r.alto));
    }

    if (recibeInput)
        enrutarRaton(interfaces);
    else
        soltarFoco();
}

void OverlayInterfaces::enrutarRaton(const std::vector<InterfaceComponent*>& interfaces) {
    const ImVec2 raton = ImGui::GetIO().MousePos;
    // Recorrido de arriba hacia abajo: la lista viene ordenada por z creciente.
    for (std::size_t k = interfaces.size(); k-- > 0;) {
        VistaWebUI* vista = hud_.buscar(interfaces[k]);
        const RectInterfaz& r = rects_[k];
        if (!vista || !vista->cargada || !r.contiene(raton.x, raton.y)) continue;
        if (foco_ != interfaces[k]) {
            soltarFoco();
            foco_ = interfaces[k];
            vistaFoco_ = vista;
            WebUIRuntime::foco(*vista, true);
        }
        reenviarRaton(*vista, static_cast<int>(raton.x - r.x), static_cast<int>(raton.y - r.y));
        return;
    }
    soltarFoco();
}

void OverlayInterfaces::tecla(int codigo, int action) {
    if (!vistaFoco_) return;
    WebUIRuntime::tecla(*vistaFoco_, codigo, action != 0, action == 2);
}

void OverlayInterfaces::pintarVistaPrevia(GameObject* seleccionado, EstadoInterfaces* estado,
                                          float deltaTime, const char* nombreVentana) {
    InterfaceComponent* interfaz =
        seleccionado ? seleccionado->getComponent<InterfaceComponent>() : nullptr;
    if (!interfaz || interfaz->getRutaHtml().empty()) {
        // Sin interfaz que mostrar se suelta la vista que hubiera.
        vistaPrevia_.vaciar();
        previaConFoco_ = false;
        return;
    }
    const ImGuiIO& io = ImGui::GetIO();
    const std::vector<InterfaceComponent*> una{interfaz};
    vistaPrevia_.sincronizar(una, static_cast<int>(io.DisplaySize.x),
                             static_cast<int>(io.DisplaySize.y), deltaTime, estado);

    if (!ImGui::Begin(nombreVentana)) {
        ImGui::End();
        return;
    }
    VistaWebUI* vista = vistaPrevia_.buscar(interfaz);
    if (!vista || !vista->cargada) {
        ImGui::Text(Localizacion::tr(Texto::Cargando), interfaz->getRutaHtml().c_str());
    } else if (!vista->error.empty()) {
        ImGui::Text(Localizacion::tr(Texto::Error), vista->error.c_str());
    } else if (!vista->handleImGui || vista->ancho <= 0 || vista->alto <= 0) {
        ImGui::Text(Localizacion::tr(Texto::EsperandoTexturaDelMotorWeb), interfaz->getRutaHtml().c_str());
        ImGui::TextDisabled("%s", Localizacion::tr(Texto::SinBackendWebRealCEF));
    } else {
        const float disponible = ImGui::GetContentRegionAvail().x;
        const float escala =
            disponible > 0 ? disponible / static_cast<float>(vista->ancho) : 1.0f;
        const ImVec2 tamano(static_cast<float>(vista->ancho) * escala,
                            static_cast<float>(vista->alto) * escala);
        const ImVec2 origen = ImGui::GetCursorScreenPos();
        ImGui::Image(aTextura(vista->handleImGui), tamano);
        // El foco sigue al mouse: sobre la imagen la vista recibe el input y lo
        // pierde al salir (antes nunca se soltaba).
        const bool encima = ImGui::IsItemHovered();
        if (encima != previaConFoco_) {
            WebUIRuntime::foco(*vista, encima);
            previaConFoco_ = encima;
        }
        if (encima) {
            const ImVec2 raton = io.MousePos;
            reenviarRaton(*vista, static_cast<int>((raton.x - origen.x) / escala),
                          static_cast<int>((raton.y - origen.y) / escala));
        }
    }
    ImGui::End();
}

void OverlayInterfaces::descargarHud() {
    soltarFoco();
    hud_.vaciar();
    rects_.clear();
}

void OverlayInterfaces::vaciar() {
    soltarFoco();
    hud_.vaciar();
    vistaPrevia_.vaciar();
    rects_.clear();
    previaConFoco_ = false;
}
