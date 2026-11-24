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
#include "SettingsUIComponent.h"

#include "../../../Objetos/Componentes/Interfaz/UIComponent.h"
#include "../../../Objetos/GameObject.h"

#include <imgui.h>

#include <cctype>
#include <cstring>
#include <string>
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

namespace {

// Opciones del combo de tipo, en el orden del enum. Se traducen al dibujar.
constexpr Texto TEXTOS_TIPO[] = {Texto::Panel, Texto::Etiqueta, Texto::Imagen, Texto::Boton};

// Opciones del combo de ancla, en el orden del enum. Se traducen al dibujar.
constexpr Texto TEXTOS_ANCLA[] = {
    Texto::ArribaIzquierda, Texto::ArribaCentro, Texto::ArribaDerecha,
    Texto::MedioIzquierda,  Texto::Centro,       Texto::MedioDerecha,
    Texto::AbajoIzquierda,  Texto::AbajoCentro,  Texto::AbajoDerecha};

constexpr int TOTAL_TIPOS = 4;
constexpr int TOTAL_ANCLAS = 9;

// Textos de una lista de opciones en el idioma vigente (para ImGui::Combo).
template <std::size_t N>
void traducirOpciones(const Texto (&textos)[N], const char* (&salida)[N]) {
    for (std::size_t i = 0; i < N; ++i) salida[i] = Localizacion::tr(textos[i]);
}

// Tope de los campos de texto del inspector.
constexpr std::size_t kCapacidadTexto = 256;
constexpr std::size_t kCapacidadId = 64;

// Extensiones de imagen que el HUD puede usar como textura de un widget.
bool esImagen(const std::string& ruta) {
    static const char* extensiones[] = {".png", ".jpg", ".jpeg",
                                        ".bmp", ".tga", ".gif"};
    std::string final = ruta;
    for (char& caracter : final) {
        caracter =
            static_cast<char>(std::tolower(static_cast<unsigned char>(caracter)));
    }
    for (const char* extension : extensiones) {
        const std::string terminacion(extension);
        if (final.size() >= terminacion.size() &&
            final.compare(final.size() - terminacion.size(), terminacion.size(),
                          terminacion) == 0) {
            return true;
        }
    }
    return false;
}

// Muestra un campo de texto y deja en `salida` lo que se escribio. Devuelve
// true solo si el usuario edito el campo.
bool editarTexto(const char* etiqueta, std::string* salida,
                  std::size_t capacidad) {
    char buffer[kCapacidadTexto] = {0};
    const std::size_t copiado =
        salida->size() < capacidad - 1 ? salida->size() : capacidad - 1;
    std::memcpy(buffer, salida->data(), copiado);
    buffer[copiado] = '\0';
    if (!ImGui::InputText(etiqueta, buffer, capacidad)) return false;
    *salida = buffer;
    return true;
}

} // namespace

SettingsUIComponent::SettingsUIComponent(GameObject* gameObject)
    : SettingsComponent(demangle(typeid(UIComponent).name())),
      gameObject(gameObject) {}

