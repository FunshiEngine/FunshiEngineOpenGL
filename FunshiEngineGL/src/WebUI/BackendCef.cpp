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
#include "BackendCefDetalle.h"

namespace {

// Estado global del proceso CEF (una vez por proceso).
bool g_cefInicializado = false;

// Traduce un codigo GLFW a windows_key_code de CEF. Cubre ASCII imprimible,
// control (Enter/Tab/Escape/Backspace/Supr), flechas, Home/End/Pagina, funcion
// F1-F12 y modificadores. Devuelve false si la tecla no tiene traduccion (se
// ignora sin romper el contrato).
bool teclaDeGlfw(int codigo, int& windowsKey) {
    windowsKey = 0;
    if (codigo >= 32 && codigo <= 126) {  // ASCII imprimible (GLFW = Unicode)
        windowsKey = codigo;
        if (codigo >= 'a' && codigo <= 'z')
            windowsKey = codigo - 32;  // VK en mayusculas
        return true;
    }
    switch (codigo) {
    case 257: windowsKey = 13; break;   // Enter (GLFW_KEY_ENTER)
    case 258: windowsKey = 9; break;    // Tab
    case 259: windowsKey = 8; break;    // Backspace
    case 256: windowsKey = 27; break;   // Escape
    case 261: windowsKey = 46; break;   // Supr
    case 262: windowsKey = 39; break;   // Flecha derecha (VK_RIGHT)
    case 263: windowsKey = 37; break;   // Flecha izquierda
    case 264: windowsKey = 40; break;   // Flecha abajo
    case 265: windowsKey = 38; break;   // Flecha arriba
    case 266: windowsKey = 33; break;   // Pagina arriba (VK_PRIOR)
    case 267: windowsKey = 34; break;   // Pagina abajo (VK_NEXT)
    case 268: windowsKey = 36; break;   // Home
    case 269: windowsKey = 35; break;   // End
    case 290: windowsKey = 112; break;  // F1..F12 (VK_F1..VK_F12)
    case 291: windowsKey = 113; break;
    case 292: windowsKey = 114; break;
    case 293: windowsKey = 115; break;
    case 294: windowsKey = 116; break;
    case 295: windowsKey = 117; break;
    case 296: windowsKey = 118; break;
    case 297: windowsKey = 119; break;
    case 298: windowsKey = 120; break;
    case 299: windowsKey = 121; break;
    case 300: windowsKey = 122; break;
    case 301: windowsKey = 123; break;
    case 340: windowsKey = 16; break;   // Shift (VK_SHIFT)
    case 341: windowsKey = 17; break;   // Control (VK_CONTROL)
    case 342: windowsKey = 18; break;   // Alt (VK_MENU)
    case 343: windowsKey = 91; break;   // Super (VK_LWIN)
    default: return false;
    }
    return true;
}

// Convierte una ruta de archivo en una URL file:// valida en Linux y Windows.
std::string urlDeArchivo(const std::string& ruta) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::path absoluta = fs::absolute(fs::path(ruta), ec);
    std::string s = (ec ? ruta : absoluta.string());
    for (char& c : s)
        if (c == '\\') c = '/';
    if (s.empty()) return s;
    // POSIX: "/home/x" -> "file:///home/x"; Windows: "C:/x" -> "file:///C:/x".
    if (s[0] == '/') return "file://" + s;
    return "file:///" + s;
}

}  // namespace

// ---------------------------------------------------------------------------
// BackendCefWebUI
// ---------------------------------------------------------------------------

BackendCefWebUI::BackendCefWebUI() : impl_(nullptr) {}

BackendCefWebUI::~BackendCefWebUI() { delete impl_; }

const char* BackendCefWebUI::nombre() const { return "cef"; }

BackendCefImpl* BackendCefWebUI::vistaDe(const VistaWebUI& vista) const {
    if (vista.backend != "cef") return nullptr;
    return static_cast<BackendCefImpl*>(vista.manejador);
}

int BackendCefWebUI::ejecutarSubproceso(int argc, char** argv) {
    CefMainArgs main_args(argc, argv);
    CefRefPtr<AppCef> app(new AppCef());
    return CefExecuteProcess(main_args, app.get(), nullptr);
}

