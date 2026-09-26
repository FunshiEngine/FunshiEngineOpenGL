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
#ifndef FILEMANAGER_H
#define FILEMANAGER_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "../GestorDeArchivos/GestorDeArchivos.h"
#include "FileSelection.h"

class GestorDeArchivos;
class FileSystemWatcher;

// Fachada del explorador de archivos (mismo patron que EditorController y la
// fachada MenuGUI): es DUENA del modelo (GestorDeArchivos) y del estado de
// navegacion compartido (FileSelection). Expone las operaciones de dominio
// (crear, copiar, eliminar) y las operaciones nativas del sistema (dialogos de
// seleccion, abrir con la app predeterminada, listado de un directorio y
// plantillas de scripts) que antes estaban inline en las vistas con
// system()/popen()/ifstream/ShellExecute*. Las vistas solo conversan con esta
// fachada, nunca con GestorDeArchivos ni con el Filesystem directamente.
class FileManager {
public:
    // Entrada de un listado de directorio (el grid del explorador la dibuja).
    struct EntradaDirectorio {
        std::string nombre;
        std::string ruta;
        bool esCarpeta = false;
        std::string extension;
    };

    explicit FileManager(const std::string& pathProyect, const std::string& rootName = "MotorGrafico");
    ~FileManager();

    FileManager(const FileManager&) = delete;
    FileManager& operator=(const FileManager&) = delete;

    FileSelection* getSelection() { return &selection; }
    ArbolEnlazado<File*>* getArbol() { return gestor->getTreeFilePath(); }
    const std::string& getPathProyect() const noexcept { return pathProyect; }
    const std::string& getRootName() const noexcept { return rootName; }

    // Cambia la raiz del proyecto que vigila y explora el FileManager
    void setProyecto(const std::string& nuevoPath, const std::string& nuevoRootName);

    // Rescanea el Filesystem y reconstruye el arbol si cambio. Re-resuelve
    // rutaVisible en el nuevo arbol (los punteros al arbol viejo quedarian
    // colgando).
    void refrescar();

    // Cambios externos detectados por FileSystemWatcher: drena el fd (aplica
    // vigilancia a carpetas nuevas) y consume la marca al consultarse. El
    // arbol la usa como condicion de rescaneo ademas de contadorCambios.
    bool huboCambiosExternos();

    bool crearCarpeta(const std::string& ruta);
    bool eliminarCarpeta(const std::string& ruta);
    bool crearArchivo(const std::string& ruta, const std::string& contenido);
    bool copiarCarpeta(const std::string& origen, const std::string& destino);
    bool copiarArchivo(const std::string& origen, const std::string& destino);
    bool renombrar(const std::string& ruta, const std::string& nuevoNombre);

    // Busca por ruta completa en el arbol vigente (navegacion diferida del
    // doble clic). Devuelve nullptr si la ruta ya no existe (carpeta borrada
    // en otro lugar).
    Carpeta* buscarCarpetaPorRuta(const std::string& ruta);

    // ---- Operaciones nativas del sistema (sin estado; static) ----

    // Dialogo nativo de seleccion de carpeta (BROWSEINFO en Windows, zenity en
    // Linux). Devuelve la ruta elegida o vacio si el usuario cancelo.
    static std::string seleccionarCarpetaSistema();
    // Dialogo nativo de seleccion de archivo.
    static std::string seleccionarArchivoSistema();

    // Abre la ruta con la aplicacion predeterminada del sistema (doble clic
    // sobre un archivo del grid). fork+exec sin shell en Linux (los nombres
    // pueden tener metacaracteres; system() no debe tocar mas shells).
    static bool abrirConAppPredeterminada(const std::string& ruta);

    // Lista una carpeta (sin seguir symlinks) rellenando `salida` con una
    // entrada por item. Devuelve false si la carpeta no se puede leer.
    static bool listarDirectorio(const std::string& path,
                                 std::vector<EntradaDirectorio>& salida);

    // Momento de ultima escritura de un directorio, para cachear el grid
    // (una carpeta cambia su mtime al agregar/quitar entradas).
    static std::filesystem::file_time_type mtimeDirectorio(const std::string& path);

    // Dice si la ruta es un directorio en disco (false si no existe o es
    // archivo).
    static bool esDirectorio(const std::string& path);

    // Plantilla de script para el explorador: `.cpp` (backend C++ con
    // REFLECT_*) o `.java` (backend JNI). `clase` es el nombre sin extension.
    static std::string plantillaScript(const std::string& clase, bool esJava);

private:
    std::string pathProyect;
    std::string rootName;
    std::unique_ptr<GestorDeArchivos> gestor;
    std::unique_ptr<FileSystemWatcher> vigilante;
    FileSelection selection;
};

#endif