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
#ifndef PREFAB_H
#define PREFAB_H

#include <string>
#include <vector>
#include <memory>

class GameObject;
class SceneRegistry;
class EditorController;

// Prefab: serializa un arbol de GameObject (raiz + hijos) a un archivo .prefab
// y permite instanciarlo multiples veces en la escena. El formato es el mismo
// binario que la serializacion de escenas (preorden con marcadores), pero
// solo guarda el subarbol del prefab, no la escena completa.
//
// Uso:
//   Prefab prefab("MiPrefab");
//   prefab.guardarDesdeObjeto(objetoRaiz, editor);  // serializa el subarbol
//   GameObject* instancia = prefab.instanciar(editor, padre);  // crea copia en escena

class Prefab {
public:
    explicit Prefab(const std::string& nombre);
    ~Prefab() = default;

    // Serializa el subarbol cuyo nodo raiz es `raiz` (incluye raiz + todos
    // sus descendientes). `editor` se usa para acceder a la fisica/assetManager
    // si hiciera falta durante la serializacion.
    bool guardarDesdeObjeto(GameObject* raiz, EditorController* editor);

    // Instancia el prefab en la escena bajo `padre` (nullptr = raiz de la
    // escena). Devuelve la raiz de la instancia creada (con IDs nuevos).
    // Los tags y nombres se conservan; los IDs se reasignan.
    GameObject* instanciar(EditorController* editor, GameObject* padre = nullptr);

    const std::string& getNombre() const noexcept { return nombre_; }
    const std::string& getRutaArchivo() const noexcept { return rutaArchivo_; }
    bool existeArchivo() const noexcept;

private:
    std::string nombre_;
    std::string rutaArchivo_;

    std::string rutaParaNombre(const std::string& nombre) const;
};

#endif