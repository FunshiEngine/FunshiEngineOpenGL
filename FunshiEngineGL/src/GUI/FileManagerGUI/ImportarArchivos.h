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
#ifndef IMPORTARARCHIVOS_H
#define IMPORTARARCHIVOS_H

// Logica de importar en el proyecto archivos que vienen de FUERA: lo que el
// usuario arrastra desde el explorador del sistema sobre la ventana del
// editor, o lo que pega con Ctrl+V desde el portapapeles del sistema.
//
// Header-only a proposito, como RenombrarElemento.h y CrearCarpeta.h: son
// unas pocas lineas de politica y asi el helper no obliga a tocar la lista
// de fuentes de CMakeLists.txt.
//
// La politica que aplica:
//   - No pisa nada. Si el destino ya existe, se genera un nombre unico
//     ("a.txt" -> "a (1).txt"), que es lo que hace el explorador de archivos
//     del sistema. Perder el archivo que ya estaba por un pegado accidental
//     seria peor que un nombre con sufijo.
//   - Las carpetas se copian enteras, con su contenido.
//   - Un origen que no existe o no se puede leer genera un error y no aborta
//     el resto: pegar cinco archivos de los que uno falla copia cuatro.
//
// No decide donde va cada archivo (eso lo sabe el panel, que es quien tiene
// la carpeta visible) ni invalida vistas (eso lo hace quien la drew).

#include <string>
#include <vector>

#include "../../FileManager/FileManager.h"
#include "../../Herramientas/PathUtils.h"

namespace ImportarArchivos {

struct Resultado {
    // Cuantos elementos se copiaron (archivos y carpetas).
    int copiados = 0;
    // Cuantas de esos eran carpetas (las carpetas si aparecen en el arbol de
    // navegacion, los archivos no).
    int carpetasCopiadas = 0;
    // Ruta final de cada copia, en el mismo orden que las rutas de entrada.
    std::vector<std::string> rutasCopiadas;
    // Motivo de cada origen que no se pudo copiar.
    std::vector<std::string> errores;
};

// Copia en `destino` los elementos de `rutas` (rutas absolutas de archivos o
// carpetas que estan fuera del proyecto).
inline Resultado pegar(FileManager* fileManager, const std::string& destino,
                        const std::vector<std::string>& rutas) {
    Resultado resultado;
    if (fileManager == nullptr) {
        resultado.errores.push_back(
            "No hay gestor de archivos: no se puede importar nada.");
        return resultado;
    }
    if (destino.empty()) {
        resultado.errores.push_back(
            "No hay carpeta de destino: no se puede importar nada.");
        return resultado;
    }

    for (const std::string& origen : rutas) {
        if (origen.empty()) continue;

        const std::string::size_type sep = indiceSeparadorFinal(origen);
        const std::string nombre =
            (sep != std::string::npos) ? origen.substr(sep + 1) : origen;
        if (nombre.empty()) {
            resultado.errores.push_back(
                "La ruta '" + origen + "' no tiene nombre de archivo.");
            continue;
        }

        const std::string nombreLibre = FileManager::nombreUnico(destino, nombre);
        const std::string finalDest = destino + PATH_SEP + nombreLibre;

        const bool esCarpeta = FileManager::esDirectorio(origen);
        const bool ok =
            esCarpeta ? fileManager->copiarCarpeta(origen, finalDest)
                      : fileManager->copiarArchivo(origen, finalDest);
        if (!ok) {
            resultado.errores.push_back("No se pudo copiar '" + origen + "'.");
            continue;
        }

        ++resultado.copiados;
        if (esCarpeta) ++resultado.carpetasCopiadas;
        resultado.rutasCopiadas.push_back(finalDest);
    }
    return resultado;
}

}  // namespace ImportarArchivos

#endif
