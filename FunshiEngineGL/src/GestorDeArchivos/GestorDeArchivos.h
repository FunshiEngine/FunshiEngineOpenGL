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
#ifndef GESTORDEARCHIVOS_H
#define GESTORDEARCHIVOS_H

#include <string>

#include "../Estructuras/Trees/ArbolesEnlazados/ArbolEnlazado.h"
#include "Carpeta.h"

// Infraestructura de acceso al Filesystem del explorador de archivos:
// construye el arbol de carpetas de un proyecto y ejecuta las operaciones de
// dominio (crear/eliminar/copiar). R2: todo el acceso a disco usa
// std::filesystem (ambas plataformas) en lugar de APIS por #ifdef
// (dirent.h / WinAPI) y de system("rm -rf ...").
//
// Nunca es dueno del ArbolEnlazado salvo del arbol que construye: libera los
// File* que crea (los nodos del arbol no son duenos de sus elementos).
class GestorDeArchivos {
private:
    ArbolEnlazado<File*>* treeFilePath;
    Position<File*>* folderActual;
    std::string rootName;

    void recorrerDir(const std::string& path, Position<File*>* parent);
    // Borra los File* (elementos) de un arbol en pre-orden. El arbol de
    // nodos (TNodo/ListaDE) no libera sus elementos al destruirse.
    void liberarPreOrden(ArbolEnlazado<File*>* arbol, Position<File*>* p);
    bool compareTreesByPath(ArbolEnlazado<File*>* first,
                            ArbolEnlazado<File*>* second);
    bool preOrdenNoExaustivo(ArbolEnlazado<File*>* first,
                             ArbolEnlazado<File*>* second,
                             Position<File*>* left, Position<File*>* right);

public:
    explicit GestorDeArchivos(std::string pathProyect, std::string rootName = "MotorGrafico");
    ~GestorDeArchivos();

    GestorDeArchivos(const GestorDeArchivos&) = delete;
    GestorDeArchivos& operator=(const GestorDeArchivos&) = delete;

    ArbolEnlazado<File*>* getTreeFilePath();
    bool setTreeFilePath(const std::string& path, std::string name);
    const std::string& getRootName() const noexcept { return rootName; }
    bool eliminarCarpeta(const std::string& path);
    bool eliminarArchivo(const std::string& path);
    bool crearCarpeta(const std::string& path);
    bool crearArchivo(const std::string& path, const std::string& contenido);
    bool copiarCarpeta(const std::string& origen, const std::string& destino);
    bool copiarArchivo(const std::string& origen, const std::string& destino);
    bool renombrar(const std::string& ruta, const std::string& nuevoNombre);

    // Mueve un archivo o una carpeta de `origen` a `destino` (ruta completa
    // final, no el nombre). Intenta rename, que es atomico y barato; si el
    // destino esta en otro volumen, cae a copiar+eliminar.
    //
    // Se niega a mover una carpeta dentro de si misma o de un descendiente: el
    // `error_code` cortaria la recursion a mitad, dejando el arbol a medias en
    // disco. Tambien se niega a pisar un destino existente, para no perder datos
    // con un arrastre accidental.
    bool mover(const std::string& origen, const std::string& destino);
};

#endif