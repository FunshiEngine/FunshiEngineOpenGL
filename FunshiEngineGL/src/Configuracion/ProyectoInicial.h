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
#ifndef PROYECTO_INICIAL_H
#define PROYECTO_INICIAL_H

#include <string>

// Politica de arranque del proyecto activo, sin disco ni UI: a partir de lo
// persistido en la config general (ultimoProyecto) y del argumento --proyecto
// decide que proyecto queda abierto y si es un primer arranque. Header-only e
// inline para poder probarla headless sin depender de main.cpp.
namespace ProyectoInicial {

struct Resolucion {
    // Proyecto activo resuelto. Vacio = no hay proyecto abierto (primer
    // arranque o config sin ultimoProyecto): el menu obliga a elegir o crear.
    std::string nombre;
    // True si no existe la config general todavia. Es un primer arranque.
    bool primerArranque = false;
};

// nombrePersistido: ultimo proyecto de la config general (vacio si no hay).
// proyectoCLI: valor de --proyecto (vacio si no se paso).
// hayConfigGeneral: si ya existe Configuracion.json.
// La linea de comandos (--proyecto) tiene prioridad y entra directo al editor;
// si NO se pasa --proyecto, SIEMPRE arranca desde el menu (nombre vacio),
// obligando al usuario a elegir o crear un proyecto. Esto evita entrada
// automatica al editor por haber un ultimoProyecto persistido.
inline Resolucion resolver(const std::string& nombrePersistido,
                           const std::string& proyectoCLI,
                           bool hayConfigGeneral) {
    Resolucion r;
    r.primerArranque = !hayConfigGeneral;
    if (!proyectoCLI.empty()) {
        r.nombre = proyectoCLI;
        r.primerArranque = false;
    }
    // Si no hay --proyecto, dejamos nombre vacio para forzar el menu.
    // El ultimoProyecto persistido NO se usa automaticamente.
    return r;
}

}  // namespace ProyectoInicial

#endif  // PROYECTO_INICIAL_H
