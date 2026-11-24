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
#include "HerramientaTilemap.h"

#include <memory>
#include <optional>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "RayoPantalla.h"
#include "../Registro/SceneRegistry.h"
#include "../../Comandos/GestorComandos.h"
#include "../../Comandos/Tilemap/PintarTilesComando.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Escena/CameraComponent.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/Componentes/Juego2D/Tilemap.h"

namespace {

struct Matrices {
    glm::mat4 vistaProyeccion{1.f};
    glm::mat4 modelo{1.f};
    Rayo rayoLocal;
    bool rayoValido = false;
};

bool calcular(CameraComponent* camara, GameObject* objeto, Matrices& m) {
    Transform* global = objeto->getGlobalTransform();
    if (!camara || !global) return false;
    const ImGuiIO& io = ImGui::GetIO();
    if (io.DisplaySize.x <= 0.f || io.DisplaySize.y <= 0.f) return false;
    float vista[16], proyeccion[16], modelo[16];
    camara->getViewMatrix(vista);
    camara->getProjectionMatrix(proyeccion, io.DisplaySize.x / io.DisplaySize.y);
    buildMatrixFromTransform(global, modelo);
    m.modelo = glm::make_mat4(modelo);
    m.vistaProyeccion = glm::make_mat4(proyeccion) * glm::make_mat4(vista);

    Rayo rayo;
    if (!RayoPantalla::desdePixel(io.MousePos.x, io.MousePos.y, io.DisplaySize.x,
                                  io.DisplaySize.y, glm::make_mat4(vista),
                                  glm::make_mat4(proyeccion), rayo))
        return true;
    const glm::mat4 inversa = glm::inverse(m.modelo);
    m.rayoLocal.origen = glm::vec3(inversa * glm::vec4(rayo.origen, 1.f));
    const glm::vec3 dir = glm::vec3(inversa * glm::vec4(rayo.direccion, 0.f));
    if (glm::length(dir) < 1e-6f) return true;
    m.rayoLocal.direccion = glm::normalize(dir);
    m.rayoValido = true;
    return true;
}

bool aPantalla(const Matrices& m, glm::vec3 local, ImVec2& salida) {
    const glm::vec4 clip = m.vistaProyeccion * m.modelo * glm::vec4(local, 1.f);
    if (clip.w <= 1e-6f) return false;
    const ImGuiIO& io = ImGui::GetIO();
    salida.x = (clip.x / clip.w * 0.5f + 0.5f) * io.DisplaySize.x;
    salida.y = (1.f - (clip.y / clip.w * 0.5f + 0.5f)) * io.DisplaySize.y;
    return true;
}

// Contorno del rectangulo de celdas [x0, x1] x [y0, y1] en local.
void dibujarCeldas(const Matrices& m, int x0, int y0, int x1, int y1, ImU32 color,
                   float grosor) {
    const float ax = static_cast<float>(x0), bx = static_cast<float>(x1 + 1);
    const float ay = -static_cast<float>(y0), by = -static_cast<float>(y1 + 1);
    ImVec2 p[4];
    if (!aPantalla(m, {ax, ay, 0.f}, p[0]) || !aPantalla(m, {bx, ay, 0.f}, p[1]) ||
        !aPantalla(m, {bx, by, 0.f}, p[2]) || !aPantalla(m, {ax, by, 0.f}, p[3]))
        return;
    ImGui::GetForegroundDrawList()->AddPolyline(p, 4, color, ImDrawFlags_Closed, grosor);
}

}  // namespace

void HerramientaTilemap::setActiva(bool valor) noexcept {
    activa_ = valor;
    if (!valor) {
        trazando_ = false;
        trazo_.clear();
    }
}

void HerramientaTilemap::terminarTrazo(GameObject* objeto, SceneRegistry* escena,
                                       GestorComandos* comandos) {
    trazando_ = false;
    if (!trazo_.empty() && objeto && comandos) {
        comandos->ejecutar(std::make_unique<PintarTilesComando>(escena, objeto->getId(),
                                                                std::move(trazo_)));
    }
    trazo_.clear();
}

