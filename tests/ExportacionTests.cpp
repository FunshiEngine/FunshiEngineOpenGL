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
// Pruebas headless de la exportacion de juegos.
//
// El dialogo de exportacion es lo que el usuario toca para exportar. Lo que se
// comprueba aqui son sus reglas, que no dependen de la ventana:
//   - las precondiciones (proyecto elegido y con carpeta) dicen el motivo
//     cuando no se pueden cumplir,
//   - el estado de la exportacion conserva el resultado (exito o error) al
//     terminar, para que la interfaz pueda mostrarlo en vez de perderlo,
//   - la barra de menu le pasa al dialogo el proyecto activo.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/Exportador/EstadoExportacion.h"
#include "../FunshiEngineGL/src/Exportador/GameExporter.h"
#include "../FunshiEngineGL/src/Exportador/ImportacionesPE.h"
#include "../FunshiEngineGL/src/Exportador/PrecondicionesExportacion.h"
#include "../FunshiEngineGL/src/GUI/Export/ExportDialog.h"
#include "../FunshiEngineGL/src/GUI/SceneGUI/SceneMenuBarInterface.h"

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Carpeta temporal que se borra al salir (RAII).
struct CarpetaTemporal {
    std::filesystem::path ruta;

    explicit CarpetaTemporal(const std::string& nombre) {
        ruta = std::filesystem::temp_directory_path() /
               ("funshi_exportacion_" + nombre);
        std::error_code ec;
        std::filesystem::remove_all(ruta, ec);
        std::filesystem::create_directories(ruta, ec);
    }

    ~CarpetaTemporal() {
        std::error_code ec;
        std::filesystem::remove_all(ruta, ec);
    }
};

void comprobarPrecondiciones() {
    std::string error;
    CHECK(!PrecondicionesExportacion::comprobar("", "", error),
          "sin proyecto no se puede exportar");
    CHECK(!error.empty(), "sin proyecto se explica el motivo");
    CHECK(error.find("proyecto") != std::string::npos,
          "el motivo menciona el proyecto que falta");

    error.clear();
    CHECK(!PrecondicionesExportacion::comprobar("MiProyecto",
                                                 "/no/existe/este/proyecto",
                                                 error),
          "una carpeta inexistente impide exportar");
    CHECK(error.find("no existe") != std::string::npos,
          "el motivo dice que el proyecto no existe");

    CarpetaTemporal carpeta("proyecto_valido");
    error.clear();
    CHECK(PrecondicionesExportacion::comprobar("MiProyecto",
                                                carpeta.ruta.string(), error),
          "un proyecto con carpeta se puede exportar");
    CHECK(error.empty(), "sin motivo de error cuando todo esta bien");
}

void comprobarEstado() {
    EstadoExportacion estado;
    CHECK(estado.fase() == EstadoExportacion::Fase::Configuracion,
          "el estado arranca en configuracion");
    CHECK(!estado.hayMensaje(), "sin mensaje antes de exportar");

    estado.iniciar();
    CHECK(estado.exportando(), "al iniciar esta exportando");
    CHECK(estado.progreso() == 0.0f, "el progreso vuelve a cero");
    CHECK(estado.resultado() == EstadoExportacion::Resultado::Pendiente,
          "todavia no hay resultado");

    estado.registrarProgreso(0.5f, "Compilando");
    CHECK(estado.progreso() == 0.5f, "el progreso se actualiza");
    CHECK(estado.etapa() == "Compilando", "la etapa se actualiza");

    estado.registrarLog("una linea de bitacora");
    CHECK(estado.mensaje() == "una linea de bitacora", "la bitacora se guarda");

    // El resultado no se pierde al terminar: es lo que le dice al usuario que
    // paso con lo que pidio.
    estado.finalizar(false, "Error generando CMake");
    CHECK(estado.terminado(), "al terminar queda en resultado");
    CHECK(estado.resultado() == EstadoExportacion::Resultado::Error,
          "un fallo se marca como error");
    CHECK(estado.hayMensaje(), "el motivo del fallo se conserva");
    CHECK(estado.mensaje() == "Error generando CMake",
          "el motivo del fallo es el que se dio");

    estado.reiniciar();
    CHECK(estado.fase() == EstadoExportacion::Fase::Configuracion,
          "reiniciar vuelve al formulario");
    CHECK(!estado.hayMensaje(), "reiniciar descarta el mensaje anterior");

    EstadoExportacion conExito;
    conExito.iniciar();
    conExito.finalizar(true, "Exportacion completada");
    CHECK(conExito.resultado() == EstadoExportacion::Resultado::Exito,
          "el exito se marca como exito");
    CHECK(conExito.progreso() == 1.0f, "con exito el progreso llega a uno");
    CHECK(conExito.mensaje() == "Exportacion completada",
          "el mensaje de exito se conserva");
}

