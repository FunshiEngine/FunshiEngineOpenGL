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
#include "GestorDeArchivos.h"

#include <filesystem>
#include <fstream>
#include <utility>

GestorDeArchivos::GestorDeArchivos(std::string pathProyect, std::string rootName)
    : treeFilePath(new ArbolEnlazado<File*>()), folderActual(nullptr), rootName(std::move(rootName)) {
    setTreeFilePath(pathProyect, this->rootName);
}

GestorDeArchivos::~GestorDeArchivos() {
    if (treeFilePath && !treeFilePath->isEmpty())
        liberarPreOrden(treeFilePath, treeFilePath->rootOfTree());
    delete treeFilePath;
    treeFilePath = nullptr;
    folderActual = nullptr;
}

ArbolEnlazado<File*>* GestorDeArchivos::getTreeFilePath() { return treeFilePath; }

void GestorDeArchivos::recorrerDir(const std::string& path, Position<File*>* parent) {
    std::error_code ec;
    std::filesystem::directory_iterator it(path, ec);
    if (ec) return;
    const std::filesystem::directory_iterator fin;
    for (; it != fin;) {
        const std::filesystem::directory_entry entrada = *it;
        it.increment(ec);
        if (ec) { ec.clear(); continue; } // entrada con errores: la saltamos

        const std::string nombre = entrada.path().filename().string();
        if (nombre == "." || nombre == "..") continue;
        const std::string fullPath = entrada.path().string();

        // No seguir enlaces simbolicos: pueden apuntar a carpetas del sistema
        // o a si mismos (recursion infinita -> arbol gigante / stack overflow).
        if (std::filesystem::is_symlink(entrada.symlink_status())) continue;
        const bool esCarpeta = entrada.is_directory();

        File* item = esCarpeta
            ? static_cast<File*>(new Carpeta(nombre)) : new File(nombre);
        item->setPathRoot(path);
        Position<File*>* child = treeFilePath->addNodeChildOf(parent, item);
        if (esCarpeta) recorrerDir(fullPath, child);
    }
}

bool GestorDeArchivos::setTreeFilePath(const std::string& path, std::string name) {
    if (!name.empty()) rootName = std::move(name);
    auto* newTree = new ArbolEnlazado<File*>();
    Carpeta* root = new Carpeta(rootName);
    std::filesystem::path fsPath(path);
    if (!fsPath.empty() && fsPath.filename().string() == rootName) {
        root->setPathRoot(fsPath.parent_path().string());
    } else {
        root->setPathRoot(path);
    }
    Position<File*>* rootPosition = newTree->createRoot(root);
    auto* original = treeFilePath;
    treeFilePath = newTree;
    recorrerDir(path, rootPosition);
    if (!compareTreesByPath(original, newTree)) {
        folderActual = rootPosition;
        // Liberar el arbol reemplazado junto a sus File*.
        if (original && !original->isEmpty())
            liberarPreOrden(original, original->rootOfTree());
        delete original;
        return true;
    }
    treeFilePath = original;
    // Liberar el arbol descartado en la comparacion junto a sus File*.
    if (newTree && !newTree->isEmpty())
        liberarPreOrden(newTree, newTree->rootOfTree());
    delete newTree;
    return false;
}

void GestorDeArchivos::liberarPreOrden(ArbolEnlazado<File*>* arbol,
                                       Position<File*>* p) {
    if (!arbol || !p) return;
    delete p->getElement();
    if (arbol->isInternal(p)) {
        auto* hijos = arbol->childsOf(p);
        auto* h = hijos->first();
        while (h) {
            liberarPreOrden(arbol, h->getElement());
            h = (h != hijos->last()) ? hijos->next(h) : nullptr;
        }
        delete hijos; // childsOf() asigna una lista nueva en cada llamada
    }
}

bool GestorDeArchivos::compareTreesByPath(ArbolEnlazado<File*>* first,
                                          ArbolEnlazado<File*>* second) {
    if (first->isEmpty() && second->isEmpty()) return true;
    if (first->isEmpty() || second->isEmpty()) return false;
    return preOrdenNoExaustivo(first, second, first->rootOfTree(), second->rootOfTree());
}

bool GestorDeArchivos::preOrdenNoExaustivo(ArbolEnlazado<File*>* first,
                                           ArbolEnlazado<File*>* second,
                                           Position<File*>* left, Position<File*>* right) {
    if (!left || !right) return false;
    File* a = left->getElement();
    File* b = right->getElement();
    if (a->getPathName() != b->getPathName() || a->getPathRoot() != b->getPathRoot()) return false;
    const bool externalA = first->isExternal(left), externalB = second->isExternal(right);
    if (externalA && externalB) return true;
    if (externalA != externalB) return false;
    auto* childrenA = first->childsOf(left);
    auto* childrenB = second->childsOf(right);
    if (childrenA->tam() != childrenB->tam()) {
        delete childrenA; // childsOf() asigna una lista nueva en cada llamada
        delete childrenB;
        return false;
    }
    auto* pA = childrenA->first();
    auto* pB = childrenB->first();
    while (pA && pB) {
        if (!preOrdenNoExaustivo(first, second, pA->getElement(), pB->getElement())) {
            delete childrenA;
            delete childrenB;
            return false;
        }
        pA = (pA != childrenA->last()) ? childrenA->next(pA) : nullptr;
        pB = (pB != childrenB->last()) ? childrenB->next(pB) : nullptr;
    }
    delete childrenA;
    delete childrenB;
    return true;
}

