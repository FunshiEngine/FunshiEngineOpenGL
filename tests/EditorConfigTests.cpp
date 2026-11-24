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
#include "EditorConfigTestsDetalle.h"

namespace EditorConfigTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace EditorConfigTestsDetalle
using namespace EditorConfigTestsDetalle;

// 1. Sin archivo: todo default, sin crashear.
void seccion01(const std::string& rutaGeneral, const std::string& proyNombreTest, const std::string& rutaProyecto) {
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
        CHECK(cfg.datos().dimension == DimensionProyecto::TresD,
              "default dimension 3D");
        CHECK(cfg.datos().estadoVentanas.empty(), "default sin ventanas");
        CHECK(cfg.datos().modsMotor.empty() && cfg.datos().modsProyecto.empty(),
              "default sin mods activos");
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
}

// 1b. Persistencia del proyecto activo: sin "ultimoProyecto" no se abre
//     ningun proyecto y el nombre queda vacio; asi guardarGeneral no
//     materializa un proyecto fantasma ("Nuevo Proyecto").
void seccion02(const std::string& rutaGeneral) {
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
}

// 1c. ProyectoInicial::resolver: politica de arranque sin disco ni UI.
// Sin --proyecto SIEMPRE arranca en el menu (nombre vacio): el
// ultimoProyecto persistido NO reabre solo el editor.
void seccion03() {
    {
        using ProyectoInicial::resolver;
        const auto conConfig = resolver("MiEscena", "", true);
        CHECK(conConfig.nombre.empty(),
              "resolver: sin --proyecto no retoma el persistido (menu)");
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

        const auto conCLI = resolver("MiEscena", "ProyectoCLI", false);
        CHECK(conCLI.nombre == "ProyectoCLI",
              "resolver: --proyecto tiene prioridad sobre el persistido");
        CHECK(!conCLI.primerArranque,
              "resolver: --proyecto entra directo al editor");
    }
}

// 1d. Colores del cielo: se guardan y se leen tal cual quedaron, incluidos
// los claros (un cielo de tonos altos es una eleccion valida del usuario).
// Solo se acotan los valores que no pueden salir del selector y romperian
// el degradado: componentes fuera de [0, 1].
void seccion04(const std::string& rutaGeneral) {
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
}

// 2. Round-trip: los valores cambiados sobreviven a guardar/cargar.
//    Config general y config de proyecto se guardan en archivos separados.
void seccion05(const std::string& rutaGeneral, const std::string& proyNombreTest, const std::string& rutaProyecto) {
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
        cfg.datos().dimension = DimensionProyecto::DosD;
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
        CHECK(cfg2.datos().dimension == DimensionProyecto::DosD,
              "roundtrip dimension 2D");
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
}

// 2b. Dimension del proyecto: roundtrip 3D explicito, clave ausente e
//     invalida. Los proyectos previos al campo (sin "dimension") y los
//     valores fuera de rango se leen como 3D, la dimension con la que se
//     crearon todos los proyectos anteriores.
void seccion06(const fs::path& base, const std::string& proyNombreTest) {
    {
        const std::string rutaDim = (base / "dim_proyecto.json").string();

        // 3D explicito sobrevive el roundtrip.
        {
            EditorConfig cfg;
            cfg.datos().dimension = DimensionProyecto::TresD;
            cfg.guardarProyecto(proyNombreTest, rutaDim);
            EditorConfig leido;
            leido.cargarProyecto(proyNombreTest, rutaDim);
            CHECK(leido.datos().dimension == DimensionProyecto::TresD,
                  "roundtrip dimension 3D");
        }

        // Clave ausente -> 3D (retrocompatibilidad con proyectos viejos).
        {
            std::ofstream f(rutaDim, std::ios::trunc);
            f << R"({"version": 2, "editor": {"camaraActivaId": 3}})";
            EditorConfig leido;
            leido.cargarProyecto(proyNombreTest, rutaDim);
            CHECK(leido.datos().dimension == DimensionProyecto::TresD,
                  "dimension ausente -> 3D");
        }

        // Valor invalido (fuera del enum) -> 3D.
        {
            std::ofstream f(rutaDim, std::ios::trunc);
            f << R"({"version": 3, "editor": {"dimension": 9}})";
            EditorConfig leido;
            leido.cargarProyecto(proyNombreTest, rutaDim);
            CHECK(leido.datos().dimension == DimensionProyecto::TresD,
                  "dimension invalida -> 3D");
        }

        // El JSON persistido guarda la clave editor.dimension como entero.
        {
            EditorConfig cfg;
            cfg.datos().dimension = DimensionProyecto::DosD;
            cfg.guardarProyecto(proyNombreTest, rutaDim);
            std::ifstream f(rutaDim);
            std::string contenido((std::istreambuf_iterator<char>(f)),
                                  std::istreambuf_iterator<char>());
            CHECK(contenido.find("\"dimension\": 2") != std::string::npos,
                  "JSON guarda editor.dimension = 2");
        }
    }
}

