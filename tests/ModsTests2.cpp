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

namespace {

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

}  // namespace

// 7. LectorZip: manifiesto desde un .fmod y extraccion anti zip-slip.
void seccion07() {
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
void seccion08() {
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
void seccion09() {
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
