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
// Pruebas headless de EditorConfig (la configuracion del editor en JSON):
// tolerancia ante archivo ausente/corrupto/parcial y round-trip escrito-leido.
// Sin pila grafica: solo std C++17 + nlohmann/json del intermedio.
//
// Separacion de archivos (nueva arquitectura):
//   - Configuracion.json (general): apariencia, idioma, sensibilidad, ultimo proyecto.
//   - ConfiguracionProyecto.json (por proyecto): ventanas, gizmo, camara activa.
// Los metodos guardarGeneral/cargarGeneral y guardarProyecto/cargarProyecto
// permiten escribir/leer cada archivo por separado.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Configuracion/ProjectPaths.h"
#include "../FunshiEngineGL/src/Configuracion/ProyectoInicial.h"
#include "../FunshiEngineGL/src/Configuracion/Apariencia.h"
#include "../FunshiEngineGL/src/Configuracion/RutasLog.h"

namespace fs = std::filesystem;

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"  \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)
} // namespace

int main() {
    // Carpeta temporal unica por proceso: crea y se limpia al salir (RAII).
    TempPruebas::CarpetaPrueba carpetaBase("funshi_editorconfig_tests");
    const fs::path base = carpetaBase.ruta();
    const std::string rutaGeneral  = (base / "Configuracion.json").string();
    const std::string proyNombreTest = "TestProyecto";
    const std::string rutaProyecto = (base / "ConfiguracionProyecto.json").string();

    // 1. Sin archivo: todo default, sin crashear.
    {
        EditorConfig cfg;
        cfg.cargarGeneral(rutaGeneral);
        cfg.cargarProyecto(proyNombreTest, rutaProyecto);
        CHECK(cfg.datos().nombreProyecto.empty(),
              "default nombreProyecto (sin proyecto abierto)");
        CHECK(cfg.datos().idioma == "Espanol", "default idioma");
        CHECK(cfg.datos().sensibilidadCamara == 0.15f, "default sensibilidad");
        CHECK(cfg.datos().sensibilidadMovimientoCamara == 1.0f,
              "default sensibilidad de movimiento");
        CHECK(cfg.datos().ventanaCamarasAbierta == true, "default ventanaCamaras");
        CHECK(cfg.datos().gizmoOperacion == 7, "default gizmoOperacion");
        CHECK(cfg.datos().gizmoGlobal == false, "default gizmo LOCAL");
        CHECK(cfg.datos().camaraActivaId == -1, "default camaraActivaId");
        CHECK(cfg.datos().estadoVentanas.empty(), "default sin ventanas");
        CHECK(cfg.datos().apariencia.temaClaro == false, "default tema oscuro");
        CHECK(cfg.datos().apariencia.blancoYNegro == false, "default no B/N");
        CHECK(cfg.datos().apariencia.acento[3] == 1.0f, "default acento opaco");
        CHECK(cfg.datos().apariencia.radioDifuminado ==
                  AparienciaUtil::kRadioDifuminadoPorDefecto,
              "default radio de difuminado");
        // Defaults de colores del cielo (gris oscuro historico)
        CHECK(cfg.datos().apariencia.fondoSuperior[0] == 0.10f &&
                  cfg.datos().apariencia.fondoSuperior[1] == 0.10f &&
                  cfg.datos().apariencia.fondoSuperior[2] == 0.10f,
              "default fondoSuperior = 0.1, 0.1, 0.1");
        CHECK(cfg.datos().apariencia.fondoInferior[0] == 0.10f &&
                  cfg.datos().apariencia.fondoInferior[1] == 0.10f &&
                  cfg.datos().apariencia.fondoInferior[2] == 0.10f,
              "default fondoInferior = 0.1, 0.1, 0.1");
    }

    // 1b. Persistencia del proyecto activo: sin "ultimoProyecto" no se abre
    //     ningun proyecto y el nombre queda vacio; asi guardarGeneral no
    //     materializa un proyecto fantasma ("Nuevo Proyecto").
    {
        // JSON general existente pero sin ultimoProyecto: el proyecto resuelto
        // es ninguno (vacio), no el default del struct.
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << R"({"version": 2, "idioma": "Espanol"})";
        }
        EditorConfig cfg;
        cfg.cargarGeneral(rutaGeneral);
        CHECK(cfg.datos().nombreProyecto.empty(),
              "config sin ultimoProyecto -> sin proyecto abierto");

        // Con el nombre vacio, guardarGeneral no debe escribir la clave.
        cfg.guardarGeneral(rutaGeneral);
        std::ifstream entrada(rutaGeneral);
        const std::string contenido((std::istreambuf_iterator<char>(entrada)),
                                    std::istreambuf_iterator<char>());
        CHECK(contenido.find("ultimoProyecto") == std::string::npos,
              "nombre vacio: guardarGeneral no escribe ultimoProyecto");
    }

    // 1c. ProyectoInicial::resolver: politica de arranque sin disco ni UI.
    {
        using ProyectoInicial::resolver;
        const auto conConfig = resolver("MiEscena", "", true);
        CHECK(conConfig.nombre == "MiEscena", "resolver: retoma el persistido");
        CHECK(!conConfig.primerArranque,
              "resolver: con config no es primer arranque");

        const auto sinUltimo = resolver("", "", true);
        CHECK(sinUltimo.nombre.empty(),
              "resolver: config sin ultimoProyecto -> sin proyecto");
        CHECK(!sinUltimo.primerArranque,
              "resolver: la config existe aunque falte el nombre");

        const auto primer = resolver("", "", false);
        CHECK(primer.nombre.empty(), "resolver: primer arranque sin proyecto");
        CHECK(primer.primerArranque, "resolver: sin config es primer arranque");

        const auto conCLI = resolver("", "ProyectoCLI", false);
        CHECK(conCLI.nombre == "ProyectoCLI",
              "resolver: --proyecto tiene prioridad");
        CHECK(!conCLI.primerArranque,
              "resolver: --proyecto entra directo al editor");
    }

    // 1d. Colores del cielo: se guardan y se leen tal cual quedaron, incluidos
    // los claros (un cielo de tonos altos es una eleccion valida del usuario).
    // Solo se acotan los valores que no pueden salir del selector y romperian
    // el degradado: componentes fuera de [0, 1].
    {
        // Par claro y casi igual: antes se reseteaba al default 0.1 y el cielo
        // perdia el color elegido (y la division, si los dos extremos caian).
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << R"({"version": 1, "apariencia": {"blancoYNegro": false, "temaClaro": false, "fondoSuperior": [0.95, 0.94, 0.94], "fondoInferior": [0.95, 0.94, 0.94]}})";
        }
        EditorConfig cfgClaro;
        cfgClaro.cargarGeneral(rutaGeneral);
        CHECK(cfgClaro.datos().apariencia.fondoSuperior[0] == 0.95f &&
                  cfgClaro.datos().apariencia.fondoSuperior[1] == 0.94f &&
                  cfgClaro.datos().apariencia.fondoSuperior[2] == 0.94f,
              "cielo claro: fondoSuperior se conserva tal cual");
        CHECK(cfgClaro.datos().apariencia.fondoInferior[0] == 0.95f &&
                  cfgClaro.datos().apariencia.fondoInferior[1] == 0.94f &&
                  cfgClaro.datos().apariencia.fondoInferior[2] == 0.94f,
              "cielo claro: fondoInferior se conserva tal cual");

        // Dos tonos claros distintos: la division de colores sobrevive a la carga.
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << R"({"version": 1, "apariencia": {"fondoSuperior": [0.9, 0.9, 0.95], "fondoInferior": [0.85, 0.8, 0.8]}})";
        }
        EditorConfig cfgClaroDistinto;
        cfgClaroDistinto.cargarGeneral(rutaGeneral);
        CHECK(cfgClaroDistinto.datos().apariencia.fondoSuperior[2] == 0.95f &&
                  cfgClaroDistinto.datos().apariencia.fondoInferior[0] == 0.85f,
              "cielo claro distinto arriba/abajo: la division se conserva");

        // Fuera de [0, 1]: se acota al extremo valido en vez de descartar el color.
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << R"({"version": 1, "apariencia": {"fondoSuperior": [5.0, 1.5, -2.0], "fondoInferior": [-0.5, 9.0, 12.0]}})";
        }
        EditorConfig cfgFueraDeRango;
        cfgFueraDeRango.cargarGeneral(rutaGeneral);
        CHECK(cfgFueraDeRango.datos().apariencia.fondoSuperior[0] == 1.0f &&
                  cfgFueraDeRango.datos().apariencia.fondoSuperior[1] == 1.0f &&
                  cfgFueraDeRango.datos().apariencia.fondoSuperior[2] == 0.0f,
              "cielo fuera de rango: fondoSuperior acotado a [0, 1]");
        CHECK(cfgFueraDeRango.datos().apariencia.fondoInferior[0] == 0.0f &&
                  cfgFueraDeRango.datos().apariencia.fondoInferior[1] == 1.0f &&
                  cfgFueraDeRango.datos().apariencia.fondoInferior[2] == 1.0f,
              "cielo fuera de rango: fondoInferior acotado a [0, 1]");
    }

    // 2. Round-trip: los valores cambiados sobreviven a guardar/cargar.
    //    Config general y config de proyecto se guardan en archivos separados.
    {
        EditorConfig cfg;
        cfg.datos().nombreProyecto = "MiEscena";
        cfg.datos().idioma = "English";
        cfg.datos().sensibilidadCamara = 2.5f;
        cfg.datos().sensibilidadMovimientoCamara = 1.8f;
        cfg.datos().ventanaCamarasAbierta = false;
        cfg.datos().gizmoOperacion = 2;
        cfg.datos().gizmoGlobal = true;
        cfg.datos().camaraActivaId = 7;
        cfg.datos().estadoVentanas["BrowseFile"] = false;
        cfg.datos().estadoVentanas["ShowFolder"] = true;
        cfg.datos().apariencia.temaClaro = true;
        cfg.datos().apariencia.blancoYNegro = true;
        cfg.datos().apariencia.acento[0] = 0.9f;
        cfg.datos().apariencia.acento[1] = 0.1f;
        cfg.datos().apariencia.acento[2] = 0.2f;
        cfg.datos().apariencia.acento[3] = 0.5f;
        cfg.datos().apariencia.fondoSuperior[0] = 0.3f;
        cfg.datos().apariencia.fondoSuperior[1] = 0.4f;
        cfg.datos().apariencia.fondoSuperior[2] = 0.5f;
        cfg.datos().apariencia.fondoInferior[0] = 0.3f;
        cfg.datos().apariencia.fondoInferior[1] = 0.4f;
        cfg.datos().apariencia.fondoInferior[2] = 0.5f;
        cfg.datos().apariencia.radioDifuminado = 275.0f;
        // Guardar en dos archivos separados (nuevo flujo)
        cfg.guardarGeneral(rutaGeneral);
        cfg.guardarProyecto(proyNombreTest, rutaProyecto);
        CHECK(fs::exists(rutaGeneral),  "se escribio el archivo general");
        CHECK(fs::exists(rutaProyecto), "se escribio el archivo de proyecto");

        EditorConfig cfg2;
        cfg2.cargarGeneral(rutaGeneral);
        cfg2.cargarProyecto(proyNombreTest, rutaProyecto);
        CHECK(cfg2.datos().nombreProyecto == "MiEscena",  "roundtrip nombreProyecto");
        CHECK(cfg2.datos().idioma == "English",           "roundtrip idioma");
        CHECK(cfg2.datos().sensibilidadCamara == 2.5f,    "roundtrip sensibilidad");
        CHECK(cfg2.datos().sensibilidadMovimientoCamara == 1.8f,
              "roundtrip sensibilidad de movimiento");
        CHECK(cfg2.datos().ventanaCamarasAbierta == false,"roundtrip ventanaCamaras");
        CHECK(cfg2.datos().gizmoOperacion == 2,           "roundtrip gizmoOperacion");
        CHECK(cfg2.datos().gizmoGlobal == true,      "roundtrip gizmo GLOBAL");
        CHECK(cfg2.datos().camaraActivaId == 7,           "roundtrip camaraActivaId");
        CHECK(cfg2.datos().estadoVentanas.at("BrowseFile") == false,
              "roundtrip ventana BrowseFile");
        CHECK(cfg2.datos().estadoVentanas.at("ShowFolder") == true,
              "roundtrip ventana ShowFolder");
        CHECK(cfg2.datos().estadoVentanas.size() == 2,   "cantidad de ventanas");
        CHECK(cfg2.datos().apariencia.temaClaro == true,  "roundtrip temaClaro");
        CHECK(cfg2.datos().apariencia.blancoYNegro == true,"roundtrip blancoYNegro");
        CHECK(cfg2.datos().apariencia.acento[0] == 0.9f, "roundtrip acento r");
        CHECK(cfg2.datos().apariencia.acento[3] == 0.5f, "roundtrip acento a");
        CHECK(cfg2.datos().apariencia.fondoSuperior[2] == 0.5f,
              "roundtrip fondoSuperior b");
        CHECK(cfg2.datos().apariencia.fondoInferior[2] == 0.5f,
              "roundtrip fondoInferior b");
        CHECK(cfg2.datos().apariencia.radioDifuminado == 275.0f,
              "roundtrip radio de difuminado");
        CHECK(cfg2.datos().apariencia == cfg.datos().apariencia,
              "roundtrip Apariencia completa");
    }

    // 3. Archivo corrupto: defaults (sin crash).
    {
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{ json roto";
        }
        EditorConfig cfg;
        cfg.cargarGeneral(rutaGeneral);
        CHECK(cfg.datos().nombreProyecto.empty(), "corrupto -> defaults");
        CHECK(cfg.datos().idioma == "Espanol", "corrupto -> defaults idioma");
    }

    // 4. Parcial: el campo presente se aplica, el ausente conserva el default.
    //    El campo "editor" (ventanaCamarasAbierta) vive en el archivo de proyecto.
    {
        {
            std::ofstream f(rutaProyecto, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"editor\": {\n"
                 "    \"ventanaCamarasAbierta\": false\n  }\n}\n";
        }
        EditorConfig cfg;
        cfg.cargarProyecto(proyNombreTest, rutaProyecto);
        CHECK(cfg.datos().ventanaCamarasAbierta == false,
              "parcial: campo presente se aplica");
        CHECK(cfg.datos().sensibilidadCamara == 0.15f,
              "parcial: campo ausente conserva default");
        CHECK(cfg.datos().nombreProyecto.empty(),
              "parcial: sin seccion general -> proyecto no abierto");
    }

    // 4b. Radio de difuminado: un archivo sin el campo conserva el default (no
    //     falla al cargar una configuracion anterior a la opcion) y uno con un
    //     valor fuera del rango admitido lo acota, para que el perfil en memoria
    //     sea siempre valido aunque el archivo se haya editado a mano.
    {
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"temaClaro\": true}\n}\n";
        }
        EditorConfig cfg;
        cfg.cargarGeneral(rutaGeneral);
        CHECK(cfg.datos().apariencia.radioDifuminado ==
                  AparienciaUtil::kRadioDifuminadoPorDefecto,
              "radio ausente -> default");

        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": "
                 "{\"radioDifuminado\": 99999}\n}\n";
        }
        EditorConfig cfgAlto;
        cfgAlto.cargarGeneral(rutaGeneral);
        CHECK(cfgAlto.datos().apariencia.radioDifuminado ==
                  AparienciaUtil::kRadioDifuminadoMaximo,
              "radio enorme -> acotado al maximo");

        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": "
                 "{\"radioDifuminado\": -3}\n}\n";
        }
        EditorConfig cfgBajo;
        cfgBajo.cargarGeneral(rutaGeneral);
