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
#include "GestorMods.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <utility>

#include "../Formato/LectorZip.h"

#ifndef FUNSHI_VERSION_STR
// Fuera del build de CMake (que la define) no hay version del motor con la que
// contrastar: se deja sin restriccion en vez de rechazar todo.
#define FUNSHI_VERSION_STR "0.0.0"
#endif

namespace {

constexpr const char* MODULO_MODS = "Mods";

bool extensionEsFmod(const std::filesystem::path& ruta) {
    std::string extension = ruta.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return extension == ".fmod";
}

// Version compilada del motor (macro), ya parseada. Devuelve false si la macro
// no tiene formato semver.
bool versionDelMotor(ModVersion& salida) {
    return ModVersion::parsear(FUNSHI_VERSION_STR, salida);
}

// Un candidato a cargarse mientras se valida; `vivo` en false es un rechazado.
struct Candidato {
    PaqueteMod paquete;
    std::string causa;
    bool vivo = true;
};

} // namespace

GestorMods::Resultado GestorMods::descubrir(
    const std::vector<CarpetaMods>& carpetas) {
    Resultado resultado;

    // 1. Descubrimiento: .fmod de primer nivel en cada carpeta (las
    //    inexistentes se ignoran). Se ordena por ruta para que el resultado no
    //    dependa del orden del directorio.
    struct Encontrado {
        std::filesystem::path ruta;
        AmbitoMod ambito;
    };
    std::vector<Encontrado> encontrados;
    for (const CarpetaMods& carpeta : carpetas) {
        std::error_code ec;
        const std::filesystem::path base(carpeta.ruta);
        if (!std::filesystem::is_directory(base, ec))
            continue;
        for (const auto& entrada :
             std::filesystem::directory_iterator(base, ec)) {
            if (!entrada.is_regular_file(ec))
                continue;
            if (!extensionEsFmod(entrada.path()))
                continue;
            encontrados.push_back({entrada.path(), carpeta.ambito});
        }
    }
    std::sort(encontrados.begin(), encontrados.end(),
              [](const Encontrado& a, const Encontrado& b) {
                  return a.ruta.string() < b.ruta.string();
              });

    // 2. Lectura del manifiesto de cada paquete.
    std::vector<Candidato> candidatos;
    for (const Encontrado& encontrado : encontrados) {
        const std::string nombrePaquete = encontrado.ruta.filename().string();
        std::string contenido;
        std::string error;
        if (!LectorZip::leerEntrada(encontrado.ruta.string(),
                                    "ManifiestoMod.json", contenido, error)) {
            resultado.fallos.push_back(
                {nombrePaquete, "no se pudo leer el manifiesto: " + error});
            continue;
        }
        Candidato candidato;
        if (!ManifiestoMod::parsear(contenido, candidato.paquete.manifiesto,
                                    error)) {
            resultado.fallos.push_back(
                {nombrePaquete, "manifiesto invalido: " + error});
            continue;
        }
        candidato.paquete.paquete = encontrado.ruta;
        candidato.paquete.ambito = encontrado.ambito;
        candidatos.push_back(std::move(candidato));
    }

    // 3. Id duplicado: se rechazan TODAS las instancias con ese id.
    std::map<std::string, int> apariciones;
    for (const Candidato& candidato : candidatos)
        ++apariciones[candidato.paquete.manifiesto.id];
    for (Candidato& candidato : candidatos) {
        if (apariciones[candidato.paquete.manifiesto.id] <= 1)
            continue;
        candidato.vivo = false;
        candidato.causa = "id duplicado '" + candidato.paquete.manifiesto.id +
                          "': el id de un mod debe ser unico";
    }

    // 4. Guarda contra la version del motor (rango versionMotor).
    ModVersion motor;
    const bool hayVersionMotor = versionDelMotor(motor);
    for (Candidato& candidato : candidatos) {
        if (!candidato.vivo || candidato.paquete.manifiesto.versionMotor.empty())
            continue;
        RangoVersion rango;
        if (!RangoVersion::parsear(candidato.paquete.manifiesto.versionMotor,
                                   rango))
            continue; // el manifiesto ya se valido al parsear
        if (!hayVersionMotor || !rango.contiene(motor)) {
            candidato.vivo = false;
            candidato.causa =
                "requiere el motor '" + candidato.paquete.manifiesto.versionMotor +
                "' y esta build es " + std::string(FUNSHI_VERSION_STR);
        }
    }

    // Indice de los vivos por id, para resolver dependencias y conflictos.
    auto reindexar = [&candidatos]() {
        std::map<std::string, std::size_t> indice;
        for (std::size_t i = 0; i < candidatos.size(); ++i)
            if (candidatos[i].vivo)
                indice[candidatos[i].paquete.manifiesto.id] = i;
        return indice;
    };

    // 5. Conflictos declarados contra un mod presente.
    {
        const std::map<std::string, std::size_t> indice = reindexar();
        for (Candidato& candidato : candidatos) {
            if (!candidato.vivo)
                continue;
            for (const std::string& conflicto :
                 candidato.paquete.manifiesto.conflictos) {
                if (indice.count(conflicto) == 0)
                    continue;
                candidato.vivo = false;
                candidato.causa = "declara un conflicto con '" + conflicto +
                                  "', que esta presente";
                break;
            }
        }
    }

    // 6. Dependencias: faltantes, versionMin insatisfecha y propagacion del
    //    rechazo a quien dependa de un mod rechazado (punto fijo).
    bool cambio = true;
    while (cambio) {
        cambio = false;
        const std::map<std::string, std::size_t> indice = reindexar();
        for (Candidato& candidato : candidatos) {
            if (!candidato.vivo)
                continue;
            for (const DependenciaMod& dependencia :
                 candidato.paquete.manifiesto.dependencias) {
                const auto ubicacion = indice.find(dependencia.id);
                if (ubicacion == indice.end()) {
                    if (!dependencia.requerida)
                        continue; // dependencia opcional ausente: no bloquea
                    candidato.vivo = false;
                    candidato.causa = "depende de '" + dependencia.id +
                                      "', que no esta disponible";
                    cambio = true;
                    break;
                }
                if (dependencia.versionMin.empty())
                    continue;
                ModVersion minima;
                ModVersion presente;
                if (!ModVersion::parsear(dependencia.versionMin, minima) ||
                    !ModVersion::parsear(
                        candidatos[ubicacion->second].paquete.manifiesto.version,
                        presente))
                    continue;
                if (presente.satisfaceMinimo(minima))
                    continue;
                if (!dependencia.requerida)
                    continue; // opcional por debajo del minimo: se ignora
                candidato.vivo = false;
                candidato.causa = "exige '" + dependencia.id + "' >= " +
                                  dependencia.versionMin +
                                  " y la version presente es " +
                                  presente.aTexto();
                cambio = true;
                break;
            }
        }
    }

    // 7. Orden topologico (deterministico, empate por id). Los que no se pueden
    //    ordenar (ciclos, o dependen de un ciclo) se rechazan.
    const std::map<std::string, std::size_t> indice = reindexar();
    std::map<std::string, std::set<std::string>> aristas; // origen -> destinos
    std::map<std::string, int> grados;
    for (const auto& par : indice) {
        aristas[par.first];
        grados[par.first] = 0;
    }
    for (const auto& par : indice) {
        const std::size_t i = par.second;
        for (const DependenciaMod& dependencia :
             candidatos[i].paquete.manifiesto.dependencias) {
            if (indice.count(dependencia.id) == 0)
                continue; // ausente opcional: sin arista
            const std::string& mod = par.first;
            const std::string& dep = dependencia.id;
            bool aristaDepPrimero = false;
            bool aristaModPrimero = false;
            switch (dependencia.orden) {
            case DependenciaMod::Orden::Antes:
                aristaDepPrimero = true;
                break;
            case DependenciaMod::Orden::Despues:
                aristaModPrimero = true;
                break;
            case DependenciaMod::Orden::Ninguno:
                aristaDepPrimero = dependencia.requerida;
                break;
            }
            if (aristaDepPrimero && aristas[dep].insert(mod).second)
                ++grados[mod];
            if (aristaModPrimero && aristas[mod].insert(dep).second)
                ++grados[dep];
        }
    }

    std::set<std::string> disponibles;
    for (const auto& par : grados)
        if (par.second == 0)
            disponibles.insert(par.first);

    std::vector<std::string> orden;
    while (!disponibles.empty()) {
        const std::string actual = *disponibles.begin();
        disponibles.erase(disponibles.begin());
        orden.push_back(actual);
        for (const std::string& destino : aristas[actual]) {
            if (--grados[destino] == 0)
                disponibles.insert(destino);
        }
    }

    for (Candidato& candidato : candidatos) {
        if (!candidato.vivo)
            continue;
        if (std::find(orden.begin(), orden.end(),
                      candidato.paquete.manifiesto.id) == orden.end()) {
            candidato.vivo = false;
            candidato.causa =
                "ciclo de dependencias: no se puede resolver su orden de carga";
        }
    }

    // 8. Resultado: mods en orden de carga y rechazados con su causa.
    for (const std::string& id : orden) {
        const std::size_t i = indice.at(id);
        if (candidatos[i].vivo)
            resultado.mods.push_back(candidatos[i].paquete);
    }
    for (const Candidato& candidato : candidatos) {
        if (!candidato.vivo) {
            // La causa lleva el id del mod (cuando se pudo leer) ademas del
            // nombre del paquete, para que el reporte sea inequivoco.
            const std::string id = candidato.paquete.manifiesto.id;
            const std::string causa =
                id.empty() ? candidato.causa : id + ": " + candidato.causa;
            resultado.fallos.push_back(
                {candidato.paquete.paquete.filename().string(), causa});
        }
    }
    return resultado;
}

