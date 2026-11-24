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
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Diagnosticos/RegistroDiagnosticos.h"
#include "../FunshiEngineGL/src/ExcepcionesCPP/RuntimeException.h"

namespace fs = std::filesystem;

namespace {
int total = 0;
int fallos = 0;

#define CHECK(condicion, mensaje)                                             \
    do {                                                                      \
        ++total;                                                              \
        if (!(condicion)) {                                                   \
            ++fallos;                                                         \
            std::cout << "FALLO: " << mensaje << " (linea " << __LINE__     \
                      << ")" << std::endl;                                   \
        }                                                                     \
    } while (false)

EventoDiagnostico crearEvento(const std::string& mensaje,
                              NivelDiagnostico nivel =
                                  NivelDiagnostico::Informacion) {
    EventoDiagnostico evento;
    evento.nivel = nivel;
    evento.modulo = "Pruebas/Registro";
    evento.mensaje = mensaje;
    evento.proyecto = "ProyectoPrueba";
    return evento;
}
}  // namespace

int main() {
    TempPruebas::CarpetaPrueba carpeta("funshi_diagnosticos_tests");
    const fs::path archivo = carpeta.ruta() / "diagnosticos.jsonl";

    {
        RegistroDiagnosticos registro(archivo, 3);
        CHECK(registro.ultimos().empty(),
              "el registro nuevo empieza sin eventos");

        const std::vector<NivelDiagnostico> niveles = {
            NivelDiagnostico::Depuracion, NivelDiagnostico::Informacion,
            NivelDiagnostico::Advertencia, NivelDiagnostico::Error,
            NivelDiagnostico::Critico};
        const std::vector<std::string> nombres = {
            "debug", "info", "warning", "error", "critical"};
        for (std::size_t i = 0; i < niveles.size(); ++i) {
            CHECK(RegistroDiagnosticos::nombreNivel(niveles[i]) == nombres[i],
                  "el nivel tiene una etiqueta estable");
        }

        CHECK(registro.registrar(crearEvento("primero",
                                               NivelDiagnostico::Advertencia)),
              "el evento se persiste");
        CHECK(registro.registrar(crearEvento("segundo",
                                               NivelDiagnostico::Error)),
              "el segundo evento se persiste");
        const auto enMemoria = registro.ultimos(10);
        CHECK(enMemoria.size() == 2, "ultimos devuelve los eventos disponibles");
        CHECK(enMemoria[0].modulo == "Pruebas/Registro" &&
                  enMemoria[0].proyecto == "ProyectoPrueba" &&
                  enMemoria[0].mensaje == "primero" &&
                  enMemoria[0].nivel == NivelDiagnostico::Advertencia,
              "el evento conserva módulo, proyecto, mensaje y nivel");
        CHECK(!enMemoria[0].fechaUtc.empty(), "el evento recibe timestamp UTC");
        CHECK(registro.ultimos(0).empty(), "un límite cero no devuelve eventos");
    }

    const auto persistidos = RegistroDiagnosticos::leerArchivo(archivo, 10);
    CHECK(persistidos.size() == 2, "el historial JSONL se vuelve a leer");
    CHECK(persistidos.size() == 2 &&
              persistidos[0].nivel == NivelDiagnostico::Advertencia &&
              persistidos[0].modulo == "Pruebas/Registro" &&
              persistidos[0].proyecto == "ProyectoPrueba" &&
              persistidos[0].mensaje == "primero",
          "JSONL conserva los campos estructurados");
    CHECK(RegistroDiagnosticos::leerArchivo(archivo, 1).size() == 1 &&
              RegistroDiagnosticos::leerArchivo(archivo, 1)[0].mensaje ==
                  "segundo",
          "la lectura limita el historial desde el evento más reciente");

    const fs::path archivoRetencion = carpeta.ruta() / "retencion.jsonl";
    {
        RegistroDiagnosticos registro(archivoRetencion, 2);
        CHECK(registro.registrar(crearEvento("uno")),
              "se persiste el primer evento de retención");
        CHECK(registro.registrar(crearEvento("dos")),
              "se persiste el segundo evento de retención");
        CHECK(registro.registrar(crearEvento("tres")),
              "se persiste el tercer evento de retención");
        const auto recientes = registro.ultimos(10);
        CHECK(recientes.size() == 2 && recientes[0].mensaje == "dos" &&
                  recientes[1].mensaje == "tres",
              "la memoria conserva solo los eventos más recientes");
    }
    CHECK(RegistroDiagnosticos::leerArchivo(archivoRetencion, 10).size() == 3,
          "la retención en memoria no borra el historial persistido");

    const fs::path archivoPublicadores = carpeta.ruta() / "publicadores.jsonl";
    RegistroDiagnosticos registro(archivoPublicadores, 20);
    RuntimeException excepcion("fallo de carga", "AssetLoadException");
    registrarExcepcionDiagnostica(registro, "Assets/Modelo", excepcion,
                                  NivelDiagnostico::Error, "malla: nave");
    registrarExcepcionDiagnostica(registro, "Assets/Modelo", excepcion,
                                  NivelDiagnostico::Error, "malla: nave");
    auto eventos = registro.ultimos(10);
    CHECK(eventos.size() == 1, "la misma excepción se deduplica por clave");
    CHECK(eventos.size() == 1 && eventos[0].modulo == "Assets/Modelo" &&
              eventos[0].tipoExcepcion == "AssetLoadException" &&
              eventos[0].mensaje == "fallo de carga" &&
              eventos[0].nivel == NivelDiagnostico::Error,
          "el publicador conserva contexto y tipo de RuntimeException");

    registrarExcepcionDiagnostica(registro, "Scripts/Compilacion",
                                  std::runtime_error("compilador no disponible"),
                                  NivelDiagnostico::Advertencia, "script:1");
    registrarFalloDiagnostico(registro, "Scenes/Load", "SCENE_CORRUPT",
                              "La escena está truncada",
                              NivelDiagnostico::Error);
    registrarFalloDiagnostico(registro, "Scenes/Load", "SCENE_CORRUPT",
                              "La escena está truncada",
                              NivelDiagnostico::Error);
    eventos = registro.ultimos(10);
    CHECK(eventos.size() == 3,
          "los publicadores agregan cada fallo distinto una sola vez");
    CHECK(eventos.size() == 3 && eventos[1].modulo == "Scripts/Compilacion" &&
              eventos[1].nivel == NivelDiagnostico::Advertencia &&
              !eventos[1].tipoExcepcion.empty(),
          "una excepción estándar conserva módulo y nivel");
    CHECK(eventos.size() == 3 && eventos[2].modulo == "Scenes/Load" &&
              eventos[2].tipoExcepcion == "SCENE_CORRUPT" &&
              eventos[2].mensaje == "La escena está truncada",
          "un fallo sin excepción conserva el código y el mensaje");
    CHECK(RegistroDiagnosticos::leerArchivo(archivoPublicadores, 10).size() == 3,
          "los publicadores escriben sus eventos en JSONL");

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
