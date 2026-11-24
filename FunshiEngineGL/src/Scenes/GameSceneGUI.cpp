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
#include "../Localizacion/Localizacion.h"
#include "../GUI/WindowNames.h"

using Localizacion::Texto;

void GameScene::GUI() {
    // En modo runtime (juego exportado sin editor), no hay GUI del editor
    if (!managerGUI) return;
    
    auto* gameObjects = getGameObjectsScene();
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, menuActivo ? 1.0f : 0.0f);
    ImGui::BeginDisabled(!menuActivo);
    ImGui::BeginDisabled(modoJuego);
    selecteableGUI->printGUI();
    if (gameObjects->isElement(selecteableGUI->getReturnableEntity())) {
        SettingsObjectInterface* settings =
            managerGUI->getSettingGUI(selecteableGUI->getReturnableEntity());
        if (settings) {
            settings->setMostrarVisualesDepuracion(!modoJuego);
            settings->printGUI();
        }
    }
    pintarViewportsGUI();
    pintarVentanaCamaras();
    pintarVistaInterfaz();
    if (managerGUI) managerGUI->getStatusBarGUI()->printGUI();
    ImGui::EndDisabled();
    menuBarGUI->printGUI();
    if (TerminalInterface* terminal = managerGUI->getTerminalGUI())
        terminal->printGUI();
    // Taller de efectos (animaciones y particulas): como el terminal, queda
    // disponible mientras la GUI del editor este activa.
    if (AnimacionesParticulasInterface* tallerEfectos =
            managerGUI->getAnimacionesParticulasGUI())
        tallerEfectos->printGUI();
    // El overlay de las interfaces de juego (HUD) se dibuja aparte en
    // pintarInterfacesJuego(), porque tambien corre en el juego exportado,
    // donde no existe la GUI del editor.
    ImGui::EndDisabled();
    ImGui::PopStyleVar();
}

void GameScene::pintarInterfacesJuego() {
    // HUD web: en el juego exportado siempre y en el editor durante el modo
    // juego (Play). La logica vive en OverlayInterfaces. Con el mouse sobre una
    // ventana del editor la interfaz se dibuja pero no recibe el input.
    const bool recibeInput = !managerGUI || !ImGui::GetIO().WantCaptureMouse;
    overlayInterfaces_.pintarHud(getGameObjectsScene(), &estadoInterfaces_, deltaTime,
                                 recibeInput);
}

bool GameScene::interfazNativaHabilitada() const noexcept {
    // En el juego exportado (sin GUI del editor) la interfaz nativa se dibuja
    // siempre; en el editor, solo en modo juego, para no tapar los paneles
    // mientras se edita la escena.
    return managerGUI == nullptr || modoJuego;
}

void GameScene::actualizarInterfazNativa() {
    interfacesNativas_.clear();
    if (!interfazNativaHabilitada()) return;

    interfacesNativas_ =
        InterfacesJuego::recolectarUINativas(getGameObjectsScene());
    if (interfacesNativas_.empty()) return;

    // El raton llega por el mismo canal que el resto de la GUI (el ImGuiIO).
    // Sin contexto de ImGui no hay puntero que mirar.
    if (ImGui::GetCurrentContext() == nullptr) return;

    const ImGuiIO& io = ImGui::GetIO();
    EntradaUI entrada;
    entrada.posicion.x = io.MousePos.x;
    entrada.posicion.y = io.MousePos.y;
    entrada.botonIzquierdo = io.MouseDown[0];
    entrada.clicIzquierdo = io.MouseClicked[0];

    const float ancho = io.DisplaySize.x;
    const float alto = io.DisplaySize.y;
    for (UIComponent* interfaz : interfacesNativas_) {
        interfaz->actualizarInteraccion(entrada, ancho, alto);
    }
}

