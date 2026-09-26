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
#ifndef GUIMANAGER_H
#define GUIMANAGER_H

#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <cstdlib>
#include "../GUI/MenusGUI/MenuGUI.h"
#include "../GUI/SceneGUI/SceneSelectedInterface.h"
#include "../GUI/FileManagerGUI/TreeFilesInterface.h"
#include "../GUI/FileManagerGUI/ContentFolderInterface.h"
#include "../GUI/ObjetosGUI/SettingsObjectInterface.h"
#include "../GUI/SceneGUI/SceneMenuBarInterface.h"
#include "../GUI/DockSpaceGUI/DockSpaceInterface.h"
#include "../GUI/Estado/StatusBarInterface.h"
#include "../GUI/CreadorUI/CreadorDeInterfaces.h"
#include "../GUI/CreadorUI/CanvasInterface.h"
#include "../Herramientas/IconosGUI/IconosGUI.h"
#include "../FileManager/FileManager.h"
#include "../Estructuras/ListasEnlazadas/ListasConPrioridad/PriorityListaDE.h"
#include "../Events/EditorEventBus.h"

// Forward declarations
class SceneRegistry;
class EditorController;
class EventBus;
class Modelos3D;
class AudioEngine;

using namespace std;

class GUIManager {
private:
	// Use unique_ptr for owned resources
	// menuGUI es la fachada del paquete MenuGUI (GUI de inicio del motor):
	// GUIManager y main solo conversan con ella, no con las clases internas.
	// fileManager es la fachada del explorador de archivos (R1): posee el
	// modelo y la seleccion compartida que ambos paneles leen cada frame.
	std::unique_ptr<FileManager> fileManager;
	std::unique_ptr<MenuGUI> menuGUI;
	// Objeto de inspeccion por defecto del inspector: dueno de la instancia
	// que se le pasa a SettingsObjectInterface en el ctor (evita leak).
	std::unique_ptr<Modelos3D> modeloSettings;
	std::unique_ptr<SettingsObjectInterface> settingGUI;
	std::unique_ptr<SceneSelectedInterface> selecteableGUI;
	std::unique_ptr<SceneMenuBarInterface> menuBarGUI;
	std::unique_ptr<TreeFilesInterface> treeFilesGUI;
	std::unique_ptr<DockSpaceInterface> dockSpaceGUI;
	std::unique_ptr<IconosGUI> iconosGUI;
	EditorController* editor = nullptr;
	std::unique_ptr<ContentFolderInterface> contentOfThisFolder;
	std::unique_ptr<StatusBarInterface> statusBarGUI;
	// Creador de interfaces de usuario (assets JSON) y canvas que las pinta
	// con su sonido. Ventanas persistentes mas: anclables, alternables desde
	// el menu "Ventanas" y con estado guardado por proyecto.
	std::unique_ptr<CreadorDeInterfaces> creadorInterfacesGUI;
	std::unique_ptr<CanvasInterface> canvasGUI;
	// Motor de audio inyectado por la escena para el inspector de AudioSource.
	AudioEngine* audioMotor = nullptr;

	// Canal de eventos de GUI interna (ARQUITECTURA_ESTADOS_GUI.md §4.2B).
	// El dueño del bus es GUIManager (su registro central de ventanas): el
	// menu, la escena y main publican/suscriben sin conocerse entre si.
	EditorEventBus eventosEditor;

	// Ventanas cuyo stateGUI se persiste. Helper unico para que restaurar y
	// obtener no se desincronicen al agregar una ventana nueva.
	std::vector<GeneralUserInterface*> ventanasPersistentes() const;

public:
	GUIManager(GLFWwindow* window);
	~GUIManager();

	void bindScene(SceneRegistry* scene, EditorController* editor, EventBus* events);

	EditorEventBus* getEditorEventBus() noexcept { return &eventosEditor; }

	MenuGUI* getMenuGUI();
	TreeFilesInterface* getTreeFilesGUI();
	SettingsObjectInterface* getSettingGUI(GameObject* gameObject);
	void removeSettingsGUI();
	void setSelecteableGUI(PriorityListaDE<GameObject*>* gameObjects);
	SceneMenuBarInterface* getMenuBarGUI();
	SceneMenuBarInterface* getMenuBarGUI(bool* targetBool);
	SceneSelectedInterface* getSelecteableGUI();
	ContentFolderInterface* getContentFolderGUI();
	DockSpaceInterface* getDockSpaceGUI();
	StatusBarInterface* getStatusBarGUI();

	// Acceso a las ventanas del sistema de audio + creador de interfaces.
	CreadorDeInterfaces* getCreadorInterfacesGUI() { return creadorInterfacesGUI.get(); }
	CanvasInterface* getCanvasGUI() { return canvasGUI.get(); }
	// La escena inyecta su AudioEngine (para el inspector de AudioSource).
	void setAudioEngine(AudioEngine* motor);

	// Configura la ruta y el nombre del proyecto en el explorador de archivos
	void configurarProyecto(const std::string& nombreProyecto);

	// Persistencia del estado de las ventanas (EditorConfig): restaura el
	// stateGUI de cada ventana persistente por su nombre y recoge el estado
	// actual. La ventana Settings es dinamica (depende de la seleccion) y se
	// deja fuera.
	void restaurarEstadosVentanas(const std::map<std::string, bool>& estados);
	std::map<std::string, bool> obtenerEstadosVentanas() const;
	// Cambia la visibilidad de una sola ventana persistente por su nombre
	// (responsive del menu "Ventanas" y del cierre con 'X'). No esta en el
	// mapa persistido: no crea la entrada, solo la aplica.
	void setEstadoVentana(const std::string& nombre, bool abierta);
	// Refresca las casillas del menu "Ventanas" de la barra con el estado
	// actual de los paneles (se llama una vez por frame).
	void sincronizarVentanasMenu();
};
#endif
