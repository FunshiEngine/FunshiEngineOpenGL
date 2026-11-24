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

namespace {

int total = 0;
int fallos = 0;

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

// miniz se niega a escribir una entrada cuyo nombre empiece por '/', asi que
// para fabricar un paquete con una ruta absoluta de verdad hay que escribir el
// ZIP a mano. Se arma el formato minimo valido (una entrada almacenada, sin
// comprimir) con el nombre que se le pase: el paquete tiene que ser legible
// por el motor, y lo que se comprueba es que lo rechace.
void escribirU16(std::vector<unsigned char>& buffer, unsigned int valor) {
    buffer.push_back(static_cast<unsigned char>(valor & 0xFFu));
    buffer.push_back(static_cast<unsigned char>((valor >> 8) & 0xFFu));
}

void escribirU32(std::vector<unsigned char>& buffer, unsigned int valor) {
    escribirU16(buffer, valor & 0xFFFFu);
    escribirU16(buffer, (valor >> 16) & 0xFFFFu);
}

bool crearZipConEntradaNombrada(const std::string& ruta,
                                 const std::string& nombreEntrada,
                                 const std::string& contenido) {
    const unsigned int crc = static_cast<unsigned int>(
        mz_crc32(MZ_CRC32_INIT,
                 reinterpret_cast<const unsigned char*>(contenido.data()),
                 contenido.size()));

    // Cabecera local: la entrada, sin comprimir.
    std::vector<unsigned char> local;
    escribirU32(local, 0x04034b50u);   // firma de cabecera local
    escribirU16(local, 20);            // version minima
    escribirU16(local, 0);             // flags
    escribirU16(local, 0);             // metodo: almacenado
    escribirU16(local, 0);             // hora
    escribirU16(local, 0);             // fecha
    escribirU32(local, crc);
    escribirU32(local, static_cast<unsigned int>(contenido.size()));
    escribirU32(local, static_cast<unsigned int>(contenido.size()));
    escribirU16(local, static_cast<unsigned int>(nombreEntrada.size()));
    escribirU16(local, 0);             // sin campo extra
    local.insert(local.end(), nombreEntrada.begin(), nombreEntrada.end());
    local.insert(local.end(), contenido.begin(), contenido.end());

    // Directorio central: el indice de la entrada.
    const unsigned int offsetCentral = static_cast<unsigned int>(local.size());
    std::vector<unsigned char> central;
    escribirU32(central, 0x02014b50u); // firma del directorio central
    escribirU16(central, 20);          // version creadora
    escribirU16(central, 20);          // version minima
    escribirU16(central, 0);           // flags
    escribirU16(central, 0);           // metodo: almacenado
    escribirU16(central, 0);           // hora
    escribirU16(central, 0);           // fecha
    escribirU32(central, crc);
    escribirU32(central, static_cast<unsigned int>(contenido.size()));
    escribirU32(central, static_cast<unsigned int>(contenido.size()));
    escribirU16(central, static_cast<unsigned int>(nombreEntrada.size()));
    escribirU16(central, 0);           // sin campo extra
    escribirU16(central, 0);           // sin comentario
    escribirU16(central, 0);           // numero de disco
    escribirU16(central, 0);           // atributos internos
    escribirU32(central, 0);           // atributos externos
    escribirU32(central, 0);           // offset de la cabecera local
    central.insert(central.end(), nombreEntrada.begin(), nombreEntrada.end());

    // Fin del directorio central.
    std::vector<unsigned char> fin;
    escribirU32(fin, 0x06054b50u);     // firma del fin del directorio
    escribirU16(fin, 0);               // disco actual
    escribirU16(fin, 0);               // disco del directorio central
    escribirU16(fin, 1);               // entradas en este disco
    escribirU16(fin, 1);               // entradas en total
    escribirU32(fin, static_cast<unsigned int>(central.size()));
    escribirU32(fin, offsetCentral);
    escribirU16(fin, 0);               // sin comentario

    std::ofstream archivo(ruta, std::ios::binary | std::ios::trunc);
    if (!archivo)
        return false;
    archivo.write(reinterpret_cast<const char*>(local.data()),
                  static_cast<std::streamsize>(local.size()));
    archivo.write(reinterpret_cast<const char*>(central.data()),
                  static_cast<std::streamsize>(central.size()));
    archivo.write(reinterpret_cast<const char*>(fin.data()),
                  static_cast<std::streamsize>(fin.size()));
    return archivo.good();
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

// Manifiesto sintetico para los tests del gestor: arma el JSON de un mod con
// los obligatorios y el bloque `extra` que pase cada caso.
std::string manifiestoPara(const std::string& id, const std::string& version,
                           const std::string& extra = std::string()) {
    return std::string("{\"esquemaVersion\":1,\"id\":\"") + id +
           "\",\"nombre\":\"" + id + "\",\"version\":\"" + version +
           "\",\"tipo\":\"nativo\",\"puntoEntrada\":\"bin/m.so\"" +
           (extra.empty() ? std::string() : "," + extra) + "}";
}

// Arma un paquete .fmod con el manifiesto indicado dentro de `carpeta` (que
// crea). Es lo que el usuario copiaria a Mods/, con la ruta de paquete real.
bool crearPaquete(const fs::path& carpeta, const std::string& archivo,
                  const std::string& manifiesto) {
    std::error_code ec;
    fs::create_directories(carpeta, ec);
    return crearZip((carpeta / archivo).string(),
                    {{"ManifiestoMod.json", manifiesto},
                     {"bin/m.so", std::string("binario")}});
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

// Callback de panel de prueba: no dibuja nada, solo cuenta que se lo llamo.
int gLlamadasAlPanel = 0;
void panelDePrueba(const FunshiMods::ContextoMod* contexto, void* datos) {
    (void)contexto;
    (void)datos;
    ++gLlamadasAlPanel;
}

// Fuente de datos de escena falsa: dos objetos con componentes, para probar
// que la tabla de servicios reparte las consultas al motor sin conocerlo.
struct ObjetoFalso {
    const char* nombre;
    int componentes;
};
const ObjetoFalso kObjetosFalsos[] = {{"Cubo", 2}, {"Esfera", 1}};

int falsoContarObjetos(void*) { return 2; }

bool falsoObtenerObjeto(void*, int indice,
                         FunshiMods::ApiModGameObject* salida) {
    if (indice < 0 || indice >= 2) return false;
    std::snprintf(salida->nombre, sizeof(salida->nombre), "%s",
                  kObjetosFalsos[indice].nombre);
    salida->id = indice + 1;
    salida->cantidadComponentes = kObjetosFalsos[indice].componentes;
    salida->tieneCollider = indice == 0 ? 1 : 0;
    return true;
}

int falsoContarComponentes(void*, int indice) {
    if (indice < 0 || indice >= 2) return 0;
    return kObjetosFalsos[indice].componentes;
}

bool falsoObtenerComponente(void*, int indiceObjeto, int indiceComponente,
                             FunshiMods::ApiModComponente* salida) {
    if (indiceObjeto < 0 || indiceObjeto >= 2) return false;
    if (indiceComponente < 0 ||
        indiceComponente >= kObjetosFalsos[indiceObjeto].componentes) {
        return false;
    }
    std::snprintf(salida->tipo, sizeof(salida->tipo), "Componente%d",
                  indiceComponente);
    return true;
}

} // namespace

// 1. ModVersion: parseo y comparacion.
static void seccion01() {
    {
        ModVersion v;
        CHECK(ModVersion::parsear("1.2.3", v), "parsear acepta X.Y.Z");
        CHECK(v.valida && v.mayor == 1 && v.menor == 2 && v.parche == 3,
              "parsear descompone mayor.menor.parche");
        CHECK(v.aTexto() == "1.2.3", "aTexto reconstruye X.Y.Z");

        ModVersion otra;
        CHECK(ModVersion::parsear("1.2.4", otra), "parsear acepta otra version");
        CHECK(v.comparar(otra) < 0, "1.2.3 es menor que 1.2.4");
        CHECK(otra.comparar(v) > 0, "1.2.4 es mayor que 1.2.3");
        ModVersion igual;
        CHECK(ModVersion::parsear("1.2.3", igual), "parsear version igual");
        CHECK(v.comparar(igual) == 0, "versiones iguales comparan 0");

        ModVersion invalida;
        CHECK(!ModVersion::parsear("1.2", invalida), "rechaza X.Y sin parche");
        CHECK(!invalida.valida, "una version no parseada no queda valida");
        CHECK(!ModVersion::parsear("1.2.3.4", invalida), "rechaza cuatro segmentos");
        CHECK(!ModVersion::parsear("1.x.3", invalida), "rechaza segmento no numerico");
        CHECK(!ModVersion::parsear("", invalida), "rechaza texto vacio");

        // satisfaceMinimo: >= contra el minimo declarado.
        ModVersion minimo;
        CHECK(ModVersion::parsear("1.2.0", minimo), "parsear minimo");
        CHECK(v.satisfaceMinimo(minimo), "1.2.3 satisface >= 1.2.0");
        CHECK(igual.satisfaceMinimo(igual), "una version satisface su propio minimo");
        CHECK(!minimo.satisfaceMinimo(v), "1.2.0 no satisface >= 1.2.3");
    }
}

// 2. RangoVersion: cotas motor<->mod y mod<->mod.
static void seccion02() {
    {
        RangoVersion rango;
        CHECK(RangoVersion::parsear(">=1.0.0 <2.0.0", rango), "parsear rango con cota superior");
        CHECK(rango.valida, "el rango parseado es valido");

        ModVersion enRango, enCotaSup, enCotaInf, bajo, alto;
        ModVersion::parsear("1.0.0", enCotaInf);
        ModVersion::parsear("1.5.0", enRango);
        ModVersion::parsear("2.0.0", enCotaSup);
        ModVersion::parsear("0.9.9", bajo);
        ModVersion::parsear("2.0.1", alto);
        CHECK(rango.contiene(enCotaInf), "la cota inferior es inclusiva");
        CHECK(rango.contiene(enRango), "una version intermedia esta en el rango");
        CHECK(!rango.contiene(enCotaSup), "la cota superior es exclusiva");
        CHECK(!rango.contiene(bajo), "por debajo del minimo no esta en el rango");
        CHECK(!rango.contiene(alto), "por encima del maximo no esta en el rango");

        // Rango sin cota superior: solo minimo.
        RangoVersion soloMin;
        CHECK(RangoVersion::parsear(">=1.0.0", soloMin), "parsear rango sin cota superior");
        CHECK(soloMin.contiene(alto), "sin cota superior cualquier version mayor entra");

        RangoVersion malo;
        CHECK(!RangoVersion::parsear("1.0.0", malo), "rechaza rango sin operador >=");
        CHECK(!RangoVersion::parsear(">=1.0.0 <", malo), "rechaza cota superior sin version");
        CHECK(!RangoVersion::parsear("", malo), "rechaza rango vacio");
        CHECK(!malo.contiene(enRango), "un rango invalido no contiene nada");
    }
}

// 3. Manifiesto completo: parseo de todos los campos de la v1.
static void seccion03() {
    {
        ManifiestoMod mod;
        std::string error;
        CHECK(ManifiestoMod::parsear(manifiestoBase(), mod, error),
              "parsear acepta el manifiesto minimo valido");
        CHECK(error.empty(), "un parseo correcto no deja error");
        CHECK(mod.esquemaVersion == 1, "se lee esquemaVersion");
        CHECK(mod.id == "Autor.MiMod", "se lee el id");
        CHECK(mod.nombre == "Mi Mod", "se lee el nombre");
        CHECK(mod.autor == "Autor", "se lee el autor");
        CHECK(mod.version == "1.2.3", "se lee la version");
        CHECK(mod.licencia == "MIT", "se lee la licencia");
        CHECK(mod.tipo == "nativo", "se lee el tipo");
        CHECK(mod.versionMotor == ">=0.5.0 <1.0.0", "se lee versionMotor");
        CHECK(mod.puntoEntrada == "bin/mimod.dll", "se lee el punto de entrada");
        CHECK(mod.dependencias.size() == 1, "se lee una dependencia");
        CHECK(mod.dependencias.size() == 1 &&
                  mod.dependencias[0].id == "Otro.Base",
              "la dependencia conserva su id");
        CHECK(mod.dependencias.size() == 1 &&
                  mod.dependencias[0].versionMin == "2.0.0",
              "la dependencia conserva su versionMin");
        CHECK(mod.dependencias.size() == 1 &&
                  !mod.dependencias[0].requerida,
              "requerida:false se lee");
        CHECK(mod.dependencias.size() == 1 &&
                  mod.dependencias[0].orden == DependenciaMod::Orden::Antes,
              "orden 'antes' se traduce al enum");
        CHECK(mod.conflictos.size() == 1 && mod.conflictos[0] == "Tercero.Malo",
              "se lee la lista de conflictos");
    }
}

// 4. Tolerancia: los campos opcionales pueden faltar.
static void seccion04() {
    {
        ManifiestoMod mod;
        std::string error;
        const std::string minimo = R"json({
            "esquemaVersion": 1,
            "id": "Autor.Minimo",
            "nombre": "Minimo",
            "version": "1.0.0",
            "tipo": "nativo",
            "puntoEntrada": "bin/minimo.so"
        })json";
        CHECK(ManifiestoMod::parsear(minimo, mod, error),
              "un manifiesto sin opcionales es valido");
        CHECK(error.empty(), "sin opcionales tampoco hay error");
        CHECK(mod.descripcion.empty() && mod.icono.empty(),
              "descripcion/icono quedan vacios si faltan");
        CHECK(mod.dependencias.empty() && mod.conflictos.empty(),
              "sin dependencias ni conflictos las listas quedan vacias");
        CHECK(mod.versionMotor.empty(),
              "sin versionMotor no se exige rango contra el motor");

        // orden por defecto: ninguno; requerida por defecto: true.
        const std::string dependenciaMinima = R"json({
            "esquemaVersion": 1,
            "id": "Autor.Dep",
            "nombre": "Dep",
            "version": "1.0.0",
            "tipo": "nativo",
            "puntoEntrada": "bin/dep.dll",
            "dependencias": [{"id": "Otro.Base"}]
        })json";
        CHECK(ManifiestoMod::parsear(dependenciaMinima, mod, error),
              "una dependencia sin campos opcionales es valida");
        CHECK(mod.dependencias.size() == 1 &&
                  mod.dependencias[0].requerida,
              "requerida por defecto es true");
        CHECK(mod.dependencias.size() == 1 &&
                  mod.dependencias[0].orden == DependenciaMod::Orden::Ninguno,
              "orden por defecto es ninguno");
    }
}

