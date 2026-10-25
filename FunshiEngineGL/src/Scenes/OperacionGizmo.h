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
#ifndef OPERACIONGIZMO_H
#define OPERACIONGIZMO_H

// Operaciones del gizmo de transformacion del editor.
//
// El estado se persiste en la configuracion del proyecto como numero, asi que
// los valores son los del gizmo (mascaras de ejes) y no se pueden cambiar sin
// migrar el archivo. Que el numero sea el del gizmo lo fija la prueba: la
// equivalencia se comprueba contra la propia biblioteca del gizmo.
//
// El modulo de input usa esta enumeracion (y no la del gizmo) para pedir una
// operacion: asi no necesita conocer la biblioteca que dibuja el gizmo.
enum class OperacionGizmo : int {
    Ninguna = 0,
    Trasladar = 7,
    Rotar = 120,
    Escalar = 896,
    Universal = 14463,
};

// Traduce el numero persistido a la operacion. Un valor desconocido (un
// archivo escrito por otra version) cae en Trasladar, la operacion por
// defecto.
constexpr OperacionGizmo operacionDesdeNumero(int valor) noexcept {
    switch (valor) {
        case 0: return OperacionGizmo::Ninguna;
        case 7: return OperacionGizmo::Trasladar;
        case 120: return OperacionGizmo::Rotar;
        case 896: return OperacionGizmo::Escalar;
        case 14463: return OperacionGizmo::Universal;
        default: return OperacionGizmo::Trasladar;
    }
}

// Numero persistido de la operacion (el que viaja en la configuracion).
constexpr int numeroDeOperacion(OperacionGizmo operacion) noexcept {
    return static_cast<int>(operacion);
}

#endif // OPERACIONGIZMO_H
