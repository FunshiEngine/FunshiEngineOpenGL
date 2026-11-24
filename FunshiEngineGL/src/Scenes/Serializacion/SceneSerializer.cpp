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
#include "SceneSerializerDetalle.h"

namespace {

void recolectarIdsRecursivo(SceneRegistry* scene,
                           Position<GameObject*>* root,
                           std::unordered_set<int>& ids) {
    if (!scene || !root)
        return;
    GameObject* object = root->getElement();
    if (object) {
        ids.insert(object->getId());
    }
    if (!scene->getEntitysTree()->isInternal(root))
        return;
    auto* children = scene->getEntitysTree()->childsOf(root);
    if (!children)
        return;
    auto* position = children->first();
    while (position != nullptr) {
        recolectarIdsRecursivo(scene, position->getElement(), ids);
        position = (position != children->last()) ? children->next(position) : nullptr;
    }
    delete children;
}

void limpiarBinariosHuérfanos(SceneRegistry* scene,
                             const std::string& filename) {
    if (!scene)
        return;
    std::unordered_set<int> idsActuales;
    if (!scene->getEntitysTree()->isEmpty()) {
        recolectarIdsRecursivo(scene,
                               scene->getEntitysTree()->rootOfTree(),
                               idsActuales);
    }

    std::filesystem::path dirBinarios = std::filesystem::path(filename);
    if (dirBinarios.extension() == ".txt") {
        dirBinarios = dirBinarios.parent_path();
    } else if (!dirBinarios.empty() && dirBinarios.filename().string() == "Scene") {
        // filename puede ser el prefijo sin /Scene al final? Revisar: rutaScenePrefijo devuelve .../Scene
        // o puede ser .../Scene/
    }
    // Asegurar que apunte al directorio Scene
    if (dirBinarios.filename().string() != "Scene") {
        // Intentar añadir /Scene
        std::filesystem::path candidato = dirBinarios / "Scene";
        if (std::filesystem::exists(candidato)) {
            dirBinarios = candidato;
        } else {
            // filename puede ya ser .../Memory/Binarios/Scene (sin / final)
            // o .../Memory/Binarios/Scene/
        }
    }

    std::error_code ec;
    if (!std::filesystem::exists(dirBinarios, ec) || ec) {
        return;
    }
    if (!std::filesystem::is_directory(dirBinarios, ec) || ec) {
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dirBinarios, ec)) {
        if (ec)
            break;
        if (!entry.is_regular_file(ec) || ec)
            continue;
        const std::string nombreArchivo = entry.path().filename().string();
        if (nombreArchivo.size() < 8)
            continue;
        if (nombreArchivo.rfind("ObjectN", 0) != 0)
            continue;
        if (nombreArchivo.find(".db") == std::string::npos)
            continue;
        // Extraer ID
        std::string texto = nombreArchivo.substr(7);
        const std::size_t finDb = texto.find(".db");
        if (finDb == std::string::npos)
            continue;
        texto = texto.substr(0, finDb);
        if (texto.empty())
            continue;
        // Validar que son solo dígitos
        bool soloDigitos = true;
        for (char c : texto) {
            if (c < '0' || c > '9') {
                soloDigitos = false;
                break;
            }
        }
        if (!soloDigitos)
            continue;
        int idArchivo = 0;
        try {
            idArchivo = std::stoi(texto);
        } catch (...) {
            continue;
        }
        if (idArchivo == 0)
            continue; // Nunca borrar ObjectN0.db
        if (idsActuales.find(idArchivo) == idsActuales.end()) {
            std::error_code ecRemove;
            if (!std::filesystem::remove(entry.path(), ecRemove)) {
                std::cerr << "[escena] no se pudo eliminar binario huérfano: "
                          << entry.path() << " (" << ecRemove.message() << ")\n";
            } else {
                std::cerr << "[escena] eliminado binario huérfano: "
                          << entry.path() << "\n";
            }
        }
    }
}

}  // namespace

SceneSerializer::SceneSerializer(SceneRegistry* value,
                                 EditorController* controller,
                                 AssetManager* assetsManager,
                                 RegistroDiagnosticos* registro)
    : scene(value),
      editor(controller),
      assets(assetsManager),
      diagnosticos(registro) {
}