// 5. Cortafuegos y validacion de campos obligatorios.
static void seccion05() {
    {
        ManifiestoMod mod;
        std::string error;

        // JSON corrupto.
        error.clear();
        CHECK(!ManifiestoMod::parsear("{", mod, error),
              "un JSON invalido no parsea");
        CHECK(!error.empty(), "el JSON invalido deja mensaje de error");

        // esquemaVersion ausente.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"id":"Autor.Mod","nombre":"N","version":"1.0.0",
                          "tipo":"nativo","puntoEntrada":"b.dll"})json",
                  mod, error),
              "falta esquemaVersion: se rechaza");
        CHECK(contiene(error, "esquemaVersion"),
              "el error nombra esquemaVersion faltante");

        // esquemaVersion desconocida: cortafuegos.
        error.clear();
        std::string futura = manifiestoBase();
        const std::string desdeEsquema =
            "\"esquemaVersion\": 1";
        futura.replace(futura.find(desdeEsquema), desdeEsquema.size(),
                       "\"esquemaVersion\": 99");
        CHECK(!ManifiestoMod::parsear(futura, mod, error),
              "un esquemaVersion desconocido se rechaza");
        CHECK(contiene(error, "esquemaVersion"),
              "el error nombra el esquemaVersion desconocido");

        // id sin formato Autor.Nombre.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"SinPunto","nombre":"N",
                          "version":"1.0.0","tipo":"nativo","puntoEntrada":"b.dll"})json",
                  mod, error),
              "un id sin punto se rechaza");
        CHECK(contiene(error, "id"), "el error nombra el id");
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Solo.","nombre":"N",
                          "version":"1.0.0","tipo":"nativo","puntoEntrada":"b.dll"})json",
                  mod, error),
              "un id con parte vacia se rechaza");

        // version invalida.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod","nombre":"N",
                          "version":"1.0","tipo":"nativo","puntoEntrada":"b.dll"})json",
                  mod, error),
              "una version no semver se rechaza");
        CHECK(contiene(error, "version"), "el error nombra la version");

        // tipo distinto de nativo (v1).
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod","nombre":"N",
                          "version":"1.0.0","tipo":"contenido","puntoEntrada":"b.dll"})json",
                  mod, error),
              "el tipo 'contenido' se rechaza en la v1");
        CHECK(contiene(error, "tipo"), "el error nombra el tipo");

        // puntoEntrada ausente.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod","nombre":"N",
                          "version":"1.0.0","tipo":"nativo"})json",
                  mod, error),
              "sin puntoEntrada se rechaza");
        CHECK(contiene(error, "puntoEntrada"),
              "el error nombra puntoEntrada");

        // nombre ausente.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod",
                          "version":"1.0.0","tipo":"nativo","puntoEntrada":"b.dll"})json",
                  mod, error),
              "sin nombre se rechaza");

        // versionMotor con rango invalido.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod","nombre":"N",
                          "version":"1.0.0","tipo":"nativo","puntoEntrada":"b.dll",
                          "versionMotor":"1.0.0"})json",
                  mod, error),
              "un versionMotor mal formado se rechaza");
        CHECK(contiene(error, "versionMotor"),
              "el error nombra versionMotor");

        // orden desconocido dentro de una dependencia.
        error.clear();
        CHECK(!ManifiestoMod::parsear(
                  R"json({"esquemaVersion":1,"id":"Autor.Mod","nombre":"N",
                          "version":"1.0.0","tipo":"nativo","puntoEntrada":"b.dll",
                          "dependencias":[{"id":"Otro.Base","orden":"al_lado"}]})json",
                  mod, error),
              "un orden desconocido se rechaza");
    }
}

