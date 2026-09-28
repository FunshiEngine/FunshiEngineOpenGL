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
#ifndef PATHUTILS_H
#define PATHUTILS_H

#include <string>

// Separador de rutas dependiente de la plataforma, compartido por los
// modulos que construyen rutas de archivo a mano (explorador de archivos).
// Antes cada .cpp definia su propio PATH_SEP, y era facil divergir.
#ifdef _WIN32
inline constexpr char PATH_SEP = '\\';
#else
inline constexpr char PATH_SEP = '/';
#endif

// Regla unica de "que es un separador de ruta" para COTEJAR rutas (comparar
// prefijos, decidir si una ruta cae bajo otra). No confundir con PATH_SEP,
// que es el separador con el que el motor ESCRIBE rutas nuevas.
//
// En Windows '/' y '\' son el mismo separador para el sistema de archivos y
// el motor mezcla los dos: ProjectPaths arma la raiz con '/' (directorioSrc)
// y absolutizarRuta concatena con '/', mientras que el explorador publica lo
// que devuelve std::filesystem, que concatena con '\'. Cotejar literalmente
// hacia que mover o renombrar una carpeta no reescribiera ninguna referencia
// de la escena (H-18).
//
// En Linux '\' es un caracter perfectamente valido en un nombre de archivo
// (p. ej. "a\b.fbx"), asi que ahi SOLO '/' separa: tratarlo como separador
// inventaria rutas que no existen.
inline bool esSeparadorPath(char c) {
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

// Dos bytes de ruta equivalen: iguales, o los dos separadores (en Windows).
inline bool bytesRutaEquivalentes(char a, char b) {
    if (a == b) return true;
    return esSeparadorPath(a) && esSeparadorPath(b);
}

// Dice si `ruta` cae exactamente bajo `prefijo` (igual o seguida de un
// separador), respetando NUNCA igualar un prefijo que no cierre en un
// separador (p.ej. "srcA" no debe cubrir "srcAb"). El cotejo es byte a byte
// con la equivalencia de separadores de arriba: como es posicional y de
// longitudes iguales, el punto de corte (`prefijo.size()`) sigue siendo
// valido aunque los separadores del prefijo difieran de los de `ruta`.
inline bool rutaBajo(const std::string& ruta, const std::string& prefijo) {
    if (ruta.size() < prefijo.size()) return false;
    for (std::size_t i = 0; i < prefijo.size(); ++i)
        if (!bytesRutaEquivalentes(ruta[i], prefijo[i])) return false;
    if (ruta.size() == prefijo.size()) return true;
    return esSeparadorPath(ruta[prefijo.size()]);
}

#endif