void comprobarDialogo() {
    // Sin proyecto no hay nada que exportar: el dialogo tiene que decirlo, no
    // quedarse quieto como si el boton no hubiera reaccionado.
    ExportDialog sinProyecto("");
    sinProyecto.iniciarExportacion();
    CHECK(sinProyecto.vista() == ExportDialog::Vista::Resultado,
          "sin proyecto el dialogo muestra el resultado, no el formulario");
    CHECK(sinProyecto.estado().resultado() ==
              EstadoExportacion::Resultado::Error,
          "sin proyecto el resultado es un error");
    CHECK(sinProyecto.estado().hayMensaje(),
          "sin proyecto se conserva el motivo");

    // Un proyecto cuyo folder no existe tampoco se puede exportar.
    ExportDialog sinCarpeta("ProyectoQueNoExiste");
    sinCarpeta.iniciarExportacion();
    CHECK(sinCarpeta.vista() == ExportDialog::Vista::Resultado,
          "una carpeta inexistente deja el resultado a la vista");
    CHECK(sinCarpeta.estado().hayMensaje(),
          "una carpeta inexistente conserva el motivo");
}

void comprobarBarraDeMenu() {
    // La barra de menu refleja el proyecto activo en el dialogo: sin esto la
    // exportacion no sabe que proyecto exportar.
    SceneMenuBarInterface barra(true);
    barra.setProyectoActual("MiProyecto");
    barra.abrirDialogoExportacion();
    CHECK(barra.dialogoExportacion() != nullptr, "el dialogo se abre");
    CHECK(barra.dialogoExportacion()->proyecto() == "MiProyecto",
          "el dialogo recibe el proyecto activo");
    CHECK(barra.proyectoActual() == "MiProyecto",
          "la barra recuerda el proyecto activo");

    // Cambiar de proyecto con el dialogo ya abierto tambien lo actualiza.
    barra.setProyectoActual("OtroProyecto");
    CHECK(barra.dialogoExportacion()->proyecto() == "OtroProyecto",
          "el dialogo abierto sigue al proyecto activo");

    // Sin proyecto, el dialogo lo tiene vacio (y lo dira al exportar).
    SceneMenuBarInterface sinProyecto(true);
    sinProyecto.abrirDialogoExportacion();
    CHECK(sinProyecto.dialogoExportacion()->proyecto().empty(),
          "sin proyecto el dialogo lo tiene vacio");
}

// --- Dependencias del juego exportado --------------------------------------
//
// El juego exportado tiene que llevar las DLL de las que depende: si no,
// Windows las pide al arrancar. El exportador las saca de la tabla de
// importaciones del ejecutable (un PE) y las copia desde la carpeta del
// toolchain.

