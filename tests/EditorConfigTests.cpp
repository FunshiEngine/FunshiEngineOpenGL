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
#include <string>
#include <thread>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"

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
        CHECK(cfg.datos().nombreProyecto == "Nuevo Proyecto", "default nombreProyecto");
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
        cfg.datos().apariencia.fondo[0] = 0.3f;
        cfg.datos().apariencia.fondo[1] = 0.4f;
        cfg.datos().apariencia.fondo[2] = 0.5f;
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
        CHECK(cfg2.datos().apariencia.fondo[2] == 0.5f,  "roundtrip fondo b");
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
        CHECK(cfg.datos().nombreProyecto == "Nuevo Proyecto", "corrupto -> defaults");
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
        CHECK(cfg.datos().nombreProyecto == "Nuevo Proyecto",
              "parcial: sin seccion general -> default");
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
        // Sonidos es un asset: vive dentro del src (raiz del explorador).
        const std::string sonidosDir = EditorConfig::directorioSonidos(proyNombre);
        CHECK(sonidosDir == srcDir + "/Sonidos",
              "directorioSonidos dentro de src<proyecto>");
        CHECK(fs::is_directory(sonidosDir),
              "asegurarEstructuraProyecto creo src<proyecto>/Sonidos");

        // Limpieza de prueba
        std::error_code ec;
        fs::remove_all(proyDir, ec);
    }

    // 5b. Migracion de Sonidos: la carpeta que vivia en la raiz del proyecto
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
        CHECK(cfg.datos().nombreProyecto == "Nuevo Proyecto",
              "restablecer vuelve el nombre por defecto (main lo conserva luego)");
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

    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "EDITORCONFIG TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}