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
#include "GUIManager.h"
#include "../Configuracion/EditorConfig.h"
#include "../Objetos/Modelos3D.h"
#include "../Scenes/EditorController.h"
#include "../Scenes/SceneRegistry.h"

#include <cstdlib>
#include <vector>

GUIManager::GUIManager(GLFWwindow* window)
    : menuGUI(new MenuGUI(window)),
      modeloSettings(std::make_unique<Modelos3D>()),
      settingGUI(new SettingsObjectInterface(modeloSettings.get(), false)),
      selecteableGUI(new SceneSelectedInterface(true)),
      menuBarGUI(std::make_unique<SceneMenuBarInterface>(true)),
      treeFilesGUI(nullptr), contentOfThisFolder(nullptr) {
    // No se crea ningun proyecto aqui: antes se corria
    // asegurarEstructuraProyecto("Nuevo Proyecto") al arrancar y ese proyecto
    // aparecia aunque ya existiera el proyecto del usuario. La raiz del
    // FileManager se configura en main con configurarProyecto(proyectoActual).
    const std::string path = EditorConfig::directorioSrc("Nuevo Proyecto");
    const std::string rootName = EditorConfig::nombreRaizSrc("Nuevo Proyecto");
    // El FileManager (fachada + modelo + seleccion) viva tanto como los
    // paneles que lo consumen. La raiz es el folder src<nombreProyecto>.
    fileManager = std::make_unique<FileManager>(path, rootName);
    treeFilesGUI = std::make_unique<TreeFilesInterface>(true, fileManager.get());
    contentOfThisFolder = std::make_unique<ContentFolderInterface>(false, fileManager.get());
    dockSpaceGUI = std::make_unique<DockSpaceInterface>(true);
    iconosGUI = std::make_unique<IconosGUI>();
    iconosGUI->init();
    statusBarGUI = std::make_unique<StatusBarInterface>(true);
    // Ventanas del creador de interfaces: arrancan ocultas; se alternan desde
    // el menu "Ventanas" o al crear una interfaz (se persisten por proyecto).
    creadorInterfacesGUI = std::make_unique<CreadorDeInterfaces>(false);
    canvasGUI = std::make_unique<CanvasInterface>(false);
    treeFilesGUI->setIconosGUI(iconosGUI.get());
    contentOfThisFolder->setIconosGUI(iconosGUI.get());
    selecteableGUI->setIconosGUI(iconosGUI.get());
    // El bus de GUI interna se inyecta a los publicadores internos del paquete
    // (menu fachada, ventana Estado); el resto del motor lo alcanza con
    // getEditorEventBus().
    menuGUI->setEditorEventBus(&eventosEditor);
    statusBarGUI->setEditorEventBus(&eventosEditor);
    // El menu "Ventanas" de la barra publica VentanaEstadoCambio para alternar
    // la visibilidad de los paneles del editor (explorador, contenido, etc).
    menuBarGUI->setEditorEventBus(&eventosEditor);
    // Movimientos/renombrados dentro del explorador (arbol o grid): se
    // publican para que main reescriba las referencias de la escena.
    treeFilesGUI->setEditorEventBus(&eventosEditor);
    contentOfThisFolder->setEditorEventBus(&eventosEditor);
    // "Usar" una camara en la ventana Camaras: la escena publica y la fachada
    // reacciona seleccionando el objeto para el inspector (antes GameScene
    // llamaba a EditorController directamente).
    eventosEditor.subscribe([this](const EditorEvent& ev) {
        if (ev.type != EditorEventType::CamaraActivaCambio) return;
        if (editor && ev.camara) editor->selectObject(ev.camara);
    });
}

GUIManager::~GUIManager() {
}

void GUIManager::bindScene(SceneRegistry* scene, EditorController* editor,
                           EventBus* events) {
    this->editor = editor;
    selecteableGUI->bindScene(scene, editor, events);
    statusBarGUI->bindScene(scene);
    settingGUI->setEditor(editor);
    settingGUI->setAudioEngine(audioMotor);
    canvasGUI->setAudioEngine(audioMotor);
}
MenuGUI* GUIManager::getMenuGUI() { return menuGUI.get(); }
TreeFilesInterface* GUIManager::getTreeFilesGUI() { return treeFilesGUI.get(); }

