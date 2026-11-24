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
#ifndef MOD_VERSION_H
#define MOD_VERSION_H

#include <string>

// Version semver MINIMA del sistema de mods: solo mayor.menor.parche, sin
// prerelease ni metadata de build. Es la unidad de comparacion de las dos
// guardas que protegen la carga: el rango del motor contra la version de un
// mod (campo versionMotor del manifiesto) y el minimo que un mod exige de sus
// dependencias (versionMin). Un alcance mayor (operadores al estilo npm,
// rangos parciales, comodines) queda para una evolucion aditiva.
class ModVersion {
public:
    int mayor = 0;
    int menor = 0;
    int parche = 0;
    // La pone en true parsear(); una version construida a mano es invalida
    // hasta que se parsea, para poder distinguir "0.0.0" de "no inicializada".
    bool valida = false;

    // Parsea "X.Y.Z" con los tres segmentos numericos. Devuelve false y deja
    // `salida` sin tocar si el texto no tiene ese formato.
    static bool parsear(const std::string& texto, ModVersion& salida);

    // Orden natural: negativo si *this < otra, 0 si son iguales, positivo si
    // *this > otra.
    int comparar(const ModVersion& otra) const;

    // *this >= minimo (la cota es inclusiva). False si alguna no es valida.
    bool satisfaceMinimo(const ModVersion& minimo) const;

    // Representacion "X.Y.Z".
    std::string aTexto() const;
};

// Rango de versiones con la misma disciplina minima, con dos formas admitidas:
//   ">=X.Y.Z"            cota inferior inclusiva, sin cota superior
//   ">=X.Y.Z <X.Y.Z"     cota inferior inclusiva, superior EXCLUSIVA
// El separador es espacio (se tolera mas de uno). Es lo que basta para las
// dos guardas: motor<->mod y mod<->mod.
class RangoVersion {
public:
    ModVersion minimo;
    ModVersion maximo;
    bool tieneMaximo = false;
    bool valida = false;

    // Parsea el rango. Devuelve false y deja `salida` sin tocar si el texto no
    // respeta el formato (p. ej. sin ">=" o con "<" sin version).
    static bool parsear(const std::string& texto, RangoVersion& salida);

    // True si `version` cae dentro del rango (cota inferior inclusiva, cota
    // superior exclusiva). Un rango invalido no contiene nada.
    bool contiene(const ModVersion& version) const;

    // Representacion textual canonica del rango.
    std::string aTexto() const;
};

#endif // MOD_VERSION_H
