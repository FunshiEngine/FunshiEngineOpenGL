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
// Preambulo compartido por las unidades de ModsTests.cpp (generado al dividirlo).
#ifndef MODSTESTSDETALLE_H
#define MODSTESTSDETALLE_H

// Pruebas headless del nucleo del sistema de mods: versiones semver minimas
// (ModVersion y RangoVersion), el esquema del manifiesto de un paquete .fmod
// (ManifiestoMod.json) y la lectura del paquete ZIP (LectorZip: manifiesto
// desde el .fmod, extraccion a cache y contrato anti zip-slip). Cubre parseo de
// campos obligatorios y opcionales, validacion del id, tolerancia a campos
// ausentes, cortafuegos de esquemaVersion, dependencias/conflictos y paquetes
// generados en el momento con miniz.

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>
#include <miniz/miniz.h>
#include "TempPruebas.h"
#include "RutasPluginsMods.h"
#include "../FunshiEngineGL/src/Configuracion/ProjectPaths.h"
#include "../FunshiEngineGL/src/Diagnosticos/RegistroDiagnosticos.h"
#include "../FunshiEngineGL/src/Mods/Carga/CargadorMods.h"
#include "../FunshiEngineGL/src/Mods/Carga/GestorMods.h"
#include "../FunshiEngineGL/src/Mods/Formato/LectorZip.h"
#include "../FunshiEngineGL/src/Mods/Formato/ManifiestoMod.h"
#include "../FunshiEngineGL/src/Mods/Formato/ModVersion.h"
#include "../FunshiEngineGL/src/Mods/Api/RegistroPanelesMods.h"
#include "../FunshiEngineGL/src/Mods/Api/ServiciosMods.h"

namespace fs = std::filesystem;

namespace ModsTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ModsTestsDetalle
using namespace ModsTestsDetalle;

namespace {

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

bool contiene(const std::string& texto, const std::string& aguja) {
    return texto.find(aguja) != std::string::npos;
}

// Arma un ZIP en `ruta` con las entradas (nombre, contenido) recibidas, usando
// el escritor de miniz. Es el mismo camino con el que un empaquetador de mods
// generaria un .fmod, y permite a los tests fabricar paquetes validos y
// maliciosos (entradas con "..", absolutas) sin depender de binarios externos.
bool crearZip(const std::string& ruta,
              const std::vector<std::pair<std::string, std::string>>& entradas) {
    mz_zip_archive zip;
    mz_zip_zero_struct(&zip);
    if (!mz_zip_writer_init_file(&zip, ruta.c_str(), 0))
        return false;
    for (const auto& entrada : entradas) {
        if (!mz_zip_writer_add_mem(&zip, entrada.first.c_str(),
                                   entrada.second.data(), entrada.second.size(),
                                   MZ_BEST_COMPRESSION)) {
            mz_zip_writer_end(&zip);
            return false;
        }
    }
    const bool ok = mz_zip_writer_finalize_archive(&zip) != MZ_FALSE;
    mz_zip_writer_end(&zip);
    return ok;
}

std::string leerArchivoTexto(const std::string& ruta) {
    std::ifstream archivo(ruta, std::ios::binary);
    if (!archivo)
        return std::string();
    return std::string(std::istreambuf_iterator<char>(archivo),
                       std::istreambuf_iterator<char>());
}

// Manifiesto minimo valido, con los campos obligatorios de la v1. Los tests
// alteran un campo por caso para aislar la validacion que ejercitan.
std::string manifiestoBase() {
    return R"json({
        "esquemaVersion": 1,
        "id": "Autor.MiMod",
        "nombre": "Mi Mod",
        "autor": "Autor",
        "version": "1.2.3",
        "licencia": "MIT",
        "tipo": "nativo",
        "versionMotor": ">=0.5.0 <1.0.0",
        "puntoEntrada": "bin/mimod.dll",
        "dependencias": [
            {"id": "Otro.Base", "versionMin": "2.0.0", "requerida": false, "orden": "antes"}
        ],
        "conflictos": ["Tercero.Malo"]
    })json";
}

// Manifiesto con el punto de entrada declarado: el cargador busca el binario
// del mod por esa ruta dentro del paquete.
std::string manifiestoConEntrada(const std::string& id, const std::string& version,
                                 const std::string& puntoEntrada) {
    return std::string("{\"esquemaVersion\":1,\"id\":\"") + id +
           "\",\"nombre\":\"" + id + "\",\"version\":\"" + version +
           "\",\"tipo\":\"nativo\",\"puntoEntrada\":\"" + puntoEntrada + "\"}";
}

// Arma un paquete .fmod con un binario real dentro (el caso del mod nativo:
// el .fmod lleva el .dll/.so que el motor carga).
bool crearPaqueteConBinario(const fs::path& carpeta, const std::string& archivo,
                             const std::string& manifiesto,
                             const std::string& nombreEntrada,
                             const std::string& binario) {
    std::error_code ec;
    fs::create_directories(carpeta, ec);
    return crearZip((carpeta / archivo).string(),
                    {{"ManifiestoMod.json", manifiesto},
                     {nombreEntrada, binario}});
}

// Fija una variable de entorno para el proceso actual: es el canal por el que
// el plugin de prueba recibe la ruta de su archivo de log.
void fijarVariableEntorno(const std::string& nombre, const std::string& valor) {
#if defined(_WIN32)
    _putenv_s(nombre.c_str(), valor.c_str());
#else
    setenv(nombre.c_str(), valor.c_str(), 1);
#endif
}

}  // namespace

namespace ModsTestsDetalle {

extern int gLlamadasAlPanel;

}  // namespace ModsTestsDetalle
using namespace ModsTestsDetalle;

namespace {

void panelDePrueba(const FunshiMods::ContextoMod* contexto, void* datos) {
    (void)contexto;
    (void)datos;
    ++gLlamadasAlPanel;
}

}  // namespace

namespace ModsTestsDetalle {

// Fuente de datos de escena falsa: dos objetos con componentes, para probar
// que la tabla de servicios reparte las consultas al motor sin conocerlo.
struct ObjetoFalso {
    const char* nombre;
    int componentes;
};

}  // namespace ModsTestsDetalle
using namespace ModsTestsDetalle;

namespace {

const ObjetoFalso kObjetosFalsos[] = {{"Cubo", 2}, {"Esfera", 1}};

}  // namespace

// Funciones repartidas entre las unidades.

void seccion01();
void seccion02();
void seccion03();
void seccion04();
void seccion05();
void seccion06();
void seccion07();
void seccion08();
void seccion09();
void seccion10();
void seccion11();
void seccion12();
void seccion13();
void seccion14();
void seccion15();

#endif