void SettingsUIComponent::showDataComponent() {
    UIComponent* comp = objetivo<UIComponent>(gameObject);
    if (!comp) return;

    ImGui::TextWrapped(
        "%s", Localizacion::tr(Texto::WidgetsQueDibujaElMotor));

    // Origen de arrastre: lleva el nombre del objeto dueno de la interfaz
    // para asignarla a un campo TagTipo::Componente de un script.
    const std::string etiquetaDrag =
        std::string("Interfaz de: ") +
        (gameObject->inputName[0] != '\0' ? gameObject->inputName
                                          : "(objeto)");
    ImGui::Button(etiquetaDrag.c_str());
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip(
            "%s", Localizacion::tr(Texto::ArrastraAUnCampoDe));
    if (ImGui::BeginDragDropSource()) {
        const char* dueno = gameObject->inputName;
        ImGui::SetDragDropPayload("COMPONENTE_INTERFAZ", dueno,
                                  std::strlen(dueno) + 1);
        ImGui::TextUnformatted(Localizacion::tr(Texto::AsignarInterfazAUnScript));
        ImGui::EndDragDropSource();
    }

    bool activo = comp->estaActivo();
    if (ImGui::Checkbox(Localizacion::etiqueta(Texto::Activa).c_str(), &activo)) comp->setActivo(activo);

    float z = comp->getZ();
    if (ImGui::DragFloat("Z", &z, 0.1f)) comp->setZ(z);

    ImGui::Separator();

    LienzoUI& lienzo = comp->lienzo();

    if (ImGui::Button(Localizacion::etiqueta(Texto::AnadirWidget).c_str())) ImGui::OpenPopup("Anadir widget");

    if (ImGui::BeginPopup("Anadir widget")) {
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Panel).c_str())) {
            WidgetUI widget;
            widget.tipo = TipoWidgetUI::Panel;
            widget.tamano = {240.0f, 160.0f};
            widget.color = {0.15f, 0.15f, 0.18f, 0.85f};
            lienzo.agregar(widget);
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Etiqueta).c_str())) {
            WidgetUI widget;
            widget.tipo = TipoWidgetUI::Etiqueta;
            widget.tamano = {200.0f, 28.0f};
            widget.texto = "Texto";
            lienzo.agregar(widget);
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Imagen).c_str())) {
            WidgetUI widget;
            widget.tipo = TipoWidgetUI::Imagen;
            widget.tamano = {128.0f, 128.0f};
            lienzo.agregar(widget);
        }
        if (ImGui::MenuItem(Localizacion::etiqueta(Texto::Boton).c_str())) {
            WidgetUI widget;
            widget.tipo = TipoWidgetUI::Boton;
            widget.tamano = {160.0f, 36.0f};
            widget.color = {0.20f, 0.35f, 0.55f, 1.0f};
            widget.texto = "Boton";
            lienzo.agregar(widget);
        }
        ImGui::EndPopup();
    }

    ImGui::Separator();

    const std::size_t cantidad = lienzo.widgets().size();
    for (std::size_t i = 0; i < cantidad; ++i) {
        // Copia local: los cambios se aplican al final, para que el lienzo no
        // cambie mientras se esta editando.
        WidgetUI widget = lienzo.widgets()[i];

        ImGui::PushID(static_cast<int>(i));
        std::string titulo = widget.texto.empty() ? widget.id : widget.texto;
        if (titulo.empty()) titulo = Localizacion::tr(TEXTOS_TIPO[static_cast<int>(widget.tipo)]);

        const bool abierto = ImGui::CollapsingHeader(titulo.c_str());
        if (abierto) {
            editarTexto(Localizacion::etiqueta(Texto::Identificador).c_str(), &widget.id, kCapacidadId);

            const char* nombresTipo[TOTAL_TIPOS];
            traducirOpciones(TEXTOS_TIPO, nombresTipo);
            int tipo = static_cast<int>(widget.tipo);
            if (ImGui::Combo(Localizacion::etiqueta(Texto::Tipo).c_str(), &tipo, nombresTipo, TOTAL_TIPOS))
                widget.tipo = static_cast<TipoWidgetUI>(tipo);

            const char* nombresAncla[TOTAL_ANCLAS];
            traducirOpciones(TEXTOS_ANCLA, nombresAncla);
            int ancla = static_cast<int>(widget.ancla);
            if (ImGui::Combo(Localizacion::etiqueta(Texto::Ancla).c_str(), &ancla, nombresAncla, TOTAL_ANCLAS))
                widget.ancla = static_cast<AnclaUI>(ancla);

            float desplazamiento[2] = {widget.desplazamiento.x,
                                       widget.desplazamiento.y};
            if (ImGui::DragFloat2(Localizacion::etiqueta(Texto::Desplazamiento).c_str(), desplazamiento, 1.0f)) {
                widget.desplazamiento.x = desplazamiento[0];
                widget.desplazamiento.y = desplazamiento[1];
            }

            float tamano[2] = {widget.tamano.x, widget.tamano.y};
            if (ImGui::DragFloat2(Localizacion::etiqueta(Texto::Tamano).c_str(), tamano, 1.0f)) {
                widget.tamano.x = tamano[0];
                widget.tamano.y = tamano[1];
            }

            float color[4] = {widget.color.r, widget.color.g, widget.color.b,
                              widget.color.a};
            if (ImGui::ColorEdit4(Localizacion::etiqueta(Texto::Color).c_str(), color)) {
                widget.color = {color[0], color[1], color[2], color[3]};
            }

            float colorTexto[4] = {widget.colorTexto.r, widget.colorTexto.g,
                                   widget.colorTexto.b, widget.colorTexto.a};
            if (ImGui::ColorEdit4(Localizacion::etiqueta(Texto::ColorDelTexto).c_str(), colorTexto)) {
                widget.colorTexto = {colorTexto[0], colorTexto[1],
                                     colorTexto[2], colorTexto[3]};
            }

            if (widget.tipo == TipoWidgetUI::Etiqueta ||
                widget.tipo == TipoWidgetUI::Boton) {
                editarTexto(Localizacion::etiqueta(Texto::TextoDelWidget).c_str(), &widget.texto, kCapacidadTexto);
                float tamanoTexto = widget.tamanoTexto;
                if (ImGui::DragFloat(Localizacion::etiqueta(Texto::TamanoDelTexto).c_str(), &tamanoTexto, 1.0f))
                    widget.tamanoTexto = tamanoTexto;
            }

            if (widget.tipo == TipoWidgetUI::Imagen) {
                const std::string& ruta = widget.rutaTextura;
                const char* texto =
                    ruta.empty() ? "[Arrastra una imagen]" : ruta.c_str();
                const float ancho = ImGui::CalcTextSize(texto).x +
                                    ImGui::GetStyle().FramePadding.x * 2.0f;
                ImGui::SetNextItemWidth(ancho);
                ImGui::Button(texto);
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload =
                            ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                        const char* rutaSoltada =
                            static_cast<const char*>(payload->Data);
                        if (esImagen(rutaSoltada))
                            widget.rutaTextura = rutaSoltada;
                    }
                    ImGui::EndDragDropTarget();
                }
            }

            bool visible = widget.visible;
            if (ImGui::Checkbox(Localizacion::etiqueta(Texto::Visible).c_str(), &visible)) widget.visible = visible;

            ImGui::Separator();
            // El orden de la lista es el orden de apilado: el ultimo se dibuja
            // encima.
            ImGui::BeginDisabled(i == 0);
            if (ImGui::Button(Localizacion::etiqueta(Texto::Subir).c_str())) lienzo.mover(i, i - 1);
            ImGui::EndDisabled();
            ImGui::SameLine();
            ImGui::BeginDisabled(i + 1 >= cantidad);
            if (ImGui::Button(Localizacion::etiqueta(Texto::Bajar).c_str())) lienzo.mover(i, i + 1);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button(Localizacion::etiqueta(Texto::QuitarWidget).c_str())) {
                lienzo.quitar(i);
                ImGui::PopID();
                break; // la lista cambio: se vuelve a dibujar en el proximo frame
            }
        }
        lienzo.actualizar(i, widget);
        ImGui::PopID();
    }
}