void GestorMods::reportar(const Resultado& resultado,
                          RegistroDiagnosticos& registro) {
    // Un evento por mod rechazado, con su nombre y la causa tecnica.
    for (const FalloMod& fallo : resultado.fallos) {
        EventoDiagnostico evento;
        evento.nivel = NivelDiagnostico::Error;
        evento.modulo = MODULO_MODS;
        evento.mensaje = "mod rechazado: " + fallo.paquete + " (" +
                         fallo.causa + ")";
        registro.registrar(std::move(evento));
    }

    // Resumen por ambito (motor / proyecto), visible en el panel de estado/log.
    // El recuento es de paquetes ENCONTRADOS: que un paquete se haya
    // encontrado no significa que su binario se haya podido cargar (eso lo
    // reporta el cargador, con su propio resumen).
    std::size_t motorCount = 0;
    std::size_t proyectoCount = 0;
    for (const PaqueteMod& mod : resultado.mods) {
        if (mod.ambito == AmbitoMod::Motor)
            ++motorCount;
        else
            ++proyectoCount;
    }

    if (motorCount > 0) {
        EventoDiagnostico resumenMotor;
        resumenMotor.nivel = NivelDiagnostico::Informacion;
        resumenMotor.modulo = MODULO_MODS;
        resumenMotor.mensaje = "mods de motor encontrados: " + std::to_string(motorCount);
        registro.registrar(std::move(resumenMotor));
    }
    if (proyectoCount > 0) {
        EventoDiagnostico resumenProyecto;
        resumenProyecto.nivel = NivelDiagnostico::Informacion;
        resumenProyecto.modulo = MODULO_MODS;
        resumenProyecto.mensaje = "mods de proyecto encontrados: " + std::to_string(proyectoCount);
        registro.registrar(std::move(resumenProyecto));
    }
    if (motorCount == 0 && proyectoCount == 0) {
        EventoDiagnostico resumen;
        resumen.nivel = resultado.fallos.empty() ? NivelDiagnostico::Informacion
                                                 : NivelDiagnostico::Advertencia;
        resumen.modulo = MODULO_MODS;
        resumen.mensaje = "mods encontrados: 0";
        registro.registrar(std::move(resumen));
    }
}

void GestorMods::reportar(const Resultado& resultado) {
    reportar(resultado, RegistroDiagnosticos::global());
}