SettingsObjectInterface* GUIManager::getSettingGUI(GameObject* gameObject) {
    if (gameObject == nullptr) {
        settingGUI->setStateGui(false);
        return settingGUI.get();
    }
    settingGUI->setEditor(editor);
    // Solo se refresca el contenido al cambiar de objeto; no se recrea la
    // ventana (mismo patron que ContentFolderInterface).
    if (settingGUI->getObjectInInspector() != gameObject) {
        settingGUI->setTargetObject(gameObject);
    }
    settingGUI->setStateGui(true);
    return settingGUI.get();
}

void GUIManager::removeSettingsGUI() { settingGUI->setStateGui(false); }
void GUIManager::setSelecteableGUI(PriorityListaDE<GameObject*>* gameObjects) {
    // Kept for source compatibility. Scene state is injected with bindScene.
    (void)gameObjects;
}
SceneMenuBarInterface* GUIManager::getMenuBarGUI() {
    return menuBarGUI.get();
}
SceneMenuBarInterface* GUIManager::getMenuBarGUI(bool* targetBool) {
    menuBarGUI->setActivador(targetBool);
    return menuBarGUI.get();
}
SceneSelectedInterface* GUIManager::getSelecteableGUI() { return selecteableGUI.get(); }
ContentFolderInterface* GUIManager::getContentFolderGUI() { return contentOfThisFolder.get(); }
DockSpaceInterface* GUIManager::getDockSpaceGUI() { return dockSpaceGUI.get(); }

void GUIManager::setAudioEngine(AudioEngine* motor) {
    audioMotor = motor;
    if (settingGUI) settingGUI->setAudioEngine(motor);
    if (canvasGUI) canvasGUI->setAudioEngine(motor);
}

StatusBarInterface* GUIManager::getStatusBarGUI() { return statusBarGUI.get(); }

void GUIManager::restaurarEstadosVentanas(const std::map<std::string, bool>& estados) {
    for (GeneralUserInterface* ventana : ventanasPersistentes()) {
        if (!ventana) continue;
        auto it = estados.find(ventana->getNameGui());
        if (it != estados.end()) ventana->setStateGui(it->second);
    }
}

std::vector<GeneralUserInterface*> GUIManager::ventanasPersistentes() const {
    // La ventana Settings es dinamica (depende de la seleccion) y se deja
    // fuera. El resto se persiste por su WindowName.
    return {selecteableGUI.get(), menuBarGUI.get(), treeFilesGUI.get(),
            contentOfThisFolder.get(), dockSpaceGUI.get(), statusBarGUI.get(),
            creadorInterfacesGUI.get(), canvasGUI.get()};
}

std::map<std::string, bool> GUIManager::obtenerEstadosVentanas() const {
    std::map<std::string, bool> estados;
    for (const GeneralUserInterface* ventana : ventanasPersistentes()) {
        if (!ventana) continue;
        estados[ventana->getNameGui()] = ventana->getStateGui();
    }
    return estados;
}

void GUIManager::setEstadoVentana(const std::string& nombre, bool abierta) {
    for (GeneralUserInterface* ventana : ventanasPersistentes()) {
        if (ventana && ventana->getNameGui() == nombre) {
            ventana->setStateGui(abierta);
            return;
        }
    }
}

void GUIManager::sincronizarVentanasMenu() {
    if (menuBarGUI) menuBarGUI->setVentanas(obtenerEstadosVentanas());
}

void GUIManager::configurarProyecto(const std::string& nombreProyecto) {
    EditorConfig::asegurarEstructuraProyecto(nombreProyecto);
    const std::string path = EditorConfig::directorioSrc(nombreProyecto);
    const std::string rootName = EditorConfig::nombreRaizSrc(nombreProyecto);
    if (fileManager) {
        fileManager->setProyecto(path, rootName);
    }
}
