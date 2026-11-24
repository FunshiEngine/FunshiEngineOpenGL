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
#include "GameSceneDetalle.h"

void GameScene::saveScene(const std::string& filename) {
    if (sceneSerializer) sceneSerializer->save(filename);
    // Manifiesto de assets (add-on): se regenera en CADA guardado, asi el
    // JSON centraliza siempre el estado vigente de las rutas (mover/renombrar
    // assets lo renueva de paso). Es independiente del .db binario.
    ManifiestoAssets::guardar(filename + "SceneAssets.json",
                              getGameObjectsScene());
}

bool GameScene::isStart() { return start; }

// La maquina de estados (orquestador) es la fuente de verdad de la simulacion;
// main refleja su decision aca al inicio de cada frame.
void GameScene::setStart(bool activo) noexcept { start = activo; }

void GameScene::setModoJuego(bool juego) noexcept { modoJuego = juego; }

void GameScene::solicitarResetSimulacion() noexcept {
    if (start) resetSimulacionPendiente = true;
}

void GameScene::cerrarSimulacionAntesDeGuardar() {
    resetSimulacionPendiente = false;
    if (!previousStart) {
        start = false;
        eliminarSnapshotSimulacion();
        return;
    }
    start = false;
    simulacionPausada = false;
    update(0.0f);
}

bool GameScene::isModoJuego() const noexcept { return modoJuego; }

bool GameScene::isEditorGUIVisible() const noexcept { return menuActivo; }

bool GameScene::guardarSnapshotSimulacion() {
    eliminarSnapshotSimulacion();
    if (!sceneSerializer || directorioEscena_.empty()) return false;

    namespace fs = std::filesystem;
    std::error_code ec;
    const auto marca = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path base = fs::path(directorioEscena_).parent_path();
    for (unsigned int intento = 0; intento < 32; ++intento) {
        const fs::path candidato =
            base / (".funshi-simulation-" + std::to_string(marca) + "-" +
                    std::to_string(intento));
        ec.clear();
        if (!fs::create_directory(candidato, ec)) {
            if (ec) return false;
            continue;
        }
        ec.clear();
        if (!fs::create_directory(candidato / "Scene", ec) || ec) {
            fs::remove_all(candidato, ec);
            return false;
        }
        directorioSnapshot_ = candidato.string();
        indiceSnapshot_ = (candidato / "SceneBBDDObjetos.txt").string();
        break;
    }
    if (directorioSnapshot_.empty()) return false;

    camaraActivaAlIniciar = getActiveCameraId();
    sceneSerializer->save((fs::path(directorioSnapshot_) / "Scene").string());
    if (!snapshotSimulacionDisponible()) {
        std::cerr << "[escena] no se pudo crear el estado inicial de la "
                     "simulacion; Reset y la restauracion al terminar no "
                     "estaran disponibles\n";
        eliminarSnapshotSimulacion();
        return false;
    }
    return true;
}

bool GameScene::snapshotSimulacionDisponible() const {
    if (indiceSnapshot_.empty() || directorioSnapshot_.empty()) return false;
    namespace fs = std::filesystem;
    std::error_code ec;
    const fs::path indicePath(indiceSnapshot_);
    if (!fs::is_regular_file(indicePath, ec) || ec ||
        fs::file_size(indicePath, ec) == 0 || ec)
        return false;

    const fs::path raizBinaria =
        fs::path(directorioSnapshot_) / "Scene" / "ObjectN0.db";
    ec.clear();
    if (!fs::is_regular_file(raizBinaria, ec) || ec ||
        fs::file_size(raizBinaria, ec) == 0 || ec)
        return false;

    std::ifstream indice(indiceSnapshot_, std::ios::binary);
    if (!indice.is_open()) return false;
    std::string nombreObjeto;
    while (std::getline(indice, nombreObjeto)) {
        if (!nombreObjeto.empty() && nombreObjeto.back() == '\r')
            nombreObjeto.pop_back();
        if (nombreObjeto.empty() || nombreObjeto == "=>" ||
            nombreObjeto == "<=")
            continue;
        const fs::path binario =
            fs::path(directorioSnapshot_) / "Scene" / nombreObjeto;
        ec.clear();
        if (!fs::is_regular_file(binario, ec) || ec ||
            fs::file_size(binario, ec) == 0 || ec)
            return false;
    }
    return !indice.bad() && indice.eof();
}

bool GameScene::restaurarSnapshotSimulacion() {
    if (!sceneSerializer || !snapshotSimulacionDisponible()) return false;

    sceneSerializer->load(indiceSnapshot_,
                          (std::filesystem::path(directorioSnapshot_) / "Scene")
                              .string());
    if (editorController) editorController->registerSceneRigidBodies();
    if (selecteableGUI) {
        selecteableGUI->bindScene(sceneRegistry.get(), editorController.get(),
                                   &events);
        selecteableGUI->setReturnableEntity(nullptr);
    }
    if (managerGUI) managerGUI->removeSettingsGUI();
    requestedActiveCamera = nullptr;
    activeCameraObject = nullptr;
    activeCamera = nullptr;
    contadorCamaras = 0;
    setActiveCameraById(camaraActivaAlIniciar);
    getGameObjectsScene();
    return true;
}

