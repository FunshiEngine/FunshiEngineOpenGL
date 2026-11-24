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
#include "../TablasTextos.h"

// Textos en espanol (idioma base). Toda clave nueva se agrega aqui primero:
// es el respaldo cuando otro idioma no tiene la clave.
namespace Localizacion {
namespace Detalle {

const std::vector<EntradaTexto>& textosEs() {
    static const std::vector<EntradaTexto> tabla = {
        {"iniciar_estudio", "Iniciar Estudio"},
        {"proyectos", "Proyectos"},
        {"sin_proyectos",
         "(no hay proyectos: crea uno en Configurar proyecto)"},
        {"config_proyecto", "Configurar proyecto"},
        {"crear_proyecto", "Crear proyecto"},
        {"importar_proyecto", "Importar proyecto"},
        {"importar_no_disponible",
         "Importar un proyecto todavia no esta disponible."},
        {"proyecto_actual", "Proyecto actual"},
        {"dimension", "Tipo de proyecto"},
        {"dimension_2d", "2D - Plano"},
        {"descripcion_2d",
         "Para juegos vistos de frente, como una hoja de papel: los objetos se "
         "mueven a lo ancho y a lo alto. Ideal para plataformas, puzzles y "
         "juegos arcade."},
        {"dimension_3d", "3D - Profundidad"},
        {"descripcion_3d",
         "Para mundos con profundidad, como el mundo real: los objetos se "
         "mueven a lo ancho, a lo alto y hacia adentro. Ideal para "
         "exploracion, accion y mundos abiertos."},
        {"aviso_no_revertir",
         "Una vez creado, el tipo de proyecto no se puede cambiar: no se puede "
         "pasar de 2D a 3D ni al reves. Elegi con cuidado."},
        {"crear", "Crear"},
        {"cancelar", "Cancelar"},
        {"nombre_vacio", "Escribi un nombre para el proyecto."},
        {"nombre_repetido", "Ya existe un proyecto con ese nombre."},
        {"opciones", "Opciones"},
        {"configuracion", "Configuracion"},
        {"salir", "Salir"},
        {"volver", "Volver"},
        {"juego", "Juego"},
        {"idioma", "Idioma"},
        {"radio_difuminado", "Radio de difuminado"},
        {"sensibilidad_camara", "Sensibilidad de camara"},
        {"sensibilidad_movimiento", "Sensibilidad de movimiento"},
        {"apariencia", "Apariencia"},
        {"audio", "Audio"},
        {"controles", "Controles"},
        {"volumen_maestro", "Volumen maestro"},
        {"tema_claro", "Tema claro de la interfaz"},
        {"modo_bn", "Modo blanco y negro (interfaz y viewport)"},
        {"color_acento", "Color de acento de la interfaz"},
        {"color_cielo_superior", "Color de la parte superior del cielo"},
        {"color_cielo_inferior", "Color de la parte inferior del cielo"},
        {"ayuda_bn",
         "El modo blanco y negro desatura toda la interfaz (incluido el\n"
         "acento) y usa blanco/negro en el fondo y la grilla del viewport\n"
         "segun el tema claro u oscuro."},
        {"restablecer_apariencia", "Restablecer apariencia"},
        {"restablecer_configuracion", "Restablecer configuracion"},
        {"ayuda_reset",
         "Reinicia idioma, apariencia, sensibilidades y estado del editor a\n"
         "los valores de fabrica."},
        {"nombre", "Nombre"},
        {"editar_nombre", "Editar nombre"},
        {"eliminar_proyecto", "Eliminar proyecto"},
        {"aviso_eliminar",
         "Se eliminaran de disco la escena, los assets y la configuracion\n"
         "de este proyecto. Esta accion no se puede deshacer."},
        {"eliminar", "Eliminar"},
        {"confirmar", "Confirmar"},
        {"actual", "(actual: %s)"},
    };
    return tabla;
}

} // namespace Detalle
} // namespace Localizacion
