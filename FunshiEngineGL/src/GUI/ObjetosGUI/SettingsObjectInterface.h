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
#ifndef SETTINGSOBJECTINTERFACE_H
#define SETTINGSOBJECTINTERFACE_H

#include "../GeneralUserInterface.h"
#include "../../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

class GameObject;
class SettingsComponent;
class EditorController;
class AudioEngine;

class SettingsObjectInterface : public GeneralUserInterface {
private:
	GameObject* object;
	ListaDE<SettingsComponent*>* listaDESettingsComponent;
	EditorController* editor = nullptr;
	AudioEngine* audioMotor = nullptr;
	int momentaneantID = 0;

public:
	SettingsObjectInterface(GameObject* object, bool stateGUI);
	~SettingsObjectInterface();

	void setEditor(EditorController* editor);
	// El motor de audio se inyecta desde la escena para que los inspectores de
	// AudioSource puedan probar la reproduccion. Puede ser nullptr.
	void setAudioEngine(AudioEngine* motor) { audioMotor = motor; }
	void loadComponents();

	// Cambia el objeto inspeccionado sin recrear la ventana: limpia y recarga
	// solo el contenido (mismo patron que ContentFolderInterface).
	void setTargetObject(GameObject* newObject);

	virtual GameObject* getObjectInInspector();

	virtual void initGUI() override;
	virtual void contentGUI() override;
	virtual void endGUI() override;
	virtual void printGUI() override;
};
#endif