CHECK(cfgBajo.datos().apariencia.radioDifuminado ==
              AparienciaUtil::kRadioDifuminadoMinimo,
              "radio negativo -> acotado al minimo");
    }

    // 4c. Colores del cielo (fondoSuperior/fondoInferior): compatibilidad con
    //     "fondo" antiguo (se copia a ambos), modo B/N fuerza ambos a
    //     blanco/negro segun el tema, y round-trip de colores personalizados.
    {
        // Compatibilidad: archivo viejo con "fondo" -> se copia a superior e inferior.
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"fondo\": [0.1, 0.2, 0.3]}\n}\n";
        }
        EditorConfig cfgCompat;
        cfgCompat.cargarGeneral(rutaGeneral);
        CHECK(cfgCompat.datos().apariencia.fondoSuperior[0] == 0.1f &&
                  cfgCompat.datos().apariencia.fondoSuperior[1] == 0.2f &&
                  cfgCompat.datos().apariencia.fondoSuperior[2] == 0.3f,
              "compat: fondo -> fondoSuperior");
        CHECK(cfgCompat.datos().apariencia.fondoInferior[0] == 0.1f &&
                  cfgCompat.datos().apariencia.fondoInferior[1] == 0.2f &&
                  cfgCompat.datos().apariencia.fondoInferior[2] == 0.3f,
              "compat: fondo -> fondoInferior");

        // Modo B/N oscuro: ambos negros (en color efectivo).
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"blancoYNegro\": true, \"temaClaro\": false,"
                 "\"fondoSuperior\": [0.8, 0.8, 0.8],"
                 "\"fondoInferior\": [0.2, 0.2, 0.2]}\n}\n";
        }
        EditorConfig cfgBNOscuro;
        cfgBNOscuro.cargarGeneral(rutaGeneral);
        float sup[3], inf[3];
        AparienciaUtil::fondoEfectivoSuperior(cfgBNOscuro.datos().apariencia, sup);
        AparienciaUtil::fondoEfectivoInferior(cfgBNOscuro.datos().apariencia, inf);
        CHECK(sup[0] == 0.0f && inf[0] == 0.0f,
              "B/N oscuro: color efectivo superior/inferior negros");

        // Modo B/N claro: ambos blancos (en color efectivo).
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"blancoYNegro\": true, \"temaClaro\": true,"
                 "\"fondoSuperior\": [0.1, 0.1, 0.1],"
                 "\"fondoInferior\": [0.5, 0.5, 0.5]}\n}\n";
        }
        EditorConfig cfgBNClaro;
        cfgBNClaro.cargarGeneral(rutaGeneral);
        AparienciaUtil::fondoEfectivoSuperior(cfgBNClaro.datos().apariencia, sup);
        AparienciaUtil::fondoEfectivoInferior(cfgBNClaro.datos().apariencia, inf);
        CHECK(sup[0] == 1.0f && inf[0] == 1.0f,
              "B/N claro: color efectivo superior/inferior blancos");

        // Colores personalizados round-trip (modo normal, no B/N).
        {
            EditorConfig cfgColor;
            cfgColor.datos().apariencia.fondoSuperior[0] = 0.2f;
            cfgColor.datos().apariencia.fondoSuperior[1] = 0.3f;
            cfgColor.datos().apariencia.fondoSuperior[2] = 0.8f;
            cfgColor.datos().apariencia.fondoInferior[0] = 0.8f;
            cfgColor.datos().apariencia.fondoInferior[1] = 0.4f;
            cfgColor.datos().apariencia.fondoInferior[2] = 0.2f;
            cfgColor.guardarGeneral(rutaGeneral);

            EditorConfig cfgColor2;
            cfgColor2.cargarGeneral(rutaGeneral);
            CHECK(cfgColor2.datos().apariencia.fondoSuperior[0] == 0.2f &&
                      cfgColor2.datos().apariencia.fondoSuperior[1] == 0.3f &&
                      cfgColor2.datos().apariencia.fondoSuperior[2] == 0.8f,
                  "roundtrip fondoSuperior personalizado");
            CHECK(cfgColor2.datos().apariencia.fondoInferior[0] == 0.8f &&
                      cfgColor2.datos().apariencia.fondoInferior[1] == 0.4f &&
                      cfgColor2.datos().apariencia.fondoInferior[2] == 0.2f,
                  "roundtrip fondoInferior personalizado");
        }
    }

    // 5. Nuevo sistema de guardado por proyecto:
    // Cada proyecto genera su carpeta en MotorGrafico, con Memory (escena, config, imgui)
    // y su hermano srcProyectName como raiz del explorador de archivos.
    // La config general vive en la raiz de MotorGrafico (hermana de los proyectos).
    {
        const std::string baseMotor = EditorConfig::directorioBaseMotorGrafico();
        CHECK(!baseMotor.empty(), "directorioBaseMotorGrafico no vacio");

        const std::string proyNombre = "JuegoPrueba";
        const std::string proyDir  = EditorConfig::directorioProyecto(proyNombre);
        const std::string memDir   = EditorConfig::directorioMemory(proyNombre);
        const std::string srcDir   = EditorConfig::directorioSrc(proyNombre);
        const std::string rootName = EditorConfig::nombreRaizSrc(proyNombre);

        CHECK(rootName == "srcJuegoPrueba",              "nombreRaizSrc correcto");
        CHECK(proyDir == baseMotor + "/Proyects/" + proyNombre,   "directorioProyecto en Proyects/");
        CHECK(memDir  == proyDir + "/Memory",            "directorioMemory dentro del proyecto");
        CHECK(srcDir  == proyDir + "/srcJuegoPrueba",    "directorioSrc hermano de Memory");

        // Config general: en <base>/Configuraciones/Configuracion.json
        CHECK(EditorConfig::rutaConfiguracionGeneral() == baseMotor + "/Configuraciones/Configuracion.json",
              "rutaConfiguracionGeneral en Configuraciones/");
        // Config del proyecto: dentro de Memory del proyecto.
        CHECK(EditorConfig::rutaConfiguracionProyecto(proyNombre) ==
              memDir + "/ConfiguracionProyecto.json",
              "rutaConfiguracionProyecto dentro de Memory");

        CHECK(EditorConfig::rutaSceneBBDD(proyNombre) ==
              memDir + "/Binarios/SceneBBDDObjetos.txt",
              "rutaSceneBBDD dentro de Memory/Binarios");
        CHECK(EditorConfig::rutaSceneDir(proyNombre) == memDir + "/Binarios/Scene/",
              "rutaSceneDir dentro de Memory/Binarios/Scene/");
        CHECK(EditorConfig::rutaImguiIni(proyNombre) == memDir + "/imgui.ini",
              "rutaImguiIni dentro de Memory");

        // Asegurar que la creacion de estructura crea las carpetas en disco
        EditorConfig::asegurarEstructuraProyecto(proyNombre);
        CHECK(fs::is_directory(memDir + "/Binarios/Scene"),
              "asegurarEstructuraProyecto creo Memory/Binarios/Scene");
        CHECK(fs::is_directory(srcDir),
              "asegurarEstructuraProyecto creo srcJuegoPrueba");
        // Sonidos es un asset: el usuario decide cuando crearlo (no auto-creado).
        const std::string sonidosDir = EditorConfig::directorioSonidos(proyNombre);
        CHECK(sonidosDir == srcDir + "/Sonidos",
              "directorioSonidos dentro de src<proyecto>");
        // El directorio NO se crea automaticamente: lo decide el usuario.

        // Limpieza de prueba
        std::error_code ec;
        fs::remove_all(proyDir, ec);
    }

    // 5b. Migracion de Sonidos: si el usuario habia creado la carpeta en la raiz,
    //     se mueve al src conservando sus clips.
    {
        const std::string proyNombre = "JuegoMigracion";
        const std::string proyDir = EditorConfig::directorioProyecto(proyNombre);
        const std::string sonidosViejo = proyDir + "/Sonidos";
        std::error_code ec;
        fs::create_directories(sonidosViejo, ec);
        { std::ofstream out(sonidosViejo + "/tema.wav"); out << "clip"; }

        EditorConfig::asegurarEstructuraProyecto(proyNombre);

        const std::string sonidosNuevo =
            EditorConfig::directorioSonidos(proyNombre);
        CHECK(!fs::exists(sonidosViejo),
              "migracion retira Sonidos de la raiz del proyecto");
        CHECK(fs::is_directory(sonidosNuevo),
              "migracion crea Sonidos dentro del src");
        CHECK(fs::exists(sonidosNuevo + "/tema.wav"),
              "migracion conserva los clips de audio");

        fs::remove_all(proyDir, ec);
    }

    // 5c. Renombre fisico de proyecto: mueve la carpeta raiz y la raiz src
    //     (src<viejo> -> src<nuevo>); no hace nada si falta el origen o si el
    //     destino ya existe (main conmuta en ese caso).
    {
        const std::string viejo = "JuegoRenombrado";
        const std::string nuevo = "JuegoRenombradoV2";
        const std::string dirViejo = EditorConfig::directorioProyecto(viejo);
        const std::string dirNuevo = EditorConfig::directorioProyecto(nuevo);
        std::error_code ec;
        EditorConfig::asegurarEstructuraProyecto(viejo);
        CHECK(EditorConfig::renombrarProyecto("", nuevo) == false,
              "renombrar con origen vacio falla");
        CHECK(EditorConfig::renombrarProyecto(viejo, viejo) == false,
              "renombrar al mismo nombre falla");
        CHECK(EditorConfig::renombrarProyecto(viejo, nuevo),
              "renombrar mueve la carpeta del proyecto");
        CHECK(!fs::exists(dirViejo), "la carpeta vieja desaparece");
        CHECK(fs::is_directory(dirNuevo), "la carpeta nueva existe");
        CHECK(fs::is_directory(dirNuevo + "/src" + nuevo),
              "la raiz src tambien cambia de nombre");
        CHECK(!fs::exists(dirNuevo + "/src" + viejo),
              "la raiz src vieja no queda como fantasma");
        CHECK(EditorConfig::renombrarProyecto(viejo, nuevo) == false,
              "sin origen ya no se puede renombrar de nuevo");
        fs::remove_all(dirNuevo, ec);
    }

    // 5d. Eliminacion de proyecto: borra la carpeta completa (escena, src,
    //     config). No toca nada si el proyecto no existe o el nombre es vacio.
    {
        const std::string nombre = "JuegoAEliminar";
        const std::string dirProyecto = EditorConfig::directorioProyecto(nombre);
        std::error_code ec;
        EditorConfig::asegurarEstructuraProyecto(nombre);
        CHECK(EditorConfig::eliminarProyecto("") == false,
              "eliminar con nombre vacio falla");
        CHECK(EditorConfig::eliminarProyecto("Inexistente") == false,
              "eliminar un proyecto inexistente falla");
        CHECK(fs::is_directory(dirProyecto), "el proyecto existe antes de eliminar");
        CHECK(EditorConfig::eliminarProyecto(nombre),
              "eliminar retira la carpeta del proyecto");
        CHECK(!fs::exists(dirProyecto), "la carpeta del proyecto desaparece");
        CHECK(EditorConfig::eliminarProyecto(nombre) == false,
              "una vez eliminado ya no se puede eliminar de nuevo");
    }

    // 6a. Contexto de rutas de la serializacion portable: sin raiz fijada las
    //     rutas pasan tal cual (passthrough, igual que antes del cambio).
    {
        EditorConfig::limpiarRaizAssets();
        CHECK(EditorConfig::hayRaizAssets() == false,
              "sin proyecto no hay raiz de assets");
        CHECK(EditorConfig::relativizarRuta("/a/b/MiModelo.fbx") == "/a/b/MiModelo.fbx",
              "sin raiz no se relativiza");
        CHECK(EditorConfig::absolutizarRuta("Carpetas/MiModelo.fbx") ==
                  "Carpetas/MiModelo.fbx",
              "sin raiz no se absolutiza");
        CHECK(EditorConfig::reemplazarPrefijoRuta("x/UnArchivo.fbx", "x", "y") ==
                  "y/UnArchivo.fbx",
              "reemplazo de prefijo puro funciona sin contexto");
        CHECK(EditorConfig::reemplazarPrefijoRuta("zzz/UnArchivo.fbx", "x", "y")
                  .empty(),
              "prefijo que no cierra en separador no reemplaza (x vs zzz)");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/otro/a.fbx", "/ruta", "/ln")
                  .empty(),
              "sin match devuelve vacio");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/ruta/a.fbx", "/ruta", "/ruta")
                  .empty(),
              "reemplazo identico no hace nada");
    }

    // 6b. Con raiz de assets fijada (src<nombre> del proyecto abierto): las
    //     rutas de la escena se guardan relativas y se resuelven al cargar.
    //     Las escenas legacy (absolutas) se dejan intactas.
    {
        const std::string raiz = "/dato/MotorGrafico/JuegoX/srcJuegoX";
        EditorConfig::fijarRaizAssets(raiz);
        CHECK(EditorConfig::hayRaizAssets(), "con proyecto hay raiz de assets");

        const std::string abs = raiz + "/Modelos/Auto/model.fbx";
        const std::string rel = EditorConfig::relativizarRuta(abs);
        CHECK(rel == "Modelos/Auto/model.fbx",
              "ruta bajo la raiz se guarda relativa");
        CHECK(EditorConfig::absolutizarRuta(rel) == abs,
              "la relativa vuelve a absoluta al cargar");

        CHECK(EditorConfig::relativizarRuta("/dato/Otro/fuera.fbx") ==
                  "/dato/Otro/fuera.fbx",
              "ruta fuera de la raiz se conserva absoluta");
        CHECK(EditorConfig::absolutizarRuta("C:\\escena\\legacy\\x.dds") ==
                  "C:\\escena\\legacy\\x.dds",
              "absoluta legacy (windows) no se toca");
        CHECK(EditorConfig::absolutizarRuta("/abs/legacy/x.dds") ==
                  "/abs/legacy/x.dds",
              "absoluta legacy (unix) no se toca");

        // reemplazarPrefijoRuta NO depende del contexto: reescribe el prefijo
        // de una ruta absoluta (base de la actualizacion tras mover/renombrar).
        CHECK(EditorConfig::reemplazarPrefijoRuta(abs, raiz + "/Modelos",
                                                  raiz + "/Assets/Modelos") ==
                  raiz + "/Assets/Modelos/Auto/model.fbx",
              "reescribe prefijo de carpeta movida");

        EditorConfig::limpiarRaizAssets();
        CHECK(EditorConfig::hayRaizAssets() == false,
              "limpiar deja de relativizar");
        CHECK(EditorConfig::absolutizarRuta("Modelos/Auto/model.fbx") ==
                  "Modelos/Auto/model.fbx",
              "sin raiz la relativa almacenada queda como estaba");
    }

    // 6c. Cotejo de prefijos del explorador contra las rutas de la
    //     escena. ProjectPaths arma la raiz con '/' y absolutizarRuta
    //     concatena con '/', mientras que el explorador publica la ruta con
    //     el separador nativo (std::filesystem: '\' en Windows). Antes del
    //     fix, mover o renombrar una carpeta devolvia 0 cambios en silencio
    //     y las referencias de la escena quedaban apuntando al lugar viejo.
    {
        EditorConfig::limpiarRaizAssets();
        const std::string raiz =
            "C:\\base\\MotorGrafico/Proyects/JuegoX/srcJuegoX";
        EditorConfig::fijarRaizAssets(raiz);

        // Ruta en memoria tal como queda al cargar la escena.
        const std::string enMemoria =
            EditorConfig::absolutizarRuta("Scripts/cpp.cpp");
        CHECK(enMemoria == raiz + "/Scripts/cpp.cpp",
              "absolutizar concatena raiz + '/' + relativa");

        // Prefijo tal como lo arma el explorador: raiz + separador nativo.
        const std::string prefijoExplorador = raiz + "\\Scripts";
        const std::string destino = raiz + "\\Assets\\Scripts";

        const std::string reescrita = EditorConfig::reemplazarPrefijoRuta(
            enMemoria, prefijoExplorador, destino);
#ifdef _WIN32
        CHECK(reescrita == destino + "/cpp.cpp",
              "prefijo con '\\' reescribe la ruta resuelta con '/'");
        // El prefijo con '/' (caso de 6a/6b) tiene que seguir funcionando.
        CHECK(EditorConfig::reemplazarPrefijoRuta(enMemoria, raiz + "/Scripts",
                                                  raiz + "/Assets/Scripts") ==
                  raiz + "/Assets/Scripts/cpp.cpp",
              "el prefijo con '/' no se rompe");
        // Frontera: el prefijo cierra solo si le sigue un separador.
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/ScriptsEx/cpp.cpp", prefijoExplorador, destino)
                  .empty(),
              "'Scripts' no cubre 'ScriptsEx' (frontera con separadores "
              "mixtos)");
        // '\\' cierra el prefijo tambien en rutas con '/' puro (raiz unix).
        CHECK(EditorConfig::reemplazarPrefijoRuta("/dato/a\\b/f.fbx", "/dato/a",
                                                  "/dato/z") ==
                  "/dato/z\\b/f.fbx",
              "en Windows '\\' es separador para el cotejo");
