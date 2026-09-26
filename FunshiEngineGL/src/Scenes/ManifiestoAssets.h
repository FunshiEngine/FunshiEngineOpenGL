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
#ifndef MANIFIESTO_ASSETS_H
#define MANIFIESTO_ASSETS_H

#include <string>

#include "Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

class GameObject;

// Manifiesto de assets de la escena (add-on de la serializacion binaria).
//
// El .db de cada objeto guarda sus paths de asset (mallas, texturas, dll de
// script) pero es binario, sin versionado y de lectura por-componenente; el
// manifiesto los centraliza en un JSON legible (Memory/Binarios/SceneAssets.json)
// que se regenera en cada guardado y se aplica al cargar con precedencia
// sobre el .db. Sirve de autoridad de rutas para mover/renombrar assets y
// habilitar el guardado en caliente (Ctrl+S), no de reemplazo del .db.
//
// Como RutasReescritura, opera sobre la lista lineal de GameObjects de la
// escena (GameScene::getGameObjectsScene) y no conoce la estructura interna
// de la escena: recorre cada objeto y sus componentes (Modelos3D, Model,
// Material y Script) para fotografiar sus rutas.
class ManifiestoAssets {
public:
    // Regenera el manifiesto completo de la escena. Las rutas se persisten
    // RELATIVAS a la raiz de assets del proyecto (src<nombre>), igual que en
    // el .db, de modo que mover/renombrar el proyecto no invalida la escena.
    static bool guardar(const std::string& rutaArchivo,
                        ListaDE<GameObject*>* objetos);

    // Aplica el manifiesto leido (si existe) sobre los objetos cargados:
    // absolutiza cada path y se lo asigna al componente solo cuando es no
    // vacio y distinto del que ya tiene (no recarga mallas/scripts iguales).
    // Devuelve true si al menos un path se aplico. Sin manifiesto en disco
    // devuelve false y la escena queda con los paths del .db (compat legacy).
    static bool cargar(const std::string& rutaArchivo,
                       ListaDE<GameObject*>* objetos);
};

#endif // MANIFIESTO_ASSETS_H