// 6. leerDeArchivo: manifiesto desde disco y archivo inexistente.
static void seccion06() {
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_manifiesto_tests");
        const std::string ruta =
            (carpeta.ruta() / "ManifiestoMod.json").string();

        std::ofstream salida(ruta);
        salida << manifiestoBase();
        salida.close();

        ManifiestoMod mod;
        std::string error;
        CHECK(ManifiestoMod::leerDeArchivo(ruta, mod, error),
              "leerDeArchivo lee un manifiesto escrito en disco");
        CHECK(mod.id == "Autor.MiMod", "el manifiesto leido conserva su id");

        error.clear();
        CHECK(!ManifiestoMod::leerDeArchivo(
                  (carpeta.ruta() / "NoExiste.json").string(), mod, error),
              "un archivo inexistente no parsea");
        CHECK(!error.empty(),
              "el archivo inexistente deja mensaje de error");
    }
}

// 7. LectorZip: manifiesto desde un .fmod y extraccion anti zip-slip.
static void seccion07() {
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_zip_tests");
        const fs::path zip = carpeta.ruta() / "mod.fmod";
        CHECK(crearZip(zip.string(),
                       {{"ManifiestoMod.json", manifiestoBase()},
                        {"bin/mimod.so", std::string("binario falso")},
                        {"icono.png", std::string("png falso")}}),
              "el helper arma el .fmod de prueba con miniz");

        std::string contenido;
        std::string error;
        CHECK(LectorZip::leerEntrada(zip.string(), "ManifiestoMod.json",
                                     contenido, error),
              "leerEntrada encuentra el manifiesto en el paquete");
        CHECK(contenido == manifiestoBase(),
              "el manifiesto extraido coincide byte a byte con el original");

        ManifiestoMod mod;
        CHECK(ManifiestoMod::parsear(contenido, mod, error),
              "el manifiesto leido del .fmod parsea");

        error.clear();
        CHECK(!LectorZip::leerEntrada(zip.string(), "NoExiste.json", contenido,
                                      error),
              "una entrada ausente no se lee");
        CHECK(!error.empty(), "la entrada ausente deja mensaje de error");

        // Extraccion completa: respeta subdirectorios y contenido.
        const fs::path destino = carpeta.ruta() / "extraido";
        error.clear();
        CHECK(LectorZip::extraerTodo(zip.string(), destino.string(), error),
              "extraerTodo extrae el paquete completo");
        CHECK(error.empty(), "una extraccion correcta no deja error");
        CHECK(fs::exists(destino / "ManifiestoMod.json"),
              "el manifiesto queda en la raiz del destino");
        CHECK(fs::exists(destino / "bin" / "mimod.so"),
              "la extraccion crea los subdirectorios del paquete");
        CHECK(leerArchivoTexto((destino / "bin" / "mimod.so").string()) ==
                  "binario falso",
              "el contenido extraido coincide");

        // Zip-slip con "..": se rechaza entero y no escribe fuera.
        const fs::path malicioso = carpeta.ruta() / "malicioso.fmod";
        CHECK(crearZip(malicioso.string(),
                       {{"../escape.txt", std::string("malo")},
                        {"ManifiestoMod.json", manifiestoBase()}}),
              "se arma un .fmod con una entrada que escapa");
        error.clear();
        CHECK(!LectorZip::extraerTodo(malicioso.string(),
                                      (carpeta.ruta() / "extraido_mal").string(),
                                      error),
              "extraerTodo rechaza una entrada con '..'");
        CHECK(!error.empty(), "el rechazo de zip-slip deja mensaje");
        CHECK(!fs::exists(carpeta.ruta() / "escape.txt"),
              "nada se escribio fuera del destino");

        // Zip-slip con ruta absoluta: la raiz en POSIX ("/escape_abs.txt") y la
        // unidad en Windows ("C:/escape_unidad.txt"). miniz no escribe
        // entradas que empiezan por '/', asi que el paquete se arma a mano:
        // tiene que ser un ZIP valido (el motor lo lee) y aun asi rechazarlo.
        const fs::path absoluto = carpeta.ruta() / "absoluto.fmod";
        CHECK(crearZipConEntradaNombrada(absoluto.string(), "/escape_abs.txt",
                                          "malo"),
              "se arma un .fmod con una entrada absoluta de raiz");
        std::string contenidoAbsoluto;
        error.clear();
        CHECK(LectorZip::leerEntrada(absoluto.string(), "/escape_abs.txt",
                                     contenidoAbsoluto, error),
              "el paquete con la entrada absoluta es un ZIP valido");
        CHECK(error.empty(), "leer la entrada absoluta no deja error");
        error.clear();
        CHECK(!LectorZip::extraerTodo(absoluto.string(),
                                      (carpeta.ruta() / "extraido_abs").string(),
                                      error),
              "extraerTodo rechaza una entrada absoluta de raiz");
        CHECK(!error.empty(), "el rechazo de ruta absoluta deja mensaje");

        const fs::path absolutoUnidad = carpeta.ruta() / "absoluto_unidad.fmod";
        CHECK(crearZipConEntradaNombrada(absolutoUnidad.string(),
                                          "C:/escape_unidad.txt", "malo"),
              "se arma un .fmod con una entrada absoluta de unidad");
        error.clear();
        CHECK(!LectorZip::extraerTodo(absolutoUnidad.string(),
                                      (carpeta.ruta() / "extraido_unidad").string(),
                                      error),
              "extraerTodo rechaza una entrada absoluta de unidad");

        // Contrato anti zip-slip a nivel de ruta.
        CHECK(LectorZip::esRutaSegura("bin/mimod.so"),
              "una ruta relativa normal es segura");
        CHECK(!LectorZip::esRutaSegura("../escape.txt"),
              "una ruta con '..' no es segura");
        CHECK(!LectorZip::esRutaSegura("/abs.txt") &&
                  !LectorZip::esRutaSegura("C:/abs.txt"),
              "una ruta absoluta no es segura");
        CHECK(!LectorZip::esRutaSegura("bin\\..\\escape.txt"),
              "un '..' con separador de Windows tampoco es seguro");

        // Un archivo que no es un ZIP se rechaza sin volcar basura.
        const fs::path noZip = carpeta.ruta() / "no_es_zip.fmod";
        {
            std::ofstream salida(noZip);
            salida << "esto no es un zip";
        }
        error.clear();
        CHECK(!LectorZip::leerEntrada(noZip.string(), "ManifiestoMod.json",
                                      contenido, error),
              "un archivo que no es ZIP no se lee");
        error.clear();
        CHECK(!LectorZip::extraerTodo(noZip.string(),
                                      (carpeta.ruta() / "extraido_nozip").string(),
                                      error),
              "extraerTodo rechaza un archivo que no es ZIP");

        // ZIP inexistente.
        error.clear();
        CHECK(!LectorZip::leerEntrada((carpeta.ruta() / "no.fmod").string(),
                                      "ManifiestoMod.json", contenido, error),
              "un ZIP inexistente no se lee");
        CHECK(!error.empty(), "el ZIP inexistente deja mensaje de error");
    }
}

