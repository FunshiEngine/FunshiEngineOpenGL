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
#ifndef GESTOR_MODS_H
#define GESTOR_MODS_H

#include <filesystem>
#include <string>
#include <vector>

#include "../Formato/ManifiestoMod.h"

#include "Diagnosticos/RegistroDiagnosticos.h"

// Ambito de un paquete: los mods del motor viven en la carpeta global
// (<directorioBase>/Mods) y los del proyecto abierto en <proyecto>/Mods. El
// ambito solo describe de donde salio el paquete; la carga es unica y el orden
// se resuelve entre ambos.
enum class AmbitoMod {
    Motor,
    Proyecto
};

// Carpeta que el descubrimiento recorre (no recursivo) buscando .fmod.
struct CarpetaMods {
    std::string ruta;
    AmbitoMod ambito = AmbitoMod::Motor;
};

// Un paquete valido, listo para cargarse: su archivo .fmod, el ambito del que
// salio y el manifiesto ya parseado.
struct PaqueteMod {
    std::filesystem::path paquete;
    AmbitoMod ambito = AmbitoMod::Motor;
    ManifiestoMod manifiesto;
};

// Un paquete rechazado, con el motivo legible (se reporta en el panel de
// estado/log).
struct FalloMod {
    std::string paquete;
    std::string causa;
};

// Descubrimiento, validacion y orden de carga de los mods del editor.
//
// El gestor no ejecuta codigo de terceros: lee cada .fmod, parsea su
// manifiesto y decide que puede cargarse y en que orden. Las reglas:
//
// - Un id repetido (en la misma carpeta o entre motor y proyecto) es ERROR:
//   se rechazan TODAS las instancias con ese id.
// - Una dependencia requerida que no esta, o cuya version no cumple el
//   versionMin declarado, rechaza al que la declara; el rechazo se propaga a
//   quien dependa de un mod rechazado.
// - Un conflicto declarado contra un mod presente rechaza al que lo declara.
// - versionMotor es un rango contra la version compilada del motor
//   (FUNSHI_VERSION_STR): si no la contiene, el mod se rechaza.
// - Un ciclo de dependencias rechaza a todos los mods que lo forman.
//
// El orden de carga es topologico y deterministico (empate por id): una
// dependencia "antes" (o requerida sin orden explicito) carga primero; "despues"
// invierte el sentido; "ninguno" no ordena.
class GestorMods {
public:
    struct Resultado {
        // Mods validos, en orden de carga.
        std::vector<PaqueteMod> mods;
        // Paquetes rechazados, cada uno con su causa.
        std::vector<FalloMod> fallos;
    };

    // Recorre las carpetas (ignora las inexistentes), valida cada .fmod y
    // devuelve los mods cargables en orden junto a los rechazados. No lanza.
    static Resultado descubrir(const std::vector<CarpetaMods>& carpetas);

    // Vuelca el resultado al registro de diagnosticos: un evento de error por
    // cada mod rechazado (nombre del mod y causa) y un resumen final
    // "mods cargados: N".
    static void reportar(const Resultado& resultado,
                         RegistroDiagnosticos& registro);

    // Igual que reportar(), contra el registro global.
    static void reportar(const Resultado& resultado);
};

#endif // GESTOR_MODS_H