// Construye un PE minimo (PE32) con una tabla de importaciones de una DLL,
// para ejercitar el lector sin depender de un binario del sistema.
std::vector<unsigned char> construirPE(const std::string& nombreDll) {
    std::vector<unsigned char> datos(0x400, 0);
    auto escribirU16 = [&datos](std::size_t off, std::uint16_t valor) {
        datos[off] = static_cast<unsigned char>(valor & 0xFF);
        datos[off + 1] = static_cast<unsigned char>((valor >> 8) & 0xFF);
    };
    auto escribirU32 = [&datos](std::size_t off, std::uint32_t valor) {
        for (int i = 0; i < 4; ++i) {
            datos[off + i] =
                static_cast<unsigned char>((valor >> (8 * i)) & 0xFF);
        }
    };

    // Cabecera DOS: firma "MZ" y el offset de la cabecera PE.
    datos[0] = 'M';
    datos[1] = 'Z';
    escribirU32(0x3C, 0x40);

    // Firma PE y cabecera COFF.
    const std::size_t pe = 0x40;
    datos[pe] = 'P';
    datos[pe + 1] = 'E';
    const std::size_t coff = pe + 4;
    escribirU16(coff + 0, 0x8664);  // Machine
    escribirU16(coff + 2, 1);       // NumberOfSections
    escribirU16(coff + 16, 0xF0);   // SizeOfOptionalHeader

    // Cabecera opcional (PE32): el magic y los directorios de datos.
    const std::size_t opcional = coff + 20;
    escribirU16(opcional, 0x10B);
    const std::size_t directorios = opcional + 96;
    // Directorio #1 (importaciones): RVA 0x1000, dentro de la seccion.
    escribirU32(directorios + 8, 0x1000);
    escribirU32(directorios + 12, 40);

    // Una seccion: RVA 0x1000 -> offset de archivo 0x200.
    const std::size_t secciones = opcional + 0xF0;
    escribirU32(secciones + 8, 0x200);    // VirtualSize
    escribirU32(secciones + 12, 0x1000);  // VirtualAddress
    escribirU32(secciones + 16, 0x200);   // SizeOfRawData
    escribirU32(secciones + 20, 0x200);   // PointerToRawData

    // Descriptor de importacion: el nombre de la DLL esta en el RVA 0x1010.
    const std::size_t descriptores = 0x200;
    escribirU32(descriptores + 12, 0x1010);
    escribirU32(descriptores + 16, 0x1020);  // FirstThunk

    std::size_t off = 0x210;
    for (char c : nombreDll) {
        datos[off++] = static_cast<unsigned char>(c);
    }
    datos[off] = 0;
    return datos;
}

void comprobarDependencias() {
    // El lector de importaciones encuentra la DLL que pide el ejecutable.
    const std::vector<unsigned char> pe = construirPE("KERNEL32.dll");
    const std::vector<std::string> dlls = ImportacionesPE::leer(pe);
    CHECK(dlls.size() == 1 && dlls[0] == "KERNEL32.dll",
          "el lector de importaciones encuentra la DLL que pide el binario");

    // Y tambien desde el archivo (el camino real del exportador).
    {
        CarpetaTemporal carpeta("pe");
        const std::string ruta = (carpeta.ruta / "juego.exe").string();
        {
            std::ofstream archivo(ruta, std::ios::binary);
            const std::vector<unsigned char>& datos = pe;
            archivo.write(reinterpret_cast<const char*>(datos.data()),
                          static_cast<std::streamsize>(datos.size()));
        }
        const std::vector<std::string> desdeArchivo =
            ImportacionesPE::leer(ruta);
        CHECK(desdeArchivo.size() == 1 && desdeArchivo[0] == "KERNEL32.dll",
              "el lector de importaciones funciona sobre el archivo");
    }

    // Un archivo que no es PE no da ninguna DLL (ni error).
    CHECK(ImportacionesPE::leer(std::vector<unsigned char>{'h', 'o', 'l', 'a'})
                  .empty(),
          "un archivo que no es PE no aporta importaciones");
    CHECK(ImportacionesPE::leer(std::string("no/existe/ejecutable.exe"))
              .empty(),
          "un ejecutable inexistente no aporta importaciones");

    // Las DLL del sistema no se copian: Windows ya las trae.
    CHECK(GameExporter::esDllDelSistema("KERNEL32.dll"),
          "kernel32 es una DLL del sistema");
    CHECK(GameExporter::esDllDelSistema("api-ms-win-crt-runtime-l1-1-0.dll"),
          "los conjuntos de API son DLL del sistema");
    CHECK(!GameExporter::esDllDelSistema("libstdc++-6.dll"),
          "el runtime de C++ no es una DLL del sistema");

    // La busqueda de la DLL recorre las carpetas: si esta en una, la encuentra.
    {
        CarpetaTemporal carpeta("dlls");
        const std::string dll = "libstdc++-6.dll";
        {
            std::ofstream archivo((carpeta.ruta / dll).string());
            archivo << "stub";
        }
        const std::vector<std::string> directorios = {carpeta.ruta.string()};
        CHECK(!GameExporter::buscarDependencia(directorios, dll).empty(),
              "la DLL se encuentra en una de las carpetas");
        CHECK(GameExporter::buscarDependencia(directorios, "no-existe.dll")
                  .empty(),
              "una DLL que no esta no se encuentra");
    }

    // Las carpetas donde se buscan las DLL incluyen la del compilador (el
    // runtime y las bibliotecas de terceros viven ahi) y las del build.
    {
        const std::vector<std::string> directorios =
            GameExporter::directoriosDeDependencias("/tmp/build", true);
        CHECK(!directorios.empty(), "hay carpetas donde buscar las DLL");
        bool tieneBuild = false;
        for (const std::string& directorio : directorios) {
            if (directorio.find("build") != std::string::npos) tieneBuild = true;
        }
        CHECK(tieneBuild, "la carpeta del build esta entre las buscadas");
    }

    // El cierre de dependencias es transitivo: si el ejecutable importa una
    // DLL y esa importa otra, las dos tienen que ir con el juego (assimp
    // arrastra a zlib y a minizip).
    {
        CarpetaTemporal carpeta("cierre");
        auto escribirPE = [&carpeta](const std::string& nombre,
                                     const std::string& dllImportada) {
            const std::vector<unsigned char> pe = construirPE(dllImportada);
            std::ofstream archivo((carpeta.ruta / nombre).string(),
                                  std::ios::binary);
            archivo.write(reinterpret_cast<const char*>(pe.data()),
                          static_cast<std::streamsize>(pe.size()));
        };
        escribirPE("juego.exe", "primera.dll");
        escribirPE("primera.dll", "segunda.dll");
        escribirPE("segunda.dll", "tercera.dll");

        const auto buscador = [&carpeta](const std::string& dll) -> std::string {
            const std::string ruta = (carpeta.ruta / dll).string();
            return std::filesystem::exists(ruta) ? ruta : std::string();
        };
        const std::vector<std::pair<std::string, std::string>> cierre =
            GameExporter::cierreDeDependencias(
                (carpeta.ruta / "juego.exe").string(), buscador);

        bool primera = false, segunda = false, tercera = false;
        for (const auto& par : cierre) {
            if (par.first == "primera.dll") primera = true;
            if (par.first == "segunda.dll") segunda = true;
            if (par.first == "tercera.dll") tercera = true;
        }
        CHECK(cierre.size() == 3,
              "el cierre incluye las DLL de segundo y tercer nivel");
        CHECK(primera && segunda && tercera,
              "el cierre trae la DLL de primer nivel y las que arrastra");
    }
}