bool BackendCefWebUI::inicializar(int argc, char** argv) {
    if (g_cefInicializado) return true;

    CefMainArgs main_args(argc, argv);
    CefSettings settings;
    settings.windowless_rendering_enabled = true;
    settings.no_sandbox = true;
    settings.external_message_pump = true;
    settings.multi_threaded_message_loop = false;
    settings.log_severity = LOGSEVERITY_ERROR;

    // resources_dir_path y locales_dir_path: CEF los detecta junto a libcef.so
    // (como la sonda OSR valida). Al no fijarlos, evita el fallo de ICU que se
    // produce al asignarlos explicitamente con rutas que contienen './' en este
    // entorno. Para el export, los recursos se copian junto al ejecutable y
    // CEF los encuentra por deteccion automatica.

    CefRefPtr<AppCef> app(new AppCef());
    if (!CefInitialize(main_args, settings, app.get(), nullptr)) return false;
    g_cefInicializado = true;
    return true;
}

void BackendCefWebUI::bombear() {
    if (g_cefInicializado) CefDoMessageLoopWork();
}

bool BackendCefWebUI::cargar(VistaWebUI& vista, const std::string& rutaHtml,
                             int ancho, int alto, std::string& error) {
    if (!g_cefInicializado) {
        error = "CEF no inicializado";
        return false;
    }
    if (rutaHtml.empty()) {
        error = "ruta .html vacia";
        return false;
    }

    BackendCefImpl* imp = new BackendCefImpl();
    imp->ancho = ancho > 0 ? ancho : 1;
    imp->alto = alto > 0 ? alto : 1;
    imp->ruta = rutaHtml;
    imp->cliente = new ClienteCef(imp);

    CefWindowInfo window_info;
    window_info.SetAsWindowless(0);
    CefBrowserSettings browser_settings;
    browser_settings.windowless_frame_rate = 60;

    const std::string url = urlDeArchivo(rutaHtml);
    const bool creado = CefBrowserHost::CreateBrowser(
        window_info, imp->cliente, url, browser_settings, nullptr, nullptr);
    if (!creado) {
        imp->cliente->dueno = nullptr;
        delete imp;
        error = "CefBrowserHost::CreateBrowser fallo";
        return false;
    }

    vista.manejador = imp;
    vista.ruta = rutaHtml;
    vista.backend = nombre();
    vista.ancho = ancho;
    vista.alto = alto;
    vista.textura = 0;
    vista.handleImGui = nullptr;
    vista.cargada = true;
    vista.error.clear();
    error.clear();
    return true;
}

void BackendCefWebUI::redimensionar(VistaWebUI& vista, int ancho, int alto) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp) return;
    imp->ancho = ancho > 0 ? ancho : 1;
    imp->alto = alto > 0 ? alto : 1;
    vista.ancho = ancho;
    vista.alto = alto;
    if (imp->browser) imp->browser->GetHost()->WasResized();
}

void BackendCefWebUI::actualizar(VistaWebUI& vista, float /*deltaTime*/) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    CefRefPtr<CefFrame> frame = imp->browser->GetMainFrame();
    if (!frame) return;
    // Polling de la salida: la pagina no tiene API nativa, asi que el motor lee
    // FunshiUI.salida por el canal de consola (una evaluacion por frame).
    const std::string js =
        std::string("try{console.log('") + kSentinelaSalida +
        "'+JSON.stringify((typeof FunshiUI!=='undefined'&&FunshiUI&&"
        "FunshiUI.salida!==undefined)?FunshiUI.salida:null));}catch(e){}";
    frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

void BackendCefWebUI::establecerEstado(VistaWebUI& vista,
                                       const std::string& jsonEstado) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser || jsonEstado.empty()) return;
    CefRefPtr<CefFrame> frame = imp->browser->GetMainFrame();
    if (!frame) return;
    const std::string js =
        "try{if(window.FunshiUI&&typeof FunshiUI.actualizar==='function')"
        "FunshiUI.actualizar(" +
        jsonEstado + ");}catch(e){console.error('FunshiUI.actualizar: '+e);}";
    frame->ExecuteJavaScript(js, frame->GetURL(), 0);
}

std::string BackendCefWebUI::leerSalida(VistaWebUI& vista) {
    BackendCefImpl* imp = vistaDe(vista);
    return imp ? imp->salida : std::string();
}

