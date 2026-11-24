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
#ifndef COMANDOCOMPILACIONVCVARS_H
#define COMANDOCOMPILACIONVCVARS_H

// Parte MSVC del armado del comando de compilacion de scripts C++: busqueda de
// vcvars64.bat y del Visual Studio instalado, y el comando que prepara su
// entorno. Se separo de ComandoCompilacionCpp.h (que la incluye al final, asi
// quien incluye ese header sigue viendo todo).

#include "ComandoCompilacionCpp.h"

namespace CompilacionCpp {

// Ruta a vcvars64.bat subiendo desde la carpeta del compilador: el toolset
// MSVC la tiene en <VS>/VC/Auxiliary/Build, arriba de VC/Tools/MSVC/<ver>.
// Devuelve vacia si no es un compilador MSVC (MinGW, FUNSHI_CXX manual, etc.).
//
// Vive en el header para que el test de contrato pueda assertar que TERMINA
// (bug real: en MinGW/libstdc++ `path("C:\\").parent_path()` devuelve
// `C:\\` y nunca vacio, asi que el bucle sobre parent_path() giraba para
// siempre y BackendCpp se colgaba al compilar un script en Windows+MinGW;
// jamas aparecio porque scripts-runtime-tests se saltaba en Windows por
// plataforma). Dos guardas contra eso:
//   1. Si la familia no es MSVC, ni siquiera se recorre: MinGW no usa vcvars.
//   2. El recorte se detiene cuando parent_path() deja de avanzar (raiz).
inline std::string vcvars64Ruta(const std::string& compiladorRuta) {
    if (familiaDe(compiladorRuta) != Familia::Msvc) return std::string();
    std::error_code ec;
    std::filesystem::path p =
        std::filesystem::weakly_canonical(detalle::sinComillas(compiladorRuta),
                                          ec);
    if (ec) p = std::filesystem::path(detalle::sinComillas(compiladorRuta));
    for (std::filesystem::path dir = p.parent_path(); !dir.empty();) {
        std::filesystem::path cand =
            dir / "Auxiliary" / "Build" / "vcvars64.bat";
        if (std::filesystem::exists(cand, ec)) return cand.string();
        const std::filesystem::path arriba = dir.parent_path();
        if (arriba == dir) break; // llego a la raiz: no avanza mas
        dir = arriba;
    }
    return std::string();
}

// Que compilador invocar. Los parametros van sueltos para que las pruebas la
// ejerciten: el override del entorno, el valor horneado por el build y si esta
// el toolset de MSVC en la maquina (que averigua el llamador con
// vcvars64EnRaices). El orden es el del comentario de compilador() en
// BackendCpp.cpp y no cambia por plataforma:
//   1. override del entorno, gana siempre: es una peticion explicita;
//   2. el valor horneado, SOLO si existe en disco. La ruta que hornea el build
//      es la del equipo que compilo, y en el del usuario no existe;
//   3. "cl" a secas si hay toolset: con el entorno de vcvars en el PATH el
//      nombre sin ruta basta, y asi el motor no queda atado a la version
//      instalada;
//   4. "g++": lo que resuelva el PATH, que es lo unico que se puede afirmar sin
//      conocer la maquina.
inline std::string elegirCompilador(const std::string& overrideEntorno,
                                    const std::string& horneado,
                                    bool hayToolsetEnMaquina) {
    if (!overrideEntorno.empty()) return detalle::sinComillas(overrideEntorno);
    if (!horneado.empty()) {
        std::error_code ec;
        if (std::filesystem::exists(horneado, ec))
            return detalle::sinComillas(horneado);
    }
    return hayToolsetEnMaquina ? std::string("cl") : std::string("g++");
}

// --- Toolset MSVC de la maquina del usuario ---------------------------------
//
// vcvars64Ruta() solo sabe derivar el entorno de la ruta del COMPILADOR, y esa
// ruta la hornea el build. En el paquete publicado es la del runner que compilo
// (C:\Program Files\Microsoft Visual Studio\...\cl.exe): en el equipo del
// usuario no existe, y subir desde ahi no encuentra ningun vcvars aunque tenga
// Visual Studio instalado al lado. Por eso el toolset se busca tambien en las
// carpetas donde se instala de verdad.
//
// El barrido recibe las raices candidatas por parametro (no las calcula dentro)
// para que las pruebas lo ejerciten con un arbol falso, que es lo unico
// verificable fuera de Windows: 'existe' no necesita un Windows.

// Raices de Visual Studio de esta maquina, en orden de prioridad: lo primero es
// lo que el propio motor ya respeta por entorno, despues las carpetas
// estandar de instalacion (Program Files y Program Files (x86)).
inline std::vector<std::string> raicesVisualStudio() {
    std::vector<std::string> raices;
    const char* vars[] = {"VSINSTALLDIR", "VCToolsInstallDir"};
    for (const char* v : vars) {
        const char* valor = std::getenv(v);
        if (valor && *valor) raices.emplace_back(valor);
    }
#ifdef _WIN32
    const char* bases[] = {"C:/Program Files/Microsoft Visual Studio",
                           "C:/Program Files (x86)/Microsoft Visual Studio"};
    for (const char* base : bases) raices.emplace_back(base);
#endif
    return raices;
}

// vcvars64.bat de la maquina, o vacio. Busca a dos niveles porque las Build
// Tools cuelgan un nivel mas abajo que las ediciones completas
// (<raiz>/2022/Community/VC/... y <raiz>/2022/BuildTools/VC/...), y no baja mas: la variante
// Preview cuelga del mismo nivel que Community. La primera coincidencia gana, asi que el
// orden de las raices es el orden de preferencia.
inline std::string vcvars64EnRaices(const std::vector<std::string>& raices) {
    std::error_code ec;
    for (const std::string& raiz : raices) {
        const std::filesystem::path base(raiz);
        if (!std::filesystem::exists(base, ec)) continue;
        // <raiz>/<anio>/<edicion>/VC/Auxiliary/Build/vcvars64.bat
        for (const auto& anio : std::filesystem::directory_iterator(base, ec)) {
            if (ec) break;
            if (!anio.is_directory()) continue;
            for (const auto& edicion : std::filesystem::directory_iterator(anio.path(), ec)) {
                if (ec) break;
                if (!edicion.is_directory()) continue;
                const std::filesystem::path cand =
                    edicion.path() / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
                if (std::filesystem::exists(cand, ec)) return cand.string();
            }
        }
    }
    return std::string();
}

// --- Entorno de vcvars sin shell (H-3 nivel 2) --------------------------------
//
// cl.exe no arranca sin el entorno del toolset (INCLUDE/LIB/PATH con
// link.exe). Antes eso se lograba encadenando `call vcvars64.bat && cl ...`
// DENTRO de cmd.exe — junto con el comando completo, con todos los datos de
// usuario. Ahora el compilador corre solo (Proceso::ejecutar) y lo unico que
// pasa por cmd es la receta fija de abajo, que no lleva datos de usuario:
// solo la ruta de vcvars, balanceada a mano. Ver PLAN GENERAL DE FIX.md §21.

// Receta cruda para Proceso::ejecutarCmdCrudo. Detalles:
//  - `call`: sin el, cmd entrega el control al .bat y al terminar sale sin
//    ejecutar lo que sigue del `&&`;
//  - `/U`: la salida de `set` en UTF-16LE. Sin /U escribe en la CP OEM
//    (sonda: en espanol, CP850) y una ruta no-ASCII se corromperia al
//    reconvertirla (el round-trip con /U quedo byte-exacto);
//  - `/d`: ignora el AutoRun del registro;
//  - el remainder arranca con `call` (no con comilla), asi que no aplica el
//    strip de primera/ultima comilla de cmd.exe.
inline std::string comandoEntornoVcvars(const std::string& vcvarsRuta) {
    return "/U /d /c call \"" + vcvarsRuta + "\" && set";
}

// Parsea la salida UTF-16 de `cmd /U ... set` y devuelve el bloque de
// entorno (multi-sz: pares clave=valor\0 terminado con un \0 extra),
// ordenado alfabeticamente como pide CreateProcessW. Solo entran lineas con
// nombre de variable valido: el banner de vcvars (sin `=` o con formas que
// no son CLAVE=valor) se descarta. El valor puede contener `=` (se corta en
// el PRIMERO). Devuelve vacio si no hay ni 3 variables: no es una salida de
// `set` utilizable (archivo vacio, cmd no corrio, archivo viejo).
inline std::wstring bloqueDesdeSet(const std::wstring& salida) {
    std::map<std::wstring, std::wstring> variables;
    std::wstring::size_type inicio = 0;
    while (inicio < salida.size()) {
        std::wstring::size_type fin = salida.find(L'\n', inicio);
        if (fin == std::wstring::npos)
            fin = salida.size();
        std::wstring linea = salida.substr(inicio, fin - inicio);
        inicio = fin + 1;
        if (!linea.empty() && linea.back() == L'\r')
            linea.pop_back();

        const std::wstring::size_type igual = linea.find(L'=');
        if (igual == std::wstring::npos || igual == 0)
            continue; // banner u otras lineas sin clave
        bool nombreValido = true;
        for (std::wstring::size_type i = 0; i < igual; ++i) {
            const wchar_t c = linea[i];
            const bool ok = (c >= L'A' && c <= L'Z') || (c >= L'a' && c <= L'z') ||
                            (c >= L'0' && c <= L'9') || c == L'_';
            if (!ok || (i == 0 && c >= L'0' && c <= L'9')) {
                nombreValido = false;
                break;
            }
        }
        if (!nombreValido)
            continue;
        variables[linea.substr(0, igual)] = linea.substr(igual + 1);
    }
    if (variables.size() < 3)
        return std::wstring();

    std::wstring bloque;
    for (const auto& par : variables) {
        bloque += par.first;
        bloque += L'=';
        bloque += par.second;
        bloque += L'\0';
    }
    bloque += L'\0';
    return bloque;
}

#if defined(_WIN32)

// Entorno completo de vcvars, ejecutado y parseado UNA vez por proceso
// (cache: el primer script MSVC paga los ~200 ms del .bat). Devuelve el
// bloque multi-sz listo para Proceso::ejecutarConBloque, o VACIO si algo
// fallo (vcvars inexistente, cmd no corrio, salida ilegible): en ese caso
// BackendCpp compila con el entorno heredado y el error exacto de cl queda
// en el log. El archivo se lee como bytes (UTF-16LE) sin windows.h, para
// que el header siga siendo seguro en cualquier TU.
inline const std::wstring& entornoVcvars(const std::string& vcvarsRuta) {
    static std::wstring cache;
    static std::string cacheLlave;
    if (vcvarsRuta == cacheLlave && !cache.empty())
        return cache;
    cacheLlave = vcvarsRuta;
    cache.clear();
    if (vcvarsRuta.empty())
        return cache;

    std::error_code ec;
    const std::filesystem::path tmp =
        std::filesystem::temp_directory_path(ec) /
        ("funshi_vcvars_entorno_" +
         std::to_string(std::chrono::steady_clock::now()
                            .time_since_epoch()
                            .count()) +
         ".txt");
    if (ec)
        return cache;

    Proceso::ejecutarCmdCrudo(comandoEntornoVcvars(vcvarsRuta), tmp.string());

    std::ifstream archivo(tmp, std::ios::binary);
    if (archivo) {
        const std::string bytes((std::istreambuf_iterator<char>(archivo)),
                                std::istreambuf_iterator<char>());
        archivo.close();
        // UTF-16LE: pares de bytes (posible BOM FF FE inicial; los pares
        // incompletos de un archivo truncado se ignoran).
        std::size_t i = 0;
        if (bytes.size() >= 2 &&
            static_cast<unsigned char>(bytes[0]) == 0xFF &&
            static_cast<unsigned char>(bytes[1]) == 0xFE)
            i = 2;
        std::wstring salida;
        salida.reserve((bytes.size() - i) / 2);
        for (; i + 1 < bytes.size(); i += 2)
            salida.push_back(static_cast<wchar_t>(
                static_cast<unsigned char>(bytes[i]) |
                (static_cast<unsigned char>(bytes[i + 1]) << 8)));
        cache = bloqueDesdeSet(salida);
    }
    std::filesystem::remove(tmp, ec);
    return cache;
}

#endif // _WIN32

// Verdadero si `flags` contiene el flag exacto, comparando por tokens: "/MD" no
// tiene que dar positivo sobre "/MDd" ni "-O2" sobre "-O2x". Se usa en el test
// de contrato y deja el chequeo en un solo lugar.
inline bool tieneFlag(const std::string& flags, const std::string& flag) {
    if (flag.empty()) return false;
    std::string::size_type pos = 0;
    while ((pos = flags.find(flag, pos)) != std::string::npos) {
        const bool bordeIzquierdo = pos == 0 || flags[pos - 1] == ' ';
        const std::string::size_type fin = pos + flag.size();
        const bool bordeDerecho = fin >= flags.size() || flags[fin] == ' ';
        if (bordeIzquierdo && bordeDerecho) return true;
        pos = fin;
    }
    return false;
}

} // namespace CompilacionCpp

#endif // COMANDOCOMPILACIONVCVARS_H