static void comprobarLogExportacion() {
    // La exportacion deja su traza en un archivo propio (la salida de la
    // compilacion): el usuario tiene que poder abrirlo y leerlo.
    const std::string ruta = GameExporter::rutaLogExportacion();
    CHECK(!ruta.empty(), "se encuentra una carpeta donde dejar la traza");
    CHECK(std::filesystem::exists(ruta),
          "la traza de la exportacion es un archivo que existe");
    const std::filesystem::path carpeta = std::filesystem::path(ruta).parent_path();
    CHECK(std::filesystem::exists(carpeta),
          "la traza vive en una carpeta que existe");
    // Dos exportaciones seguidas no comparten archivo: la segunda no pisa lo
    // que escribio la primera.
    const std::string otra = GameExporter::rutaLogExportacion();
    CHECK(!otra.empty() && otra != ruta,
          "una segunda exportacion no reutiliza el archivo de la primera");
    // Los archivos reservados se borran: la prueba no deja rastro en la
    // carpeta de logs del motor.
    std::error_code ec;
    std::filesystem::remove(ruta, ec);
    std::filesystem::remove(otra, ec);
}

static void comprobarDirectorioBuild() {
    // Cada exportacion compila en su propio directorio: dos exportaciones
    // simultaneas (o una que se queda a medias) no pueden vaciar el
    // directorio de la otra.
    const std::string uno = GameExporter::directorioBuildTemporal();
    const std::string otro = GameExporter::directorioBuildTemporal();
    CHECK(!uno.empty() && !otro.empty(),
          "se puede elegir un directorio de compilacion");
    CHECK(uno != otro,
          "dos exportaciones no comparten el directorio de compilacion");
    CHECK(std::filesystem::path(uno).filename().string().rfind(
              "funshi_export_", 0) == 0,
          "el directorio de compilacion es propio de la exportacion");
    CHECK(std::filesystem::exists(uno),
          "el directorio de compilacion queda reservado para esta exportacion");
    // La prueba no deja directorios sueltos en la carpeta temporal.
    std::error_code ec;
    std::filesystem::remove_all(uno, ec);
    std::filesystem::remove_all(otro, ec);
}

int main() {
    comprobarPrecondiciones();
    comprobarEstado();
    comprobarDialogo();
    comprobarBarraDeMenu();
    comprobarDependencias();
    comprobarLogExportacion();
    comprobarDirectorioBuild();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
