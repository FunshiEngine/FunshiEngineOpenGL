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
#ifndef MENUMODEL_H
#define MENUMODEL_H

#include <functional>
#include <string>
#include <vector>

#include "../../Configuracion/Apariencia.h"

// Modelo del paquete MenuGUI: la capa de logica pura del menu de inicio del
// motor (sin ImGui/GLFW ni rendering). Es el patron MVP de este paquete:
// - MenuModel    (este archivo): estado + navegacion entre vistas + datos de
//   configuracion. Independiente de la interfaz y testeable.
// - MenuView     (MenuView.h): presentacion. Solo lee el estado y delega las
//   acciones de vuelta aca; nunca decide la logica.
// - StartMenuPresenter (StartMenuPresenter.h): puente con el resto del motor:
//   traduce el estado del modelo a "el menu esta abierto/cerrado" para main
//   y sincroniza de vuelta cambios externos (p. ej. el nombre leido del
//   proyecto en disco).
// - MenuGUI      (MenuGUI.h): fachada del paquete; main no conoce las clases
//   internas, solo interactua con la interfaz publica del paquete (ver
//   README.md del paquete).
// El modelo centraliza el estado del menu para que las vistas se puedan
// reemplazar o recorrer sin tocar la logica.
class MenuModel {
public:
    enum class Vista { Principal, Opciones, ConfigProyecto, Ninguna };

    // Abre el menu principal (por ejemplo desde el editor con Escape).
    void mostrarMenu();
    // Cierra el menu y deja el motor listo para editar (boton Iniciar Estudio).
    void iniciarEstudio();
    void abrirOpciones();
    void abrirConfigProyecto();
    // Vuelve de Opciones/ConfigProyecto al menu principal.
    void volver();

    bool estaVisible() const noexcept;
    Vista getVista() const noexcept;

    const std::string& getNombreProyecto() const noexcept;
    void setNombreProyecto(const std::string& nombre);

    // Proyectos disponibles: las carpetas de <directorioBase>/MotorGrafico
    // (los proyectos SON carpetas). La lista la rellena la fachada
    // (MenuGUI::actualizarProyectos) desde el disco; la vista solo la muestra.
    const std::vector<std::string>& getProyectosDisponibles() const noexcept;
    void setProyectosDisponibles(const std::vector<std::string>& proyectos);

    // Edicion de nombre por click derecho: la vista registra aqui el nombre
    // original de la carpeta a renombrar y main la consume al confirmar.
    // Vacia = confirmacion normal (crear/cambiar). Con valor = renombrar esa
    // carpeta al nombre confirmado (modelo puro, sin disco/ImGui).
    const std::string& getProyectoARenombrar() const noexcept;
    void setProyectoARenombrar(const std::string& nombre);
    void limpiarProyectoARenombrar() noexcept;

    // Eliminacion por click derecho: la vista registra aqui el nombre de la
    // carpeta a eliminar y main la consume al confirmar el modal (borrado de
    // disco + reset del estado si era el proyecto activo). Vacia = sin
    // eliminacion pendiente.
    const std::string& getProyectoAEliminar() const noexcept;
    void setProyectoAEliminar(const std::string& nombre);
    void limpiarProyectoAEliminar() noexcept;

    const std::string& getIdioma() const noexcept;
    void setIdioma(const std::string& valor);
    const std::vector<std::string>& getIdiomas() const noexcept;

    // Traduccion declarativa de las etiquetas del menu (idioma actual). Vive
    // en el modelo para que el efecto del idioma sea probable y no dependa de
    // ImGui: la vista solo pregunta el texto de la clave. Un clave inexistente
    // se devuelve tal cual (nunca rompe). Codigos: "iniciar_estudio",
    // "config_proyecto", "opciones", "salir", "volver", "juego", "idioma",
    // "sensibilidad_camara", "apariencia", "tema_claro", "modo_bn",
    // "color_acento", "color_fondo", "ayuda_bn", "restablecer_apariencia",
    // "restablecer_configuracion", "ayuda_reset", "nombre".
    std::string traducir(const std::string& clave) const;

    // Sensibilidad global del mouse look de la camara (vista Opciones del
    // menu). Multiplicador aplicado en main al offset del raton; 1.0 = 1:1
    // pixel/grado (comportamiento historico).
    float getSensibilidadCamara() const noexcept;
    void setSensibilidadCamara(float sensibilidad);
    // Sensibilidad de MOVIMIENTO (WASD) de la camara del editor (vista
    // Opciones): multiplica la velocidad base de la camara activa. Se aplica a
    // la escena por SensibilidadMovimientoCambio (independiente del mouse look).
    float getSensibilidadMovimientoCamara() const noexcept;
    void setSensibilidadMovimientoCamara(float sensibilidad);

    // Perfil de apariencia (tema, modo B/N, acento de la UI y fondo 3D). La
    // vista Opciones lo edita y main lo aplica a ImGui y a la escena.
    const Apariencia& getApariencia() const noexcept;
    void setApariencia(const Apariencia& valor);

    // Vuelve idioma, sensibilidad y apariencia a los valores de fabrica (accion
    // "Restablecer configuracion" de Opciones). El nombre del proyecto se
    // conserva: define la carpeta/proyecto y un reset lo destruiria. El resto
    // del motor (estado de ventanas, gizmo, camara) lo reaplica main via
    // ReiniciarConfiguracion.
    void restablecerConfiguracion();

    // Campo modificado (patron observer simple y testeable): la fachada se
    // suscribe para publicar el cambio en el bus de GUI (EditorEventBus) SIN
    // que la vista conozca el bus. Asi toda mutacion del modelo (vista o main)
    // fluye por un unico lugar y los cambios en vivo llegan a la escena.
    enum class Campo {
        Nombre,
        Idioma,
        SensibilidadCamara,
        SensibilidadMovimientoCamara,
        Apariencia,
        Reiniciar, // restablecerConfiguracion() completo
    };
    using OnCampoCambio = std::function<void(Campo)>;
    void setOnCampoCambio(OnCampoCambio cb);

private:
    Vista vista = Vista::Principal;
    std::string nombreProyecto = "Nuevo Proyecto";
    std::vector<std::string> proyectosDisponibles;
    // Carpeta original en edicion por click derecho (vacia = confirmar normal).
    std::string proyectoARenombrar;
    // Carpeta pendiente de eliminacion (vacia = sin eliminacion registrada).
    std::string proyectoAEliminar;
    std::string idioma = "Espanol";
    std::vector<std::string> idiomasDisponibles = {"Espanol", "English"};
    float sensibilidadCamara = 0.15f;
    // Sensibilidad de movimiento (WASD) de la camara del editor. Se configura
    // en la vista Opciones junto a la sensibilidad de camara y se aplica a la
    // escena por SensibilidadMovimientoCambio.
    float sensibilidadMovimientoCamara = 1.0f;
    Apariencia apariencia;
    // Un solo observador (la fachada). Puntero a funcion/closure NO propietario;
    // si no hay observador, no hacer nada.
    OnCampoCambio onCampoCambio;
};

#endif