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
#ifndef LECTOR_ZIP_H
#define LECTOR_ZIP_H

#include <string>

// Acceso de solo lectura a un paquete de mod (.fmod), que es un ZIP con
// extension propia. Dos operaciones bastan al descubrimiento: sacar UNA
// entrada a memoria (el manifiesto, que viaja en la raiz) y extraer todo el
// paquete a una carpeta de la cache escribible desde la que luego se carga el
// .dll/.so (nunca se carga desde el ZIP: en Windows el sistema mantiene
// bloqueado un binario cargado).
//
// La extraccion valida cada ruta contra el directorio destino (anti zip-slip):
// un paquete con entradas absolutas, con "..", o que resuelvan fuera del
// destino se rechaza entero y no escribe nada. No lanza.
class LectorZip {
public:
    // Lee a memoria la entrada `nombreEntrada` del ZIP en `rutaZip`. Devuelve
    // false con `error` si el archivo no es un ZIP legible o la entrada no
    // existe.
    static bool leerEntrada(const std::string& rutaZip,
                            const std::string& nombreEntrada,
                            std::string& contenido, std::string& error);

    // Extrae todas las entradas del ZIP bajo `directorioDestino` (lo crea si no
    // existe), respetando los subdirectorios. Los nombres de entrada se
    // separan por '/' segun el formato ZIP y se traducen al separador nativo.
    // Devuelve false con `error` ante la primera entrada insegura (sin escribir
    // nada) o si falla la lectura/escritura.
    static bool extraerTodo(const std::string& rutaZip,
                            const std::string& directorioDestino,
                            std::string& error);

    // True si `nombreEntrada` es una ruta relativa sin escapes (sin raiz, sin
    // unidades, sin componentes ".."). Expuesta para que los tests fijen el
    // contrato anti zip-slip por separado de la extraccion.
    static bool esRutaSegura(const std::string& nombreEntrada);
};

#endif // LECTOR_ZIP_H
