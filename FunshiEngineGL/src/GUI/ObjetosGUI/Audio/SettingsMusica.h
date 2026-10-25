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
#ifndef SETTINGSMUSICA_H
#define SETTINGSMUSICA_H

#include "../SettingsComponent.h"
#include "../../../Objetos/GameObject.h"
#include "../../../Objetos/Componentes/Musica.h"

class AudioEngine;

// Inspector del componente Musica: la lista ordenada de pistas (cada una se
// asigna arrastrandola desde el explorador, como con los scripts y los
// efectos de sonido), el volumen, el fundido entre pistas, la repeticion de la
// lista, la reproduccion automatica y los botones Reproducir/Detener
// (requieren el motor inyectado por el catalogo de componentes).
class SettingsMusica : public SettingsComponent {
private:
	GameObject* gameObject = nullptr;
	AudioEngine* motor = nullptr;

public:
	explicit SettingsMusica(GameObject* gameObject);
	void setAudioEngine(AudioEngine* motor) noexcept { this->motor = motor; }
	void showDataComponent() override;
	Component* getComponent() override {
		return gameObject ? gameObject->getComponent<Musica>() : nullptr;
	}
};

#endif // SETTINGSMUSICA_H
