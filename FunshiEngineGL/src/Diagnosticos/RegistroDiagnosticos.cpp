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
#include "../Configuracion/ProjectPaths.h"
#include "RegistroDiagnosticos.h"
#include "../ExcepcionesCPP/RuntimeException.h"
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <typeinfo>
#include <utility>

namespace {
using json = nlohmann::json;

std::string fechaUtcAhora() {
    using namespace std::chrono;
    const system_clock::time_point ahora = system_clock::now();
    const auto ms = duration_cast<milliseconds>(ahora.time_since_epoch()) % 1000;
    const std::time_t tiempo = system_clock::to_time_t(ahora);
    std::tm utcValor{};
#ifdef _WIN32
    if (gmtime_s(&utcValor, &tiempo) != 0) return {};
#else
    if (!gmtime_r(&tiempo, &utcValor)) return {};
#endif
    char buffer[32] = {};
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &utcValor);
    char resultado[40] = {};
    std::snprintf(resultado, sizeof(resultado), "%s.%03lldZ", buffer,
                  static_cast<long long>(ms.count()));
    return resultado;
}

json serializar(const EventoDiagnostico& evento) {
    return json{{"timestamp", evento.fechaUtc},
                {"level", RegistroDiagnosticos::nombreNivel(evento.nivel)},
                {"module", evento.modulo},
                {"message", evento.mensaje},
                {"project", evento.proyecto},
                {"exception", evento.tipoExcepcion},
                {"stack", evento.traza}};
}

EventoDiagnostico deserializar(const json& valor) {
    EventoDiagnostico evento;
    evento.fechaUtc = valor.value("timestamp", std::string{});
    const std::string nivel = valor.value("level", std::string("info"));
    if (nivel == "debug") evento.nivel = NivelDiagnostico::Depuracion;
    else if (nivel == "warning") evento.nivel = NivelDiagnostico::Advertencia;
    else if (nivel == "error") evento.nivel = NivelDiagnostico::Error;
    else if (nivel == "critical") evento.nivel = NivelDiagnostico::Critico;
    else evento.nivel = NivelDiagnostico::Informacion;
    evento.modulo = valor.value("module", std::string{});
    evento.mensaje = valor.value("message", std::string{});
    evento.proyecto = valor.value("project", std::string{});
    evento.tipoExcepcion = valor.value("exception", std::string{});
    evento.traza = valor.value("stack", std::string{});
    return evento;
}
}  // namespace

RegistroDiagnosticos::RegistroDiagnosticos(std::filesystem::path archivo,
                                           std::size_t maximoEnMemoria)
    : archivo_(std::move(archivo)),
      maximoEnMemoria_(std::max<std::size_t>(1, maximoEnMemoria)),
      eventos_(leerArchivo(archivo_, maximoEnMemoria_)) {}

bool RegistroDiagnosticos::registrar(EventoDiagnostico evento) {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    return guardarBajoBloqueo(std::move(evento));
}

bool RegistroDiagnosticos::registrarUnaVez(
    EventoDiagnostico evento, const std::string& clave,
    std::chrono::milliseconds intervaloMinimo) {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    const auto ahora = std::chrono::steady_clock::now();
    const auto previo = ultimoPorClave_.find(clave);
    if (previo != ultimoPorClave_.end() &&
        ahora - previo->second < intervaloMinimo) {
        return true;
    }
    ultimoPorClave_[clave] = ahora;
    return guardarBajoBloqueo(std::move(evento));
}

bool RegistroDiagnosticos::guardarBajoBloqueo(EventoDiagnostico evento) {
    if (evento.fechaUtc.empty()) evento.fechaUtc = fechaUtcAhora();

    eventos_.push_back(evento);
    if (eventos_.size() > maximoEnMemoria_) {
        eventos_.erase(eventos_.begin(),
                       eventos_.begin() +
                           static_cast<std::ptrdiff_t>(eventos_.size() -
                                                       maximoEnMemoria_));
    }

    std::error_code error;
    if (!archivo_.parent_path().empty()) {
        std::filesystem::create_directories(archivo_.parent_path(), error);
        if (error) return false;
    }
    std::ofstream salida(archivo_, std::ios::out | std::ios::app | std::ios::binary);
    if (!salida) return false;
    const std::string linea =
        serializar(evento).dump(-1, ' ', false, json::error_handler_t::replace) +
        "\n";
    salida.write(linea.data(), static_cast<std::streamsize>(linea.size()));
    salida.flush();
    return static_cast<bool>(salida);
}

std::vector<EventoDiagnostico> RegistroDiagnosticos::ultimos(
    std::size_t limite) const {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    const std::size_t inicio = eventos_.size() > limite
                                   ? eventos_.size() - limite
                                   : 0;
    return std::vector<EventoDiagnostico>(eventos_.begin() +
                                              static_cast<std::ptrdiff_t>(inicio),
                                          eventos_.end());
}

std::vector<EventoDiagnostico> RegistroDiagnosticos::todos() const {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    return eventos_;
}

