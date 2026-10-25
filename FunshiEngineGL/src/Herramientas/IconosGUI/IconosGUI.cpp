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

// Windows.h ANTES del header propio y de la stdlib, para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte.
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "IconosGUI.h"
#include "RutasIconos.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>

// Solo las declaraciones: la implementacion de stb_image vive en su propia
// unidad (External/stb_image/stb_image_impl.cpp).
#include <stb_image/stb_image.h>

IconosGUI::IconosGUI() {}

IconosGUI::~IconosGUI() {
    // Todos los iconos cargados se liberan igual; conviene recorrer el array
    // en vez de repetir el bloque destroyTexture2D para cada miembro.
    Rendering::Backend::Handle* iconos[] = {
        &iconoCarpeta, &iconoArchivo, &iconoCpp, &iconoHpp, &iconoJava,
        &iconoGameObject, &iconoDepuracion, &iconoPlay, &iconoPausa,
        &iconoReset, &iconoStop, &iconoLogo, &iconoBlend, &iconoCsv, &iconoExr,
        &iconoFbx, &iconoHdr, &iconoJpeg, &iconoJpg, &iconoJson, &iconoMax,
        &iconoMaya, &iconoMp3, &iconoObj, &iconoPrefab, &iconoOgg, &iconoOtf, &iconoPng,
        &iconoPsd, &iconoRs, &iconoTga, &iconoTtf, &iconoWav, &iconoXml,
        &iconoDb, &iconoMtl, &iconoRar, &iconoZip,
    };
    for (Rendering::Backend::Handle* icono : iconos) {
        if (*icono != Rendering::Backend::kInvalidHandle) {
            Rendering::Backend::activeBackend().destroyTexture2D(*icono);
            *icono = Rendering::Backend::kInvalidHandle;
        }
    }
    // Los iconos de componente se cargaron a demanda: se liberan tambien.
    for (auto& par : iconosComponentes_) {
        if (par.second != Rendering::Backend::kInvalidHandle) {
            Rendering::Backend::activeBackend().destroyTexture2D(par.second);
        }
    }
    iconosComponentes_.clear();
}

void IconosGUI::init() {
    if (inicializado) return;
    iconoCarpeta = cargarPNG("File-Image/folder.png");
    iconoArchivo = cargarPNG("File-Image/file.png");
    iconoCpp = cargarPNG("File-Image/cpp.png");
    iconoHpp = cargarPNG("File-Image/hpp.png");
    iconoJava = cargarPNG("File-Image/java.png");
    iconoGameObject = cargarPNG("GUI-Image/Inspector/cubo.png");
    iconoDepuracion = cargarPNG("GUI-Image/MenuBar/debug-button-white.png");
    iconoPlay = cargarPNG("GUI-Image/MenuBar/play-button-triangle-white.png");
    iconoPausa = cargarPNG("GUI-Image/MenuBar/pause-button-white.png");
    iconoReset = cargarPNG("GUI-Image/MenuBar/restart-button-white.png");
    iconoStop = cargarPNG("GUI-Image/MenuBar/stop-button-white.png");
    // El logo guarda sus dimensiones para que los consumidores respeten la
    // proporcion del asset a cualquier alto de pantalla.
    iconoLogo = cargarPNG("Logo/FunshiEngineGL_Isotipo_Blanco.png", &anchoLogo,
                          &altoLogo);
    if (altoLogo < 1) altoLogo = 1;
    // Iconos por extension de asset/formato (Imagenes/File-Image/ + nombre.png).
    iconoBlend = cargarPNG("File-Image/blend.png");
    iconoCsv = cargarPNG("File-Image/csv.png");
    iconoExr = cargarPNG("File-Image/exr.png");
    iconoFbx = cargarPNG("File-Image/fbx.png");
    iconoHdr = cargarPNG("File-Image/hdr.png");
    iconoJpeg = cargarPNG("File-Image/jpeg.png");
    iconoJpg = cargarPNG("File-Image/jpg.png");
    iconoJson = cargarPNG("File-Image/json.png");
    iconoMax = cargarPNG("File-Image/max.png");
    iconoMaya = cargarPNG("File-Image/maya.png");
    iconoMp3 = cargarPNG("File-Image/mp3.png");
    iconoObj = cargarPNG("File-Image/obj.png");
    iconoPrefab = cargarPNG("File-Image/prefab.png");
    iconoOgg = cargarPNG("File-Image/ogg.png");
    iconoOtf = cargarPNG("File-Image/otf.png");
    iconoPng = cargarPNG("File-Image/png.png");
    iconoPsd = cargarPNG("File-Image/psd.png");
    iconoRs = cargarPNG("File-Image/rs.png");
    iconoTga = cargarPNG("File-Image/tga.png");
    iconoTtf = cargarPNG("File-Image/ttf.png");
    iconoWav = cargarPNG("File-Image/wav.png");
    iconoXml = cargarPNG("File-Image/xml.png");
    iconoDb = cargarPNG("File-Image/db.png");
    iconoMtl = cargarPNG("File-Image/mtl.png");
    iconoRar = cargarPNG("File-Image/rar.png");
    iconoZip = cargarPNG("File-Image/zip.png");
    inicializado = true;
}

ImTextureID IconosGUI::aImTexture(Rendering::Backend::Handle handle) {
    if (handle == Rendering::Backend::kInvalidHandle) return ImTextureID_Invalid;
    const void* descriptor =
        Rendering::Backend::activeBackend().imguiTextureId(handle);
    return descriptor
               ? static_cast<ImTextureID>(reinterpret_cast<std::intptr_t>(
                     descriptor))
               : ImTextureID_Invalid;
}

