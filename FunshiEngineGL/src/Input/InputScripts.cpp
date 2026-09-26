#include "InputScripts.h"

#include <unordered_map>

// Estado por tecla: down (sostenida) y edges pendientes de consumir.
namespace {
struct EstadoTecla {
    bool down = false;
    bool pressEdge = false;
    bool releaseEdge = false;
};

std::unordered_map<int, EstadoTecla>& estados() {
    static std::unordered_map<int, EstadoTecla> mapa;
    return mapa;
}
} // namespace

void InputScripts::onKey(int key, int action) {
    if (key < 0) return;
    EstadoTecla& e = estados()[key];
    if (action == 1) {          // GLFW_PRESS
        if (!e.down) e.pressEdge = true;
        e.down = true;
    } else if (action == 0) {   // GLFW_RELEASE
        if (e.down) e.releaseEdge = true;
        e.down = false;
    }
    // action == 2 (GLFW_REPEAT) no altera el estado.
}

void InputScripts::avanzarFrame() {
    for (auto& par : estados()) {
        par.second.pressEdge = false;
        par.second.releaseEdge = false;
    }
}

bool InputScripts::sostiene(const std::string& tecla) const {
    const int codigo = codigoDe(tecla);
    if (codigo < 0) return false;
    const auto it = estados().find(codigo);
    return it != estados().end() && it->second.down;
}

bool InputScripts::presionada(const std::string& tecla) const {
    const int codigo = codigoDe(tecla);
    if (codigo < 0) return false;
    const auto it = estados().find(codigo);
    return it != estados().end() && it->second.pressEdge;
}

bool InputScripts::soltada(const std::string& tecla) const {
    const int codigo = codigoDe(tecla);
    if (codigo < 0) return false;
    const auto it = estados().find(codigo);
    return it != estados().end() && it->second.releaseEdge;
}

int InputScripts::codigoDe(const std::string& tecla) {
    // Tabla de nombres GLFW (sin el prefijo GLFW_KEY_). Cubre letras, digitos,
    // flechas, modificadoras y teclas comunes de gameplay; cualquier otra
    // cadena devuelve -1 (la consulta simplemente retorna false).
    static const std::unordered_map<std::string, int> nombres = {
        {"A", 65}, {"B", 66}, {"C", 67}, {"D", 68}, {"E", 69}, {"F", 70},
        {"G", 71}, {"H", 72}, {"I", 73}, {"J", 74}, {"K", 75}, {"L", 76},
        {"M", 77}, {"N", 78}, {"O", 79}, {"P", 80}, {"Q", 81}, {"R", 82},
        {"S", 83}, {"T", 84}, {"U", 85}, {"V", 86}, {"W", 87}, {"X", 88},
        {"Y", 89}, {"Z", 90},
        {"SPACE", 32},
        {"APOSTROPHE", 39}, {"COMMA", 44}, {"MINUS", 45}, {"PERIOD", 46},
        {"SLASH", 47}, {"SEMICOLON", 59}, {"EQUAL", 61},
        {"LEFT_BRACKET", 91}, {"BACKSLASH", 92}, {"RIGHT_BRACKET", 93},
        {"GRAVE_ACCENT", 96},
        {"WORLD_1", 161}, {"WORLD_2", 162},
        {"ESCAPE", 256}, {"ENTER", 257}, {"TAB", 258}, {"BACKSPACE", 259},
        {"INSERT", 260}, {"DELETE", 261},
        {"RIGHT", 262}, {"LEFT", 263}, {"DOWN", 264}, {"UP", 265},
        {"PAGE_UP", 266}, {"PAGE_DOWN", 267}, {"HOME", 268}, {"END", 269},
        {"CAPS_LOCK", 280},
        {"SCROLL_LOCK", 281}, {"NUM_LOCK", 282}, {"PRINT_SCREEN", 283},
        {"PAUSE", 284},
        {"F1", 290}, {"F2", 291}, {"F3", 292}, {"F4", 293}, {"F5", 294},
        {"F6", 295}, {"F7", 296}, {"F8", 297}, {"F9", 298}, {"F10", 299},
        {"F11", 300}, {"F12", 301}, {"F13", 302}, {"F14", 303}, {"F15", 304},
        {"F16", 305}, {"F17", 306}, {"F18", 307}, {"F19", 308}, {"F20", 309},
        {"F21", 310}, {"F22", 311}, {"F23", 312}, {"F24", 313}, {"F25", 314},
        {"KP_0", 320}, {"KP_1", 321}, {"KP_2", 322}, {"KP_3", 323},
        {"KP_4", 324}, {"KP_5", 325}, {"KP_6", 326}, {"KP_7", 327},
        {"KP_8", 328}, {"KP_9", 329},
        {"KP_DECIMAL", 330}, {"KP_DIVIDE", 331}, {"KP_MULTIPLY", 332},
        {"KP_SUBTRACT", 333}, {"KP_ADD", 334}, {"KP_ENTER", 335},
        {"KP_EQUAL", 336},
        {"LEFT_SHIFT", 340}, {"LEFT_CONTROL", 341}, {"LEFT_ALT", 342},
        {"LEFT_SUPER", 343},
        {"RIGHT_SHIFT", 344}, {"RIGHT_CONTROL", 345}, {"RIGHT_ALT", 346},
        {"RIGHT_SUPER", 347}, {"MENU", 348},
        {"D0", 48}, {"D1", 49}, {"D2", 50}, {"D3", 51}, {"D4", 52},
        {"D5", 53}, {"D6", 54}, {"D7", 55}, {"D8", 56}, {"D9", 57},
    };
    const auto it = nombres.find(tecla);
    return it != nombres.end() ? it->second : -1;
}

void InputScripts::reset() { estados().clear(); }
