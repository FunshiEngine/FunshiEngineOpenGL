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
#include "RutasReescritura.h"

#include <functional>

#include "Configuracion/EditorConfig.h"
#include "Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "Objetos/Componentes/Material.h"
#include "Objetos/Componentes/Model.h"
#include "Objetos/Componentes/Script.h"
#include "Objetos/GameObject.h"
#include "Objetos/Modelos3D.h"

namespace {

// Reescribe la ruta de un asset cuyo prefijo coincida con `anterior`; aplica
// el reemplazo via `asignar` y suma las ocurrencias. El lambda de escritura
// no toca el objeto si el cambio exige recompilar (Script usa setDllPath, que
// invalida la carga y refresca el nombre de clase); el resto de componentes
// solo almacenan el string.
int reescribirPath(const std::string& ruta, const std::string& anterior,
                   const std::string& reemplazo,
                   const std::function<void(const std::string&)>& asignar) {
    if (ruta.empty()) return 0;
    const std::string recalculada =
        EditorConfig::reemplazarPrefijoRuta(ruta, anterior, reemplazo);
    if (recalculada.empty() || recalculada == ruta) return 0;
    asignar(recalculada);
    return 1;
}

// Reescribe las referencias de asset de un solo componente (lista no
// propietaria: la iteracion no la libera).
int reescribirComponente(Component& componente, const std::string& anterior,
                         const std::string& reemplazo) {
    int cambios = 0;
    // Los cuatro slots de textura del material: cada descriptor es una ruta
    // independiente susceptible de caer bajo el prefijo movido/renombrado.
    if (auto* material = dynamic_cast<Material*>(&componente)) {
        cambios += reescribirPath(material->getDiffuseMapPath(), anterior,
                                  reemplazo, [material](const std::string& p) {
                                      material->setDiffuseMapPath(p);
                                  });
        cambios += reescribirPath(material->getSpecularMapPath(), anterior,
                                  reemplazo, [material](const std::string& p) {
                                      material->setSpecularMapPath(p);
                                  });
        cambios += reescribirPath(material->getNormalMapPath(), anterior,
                                  reemplazo, [material](const std::string& p) {
                                      material->setNormalMapPath(p);
                                  });
        cambios += reescribirPath(material->getEmissionMapPath(), anterior,
                                  reemplazo, [material](const std::string& p) {
                                      material->setEmissionMapPath(p);
                                  });
        return cambios;
    }
    if (auto* model = dynamic_cast<Model*>(&componente)) {
        return reescribirPath(model->getPath(), anterior, reemplazo,
                              [model](const std::string& p) { model->setPath(p); });
    }
    if (auto* script = dynamic_cast<Script*>(&componente)) {
        return reescribirPath(script->getPath(), anterior, reemplazo,
                              [script](const std::string& p) { script->setDllPath(p); });
    }
    return cambios;
}

// Recorre los componentes de un GameObject (la lista es propiedad del objeto,
// no se libera aqui).
int reescribirObjeto(GameObject& objeto, const std::string& anterior,
                     const std::string& reemplazo) {
    int cambios = 0;

    // El objeto puede ser en si un Modelos3D: su ruta de malla se guarda en
    // el propio GameObject (no en un componente Model).
    if (auto* modelo = dynamic_cast<Modelos3D*>(&objeto)) {
        cambios += reescribirPath(modelo->getPath(), anterior, reemplazo,
                                  [modelo](const std::string& p) {
                                      modelo->setPath(p);
                                  });
    }

    auto* componentes = objeto.getComponents();
    if (componentes && !componentes->isEmpty()) {
        auto* pos = componentes->first();
        while (pos != nullptr) {
            if (Component* componente = pos->getElement(); componente != nullptr)
                cambios +=
                    reescribirComponente(*componente, anterior, reemplazo);
            pos = (pos == componentes->last()) ? nullptr : componentes->next(pos);
        }
    }
    return cambios;
}

} // namespace

int RutasReescritura::reescribirEnEscena(ListaDE<GameObject*>* objetos,
                                         const std::string& anterior,
                                         const std::string& reemplazo) {
    if (objetos == nullptr || objetos->isEmpty() || anterior.empty() ||
        reemplazo.empty())
        return 0;

    int cambios = 0;
    auto* pos = objetos->first();
    while (pos != nullptr) {
        if (GameObject* objeto = pos->getElement(); objeto != nullptr)
            cambios += reescribirObjeto(*objeto, anterior, reemplazo);
        pos = (pos == objetos->last()) ? nullptr : objetos->next(pos);
    }
    return cambios;
}