bool HerramientaTilemap::procesar(CameraComponent* camara, GameObject* seleccionado,
                                  SceneRegistry* escena, GestorComandos* comandos) {
    Tilemap* tilemap = seleccionado ? seleccionado->getComponent<Tilemap>() : nullptr;
    if (!activa_ || !tilemap) {
        if (trazando_) terminarTrazo(nullptr, escena, comandos);
        return false;
    }
    // Si se cambio de objeto a mitad de un trazo, el trazo queda en el anterior.
    if (trazando_ && seleccionado->getId() != idObjetoTrazo_) {
        GameObject* anterior = escena ? escena->getObjectByID(idObjetoTrazo_) : nullptr;
        terminarTrazo(anterior, escena, comandos);
    }

    // Escape sale del modo de pintado (fuera de un campo de texto).
    if (!ImGui::GetIO().WantCaptureKeyboard && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (trazando_) terminarTrazo(seleccionado, escena, comandos);
        setActiva(false);
        return false;
    }

    Matrices m;
    if (!calcular(camara, seleccionado, m)) return false;

    const CapaTilemap* capaActiva = tilemap->capa(capa_);
    if (capaActiva)
        dibujarCeldas(m, 0, 0, capaActiva->ancho - 1, capaActiva->alto - 1,
                      ImGui::GetColorU32(ImGuiCol_Border), 1.f);

    const ImGuiIO& io = ImGui::GetIO();
    // Sobre una ventana del editor manda la ventana (salvo que el trazo ya
    // haya empezado en la escena).
    if (io.WantCaptureMouse && !trazando_) return false;

    std::optional<CeldaTilemap> celda;
    glm::vec3 punto;
    if (m.rayoValido &&
        RayoPantalla::cortarPlano(m.rayoLocal, glm::vec3(0.f, 0.f, 1.f), 0.f, punto))
        celda = PincelTilemap::celdaDesdeLocal(*tilemap, capa_, punto.x, punto.y);

    const bool borrar = modo_ == Modo::Borrador || io.KeyShift;
    const std::uint32_t id = borrar ? 0u : tile_;
    const ImU32 colorCursor = ImGui::GetColorU32(borrar ? ImGuiCol_PlotHistogram
                                                        : ImGuiCol_CheckMark);

    if (celda && modo_ == Modo::Rectangulo && trazando_)
        dibujarCeldas(m, inicioRect_.x, inicioRect_.y, celda->x, celda->y, colorCursor, 2.f);
    else if (celda)
        dibujarCeldas(m, celda->x, celda->y, celda->x, celda->y, colorCursor, 2.f);

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && celda) {
        trazando_ = true;
        idObjetoTrazo_ = seleccionado->getId();
        trazo_.clear();
        inicioRect_ = *celda;
        if (modo_ == Modo::Relleno) {
            trazo_ = PincelTilemap::rellenar(*tilemap, capa_, *celda, id);
            terminarTrazo(seleccionado, escena, comandos);
            return true;
        }
    }

    if (trazando_ && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        if (celda && modo_ != Modo::Rectangulo) {
            std::vector<CambioTile> cambios =
                PincelTilemap::pintar(*tilemap, capa_, *celda, id);
            trazo_.insert(trazo_.end(), cambios.begin(), cambios.end());
        }
        return true;
    }

    if (trazando_) {
        // Se solto el boton: el rectangulo se aplica al soltar.
        if (modo_ == Modo::Rectangulo && celda) {
            std::vector<CambioTile> cambios =
                PincelTilemap::rectangulo(*tilemap, capa_, inicioRect_, *celda, id);
            trazo_.insert(trazo_.end(), cambios.begin(), cambios.end());
        }
        terminarTrazo(seleccionado, escena, comandos);
        return true;
    }
    // Sin trazo: el clic sobre la escena fuera del tilemap tampoco selecciona
    // (mientras se pinta no se quiere perder la seleccion por un clic errado).
    return ImGui::IsMouseClicked(ImGuiMouseButton_Left);
}
