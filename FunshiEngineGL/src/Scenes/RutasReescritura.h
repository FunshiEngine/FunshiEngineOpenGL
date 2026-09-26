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

// Actualiza las referencias de la escena despues de que el usuario mueve o
// renombra un archivo/carpeta dentro del explorador. Recorre la lista lineal
// de entidades (GameScene::getGameObjectsScene) y, para cada modelo / textura
// / fuente de script cuya ruta cae bajo el prefijo anterior, reescribe el
// prefijo por el nuevo. Devuelve el numero total de referencias reescritas
// (0 = nada que hacer). No toca nada fuera de la escena: interfaz, panel, etc.
// se referencian por nombre, no por ruta.
class RutasReescritura {
public:
    static int reescribirEnEscena(ListaDE<GameObject*>* objetos,
                                  const std::string& anterior,
                                  const std::string& reemplazo);
};

#endif // RUTAS_REESCRITURA_H