void GameScene::pintarInterfazNativa() {
    if (!interfazNativaHabilitada()) return;
    if (!uiRenderer_) return;

    if (interfacesNativas_.empty()) {
        interfacesNativas_ =
            InterfacesJuego::recolectarUINativas(getGameObjectsScene());
    }
    if (interfacesNativas_.empty()) return;

    const ImGuiIO& io = ImGui::GetIO();
    ImDrawList* dibujo = ImGui::GetForegroundDrawList();
    for (UIComponent* interfaz : interfacesNativas_) {
        // El layout se (re)calcula aqui por si el frame de entrada no corrio
        // (por ejemplo, con la simulacion en pausa).
        interfaz->lienzo().layout(io.DisplaySize.x, io.DisplaySize.y);
        uiRenderer_->dibujar(interfaz->lienzo(), interfaz->sistema(), dibujo);
    }
}

bool GameScene::ratonSobreInterfazNativa() const noexcept {
    for (const UIComponent* interfaz : interfacesNativas_) {
        if (interfaz && interfaz->sistema().hayInteractivoSobre()) return true;
    }
    return false;
}

void GameScene::pintarVistaInterfaz() {
    // Vista previa en la ventana dockable. En el modo juego el estado de los
    // scripts lo consume el HUD; fuera de el, la previa lo recibe (asi se ve lo
    // que fijan los scripts sin entrar en Play).
    GameObject* seleccionado =
        editorController ? editorController->getSelectedObject() : nullptr;
    overlayInterfaces_.pintarVistaPrevia(seleccionado, modoJuego ? nullptr : &estadoInterfaces_,
                                         deltaTime, WindowNames::VistaInterfaz);
}

void GameScene::pintarViewportsGUI() {
    if (!sceneRenderer) return;
    const auto& targets = sceneRenderer->viewportTargets();
    const auto& objetos = sceneRenderer->viewportObjects();
    for (size_t index = 0; index < targets.size(); ++index) {
        RenderTarget* target = targets[index].get();
        GameObject* objeto = objetos[index];
        if (!target || !objeto) continue;

        char title[96];
        // "##<id camara>" oculta el sufijo del id de la ventana (el titulo se
        // ve "Vista previa: <nombre>") y da un ID ESTABLE a ImGui: renombrar la
        // camara o tener dos con el mismo nombre ya no crea una ventana nueva
        // ni pierde la posicion/dock guardados en el imgui.ini del proyecto
        // (antes, "Vista previa: %s" hacia que cada rename mudara el id y la
        // ventana saltara de lugar).
        // El titulo se traduce y el ID ("###vistaPrevia<id>") no depende ni
        // del idioma ni del nombre de la camara.
        char visible[80];
        std::snprintf(visible, sizeof(visible), Localizacion::tr(Texto::VistaPreviaCamara),
                      objeto->inputName[0] != '\0' ? objeto->inputName
                                                   : Localizacion::tr(Texto::CamaraSinNombre));
        std::snprintf(title, sizeof(title), "%s###vistaPrevia%d", visible,
                      static_cast<int>(objeto->getId()));

        ImGui::SetNextWindowSize(
            ImVec2(static_cast<float>(target->getWidth()) + 16.f,
                   static_cast<float>(target->getHeight()) + 38.f),
            ImGuiCond_Once);
        ImGui::Begin(title);
        // La textura del FBO tiene origen abajo-izquierda; se voltea el UV
        // vertical ("v" invertida) para que la vista previa no quede dada
        // vuelta. La GUI no ve handles concretos: pide al backend el descriptor
        // opaco (en GL el GLuint viaja como puntero = ImTextureID).
        const void* imguiTex =
            backend_.imguiTextureId(target->getColorTexture());
        ImGui::Image(
            static_cast<ImTextureID>(reinterpret_cast<std::intptr_t>(imguiTex)),
            ImVec2(static_cast<float>(target->getWidth()),
                   static_cast<float>(target->getHeight())),
            ImVec2(0.f, 1.f), ImVec2(1.f, 0.f));

        // Soltar un archivo sobre la vista previa crea su objeto (sin
        // posicion: la vista previa no es la camara con la que se edita).
        ImGui::InvisibleButton(("##ViewportDropTarget" + std::to_string(index)).c_str(),
                               ImVec2(static_cast<float>(target->getWidth()),
                                      static_cast<float>(target->getHeight())));
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ARCHIVO_PATH")) {
                const char* ruta = static_cast<const char*>(payload->Data);
                if (ruta && editorController) {
                    const ArrastreArchivos::Resultado r = ArrastreArchivos::crearDesdeArchivo(
                        *editorController, ruta, ArrastreArchivos::Destino());
                    if (r.ok && r.objeto) editorController->selectObject(r.objeto);
                    else std::cerr << "[Escena] " << ruta << ": " << r.mensaje << '\n';
                }
            }
            ImGui::EndDragDropTarget();
        }
        ImGui::End();
    }
}

