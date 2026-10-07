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
#ifndef CREAR_CARPETA_H
#define CREAR_CARPETA_H

#include <filesystem>
#include <string>

#include "../../FileManager/FileManager.h"
#include "../../FileManager/FileSelection.h"
#include "../../Herramientas/PathUtils.h"

// Crear una carpeta desde el explorador, compartido por el arbol y el grid:
// un solo camino de disco y un solo criterio de rechazo, para que crear una
// carpeta se comporte igual desde cualquiera de los dos paneles.
//
// La creacion puede fallar y el fallo tiene que verse: si el modal se cerrara
// igual, el usuario creeria que la carpeta existe cuando no, y un nombre con
// separador crearia una carpeta mas profunda en lugar de una al lado de las
// demas. Por eso la decision vive aqui (validar, crear y decir por que no se
// pudo) y cada modal decide cuando cerrarse con el resultado.
namespace CrearCarpeta {

// Un nombre de carpeta es un solo tramo de ruta, no una ruta: la entrada se pega
// al final de la carpeta padre y un separador la convertiria en otra ruta. Es
// la misma regla que ya aplica el renombrado de un elemento.
inline bool nombreValido(const std::string& nombre) {
    if (nombre.empty()) return false;
    // "." y ".." no son nombres: el sistema los resuelve al directorio actual o
    // al padre, y la carpeta creada no seria la que se pidio.
    if (nombre == "." || nombre == "..") return false;
    return nombre.find_first_of("/\\") == std::string::npos;
}

// Lo que quedo hecho y, si no se pudo, por que. `error` lleva el motivo ya
// redactado para el modal y va vacio cuando `creada` es true.
struct Resultado {
    bool creada = false;
    std::string error;
};

// Crea `nombre` dentro de `rutaPadre` y sube el contador de cambios, que es lo
// que dispara el rescaneo del arbol de carpetas. Devuelve creada=true SOLO si la
// carpeta quedo creada ahora.
inline Resultado crear(FileManager* fileManager, const std::string& rutaPadre,
                       const std::string& nombre) {
    Resultado resultado;
    if (fileManager == nullptr || rutaPadre.empty()) {
        resultado.error = "No hay proyecto abierto: no se puede crear la carpeta.";
        return resultado;
    }
    if (nombre.empty()) {
        resultado.error = "Escribe un nombre para la carpeta.";
        return resultado;
    }
    if (!nombreValido(nombre)) {
        resultado.error =
            "El nombre no puede contener '/' ni '\\': "
            "indica una carpeta, no una ruta.";
        return resultado;
    }

    const std::string ruta = rutaPadre + PATH_SEP + nombre;
    std::error_code ec;
    // Aviso preciso del caso mas frecuente, que ademas es el que hoy reporta
    // un exito falso: el nombre ya lo usa otro elemento. La comprobacion es solo
    // para el mensaje; la puerta de paso sigue siendo crearCarpeta.
    if (std::filesystem::exists(ruta, ec)) {
        resultado.error = "Ya existe un elemento con ese nombre en esa carpeta.";
        return resultado;
    }
    if (!fileManager->crearCarpeta(ruta)) {
        resultado.error =
            "No se pudo crear la carpeta: el nombre no lo admite el sistema de "
            "archivos o la carpeta no tiene permisos de escritura.";
        return resultado;
    }

    fileManager->getSelection()->contadorCambios++;
    resultado.creada = true;
    return resultado;
}

}  // namespace CrearCarpeta

#endif
