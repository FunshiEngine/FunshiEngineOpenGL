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

// Windows.h ANTES del header propio (que incluye <filesystem>), para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte.
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <shellapi.h>
#elif defined(__linux__)
#include <cstdio>
#include <unistd.h>
#endif

#include "FileManager.h"

#include <filesystem>

#include "FileSystemWatcher.h"
#include "../GestorDeArchivos/Carpeta.h"
#include "../Herramientas/PathUtils.h"
#include "../Herramientas/TreeGUI/TreeGUI.h"

namespace {
// Busca pre-orden la primera Carpeta cuya ruta completa coincida.
Carpeta* buscarPreOrden(ArbolEnlazado<File*>* arbol,
                        Position<File*>* current,
                        const std::string& ruta) {
    if (!arbol || !current) return nullptr;
    File* elemento = current->getElement();
    if (elemento) {
        // Comparacion de rutas como std::filesystem::path, NO de string crudo:
        // en Windows '/' y '\\' son equivalentes pero la representacion puede
        // diferir segun quien armo la ruta (el arbol vs una ingresada a mano),
        // y string == las trataria como rutas distintas.
        const std::filesystem::path rutaNodo =
            elemento->getPathRoot() + PATH_SEP + elemento->getPathName();
        if (rutaNodo == std::filesystem::path(ruta))
            return dynamic_cast<Carpeta*>(elemento);
    }
    if (arbol->isInternal(current)) {
        Carpeta* encontrado = nullptr;
        TreeIG::forEachChild((TNodo<File*>*)current, [&](Position<File*>* hijo) {
            if (!encontrado) encontrado = buscarPreOrden(arbol, hijo, ruta);
        });
        return encontrado;
    }
    return nullptr;
}
} // namespace

FileManager::FileManager(const std::string& pathProyect, const std::string& rootName)
    : pathProyect(pathProyect),
      rootName(rootName.empty() ? "MotorGrafico" : rootName),
      gestor(new GestorDeArchivos(pathProyect, this->rootName)),
      vigilante(new FileSystemWatcher(pathProyect)) {}

FileManager::~FileManager() = default;

void FileManager::setProyecto(const std::string& nuevoPath, const std::string& nuevoRootName) {
    if (pathProyect == nuevoPath && rootName == nuevoRootName) return;
    pathProyect = nuevoPath;
    rootName = nuevoRootName.empty() ? "MotorGrafico" : nuevoRootName;
    std::error_code ec;
    std::filesystem::create_directories(pathProyect, ec);
    vigilante = std::make_unique<FileSystemWatcher>(pathProyect);
    gestor->setTreeFilePath(pathProyect, rootName);
    FileSelection* sel = getSelection();
    sel->rutaVisible = pathProyect;
    sel->carpetaActual = nullptr;
    if (gestor->getTreeFilePath() && !gestor->getTreeFilePath()->isEmpty()) {
        Position<File*>* rootPos = gestor->getTreeFilePath()->rootOfTree();
        if (rootPos && rootPos->getElement())
            sel->carpetaActual = dynamic_cast<Carpeta*>(rootPos->getElement());
    }
    sel->navegacionPendiente.clear();
    sel->contadorCambios++;
}

void FileManager::refrescar() {
    FileSelection* sel = getSelection();
    const std::string rutaVisible = sel->rutaVisible;
    // GestorDeArchivos ya comprueba por rutas y solo reconstruye si cambio.
    gestor->setTreeFilePath(pathProyect, rootName);
    // Los punteros al arbol viejo quedaron liberados (o apuntarian a una
    // seleccion caducada): re-resolvemos la carpeta visible por su ruta.
    sel->carpetaActual = nullptr;
    if (!rutaVisible.empty())
        sel->carpetaActual = buscarCarpetaPorRuta(rutaVisible);
    if (!sel->carpetaActual && gestor->getTreeFilePath() && !gestor->getTreeFilePath()->isEmpty()) {
        Position<File*>* rootPos = gestor->getTreeFilePath()->rootOfTree();
        if (rootPos && rootPos->getElement()) {
            sel->carpetaActual = dynamic_cast<Carpeta*>(rootPos->getElement());
            if (sel->carpetaActual) sel->rutaVisible = pathProyect;
        }
    }
}

bool FileManager::huboCambiosExternos() {
    return vigilante ? vigilante->huboCambiosYConsumir() : false;
}

bool FileManager::crearCarpeta(const std::string& ruta) {
    return gestor->crearCarpeta(ruta);
}

bool FileManager::eliminarCarpeta(const std::string& ruta) {
    return gestor->eliminarCarpeta(ruta);
}

bool FileManager::crearArchivo(const std::string& ruta, const std::string& contenido) {
    return gestor->crearArchivo(ruta, contenido);
}

bool FileManager::copiarCarpeta(const std::string& origen, const std::string& destino) {
    return gestor->copiarCarpeta(origen, destino);
}

bool FileManager::copiarArchivo(const std::string& origen, const std::string& destino) {
    return gestor->copiarArchivo(origen, destino);
}

bool FileManager::renombrar(const std::string& ruta, const std::string& nuevoNombre) {
    return gestor->renombrar(ruta, nuevoNombre);
}

Carpeta* FileManager::buscarCarpetaPorRuta(const std::string& ruta) {
    ArbolEnlazado<File*>* arbol = gestor->getTreeFilePath();
    if (!arbol || arbol->isEmpty()) return nullptr;
    return buscarPreOrden(arbol, arbol->rootOfTree(), ruta);
}

std::string FileManager::seleccionarCarpetaSistema() {
#if defined(_WIN32)
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = "Selecciona una carpeta para copiar";
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        char path[4096];
        if (SHGetPathFromIDListA(pidl, path)) return std::string(path);
    }
    return "";
