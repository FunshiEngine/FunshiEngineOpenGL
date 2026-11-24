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

}  // namespace

void SceneSerializer::loadPreOrder(
    std::ifstream& file,
    GameObject* parent,
    const std::string& semiPath,
    bool& cargaCompleta)
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
            cargaCompleta = false;
            publicarFalloEscena(
                diagnosticos, "SCENE_INDEX_INVALID",
                "Línea inválida en el índice de escena; se ignora: '" + line +
                    "'",
                NivelDiagnostico::Advertencia);
            std::cerr << "[escena] linea invalida en el indice de escena; se "
                         "ignora: '"
                      << line << "'\n";
            saltarBloqueHijos(file);
            continue;
        }
        if (parent && idParseado == 0) {
            cargaCompleta = false;
            publicarFalloEscena(
                diagnosticos, "SCENE_INDEX_INVALID",
                "El índice de escena referencia el objeto raíz como hijo: '" +
                    line + "'",
                NivelDiagnostico::Advertencia);
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
         * de los nodos se cargan como GameObject con un componente Model, a
         * quien se le inyecta el AssetManager antes de loadEntity(): la
         * deserializacion lee el path del modelo y el componente Model carga
         * la geometria; con el manager ya asignado se comparte el asset
         * cacheado.
         */
        std::unique_ptr<GameObject> object;
        if (!parent) {
            object = std::make_unique<ObjetoEscena>();
        } else {
            auto objeto = std::make_unique<GameObject>();
            auto* model = new Model();
            model->setAssetManager(assets);
            model->setRegistroDiagnosticos(diagnosticos);
            objeto->addComponent(std::unique_ptr<Component>(model));
            object = std::move(objeto);
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
            cargaCompleta = false;
            const std::filesystem::path rutaObjeto =
                std::filesystem::path(semiPath) /
                ("ObjectN" + std::to_string(object->getId()) + ".db");
            publicarFalloEscena(
                diagnosticos, "SCENE_OBJECT_LOAD_FAILED",
                "No se pudo cargar el binario del objeto id=" +
                    std::to_string(object->getId()) + " desde '" +
                    rutaObjeto.string() + "'",
                NivelDiagnostico::Error);
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
            cargaCompleta = false;
            publicarFalloEscena(
                diagnosticos, "SCENE_OBJECT_INSERT_FAILED",
                "No se pudo insertar el objeto id=" +
                    std::to_string(idParseado) + " durante la carga de escena",
                NivelDiagnostico::Error);
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
                semiPath,
                cargaCompleta
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