// 3. Archivo corrupto: defaults (sin crash).
void seccion07(const fs::path& base, const std::string& rutaGeneral, const std::string& proyNombreTest) {
    {
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{ json roto";
        }
        EditorConfig cfg;
        cfg.cargarGeneral(rutaGeneral);
        CHECK(cfg.datos().nombreProyecto.empty(), "corrupto -> defaults");
        CHECK(cfg.datos().idioma == "Espanol", "corrupto -> defaults idioma");
        CHECK(cfg.datos().modsMotor.empty(), "corrupto -> sin mods");

        // 2d. Estado de mods por ambito: los del motor viajan en la config
        //     general y los del proyecto en la de proyecto; cada uno sobrevive
        //     su round-trip sin mezclarse.
        {
            const std::string rutaModsG = (base / "mods_general.json").string();
            const std::string rutaModsP = (base / "mods_proyecto.json").string();
            EditorConfig cfgMods;
            cfgMods.datos().modsMotor["Autor.MotorUno"] = false;
            cfgMods.datos().modsMotor["Autor.MotorDos"] = true;
            cfgMods.datos().modsProyecto["Autor.ProyectoUno"] = false;
            cfgMods.guardarGeneral(rutaModsG);
            cfgMods.guardarProyecto(proyNombreTest, rutaModsP);

            EditorConfig leido;
            leido.cargarGeneral(rutaModsG);
            leido.cargarProyecto(proyNombreTest, rutaModsP);
            CHECK(leido.datos().modsMotor.size() == 2, "round-trip de mods del motor");
            CHECK(leido.datos().modsMotor["Autor.MotorUno"] == false,
                  "el mod del motor deshabilitado se conserva");
            CHECK(leido.datos().modsMotor["Autor.MotorDos"] == true,
                  "el mod del motor habilitado se conserva");
            CHECK(leido.datos().modsProyecto.size() == 1,
                  "round-trip de mods del proyecto");
            CHECK(leido.datos().modsProyecto["Autor.ProyectoUno"] == false,
                  "el mod del proyecto deshabilitado se conserva");
            // Los dos ambitos no se mezclan entre archivos.
            CHECK(leido.datos().modsMotor.count("Autor.ProyectoUno") == 0,
                  "el mod del proyecto no se guarda en la config general");
            CHECK(leido.datos().modsProyecto.count("Autor.MotorUno") == 0,
                  "el mod del motor no se guarda en la config del proyecto");
        }
    }
}

// 4. Parcial: el campo presente se aplica, el ausente conserva el default.
//    El campo "editor" (ventanaCamarasAbierta) vive en el archivo de proyecto.
void seccion08(const std::string& proyNombreTest, const std::string& rutaProyecto) {
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
}

// 4b. Radio de difuminado: un archivo sin el campo conserva el default (no
//     falla al cargar una configuracion anterior a la opcion) y uno con un
//     valor fuera del rango admitido lo acota, para que el perfil en memoria
//     sea siempre valido aunque el archivo se haya editado a mano.
void seccion09(const std::string& rutaGeneral) {
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
}
