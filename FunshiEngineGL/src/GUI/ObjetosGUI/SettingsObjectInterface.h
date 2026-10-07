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
#include <array>

class GameObject;
class SettingsComponent;
class Component;
class EditorController;
class AudioEngine;
class EventBus;

class SettingsObjectInterface : public GeneralUserInterface {
private:
	GameObject* object;
	GameObject* tagBufferOwner_ = nullptr;
	std::array<char, 128> tagBuffer_{};
	ListaDE<SettingsComponent*>* listaDESettingsComponent;
	EditorController* editor = nullptr;
	AudioEngine* audioMotor = nullptr;
	EventBus* events = nullptr;
	size_t eventSubscription = 0;
	bool mostrarVisualesDepuracion_ = true;
	// Cuenta los cambios estructurales (alta/baja de componente) que
	// reconciliaron la lista de paneles. La carga inicial no cuenta.
	size_t reconciliaciones = 0;
	// Paneles Settings* creados en total (monotono). Reutilizar un panel no
	// crea ninguno; reconstruir de cero si. Sirve para distinguir ambos casos.
	size_t creacionesSettings = 0;

	// Para evitar reentrencia durante iteracion: si ComponentStructureChanged
	// llega mientras iteramos en contentGUI, no reconciliamos ya; lo hace el caller.
	bool iterandoComponentes = false;

	// Borrado diferido: el componente a eliminar se encola y se borra
	// al final de contentGUI, fuera de la iteracion.
	SettingsComponent* componenteABorrar = nullptr;

	void desvincular();
	// Reconciliacion ante un cambio estructural: reutiliza los Settings cuyo
	// componente sigue vigente, borra los huerfanos y crea los que faltan.
	void reconciliarComponentes();
	void purgarSettingsHuerfanos();
	void crearSettingsFaltantes();
	bool tieneSettingsPara(Component* componente);

public:
	SettingsObjectInterface(GameObject* object, bool stateGUI);
	~SettingsObjectInterface();

	void setEditor(EditorController* editor);
	void setMostrarVisualesDepuracion(bool mostrar) noexcept {
		mostrarVisualesDepuracion_ = mostrar;
	}
	// El motor de audio se inyecta desde la escena para que los inspectores de
	// AudioSource puedan probar la reproduccion. Puede ser nullptr.
	void setAudioEngine(AudioEngine* motor) { audioMotor = motor; }
	// Canal de eventos de la escena: el inspector se suscribe para desvincularse
	// cuando el objeto inspeccionado se borra o se limpia la escena.
	void setEventBus(EventBus* bus);
	void loadComponents();

	// Numero de reconciliaciones disparadas por cambios estructurales (alta o
	// baja de componente). La carga inicial y el cambio de objeto no cuentan.
	size_t reconstruccionesSettings() const { return reconciliaciones; }
	// Total de paneles creados (monotono). Si sube ante un cambio, hubo
	// recreacion; si no, los paneles vigentes se reutilizaron.
	size_t settingsCreados() const { return creacionesSettings; }
	// Settings en la posicion indicada (0..n-1), o nullptr si esta fuera.
	SettingsComponent* settingsEnIndice(size_t indice);

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