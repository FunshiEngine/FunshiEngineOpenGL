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

#include <filesystem>
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


namespace {

/*
 * getline que ademas recorta el '\r' final de los archivos con CRLF.
 *
 * SceneBBDDObjetos.txt se abre en BINARIO (ver load()): la traduccion de
 * CRLF del CRT en modo texto rompe los offsets de tellg/seekg del
 * look-ahead (la releitura de la siguiente linea arranca en un offset
 * erroneo y sale truncada, ver DocuTecnicoBugs.md). En binario los offsets
 * son bytes exactos y el '\r' queda en la linea: se recorta aca.
 */
bool getlineLimpio(std::ifstream& file, std::string& line) {
    if (!std::getline(file, line))
        return false;
    if (!line.empty() && line.back() == '\r')
        line.pop_back();
    return true;
}

/*
 * Parsea el basename de una linea de SceneBBDDObjetos.txt con el patron
 * estricto que escribe savePreOrder: ObjectN<entero>.db.
 *
 * Devuelve false para cualquier otra cosa (basura, "ObjectN.db" con id
 * vacio, sufijos raros, id fuera de rango de int). El llamador la salta con
 * aviso en vez de caer al id default 0: una linea que resuelve a id=0 en
 * posicion de hijo hacia que loadPreOrder lea ObjectN0.db (el binario de la
 * RAIZ) y nace un hijo fantasma "Scene" que, al guardarse con id propio, se
 * auto-propaga. Ver PLAN GENERAL DE FIX.md §20 (H-17).
 */
bool idDesdeLinea(const std::string& name, int& id) {
    const std::size_t marcador = name.find("ObjectN");
    if (marcador == std::string::npos)
        return false;

    std::string texto = name.substr(marcador + 7);

    const std::size_t finDb = texto.find(".db");
    if (finDb == std::string::npos || finDb != texto.size() - 3)
        return false;
    texto.erase(finDb);

    // Vacio ("ObjectN.db") o sin digitos no es un id.
    std::size_t inicio = 0;
    if (!texto.empty() && texto[0] == '-')
        inicio = 1;
    if (inicio >= texto.size())
        return false;
    for (std::size_t i = inicio; i < texto.size(); ++i) {
        if (texto[i] < '0' || texto[i] > '9')
            return false;
    }

    try {
        id = std::stoi(texto);
    }
    catch (const std::exception&) {
        return false;
    }
    return true;
}

/*
 * Consume el bloque de hijos que pudiera seguir a una linea saltada, para
 * que los marcadores "=>" "<=" no queden desalineados: si la linea mala tenia
 * hijos y no se consume su bloque, el "<=" de cierre se lo come el nivel
 * anterior y las lineas que siguen se procesan contra la raiz (replaceRoot),
 * destruyendo la escena. Si lo que sigue no es "=>", se devuelve a la
 * posicion para que el bucle principal la procese con normalidad.
 */
void saltarBloqueHijos(std::ifstream& file) {
    const std::streampos trasLinea = file.tellg();
    std::string marcador;
    if (!getlineLimpio(file, marcador))
        return;

    if (marcador != "=>") {
        file.seekg(trasLinea);
        return;
    }

    int profundidad = 1;
    while (profundidad > 0 && getlineLimpio(file, marcador)) {
        if (marcador == "=>")
            ++profundidad;
        else if (marcador == "<=")
            --profundidad;
    }
}

} // namespace


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


void SceneSerializer::loadPreOrder(
    std::ifstream& file,
    GameObject* parent,
    const std::string& semiPath)
{
    std::string line;

    while (getlineLimpio(file, line)) {

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
         * ========================================================
         * VALIDAR LA LINEA (H-17)
         * ========================================================
         *
         * Solo se acepta el patron estricto que escribe savePreOrder:
         * ObjectN<entero>.db. Una linea que no cumple el patron, o una linea
         * de hijo con id=0 (ObjectN0.db es el binario de la RAIZ), se salta
         * con aviso en vez de caer al id default 0: leer la raiz en posicion
         * de hijo crea el hijo fantasma "Scene" que, al guardarse con id
         * propio, se auto-propaga. Ver PLAN GENERAL DE FIX.md §20.
         */
        int idParseado = 0;
        if (!idDesdeLinea(name, idParseado)) {
            std::cerr << "[escena] linea invalida en el indice de escena; se "
                         "ignora: '"
                      << line << "'\n";
            saltarBloqueHijos(file);
            continue;
        }
        if (parent && idParseado == 0) {
            std::cerr << "[escena] hijo con id=0 en el indice (ObjectN0.db es "
                         "la raiz); se ignora: '"
                      << line << "'\n";
            saltarBloqueHijos(file);
            continue;
        }

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
         * RECUPERAR ID (ya validado arriba)
         * ========================================================
         *
         * ObjectN123.db
         *       ^^^
         */
        object->setId(idParseado);

        /*
         * Cargar los datos binarios del objeto.
         *
         * GameObject::loadEntity() construye:
         *
         * semiPath/ObjectN#.db
         */
        if (!object->loadEntity(semiPath)) {
            std::cerr << "[escena] no se pudo cargar el binario del objeto id="
                      << object->getId() << " desde: " << semiPath << std::endl;
        }

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
         * Si no se pudo insertar, saltar su bloque de hijos tambien (si lo
         * tiene) para no desalinear los marcadores, y continuar.
         */
        if (!loaded) {
            saltarBloqueHijos(file);
            continue;
        }

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

        if (!getlineLimpio(file, marker))
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
     *
     * BINARIO a proposito: el look-ahead de loadPreOrder hace
     * seekg(tellg()) tras getline, y en modo texto la traduccion de CRLF
     * del CRT hace que esa releitura arranque en un offset erroneo (la
     * siguiente linea sale truncada, ver DocuTecnicoBugs.md). En binario
     * los offsets son bytes exactos; el '\r' de las lineas lo recorta
     * getlineLimpio().
     */
    std::ifstream file(pathTxt, std::ios::binary);

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
