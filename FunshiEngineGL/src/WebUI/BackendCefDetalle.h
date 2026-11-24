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
// Preambulo compartido por las unidades de BackendCef.cpp (generado al dividirlo).
#ifndef BACKENDCEFDETALLE_H
#define BACKENDCEFDETALLE_H

// Unica unidad del motor que incluye CEF. CEF 154 exige C++20, asi que este
// archivo se compila aparte (CXX_STANDARD 20) y no se incluye ningun header de
// CEF en el resto del motor. El puente con el motor es BackendCef.h (pimpl) e
// IWebUIBackend.
#include "BackendCef.h"
#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/cef_client.h"
#include "include/cef_display_handler.h"
#include "include/cef_life_span_handler.h"
#include "include/cef_load_handler.h"
#include "include/cef_render_handler.h"
#include "../Rendering/Backend/IRenderBackend.h"
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

// Sentinela con el que la pagina vuelca su salida al motor. El backend corre
// `console.log(SENTINELA + JSON.stringify(FunshiUI.salida))` y CefDisplayHandler
// recorta el prefijo. Es un canal interno backend<->pagina; el contrato de la
// pagina sigue siendo `window.FunshiUI` (entrada por actualizar(), salida por
// FunshiUI.salida), sin conocer que hay CEF detras.
constexpr char kSentinelaSalida[] = "__funshi_salida__";

}  // namespace

// ---------------------------------------------------------------------------
// Estado interno del backend y su cliente de CEF
// ---------------------------------------------------------------------------

struct BackendCefImpl {
    CefRefPtr<CefBrowser> browser;
    CefRefPtr<class ClienteCef> cliente;

    int ancho = 0;
    int alto = 0;
    std::string ruta;
    bool cargado = false;

    // Ultimo frame OSR entregado por CEF, en BGRA. OnPaint lo deja aca y render()
    // lo convierte a RGBA y lo sube a la textura del backend grafico.
    std::vector<unsigned char> frame;
    std::vector<unsigned char> rgba;
    int frameAncho = 0;
    int frameAlto = 0;
    bool frameNuevo = false;
    bool pintado = false;

    // Textura del backend grafico de la vista (0 = sin textura todavia).
    std::uint64_t textura = 0;

    // Ultima salida leida del JS (FunshiUI.salida serializada).
    std::string salida;

    // Ultima posicion del raton en coordenadas locales (para los clics, que CEF
    // pide junto a la posicion).
    int ratonX = 0;
    int ratonY = 0;
};

// Cliente de CEF de UNA vista: render off-screen, titulo/consola, carga y ciclo
// de vida. Todos los callbacks corren en el hilo del motor porque el bucle de
// mensajes se bombea con CefDoMessageLoopWork() desde bombear().
class ClienteCef : public CefClient,
                   public CefRenderHandler,
                   public CefDisplayHandler,
                   public CefLoadHandler,
                   public CefLifeSpanHandler {
public:
    explicit ClienteCef(BackendCefImpl* dueno) : dueno(dueno) {}

    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }

    // --- CefLifeSpanHandler ---
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        if (dueno) dueno->browser = browser;
    }

    // --- CefRenderHandler (OSR) ---
    void GetViewRect(CefRefPtr<CefBrowser>, CefRect& rect) override {
        const int w = dueno ? dueno->ancho : 1;
        const int h = dueno ? dueno->alto : 1;
        rect = CefRect(0, 0, w > 0 ? w : 1, h > 0 ? h : 1);
    }

    bool GetScreenInfo(CefRefPtr<CefBrowser>, CefScreenInfo& info) override {
        const int w = dueno ? dueno->ancho : 1;
        const int h = dueno ? dueno->alto : 1;
        info.device_scale_factor = 1.0f;
        info.rect = CefRect(0, 0, w > 0 ? w : 1, h > 0 ? h : 1);
        return true;
    }

    void OnPaint(CefRefPtr<CefBrowser>,
                 PaintElementType type,
                 const RectList&,
                 const void* buffer,
                 int width,
                 int height) override {
        if (!dueno || type != PET_VIEW || !buffer || width <= 0 || height <= 0)
            return;
        const size_t bytes =
            static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
        if (dueno->frame.size() != bytes) dueno->frame.resize(bytes);
        std::memcpy(dueno->frame.data(), buffer, bytes);
        dueno->frameAncho = width;
        dueno->frameAlto = height;
        dueno->frameNuevo = true;
        dueno->pintado = true;
    }

    // --- CefDisplayHandler ---
    bool OnConsoleMessage(CefRefPtr<CefBrowser>,
                          cef_log_severity_t,
                          const CefString& message,
                          const CefString&,
                          int) override {
        if (!dueno) return false;
        const std::string m = message.ToString();
        const size_t prefijo = sizeof(kSentinelaSalida) - 1;
        if (m.compare(0, prefijo, kSentinelaSalida) == 0)
            dueno->salida = m.substr(prefijo);
        return true;
    }

    // --- CefLoadHandler ---
    void OnLoadingStateChange(CefRefPtr<CefBrowser>,
                              bool isLoading,
                              bool,
                              bool) override {
        if (dueno && !isLoading) dueno->cargado = true;
    }

    BackendCefImpl* dueno = nullptr;

private:
    IMPLEMENT_REFCOUNTING(ClienteCef);
};

// Aplicacion CEF minima. No usa el browser process handler: los navegadores los
// crea el propio backend en cargar().
class AppCef : public CefApp {
public:
    AppCef() = default;

private:
    IMPLEMENT_REFCOUNTING(AppCef);
};

namespace {

// Ruta del directorio del ejecutable (para que CEF encuentre libcef.so,
// resources.pak, icudtl.dat y locales/ junto al binario).
std::string directorioEjecutable(char** argv) {
    try {
        namespace fs = std::filesystem;
        if (!argv || !argv[0] || argv[0][0] == '\0') return std::string();
        fs::path exe = fs::absolute(fs::path(argv[0]));
        // Normaliza componentes '.'/'..' (p. ej. "./build/FunshiEngineGL" ->
        // "/home/.../build") para que CEF no rechace el path de recursos.
        return exe.lexically_normal().parent_path().string();
    } catch (...) {
        return std::string();
    }
}

}  // namespace

#endif
