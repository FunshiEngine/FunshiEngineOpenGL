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
#ifndef ICONOSGUI_H
#define ICONOSGUI_H

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include <string>
#include <imgui.h>

#include "../../Rendering/Backend/IRenderBackend.h"

class IconosGUI {
public:
    IconosGUI();
    ~IconosGUI();

    void init();

    bool estaInicializado() const { return inicializado; }

    ImTextureID getIconoCarpeta() const { return aImTexture(iconoCarpeta); }
    ImTextureID getIconoArchivo() const { return aImTexture(iconoArchivo); }
    ImTextureID getIconoCpp() const { return aImTexture(iconoCpp); }
    ImTextureID getIconoHpp() const { return aImTexture(iconoHpp); }
    ImTextureID getIconoJava() const { return aImTexture(iconoJava); }
    ImTextureID getIconoGameObject() const { return aImTexture(iconoGameObject); }
    ImTextureID getIconoLogo() const { return aImTexture(iconoLogo); }
    // Proporcion ancho/alto del logo (1 si no cargo): util para dibujarlo
    // con un alto dado manteniendo el aspecto del asset.
    float aspectoLogo() const {
        return altoLogo > 0 ? static_cast<float>(anchoLogo) / altoLogo : 1.f;
    }
    ImTextureID getIconoPorExtension(const std::string& extension) const;

private:
    // La GUI nunca ve handles: los convierte el backend a ImTextureID.
    static ImTextureID aImTexture(Rendering::Backend::Handle handle);
    Rendering::Backend::Handle cargarPNG(const char* nombrePNG);
    Rendering::Backend::Handle cargarPNG(const char* nombrePNG, int* ancho,
                                         int* alto);

    // Iconos base del explorador de archivos (ya existentes).
    Rendering::Backend::Handle iconoCarpeta = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoArchivo = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoCpp = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoHpp = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoJava = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoGameObject = Rendering::Backend::kInvalidHandle;

    // Logo del motor (marca del editor; se muestra en el menu superior).
    Rendering::Backend::Handle iconoLogo = Rendering::Backend::kInvalidHandle;
    // Dimensiones del logo (rellenadas en init; proporcion para dibujarlo).
    int anchoLogo = 0;
    int altoLogo = 0;

    // Iconos por extension de asset/formato (Imagenes/ + nombre.png).
    Rendering::Backend::Handle iconoBlend = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoCsv = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoExr = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoFbx = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoHdr = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoJpeg = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoJpg = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoJson = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoMax = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoMaya = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoMp3 = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoObj = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoOgg = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoOtf = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoPng = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoPsd = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoRs = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoTga = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoTtf = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoWav = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoXml = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoDb = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoMtl = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoRar = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle iconoZip = Rendering::Backend::kInvalidHandle;
    bool inicializado = false;
};

#endif