// 8. GestorMods: descubrimiento, validacion, orden por dependencias y
//    ambito motor/proyecto, con reporte al registro de diagnosticos.
static void seccion08() {
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_gestor_tests");
        const fs::path motor = carpeta.ruta() / "motor" / "Mods";
        const fs::path proyecto = carpeta.ruta() / "proyecto" / "Mods";

        // A y B (B depende de A), C que pide cargarse despues de D.
        CHECK(crearPaquete(motor, "a.fmod", manifiestoPara("Autor.A", "1.0.0")),
              "se empaqueta el mod A");
        CHECK(crearPaquete(motor, "b.fmod",
                           manifiestoPara("Autor.B", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.A\"}]")),
              "se empaqueta el mod B (depende de A)");
        CHECK(crearPaquete(proyecto, "c.fmod",
                           manifiestoPara("Autor.C", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.D\",\"requerida\":false,\"orden\":\"despues\"}]")),
              "se empaqueta el mod C del proyecto");
        CHECK(crearPaquete(proyecto, "d.fmod", manifiestoPara("Autor.D", "1.0.0")),
              "se empaqueta el mod D del proyecto");

        GestorMods::Resultado resultado = GestorMods::descubrir(
            {{motor.string(), AmbitoMod::Motor},
             {proyecto.string(), AmbitoMod::Proyecto}});
        CHECK(resultado.fallos.empty(), "los cuatro mods validos no dan fallos");
        CHECK(resultado.mods.size() == 4, "se descubren los cuatro mods");

        // Orden: A antes que B (dependencia requerida); C antes que D (orden
        // "despues" de C respecto de D).
        std::vector<std::string> ids;
        for (const PaqueteMod& mod : resultado.mods)
            ids.push_back(mod.manifiesto.id);
        auto posicionDe = [&ids](const std::string& id) {
            for (std::size_t i = 0; i < ids.size(); ++i)
                if (ids[i] == id)
                    return static_cast<int>(i);
            return -1;
        };
        CHECK(posicionDe("Autor.A") < posicionDe("Autor.B"),
              "A se ordena antes que B (B lo requiere)");
        CHECK(posicionDe("Autor.C") < posicionDe("Autor.D"),
              "C se ordena antes que D (C lo pide con orden 'despues')");

        // Ambito: cada paquete conserva de que carpeta salio.
        for (const PaqueteMod& mod : resultado.mods) {
            if (mod.manifiesto.id == "Autor.A")
                CHECK(mod.ambito == AmbitoMod::Motor,
                      "el mod de la carpeta del motor queda como Motor");
            if (mod.manifiesto.id == "Autor.C")
                CHECK(mod.ambito == AmbitoMod::Proyecto,
                      "el mod del proyecto queda como Proyecto");
        }

        // Reporte: resumen de cargados separado por ambito (motor / proyecto) y,
        // por cada fallo, mod y causa. Los mods del motor y los del proyecto se
        // cuentan y se reportan por separado, no como un unico total.
        const fs::path registro = carpeta.ruta() / "diagnosticos.jsonl";
        RegistroDiagnosticos diario(registro.string());
        GestorMods::reportar(resultado, diario);
        std::string resumen;
        for (const EventoDiagnostico& evento : diario.ultimos(50))
            resumen += evento.mensaje + "\n";
        CHECK(contiene(resumen, "mods de motor encontrados: 2"),
              "el reporte cuenta los mods del motor");
        CHECK(contiene(resumen, "mods de proyecto encontrados: 2"),
              "el reporte cuenta los mods del proyecto");
    }
}

