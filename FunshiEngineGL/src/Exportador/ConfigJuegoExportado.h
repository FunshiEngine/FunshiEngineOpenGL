#ifndef CONFIGJUEGOEXPORTADO_H
#define CONFIGJUEGOEXPORTADO_H

// Manifiesto de arranque de una exportacion: JuegoExportado.json junto al
// binario del juego fija el proyecto y el modo juego, para que el doble clic
// arranque la partida sin argumentos de linea de comandos. Header-only puro:
// leer/escribir un JSON no toca mas disco que el archivo indicado.

#include <fstream>
#include <string>
#include <nlohmann/json.hpp>

namespace ConfigJuegoExportado {

// Ruta del manifiesto junto al ejecutable (dirEjecutable con o sin
// separador final).
inline std::string rutaPorDefecto(const std::string& dirEjecutable) {
    std::string dir = dirEjecutable;
    if (!dir.empty() && dir.back() != '/' && dir.back() != '\\') dir += '/';
    return dir + "JuegoExportado.json";
}

// false si el archivo no existe, no es JSON valido o no trae un proyecto.
// modoJuego ausente se asume true (la razon de ser del manifiesto es arrancar
// en juego).
inline bool leer(const std::string& ruta, std::string& proyecto,
                 bool& modoJuego) {
    std::ifstream entrada(ruta);
    if (!entrada.is_open()) return false;
    try {
        const nlohmann::json j = nlohmann::json::parse(entrada, nullptr, false);
        if (j.is_discarded() || !j.is_object()) return false;
        if (!j.contains("proyecto") || !j["proyecto"].is_string()) return false;
        proyecto = j["proyecto"].get<std::string>();
        if (proyecto.empty()) return false;
        if (!j.contains("modoJuego") || !j["modoJuego"].is_boolean())
            modoJuego = true;
        else
            modoJuego = j["modoJuego"].get<bool>();
        return true;
    } catch (...) {
        return false;
    }
}

inline bool escribir(const std::string& ruta, const std::string& proyecto,
                     bool modoJuego) {
    if (proyecto.empty()) return false;
    std::ofstream salida(ruta, std::ios::trunc);
    if (!salida.is_open()) return false;
    const nlohmann::json j = {{"proyecto", proyecto}, {"modoJuego", modoJuego}};
    salida << j.dump(2);
    return salida.good();
}

} // namespace ConfigJuegoExportado

#endif // CONFIGJUEGOEXPORTADO_H