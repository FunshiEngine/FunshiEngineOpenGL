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
#include <functional>
#include "../GUI/MenusGUI/MenuGUI.h"
#include "../GUI/SceneGUI/SceneSelectedInterface.h"
#include "../GUI/FileManagerGUI/TreeFilesInterface.h"
#include "../GUI/FileManagerGUI/ContentFolderInterface.h"
#include "../GUI/ObjetosGUI/SettingsObjectInterface.h"
#include "../GUI/SceneGUI/SceneMenuBarInterface.h"
#include "../GUI/DockSpaceGUI/DockSpaceInterface.h"
#include "../GUI/Estado/StatusBarInterface.h"
#include "../GUI/Estado/EstadoInterfazImGui.h"
#include "../GUI/Navegacion/SidebarOrbitaGUI.h"
#include "../GUI/Terminal/TerminalInterface.h"
#include "../GUI/ModsGUI/PanelesModsInterface.h"
#include "../GUI/EfectosGUI/AnimacionesParticulasInterface.h"
#include "../Herramientas/IconosGUI/IconosGUI.h"
#include "../Herramientas/IconosGUI/CatalogoIconos.h"
#include "../Herramientas/IconosGUI/ReglaIconoCatalogo.h"
#include "../FileManager/FileManager.h"
#include "../FileManager/PortapapelesSistema.h"
#include "../FileManager/ReceptorDropGlfw.h"
#include "../Estructuras/ListasEnlazadas/ListasConPrioridad/PriorityListaDE.h"
#include "../Events/EditorEventBus.h"

// Forward declarations
class SceneRegistry;
class EditorController;
class EventBus;
class GameObject;
class AudioEngine;
class IPreviewMaterial;

using namespace std;

class GUIManager {
private:
	// Use unique_ptr for owned resources
	// menuGUI es la fachada del paquete MenuGUI (GUI de inicio del motor):
	// GUIManager y main solo conversan con ella, no con las clases internas.
	// fileManager es la fachada del explorador de archivos: posee el
	// modelo y la seleccion compartida que ambos paneles leen cada frame.
	std::unique_ptr<FileManager> fileManager;
	std::unique_ptr<MenuGUI> menuGUI;
	// Objeto de inspeccion por defecto del inspector: dueno de la instancia
	// que se le pasa a SettingsObjectInterface en el ctor (evita leak).
	std::unique_ptr<GameObject> modeloSettings;
	std::unique_ptr<SettingsObjectInterface> settingGUI;
	std::unique_ptr<SceneSelectedInterface> selecteableGUI;
	std::unique_ptr<SceneMenuBarInterface> menuBarGUI;
	std::unique_ptr<TreeFilesInterface> treeFilesGUI;
	std::unique_ptr<DockSpaceInterface> dockSpaceGUI;
	std::unique_ptr<IconosGUI> iconosGUI;
	// Iconos de componente: la regla lee el catalogo (que es la fuente unica
	// del icono de cada tipo) y el resolvedor lo pinta en el inspector.
	std::unique_ptr<ReglaIconoCatalogo> reglaIconos;
	std::unique_ptr<CatalogoIconos> resolutorIconos;
	EditorController* editor = nullptr;
	std::unique_ptr<ContentFolderInterface> contentOfThisFolder;
	std::unique_ptr<StatusBarInterface> statusBarGUI;
	std::unique_ptr<TerminalInterface> terminalGUI;
	// Paneles que aportan los mods cargados. Cada uno es una ventana propia
	// que dibuja el motor alrededor del callback que entrego el mod.
	std::unique_ptr<PanelesModsInterface> panelesModsGUI;
	// Taller de efectos: perfil de particulas configurado en la ventana y
	// generado como archivo de assets (el explorador lo ofrece al arrastre).
	std::unique_ptr<AnimacionesParticulasInterface> animacionesParticulasGUI;
	// Estado de la interfaz (la captura de teclado y mouse de ImGui): el
	// input lo consulta para saber si una tecla es de un widget.
	std::unique_ptr<EstadoInterfazImGui> estadoInterfaz;
	// Barra lateral del radio de orbita (ayuda de navegacion del editor).
	std::unique_ptr<SidebarOrbitaGUI> sidebarOrbitaGUI;
	// Servicios del sistema operativo que usa el explorador de archivos:
	// el portapapeles (para pegar con Ctrl+V) y la recepcion de archivos
	// soltados sobre la ventana del editor. Los crea el gestor porque es
	// quien tiene la ventana; el explorador solo conoce el contrato.
	std::unique_ptr<PortapapelesSistema> portapapeles;
	std::unique_ptr<ReceptorDropGlfw> receptorDrop;
	// Motor de audio inyectado por la escena para el inspector de AudioSource.
	AudioEngine* audioMotor = nullptr;
	// Vista previa de material (inyectada por la escena).
	IPreviewMaterial* previewMaterial = nullptr;

	// Canal de eventos de GUI interna (ARQUITECTURA_ESTADOS_GUI.md §4.2B).
	// El dueño del bus es GUIManager (su registro central de ventanas): el
	// menu, la escena y main publican/suscriben sin conocerse entre si.
	EditorEventBus eventosEditor;

	// Registro de ventanas del editor: cada ventana se registra al crearse y
	// el gestor no lleva una lista fija. Asi, restaurar el estado, obtenerlo o
	// alternar la visibilidad de una ventana no se desincronizan al agregar
	// una nueva, y una ventana que viva fuera del gestor (un panel de un mod,
	// por ejemplo) puede registrarse aqui.
	std::vector<GeneralUserInterface*> ventanas_;

	// Ventanas cuyo stateGUI se persiste (las registradas).
	std::vector<GeneralUserInterface*> ventanasPersistentes() const;

public:
	GUIManager(GLFWwindow* window);
	~GUIManager();

	void bindScene(SceneRegistry* scene, EditorController* editor,
	               EventBus* events,
	               std::function<DimensionProyecto()> dimensionProvider = {});

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
	IconosGUI* getIconosGUI() noexcept { return iconosGUI.get(); }
	StatusBarInterface* getStatusBarGUI();
	TerminalInterface* getTerminalGUI() noexcept { return terminalGUI.get(); }
	PanelesModsInterface* getPanelesModsGUI() noexcept {
		return panelesModsGUI.get();
	}
	// Taller de efectos: la ventana dedicada de animaciones y particulas.
	AnimacionesParticulasInterface* getAnimacionesParticulasGUI() noexcept {
		return animacionesParticulasGUI.get();
	}
	// Estado de la interfaz: el input lo consulta para saber si el teclado o
	// el mouse pertenecen a un widget.
	IEstadoInterfaz* getEstadoInterfaz() noexcept { return estadoInterfaz.get(); }
	// Barra lateral del radio de orbita: la dibuja la GUI con el estado que
	// entrega el input.
	SidebarOrbitaGUI* getSidebarOrbitaGUI() noexcept { return sidebarOrbitaGUI.get(); }

	// La escena inyecta su AudioEngine (para el inspector de AudioSource).
	void setAudioEngine(AudioEngine* motor);

	// La escena inyecta su vista previa de material (la esfera de muestra del
	// panel del material). Puede ser nullptr.
	void setPreviewMaterial(IPreviewMaterial* preview);

	// Registra una ventana del editor en el registro central: su estado se
	// persiste con el resto y se puede alternar por nombre desde el menu.
	void registrarVentana(GeneralUserInterface* ventana);

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