// 9. GestorMods: casos de rechazo (id duplicado, dependencia faltante,
//    ciclo, version del motor, versionMin y conflicto).
static void seccion09() {
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_rechazos_tests");
        const fs::path motor = carpeta.ruta() / "Mods";
        const fs::path proyecto = carpeta.ruta() / "Proyecto" / "Mods";

        // id duplicado entre motor y proyecto: se rechazan las dos instancias.
        CHECK(crearPaquete(motor, "dup1.fmod", manifiestoPara("Autor.Dup", "1.0.0")),
              "se empaqueta la primera instancia del id duplicado");
        CHECK(crearPaquete(proyecto, "dup2.fmod", manifiestoPara("Autor.Dup", "2.0.0")),
              "se empaqueta la segunda instancia del id duplicado");
        // dependencia faltante.
        CHECK(crearPaquete(motor, "huerfano.fmod",
                           manifiestoPara("Autor.Huerfano", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.NoExiste\"}]")),
              "se empaqueta un mod con dependencia faltante");
        // ciclo entre dos mods.
        CHECK(crearPaquete(motor, "ciclo1.fmod",
                           manifiestoPara("Autor.Ciclo1", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.Ciclo2\"}]")),
              "se empaqueta el primer mod del ciclo");
        CHECK(crearPaquete(motor, "ciclo2.fmod",
                           manifiestoPara("Autor.Ciclo2", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.Ciclo1\"}]")),
              "se empaqueta el segundo mod del ciclo");
        // versionMotor incompatible con el motor.
        CHECK(crearPaquete(motor, "futuro.fmod",
                           manifiestoPara("Autor.Futuro", "1.0.0",
                                          "\"versionMotor\":\">=99.0.0\"")),
              "se empaqueta un mod que pide un motor futuro");
        // versionMin insatisfecha por la dependencia presente.
        CHECK(crearPaquete(motor, "viejo.fmod", manifiestoPara("Autor.Viejo", "1.0.0")),
              "se empaqueta la dependencia vieja");
        CHECK(crearPaquete(motor, "exigente.fmod",
                           manifiestoPara("Autor.Exigente", "1.0.0",
                                          "\"dependencias\":[{\"id\":\"Autor.Viejo\",\"versionMin\":\"2.0.0\"}]")),
              "se empaqueta un mod que exige una version mayor");
        // conflicto con un mod presente.
        CHECK(crearPaquete(motor, "conflictivo.fmod",
                           manifiestoPara("Autor.Conflictivo", "1.0.0",
                                          "\"conflictos\":[\"Autor.Viejo\"]")),
              "se empaqueta un mod en conflicto");
        // manifiesto roto: no aporta mod.
        CHECK(crearPaquete(motor, "roto.fmod", "{ esto no es json"),
              "se empaqueta un manifiesto roto");
        // archivo que no es .fmod: se ignora.
        {
            std::ofstream otro((motor / "notas.txt").string());
            otro << "no soy un mod";
        }

        GestorMods::Resultado resultado = GestorMods::descubrir(
            {{motor.string(), AmbitoMod::Motor},
             {proyecto.string(), AmbitoMod::Proyecto}});

        std::string causas;
        for (const FalloMod& fallo : resultado.fallos)
            causas += fallo.paquete + " | " + fallo.causa + "\n";
        auto hayCausa = [&causas](const std::string& aguja) {
            return causas.find(aguja) != std::string::npos;
        };

        CHECK(contiene(causas, "Autor.Dup"),
              "el id duplicado queda reportado");
        CHECK(contiene(causas, "duplicado"),
              "la causa del id duplicado se explica");
        CHECK(contiene(causas, "Autor.Huerfano") &&
                  contiene(causas, "Autor.NoExiste"),
              "la dependencia faltante se reporta con el id que falta");
        CHECK(hayCausa("ciclo"), "el ciclo entre mods se reporta");
        CHECK(contiene(causas, "Autor.Futuro"),
              "el mod incompatible con el motor se rechaza");
        CHECK(contiene(causas, "Autor.Exigente"),
              "el mod que exige una version mayor se rechaza");
        CHECK(contiene(causas, "Autor.Conflictivo"),
              "el mod en conflicto se rechaza");
        CHECK(contiene(causas, "roto.fmod"),
              "el manifiesto roto se reporta con su paquete");

        // Ningun mod invalido ni ignorado entra en la lista de carga: quedan
        // solo los que pueden cargarse (Viejo, que nadie pudo exigir OK).
        bool cargaDuplicado = false;
        bool cargaCiclo = false;
        bool cargaRoto = false;
        for (const PaqueteMod& mod : resultado.mods) {
            if (mod.manifiesto.id == "Autor.Dup")
                cargaDuplicado = true;
            if (mod.manifiesto.id == "Autor.Ciclo1" ||
                mod.manifiesto.id == "Autor.Ciclo2")
                cargaCiclo = true;
        }
        CHECK(!cargaDuplicado, "un id duplicado no entra a la lista de carga");
        CHECK(!cargaCiclo, "un mod en ciclo no entra a la lista de carga");
        CHECK(!cargaRoto, "un manifiesto roto no entra a la lista de carga");

        // Reporte de los rechazos: un evento por mod fallido, con su causa.
        const fs::path registro = carpeta.ruta() / "diagnosticos.jsonl";
        RegistroDiagnosticos diario(registro.string());
        GestorMods::reportar(resultado, diario);
        std::string texto;
        for (const EventoDiagnostico& evento : diario.ultimos(100))
            texto += evento.mensaje + "\n";
        CHECK(contiene(texto, "mods de motor encontrados: 1"),
              "el reporte resume los mods del motor que si cargan");
        CHECK(contiene(texto, "Autor.Huerfano"),
              "el reporte incluye el mod fallido y su causa");

        // Carpetas inexistentes: no hay mods ni fallos.
        GestorMods::Resultado vacio = GestorMods::descubrir(
            {{(carpeta.ruta() / "NoExiste").string(), AmbitoMod::Motor}});
        CHECK(vacio.mods.empty() && vacio.fallos.empty(),
              "una carpeta inexistente no aporta mods ni fallos");
    }
}

// 10. Rutas de mods: la global cuelga de la raiz de datos y la del
//     proyecto de su carpeta. ProjectPaths compone con '/', que es su
//     contrato (no el separador nativo): se compara contra el mismo texto.
static void seccion10() {
    {
        CHECK(ProjectPaths::directorioMods() ==
                  ProjectPaths::directorioBase() + "/Mods",
              "directorioMods cuelga de la raiz de datos");
        CHECK(ProjectPaths::directorioModsProyecto("MiJuego") ==
                  ProjectPaths::directorioProyecto("MiJuego") + "/Mods",
              "directorioModsProyecto cuelga del proyecto");
        CHECK(fs::path(ProjectPaths::directorioMods()).filename() == "Mods",
              "la carpeta global se llama Mods");
    }
}

