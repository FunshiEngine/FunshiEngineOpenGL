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
// Pruebas headless del MenuModel (logica pura del menu de inicio, sin ImGui):
// traduccion es/en, observer de cambios (Campo) y restablecerConfiguracion.
// Como el modelo no depende de ImGui/GLFW, se compila como un test aparte.

#include <iostream>
#include <string>

#include "../FunshiEngineGL/src/GUI/MenusGUI/MenuModel.h"
#include "../FunshiEngineGL/src/Localizacion/Localizacion.h"

using Localizacion::Texto;

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
} // namespace

int main() {
    // 1. Idiomas disponibles y valor por defecto.
    {
        MenuModel m;
        CHECK(m.getIdioma() == "es", "idioma por defecto = es");
        CHECK(m.getIdiomas().size() == 2, "dos idiomas disponibles");
    }

    // 2. Traduccion es/en con efecto en vivo al cambiar el idioma.
    {
        MenuModel m;
        CHECK(std::string(m.traducir(Texto::IniciarEstudio)) == "Iniciar Estudio",
              "traduce al espanol en estado inicial");
        CHECK(std::string(m.traducir(Texto::Volver)) == "Volver", "clave volver es");

        m.setIdioma("English");
        CHECK(std::string(m.traducir(Texto::IniciarEstudio)) == "Start Studio",
              "al cambiar de idioma la etiqueta se traduce");
        CHECK(std::string(m.traducir(Texto::ConfigProyecto)) == "Configure project",
              "traduce config_proyecto");
        CHECK(std::string(m.traducir(Texto::Volver)) == "Back", "traduce volver");

        m.setIdioma("Espanol");
        CHECK(std::string(m.traducir(Texto::IniciarEstudio)) == "Iniciar Estudio",
              "vuelve al espanol al cambiar de nuevo");
    }

    // 3. Un texto fuera de rango no rompe: devuelve cadena vacia (no nullptr).
    {
        MenuModel m;
        const char* fuera = m.traducir(
            static_cast<Texto>(Localizacion::kCantidadTextos + 5));
        CHECK(fuera != nullptr && std::string(fuera).empty(),
              "texto fuera de rango devuelve vacio, nunca nullptr");
    }

    // 4. Observer: cada mutacion notifica el campo correcto.
    {
        MenuModel m;
        MenuModel::Campo notificado = MenuModel::Campo::Nombre;
        int avisos = 0;
        m.setOnCampoCambio([&](MenuModel::Campo c) {
            notificado = c;
            ++avisos;
        });

        m.setIdioma("English");
        CHECK(notificado == MenuModel::Campo::Idioma, "setIdioma notifica Idioma");
        CHECK(avisos == 1, "un aviso tras setIdioma");

        m.setSensibilidadCamara(3.0f);
        CHECK(notificado == MenuModel::Campo::SensibilidadCamara,
              "setSensibilidadCamara notifica SensibilidadCamara");

        m.setSensibilidadMovimientoCamara(2.5f);
        CHECK(notificado == MenuModel::Campo::SensibilidadMovimientoCamara,
              "setSensibilidadMovimientoCamara notifica SensibilidadMovimientoCamara");

        Apariencia ap;
        ap.temaClaro = true;
        m.setApariencia(ap);
        CHECK(notificado == MenuModel::Campo::Apariencia,
              "setApariencia notifica Apariencia");

        m.setNombreProyecto("MiProyecto");
        CHECK(notificado == MenuModel::Campo::Nombre,
              "setNombreProyecto notifica Nombre");
        CHECK(avisos == 5, "cinco avisos en total");
    }

    // 5. Reset: vuelve idioma/sensibilidad/apariencia a defaults y conserva el
    //    nombre (el nombre define carpeta/proyecto; no debe destruirse).
    {
        MenuModel m;
        m.setIdioma("English");
        m.setSensibilidadCamara(3.5f);
        Apariencia ap;
        ap.temaClaro = true;
        ap.blancoYNegro = true;
        m.setApariencia(ap);
        m.setNombreProyecto("MiProyecto");

        MenuModel::Campo notificado = MenuModel::Campo::Apariencia;
        m.setOnCampoCambio([&](MenuModel::Campo c) { notificado = c; });
        m.restablecerConfiguracion();

        CHECK(m.getIdioma() == "es", "reset restaura el idioma");
        CHECK(m.getSensibilidadCamara() == 0.15f, "reset restaura la sensibilidad");
        CHECK(m.getSensibilidadMovimientoCamara() == 1.0f,
              "reset restaura la sensibilidad de movimiento");
        CHECK(!m.getApariencia().temaClaro, "reset restaura la apariencia");
        CHECK(!m.getApariencia().blancoYNegro, "reset restaura el modo B/N");
        CHECK(m.getNombreProyecto() == "MiProyecto",
              "reset conserva el nombre del proyecto");
        CHECK(notificado == MenuModel::Campo::Reiniciar,
              "reset notifica Reiniciar para reaplicar el resto del motor");
    }

    // 5b. Reaplicar el mismo perfil de apariencia no notifica: el arranque
    //     aplica la apariencia cargada y no debe contar como cambio del usuario.
    {
        MenuModel m;
        Apariencia ap;
        ap.temaClaro = true;
        m.setApariencia(ap);

        int avisos = 0;
        m.setOnCampoCambio([&](MenuModel::Campo) { ++avisos; });
        m.setApariencia(ap);
        CHECK(avisos == 0, "setApariencia con el mismo perfil no notifica");

        Apariencia otro = ap;
        otro.blancoYNegro = true;
        m.setApariencia(otro);
        CHECK(avisos == 1, "setApariencia con perfil distinto si notifica");
    }

    // 6. La sensibilidad ignora valores no positivos.
    {
        MenuModel m;
        m.setSensibilidadCamara(-1.0f);
        CHECK(m.getSensibilidadCamara() == 0.15f,
              "sensibilidad no valida se ignora");
        m.setSensibilidadCamara(0.0f);
        CHECK(m.getSensibilidadCamara() == 0.15f,
              "sensibilidad cero se ignora");

        m.setSensibilidadMovimientoCamara(-1.0f);
        CHECK(m.getSensibilidadMovimientoCamara() == 1.0f,
              "sensibilidad de movimiento no valida se ignora");
        m.setSensibilidadMovimientoCamara(0.0f);
        CHECK(m.getSensibilidadMovimientoCamara() == 1.0f,
              "sensibilidad de movimiento cero se ignora");
    }

    // 7. Listado de proyectos disponibles (carpetas de FunshiEngine-DATA): se guarda
    //    tal cual lo rellena la fachada y se devuelve en el mismo orden.
    {
        MenuModel m;
        CHECK(m.getProyectosDisponibles().empty(),
              "sin lista asignada no hay proyectos");

        const std::vector<std::string> proyectos = {"SegundoProyecto", "MiProyecto"};
        m.setProyectosDisponibles(proyectos);
        CHECK(m.getProyectosDisponibles().size() == 2, "lista de dos proyectos");
        CHECK(m.getProyectosDisponibles()[0] == "SegundoProyecto",
              "conserva el primer proyecto");
        CHECK(m.getProyectosDisponibles()[1] == "MiProyecto",
              "conserva el segundo proyecto");
    }

    // 8. Etiquetas del selector de proyectos traducidas.
    {
        MenuModel m;
        CHECK(std::string(m.traducir(Texto::Proyectos)) == "Proyectos", "clave proyectos es");
        CHECK(std::string(m.traducir(Texto::SinProyectos)).find("Configurar proyecto") != std::string::npos,
              "aviso de lista vacia menciona Configurar proyecto");
        m.setIdioma("English");
        CHECK(std::string(m.traducir(Texto::Proyectos)) == "Projects", "clave proyectos en");
        CHECK(std::string(m.traducir(Texto::EditarNombre)) == "Edit name", "clave editar_nombre en");
    }

    // 9. Edicion de nombre por click derecho (vacia = confirmar normal).
    {
        MenuModel m;
        CHECK(m.getProyectoARenombrar().empty(),
              "por defecto no hay carpeta en edicion");
        m.setProyectoARenombrar("Viejo");
        CHECK(m.getProyectoARenombrar() == "Viejo",
              "click derecho registra la carpeta original");
        m.limpiarProyectoARenombrar();
        CHECK(m.getProyectoARenombrar().empty(),
              "confirmar/volver limpian el registro");
    }

    // 10. Seccion de la barra ConfigProyecto (por defecto abre en Crear).
    {
        MenuModel m;
        CHECK(m.getVista() == MenuModel::Vista::Principal,
              "el menu abre en Principal");
        m.abrirConfigProyecto();
        CHECK(m.getVista() == MenuModel::Vista::ConfigProyecto,
              "abrirConfigProyecto cambia la vista");
        CHECK(m.getSeccionConfig() == MenuModel::SeccionConfig::CrearProyecto,
              "ConfigProyecto abre en Crear proyecto");
        m.setSeccionConfig(MenuModel::SeccionConfig::Proyectos);
        CHECK(m.getSeccionConfig() == MenuModel::SeccionConfig::Proyectos,
              "setSeccionConfig a Proyectos");
        m.setSeccionConfig(MenuModel::SeccionConfig::Importar);
        CHECK(m.getSeccionConfig() == MenuModel::SeccionConfig::Importar,
              "setSeccionConfig a Importar");
        m.setSeccionConfig(MenuModel::SeccionConfig::CrearProyecto);
        CHECK(m.getSeccionConfig() == MenuModel::SeccionConfig::CrearProyecto,
              "setSeccionConfig de vuelta a Crear");
        m.volver();
        CHECK(m.getVista() == MenuModel::Vista::Principal,
              "volver regresa al menu principal");
        m.abrirConfigProyecto();
        CHECK(m.getSeccionConfig() == MenuModel::SeccionConfig::CrearProyecto,
              "reabrir ConfigProyecto vuelve a Crear proyecto");
    }

    // 11. Intencion de proyecto (Abrir/Crear/Ninguna).
    {
        MenuModel m;
        CHECK(m.getAccionProyecto() == MenuModel::AccionProyecto::Ninguna,
              "accion por defecto = Ninguna");
        m.setAccionProyecto(MenuModel::AccionProyecto::Abrir);
        CHECK(m.getAccionProyecto() == MenuModel::AccionProyecto::Abrir,
              "elegir carpeta registra Abrir");
        m.setAccionProyecto(MenuModel::AccionProyecto::Crear);
        CHECK(m.getAccionProyecto() == MenuModel::AccionProyecto::Crear,
              "dialogo de creacion registra Crear");
        m.limpiarAccionProyecto();
        CHECK(m.getAccionProyecto() == MenuModel::AccionProyecto::Ninguna,
              "limpiar accion vuelve a Ninguna");
    }

    // 12. Dimension elegida para el proyecto nuevo (por defecto 3D).
    {
        MenuModel m;
        CHECK(m.getDimensionProyecto() == DimensionProyecto::TresD,
              "dimension por defecto = 3D");
        m.setDimensionProyecto(DimensionProyecto::DosD);
        CHECK(m.getDimensionProyecto() == DimensionProyecto::DosD,
              "tarjeta 2D registra la dimension");
        m.setDimensionProyecto(DimensionProyecto::TresD);
        CHECK(m.getDimensionProyecto() == DimensionProyecto::TresD,
              "tarjeta 3D registra la dimension");
    }

    // 13. Claves nuevas del diseno por secciones (es/en).
    {
        MenuModel m;
        CHECK(std::string(m.traducir(Texto::CrearProyecto)) == "Crear proyecto",
              "clave crear_proyecto es");
        CHECK(std::string(m.traducir(Texto::ImportarProyecto)) == "Importar proyecto",
              "clave importar_proyecto es");
        CHECK(std::string(m.traducir(Texto::ProyectoActual)) == "Proyecto actual",
              "clave proyecto_actual es");
        CHECK(std::string(m.traducir(Texto::Dimension)) == "Tipo de proyecto",
              "clave dimension es");
        CHECK(std::string(m.traducir(Texto::Dimension2D)) == "2D - Plano",
              "clave dimension_2d es");
        CHECK(std::string(m.traducir(Texto::Dimension3D)) == "3D - Profundidad",
              "clave dimension_3d es");
        CHECK(std::string(m.traducir(Texto::Crear)) == "Crear", "clave crear es");
        CHECK(std::string(m.traducir(Texto::Cancelar)) == "Cancelar", "clave cancelar es");
        m.setIdioma("English");
        CHECK(std::string(m.traducir(Texto::CrearProyecto)) == "Create project",
              "clave crear_proyecto en");
        CHECK(std::string(m.traducir(Texto::ImportarProyecto)) == "Import project",
              "clave importar_proyecto en");
        CHECK(std::string(m.traducir(Texto::ProyectoActual)) == "Current project",
              "clave proyecto_actual en");
        CHECK(std::string(m.traducir(Texto::Dimension2D)) == "2D - Flat", "clave dimension_2d en");
        CHECK(std::string(m.traducir(Texto::Dimension3D)) == "3D - Depth", "clave dimension_3d en");
        CHECK(std::string(m.traducir(Texto::Crear)) == "Create", "clave crear en");
        CHECK(std::string(m.traducir(Texto::Cancelar)) == "Cancel", "clave cancelar en");
        CHECK(std::string(m.traducir(Texto::AvisoNoRevertir)).find("cannot") != std::string::npos,
              "aviso_no_revertir en menciona que no se puede");
    }

    // 14. existeProyecto valida nombres repetidos del dialogo de creacion:
    //     coincidencia exacta y, en Windows, sin distinguir mayusculas (igual
    //     que su filesystem).
    {
        MenuModel m;
        m.setProyectosDisponibles({"MiProyecto", "Otro"});
        CHECK(m.existeProyecto("MiProyecto"), "detecta nombre igual");
        CHECK(m.existeProyecto("Otro"), "detecta segundo nombre");
        CHECK(!m.existeProyecto("Inexistente"), "no detecta nombre distinto");
        CHECK(!m.existeProyecto(""), "nombre vacio nunca existe");
#ifdef _WIN32
        CHECK(m.existeProyecto("miproyecto"),
              "en Windows no distingue mayusculas");
#endif
    }

    // Localizacion: el idioma se guarda por codigo, los nombres viejos se
    // migran y el texto sale de la posicion numeroIdioma * N + texto.
    {
        CHECK(Localizacion::normalizarIdioma("English") == "en",
              "el nombre viejo English se normaliza a en");
        CHECK(Localizacion::normalizarIdioma("Espanol") == "es",
              "el nombre viejo Espanol se normaliza a es");
        CHECK(Localizacion::normalizarIdioma("EN") == "en",
              "el codigo se reconoce sin importar mayusculas");
        CHECK(Localizacion::normalizarIdioma("klingon") == "es",
              "un idioma desconocido cae al idioma base");
        CHECK(Localizacion::nombreIdioma("en") == "English",
              "el combo muestra el nombre del idioma, no el codigo");

        // Idiomas numerados en orden: 0 = es, 1 = en.
        CHECK(Localizacion::cantidadIdiomas() == 2, "dos bloques de idioma");
        CHECK(Localizacion::numeroIdioma("es") == 0, "espanol es el idioma 0");
        CHECK(Localizacion::numeroIdioma("en") == 1, "ingles es el idioma 1");
        CHECK(Localizacion::idioma(1).numero == 1 &&
                  std::string(Localizacion::idioma(1).codigo) == "en",
              "idioma(1) es el bloque del ingles");

        MenuModel m;
        m.setIdioma("English");
        CHECK(m.getIdioma() == "en", "setIdioma guarda el codigo");
        CHECK(std::string(m.traducir(Texto::Salir)) == "Exit", "salir en ingles");
        m.setIdioma("es");
        CHECK(std::string(m.traducir(Texto::Salir)) == "Salir",
              "salir en espanol ya no dice Exit");
        CHECK(std::string(Localizacion::texto("fr", Texto::Volver)) == "Volver",
              "un idioma sin bloque usa el texto del idioma 0");
        CHECK(std::string(Localizacion::texto(99, Texto::Volver)) == "Volver",
              "un numero de idioma fuera de rango usa el idioma 0");

        // Cada idioma tiene texto para cada posicion (propio o de respaldo).
        bool completos = true;
        for (std::size_t n = 0; n < Localizacion::cantidadIdiomas(); ++n) {
            for (std::size_t i = 0; i < Localizacion::kCantidadTextos; ++i) {
                const char* t = Localizacion::texto(n, static_cast<Texto>(i));
                if (t == nullptr || t[0] == '\0') completos = false;
            }
        }
        CHECK(completos, "ningun texto queda vacio en ningun idioma");
        CHECK(std::string(Localizacion::nombre(Texto::ConfigProyecto)) == "ConfigProyecto",
              "el nombre del texto sirve como ID estable");
    }

    // Idioma vigente del editor: tr() y etiquetas ImGui con ID fijo.
    {
        Localizacion::fijarIdiomaActual("English");
        CHECK(Localizacion::idiomaActual() == 1, "el idioma vigente se fija por nombre o codigo");
        CHECK(std::string(Localizacion::tr(Texto::Archivo)) == "File",
              "tr() usa el idioma vigente");
        CHECK(Localizacion::etiqueta(Texto::Aceptar) == "Accept###Aceptar",
              "la etiqueta lleva el nombre del texto como ID fijo");
        CHECK(Localizacion::etiqueta(Texto::Aceptar, "##fila3") == "Accept###Aceptar##fila3",
              "el sufijo ## original distingue widgets con el mismo texto");
        CHECK(Localizacion::etiqueta(Texto::DialogoRenombrar, "###DialogoRenombrar") ==
                  "Rename###DialogoRenombrar",
              "un sufijo ### se usa tal cual como ID");
        Localizacion::fijarIdiomaActual("es");
        CHECK(Localizacion::etiqueta(Texto::Aceptar) == "Aceptar###Aceptar",
              "cambiar de idioma cambia el texto y no el ID");
    }

    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    std::cout << (fallos == 0 ? "MENUMODEL TESTS OK" : "MENUMODEL TESTS FALLO")
              << std::endl;
    return fallos == 0 ? 0 : 1;
}