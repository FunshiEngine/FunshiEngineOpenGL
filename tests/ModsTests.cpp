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
#include "ModsTestsDetalle.h"

namespace ModsTestsDetalle {

int total = 0;
int fallos = 0;
// Callback de panel de prueba: no dibuja nada, solo cuenta que se lo llamo.
int gLlamadasAlPanel = 0;

}  // namespace ModsTestsDetalle
using namespace ModsTestsDetalle;

// 1. ModVersion: parseo y comparacion.
void seccion01() {
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
void seccion02() {
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
void seccion03() {
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
void seccion04() {
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
void seccion05() {
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
void seccion06() {
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