// std::filesystem no lanza excepciones usados con error_code. crearArchivo
// usa ofstream (sin excepciones de FS configuradas).
bool GestorDeArchivos::eliminarCarpeta(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    const std::uintmax_t removidos = std::filesystem::remove_all(path, ec);
    return !ec && removidos > 0;
}

bool GestorDeArchivos::eliminarArchivo(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) return false;
    std::filesystem::remove(path, ec);
    return !ec;
}

bool GestorDeArchivos::crearCarpeta(const std::string& path) {
    if (path.empty()) return false;
    std::error_code ec;
    if (std::filesystem::create_directory(path, ec)) return true;
    // create_directory devuelve false si ya existe (sin error): es exitoso.
    return std::filesystem::is_directory(path, ec);
}

bool GestorDeArchivos::crearArchivo(const std::string& path,
                                    const std::string& contenido) {
    if (path.empty()) return false;
    std::ofstream archivo(path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!archivo) return false;
    if (!contenido.empty()) archivo << contenido;
    archivo.close();
    return !archivo.fail();
}

bool GestorDeArchivos::copiarCarpeta(const std::string& origen,
                                     const std::string& destino) {
    if (origen.empty() || destino.empty()) return false;
    std::error_code ec;
    // Si destino no existe, copy() lo crea y vuelca el contenido de origen
    // dentro (con dotfiles, a diferencia del "cp -r src/* dst" anterior).
    std::filesystem::copy(origen, destino,
                          std::filesystem::copy_options::recursive |
                          std::filesystem::copy_options::overwrite_existing,
                          ec);
    return !ec && std::filesystem::is_directory(destino, ec);
}

bool GestorDeArchivos::copiarArchivo(const std::string& origen,
                                     const std::string& destino) {
    if (origen.empty() || destino.empty()) return false;
    std::error_code ec;
    std::filesystem::copy_file(origen, destino,
                               std::filesystem::copy_options::overwrite_existing,
                               ec);
    return !ec;
}

bool GestorDeArchivos::renombrar(const std::string& ruta,
                                 const std::string& nuevoNombre) {
    if (ruta.empty() || nuevoNombre.empty()) return false;
    const std::filesystem::path objetivo(ruta);
    // Cualquier separador / \ es invalido en un nombre de salida; no dejar
    // que un nombre malicioso cree una ruta nueva por accidente.
    if (nuevoNombre.find_first_of("/\\") != std::string::npos) return false;
    std::error_code ec;
    std::filesystem::rename(objetivo, objetivo.parent_path() / nuevoNombre, ec);
    return !ec;
}

bool GestorDeArchivos::mover(const std::string& origen,
                             const std::string& destino) {
    if (origen.empty() || destino.empty()) return false;

    std::error_code ec;
    const std::filesystem::path desde(origen);
    const std::filesystem::path hacia(destino);

    if (!std::filesystem::exists(desde, ec)) return false;
    // No se pisa un destino existente: un arrastre mal dirigido dejaria el
    // original intacto y el usuario creeria que se movio cuando no.
    if (std::filesystem::exists(hacia, ec)) return false;

    // Carpeta dentro de si misma (o de un descendiente): el error_code de la
    // recursion cortaria a mitad y dejaria el arbol a medias en disco.
    if (std::filesystem::is_directory(desde, ec)) {
        const std::filesystem::path padreDestino =
            hacia.parent_path().lexically_normal();
        std::filesystem::path actual = desde.lexically_normal();
        for (;;) {
            if (actual == padreDestino) return false;
            const std::filesystem::path padre = actual.parent_path();
            if (padre == actual) break; // se llego a la raiz sin coincidir
            actual = padre;
        }
    }

    std::filesystem::rename(desde, hacia, ec);
    if (!ec) return true;

    // rename solo funciona dentro del mismo volumen. Entre volumenes (o con el
    // error de permisos que deja Windows en algunos casos) se cae a copiar y
    // borrar el origen, que es mas lento pero produce el mismo resultado.
    ec.clear();
    if (std::filesystem::is_directory(desde, ec)) {
        std::filesystem::copy(desde, hacia,
                              std::filesystem::copy_options::recursive,
                              ec);
    } else {
        std::filesystem::copy_file(desde, hacia,
                                   std::filesystem::copy_options::none, ec);
    }
    if (ec) return false;

    ec.clear();
    if (std::filesystem::is_directory(desde, ec))
        std::filesystem::remove_all(desde, ec);
    else
        std::filesystem::remove(desde, ec);
    return !ec;
}