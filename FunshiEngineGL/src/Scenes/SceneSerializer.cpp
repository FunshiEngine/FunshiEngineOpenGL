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
#include "SceneSerializer.h"

#include <iostream>
#include <memory>
#include <string>

#include "EditorController.h"
#include "SceneRegistry.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Modelos3D.h"
#include "../Objetos/ObjetoEscena.h"


SceneSerializer::SceneSerializer(SceneRegistry* value,
                                 EditorController* controller,
                                 AssetManager* assetsManager)
    : scene(value),
      editor(controller),
      assets(assetsManager) {
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
    object->saveEntity(filename);

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
     * save() guarda un snapshot completo de la escena.
     *
     * Por lo tanto BBDDObjetos.txt debe reconstruirse
     * desde cero en cada guardado.
     */
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
     * Si la escena está vacía, dejamos el archivo vacío.
     */
    if (scene->getEntitysTree()->isEmpty())
        return;

    /*
     * Recorrido completo desde la raíz.
     */
    savePreOrder(
        scene->getEntitysTree()->rootOfTree(),
        archive,
        filename
    );

    /*
     * El stream se cierra automáticamente al salir
     * de la función, pero lo cerramos explícitamente
     * para dejar clara la responsabilidad.
     */
    archive.close();
}


void SceneSerializer::loadPreOrder(
    std::ifstream& file,
    GameObject* parent,
    const std::string& semiPath)
{
    std::string line;

    while (std::getline(file, line)) {

        /*
         * Ignorar líneas vacías.
         */
        if (line.empty())
            continue;

        /*
         * Terminó el bloque de hijos del objeto padre
         * correspondiente a esta llamada recursiva.
         */
        if (line == "<=")
            return;

        /*
         * "=>" se procesa después de cargar el objeto
         * padre mediante el look-ahead que se encuentra
         * debajo.
         *
         * Si aparece aislado, simplemente se ignora.
         */
        if (line == "=>")
            continue;

        /*
         * La línea representa un archivo:
         *
         * .../ObjectN#.db
         */
        const std::string name =
            line.substr(
                line.find_last_of("/\\") + 1
            );

        /*
         * Crear un nuevo objeto.
         *
         * La raiz (sin padre) es el GameObject "Scene": un contenedor sin
         * geometria (ObjetoEscena) que agrupa a todas las entidades. El resto
         * de los nodos se cargan como Modelos3D con la fuente de mallas
         * inyectada ANTES de loadEntity(): la deserializacion lee el path del
         * modelo y carga la geometria; con el manager ya asignado se comparte
         * el asset cacheado.
         */
        std::unique_ptr<GameObject> object;
        if (!parent) {
            object = std::make_unique<ObjetoEscena>();
        } else {
            auto modelo = std::make_unique<Modelos3D>();
            modelo->setAssetManager(assets);
            object = std::move(modelo);
        }

        /*
         * ========================================================
         * RECUPERAR ID
         * ========================================================
         *
         * ObjectN123.db
         *       ^^^
         */
        const std::size_t idMarker =
            name.find("ObjectN");

        if (idMarker != std::string::npos) {

            std::string id =
                name.substr(idMarker + 7);

            const std::size_t dbMarker =
                id.find(".db");

            if (dbMarker != std::string::npos)
                id.erase(dbMarker);

            try {

                object->setId(
                    std::stoi(id)
                );

            }
            catch (const std::exception&) {

                std::cerr
                    << "ID inválido en archivo de escena: "
                    << name
                    << '\n';

                object->setId(0);
            }
        }

        /*
         * Cargar los datos binarios del objeto.
         *
         * GameObject::loadEntity() construye:
         *
         * semiPath/ObjectN#.db
         */
        object->loadEntity(semiPath);

        GameObject* loaded = nullptr;

        /*
         * ========================================================
         * INSERTAR OBJETO
         * ========================================================
         */

        /*
         * Si no hay padre, este objeto representa la raíz.
         */
        if (!parent) {

            if (scene->replaceRoot(
                    std::move(object)))
            {
                loaded =
                    scene->getRoot();
            }

        }
        /*
         * Si existe un padre, crear el objeto como hijo.
         */
        else if (editor) {

            loaded =
                editor->createGameObject(
                    std::move(object),
                    parent
                );
        }

        /*
         * Si no se pudo insertar, continuar.
         */
        if (!loaded)
            continue;

        /*
         * ========================================================
         * LOOK-AHEAD
         * ========================================================
         *
         * Miramos qué aparece inmediatamente después
         * del objeto que acabamos de cargar.
         *
         * Posibilidades:
         *
         *      =>
         *          el objeto tiene hijos
         *
         *      <=
         *          terminó el bloque actual
         *
         *      ObjectN#.db
         *          siguiente objeto del mismo nivel
         */
        const std::streampos markerPosition =
            file.tellg();

        std::string marker;

        if (!std::getline(file, marker))
            return;

        /*
         * Saltar líneas vacías sin perder la posición.
         */
        if (marker.empty()) {

            file.seekg(markerPosition);

            continue;
        }

        /*
         * ========================================================
         * HIJOS
         * ========================================================
         */
        if (marker == "=>") {

            /*
             * Procesar todos los hijos del objeto actual.
             *
             * loadPreOrder() retorna cuando encuentra
             * el "<=" correspondiente.
             */
            loadPreOrder(
                file,
                loaded,
                semiPath
            );

            continue;
        }

        /*
         * ========================================================
         * FIN DEL BLOQUE
         * ========================================================
         */
        if (marker == "<=")
            return;

        /*
         * ========================================================
         * SIGUIENTE HERMANO
         * ========================================================
         *
         * No era un marcador, sino otro ObjectN#.db.
         *
         * Volvemos atrás para que la siguiente iteración
         * procese esa línea.
         */
        file.seekg(markerPosition);
    }
}


void SceneSerializer::load(
    const std::string& pathTxt,
    const std::string& semiPath)
{
    if (!scene)
        return;

    /*
     * Limpiar la escena actual antes de reconstruirla.
     */
    if (editor)
        editor->clearScene();
    else
        scene->clear();

    /*
     * Abrir el archivo de descripción de la escena.
     */
    std::ifstream file(pathTxt);

    if (!file.is_open()) {

        /*
         * Si no existe, crear un archivo vacío.
         */
        std::ofstream newFile(
            pathTxt,
            std::ios::trunc
        );

        if (!newFile.is_open()) {

            std::cerr
                << "No se pudo crear el archivo de escena: "
                << pathTxt
                << '\n';
        }

        return;
    }

    /*
     * Reconstruir la escena siguiendo la estructura:
     *
     *      objeto
     *      =>
     *          hijos
     *      <=
     *
     * de forma recursiva.
     */
    loadPreOrder(
        file,
        nullptr,
        semiPath
    );

    file.close();
}
