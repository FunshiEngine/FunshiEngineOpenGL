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
#include "TiposArchivo.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <nlohmann/json.hpp>

namespace TiposArchivo {

namespace {

// Un .json mas grande que esto no es ni un tileset ni un perfil de efecto: no
// se lee entero para clasificarlo.
constexpr std::streamsize kMaximoJson = 4 * 1024 * 1024;

bool esUna(const std::string& ext, std::initializer_list<const char*> lista) {
    for (const char* e : lista)
        if (ext == e) return true;
    return false;
}

}  // namespace

std::string extension(const std::string& ruta) {
    std::string ext = std::filesystem::path(ruta).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return ext;
}

TipoArchivo clasificarJson(const std::string& texto) {
    const nlohmann::json raiz = nlohmann::json::parse(texto, nullptr, false);
    if (raiz.is_discarded() || !raiz.is_object()) return TipoArchivo::Desconocido;
    // Tileset estilo Tiled: imagen + medidas del tile.
    if (raiz.contains("tilewidth") && raiz.contains("tileheight") && raiz.contains("image"))
        return TipoArchivo::Tileset;
    // Perfil de particulas: vida y velocidad son obligatorias en LectorPerfil.
    if (raiz.contains("vida") && raiz.contains("velocidad"))
        return TipoArchivo::PerfilParticulas;
    return TipoArchivo::Desconocido;
}

TipoArchivo clasificar(const std::string& ruta) {
    const std::string ext = extension(ruta);
    if (ext == ".prefab") return TipoArchivo::Prefab;
    if (ext == ".fnep") return TipoArchivo::Proyecto;
    if (esUna(ext, {".obj", ".fbx", ".gltf", ".glb", ".dae", ".3ds", ".blend",
                    ".ply", ".stl", ".x", ".md5mesh"}))
        return TipoArchivo::Modelo3D;
    if (esUna(ext, {".png", ".jpg", ".jpeg", ".jpe", ".tga", ".bmp", ".psd", ".gif",
                    ".hdr"}))
        return TipoArchivo::Imagen;
    if (esUna(ext, {".wav", ".ogg", ".mp3", ".flac"})) return TipoArchivo::Audio;
    if (esUna(ext, {".html", ".htm"})) return TipoArchivo::InterfazHtml;
    if (esUna(ext, {".cpp", ".cc", ".cxx"})) return TipoArchivo::ScriptCpp;
    if (ext == ".java") return TipoArchivo::ScriptJava;
    if (ext == ".json") {
        std::ifstream entrada(ruta, std::ios::binary);
        if (!entrada) return TipoArchivo::Desconocido;
        std::string texto;
        texto.resize(static_cast<std::size_t>(kMaximoJson));
        entrada.read(&texto[0], kMaximoJson);
        texto.resize(static_cast<std::size_t>(entrada.gcount()));
        if (entrada.gcount() >= kMaximoJson) return TipoArchivo::Desconocido;
        return clasificarJson(texto);
    }
    return TipoArchivo::Desconocido;
}

const char* nombre(TipoArchivo tipo) {
    switch (tipo) {
    case TipoArchivo::Prefab: return "Prefab";
    case TipoArchivo::Modelo3D: return "Modelo 3D";
    case TipoArchivo::Imagen: return "Imagen";
    case TipoArchivo::Audio: return "Audio";
    case TipoArchivo::Tileset: return "Tileset";
    case TipoArchivo::PerfilParticulas: return "Perfil de particulas";
    case TipoArchivo::InterfazHtml: return "Interfaz HTML";
    case TipoArchivo::ScriptCpp: return "Script C++";
    case TipoArchivo::ScriptJava: return "Script Java";
    case TipoArchivo::Proyecto: return "Proyecto";
    case TipoArchivo::Desconocido: break;
    }
    return "Desconocido";
}

}  // namespace TiposArchivo