#else
        CHECK(reescrita.empty(),
              "en Linux '\\' NO es separador de rutas");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/dato/a\\b/f.fbx", "/dato/a",
                                                  "/dato/z")
                  .empty(),
              "en Linux '\\' es un caracter normal de nombre");
#endif
        // Mayusculas y minusculas: el sistema de archivos de Windows no las
        // distingue, asi que "assets/scripts" y "Assets/Scripts" son la misma
        // carpeta y el cotejo tiene que reconocerlo (si no, al mover o
        // renombrar con distinta capitalizacion la referencia de la escena se
        // queda con la ruta vieja). En Linux SI se distinguen: "X" y "x" pueden
        // ser dos archivos distintos, asi que ahi el cotejo es sensible.
#ifdef _WIN32
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/assets/scripts/cpp.cpp", raiz + "/Assets/Scripts",
                  raiz + "/Assets/Cpp")
                  == raiz + "/Assets/Cpp/cpp.cpp",
              "en Windows el prefijo se coteja sin distinguir mayusculas");
        CHECK(EditorConfig::relativizarRuta("c:\proyecto\ASSETS\scripts\a.dll") ==
                  "scripts/a.dll",
              "en Windows relativizar no depende de como este escrita la raiz");
#else
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/assets/scripts/cpp.cpp", raiz + "/Assets/Scripts",
                  raiz + "/Assets/Cpp")
                  .empty(),
              "en Linux el prefijo se coteja distinguiendo mayusculas");
