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
#include "DropEscena.h"

#include <iostream>
#include <string>

#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>

#include "ArrastreArchivos.h"
#include "EditorController.h"
#include "GizmoController.h"
#include "RayoPantalla.h"
#include "../../Objetos/Componentes/Escena/CameraComponent.h"

namespace DropEscena {

namespace {

// Distancia a la que se crea el objeto si el rayo no corta el plano de juego
// (por ejemplo, mirando al cielo).
constexpr float kDistanciaSinPlano = 10.f;

bool puntoBajoMouse(CameraComponent* camara, bool modo2D, glm::vec3& punto) {
    if (!camara) return false;
    const ImGuiIO& io = ImGui::GetIO();
    float vista[16], proyeccion[16];
    camara->getViewMatrix(vista);
    camara->getProjectionMatrix(proyeccion, io.DisplaySize.x / io.DisplaySize.y);
    Rayo rayo;
    if (!RayoPantalla::desdePixel(io.MousePos.x, io.MousePos.y, io.DisplaySize.x,
                                  io.DisplaySize.y, glm::make_mat4(vista),
                                  glm::make_mat4(proyeccion), rayo))
        return false;
    const glm::vec3 normal = modo2D ? glm::vec3(0.f, 0.f, 1.f) : glm::vec3(0.f, 1.f, 0.f);
    if (!RayoPantalla::cortarPlano(rayo, normal, 0.f, punto))
        punto = rayo.origen + rayo.direccion * kDistanciaSinPlano;
    return true;
}

bool esScript(TipoArchivo tipo) {
    return tipo == TipoArchivo::ScriptCpp || tipo == TipoArchivo::ScriptJava;
}

}  // namespace

void procesar(EditorController& editor, CameraComponent* camara,
              ListaDE<GameObject*>* objetos, GizmoController* picking, bool modo2D) {
    const ImGuiPayload* payload = ImGui::GetDragDropPayload();
    if (!payload || !payload->IsDataType("ARCHIVO_PATH") || !payload->Data) return;
    // Sobre una ventana decide la ventana (sus destinos propios).
    if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow)) return;

    const std::string ruta(static_cast<const char*>(payload->Data));
    const TipoArchivo tipo = TiposArchivo::clasificar(ruta);
    const bool aceptado = ArrastreArchivos::aceptaEnEscena(tipo);

    const ImGuiIO& io = ImGui::GetIO();
    GameObject* debajo =
        picking ? picking->pickObject(io.MousePos.x, io.MousePos.y, camara, objetos) : nullptr;
    const bool aplicarAObjeto = debajo && esScript(tipo);

    // Indicacion junto al cursor mientras se arrastra.
    ImDrawList* dibujo = ImGui::GetForegroundDrawList();
    const std::string texto =
        !aceptado ? std::string("No se puede soltar: tipo no soportado")
        : aplicarAObjeto ? std::string("Agregar ") + TiposArchivo::nombre(tipo) + " al objeto"
                         : std::string("Crear objeto: ") + TiposArchivo::nombre(tipo);
    const ImVec2 posicion(io.MousePos.x + 18.f, io.MousePos.y + 30.f);
    const ImVec2 medida = ImGui::CalcTextSize(texto.c_str());
    dibujo->AddRectFilled(ImVec2(posicion.x - 6.f, posicion.y - 4.f),
                          ImVec2(posicion.x + medida.x + 6.f, posicion.y + medida.y + 4.f),
                          ImGui::GetColorU32(ImGuiCol_PopupBg), 4.f);
    dibujo->AddText(posicion, ImGui::GetColorU32(aceptado ? ImGuiCol_Text : ImGuiCol_TextDisabled),
                    texto.c_str());

    if (!aceptado || !ImGui::IsMouseReleased(ImGuiMouseButton_Left)) return;

    ArrastreArchivos::Resultado resultado;
    if (aplicarAObjeto) {
        resultado = ArrastreArchivos::aplicarAObjeto(editor, *debajo, ruta);
    } else {
        ArrastreArchivos::Destino destino;
        destino.conPosicion = puntoBajoMouse(camara, modo2D, destino.posicion);
        resultado = ArrastreArchivos::crearDesdeArchivo(editor, ruta, destino);
    }
    if (resultado.ok && resultado.objeto)
        editor.selectObject(resultado.objeto);
    else
        std::cerr << "[Escena] No se pudo usar " << ruta << ": " << resultado.mensaje << '\n';
}

}  // namespace DropEscena
