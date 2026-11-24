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

#include "EditorMaterialGUI.h"

#include <cstdint>
#include <cstring>

#include "../../../Herramientas/MaterialPresets.h"
#include "../../../Objetos/Componentes/Render/Material.h"
#include "../../../Rendering/Preview/IPreviewMaterial.h"
#include <imgui.h>
#include "../../../Localizacion/Localizacion.h"

using Localizacion::Texto;

bool dibujarEditorMaterial(Material& material, int& presetAplicado,
                            IPreviewMaterial* preview,
                            const std::string& clave) {
    // El material cambio: hay que redibujar la vista previa (la esfera de
    // muestra). Se lleva un bool y no un valor de retorno para que el panel
    // pueda invalidar la vista antes de que se dibuje.
    bool cambio = false;

    // Presets de MaterialPresets como DATOS aplicados al material actual.
    // Se traducen al dibujar: el idioma puede cambiar mientras el editor corre.
    const char* const presets[] = {
        Localizacion::tr(Texto::Personalizado), Localizacion::tr(Texto::Bronce),
        Localizacion::tr(Texto::Cobre),         Localizacion::tr(Texto::Cromo),
        Localizacion::tr(Texto::Laton),         Localizacion::tr(Texto::Jade),
        Localizacion::tr(Texto::Luz)};
    int combo = presetAplicado + 1;
    if (ImGui::Combo(Localizacion::etiqueta(Texto::Preajuste).c_str(), &combo, presets,
                     (int)(sizeof(presets) / sizeof(presets[0])))) {
        switch (combo) {
            case 1: MaterialPresets::bronze(material); break;
            case 2: MaterialPresets::copper(material); break;
            case 3: MaterialPresets::chrome(material); break;
            case 4: MaterialPresets::brass(material); break;
            case 5: MaterialPresets::jade(material); break;
            case 6: MaterialPresets::luz(material); break;
            default: break; // Personalizado
        }
        presetAplicado = combo - 1;
        cambio = true;
    }

    // Toda edicion manual marca el material como Personalizado.
    float amb[3] = {material.getAmbient()[0], material.getAmbient()[1],
                    material.getAmbient()[2]};
    if (ImGui::ColorEdit3(Localizacion::etiqueta(Texto::ColorAmbiente).c_str(), amb)) {
        material.setAmbient(amb[0], amb[1], amb[2]);
        presetAplicado = -1;
        cambio = true;
    }

    float diff[3] = {material.getDiffuse()[0], material.getDiffuse()[1],
                     material.getDiffuse()[2]};
    if (ImGui::ColorEdit3(Localizacion::etiqueta(Texto::ColorDifuso).c_str(), diff)) {
        material.setDiffuse(diff[0], diff[1], diff[2]);
        presetAplicado = -1;
        cambio = true;
    }

    float spec[3] = {material.getSpecular()[0], material.getSpecular()[1],
                     material.getSpecular()[2]};
    if (ImGui::ColorEdit3(Localizacion::etiqueta(Texto::ColorEspecular).c_str(), spec)) {
        material.setSpecular(spec[0], spec[1], spec[2]);
        presetAplicado = -1;
        cambio = true;
    }

    float emiss[3] = {material.getEmission()[0], material.getEmission()[1],
                      material.getEmission()[2]};
    if (ImGui::ColorEdit3(Localizacion::etiqueta(Texto::ColorEmision).c_str(), emiss)) {
        material.setEmission(emiss[0], emiss[1], emiss[2]);
        presetAplicado = -1;
        cambio = true;
    }

    float shin = material.getShininess();
    if (ImGui::SliderFloat(Localizacion::etiqueta(Texto::Brillo).c_str(), &shin, 0.f, 128.f)) {
        material.setShininess(shin);
        presetAplicado = -1;
        cambio = true;
    }

    // Texturas (descriptores): paths a las imagenes compartidas por el
    // TextureManager de la escena. El renderer moderno resuelve cada slot; el
    // inmediato las ignora. Vacio = slot sin textura.
    auto campoTextura = [&](const char* label, const std::string& actual,
                            void (Material::*setter)(const std::string&)) {
        char buf[1024];
        std::strncpy(buf, actual.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        if (ImGui::InputText(label, buf, sizeof(buf))) {
            (material.*setter)(buf);
            cambio = true;
        }
    };
    ImGui::Separator();
    ImGui::Text("%s", Localizacion::tr(Texto::TexturasVaciasSinTextura));
    campoTextura(Localizacion::etiqueta(Texto::Difusa).c_str(), material.getDiffuseMapPath(),
                 &Material::setDiffuseMapPath);
    campoTextura(Localizacion::etiqueta(Texto::Especular).c_str(), material.getSpecularMapPath(),
                 &Material::setSpecularMapPath);
    campoTextura(Localizacion::etiqueta(Texto::Normal).c_str(), material.getNormalMapPath(),
                 &Material::setNormalMapPath);
    campoTextura(Localizacion::etiqueta(Texto::Emision).c_str(), material.getEmissionMapPath(),
                 &Material::setEmissionMapPath);

    // Vista previa: la esfera de muestra con el material. Se redibuja solo
    // cuando el material cambia (o la primera vez).
    if (preview && preview->disponible()) {
        if (cambio) preview->invalidar(clave);
        if (preview->sucia(clave)) preview->renderizar(clave, material);
        const void* textura = preview->descriptorTextura(clave);
        if (textura) {
            ImGui::Separator();
            ImGui::Text("%s", Localizacion::tr(Texto::TituloVistaPrevia));
            // La textura del render target tiene el origen abajo-izquierda:
            // se voltea la V para que la esfera no salga dada vuelta.
            ImGui::Image(
                static_cast<ImTextureID>(
                    reinterpret_cast<std::intptr_t>(textura)),
                ImVec2(static_cast<float>(preview->lado()),
                       static_cast<float>(preview->lado())),
                ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));
        }
    }

    return cambio;
}
