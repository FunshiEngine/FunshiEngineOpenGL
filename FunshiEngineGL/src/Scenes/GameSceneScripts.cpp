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

void GameScene::encolarScriptsIniciales() {
    auto* objs = getGameObjectsScene();
    if (!objs || objs->isEmpty()) return;

    bool hayPendientes = false;
    Position<GameObject*>* pos = objs->first();
    while (pos && pos->getElement()) {
        ListaDE<Component*>* componentes = pos->getElement()->getComponents();
        if (componentes && !componentes->isEmpty()) {
            Position<Component*>* componente = componentes->first();
            while (componente) {
                if (Script* s =
                        dynamic_cast<Script*>(componente->getElement())) {
                    if (s->necesitaCompilar()) {
                        colaCompilacion_.push_back({s, pos->getElement()});
                        s->setAplazarCarga(true);
                        hayPendientes = true;
                    }
                }
                componente = componente != componentes->last()
                                 ? componentes->next(componente)
                                 : nullptr;
            }
        }
        pos = (pos != objs->last()) ? objs->next(pos) : nullptr;
    }
    if (!hayPendientes) {
        compilacionEnCurso_ = false;
        cargaActual_.clear();
        cargaTotal_ = 0;
        cargaHecha_ = 0;
        return;
    }

    resultadosCarga_.clear();
    faseCarga_ = FaseCarga::Mostrar;
    indiceCarga_ = 0;
    cargaTotal_ = colaCompilacion_.size();
    cargaHecha_ = 0;
    compilacionEnCurso_ = true;
    cargaActual_ = colaCompilacion_[0].script->rutaFuente();
}

void GameScene::procesarColaCompilacion() {
    if (!start) {
        limpiarColaCompilacion();
        return;
    }

    // Recoleccion continua: un script editado a mitad de play tambien pasa por
    // la cola, asi su compilacion se ve en la barra de estado.
    if (colaCompilacion_.empty()) {
        auto* objs = getGameObjectsScene();
        if (!objs || objs->isEmpty()) return;
        bool hay = false;
        Position<GameObject*>* pos = objs->first();
        while (pos && pos->getElement()) {
            ListaDE<Component*>* componentes = pos->getElement()->getComponents();
            if (componentes && !componentes->isEmpty()) {
                Position<Component*>* componente = componentes->first();
                while (componente) {
                    if (Script* s =
                            dynamic_cast<Script*>(componente->getElement())) {
                        if (s->necesitaCompilar()) {
                            colaCompilacion_.push_back({s, pos->getElement()});
                            s->setAplazarCarga(true);
                            hay = true;
                        }
                    }
                    componente = componente != componentes->last()
                                     ? componentes->next(componente)
                                     : nullptr;
                }
            }
            pos = (pos != objs->last()) ? objs->next(pos) : nullptr;
        }
        if (hay) {
            faseCarga_ = FaseCarga::Mostrar;
            indiceCarga_ = 0;
            cargaTotal_ = colaCompilacion_.size();
            cargaHecha_ = 0;
            compilacionEnCurso_ = true;
            cargaActual_ = colaCompilacion_[0].script->rutaFuente();
        }
        return;
    }

    // Fase 1 (Mostrar): este frame solo informa cual script se va a compilar;
    // el dibujado al final del frame muestra la barra con su progreso.
    if (faseCarga_ == FaseCarga::Mostrar) {
        cargaActual_ = colaCompilacion_[indiceCarga_].script->rutaFuente();
        faseCarga_ = FaseCarga::Compilar;
        return;
    }

    // Fase 2 (Compilar): ejecuta la compilacion sincronica del iesimo script.
    {
        const CargaPendiente& item = colaCompilacion_[indiceCarga_];
        item.script->setAplazarCarga(false);
        ScriptRuntime::ResultadoCarga r;
        r.nombre = item.script->rutaFuente();
        r.ok = item.script->aplicarCarga(item.owner);
        r.mensaje = item.script->ultimoError();
        resultadosCarga_.push_back(std::move(r));
        ++cargaHecha_;
        ++indiceCarga_;
    }
    if (indiceCarga_ >= colaCompilacion_.size()) {
        limpiarColaCompilacion();
        compilacionEnCurso_ = false;
        cargaActual_.clear();
    } else {
        faseCarga_ = FaseCarga::Mostrar;
        cargaActual_ = colaCompilacion_[indiceCarga_].script->rutaFuente();
    }
}

void GameScene::limpiarColaCompilacion() {
    for (const CargaPendiente& item : colaCompilacion_)
        if (item.script) item.script->setAplazarCarga(false);
    colaCompilacion_.clear();
    indiceCarga_ = 0;
    faseCarga_ = FaseCarga::Mostrar;
    compilacionEnCurso_ = false;
    cargaActual_.clear();
}

void GameScene::descargarScripts() {
    limpiarColaCompilacion();
    auto* gameObjects = getGameObjectsScene();
    if (!gameObjects || gameObjects->isEmpty()) return;
    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        ListaDE<Component*>* componentes = pos->getElement()->getComponents();
        if (componentes && !componentes->isEmpty()) {
            Position<Component*>* componente = componentes->first();
            while (componente) {
                if (Script* script =
                        dynamic_cast<Script*>(componente->getElement()))
                    script->liberarComportamiento();
                componente = componente != componentes->last()
                                 ? componentes->next(componente)
                                 : nullptr;
            }
        }
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}
