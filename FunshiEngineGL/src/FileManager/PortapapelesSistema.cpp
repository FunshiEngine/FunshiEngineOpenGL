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
// Lectura del portapapeles del sistema por plataforma.
//
// La parte que decide QUE hacer con las rutas no esta aqui: esta unidad solo
// traduce el formato del sistema (CF_HDROP en Windows, text/uri-list en
// Linux) a una lista de rutas. Los parsers son puros y viven en
// PortapapelesParsers.h, asi que se pueden probar sin portapapeles.
#ifdef _WIN32
// DEBE ir ANTES de cualquier include de stdlib en Windows para evitar la
// colision de typedef 'byte' de rpcndr.h vs std::byte (C++17).
#define _HAS_STD_BYTE 0
#endif

#include "PortapapelesSistema.h"

#include "PortapapelesParsers.h"

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <GLFW/glfw3.h>
#endif

namespace {

#ifdef _WIN32

// Abre el portapapeles del sistema y devuelve las rutas de archivos que
// tenga. El CF_HDROP es un bloque DROPFILES bloqueado en memoria global.
std::vector<std::string> leerPortapapelesWindows(std::string* motivo) {
    std::vector<std::string> rutas;

    if (!OpenClipboard(nullptr)) {
        if (motivo) {
            *motivo =
                "El portapapeles del sistema esta ocupado por otra "
                "aplicacion.";
        }
        return rutas;
    }

    HANDLE formato = GetClipboardData(CF_HDROP);
    if (formato == nullptr) {
        // No hay archivos en el portapapeles: puede haber texto o nada. No es
        // un error, solo no hay nada que pegar.
        CloseClipboard();
        return rutas;
    }

    HGLOBAL bloque = static_cast<HGLOBAL>(formato);
    const std::size_t tam = GlobalSize(bloque);
    const unsigned char* datos =
        static_cast<const unsigned char*>(GlobalLock(bloque));
    if (datos != nullptr && tam > 0) {
        rutas = PortapapelesParsers::parsearDropFiles(datos, tam);
        GlobalUnlock(bloque);
    }
    CloseClipboard();
    return rutas;
}

bool hayPortapapelesWindows(std::string* motivo) {
    (void)motivo;
    return IsClipboardFormatAvailable(CF_HDROP) != FALSE;
}

#else  // Linux

// En Linux el portapapeles de archivos es texto: el explorador del sistema
// deja un text/uri-list. GLFW expone el texto del portapapeles (la
// seleccion CLIPBOARD de X11).
std::vector<std::string> leerPortapapelesLinux(void* ventana,
                                                std::string* motivo) {
    std::vector<std::string> rutas;

    const char* texto =
        glfwGetClipboardString(static_cast<GLFWwindow*>(ventana));
    if (texto == nullptr) {
        if (motivo) {
            *motivo =
                "El portapapeles del sistema no tiene texto disponible "
                "(puede que no haya escritorio, o que la aplicacion que copio "
                "no exponga archivos).";
        }
        return rutas;
    }

    const std::string contenido(texto);
    rutas = PortapapelesParsers::parsearUriList(contenido);
    if (rutas.empty()) {
        // Algunos exploradores copian la ruta cruda en vez de la URI.
        rutas = PortapapelesParsers::parsearRutasSeparadas(contenido);
    }
    return rutas;
}

bool hayPortapapelesLinux(void* ventana, std::string* motivo) {
    // Preguntar por el contenido del portapapeles puede esperar a que la
    // aplicacion que lo tiene responda: la disponibilidad se decide por la
    // ventana, que es lo unico que se puede saber sin bloquear.
    if (ventana == nullptr) {
        if (motivo) {
            *motivo =
                "El portapapeles del sistema no esta disponible: la ventana "
                "del editor no expone un portapapeles.";
        }
        return false;
    }
    return true;
}

#endif

}  // namespace

PortapapelesSistema::PortapapelesSistema(void* ventana) : ventana(ventana) {}

PortapapelesSistema::~PortapapelesSistema() = default;

std::vector<std::string> PortapapelesSistema::archivosEnPortapapeles() const {
#ifdef _WIN32
    return leerPortapapelesWindows(nullptr);
#else
    return leerPortapapelesLinux(ventana, nullptr);
#endif
}

bool PortapapelesSistema::disponible(std::string* motivo) const {
#ifdef _WIN32
    return hayPortapapelesWindows(motivo);
#else
    return hayPortapapelesLinux(ventana, motivo);
#endif
}
