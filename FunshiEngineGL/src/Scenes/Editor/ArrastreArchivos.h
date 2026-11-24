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
#ifndef ARRASTREARCHIVOS_H
#define ARRASTREARCHIVOS_H

#include <glm/glm.hpp>
#include <string>

#include "../../Assets/Tipos/TiposArchivo.h"

class EditorController;
class GameObject;

// Que hace el editor con un archivo que se suelta en la escena o en el
// inspector. Toda la decision vive aca (no en las ventanas): el viewport, el
// inspector y la jerarquia solo entregan la ruta y donde se solto.
//
// Escena: se crea un objeto nuevo con el componente que corresponde al tipo
// del archivo (Prefab -> instancia, Modelo3D -> Model, Imagen -> SpriteRenderer,
// Audio -> AudioSource, Tileset -> Tilemap, PerfilParticulas ->
// EmisorParticulas, InterfazHtml -> InterfaceComponent, Script -> objeto con
// Script). La creacion pasa por el gestor de comandos (Ctrl+Z la deshace).
//
// Objeto: el archivo se aplica al objeto (reemplaza la ruta del componente que
// ya tiene, o le agrega el componente si no lo tiene); un prefab se instancia
// como hijo del objeto.
namespace ArrastreArchivos {

struct Destino {
    // Padre de lo creado (nullptr = raiz de la escena).
    GameObject* padre = nullptr;
    // Posicion en el mundo donde se solto (si se conoce).
    bool conPosicion = false;
    glm::vec3 posicion{0.f};
};

struct Resultado {
    bool ok = false;
    GameObject* objeto = nullptr;  // objeto creado o modificado
    std::string mensaje;           // motivo si no se pudo
};

// true si el tipo se puede soltar en la escena / sobre un objeto.
bool aceptaEnEscena(TipoArchivo tipo);
bool aceptaEnObjeto(TipoArchivo tipo);

Resultado crearDesdeArchivo(EditorController& editor, const std::string& ruta,
                            const Destino& destino);

Resultado aplicarAObjeto(EditorController& editor, GameObject& objeto,
                         const std::string& ruta);

}  // namespace ArrastreArchivos

#endif