std::uint64_t BackendCefWebUI::render(VistaWebUI& vista) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->pintado || imp->frame.empty() || imp->frameAncho <= 0 ||
        imp->frameAlto <= 0)
        return imp ? imp->textura : 0;

    // CEF entrega BGRA; el backend grafico del motor espera RGBA8.
    const size_t bytes = imp->frame.size();
    if (imp->rgba.size() != bytes) imp->rgba.resize(bytes);
    const unsigned char* src = imp->frame.data();
    unsigned char* dst = imp->rgba.data();
    for (size_t i = 0; i + 3 < bytes; i += 4) {
        dst[i + 0] = src[i + 2];  // R
        dst[i + 1] = src[i + 1];  // G
        dst[i + 2] = src[i + 0];  // B
        dst[i + 3] = src[i + 3];  // A
    }

    Rendering::Backend::Image2D img;
    img.width = imp->frameAncho;
    img.height = imp->frameAlto;
    img.pixels = imp->rgba.data();
    img.generateMipmaps = false;

    auto& backend = Rendering::Backend::activeBackend();
    if (imp->textura == 0) {
        imp->textura = backend.createTexture2D(img);
        imp->frameNuevo = false;
    } else if (imp->frameNuevo) {
        backend.updateTexture2D(imp->textura, img);
        imp->frameNuevo = false;
    }

    vista.textura = imp->textura;
    vista.handleImGui =
        imp->textura ? backend.imguiTextureId(imp->textura) : nullptr;
    return imp->textura;
}

void BackendCefWebUI::ratonMover(VistaWebUI& vista, int x, int y) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    imp->ratonX = x;
    imp->ratonY = y;
    CefMouseEvent ev;
    ev.x = x;
    ev.y = y;
    imp->browser->GetHost()->SendMouseMoveEvent(ev, false);
}

void BackendCefWebUI::ratonBoton(VistaWebUI& vista, BotonRatonWebUI boton,
                                 bool abajo) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    CefMouseEvent ev;
    ev.x = imp->ratonX;
    ev.y = imp->ratonY;
    CefBrowserHost::MouseButtonType tipo = MBT_LEFT;
    switch (boton) {
    case BotonRatonWebUI::Derecho: tipo = MBT_RIGHT; break;
    case BotonRatonWebUI::Medio: tipo = MBT_MIDDLE; break;
    case BotonRatonWebUI::Izquierdo: tipo = MBT_LEFT; break;
    }
    imp->browser->GetHost()->SendMouseClickEvent(ev, tipo, !abajo, 1);
}

void BackendCefWebUI::ratonRueda(VistaWebUI& vista, float delta) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    CefMouseEvent ev;
    ev.x = imp->ratonX;
    ev.y = imp->ratonY;
    // Un "click" de rueda equivale a ~53 px de scroll en CEF; se redondea el
    // delta flotante que entrega el motor.
    const int pasos = static_cast<int>(delta * 53.0f);
    imp->browser->GetHost()->SendMouseWheelEvent(ev, 0, pasos);
}

void BackendCefWebUI::tecla(VistaWebUI& vista, int codigo, bool abajo,
                            bool repetir) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    int windowsKey = 0;
    if (!teclaDeGlfw(codigo, windowsKey)) return;  // sin traduccion: se ignora

    CefKeyEvent ev;
    ev.windows_key_code = windowsKey;
    ev.native_key_code = 0;
    ev.is_system_key = false;
    // Modificadores: CEF los lee de los flags del evento, no de teclas
    // sueltas; el estado Shift/Ctrl/Alt lo aporta la plataforma en el futuro
    // lienzo completo. Hoy se reenvia la tecla con modificadores en 0.
    ev.modifiers = 0;

    CefRefPtr<CefBrowserHost> host = imp->browser->GetHost();
    if (abajo) {
        // Pulsacion: keydown (+ char para imprimibles, para que el texto
        // llegue a inputs HTML). La repeticion automatica reenvia keydown.
        ev.type = KEYEVENT_KEYDOWN;
        host->SendKeyEvent(ev);
        if (codigo >= 32 && codigo <= 126) {
            ev.type = KEYEVENT_CHAR;
            ev.character = static_cast<char16_t>(codigo);
            ev.unmodified_character = static_cast<char16_t>(codigo);
            host->SendKeyEvent(ev);
        }
        (void)repetir;
    } else {
        ev.type = KEYEVENT_KEYUP;
        host->SendKeyEvent(ev);
    }
}

void BackendCefWebUI::foco(VistaWebUI& vista, bool activo) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp || !imp->browser) return;
    imp->browser->GetHost()->SetFocus(activo);
}

void BackendCefWebUI::descargar(VistaWebUI& vista) {
    BackendCefImpl* imp = vistaDe(vista);
    if (!imp) {
        vista = VistaWebUI{};
        return;
    }
    if (imp->cliente) imp->cliente->dueno = nullptr;
    if (imp->browser) imp->browser->GetHost()->CloseBrowser(true);
    if (imp->textura)
        Rendering::Backend::activeBackend().destroyTexture2D(imp->textura);
    imp->browser = nullptr;
    imp->cliente = nullptr;
    delete imp;
    vista = VistaWebUI{};
}

void BackendCefWebUI::apagar() {
    if (!g_cefInicializado) return;
    CefShutdown();
    g_cefInicializado = false;
}