// 11. CargadorMods: carga real del binario de un mod desde la cache.
//     Se empaquetan los plugins de prueba (bibliotecas compartidas que
//     exportan la funcion de entrada) como .fmod y se cargan de verdad:
//    'extraction a cache, apertura del binario, inicializacion, orden de
//     descarga y los rechazos con su causa.
static void seccion11() {
    {
        const fs::path pluginA = FUNSHI_MODS_PLUGIN_A;
        const fs::path pluginB = FUNSHI_MODS_PLUGIN_B;
        const fs::path pluginSinSimbolo = FUNSHI_MODS_PLUGIN_SIN_SIMBOLO;
        CHECK(fs::exists(pluginA), "el plugin de prueba A esta construido");
        CHECK(fs::exists(pluginB), "el plugin de prueba B esta construido");
        CHECK(fs::exists(pluginSinSimbolo),
              "el plugin de prueba sin funcion de entrada esta construido");

        TempPruebas::CarpetaPrueba carpeta("funshi_mods_cargador_tests");
        const fs::path mods = carpeta.ruta() / "Mods";
        const fs::path cache = carpeta.ruta() / "cache";
        const fs::path archivoLog = carpeta.ruta() / "plugin.log";
        fijarVariableEntorno("FUNSHI_MODS_TEST_LOG", archivoLog.string());

        // Dos mods, cada uno con su binario, empaquetados como .fmod.
        const std::string binarioA = leerArchivoTexto(pluginA.string());
        const std::string binarioB = leerArchivoTexto(pluginB.string());
        const std::string entradaA = "bin/" + pluginA.filename().string();
        const std::string entradaB = "bin/" + pluginB.filename().string();
        CHECK(crearPaqueteConBinario(mods, "a.fmod",
                                      manifiestoConEntrada("Prueba.ModA", "1.0.0",
                                                            entradaA),
                                      entradaA, binarioA),
              "se empaqueta el mod A con su binario");
        CHECK(crearPaqueteConBinario(mods, "b.fmod",
                                      manifiestoConEntrada("Prueba.ModB", "1.0.0",
                                                            entradaB),
                                      entradaB, binarioB),
              "se empaqueta el mod B con su binario");

        const GestorMods::Resultado descubrimiento = GestorMods::descubrir(
            {{mods.string(), AmbitoMod::Motor}});
        CHECK(descubrimiento.mods.size() == 2,
              "se descubren los dos paquetes de prueba");
        CHECK(descubrimiento.fallos.empty(),
              "los dos paquetes de prueba son validos");

        CargadorMods::ResultadoCarga carga =
            CargadorMods::cargar(descubrimiento, cache);
        CHECK(carga.fallos.empty(), "no hay fallos al cargar los dos mods");
        CHECK(carga.modsCargados.size() == 2, "se cargan los dos mods");
        if (carga.modsCargados.size() == 2) {
            CHECK(carga.modsCargados[0].id == "Prueba.ModA",
                  "el primer mod cargado es el A");
            CHECK(carga.modsCargados[1].id == "Prueba.ModB",
                  "el segundo mod cargado es el B");
            CHECK(carga.modsCargados[0].interfaz != nullptr,
                  "el mod cargado entrega su interfaz");
            const FunshiMods::IMod* interfaz = carga.modsCargados[0].interfaz;
            CHECK(interfaz->nombre != nullptr &&
                      std::string(interfaz->nombre()) == "Mod de prueba",
                  "el mod responde su nombre por la interfaz");
        }

        // El paquete se extrae a la cache: de ahi se carga el binario.
        CHECK(fs::exists(cache / "Prueba.ModA" / entradaA),
              "el paquete se extrae a la carpeta de cache del mod");
        CHECK(fs::exists(carga.modsCargados.empty()
                             ? fs::path()
                             : carga.modsCargados[0].rutaBinario),
              "el binario cargado vive en la cache");

        // Lo que recibio el mod: entrada, inicializacion y tabla de servicios.
        const std::string traza = leerArchivoTexto(archivoLog.string());
        CHECK(contiene(traza, "A:entrada"),
              "el motor llama la funcion de entrada del mod");
        CHECK(contiene(traza, "A:init"), "el motor inicializa el mod");
        CHECK(contiene(traza, "A:servicios=2"),
              "el mod recibe la version de la API de mods");
        CHECK(contiene(traza, "B:init"), "el motor inicializa el segundo mod");
        CHECK(traza.find("A:init") < traza.find("B:init"),
              "los mods se inicializan en el orden de carga");

        // Reporte de la carga: un evento por fallo y el resumen de los que
        // quedaron en memoria.
        const fs::path registro = carpeta.ruta() / "diagnosticos.jsonl";
        RegistroDiagnosticos diario(registro.string());
        CargadorMods::reportar(carga, diario);
        std::string texto;
        for (const EventoDiagnostico& evento : diario.ultimos(50))
            texto += evento.mensaje + "\n";
        CHECK(contiene(texto, "mods nativos cargados: 2"),
              "el reporte resume los mods nativos cargados");

        // Descarga: cada mod se apaga y su binario se cierra, en orden
        // inverso al de carga.
        CargadorMods::descargar(carga.modsCargados);
        const std::string trazaFinal = leerArchivoTexto(archivoLog.string());
        CHECK(contiene(trazaFinal, "A:shutdown"),
              "el mod A se apaga al descargar");
        CHECK(contiene(trazaFinal, "B:shutdown"),
              "el mod B se apaga al descargar");
        CHECK(trazaFinal.find("B:shutdown") < trazaFinal.find("A:shutdown"),
              "los mods se apagan en orden inverso al de carga");
        CHECK(carga.modsCargados.empty(),
              "la descarga deja la lista de mods vacia");
    }
}

