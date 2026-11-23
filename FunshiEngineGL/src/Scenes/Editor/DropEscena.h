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
#ifndef DROPESCENA_H
#define DROPESCENA_H

class CameraComponent;
class EditorController;
class GameObject;
class GizmoController;
template <typename T>
class ListaDE;

// Soltar archivos del explorador de contenido sobre la escena (viewport).
//
// La escena no es una ventana de ImGui: se ve a traves del nodo central del
// dock (PassthruCentralNode), que deja pasar el mouse, asi que ahi no hay una
// ventana que pueda ser destino de drag & drop. En su lugar se mira el payload
// activo: si hay un ARCHIVO_PATH en vuelo y el mouse no esta sobre ninguna
// ventana, el mouse esta sobre la escena.
//
// Al soltar: si debajo del mouse hay un objeto y el archivo se puede aplicar a
// un objeto existente (script), se aplica a ese objeto; si no, se crea un
// objeto nuevo en el punto del plano de juego bajo el mouse (z = 0 en 2D,
// y = 0 en 3D) y queda seleccionado.
namespace DropEscena {

void procesar(EditorController& editor, CameraComponent* camara,
              ListaDE<GameObject*>* objetos, GizmoController* picking, bool modo2D);

}  // namespace DropEscena

#endif