void GameScene::eliminarSnapshotSimulacion() noexcept {
    if (directorioSnapshot_.empty()) return;
    std::error_code ec;
    std::filesystem::remove_all(directorioSnapshot_, ec);
    if (ec) {
        std::cerr << "[escena] no se pudo retirar el estado temporal de la "
                     "simulacion: " << ec.message() << '\n';
    }
    directorioSnapshot_.clear();
    indiceSnapshot_.clear();
}

void GameScene::limpiarRuntimeSimulacion() {
    limpiarColaCompilacion();
    auto* gameObjects = getGameObjectsScene();
    if (gameObjects && !gameObjects->isEmpty()) {
        Position<GameObject*>* pos = gameObjects->first();
        while (pos && pos->getElement()) {
            ListaDE<Component*>* componentes =
                pos->getElement()->getComponents();
            if (componentes && !componentes->isEmpty()) {
                Position<Component*>* componente = componentes->first();
                while (componente) {
                    if (Script* script =
                            dynamic_cast<Script*>(componente->getElement()))
                        script->detener(pos->getElement());
                    componente = componente != componentes->last()
                                     ? componentes->next(componente)
                                     : nullptr;
                }
            }
            pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
        }
    }
    sincronizarAudioPlay(false);
    MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
    inputScripts.reset();
    // Limpia el estado JSON de las interfaces y descarga sus vistas para que
    // una nueva partida no arrastre datos de la anterior.
    estadoInterfaces_.vaciar();
    overlayInterfaces_.vaciar();
}

bool GameScene::isSimulacionPausada() const noexcept { return simulacionPausada; }

// Pausa (F6): congela la simulacion SIN salir de play; al reanudar se retoma
// desde donde quedo (fisica y scripts solo avanzan con !pausada).
void GameScene::setSimulacionPausada(bool pausada) noexcept {
    simulacionPausada = pausada;
}

void GameScene::loadScene(const std::string& pathTxt, const std::string& semiPath) {
    directorioEscena_ = semiPath;
    if (sceneSerializer) {
        sceneSerializer->load(pathTxt, semiPath);
        // La carga limpia la escena y reconstruye los objetos: los punteros
        // cacheados de camara apuntarian a memoria liberada (heap-use-after-free
        // al cambiar de proyecto). Se olvidan aqui; setActiveCameraById o
        // getActiveCamera los resuelven de nuevo sobre la escena cargada.
        requestedActiveCamera = nullptr;
        activeCameraObject = nullptr;
        activeCamera = nullptr;
        // Los RigidBody deserializados nunca pasan por EditorController: la
        // malla se carga despues de los componentes (shape provisional) y el
        // cuerpo no se registra en el mundo. Aqui se reconstruye la shape con
        // los vertices recien cargados y se registra el cuerpo.
        if (editorController) editorController->registerSceneRigidBodies();
        if (selecteableGUI)
            selecteableGUI->bindScene(sceneRegistry.get(), editorController.get(),
                                       &events);
        if (managerGUI) managerGUI->removeSettingsGUI();
        // Las escenas viejas no guardan el objeto "Grilla": se crea sobre la
        // marcha si falta, conservando la visibilidad por defecto.
        asegurarGrilla();

        // Manifiesto de assets (add-on de la serializacion binaria): si
        // existe, sus rutas tienen precedencia sobre las que dejo el .db.
        // El pathTxt es <prefijo>BBDDObjetos.txt; el manifiesto comparte el
        // prefijo con nombre SceneAssets.json.
        const std::string sufijoBBDD = "BBDDObjetos.txt";
        std::string prefijoEscena; // vacio si el path no tiene ese formato
        if (pathTxt.size() >= sufijoBBDD.size() &&
            pathTxt.compare(pathTxt.size() - sufijoBBDD.size(),
                            sufijoBBDD.size(), sufijoBBDD) == 0) {
            prefijoEscena =
                pathTxt.substr(0, pathTxt.size() - sufijoBBDD.size());
            ManifiestoAssets::cargar(prefijoEscena + "SceneAssets.json",
                                     getGameObjectsScene());
        }

        // Sanado de referencias rotas: escenas guardadas con un asset que ya
        // no existe en esa ruta (archivos movidos o renombrados fuera del
        // motor, o daño de un guardado anterior). Con una unica coincidencia
        // del nombre bajo la raiz de assets la referencia se repara y queda
        // registrada en el log; con varias o con ninguna no se adivina. Como
        // la reparacion vive solo en memoria, si algo cambio se persiste ya por
        // el mismo camino que la reescritura de un move: si no, cada apertura
        // tendria que sanar otra vez y el manifiesto seguiria reflejando la
        // ruta vieja.
        const int reparadas =
            RutasReescritura::sanarRutasInexistentes(getGameObjectsScene());
        if (reparadas > 0 && !prefijoEscena.empty()) {
            std::cerr << "[escena] " << reparadas
                      << " referencias de asset reparadas al cargar; se guarda "
                         "la escena con las rutas sanadas\n";
            saveScene(prefijoEscena);
        }
    }
    // Configurar resolver para modo editor: permite resolver referencias a
    // GameObjects por nombre durante serializacion/deserializacion y drag&drop
    // de GameObjects en scripts (inspector) mientras se edita.
    configurarResolverObjetosEditor();
}