Rendering::Backend::Handle IconosGUI::cargarPNG(const char* nombrePNG) {
    int ancho = 0, alto = 0;
    return cargarPNG(nombrePNG, &ancho, &alto);
}

Rendering::Backend::Handle IconosGUI::cargarPNG(const char* nombrePNG,
                                                int* ancho, int* alto) {
    // El orden de busqueda vive en RutasIconos (compartido con las pruebas
    // headless, que comprueban que cada icono declarado exista en disco).
    const std::string rutaEncontrada = RutasIconos::resolverRuta(nombrePNG);
    if (rutaEncontrada.empty()) {
        std::cerr << "[IconosGUI] No se encontro la imagen '" << nombrePNG
                  << "' (se probaron varias rutas relativas al ejecutable y al "
                     "directorio de trabajo).\n";
        return Rendering::Backend::kInvalidHandle;
    }

    int anchoLocal = 0, altoLocal = 0, canales = 0;
    unsigned char* pixeles =
        stbi_load(rutaEncontrada.c_str(), &anchoLocal, &altoLocal, &canales, 4);
    if (!pixeles) {
        std::cerr << "[IconosGUI] stbi_load fallo en: " << rutaEncontrada << "\n";
        return Rendering::Backend::kInvalidHandle;
    }
    if (ancho) *ancho = anchoLocal;
    if (alto) *alto = altoLocal;

    // La textura se crea por el backend (con mipmaps trilineales para que la
    // minificacion a tamanos chicos no produzca "dientes"/alias) y la GUI
    // recibe el descriptor opaco cuando la pinta. Ningun GL aca.
    Rendering::Backend::Image2D gpuImage;
    gpuImage.width = anchoLocal;
    gpuImage.height = altoLocal;
    gpuImage.pixels = pixeles;
    gpuImage.generateMipmaps = true;
    const Rendering::Backend::Handle textura =
        Rendering::Backend::activeBackend().createTexture2D(gpuImage);

    stbi_image_free(pixeles);
    if (textura == Rendering::Backend::kInvalidHandle) {
        std::cerr << "[IconosGUI] createTexture2D fallo en: " << rutaEncontrada
                  << "\n";
        return Rendering::Backend::kInvalidHandle;
    }
    std::cout << "[IconosGUI] Textura cargada: " << rutaEncontrada
              << " (" << anchoLocal << "x" << altoLocal << ")\n";
    return textura;
}

ImTextureID IconosGUI::getIconoPorExtension(const std::string& extension) const {
    std::string ext = extension;
    for (auto& c : ext) c = (char)tolower((unsigned char)c);

    if (ext == ".cpp" || ext == ".cc" || ext == ".cxx" || ext == ".c") return aImTexture(iconoCpp);
    if (ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx") return aImTexture(iconoHpp);
    if (ext == ".java") return aImTexture(iconoJava);

    // Assets y formatos: modelos, imagenes, audio, fuentes y datos.
    if (ext == ".blend" || ext == ".blend1") return aImTexture(iconoBlend);      // Blender
    if (ext == ".max" || ext == ".3ds") return aImTexture(iconoMax);             // 3ds Max
    if (ext == ".ma" || ext == ".mb") return aImTexture(iconoMaya);              // Maya
    if (ext == ".fbx") return aImTexture(iconoFbx);
    if (ext == ".obj") return aImTexture(iconoObj);
    if (ext == ".prefab") return aImTexture(iconoPrefab);
    if (ext == ".png") return aImTexture(iconoPng);
    if (ext == ".jpg" || ext == ".jpe") return aImTexture(iconoJpg);
    if (ext == ".jpeg") return aImTexture(iconoJpeg);
    if (ext == ".tga") return aImTexture(iconoTga);
    if (ext == ".hdr") return aImTexture(iconoHdr);                              // HDR/OpenEXR legacy
    if (ext == ".exr") return aImTexture(iconoExr);
    if (ext == ".psd") return aImTexture(iconoPsd);
    if (ext == ".mp3") return aImTexture(iconoMp3);
    if (ext == ".wav") return aImTexture(iconoWav);
    if (ext == ".ogg") return aImTexture(iconoOgg);
    if (ext == ".ttf") return aImTexture(iconoTtf);
    if (ext == ".otf") return aImTexture(iconoOtf);
    if (ext == ".json") return aImTexture(iconoJson);
    if (ext == ".xml") return aImTexture(iconoXml);
    if (ext == ".csv") return aImTexture(iconoCsv);
    if (ext == ".rs") return aImTexture(iconoRs);                                // Rust
    if (ext == ".db" || ext == ".sqlite" || ext == ".sqlite3") return aImTexture(iconoDb); // Bases de datos
    if (ext == ".mtl") return aImTexture(iconoMtl);                              // Material Wavefront junto a .obj
    if (ext == ".rar" || ext == ".7z" || ext == ".tar" || ext == ".gz") return aImTexture(iconoRar);
    if (ext == ".zip") return aImTexture(iconoZip);
    return aImTexture(iconoArchivo);
}

ImTextureID IconosGUI::getIconoComponente(const std::string& rutaRelativa) {
    if (rutaRelativa.empty()) return ImTextureID_Invalid;
    auto it = iconosComponentes_.find(rutaRelativa);
    if (it != iconosComponentes_.end()) {
        return aImTexture(it->second);
    }
    // Primera vez que se pide ese icono: se carga y se cachea.
    const Rendering::Backend::Handle handle = cargarPNG(rutaRelativa.c_str());
    if (handle == Rendering::Backend::kInvalidHandle) {
        return ImTextureID_Invalid;
    }
    iconosComponentes_.emplace(rutaRelativa, handle);
    return aImTexture(handle);
}