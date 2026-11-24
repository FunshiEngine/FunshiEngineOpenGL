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
#include "MenuViewDetalle.h"
#include "../../Localizacion/Localizacion.h"

void MenuView::renderizarOpciones() {
    // Columna centrada con scroll.
    const ImVec2 win = ImGui::GetWindowSize();
    constexpr float kAncho = 460.0f;
    constexpr float kAltoInferior = 90.0f; // deja lugar al boton "Volver"

    ImGui::SetCursorPos(ImVec2((win.x - kAncho) * 0.5f, 50.0f));
    ImGui::BeginChild("##opciones", ImVec2(kAncho, win.y - 50.0f - kAltoInferior),
                      false, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    ImGui::TextUnformatted(model->traducir(Texto::Opciones));
    ImGui::Separator();

    // Idioma: afecta a todas las pestanas, por eso vive fuera de ellas.
    {
        ImGui::TextUnformatted(model->traducir(Texto::Idioma));
        const std::vector<std::string>& idiomas = model->getIdiomas();
        const std::string& actual = model->getIdioma();
        ImGui::SetNextItemWidth(-1.0f);
        // El modelo guarda codigos ("es"); el combo muestra los nombres.
        if (ImGui::BeginCombo("##idioma",
                              Localizacion::nombreIdioma(actual).c_str())) {
            for (const std::string& opcion : idiomas) {
                const bool seleccionada = (opcion == actual);
                if (ImGui::Selectable(Localizacion::nombreIdioma(opcion).c_str(),
                                      seleccionada)) {
                    model->setIdioma(opcion);
                }
                if (seleccionada) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }
    }

    ImGui::Spacing();

    if (ImGui::BeginTabBar("##pestanasOpciones")) {
        // ---- Audio: volumen maestro de efectos de sonido. ----
        if (ImGui::BeginTabItem(model->traducir(Texto::Audio))) {
            float volumen = model->getVolumenMaestro();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##volumenMaestro", &volumen, 0.0f, 1.0f,
                                   "%.2f")) {
                model->setVolumenMaestro(volumen);
            }
            ImGui::TextDisabled(
                "Afecta a todos los efectos de sonido de la escena.");
            ImGui::EndTabItem();
        }

        // ---- Apariencia: tema, B/N, acento y fondos de cielo. ----
        if (ImGui::BeginTabItem(model->traducir(Texto::Apariencia))) {
            ImGui::TextUnformatted(model->traducir(Texto::RadioDifuminado));
            // Radio del circulo-horizonte de la grilla y de la guia de eje,
            // en unidades de mundo: por debajo del slider el piso se ve mas
            // cerca y con menos lineas (mas performant); por encima llega mas
            // lejos. Se edita sobre una copia y se delega al modelo UNA vez,
            // como el resto de apariencia.
            {
                Apariencia apRadio = model->getApariencia();
                float radio = apRadio.radioDifuminado;
                ImGui::SetNextItemWidth(-1.0f);
                if (ImGui::SliderFloat("##radioDifuminado", &radio,
                                       AparienciaUtil::kRadioDifuminadoMinimo,
                                       AparienciaUtil::kRadioDifuminadoMaximo,
                                       "%.0f")) {
                    apRadio.radioDifuminado = radio;
                    model->setApariencia(apRadio);
                }
            }

            // Se edita una copia y se delega al modelo UNA vez si hubo
            // cambios; asi el modelo sigue siendo la unica fuente de verdad
            // (patron MVP).
            Apariencia ap = model->getApariencia();
            bool cambio = false;

            cambio |= ImGui::Checkbox(etiqueta(model, Texto::TemaClaro).c_str(),
                                      &ap.temaClaro);
            cambio |= ImGui::Checkbox(etiqueta(model, Texto::ModoBN).c_str(),
                                      &ap.blancoYNegro);
            ImGui::TextUnformatted(model->traducir(Texto::ColorAcento));
            // Acento solo RGB (ColorEdit3 = ColorEdit4 + NoAlpha): el alpha
            // del perfil no participa del tema (cada rol aporta su propia
            // transparencia, ver TemaEditor::acento), asi que no se expone la
            // barra de alpha que podia apagar los checks, los grabs y los
            // enlaces de la interfaz. ColorEdit3 preserva acento[3] tal como
            // estaba guardado.
            cambio |= ImGui::ColorEdit3("##acento", ap.acento);
            ImGui::TextUnformatted(model->traducir(Texto::ColorCieloSuperior));
            cambio |= ImGui::ColorEdit3("##cieloSuperior", ap.fondoSuperior);
            ImGui::TextUnformatted(model->traducir(Texto::ColorCieloInferior));
            cambio |= ImGui::ColorEdit3("##cieloInferior", ap.fondoInferior);
            ImGui::TextDisabled("%s", model->traducir(Texto::AyudaBN));

            if (ImGui::Button(etiqueta(model, Texto::RestablecerApariencia).c_str(),
                              ImVec2(-1.0f, 0.0f))) {
                ap.restablecer();
                cambio = true;
            }

            if (cambio) model->setApariencia(ap);
            ImGui::EndTabItem();
        }

        // ---- Controles: sensibilidad de camara y de movimiento. ----
        if (ImGui::BeginTabItem(model->traducir(Texto::Controles))) {
            ImGui::TextUnformatted(model->traducir(Texto::SensibilidadCamara));
            float sensibilidad = model->getSensibilidadCamara();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##sensibilidad", &sensibilidad, 0.02f,
                                   5.0f, "%.2f")) {
                model->setSensibilidadCamara(sensibilidad);
            }

            ImGui::TextUnformatted(model->traducir(Texto::SensibilidadMovimiento));
            float movimiento = model->getSensibilidadMovimientoCamara();
            ImGui::SetNextItemWidth(-1.0f);
            if (ImGui::SliderFloat("##sensibilidadMovimiento", &movimiento,
                                   0.1f, 5.0f, "%.2f")) {
                model->setSensibilidadMovimientoCamara(movimiento);
            }
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::EndChild();

    // Boton "Volver" fijo abajo, centrado, fuera del area con scroll.
    ImGui::SetCursorPos(ImVec2((win.x - kBotonAncho) * 0.5f,
                               win.y - kAltoInferior + 20.0f));
    if (ImGui::Button(etiqueta(model, Texto::Volver).c_str(),
                      ImVec2(kBotonAncho, kBotonAlto))) {
        model->volver();
    }
}
