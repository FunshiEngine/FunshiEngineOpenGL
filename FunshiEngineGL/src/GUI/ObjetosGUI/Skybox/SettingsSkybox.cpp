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
#include "SettingsSkybox.h"

#include "../../../Assets/StbImageLoader.h"
#include "../../../Objetos/Componentes/Skybox.h"
#include "../../../Objetos/GameObject.h"

#include <cstring>

#include <imgui.h>

SettingsSkybox::SettingsSkybox(GameObject* gameObject) : gameObject(gameObject) {}

void SettingsSkybox::sondearDimensiones(
    const std::string rutas[SelectorArchivoCubemap::kCaras]) {
    for (int i = 0; i < SelectorArchivoCubemap::kCaras; ++i) {
        if (rutas[i] == rutasMedidas_[i]) continue;
        // Una cara que pasa a estar sin ruta debe quedarse sin dimensiones: si
        // se conservaran las de la ruta anterior, al reasignarla se compararia
        // contra la imagen de antes.
        if (rutas[i].empty()) {
            dims_[i] = SelectorArchivoCubemap::Dimensiones{};
        } else {
            SelectorArchivoCubemap::Dimensiones d;
            if (!StbImageLoader::dimensiones(rutas[i], d.ancho, d.alto)) {
                d = SelectorArchivoCubemap::Dimensiones{};
            }
            dims_[i] = d;
        }
        rutasMedidas_[i] = rutas[i];
    }
}

void SettingsSkybox::showDataComponent() {
    Skybox* skybox = gameObject ? gameObject->getComponent<Skybox>() : nullptr;
    if (!skybox) return;

    bool visible = skybox->getVisible();
    if (ImGui::Checkbox("Visible", &visible)) skybox->setVisible(visible);
    ImGui::Separator();

    // Seis caras en el orden que espera la pasada del cubemap (+X, -X, +Y,
    // -Y, +Z, -Z). El par getter/setter se pasa por puntero a miembro para no
    // repetir el switch seis veces por lectura y otras seis por escritura.
    using Getter = const std::string& (Skybox::*)() const;
    using Setter = void (Skybox::*)(const std::string&);
    struct Cara {
        const char* etiqueta;
        Getter getter;
        Setter setter;
    };
    const Cara caras[SelectorArchivoCubemap::kCaras] = {
        {"Cara +X (Right)", &Skybox::getCaraMasX, &Skybox::setCaraMasX},
        {"Cara -X (Left)", &Skybox::getCaraMenosX, &Skybox::setCaraMenosX},
        {"Cara +Y (Top)", &Skybox::getCaraMasY, &Skybox::setCaraMasY},
        {"Cara -Y (Bottom)", &Skybox::getCaraMenosY, &Skybox::setCaraMenosY},
        {"Cara +Z (Front)", &Skybox::getCaraMasZ, &Skybox::setCaraMasZ},
        {"Cara -Z (Back)", &Skybox::getCaraMenosZ, &Skybox::setCaraMenosZ},
    };

    // Ruta actual de cada cara, releida antes de los campos para tener a mano el
    // valor de partida del InputText y el destino de cada boton.
    std::string rutas[SelectorArchivoCubemap::kCaras];
    for (int i = 0; i < SelectorArchivoCubemap::kCaras; ++i) {
        rutas[i] = (skybox->*caras[i].getter)();
    }

    for (int i = 0; i < SelectorArchivoCubemap::kCaras; ++i) {
        ImGui::PushID(i);
        char buf[512];
        std::strncpy(buf, rutas[i].c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        ImGui::SetNextItemWidth(-60.0f);
        if (ImGui::InputText(caras[i].etiqueta, buf, sizeof(buf))) {
            (skybox->*caras[i].setter)(buf);
        }
        // Arrastrar y soltar desde el explorador, igual que la malla y el
        // script: el payload lleva la ruta ya absoluta. Esta version de ImGui
        // no admite predicado en AcceptDragDropPayload (devuelve el payload
        // directo), asi que el formato se filtra despues: lo que no sea una
        // imagen decodificable se ignora en vez de dejar una cara rota.
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload =
                    ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                const std::string ruta =
                    static_cast<const char*>(payload->Data);
                if (SelectorArchivoCubemap::esImagenCubemap(ruta)) {
                    (skybox->*caras[i].setter)(ruta);
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("...##browseCubemap")) {
            selector_.solicitar(rutas[i], i);
        }
        ImGui::PopID();
    }

    ImGui::TextDisabled(
        "Formatos: PNG, JPG, TGA, BMP, PSD, HDR. Las seis caras deben medir lo "
        "mismo.");

    // Avisos de las dos condiciones que hacen que el motor caiga al degradado:
    // una cara sin asignar, o una cara que no mide lo mismo que la +X.
    const int sinAsignar = SelectorArchivoCubemap::carasSinAsignar(rutas);
    if (sinAsignar > 0) {
        ImGui::TextDisabled("Faltan %d de 6 caras: hasta que esten las seis se "
                            "dibuja el cielo degradado.",
                            sinAsignar);
    } else {
        sondearDimensiones(rutas);
        const int distinta = SelectorArchivoCubemap::caraConDimensionDistinta(
            dims_);
        if (distinta >= 0) {
            ImGui::TextDisabled(
                "%s mide %dx%d y la +X mide %dx%d: el cubemap se descarta.",
                caras[distinta].etiqueta, dims_[distinta].ancho,
                dims_[distinta].alto, dims_[0].ancho, dims_[0].alto);
        }
    }

    // El modal se dibuja una vez, despues de los campos (los popups se anidan
    // al ultimo widget activo) y su resultado se aplica en el frame de
    // confirmacion.
    const SelectorArchivoCubemap::Resultado resultado = selector_.dibujar();
    if (resultado.confirmado) {
        // La cara destino la fijo el boton ... que abrio el modal.
        (skybox->*caras[selector_.caraDestino()].setter)(resultado.ruta);
    }
}

Component* SettingsSkybox::getComponent() {
    return gameObject ? gameObject->getComponent<Skybox>() : nullptr;
}