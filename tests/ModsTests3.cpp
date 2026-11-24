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

// 10. Rutas de mods: la global cuelga de la raiz de datos y la del
//     proyecto de su carpeta. ProjectPaths compone con '/', que es su
//     contrato (no el separador nativo): se compara contra el mismo texto.
void seccion10() {
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
void seccion11() {
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
void seccion12() {
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
void seccion13() {
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
