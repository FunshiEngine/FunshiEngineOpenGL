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
#ifndef SETTINGSSKYBOX_H
#define SETTINGSSKYBOX_H

#include "../SettingsComponent.h"
#include "SelectorArchivoCubemap.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Skybox.h"

class SettingsSkybox : public SettingsComponent {
    GameObject* gameObject;
    // Estado del modal de eleccion de cara. Vive en el panel (no global) para
    // que cada inspector tenga el suyo: se dibuja una vez por frame desde aca,
    // despues de los campos, y su resultado se aplica al frame de confirmacion.
    SelectorArchivoCubemap::Modal selector_;
    // Ultimas dimensiones medidas y las rutas a las que corresponden. El panel
    // se redibuja cada frame, asi que sin esto se abririan los seis archivos en
    // cada uno solo para volver a leer la misma cabecera.
    SelectorArchivoCubemap::Dimensiones dims_[SelectorArchivoCubemap::kCaras];
    std::string rutasMedidas_[SelectorArchivoCubemap::kCaras];

    // Mide las caras que aun no se midieron. Solo toca disco cuando cambia
    // alguna ruta, y deja en cero las que no se pudieron leer.
    void sondearDimensiones(const std::string rutas[SelectorArchivoCubemap::kCaras]);

public:
    explicit SettingsSkybox(GameObject* gameObject);

    void showDataComponent() override;
    Component* getComponent() override { return gameObject ? gameObject->getComponent<Skybox>() : nullptr; }
};

#endif // SETTINGSSKYBOX_H