bool RegistroDiagnosticos::limpiarTodo() {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    eventos_.clear();

    std::error_code error;
    if (!archivo_.parent_path().empty()) {
        std::filesystem::create_directories(archivo_.parent_path(), error);
        if (error) return false;
    }
    std::ofstream salida(archivo_, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!salida) return false;
    salida.flush();
    return static_cast<bool>(salida);
}

bool RegistroDiagnosticos::insertarAntes(std::vector<EventoDiagnostico> eventos) {
    std::lock_guard<std::mutex> bloqueo(mutex_);
    eventos.insert(eventos.end(), eventos_.begin(), eventos_.end());
    eventos_ = std::move(eventos);
    if (eventos_.size() > maximoEnMemoria_) {
        eventos_.erase(eventos_.begin(),
                       eventos_.begin() +
                           static_cast<std::ptrdiff_t>(eventos_.size() -
                                                       maximoEnMemoria_));
    }

    std::error_code error;
    if (!archivo_.parent_path().empty()) {
        std::filesystem::create_directories(archivo_.parent_path(), error);
        if (error) return false;
    }
    std::ofstream salida(archivo_, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!salida) return false;
    for (const auto& e : eventos_) {
        const std::string linea =
            serializar(e).dump(-1, ' ', false, json::error_handler_t::replace) +
            "\n";
        salida.write(linea.data(), static_cast<std::streamsize>(linea.size()));
    }
    salida.flush();
    return static_cast<bool>(salida);
}

std::filesystem::path RegistroDiagnosticos::archivo() const {
    return archivo_;
}

RegistroDiagnosticos& RegistroDiagnosticos::global() {
    static RegistroDiagnosticos registro(archivoPorDefecto());
    return registro;
}

std::filesystem::path RegistroDiagnosticos::archivoPorDefecto() {
    return std::filesystem::path(ProjectPaths::directorioBase()) / "logs" /
           "FunshiEngineGL_diagnosticos.jsonl";
}

std::vector<EventoDiagnostico> RegistroDiagnosticos::leerArchivo(
    const std::filesystem::path& archivo, std::size_t limite) {
    std::ifstream entrada(archivo, std::ios::in | std::ios::binary);
    if (!entrada) return {};

    std::vector<EventoDiagnostico> eventos;
    std::string linea;
    while (std::getline(entrada, linea)) {
        if (linea.empty()) continue;
        try {
            eventos.push_back(deserializar(json::parse(linea)));
            if (eventos.size() > limite) eventos.erase(eventos.begin());
        } catch (const json::exception&) {
            // Una línea truncada por cierre abrupto no invalida el historial.
        }
    }
    return eventos;
}

std::string RegistroDiagnosticos::nombreNivel(NivelDiagnostico nivel) {
    switch (nivel) {
        case NivelDiagnostico::Depuracion: return "debug";
        case NivelDiagnostico::Informacion: return "info";
        case NivelDiagnostico::Advertencia: return "warning";
        case NivelDiagnostico::Error: return "error";
        case NivelDiagnostico::Critico: return "critical";
    }
    return "error";
}

void registrarExcepcionDiagnostica(RegistroDiagnosticos& registro,
                                   const std::string& modulo,
                                   const std::exception& excepcion,
                                   NivelDiagnostico nivel,
                                   const std::string& clave) noexcept {
    try {
        EventoDiagnostico evento;
        evento.nivel = nivel;
        evento.modulo = modulo;
        evento.mensaje = excepcion.what();
        evento.tipoExcepcion = typeid(excepcion).name();
        if (const auto* runtime =
                dynamic_cast<const RuntimeException*>(&excepcion)) {
            evento.tipoExcepcion = runtime->getTipoExcepcion();
            evento.traza = runtime->getStackTrace();
        }
        const std::string llave =
            clave.empty() ? "ExcepcionesCPP:" + evento.mensaje : clave;
        registro.registrarUnaVez(std::move(evento), llave);
    } catch (...) {
    }
}

void registrarExcepcionDiagnostica(const std::string& modulo,
                                   const std::exception& excepcion,
                                   NivelDiagnostico nivel,
                                   const std::string& clave) noexcept {
    registrarExcepcionDiagnostica(RegistroDiagnosticos::global(), modulo,
                                  excepcion, nivel, clave);
}

void registrarFalloDiagnostico(RegistroDiagnosticos& registro,
                               const std::string& modulo,
                               const std::string& codigo,
                               const std::string& mensaje,
                               NivelDiagnostico nivel) noexcept {
    try {
        EventoDiagnostico evento;
        evento.nivel = nivel;
        evento.modulo = modulo;
        evento.mensaje = mensaje;
        evento.tipoExcepcion = codigo;
        registro.registrarUnaVez(std::move(evento),
                                 modulo + ":" + codigo + ":" + mensaje);
    } catch (...) {
    }
}

void registrarFalloDiagnostico(const std::string& modulo,
                               const std::string& codigo,
                               const std::string& mensaje,
                               NivelDiagnostico nivel) noexcept {
    registrarFalloDiagnostico(RegistroDiagnosticos::global(), modulo, codigo,
                              mensaje, nivel);
}
