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
#ifndef RUTAS_REESCRITURA_H
#define RUTAS_REESCRITURA_H

#include <string>

template <typename E> class ListaDE;
class GameObject;

// Mantenimiento de las rutas de asset de la escena. Recorre la lista lineal
// de entidades (GameScene::getGameObjectsScene) y opera sobre cada modelo /
// textura / fuente de script de cada objeto. No toca nada fuera de la escena:
// interfaz, panel, etc. se referencian por nombre, no por ruta.
//
// Dos operaciones:
//  - reescribirEnEscena: el usuario movio o renombro un archivo/carpeta dentro
//    del explorador; para cada referencia cuya ruta cae bajo el prefijo
//    anterior, reescribe el prefijo por el nuevo. Devuelve cuantas cambio
//    (0 = nada que hacer).
//  - sanarRutasInexistentes: la escena guardada tiene referencias que ya no
//    resuelven (daño anterior al fix de separadores, o archivos movidos fuera
//    del motor). Para cada ruta rota busca el nombre base bajo la raiz de
//    assets: con UNA coincidencia unica la repara y la registra en el log;
//    con varias o ninguna NO adivina y la deja como esta, con aviso.
class RutasReescritura {
public:
    static int reescribirEnEscena(ListaDE<GameObject*>* objetos,
                                  const std::string& anterior,
                                  const std::string& reemplazo);

    // Devuelve cuantas referencias reparo (0 = nada que sanar). Sin raiz de
    // assets fijada no hace nada, porque no hay donde buscar.
    static int sanarRutasInexistentes(ListaDE<GameObject*>* objetos);
};

#endif // RUTAS_REESCRITURA_H