// 12. CargadorMods: rechazos. Un mod sin punto de entrada, uno cuyo
//     paquete no trae el binario y uno cuyo binario no exporta la funcion
//     de entrada: los tres se rechazan con una causa legible.
static void seccion12() {
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_cargador_fallos_tests");
        const fs::path cache = carpeta.ruta() / "cache";
        const fs::path mods = carpeta.ruta() / "Mods";

        // Sin punto de entrada: el manifiesto lo rechazaria, pero el cargador
        // tambien lo defiende (se le puede pasar un resultado armado a mano).
        GestorMods::Resultado sinEntrada;
        PaqueteMod paqueteSin;
        paqueteSin.paquete = mods / "sin-entrada.fmod";
        paqueteSin.manifiesto.id = "Prueba.SinEntrada";
        sinEntrada.mods.push_back(paqueteSin);
        const CargadorMods::ResultadoCarga cargaSin =
            CargadorMods::cargar(sinEntrada, cache);
        CHECK(cargaSin.modsCargados.empty(),
              "un mod sin punto de entrada no se carga");
        CHECK(cargaSin.fallos.size() == 1, "el fallo queda reportado");
        CHECK(!cargaSin.fallos.empty() &&
                  contiene(cargaSin.fallos[0].causa, "puntoEntrada"),
              "la causa menciona el punto de entrada que falta");

        // El paquete no trae el binario que declara el manifiesto.
        CHECK(crearPaqueteConBinario(
                  mods, "sin-binario.fmod",
                  manifiestoConEntrada("Prueba.SinBinario", "1.0.0",
                                       "bin/no-existe.dll"),
                  "bin/otro.dll", "contenido"),
              "se empaqueta un mod cuyo binario no esta en el paquete");
        const GestorMods::Resultado descSinBinario = GestorMods::descubrir(
            {{mods.string(), AmbitoMod::Motor}});
        CHECK(descSinBinario.mods.size() == 1,
              "el paquete sin binario es valido como manifiesto");
        const CargadorMods::ResultadoCarga cargaSinBinario =
            CargadorMods::cargar(descSinBinario, cache);
        CHECK(cargaSinBinario.modsCargados.empty(),
              "un mod cuyo binario falta no se carga");
        CHECK(cargaSinBinario.fallos.size() == 1,
              "el fallo del binario ausente queda reportado");
        CHECK(!cargaSinBinario.fallos.empty() &&
                  contiene(cargaSinBinario.fallos[0].causa, "no-existe.dll"),
              "la causa menciona el binario que no esta");

        // El binario existe pero no exporta la funcion de entrada.
        const fs::path pluginSinSimbolo = FUNSHI_MODS_PLUGIN_SIN_SIMBOLO;
        const std::string binarioSinSimbolo =
            leerArchivoTexto(pluginSinSimbolo.string());
        const std::string entradaSinSimbolo =
            "bin/" + pluginSinSimbolo.filename().string();
        CHECK(crearPaqueteConBinario(
                  mods, "sin-simbolo.fmod",
                  manifiestoConEntrada("Prueba.SinSimbolo", "1.0.0",
                                       entradaSinSimbolo),
                  entradaSinSimbolo, binarioSinSimbolo),
              "se empaqueta un mod cuyo binario no exporta la entrada");
        const GestorMods::Resultado descSinSimbolo = GestorMods::descubrir(
            {{mods.string(), AmbitoMod::Motor}});
        const CargadorMods::ResultadoCarga cargaSinSimbolo =
            CargadorMods::cargar(descSinSimbolo, cache);
        CHECK(cargaSinSimbolo.modsCargados.empty(),
              "un mod sin la funcion de entrada no se carga");
        CHECK(cargaSinSimbolo.fallos.size() == 2,
              "los dos paquetes defectuosos quedan reportados");
        std::string causas;
        for (const FalloMod& fallo : cargaSinSimbolo.fallos)
            causas += fallo.causa + "\n";
        CHECK(contiene(causas, "funshi_mod_api"),
              "la causa menciona el simbolo que falta");
        CHECK(contiene(causas, "no-existe.dll"),
              "la causa menciona el binario que no esta");

        // El reporte de la carga lista los fallos con su causa.
        const fs::path registro = carpeta.ruta() / "diagnosticos.jsonl";
        RegistroDiagnosticos diario(registro.string());
        CargadorMods::reportar(cargaSinSimbolo, diario);
        std::string texto;
        for (const EventoDiagnostico& evento : diario.ultimos(50))
            texto += evento.mensaje + "\n";
        CHECK(contiene(texto, "mod no cargado"),
              "el reporte dice que mod no se cargo");
        CHECK(contiene(texto, "funshi_mod_api"),
              "el reporte incluye la causa del fallo");
    }
}

// 13. Registro de paneles de mods: alta de paneles, estado por ventana de
//     panel y atribucion al mod que los registra.
static void seccion13() {
    {
        using FunshiMods::RegistroPanelesMods;
        auto& registro = RegistroPanelesMods::global();
        registro.limpiar();

        CHECK(registro.registrar("Mod.Uno", "panel", "Panel de uno",
                                  &panelDePrueba, nullptr),
              "un mod puede registrar un panel");
        CHECK(registro.paneles().size() == 1,
              "el panel registrado queda en el registro");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].modId == "Mod.Uno",
              "el panel guarda el mod al que pertenece");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].titulo == "Panel de uno",
              "el panel guarda su titulo visible");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].nombreVentana ==
                      "Mod:Mod.Uno:panel",
              "la clave de ventana del panel es unica por mod y panel");
        CHECK(!registro.paneles().empty() && registro.paneles()[0].abierta,
              "un panel recien registrado se abre");

        CHECK(!registro.registrar("Mod.Uno", "panel", "Otro", &panelDePrueba,
                                  nullptr),
              "el mismo id no se puede registrar dos veces en un mod");
        CHECK(registro.paneles().size() == 1,
              "el registro no crece con un id repetido");
        CHECK(registro.registrar("Mod.Dos", "panel", "Panel de dos",
                                  &panelDePrueba, nullptr),
              "otro mod puede usar el mismo id de panel");
        CHECK(registro.paneles().size() == 2,
              "el panel del segundo mod queda en el registro");

        CHECK(!registro.registrar("Mod.Uno", "", "Sin id", &panelDePrueba,
                                  nullptr),
              "un panel sin id no se registra");
        CHECK(!registro.registrar("Mod.Uno", "otro", "", &panelDePrueba,
                                  nullptr),
              "un panel sin titulo no se registra");
        CHECK(!registro.registrar("Mod.Uno", "otro", "Sin callback", nullptr,
                                  nullptr),
              "un panel sin callback no se registra");
        CHECK(registro.paneles().size() == 2,
              "los paneles invalidos no llegan al registro");

        // Estado por ventana de panel: es lo que se persiste y lo que alterna
        // el menu "Ventanas".
        const std::string ventana = "Mod:Mod.Uno:panel";
        CHECK(registro.buscarPorVentana(ventana) != nullptr,
              "el panel se busca por su clave de ventana");
        CHECK(registro.buscarPorVentana("Mod:Mod.Nadie:panel") == nullptr,
              "una ventana que no existe no se encuentra");
        CHECK(registro.establecerEstado(ventana, true),
              "se puede abrir un panel por su clave de ventana");
        CHECK(registro.buscarPorVentana(ventana) != nullptr &&
                  registro.buscarPorVentana(ventana)->abierta,
              "el panel queda abierto");
        CHECK(!registro.establecerEstado("Mod:Mod.Nadie:panel", true),
              "no se puede abrir un panel inexistente");
        CHECK(registro.establecerEstado(ventana, false),
              "se puede cerrar un panel por su clave de ventana");
        CHECK(registro.buscarPorVentana(ventana) != nullptr &&
                  !registro.buscarPorVentana(ventana)->abierta,
              "el panel queda cerrado");

        // Atribucion: mientras un mod esta en ejecucion, los paneles que
        // registra son suyos.
        CHECK(!registro.registrarDelModEnEjecucion("x", "X", &panelDePrueba,
                                                    nullptr),
              "sin mod en ejecucion no se registra ningun panel");
        registro.fijarModEnEjecucion("Mod.Uno");
        CHECK(registro.registrarDelModEnEjecucion("propio", "Panel propio",
                                                    &panelDePrueba, nullptr),
              "el mod en ejecucion puede registrar un panel");
        CHECK(registro.paneles().size() == 3,
              "el panel del mod en ejecucion queda en el registro");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles().back().modId == "Mod.Uno",
              "el panel se atribuye al mod en ejecucion");
        registro.limpiarModEnEjecucion();
        CHECK(registro.modEnEjecucion().empty(),
              "al salir se limpia el mod en ejecucion");

        // Al descargar un mod se van sus paneles, y solo los suyos.
        registro.quitarMod("Mod.Uno");
        CHECK(registro.paneles().size() == 1,
              "los paneles del mod descargado se van");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].modId == "Mod.Dos",
              "los paneles de los otros mods siguen");
        registro.limpiar();
        CHECK(registro.paneles().empty(),
              "el registro se puede vaciar por completo");
    }
}