#endif

        // Sin falsos positivos en ninguna plataforma: el prefijo tiene que
        // cerrar en un separador (mismo contrato que 6a).
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "Ab/cpp.cpp", prefijoExplorador, destino)
                  .empty(),
              "srcJuegoX no cubre srcJuegoXAb");

        EditorConfig::limpiarRaizAssets();
    }

    // Reset (Fase 3): restablecer vuelve a los defaults de fabrica.
    {
        EditorConfig cfg;

        // Modifica varios campos a valores no default.
        auto& d = cfg.datos();
        d.idioma = "English";
        d.sensibilidadCamara = 2.5f;
        d.sensibilidadMovimientoCamara = 2.0f;
        d.apariencia.temaClaro = true;
        d.apariencia.blancoYNegro = true;
        d.ventanaCamarasAbierta = false;
        d.gizmoOperacion = 5;
        d.gizmoGlobal = true;
        d.camaraActivaId = 3;
        d.nombreProyecto = "MiProyecto";
        d.estadoVentanas["Estado"] = false;

        cfg.restablecer();

        CHECK(cfg.datos().idioma == "Espanol", "restablecer vuelve el idioma");
        CHECK(cfg.datos().sensibilidadCamara == 0.15f,
              "restablecer vuelve la sensibilidad");
        CHECK(cfg.datos().sensibilidadMovimientoCamara == 1.0f,
              "restablecer vuelve la sensibilidad de movimiento");
        CHECK(!cfg.datos().apariencia.temaClaro,
              "restablecer vuelve el tema");
        CHECK(!cfg.datos().apariencia.blancoYNegro,
              "restablecer vuelve el modo B/N");
        CHECK(cfg.datos().ventanaCamarasAbierta, "restablecer abre la ventana camaras");
        CHECK(cfg.datos().gizmoOperacion == 7, "restablecer vuelve el gizmo");
        CHECK(cfg.datos().gizmoGlobal == false,
              "restablecer vuelve el gizmo a LOCAL");
        CHECK(cfg.datos().camaraActivaId == -1,
              "restablecer vuelve la camara a automatica");
        CHECK(cfg.datos().nombreProyecto.empty(),
              "restablecer deja el proyecto sin abrir (main conserva el actual)");
        CHECK(cfg.datos().estadoVentanas.empty(),
              "restablecer limpia el estado de ventanas");
    }

    // Escritura atomica + guardado diferido de la config general:
    //  - la escritura va a un ".tmp" y se renombra encima (un corte a mitad de
    //    escritura deja el JSON original intacto y no deja temporales colgados);
    //  - los cambios en vivo de Opciones se encolan y NO reescriben el archivo
    //    en cada evento, solo cuando vence el intervalo o en un guardarGeneral()
    //    explicito (Ctrl+S, salida, reset).
    {
        const std::string rutaDif = (base / "ConfiguracionDiferida.json").string();

        EditorConfig cfg;
        cfg.datos().idioma = "Espanol";
        cfg.guardarGeneral(rutaDif);
        CHECK(!fs::exists(rutaDif + ".tmp"),
              "no queda el temporal tras una escritura correcta");

        // Cambio en vivo dentro del intervalo: queda pendiente, no se escribe.
        cfg.datos().idioma = "English";
        cfg.solicitarGuardadoGeneral(rutaDif);
        cfg.volcarGuardadoGeneral();
        EditorConfig lector1;
        lector1.cargarGeneral(rutaDif);
        CHECK(lector1.datos().idioma == "Espanol",
              "dentro del intervalo el guardado diferido no reescribe");

        // Vencido el intervalo, el volcado si escribe (una sola vez basta).
        std::this_thread::sleep_for(EditorConfig::kIntervaloEscritura +
                                    std::chrono::milliseconds(30));
        cfg.volcarGuardadoGeneral();
        EditorConfig lector2;
        lector2.cargarGeneral(rutaDif);
        CHECK(lector2.datos().idioma == "English",
              "vencido el intervalo el volcado persiste el cambio");

        // guardarGeneral() no espera al intervalo: vuelca el pendiente.
        cfg.datos().idioma = "Espanol";
        cfg.solicitarGuardadoGeneral(rutaDif);
        cfg.guardarGeneral(rutaDif);
        EditorConfig lector3;
        lector3.cargarGeneral(rutaDif);
        CHECK(lector3.datos().idioma == "Espanol",
              "guardarGeneral vuelca el pendiente sin esperar al intervalo");
        CHECK(!fs::exists(rutaDif + ".tmp"),
              "ninguna de las escrituras dejo temporales");
    }

    // 8. Resolucion de la raiz de datos (ProjectPaths). El motor instalado en
    //    Program Files no puede escribir junto al ejecutable, asi que la raiz
    //    cae a la carpeta de datos del usuario. Lo que se comprueba aqui es el
    //    CONTRATO, no el entorno: que la raiz exista y sea escribible, que las
    //    funciones derivadas cuelguen de ella y que la migracion no pise datos.
    {
        const std::string raizDatos = ProjectPaths::directorioBase();
        const std::string original = ProjectPaths::directorioBaseOriginal();

        CHECK(!raizDatos.empty(), "directorioBase no vacio");
        CHECK(!original.empty(), "directorioBaseOriginal no vacio");
        // Termina en MotorGrafico: las dos son la MISMA raiz, solo cambia donde
        // esta. Si esto se cumpliera, el resto de la comprobacion es coherente.
        const std::string sufijo = "MotorGrafico";
        auto terminaEn = [&sufijo](const std::string& ruta) {
            return ruta.size() >= sufijo.size() &&
                   ruta.compare(ruta.size() - sufijo.size(), sufijo.size(),
                                sufijo) == 0;
        };
        CHECK(terminaEn(raizDatos), "directorioBase termina en MotorGrafico");
        CHECK(terminaEn(original), "directorioBaseOriginal termina en MotorGrafico");

        // Coherencia de datosEnRutaDeUsuario con las dos rutas: solo puede ser
        // true si resolvio a OTRO sitio, y ese otro no puede ser el de siempre.
        const bool enUsuario = ProjectPaths::datosEnRutaDeUsuario();
        CHECK(enUsuario == (raizDatos != original),
              "datosEnRutaDeUsuario refleja si la raiz cambio de ubicacion");

        // La raiz debe existir y ser escribible: es la garantia que evita el
        // bug original (escrituras fallando en silencio en Program Files).
        CHECK(fs::is_directory(raizDatos), "la raiz de datos existe");

        // Toda ruta derivada cuelga de la raiz efectiva, no de la historica.
        const std::string proyecto = "PruebaResolucion";
        CHECK(ProjectPaths::directorioProyecto(proyecto) ==
                  raizDatos + "/Proyects/" + proyecto,
              "directorioProyecto cuelga de directorioBase");
        CHECK(ProjectPaths::directorioConfiguraciones() ==
                  raizDatos + "/Configuraciones",
              "directorioConfiguraciones cuelga de directorioBase");
        CHECK(ProjectPaths::rutaConfiguracionGeneral() ==
                  raizDatos + "/Configuraciones/Configuracion.json",
              "la config general cuelga de directorioBase");
        CHECK(ProjectPaths::rutaImguiIni(proyecto) ==
                  raizDatos + "/Proyects/" + proyecto + "/Memory/imgui.ini",
              "el imgui.ini del proyecto cuelga de directorioBase");

        // Migracion: nunca debe pisar datos que ya estan en destino. Se siembra
        // un archivo marcador y se comprueba que sobrevive a la llamada.
        const fs::path sembrado = fs::path(raizDatos) / "marcador_migracion.txt";
        {
            std::ofstream salida(sembrado);
            salida << "no pisar";
        }
        std::string mensajeMigracion = "sin tocar";
        const bool migro =
            ProjectPaths::migrarDatosDesdeRutaOriginal(mensajeMigracion);
        CHECK(fs::exists(sembrado), "la migracion no borra datos existentes");
        CHECK(!migro, "con destino poblado la migracion no copia nada");
        CHECK(mensajeMigracion.empty(),
              "sin migracion el mensaje al usuario queda vacio");
        {
            std::ifstream entrada(sembrado);
            std::string contenido;
            std::getline(entrada, contenido);
            CHECK(contenido == "no pisar", "el contenido del marcador no cambio");
        }
        std::error_code ec;
        fs::remove(sembrado, ec);
    }

    // 8.b La copia de la migracion, aislada. La migracion completa solo se
    //     puede ejecutar cuando la carpeta del ejecutable no es escribible, y en
    //     un runner de CI siempre lo es, asi que sin esto la parte que de verdad
    //     se ejecuta en un fallo real (copiar el arbol) quedaria sin cubrir.
    //     Se comprueba que copia, que trae la estructura anidada y que se niega a
    //     pisar un destino poblado.
    {
        // Carpeta unica por proceso, como el resto de la suite, para que dos
        // ctest simultaneos no se pisen.
        TempPruebas::CarpetaPrueba carpetaCopia("funshi_copia_migracion");
        const fs::path raizCopia = carpetaCopia.ruta();

        const fs::path origen = raizCopia / "origen";
        fs::create_directories(origen / "Proyects" / "Ejemplo" / "Memory");
        { std::ofstream o(origen / "Configuracion.json"); o << "{}"; }
        { std::ofstream o(origen / "Proyects" / "Ejemplo" / "Memory" / "escena.bin");
          o << "escena"; }

        // Destino vacio: debe copiar, incluida la estructura de directorios.
        const fs::path destino = raizCopia / "destino";
        std::string error = "sin tocar";
        const bool copio = ProjectPaths::copiarArbolSiDestinoVacio(
            origen.string(), destino.string(), error);
        CHECK(copio, "copia el arbol cuando el destino esta vacio");
        CHECK(error.empty(), "sin error cuando la copia funciona");
        CHECK(fs::exists(destino / "Configuracion.json"),
              "llego el archivo de la raiz del arbol");
        CHECK(fs::exists(destino / "Proyects" / "Ejemplo" / "Memory" / "escena.bin"),
              "llego tambien la estructura anidada de directorios");

        // Destino poblado: no debe copiar ni pisar nada.
        const fs::path marcador = destino / "marcador.txt";
        { std::ofstream o(marcador); o << "intacto"; }
        std::string error2 = "sin tocar";
        const bool copio2 = ProjectPaths::copiarArbolSiDestinoVacio(
            origen.string(), destino.string(), error2);
        CHECK(!copio2, "no copia cuando el destino ya esta poblado");
        CHECK(error2.empty(),
              "no es un error que el destino este poblado: no hay nada que avisar");
        {
            std::ifstream e(marcador);
            std::string contenido;
            std::getline(e, contenido);
            CHECK(contenido == "intacto", "el archivo preexistente no se toco");
        }

        // Origen inexistente: se informa por `error` en vez de fingir exito.
        std::string error3;
        const bool copio3 = ProjectPaths::copiarArbolSiDestinoVacio(
            (raizCopia / "no_existe").string(),
            (raizCopia / "destino3").string(), error3);
        CHECK(!copio3, "falla si el origen no es un directorio");
        CHECK(!error3.empty(), "deja el motivo en error");
    }

    // 9. Carpetas candidatas del log: la de la raiz de datos va primero (ya cae
    //    a la carpeta del usuario cuando el ejecutable no admite escritura), y
    //    la temporal del sistema es el ultimo recurso. Sin esto, instalado en
    //    Program Files el motor se queda sin log ni consola que lo muestre.
    {
        const std::string exe = "/opt/FunshiEngineGL";
        const std::string datosUsuario = "/home/usuario/.local/share/FunshiEngineGL/MotorGrafico";
        const std::string temp = "/tmp";

        const std::vector<std::string> rutas = RutasLog::candidatas(
            datosUsuario, exe, temp);
        CHECK(rutas.size() == 3, "tres candidatas: raiz de datos, ejecutable y temporal");
        CHECK(rutas[0] == datosUsuario + "/logs",
              "la primera candidata es la raiz de datos");
        CHECK(rutas[1] == exe + "/logs", "la segunda, la carpeta del ejecutable");
        CHECK(rutas[2] == temp + "/FunshiEngineGL/logs",
              "la tercera, la temporal del motor");

        // Sin raiz de datos no se pierde la carpeta del ejecutable.
        const std::vector<std::string> sinDatos = RutasLog::candidatas("", exe, temp);
        CHECK(sinDatos.size() == 2 && sinDatos[0] == exe + "/logs",
              "sin raiz de datos arranca por la carpeta del ejecutable");

        // Todo vacio: no hay candidatas, el arranque lo reporta como log vacio.
        CHECK(RutasLog::candidatas("", "", "").empty(),
              "sin ninguna ruta no hay candidatas");

        // Candidatas repetidas (raiz de datos y ejecutable iguales) se unifican:
        // el motor prueba la primera que acepte escritura, no la ultima.
        const std::vector<std::string> repetidas =
            RutasLog::candidatas(exe, exe, temp);
        CHECK(repetidas.size() == 2 && repetidas[0] == exe + "/logs" &&
                  repetidas[1] == temp + "/FunshiEngineGL/logs",
              "una ruta repetida no se prueba dos veces");
    }

    // 10. La carpeta "logs" es de la raiz de datos, no un proyecto legacy: si
    //     no estuviera reservada, la migracion de proyectos antiguos la
    //     arrastraba dentro de Proyects/ y el log se escribia a otro sitio del
    //     que el motor anuncia.
    {
        CHECK(!ProjectPaths::esNombreValido("logs"),
              "'logs' no es un proyecto: queda reservada en la raiz de datos");
        CHECK(ProjectPaths::esNombreValido("Loges del Pueblo"),
              "un nombre de proyecto corriente sigue siendo valido");
    }

    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "EDITORCONFIG TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}