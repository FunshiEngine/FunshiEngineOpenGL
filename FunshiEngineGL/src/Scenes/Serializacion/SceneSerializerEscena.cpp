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

void publicarExcepcionEscena(RegistroDiagnosticos* diagnosticos,
                             const std::string& pathTxt,
                             const std::exception& excepcion) {
    const std::string clave = "scene-load:" + pathTxt;
    if (diagnosticos) {
        registrarExcepcionDiagnostica(*diagnosticos,
                                      "Scenes/SceneSerializer", excepcion,
                                      NivelDiagnostico::Error, clave);
    } else {
        registrarExcepcionDiagnostica("Scenes/SceneSerializer", excepcion,
                                      NivelDiagnostico::Error, clave);
    }
}

}  // namespace

bool SceneSerializer::load(
    const std::string& pathTxt,
    const std::string& semiPath)
{
    if (!scene)
        return false;

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
        std::error_code errorRuta;
        const bool existe = std::filesystem::exists(pathTxt, errorRuta);
        if (errorRuta || existe) {
            const std::string detalle = errorRuta
                ? errorRuta.message()
                : "el archivo existe pero no se puede abrir";
            publicarFalloEscena(
                diagnosticos, "SCENE_OPEN_FAILED",
                "No se pudo abrir el archivo de escena '" + pathTxt +
                    "': " + detalle,
                NivelDiagnostico::Error);
            std::cerr << "No se pudo abrir el archivo de escena: " << pathTxt
                      << '\n';
            return false;
        }

        // La ausencia del índice es normal al crear una escena nueva. Solo
        // se reporta si no se puede materializar el archivo vacío.
        std::ofstream nuevo(pathTxt, std::ios::out | std::ios::trunc |
                                         std::ios::binary);
        if (!nuevo.is_open()) {
            publicarFalloEscena(
                diagnosticos, "SCENE_CREATE_FAILED",
                "No se pudo crear el archivo de escena '" + pathTxt + "'",
                NivelDiagnostico::Error);
            std::cerr << "No se pudo crear el archivo de escena: " << pathTxt
                      << '\n';
            return false;
        }
        nuevo.close();
        if (!nuevo) {
            publicarFalloEscena(
                diagnosticos, "SCENE_CREATE_FAILED",
                "No se pudo completar el archivo de escena '" + pathTxt + "'",
                NivelDiagnostico::Error);
            return false;
        }

        if (editor)
            editor->clearScene();
        else
            scene->clear();
        return true;
    }

    // No destruir la escena vigente hasta confirmar que el índice puede
    // abrirse; un archivo existente pero ilegible no equivale a escena nueva.
    if (editor)
        editor->clearScene();
    else
        scene->clear();

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
    bool cargaCompleta = true;
    try {
        loadPreOrder(file, nullptr, semiPath, cargaCompleta);
    } catch (const std::exception& excepcion) {
        publicarExcepcionEscena(diagnosticos, pathTxt, excepcion);
        std::cerr << "Error al reconstruir la escena '" << pathTxt << "': "
                  << excepcion.what() << '\n';
        file.close();
        return false;
    } catch (...) {
        publicarFalloEscena(
            diagnosticos, "SCENE_LOAD_UNKNOWN",
            "Fallo desconocido al reconstruir la escena '" + pathTxt + "'",
            NivelDiagnostico::Error);
        file.close();
        return false;
    }

    if (file.bad()) {
        cargaCompleta = false;
        publicarFalloEscena(
            diagnosticos, "SCENE_READ_FAILED",
            "Error de lectura en el archivo de escena '" + pathTxt + "'",
            NivelDiagnostico::Error);
    }

    file.close();
    return cargaCompleta;
}
