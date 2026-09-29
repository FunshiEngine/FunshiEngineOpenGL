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
#ifndef RENOMBRARELEMENTO_H
#define RENOMBRARELEMENTO_H

#include <cstring>
#include <string>

#include "../../Events/EditorEventBus.h"
#include "../../FileManager/FileManager.h"
#include "../../Herramientas/PathUtils.h"
#include <imgui.h>

// Renombre por click derecho, COMPARTIDO por el grid (ShowFolder) y el arbol
// (BrowseFile): un solo modal y un solo camino de disco, para que renombrar un
// elemento se comporte igual se haga desde el panel que se haga.
//
// Renombrar NO cambia de carpeta: solo el ultimo tramo de la ruta. El disco lo
// hace FileManager (que rechaza separadores en el nombre) y despues hay que
// avisar al resto del editor — ArchivosReubicados — porque la escena guarda
// mallas, texturas y fuentes de script POR RUTA: sin ese aviso, renombrar un
// asset dejaria la escena apuntando a la ruta vieja.
//
// Quien llama sigue siendo dueno de su estado de navegacion: si el renombre
// toca la carpeta visible o una rama expandida, tiene que ajustar
// FileSelection::rutaVisible (y, en el arbol, las rutas de `openPaths`) ANTES
// del rescaneo; el arbol ademas sube contadorCambios para reconstruirse con el
// nombre nuevo.
namespace RenombrarElemento {

// Ruta del elemento tras cambiarle el nombre: misma carpeta, ultimo tramo
// nuevo. Conserva los separadores que ya traia la ruta (en Windows el motor
// mezcla '/' y '\'), y no depende del separador nativo: en Linux '\' es un
// caracter valido de un nombre de archivo.
inline std::string rutaConNombreNuevo(const std::string& ruta,
                                      const std::string& nombreNuevo) {
    std::string::size_type corte = ruta.size();
    while (corte > 0 && !esSeparadorPath(ruta[corte - 1])) --corte;
    // `corte` queda en "carpeta + separador" (0 si la ruta era un nombre suelto).
    return ruta.substr(0, corte) + nombreNuevo;
}

// Avisa que una ruta cambio de sitio. Su suscriptor reescribe las referencias
// de la escena que caian bajo la ruta anterior y persiste la escena; sin bus
// (panel al que no se le inyecto) simplemente no hay aviso.
inline void publicarReubicacion(EditorEventBus* bus,
                                const std::string& rutaAnterior,
                                const std::string& rutaNueva) {
    if (bus == nullptr) return;
    EditorEvent ev;
    ev.type = EditorEventType::ArchivosReubicados;
    ev.rutaAnterior = rutaAnterior;
    ev.rutaNueva = rutaNueva;
    bus->publish(ev);
}

// Renombra en disco y publica el aviso. Devuelve false —sin tocar nada— si el
// nombre viene vacio, si trae separadores (FileManager lo rechaza) o si es el
// mismo de antes, que no seria un cambio.
inline bool ejecutar(FileManager* fileManager, EditorEventBus* bus,
                     const std::string& ruta, const std::string& nombreNuevo) {
    if (fileManager == nullptr || nombreNuevo.empty()) return false;
    const std::string rutaNueva = rutaConNombreNuevo(ruta, nombreNuevo);
    if (rutaNueva == ruta) return false;
    if (!fileManager->renombrar(ruta, nombreNuevo)) return false;
    publicarReubicacion(bus, ruta, rutaNueva);
    return true;
}

// Lo confirmado por el modal: quien llama ejecuta el renombre con `ruta` /
// `nombreNuevo` y actualiza su propio estado de navegacion segun `esCarpeta`.
struct Resultado {
    bool confirmado = false;
    std::string ruta;
    std::string nombreNuevo;
    bool esCarpeta = false;
};

// Estado + dibujo del modal de renombre. El panel registra el elemento desde el
// menu contextual (solicitar) y dibuja el modal junto a sus otros modales; el
// campo se enfoca al abrirse, para poder escribir sin un clic extra (la edicion
// en la fila anterior no se enfocaba y parecia no responder al teclado).
class Modal {
public:

    // Registra el elemento a renombrar; se abre en el dibujar() siguiente.
    void solicitar(const std::string& rutaCompleta, bool esCarpeta,
                   const std::string& nombreActual) {
        ruta_ = rutaCompleta;
        esCarpeta_ = esCarpeta;
        abrir_ = true;
        std::memset(buffer_, 0, sizeof(buffer_));
        std::strncpy(buffer_, nombreActual.c_str(), sizeof(buffer_) - 1);
    }

    bool activo() const noexcept { return !ruta_.empty(); }

    // Dibuja el modal si hay un elemento registrado. El resultado viene
    // confirmado SOLO en el frame de la confirmacion; el resto de los frames
    // (o si el modal se cerro sin confirmar) devuelve uno vacio.
    Resultado dibujar() {
        Resultado resultado;
        if (ruta_.empty()) return resultado;
        if (abrir_) {
            ImGui::OpenPopup("Renombrar");
            abrir_ = false;
        }
        if (!ImGui::BeginPopupModal("Renombrar", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
            // No hay popup que editar (lo cerro el usuario): se descarta el
            // pedido en vez de dejarlo colgado esperando un dibujar() que ya no
            // puede volver a abrirlo.
            ruta_.clear();
            return resultado;
        }

        ImGui::Text("Nuevo nombre del %s:", esCarpeta_ ? "folder" : "archivo");
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        const bool enter = ImGui::InputText(
            "##renombrarElemento", buffer_, IM_ARRAYSIZE(buffer_),
            ImGuiInputTextFlags_AutoSelectAll |
                ImGuiInputTextFlags_EnterReturnsTrue);
        const bool confirmar =
            ImGui::Button("Renombrar", ImVec2(120, 0)) || enter;
        ImGui::SameLine();
        const bool cancelar = ImGui::Button("Cancelar", ImVec2(120, 0)) ||
                              ImGui::IsKeyPressed(ImGuiKey_Escape);

        if (confirmar) {
            resultado.confirmado = true;
            resultado.ruta = ruta_;
            resultado.nombreNuevo = buffer_;
            resultado.esCarpeta = esCarpeta_;
        }
        if (confirmar || cancelar) {
            ruta_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return resultado;
    }

private:
    std::string ruta_;
    bool esCarpeta_ = false;
    bool abrir_ = false;
    char buffer_[256] = "";
};

} // namespace RenombrarElemento

#endif
