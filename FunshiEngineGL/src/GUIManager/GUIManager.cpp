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
#include "../Configuracion/ProjectPaths.h"
#include "../Objetos/Modelos3D.h"
#include "../Scenes/EditorController.h"
#include "../Scenes/SceneRegistry.h"
#include "../Mods/RegistroPanelesMods.h"

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
    // Servicios del sistema operativo para el explorador: el portapapeles y
    // la recepcion de archivos soltados sobre la ventana. El gestor los crea
    // porque es quien tiene la ventana de GLFW, y los inyecta en la fachada
    // del explorador, que es la unica que ve el contrato.
    portapapeles = std::make_unique<PortapapelesSistema>(window);
    receptorDrop = std::make_unique<ReceptorDropGlfw>();
    fileManager->setPortapapeles(portapapeles.get());
    fileManager->setReceptorDrop(receptorDrop.get());
    if (window != nullptr) receptorDrop->registrar(window);
    dockSpaceGUI = std::make_unique<DockSpaceInterface>(true);
    iconosGUI = std::make_unique<IconosGUI>();
    iconosGUI->init();
    // Los iconos de componente se resuelven por el catalogo (fuente unica) y
    // se cargan a demanda en el inspector.
    reglaIconos = std::make_unique<ReglaIconoCatalogo>();
    resolutorIconos =
        std::make_unique<CatalogoIconos>(*iconosGUI, *reglaIconos);
    settingGUI->setResolutorIconos(resolutorIconos.get());
    menuBarGUI->setIconosGUI(iconosGUI.get());
    statusBarGUI = std::make_unique<StatusBarInterface>(true);
    // La barra de estado es donde el explorador avisa del resultado de una
    // importacion (archivos pegados o arrastrados, errores al copiar).
    contentOfThisFolder->setStatusBar(statusBarGUI.get());
    terminalGUI = std::make_unique<TerminalInterface>(true);
    // Cada ventana se registra al crearse: el gestor no lleva una lista fija,
    // asi que agregar una ventana no obliga a tocar el estado ni el menu. La
    // ventana se registra DESPUES de existir: registrarla antes guardaria un
    // puntero nulo y la ventana quedaria fuera del registro (sin estado
    // persistido y sin entrada en el menu de ventanas).
    registrarVentana(selecteableGUI.get());
    registrarVentana(menuBarGUI.get());
    registrarVentana(treeFilesGUI.get());
    registrarVentana(contentOfThisFolder.get());
    registrarVentana(dockSpaceGUI.get());
    registrarVentana(statusBarGUI.get());
    registrarVentana(terminalGUI.get());
    panelesModsGUI = std::make_unique<PanelesModsInterface>();
    // El taller de efectos se registra igual que el resto: su estado se
    // persiste con el nombre de la ventana y el menu Ventanas la lista solo.
    animacionesParticulasGUI =
        std::make_unique<AnimacionesParticulasInterface>();
    registrarVentana(animacionesParticulasGUI.get());
    // El estado de la interfaz y la barra de la orbita son piezas de la GUI:
    // las usa el input (que consulta el estado) y main (que dibuja la barra).
    estadoInterfaz = std::make_unique<EstadoInterfazImGui>();
    sidebarOrbitaGUI = std::make_unique<SidebarOrbitaGUI>();
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
                           EventBus* events,
                           std::function<DimensionProyecto()> dimensionProvider) {
    this->editor = editor;
    selecteableGUI->bindScene(scene, editor, events);
    statusBarGUI->bindScene(scene);
    settingGUI->setEditor(editor);
    settingGUI->setAudioEngine(audioMotor);
    settingGUI->setEventBus(events);
    if (dimensionProvider) settingGUI->setDimensionProvider(dimensionProvider);
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
}

void GUIManager::setPreviewMaterial(IPreviewMaterial* preview) {
    previewMaterial = preview;
    if (settingGUI) settingGUI->setPreviewMaterial(preview);
}

StatusBarInterface* GUIManager::getStatusBarGUI() { return statusBarGUI.get(); }

void GUIManager::restaurarEstadosVentanas(const std::map<std::string, bool>& estados) {
    for (GeneralUserInterface* ventana : ventanasPersistentes()) {
        if (!ventana) continue;
        auto it = estados.find(ventana->getNameGui());
        if (it != estados.end()) ventana->setStateGui(it->second);
    }
    // Los paneles de los mods no son ventanas del registro central: su estado
    // vive en el registro de paneles de mods (por su clave de ventana).
    for (const FunshiMods::PanelMod& panel :
         FunshiMods::RegistroPanelesMods::global().paneles()) {
        auto it = estados.find(panel.nombreVentana);
        if (it != estados.end()) {
            FunshiMods::RegistroPanelesMods::global().establecerEstado(
                panel.nombreVentana, it->second);
        }
    }
}

void GUIManager::registrarVentana(GeneralUserInterface* ventana) {
    if (!ventana) return;
    for (GeneralUserInterface* existente : ventanas_) {
        if (existente == ventana) return;
    }
    ventanas_.push_back(ventana);
}

std::vector<GeneralUserInterface*> GUIManager::ventanasPersistentes() const {
    // Las ventanas registradas: la ventana Settings es dinamica (depende de la
    // seleccion) y no se registra, asi que queda fuera.
    return ventanas_;
}

std::map<std::string, bool> GUIManager::obtenerEstadosVentanas() const {
    std::map<std::string, bool> estados;
    for (const GeneralUserInterface* ventana : ventanasPersistentes()) {
        if (!ventana) continue;
        estados[ventana->getNameGui()] = ventana->getStateGui();
    }
    // Los paneles de los mods se persisten con la misma clave con la que se
    // registra la ventana (unica por mod y panel).
    for (const FunshiMods::PanelMod& panel :
         FunshiMods::RegistroPanelesMods::global().paneles()) {
        estados[panel.nombreVentana] = panel.abierta;
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
    // Si no es una ventana del registro central, puede ser el panel de un
    // mod: su estado vive en el registro de paneles de mods.
    FunshiMods::RegistroPanelesMods::global().establecerEstado(nombre, abierta);
}

void GUIManager::sincronizarVentanasMenu() {
    if (!menuBarGUI) return;
    menuBarGUI->setVentanas(obtenerEstadosVentanas());
    // Los paneles de los mods no son ventanas fijas: su lista cambia con los
    // mods que se cargan y descargan, asi que el menu la recibe cada frame.
    if (panelesModsGUI) menuBarGUI->setPanelesMods(panelesModsGUI->etiquetas());
}

void GUIManager::configurarProyecto(const std::string& nombreProyecto) {
    EditorConfig::asegurarEstructuraProyecto(nombreProyecto);
    const std::string path = EditorConfig::directorioSrc(nombreProyecto);
    const std::string rootName = EditorConfig::nombreRaizSrc(nombreProyecto);
    if (terminalGUI)
        terminalGUI->setDirectorioProyecto(
            ProjectPaths::directorioProyecto(nombreProyecto));
    if (fileManager) {
        fileManager->setProyecto(path, rootName);
    }
}
