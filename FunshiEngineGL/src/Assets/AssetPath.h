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
#ifndef ASSETPATH_H
#define ASSETPATH_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <string>

// Utilidades de rutas de assets, separadas del resto del proyecto para
// centralizar la resolucion de recursos (pendiente "portabilidad de rutas").
// Header-only: no depende de OpenGL ni de la pila grafica, por eso los tests
// headless pueden ejercitarla en cualquier plataforma.
namespace AssetPath {

// Unifica separadores ('\' -> '/'), colapsa '/' repetidos y quita el '/' final
// (salvo la raiz de unidad estilo "C:/" en Windows). Las claves de AssetManager
// se normalizan con esto: pedir "Meshes//cubo.obj" o "Meshes\cubo.obj" debe
// devolver el MISMO recurso cacheados.
//
// Los prefijos de red y de Windows largo se tratan aparte, porque perderlos
// convierte la ruta en otra cosa: "\\servidor\recurso\a.obj" es una ruta de red
// absoluta y, si las barras iniciales se colapsan, queda "/servidor/recurso/a.obj",
// que el motor busca en el disco local y nunca encuentra. Con "\\?\" (longitud
// extendida) y "\\.\" (dispositivo) la ruta se entrega al sistema tal cual, sin
// traducir separadores: con ese prefijo Windows no convierte '/' en '\'.
inline std::string normalize(std::string path) {
    const bool prefijoLargo = path.rfind("\\\\?\\", 0) == 0 ||
                              path.rfind("\\\\.\\", 0) == 0;
    if (prefijoLargo) return path;

    std::replace(path.begin(), path.end(), '\\', '/');

    // El colapso de barras conserva el par inicial: es el prefijo UNC, no un
    // separador repetido.
    const bool esUnc = path.rfind("//", 0) == 0;
    std::string result;
    result.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i] == '/' && !result.empty() && result.back() == '/') {
            if (esUnc && result.size() == 1) result.push_back('/');
            continue;
        }
        result.push_back(path[i]);
    }

    while (result.size() > 1 && result.back() == '/') {
        // Preserva la raiz de unidad de Windows ("C:/").
        if (result.size() > 2 && result[result.size() - 2] == ':') break;
        result.pop_back();
    }
    return result;
}

inline std::string lowercase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

// Extension en minusculas ("obj", "fbx") o vacia si la ruta no tiene.
inline std::string extension(const std::string& path) {
    const std::string n = normalize(path);
    const size_t dot = n.find_last_of('.');
    const size_t sep = n.find_last_of('/');
    if (dot == std::string::npos || (sep != std::string::npos && dot < sep))
        return "";
    return lowercase(n.substr(dot + 1));
}

// true si la extension coincide (sin distinguir mayusculas).
inline bool hasExtension(const std::string& path, const std::string& ext) {
    return extension(path) == lowercase(ext);
}

// Une base + relativa normalizando todo ("/base" + "rel/m.obj" -> "/base/rel/m.obj").
inline std::string join(const std::string& base, const std::string& rel) {
    const std::string b = normalize(base);
    const std::string r = normalize(rel);
    if (b.empty() || r.empty()) return b.empty() ? r : b;
    // El normalize final colapsa el separador duplicado si r es absoluta
    // ("/base" + "/rel/m.obj" -> "/base/rel/m.obj").
    return normalize(b + "/" + r);
}

inline bool isAbsolute(const std::string& path) {
    return std::filesystem::path(normalize(path)).is_absolute();
}

// Nombre del archivo sin la extension: "models/cubo.obj" -> "cubo".
inline std::string basename(const std::string& path) {
    const std::string n = normalize(path);
    const size_t sep = n.find_last_of('/');
    std::string name = (sep == std::string::npos) ? n : n.substr(sep + 1);
    const size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name.erase(dot);
    return name;
}

} // namespace AssetPath

#endif