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
#ifndef MANIFIESTO_ASSETS_CORE_H
#define MANIFIESTO_ASSETS_CORE_H

#include <array>
#include <map>
#include <string>

// Nucleo puro y HEADLESS del manifiesto de assets de la escena
// (Memory/Binarios/SceneAssets.json). No conoce GameObject ni la escena:
// trabaja sobre «EntradaAssets», la foto de los paths de asset de un objeto
// (malla + 4 texturas + dll de script).
//
// El manifiesto es un add-on de la serializacion binaria: NO reemplaza al
// .db, sino que centraliza las rutas de asset para (1) gestionar los renames
// y moves del explorador y (2) permitir guardar en cualquier momento (Ctrl+S)
// sin depender del guardado al salir. Al cargar, su contenido tiene
// precedencia sobre los paths que vienen del .db.
struct EntradaAssets {
    // Malla del objeto: el GameObject puede ser en si un Modelos3D (la ruta
    // vive en el objeto) o llevar un componente Model (la ruta vive en el
    // componente). Sola una de las dos convive por objeto; el adaptador
    // decide cual exponer como `malla`.
    std::string malla;
    // Cuatro slots de textura del Material (diffuse, specular, normal, emision).
    std::array<std::string, 4> texturas;
    std::string script;

    EntradaAssets() { texturas.fill(std::string()); }
};

// Lectura/escritura + transformacion de rutas del manifiesto. Todo es
// estatico y sin estado: reutilizable por el modulo de escena y por los
// tests headless.
class ManifiestoAssetsCore {
public:
    // Escribe el archivo JSON (nlohmann, vendoriado en External). Devuelve
    // false si no se pudo abrir/escribir (o si el JSON es invalido).
    static bool escribirArchivo(const std::string& ruta,
                                const std::map<int, EntradaAssets>& entradas);

    // Lee el archivo JSON. Devuelve false si no existe o esta corrupto (nunca
    // lanza); en ese caso `entradas` queda sin tocar. La lectura es tolerante:
    // un objeto sin `malla`/`texturas`/`script` solo aporta los campos que
    // trae, y una entrada sin id numerico se descarta.
    static bool leerArchivo(const std::string& ruta,
                            std::map<int, EntradaAssets>& entradas);

    // Convierte TODOS los paths de una entrada a relativos de la raiz de
    // assets (EditorConfig::relativizarRuta). Sin raiz fijada son passthrough.
    static void relativizarEntrada(EntradaAssets& entrada);

    // Convierte TODOS los paths de una entrada a absolutos
    // (EditorConfig::absolutizarRuta). Un path ya absoluto (legacy) se
    // conserva intacto.
    static void absolutizarEntrada(EntradaAssets& entrada);

    // Dado un path persistido y el valor vigente en memoria, devuelve cual
    // aplicar: el persistido si es NO vacio y DISTINTO del vigente; en caso
    // contrario el vigente. Materializa la precedencia del manifiesto: un
    // path vacio significa «no aporta» (no pisa el .db) y un path igual no
    // obliga a recargar malla/script (que si recargan al asignar).
    static std::string resolverRuta(const std::string& persistida,
                                    const std::string& actual);

    // True si la entrada no aporta ningun path (nada que persistir/aplicar).
    static bool entradaVacia(const EntradaAssets& entrada);
};

#endif // MANIFIESTO_ASSETS_CORE_H