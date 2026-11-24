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
#include "SettingsObjectInterfaceDetalle.h"

SettingsObjectInterface::SettingsObjectInterface(GameObject* object,
                                                 bool stateGUI)
    : GeneralUserInterface("Settings", stateGUI, ImGuiWindowFlags_MenuBar) {
    this->object = object;
    listaDESettingsComponent = new ListaDE<SettingsComponent*>();
    if (object && !object->getComponents()->isEmpty()) {
        loadComponents();
    }
}

SettingsObjectInterface::~SettingsObjectInterface() {
    if (events && eventSubscription != 0) {
        events->unsubscribe(eventSubscription);
        eventSubscription = 0;
    }
    desvincular();
    delete listaDESettingsComponent;
}

void SettingsObjectInterface::setEditor(EditorController* editor) {
    this->editor = editor;
    // Si ya hay objeto inspeccionado, reconciliar para crear panel Prefab si falta.
    if (object && editor) {
        crearSettingsFaltantes();
    }
}

// Desvincula el inspector del objeto actual: libera los Settings* y deja
// object = nullptr. Se llama desde la suscripcion al bus y desde el destructor.
void SettingsObjectInterface::desvincular() {
    while (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
        delete pos->getElement();
        listaDESettingsComponent->remove(pos);
    }
    object = nullptr;
    tagBufferOwner_ = nullptr;
}

void SettingsObjectInterface::setEventBus(EventBus* bus) {
    if (events == bus) return;
    if (events && eventSubscription != 0) {
        events->unsubscribe(eventSubscription);
        eventSubscription = 0;
    }
    events = bus;
    if (events) {
        eventSubscription = events->subscribe([this](const SceneEvent& event) {
            if (event.type == SceneEventType::ObjectDeleted) {
                if (event.object == object) {
                    desvincular();
                }
            } else if (event.type == SceneEventType::SceneCleared) {
                desvincular();
            } else if (event.type == SceneEventType::ComponentStructureChanged) {
                if (event.object == object && !iterandoComponentes) {
                    // Solo el alta/baja de un componente invalida paneles. Un
                    // cambio de propiedad no llega por este evento.
                    reconciliarComponentes();
                }
                // Si estamos iterando, el caller (contentGUI) se encarga del reload diferido
            }
        });
    }
}

bool SettingsObjectInterface::tieneSettingsPara(Component* componente) {
    if (!componente || listaDESettingsComponent->isEmpty()) return false;
    Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
    while (pos != nullptr) {
        SettingsComponent* s = pos->getElement();
        if (s != nullptr && s->getComponent() == componente) return true;
        pos = (pos != listaDESettingsComponent->last())
                  ? listaDESettingsComponent->next(pos)
                  : nullptr;
    }
    return false;
}

// Borra los Settings cuyo componente ya no pertenece al objeto inspeccionado.
// Usa el nombre de tipo almacenado en SettingsComponent (estable en construccion)
// para evitar desreferenciar punteros colgantes.
void SettingsObjectInterface::purgarSettingsHuerfanos() {
    if (!object || listaDESettingsComponent->isEmpty()) return;
    ListaDE<Component*>* componentes = object->getComponents();
    if (!componentes || componentes->isEmpty()) {
        // Si no hay componentes, todos los settings son huerfanos
        while (!listaDESettingsComponent->isEmpty()) {
            Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
            delete pos->getElement();
            listaDESettingsComponent->remove(pos);
        }
        return;
    }

    Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
    while (pos != nullptr) {
        Position<SettingsComponent*>* siguiente =
            (pos != listaDESettingsComponent->last())
                ? listaDESettingsComponent->next(pos)
                : nullptr;
        SettingsComponent* s = pos->getElement();
        if (!s) {
            listaDESettingsComponent->remove(pos);
            pos = siguiente;
            continue;
        }

        // Verificar si existe un componente de este tipo en el objeto
        const std::string& tipoEsperado = s->getTipoComponente();
        bool vigente = false;
        if (!tipoEsperado.empty()) {
            Position<Component*>* pc = componentes->first();
            while (pc != nullptr) {
                Component* c = pc->getElement();
                if (c) {
                    std::string tipoActual = demangle(typeid(*c).name());
                    if (tipoActual == tipoEsperado) {
                        vigente = true;
                        break;
                    }
                }
                pc = (pc != componentes->last())
                         ? componentes->next(pc)
                         : nullptr;
            }
        }

        if (!vigente) {
            listaDESettingsComponent->remove(pos);
            delete s;
        }
        pos = siguiente;
    }
}

// Reconciliacion completa: reutiliza los paneles vigentes, borra los huerfanos
// y crea los que falten. Unico punto que incrementa el contador de
// reconstrucciones (un cambio estructural por vez).
void SettingsObjectInterface::reconciliarComponentes() {
    if (!object) return;
    ++reconciliaciones;
    purgarSettingsHuerfanos();
    crearSettingsFaltantes();
}

void SettingsObjectInterface::loadComponents() {
    if (!object) return;
    purgarSettingsHuerfanos();
    crearSettingsFaltantes();
}

void SettingsObjectInterface::crearSettingsFaltantes() {
    if (!object) return;
    const int creadosAntes = listaDESettingsComponent->tam();

    ContextoPanel contexto;
    contexto.object = object;
    contexto.editor = editor;
    contexto.audio = audioMotor;
    contexto.dimension = dimensionProyecto();
    contexto.preview = previewMaterial_;

    ListaDE<Component*>* componentes = object->getComponents();
    if (componentes && !componentes->isEmpty()) {
        // Se recorre el catalogo (no el orden de insercion en el objeto) para
        // que el orden de los paneles sea estable y definido en un solo lugar.
        for (const DescriptorComponente& descriptor :
             CatalogoComponentes::instancia().descriptores()) {
            if (!descriptor.crearPanel) continue;
            Position<Component*>* posicion = componentes->first();
            while (posicion != nullptr) {
                Component* componente = posicion->getElement();
                if (componente != nullptr &&
                    demangle(typeid(*componente).name()) == descriptor.nombre &&
                    !tieneSettingsPara(componente)) {
                    std::unique_ptr<SettingsComponent> panel =
                        descriptor.crearPanel(componente, object, contexto);
                    if (panel)
                        listaDESettingsComponent->addLast(panel.release());
                }
                posicion = (posicion != componentes->last())
                               ? componentes->next(posicion)
                               : nullptr;
            }
        }
    }

    creacionesSettings += static_cast<size_t>(
        listaDESettingsComponent->tam() - creadosAntes);
}

GameObject* SettingsObjectInterface::getObjectInInspector() { return object; }

SettingsComponent* SettingsObjectInterface::settingsEnIndice(size_t indice) {
    if (listaDESettingsComponent->isEmpty()) return nullptr;
    size_t i = 0;
    Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
    while (pos != nullptr) {
        if (i == indice) return pos->getElement();
        ++i;
        pos = (pos != listaDESettingsComponent->last())
                  ? listaDESettingsComponent->next(pos)
                  : nullptr;
    }
    return nullptr;
}

void SettingsObjectInterface::setTargetObject(GameObject* newObject) {
    if (object == newObject || newObject == nullptr) return;
    object = newObject;
    tagBufferOwner_ = nullptr;
    while (!listaDESettingsComponent->isEmpty()) {
        Position<SettingsComponent*>* pos = listaDESettingsComponent->first();
        delete pos->getElement();
        listaDESettingsComponent->remove(pos);
    }
    loadComponents();
}
