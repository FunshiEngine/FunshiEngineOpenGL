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

// Cargador de mods nativos. Toma los paquetes ya validados por GestorMods,
// extrae cada uno a una carpeta de cache escribible, carga su binario
// (dlopen/LoadLibrary), pide su interfaz por la funcion de entrada y ejecuta
// su inicializacion.
//
// El binario NUNCA se carga desde el .fmod: se extrae primero a la cache
// (en Windows el sistema mantiene bloqueado un binario cargado y el paquete
// es de solo lectura).
//
// La descarga es en orden INVERSO al de carga: cada mod recibe su shutdown()
// antes de que su binario se cierre, para que no llame a codigo ya descargado.

#ifndef CARGADOR_MODS_H
#define CARGADOR_MODS_H

#include <filesystem>
#include <string>
#include <vector>

#include "../Api/ApiMod.h"
#include "GestorMods.h"

// Un mod con su binario cargado y su interfaz obtenida.
struct ModCargado {
    // Identificador del mod (del manifiesto).
    std::string id;
    // Nombre visible (del manifiesto o del propio mod).
    std::string nombre;
    // Version del mod (del manifiesto).
    std::string version;
    // Paquete .fmod del que salio.
    std::string rutaPaquete;
    // Carpeta de la cache donde se extrajo el paquete.
    std::filesystem::path directorioExtraido;
    // Binario cargado.
    std::filesystem::path rutaBinario;
    // Handle opaco de la biblioteca cargada.
    void* handleDll = nullptr;
    // Interfaz del mod (nunca es nullptr en un mod cargado con exito).
    FunshiMods::IMod* interfaz = nullptr;
};

class CargadorMods {
public:
    struct ResultadoCarga {
        // Mods cargados y listos para usar, en orden de carga.
        std::vector<ModCargado> modsCargados;
        // Mods que no se pudieron cargar, cada uno con su causa.
        std::vector<FalloMod> fallos;
    };

    // Carga los mods validos del resultado de descubrimiento, usando la cache
    // por defecto del motor. No lanza: los fallos van en el resultado.
    static ResultadoCarga cargar(const GestorMods::Resultado& descubrimiento);

    // Igual que cargar(), pero con la carpeta de cache indicada. Permite
    // apuntar a una carpeta propia sin tocar la del motor.
    static ResultadoCarga cargar(const GestorMods::Resultado& descubrimiento,
                                  const std::filesystem::path& directorioCache);

    // Descarga todos los mods en orden inverso al de carga: llama shutdown()
    // (si el mod lo implementa) y cierra el binario. Deja la lista vacia.
    static void descargar(std::vector<ModCargado>& modsCargados);

    // Actualiza los mods cargados (una vez por frame): llama update() de cada
    // mod que lo implemente. Un mod puede registrar un panel desde aqui.
    static void actualizar(std::vector<ModCargado>& modsCargados,
                           float deltaTime);

    // Vuelca el resultado al registro de diagnosticos: un evento de error por
    // cada mod que no se pudo cargar (con su causa) y un resumen de cuantos
    // quedaron en memoria.
    static void reportar(const ResultadoCarga& resultado,
                          RegistroDiagnosticos& registro);

    // Igual que reportar(), contra el registro global.
    static void reportar(const ResultadoCarga& resultado);

    // Carpeta de cache de mods: <directorioBase>/cache/mods
    static std::filesystem::path directorioCacheMods();

    // Convierte el id de un mod en un nombre de carpeta seguro: solo letras,
    // digitos, punto, guion y guion bajo.
    static std::string sanitizarId(const std::string& id);

private:
    // Carpeta de cache de un mod concreto: <cache>/<id saneado>.
    static std::filesystem::path carpetaCacheDelMod(
        const PaqueteMod& mod, const std::filesystem::path& directorioCache);
};

#endif // CARGADOR_MODS_H
