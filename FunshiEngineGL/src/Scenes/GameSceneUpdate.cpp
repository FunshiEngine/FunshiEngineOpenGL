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

void GameScene::emitirParticulas(float deltaTime) {
    if (deltaTime <= 0.0f) return;
    auto* objetos = getGameObjectsScene();
    if (!objetos || objetos->isEmpty()) return;
    Position<GameObject*>* posicion = objetos->first();
    while (posicion && posicion->getElement()) {
        GameObject* objeto = posicion->getElement();
        posicion = (posicion != objetos->last()) ? objetos->next(posicion)
                                                  : nullptr;
        if (!objeto) continue;
        EmisorParticulas* emisor = objeto->getComponent<EmisorParticulas>();
        if (!emisor) continue;
        // La transformacion global se calcula en un buffer del objeto: la
        // posicion se copia de inmediato (el emisor la usa ya).
        Transform* global = objeto->getGlobalTransform();
        if (!global) continue;
        emisor->emitir(sistemaParticulas, global->getTranslatef(), deltaTime);
    }
}

void GameScene::update(float value) {
    deltaTime = value;

    // HUD nativo: se recoge el raton y se actualiza el estado de los widgets
    // antes de que corran los scripts, para que un script vea el clic en el
    // mismo frame en que ocurre. Fuera del modo juego no hay nada que
    // recoger (la funcion vacia la lista y sale).
    actualizarInterfazNativa();

    // Particulas: primero se mueven las que viven y despues los emisores
    // sueltan las nuevas. Corre en el editor y en juego (a diferencia de los
    // componentes, que solo avanzan en play), asi que un efecto se ve mientras
    // se edita.
    sistemaParticulas.actualizar(value);
    emitirParticulas(value);
    if (resetSimulacionPendiente && start) {
        resetSimulacionPendiente = false;
        if (!snapshotSimulacionDisponible()) {
            std::cerr << "[escena] no se pudo restaurar el estado inicial de "
                         "la simulacion; la ejecucion continua sin cambios\n";
            inputScripts.avanzarFrame();
            return;
        }
        limpiarRuntimeSimulacion();
        if (restaurarSnapshotSimulacion()) {
            previousStart = false;
            std::cout << "[escena] simulacion restaurada al estado inicial\n";
        } else {
            std::cerr << "[escena] no se pudo restaurar el estado inicial de "
                         "la simulacion\n";
        }
        inputScripts.avanzarFrame();
        return;
    }
    // Mientras se manipula el gizmo NO se avanza la simulacion: de lo
    // contrario la gravedad/contactos eyectan el cuerpo y la sync de vuelta
    // arrastra al objeto (efecto 'sale disparado'). Al soltar, la sim sigue.
    // La fisica SOLO corre en modo play (start==true); en editor (start==false)
    // stepSimulation no tiene consumidor (syncPhysicsToGameObject no escribe),
    // y solo causa explosiones por penetracion con el suelo (plano y=-1).

    // Transicion editor->simulacion: el cuerpo fue creado en una pose PASADA (al
    // agregar el RigidBody o al ultimo sync). Mientras estuvo en pausa el
    // usuario pudo mover el objeto o el offset del collider; si el primer
    // stepSimulation corre con el body viejo, la sync de vuelta escribe la
    // pose del collider desde una posicion descartada. Se empuja el body a
    // la pose VISUAL actual antes de arrancar.
    if (start && !previousStart) {
        inputScripts.reset();
        if (directorioSnapshot_.empty()) guardarSnapshotSimulacion();
        // El undo guarda ids de edicion: al entrar en play la escena puede
        // mutar y al salir se restaura el snapshot (los ids caducan). Sin
        // limpiar, un Ctrl+Z posterior operaria sobre objetos ajenos.
        if (editorController && editorController->getGestorComandos())
            editorController->getGestorComandos()->limpiar();
        // Feedback visual inmediato: aunque no haya nada que recompilar, se ve
        // que "Activar" disparo la carga/verificacion de scripts.
        overlayProgresoVisible_ = true;
        overlayProgresoTimer_ = kOverlayProgresoMin;
        overlayResultadoPendiente_ = true;
        // Los campos de SerializeField que referencian GameObjects se guardan
        // por nombre; aca se configura el resolver hacia los objetos de ESTA
        // escena (valido mientras se construye el arbol de valores).
        auto* gameObjects = getGameObjectsScene();
        if (!gameObjects->isEmpty()) {
            Position<GameObject*>* pos = gameObjects->first();
            while (pos && pos->getElement()) {
                if (RigidBody* body =
                        pos->getElement()->getComponent<RigidBody>())
                    body->syncGameObjectToPhysics();
                pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                                   : nullptr;
            }
        }

        // Audio: inyectar el motor y disparar los AudioSource automaticos.
        sincronizarAudioPlay(true);

        // Scripts: cablear los servicios de escena (audio, busqueda, teclado)
        // a la tabla que consultan los comportamientos via `servicios->...`.
        MotorScript::inyectarServiciosScript(audioEngine.get(),
                                              sceneRegistry.get(),
                                              &inputScripts, phisics.get(),
                                              editorController.get(),
                                              &estadoInterfaces_);

        // Todos los scripts que necesitan (re)compilarse entran a la cola: su
        // progreso se ve en la barra "Estado" antes de bloquear con g++/javac.
        encolarScriptsIniciales();

        std::cout << "[escena] simulacion iniciada: audio, servicios de "
                     "scripts y cola de compilacion armados"
                  << std::endl;
    }

    // Al terminar: avisar a los scripts para que hagan limpieza (onStop) y
    // restaurar el estado previo a cualquiera de los modos de simulacion.
    if (previousStart && !start) {
        limpiarRuntimeSimulacion();
        if (!restaurarSnapshotSimulacion()) {
            std::cerr << "[escena] no se pudo restaurar el estado inicial "
                         "al terminar la simulacion\n";
        }
        eliminarSnapshotSimulacion();
        // Como al entrar: la restauracion reasigna el mundo y los ids
        // guardados en el historial ya no corresponden.
        if (editorController && editorController->getGestorComandos())
            editorController->getGestorComandos()->limpiar();

        std::cout << "[escena] simulacion detenida: audio cortado, servicios "
                     "desconectados y scripts avisados con onStop"
                  << std::endl;
    }
    previousStart = start;

    // La fisica y los scripts SOLO avanzan en modo play (start==true) y sin
    // pausa (F6): con simulacionPausada congelada se congela el motor pero la
    // GUI/editor sigue, para reanudar desde el mismo frame.
    if (phisics && start && !gizmoInUse() && !simulacionPausada) {
        phisics->stepSimulation(value);
        const auto notificar = [](Collider* propio, Collider* otro,
                                  TipoContacto tipo) {
            if (!propio || !otro) return;
            GameObject* owner = propio->getOwner();
            ListaDE<Component*>* componentes =
                owner ? owner->getComponents() : nullptr;
            if (!componentes || componentes->isEmpty()) return;
            Position<Component*>* componente = componentes->first();
            while (componente) {
                if (Script* script =
                        dynamic_cast<Script*>(componente->getElement()))
                    script->notificarContacto(owner, propio, otro, tipo);
                componente =
                    componente != componentes->last()
                        ? componentes->next(componente)
                        : nullptr;
            }
        };
        for (const EventoContacto& evento : phisics->tomarEventosContacto()) {
            notificar(evento.colliderA, evento.colliderB, evento.tipo);
            notificar(evento.colliderB, evento.colliderA, evento.tipo);
        }
    }

    // Sincronizar la fisica de vuelta a los GameObjects del mundo
    // (GameObject::update escribe en los Transforms via RigidBody).
    procesarColaCompilacion();
    if (start && !gizmoInUse() && !simulacionPausada) {
        auto* gameObjects = getGameObjectsScene();
        if (!gameObjects->isEmpty()) {
            Position<GameObject*>* pos = gameObjects->first();
            while (pos && pos->getElement()) {
                pos->getElement()->update(value);
                pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                                   : nullptr;
            }
        }
        // Altas/bajas de objetos pedidas por scripts: tras el recorrido (crear
        // o borrar adentro reconstruiria la vista e invalidaria el iterador).
        MotorScript::procesarPeticionesObjetos();
    }

    // Overlay de carga de scripts: mientras hay trabajo en cola se mantiene
    // visible un minimo; al terminar deja un aviso breve con el resultado.
    if (compilacionEnCurso_ && !overlayEnCursoPrev_)
        overlayResultadoPendiente_ = true;
    overlayEnCursoPrev_ = compilacionEnCurso_;

    if (compilacionEnCurso_) {
        overlayProgresoTimer_ = kOverlayProgresoMin;
        overlayResultadoTimer_ = 0.0f;
        overlayProgresoVisible_ = true;
        overlayResultadoVisible_ = false;
    } else if (overlayProgresoTimer_ > 0.0f) {
        overlayProgresoTimer_ -= value;
        if (overlayProgresoTimer_ <= 0.0f) {
            overlayProgresoVisible_ = false;
            if (overlayResultadoPendiente_) {
                overlayResultadoTimer_ = kOverlayResultadoSeg;
                overlayResultadoVisible_ = true;
                overlayResultadoPendiente_ = false;
            }
        }
    } else if (overlayResultadoTimer_ > 0.0f) {
        overlayResultadoTimer_ -= value;
        if (overlayResultadoTimer_ <= 0.0f) overlayResultadoVisible_ = false;
    }

    // Publicar el estado de la compilacion para la barra "Estado" (se dibuja
    // al final del frame, por eso el "Mostrar" de la cola es visible).
    if (managerGUI)
        managerGUI->getStatusBarGUI()->setEstadoCompilacion(
            compilacionEnCurso_, cargaActual_, cargaHecha_, cargaTotal_,
            resultadosCarga_, overlayProgresoVisible_,
            overlayResultadoVisible_);
    inputScripts.avanzarFrame();
}

void GameScene::registrarTeclaScript(int key, int action) {
    inputScripts.onKey(key, action);
    // La interfaz web con foco (la que esta bajo el mouse) tambien la recibe.
    overlayInterfaces_.tecla(key, action);
}

void GameScene::registrarMouseScript(double x, double y) {
    inputScripts.onMouseMove(x, y);
}