// Ventana unica para colocar camaras: crear una nueva (en la vista activa),
// elegir cual se usa para navegar/ver, prender o apagar su vista previa y
// eliminar. Ventana de inicio de la Fase 2: con "Agregar camara" aparece
// automaticamente la vista previa de cada camara.
void GameScene::pintarVentanaCamaras() {
    if (!ImGui::Begin(WindowNames::titulo(WindowNames::CameraList).c_str(), &ventanaCamarasAbierta)) {
        ImGui::End();
        return;
    }

    if (ImGui::Button(Localizacion::etiqueta(Texto::AgregarCamara).c_str())) {
        agregarCamaraEnVistaActiva();
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(
        Localizacion::tr(Texto::CreaUnaCamaraEnLa));

    ImGui::Separator();

    auto* gameObjects = getGameObjectsScene();
    if (gameObjects && !gameObjects->isEmpty()) {
        Position<GameObject*>* pos = gameObjects->first();
        while (pos && pos->getElement()) {
            GameObject* objeto = pos->getElement();
            if (CameraComponent* camara =
                    objeto->getComponent<CameraComponent>()) {
                const std::string nombre =
                    objeto->inputName[0] ? objeto->inputName : "Camara";
                const bool esActiva =
                    objeto == requestedActiveCamera ||
                    (requestedActiveCamera == nullptr &&
                     objeto == activeCameraObject);

                ImGui::PushID(static_cast<int>(objeto->getId()));
                if (ImGui::Selectable(nombre.c_str(), esActiva)) {
                    setActiveCamera(objeto);
                    // "Usar" una camara publica el cambio en el bus de GUI: la
                    // fachada (GUIManager) selecciona el objeto para el
                    // inspector y los demas suscriptores reaccionan sin que la
                    // escena conozca a las ventanas.
                    if (managerGUI) {
                        EditorEvent ev;
                        ev.type = EditorEventType::CamaraActivaCambio;
                        ev.camara = objeto;
                        managerGUI->getEditorEventBus()->publish(ev);
                    }
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", Localizacion::tr(Texto::UsarEstaCamaraNavegacionVista));
                }

                bool pintar = camara->getPintar();
                if (ImGui::Checkbox(Localizacion::etiqueta(Texto::VistaPrevia).c_str(), &pintar)) {
                    camara->setPintar(pintar);
                }
                ImGui::SameLine();
                if (ImGui::SmallButton(Localizacion::etiqueta(Texto::Eliminar).c_str())) {
                    GameObject* objetivo = objeto;
                    if (objetivo == requestedActiveCamera)
                        requestedActiveCamera = nullptr;
                    if (editorController) editorController->deleteGameObject(objetivo);
                    ImGui::PopID();
                    break; // la lista se muto; se cierra el paso por el frame
                }
                ImGui::PopID();
            }
            pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                               : nullptr;
        }
    }

    ImGui::End();
}
