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
        CHECK(m.getIdioma() == "Espanol", "idioma por defecto = Espanol");
        CHECK(m.getIdiomas().size() == 2, "dos idiomas disponibles");
    }

    // 2. Traduccion es/en con efecto en vivo al cambiar el idioma.
    {
        MenuModel m;
        CHECK(m.traducir("iniciar_estudio") == "Iniciar Estudio",
              "traduce al espanol en estado inicial");
        CHECK(m.traducir("volver") == "Volver", "clave volver es");

        m.setIdioma("English");
        CHECK(m.traducir("iniciar_estudio") == "Start Studio",
              "al cambiar de idioma la etiqueta se traduce");
        CHECK(m.traducir("config_proyecto") == "Config Project",
              "traduce config_proyecto");
        CHECK(m.traducir("volver") == "Back", "traduce volver");

        m.setIdioma("Espanol");
        CHECK(m.traducir("iniciar_estudio") == "Iniciar Estudio",
              "vuelve al espanol al cambiar de nuevo");
    }

    // 3. Claves inexistentes se devuelven tal cual (nunca rompe).
    {
        MenuModel m;
        CHECK(m.traducir("clave_inexistente") == "clave_inexistente",
              "clave desconocida se devuelve intacta");
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

        CHECK(m.getIdioma() == "Espanol", "reset restaura el idioma");
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

    // 7. Listado de proyectos disponibles (carpetas de MotorGrafico): se guarda
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
        CHECK(m.traducir("proyectos") == "Proyectos", "clave proyectos es");
        CHECK(m.traducir("sin_proyectos").find("Config Proyect") != std::string::npos,
              "aviso de lista vacia menciona Config Proyect");
        m.setIdioma("English");
        CHECK(m.traducir("proyectos") == "Projects", "clave proyectos en");
        CHECK(m.traducir("editar_nombre") == "Edit name", "clave editar_nombre en");
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

    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    std::cout << (fallos == 0 ? "MENUMODEL TESTS OK" : "MENUMODEL TESTS FALLO")
              << std::endl;
    return fallos == 0 ? 0 : 1;
}