void SceneSerializer::savePreOrder(
    Position<GameObject*>* root,
    std::ofstream& archive,
    const std::string& filename)
{
    if (!root || !scene || !archive.is_open())
        return;

    GameObject* object = root->getElement();

    if (!object)
        return;

    /*
     * H-17: un hijo con id=0 escribiria ObjectN0.db (pisando el binario de la
     * RAIZ, que se guarda primero) y dejaria una segunda linea "ObjectN0.db"
     * en el indice: al recargar, esa linea en posicion de hijo lee la raiz y
     * siembra un hijo fantasma "Scene". Se reasigna un id libre antes de
     * escribir nada, sin perder el objeto.
     */
    if (object->getId() == 0 && object != scene->getRoot()) {
        const int reasignado = scene->siguienteIdDisponible();
        std::cerr << "[escena] hijo con id=0 al guardar; se reasigna id="
                  << reasignado << " para no pisar ObjectN0.db ('"
                  << object->inputName << "')\n";
        object->setId(reasignado);
    }

    /*
     * ============================================================
     * PREORDEN
     * ============================================================
     *
     * Primero se procesa el objeto actual.
     * Después se procesan sus hijos.
     *
     * Para un nodo con hijos:
     *
     *      ObjectN0.db
     *      =>
     *      ObjectN1.db
     *      ObjectN2.db
     *      <=
     *
     * Para:
     *
     *      N0
     *      |
     *      N1
     *      |
     *      N2
     *
     * se obtiene:
     *
     *      N0
     *      =>
     *      N1
     *      =>
     *      N2
     *      <=
     *      <=
     */

    /*
     * Guardar únicamente el archivo binario
     * correspondiente a este GameObject.
     */
    if (!object->saveEntity(filename)) {
        std::cerr << "[escena] no se pudo guardar el binario del objeto id="
                  << object->getId() << " ('" << object->inputName
                  << "') en: " << filename << std::endl;
    }

    /*
     * Registrar el archivo del objeto en BBDDObjetos.txt.
     *
     * SceneSerializer es el único responsable de
     * modificar este archivo.
     */
    const std::string objectPath =
        filename +
        "/ObjectN" +
        std::to_string(object->getId()) +
        ".db";

    // Se guarda solo el nombre del archivo: al cargar, el .db real se resuelve
    // contra el directorio de escena del proyecto (SceneSerializer::load);
    // un path absoluto aquí no añade información y se rompería al mover o
    // renombrar el proyecto. El lector solo consume el basename de cada linea.
    const std::string basename =
        objectPath.substr(objectPath.find_last_of("/\\") + 1);

    archive << basename << '\n';

    /*
     * Si el nodo no tiene hijos, termina.
     */
    if (!scene->getEntitysTree()->isInternal(root))
        return;

    /*
     * IMPORTANTE:
     *
     * La llave de apertura se escribe ANTES de recorrer
     * recursivamente los hijos.
     */
    archive << "=>\n";

    /*
     * Obtener los hijos del nodo actual.
     */
    auto* children =
        scene->getEntitysTree()->childsOf(root);

    if (children) {

        auto* position = children->first();

        while (position != nullptr) {

            /*
             * Preorden: cada hijo se procesa completamente
             * antes de pasar al siguiente hermano.
             */
            savePreOrder(
                position->getElement(),
                archive,
                filename
            );

            position =
                (position != children->last())
                    ? children->next(position)
                    : nullptr;
        }

        delete children;
    }

    /*
     * La llave de cierre se escribe DESPUÉS de todos
     * los hijos del nodo actual.
     */
    archive << "<=\n";
}

void SceneSerializer::save(const std::string& filename)
{
    if (!scene)
        return;

    const std::string pathTxt =
        filename + "BBDDObjetos.txt";

    /*
     * Limpiar binarios huérfanos antes de guardar.
     * Se mantiene únicamente ObjectN0.db (raíz). Se eliminan ObjectN*.db
     * cuyo ID no exista en el árbol actual.
     */
    limpiarBinariosHuérfanos(scene, filename);

    /*
     * save() guarda un snapshot completo de la escena.
     *
     * Por lo tanto BBDDObjetos.txt debe reconstruirse
     * desde cero en cada guardado.
     */
    // La decision se toma ANTES de abrir con trunc. Un arbol vacio es un estado
    // valido (se limpia la escena y se guarda) y se escribe vacio a proposito,
    // pero SIEMPRE con aviso: un BBDDObjetos.txt de 0 bytes sin decir nada es
    // indistinguible de una perdida de datos silenciosa (el caso investigado era
    // exactamente un return entre el trunc y la escritura).
    const bool escenaVacia = scene->getEntitysTree()->isEmpty();
    std::error_code ec;
    const bool existia = std::filesystem::exists(pathTxt, ec);
    if (escenaVacia) {
        std::cerr << "[escena] guardando con el arbol vacio";
        if (existia)
            std::cerr << ": " << pathTxt
                      << " queda vacio a proposito (antes tenia contenido)";
        std::cerr << std::endl;
    }

    std::ofstream archive(
        pathTxt,
        std::ios::trunc
    );

    if (!archive.is_open()) {

        std::cerr
            << "No se pudo abrir BBDDObjetos.txt para "
               "escritura: "
            << pathTxt
            << '\n';

        return;
    }

    /*
     * Recorrido completo desde la raíz. Con el arbol vacio no hay nada que
     * recorrer: el trunc de arriba dejo el archivo vacio, que es exactamente lo
     * que se aviso arriba.
     */
    if (!escenaVacia) {
        savePreOrder(
            scene->getEntitysTree()->rootOfTree(),
            archive,
            filename
        );
    }

    /*
     * El stream se cierra automáticamente al salir
     * de la función, pero lo cerramos explícitamente
     * para dejar clara la responsabilidad.
     */
    archive.close();
}
