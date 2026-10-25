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
#include "AjustarVariableComando.h"
#include "Objetos/GameObject.h"
#include "Scenes/SceneRegistry.h"
#include <sstream>

AjustarVariableComando::AjustarVariableComando(EditorController* ec,
                                               SceneRegistry* sr, int idObjeto,
                                               std::string nombreComp,
                                               std::string nombreVar,
                                               Lector fnLector, Escritor fnEscritor,
                                               std::string valor)
    : editorController(ec), sceneRegistry(sr), objectId(idObjeto),
      nombreComponente(std::move(nombreComp)), nombreVariable(std::move(nombreVar)),
      lector(std::move(fnLector)), escritor(std::move(fnEscritor)),
      valorNuevo(std::move(valor)) {
    // El valor anterior se captura al construir el comando: es lo que devuelve
    // el deshacer. Si el componente no esta (o no se puede leer), el deshacer
    // no tendra nada que reponer.
    if (sceneRegistry && objectId > 0 && lector) {
        GameObject* obj = sceneRegistry->getObjectByID(objectId);
        if (obj) {
            if (Component* comp = obj->getComponentByName(nombreComponente))
                valorAnterior_ = lector(comp);
        }
    }
}

void AjustarVariableComando::ejecutar() {
    escribir(valorNuevo);
}

void AjustarVariableComando::deshacer() {
    escribir(valorAnterior_);
}

void AjustarVariableComando::escribir(const std::string& valor) {
    ultimoError_.clear();
    if (!sceneRegistry || objectId <= 0 || !escritor) return;
    GameObject* obj = sceneRegistry->getObjectByID(objectId);
    if (!obj) return;
    Component* comp = obj->getComponentByName(nombreComponente);
    if (!comp) return;
    std::string error;
    if (!escritor(comp, valor, error)) ultimoError_ = error;
}

std::string AjustarVariableComando::descripcion() const {
    std::ostringstream oss;
    oss << "Ajustar " << nombreVariable << " de " << nombreComponente << " en el objeto "
        << objectId;
    return oss.str();
}
