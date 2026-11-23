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
#ifndef MANIFIESTO_MOD_H
#define MANIFIESTO_MOD_H

#include <string>
#include <vector>

#include "ModVersion.h"

// Una dependencia que el manifiesto de un mod declara sobre otro mod: id del
// mod requerido, version minima (semver; vacio = sin requisito) y si su falta
// impide cargar (requerida) o solo condiciona el orden de carga (orden
// antes/despues/ninguno).
struct DependenciaMod {
    enum class Orden {
        Ninguno,
        Antes,
        Despues,
    };

    std::string id;
    std::string versionMin;
    bool requerida = true;
    Orden orden = Orden::Ninguno;
};

// Manifiesto de un paquete de mod (.fmod): el archivo ManifiestoMod.json que
// viaja en la raiz del ZIP. Es la UNICA fuente de verdad del mod (id, version,
// punto de entrada, dependencias); el nombre del archivo .fmod se ignora.
//
// El parseo es puro y headless (nlohmann/json): valida el esquema, traduce el
// JSON a esta estructura y devuelve un mensaje de error legible cuando algo no
// cuadra. No lanza: el descubrimiento de mods tiene que seguir ante un paquete
// roto y reportar la causa.
struct ManifiestoMod {
    // Version del esquema que entiende esta build. Un manifiesto con otro
    // esquemaVersion se rechaza en vez de leerse a medias: es el cortafuegos
    // que evita interpretar con reglas viejas un formato futuro.
    static constexpr int ESQUEMA_VERSION_ACTUAL = 1;

    int esquemaVersion = 0;
    // Identificador estable, formato "Autor.Nombre" (un unico punto, ambas
    // partes no vacias).
    std::string id;
    std::string nombre;
    std::string descripcion;
    std::string autor;
    // Version del propio mod, semver.
    std::string version;
    std::string licencia;
    std::string icono;
    // v1: solo "nativo". "contenido" queda reservado para una fase posterior.
    std::string tipo;
    // Rango semver contra la version del motor (FUNSHI_VERSION_STR); vacio =
    // sin restriccion.
    std::string versionMotor;
    // Ruta del .dll/.so dentro del paquete.
    std::string puntoEntrada;
    std::vector<DependenciaMod> dependencias;
    std::vector<std::string> conflictos;

    // Parsea el texto JSON de un manifiesto. Devuelve true y llena `salida` si
    // el manifiesto es valido para el esquema actual; false con `error`
    // explicando el campo que falla en caso contrario.
    static bool parsear(const std::string& json, ManifiestoMod& salida,
                        std::string& error);

    // Lee y parsea el manifiesto de un archivo en disco (misma validacion que
    // parsear). Un archivo inexistente o ilegible tambien devuelve false con
    // `error`.
    static bool leerDeArchivo(const std::string& ruta, ManifiestoMod& salida,
                              std::string& error);
};

#endif // MANIFIESTO_MOD_H
