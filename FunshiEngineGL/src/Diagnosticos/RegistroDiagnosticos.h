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
#ifndef REGISTRO_DIAGNOSTICOS_H
#define REGISTRO_DIAGNOSTICOS_H

#include <cstddef>
#include <chrono>
#include <filesystem>
#include <exception>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

enum class NivelDiagnostico {
    Depuracion,
    Informacion,
    Advertencia,
    Error,
    Critico
};

struct EventoDiagnostico {
    std::string fechaUtc;
    NivelDiagnostico nivel = NivelDiagnostico::Informacion;
    std::string modulo;
    std::string mensaje;
    std::string proyecto;
    std::string tipoExcepcion;
    std::string traza;
};

// Almacena diagnósticos estructurados en memoria y en JSON Lines. La GUI y las
// herramientas CLI comparten el archivo bajo la raíz fija de datos del motor.
class RegistroDiagnosticos {
public:
    explicit RegistroDiagnosticos(std::filesystem::path archivo,
                                  std::size_t maximoEnMemoria = 2000);

    bool registrar(EventoDiagnostico evento);
    bool registrarUnaVez(EventoDiagnostico evento, const std::string& clave,
                         std::chrono::milliseconds intervaloMinimo =
                             std::chrono::seconds(3));
    std::vector<EventoDiagnostico> ultimos(std::size_t limite = 100) const;
    std::vector<EventoDiagnostico> todos() const;
    std::filesystem::path archivo() const;
    bool limpiarTodo();
    bool insertarAntes(std::vector<EventoDiagnostico> eventos);

    static RegistroDiagnosticos& global();

    static std::filesystem::path archivoPorDefecto();
    static std::vector<EventoDiagnostico> leerArchivo(
        const std::filesystem::path& archivo,
        std::size_t limite = 100);

    static std::string nombreNivel(NivelDiagnostico nivel);

private:
    std::filesystem::path archivo_;
    std::size_t maximoEnMemoria_;
    mutable std::mutex mutex_;
    std::vector<EventoDiagnostico> eventos_;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point>
        ultimoPorClave_;

    bool guardarBajoBloqueo(EventoDiagnostico evento);
};

void registrarExcepcionDiagnostica(
    RegistroDiagnosticos& registro, const std::string& modulo,
    const std::exception& excepcion,
    NivelDiagnostico nivel = NivelDiagnostico::Error,
    const std::string& clave = std::string()) noexcept;
void registrarExcepcionDiagnostica(
    const std::string& modulo, const std::exception& excepcion,
    NivelDiagnostico nivel = NivelDiagnostico::Error,
    const std::string& clave = std::string()) noexcept;
void registrarFalloDiagnostico(
    RegistroDiagnosticos& registro, const std::string& modulo,
    const std::string& codigo, const std::string& mensaje,
    NivelDiagnostico nivel = NivelDiagnostico::Error) noexcept;
void registrarFalloDiagnostico(
    const std::string& modulo, const std::string& codigo,
    const std::string& mensaje,
    NivelDiagnostico nivel = NivelDiagnostico::Error) noexcept;

#endif  // REGISTRO_DIAGNOSTICOS_H