#elif defined(__linux__)
    char buffer[4096];
    FILE* fp = popen("zenity --file-selection --directory 2>/dev/null", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            std::string path(buffer);
            path.erase(path.find_last_not_of("\n\r") + 1);
            pclose(fp);
            return path;
        }
        pclose(fp);
    }
    return "";
#endif
}

std::string FileManager::seleccionarArchivoSistema() {
#if defined(_WIN32)
    OPENFILENAMEA ofn;
    CHAR szFile[4096] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Todos los archivos\0*.*\0";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileNameA(&ofn) == TRUE) return std::string(szFile);
    return "";
#elif defined(__linux__)
    char buffer[4096];
    FILE* fp = popen("zenity --file-selection 2>/dev/null", "r");
    if (fp) {
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            std::string path(buffer);
            path.erase(path.find_last_not_of("\n\r") + 1);
            pclose(fp);
            return path;
        }
        pclose(fp);
    }
    return "";
#endif
}

bool FileManager::abrirConAppPredeterminada(const std::string& ruta) {
#if defined(_WIN32)
    return ShellExecuteA(NULL, "open", ruta.c_str(), NULL, NULL, SW_SHOW) > (HINSTANCE)32;
#elif defined(__linux__)
    // fork+exec (sin shell): los nombres de archivo pueden contener comillas o
    // metacaracteres y system() no debe tener los dedos en la command line.
    // xdg-open se desprende solo.
    pid_t pid = fork();
    if (pid == 0) {
        execl("/usr/bin/xdg-open", "xdg-open", ruta.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    return pid >= 0;
#endif
}

bool FileManager::listarDirectorio(const std::string& path,
                                   std::vector<EntradaDirectorio>& salida) {
    salida.clear();
    std::error_code ec;
    // No seguir symlinks: pueden apuntar a carpetas del sistema.
    std::filesystem::directory_iterator dirIt(path, ec);
    if (ec) return false;
    const std::filesystem::directory_iterator fin;
    for (; dirIt != fin;) {
        const std::filesystem::directory_entry entrada = *dirIt;
        dirIt.increment(ec);
        if (ec) { ec.clear(); continue; } // entrada con errores: la saltamos

        const std::string nombre = entrada.path().filename().string();
        if (nombre == "." || nombre == "..") continue;
        if (std::filesystem::is_symlink(entrada.symlink_status())) continue;

        EntradaDirectorio e;
        e.nombre = nombre;
        e.ruta = entrada.path().string();
        e.esCarpeta = entrada.is_directory();

        const size_t dot = nombre.find_last_of('.');
        if (!e.esCarpeta && dot != std::string::npos && dot != 0)
            e.extension = nombre.substr(dot);

        salida.push_back(std::move(e));
    }
    return true;
}

std::filesystem::file_time_type FileManager::mtimeDirectorio(const std::string& path) {
    std::error_code ec;
    const std::filesystem::file_time_type mtime =
        std::filesystem::last_write_time(path, ec);
    // Epoch si fallo: el llamador re-scaneara (mtime distinto del cache).
    return ec ? std::filesystem::file_time_type{} : mtime;
}

bool FileManager::esDirectorio(const std::string& path) {
    std::error_code ec;
    return std::filesystem::is_directory(path, ec) && !ec;
}

std::string FileManager::plantillaScript(const std::string& clase, bool esJava) {
    if (esJava) {
        return
            "// Script Java ejecutado por el motor via JNI (requiere compilar\n"
            "// el motor con -DFUNSHI_JAVA=ON). Los campos publicos son\n"
            "// SerializeField editables en el inspector.\n"
            "// El nombre de la clase debe coincidir con el del archivo.\n"
            "public class " + clase + " implements Comportamiento {\n"
            "    // public float velocidad = 5.0f;\n"
            "\n"
            "    @Override\n"
            "    public void iniciar(long objeto) {}\n"
            "\n"
            "    @Override\n"
            "    public void actualizar(long objeto, double deltaTime) {}\n"
            "\n"
            "    @Override\n"
            "    public void detener(long objeto) {}\n"
            "}\n";
    }

    // Script componente usa la convencion <ClassName>.cpp. La clase compilada
    // se llama FUNSHI_NOMBRE_CLASE: asi el mismo template compila para
    // cualquier <ClassName>.cpp del proyecto.
    return
        "#include \"Behaviour/IScriptBehaviour.h\"\n"
        "\n"
        "// Script C++ ejecutado por el motor (compilado a .so con hot\n"
        "// reload en play mode). Los campos expuestos con REFLECT_*\n"
        "// aparecen como SerializeField en el inspector.\n"
        "class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {\n"
        "public:\n"
        "    // float velocidad = 5.0f; // descomenten y agreguen aca\n"
        "\n"
        "    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)\n"
        "        // REFLECT_CAMPO(velocidad)\n"
        "    REFLECT_FIN\n"
        "\n"
        "    void onStart(GameObject* owner) override { (void)owner; }\n"
        "    void onUpdate(GameObject* owner, float deltaTime) override {\n"
        "        (void)owner; (void)deltaTime;\n"
        "    }\n"
        "    void onStop(GameObject* owner) override { (void)owner; }\n"
        "\n"
        "    std::vector<::ReflejoScripts::DefCampo>\n"
        "    camposReflejados() const override { return reflexion(); }\n"
        "};\n"
        "\n"
        "// Export requerida por el backend del motor; no renombrar.\n"
        "extern \"C\" IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(\n"
        "    const MotorScript::ApiScriptGameObject* api) {\n"
        "    (void)api;\n"
        "    return new FUNSHI_NOMBRE_CLASE();\n"
        "}\n";
}