// 14. Tabla de servicios de mods: version en offset fijo, registro de
//     paneles y consultas de datos de la escena.
static void seccion14() {
    {
        using FunshiMods::RegistroPanelesMods;
        auto& registro = RegistroPanelesMods::global();
        registro.limpiar();

        const FunshiMods::ApiModServicios* servicios =
            FunshiMods::tablaServicios();
        CHECK(servicios != nullptr, "la tabla de servicios existe");
        CHECK(servicios->version == FunshiMods::versionApiMod,
              "la tabla ofrece la version actual de la API de mods");
        CHECK(offsetof(FunshiMods::ApiModServicios, version) == 0,
              "la version va en el primer campo: un mod compilado contra una "
              "version anterior lee su version en el offset de siempre");
        CHECK(servicios->registrarPanel != nullptr &&
                  servicios->contarObjetos != nullptr &&
                  servicios->obtenerObjeto != nullptr &&
                  servicios->contarComponentes != nullptr &&
                  servicios->obtenerComponente != nullptr &&
                  servicios->nombreProyecto != nullptr &&
                  servicios->contextoImGui != nullptr,
              "la tabla ofrece los servicios de la API v2");

        // Registro de paneles desde la tabla: se atribuye al mod en ejecucion.
        registro.fijarModEnEjecucion("Mod.Servicios");
        CHECK(servicios->registrarPanel("panel", "Panel del servicio",
                                        &panelDePrueba, nullptr),
              "el mod registra un panel por la tabla de servicios");
        CHECK(registro.paneles().size() == 1 &&
                  registro.paneles()[0].modId == "Mod.Servicios",
              "el panel de la tabla pertenece al mod en ejecucion");
        CHECK(!servicios->registrarPanel(nullptr, "Sin id", &panelDePrueba,
                                         nullptr),
              "un panel sin id se rechaza");
        registro.limpiarModEnEjecucion();

        // Datos de la escena: sin fuente conectada no hay datos, y no se cae.
        FunshiMods::inyectarDatosEscena(nullptr);
        CHECK(servicios->contarObjetos() == 0,
              "sin escena conectada no hay objetos");
        FunshiMods::ApiModGameObject objeto;
        CHECK(!servicios->obtenerObjeto(0, &objeto),
              "sin escena conectada no se obtiene ningun objeto");
        CHECK(servicios->contarComponentes(0) == 0,
              "sin escena conectada no hay componentes");
        FunshiMods::ApiModComponente componente;
        CHECK(!servicios->obtenerComponente(0, 0, &componente),
              "sin escena conectada no se obtiene ningun componente");

        // Con la fuente conectada, las consultas llegan al motor.
        const FunshiMods::FuenteDatosEscena fuente = {
            /* .contarObjetos     = */ falsoContarObjetos,
            /* .obtenerObjeto     = */ falsoObtenerObjeto,
            /* .contarComponentes = */ falsoContarComponentes,
            /* .obtenerComponente = */ falsoObtenerComponente,
            /* .contexto          = */ nullptr,
        };
        FunshiMods::inyectarDatosEscena(&fuente);
        CHECK(servicios->contarObjetos() == 2,
              "la tabla cuenta los objetos de la escena");
        CHECK(servicios->obtenerObjeto(0, &objeto),
              "la tabla obtiene los datos de un objeto");
        CHECK(std::string(objeto.nombre) == "Cubo",
              "el objeto trae su nombre");
        CHECK(objeto.id == 1, "el objeto trae su id");
        CHECK(objeto.tieneCollider == 1,
              "el objeto indica si tiene collider");
        CHECK(objeto.cantidadComponentes == 2,
              "el objeto indica cuantos componentes tiene");
        CHECK(!servicios->obtenerObjeto(5, &objeto),
              "un indice de objeto que no existe se rechaza");
        CHECK(servicios->contarComponentes(1) == 1,
              "la tabla cuenta los componentes de un objeto");
        CHECK(servicios->obtenerComponente(0, 1, &componente),
              "la tabla obtiene los datos de un componente");
        CHECK(std::string(componente.tipo) == "Componente1",
              "el componente trae el nombre de su tipo");
        CHECK(!servicios->obtenerComponente(0, 9, &componente),
              "un indice de componente que no existe se rechaza");
        FunshiMods::inyectarDatosEscena(nullptr);

        // Nombre del proyecto y contexto de ImGui.
        FunshiMods::fijarNombreProyecto("Mi Juego");
        CHECK(std::string(servicios->nombreProyecto()) == "Mi Juego",
              "el mod ve el nombre del proyecto abierto");
        int contextoFalso = 0;
        FunshiMods::fijarContextoImGui(&contextoFalso);
        CHECK(servicios->contextoImGui() == &contextoFalso,
              "el mod recibe el contexto de ImGui del motor");
        FunshiMods::fijarContextoImGui(nullptr);
        CHECK(servicios->contextoImGui() == nullptr,
              "sin contexto de ImGui la tabla lo entrega vacio");
        FunshiMods::fijarNombreProyecto("");
        registro.limpiar();
    }
}

// 15. CargadorMods: los panels de un mod cargado, su actualizacion por
//     frame y su retirada al descargarlo.
static void seccion15() {
    {
        using FunshiMods::RegistroPanelesMods;
        auto& registro = RegistroPanelesMods::global();
        registro.limpiar();

        const fs::path pluginA = FUNSHI_MODS_PLUGIN_A;
        TempPruebas::CarpetaPrueba carpeta("funshi_mods_paneles_tests");
        const fs::path mods = carpeta.ruta() / "Mods";
        const fs::path cache = carpeta.ruta() / "cache";
        const fs::path archivoLog = carpeta.ruta() / "plugin.log";
        fijarVariableEntorno("FUNSHI_MODS_TEST_LOG", archivoLog.string());

        const std::string binarioA = leerArchivoTexto(pluginA.string());
        const std::string entradaA = "bin/" + pluginA.filename().string();
        CHECK(crearPaqueteConBinario(mods, "a.fmod",
                                      manifiestoConEntrada("Prueba.ModA", "1.0.0",
                                                            entradaA),
                                      entradaA, binarioA),
              "se empaqueta el mod A con su binario");

        const GestorMods::Resultado descubrimiento = GestorMods::descubrir(
            {{mods.string(), AmbitoMod::Motor}});
        CargadorMods::ResultadoCarga carga =
            CargadorMods::cargar(descubrimiento, cache);
        CHECK(carga.fallos.empty(), "el mod de prueba se carga sin fallos");
        CHECK(carga.modsCargados.size() == 1, "se carga el mod de prueba");

        // El mod implementa dibujar(): el motor le abre su propia ventana.
        CHECK(registro.paneles().size() == 1,
              "el mod cargado deja un panel en el registro");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].modId == "Prueba.ModA",
              "el panel pertenece al mod cargado");
        CHECK(!registro.paneles().empty() &&
                  registro.paneles()[0].titulo == "Mod de prueba",
              "el panel propio se titula con el nombre del mod");

        // Actualizacion por frame: el mod recibe update().
        CargadorMods::actualizar(carga.modsCargados, 0.016f);
        const std::string traza = leerArchivoTexto(archivoLog.string());
        CHECK(contiene(traza, "A:update"),
              "el motor actualiza el mod una vez por frame");

        // Al descargar el mod, sus paneles se van del registro.
        CargadorMods::descargar(carga.modsCargados);
        CHECK(registro.paneles().empty(),
              "los paneles del mod se retiran al descargarlo");
    }
}

int main() {
    seccion01();

    seccion02();

    seccion03();

    seccion04();

    seccion05();

    seccion06();

    seccion07();

    seccion08();

    seccion09();

    seccion10();

    seccion11();

    seccion12();

    seccion13();

    seccion14();

    seccion15();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << (total - fallos)
              << '/' << total << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
