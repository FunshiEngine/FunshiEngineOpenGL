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

}  // namespace

// 14. Tabla de servicios de mods: version en offset fijo, registro de
//     paneles y consultas de datos de la escena.
void seccion14() {
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
void seccion15() {
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
