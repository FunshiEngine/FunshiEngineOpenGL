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

// Windows.h ANTES del header propio y de la stdlib, para que
// _HAS_STD_BYTE=0 surta efecto antes de que la stdlib defina std::byte
// (mismo baile que BackendCpp.cpp; el header Proceso.h no trae windows.h).
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#include <windows.h>
#endif

#include "Proceso.h"

#include <map>

#if !defined(_WIN32)
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

#if defined(_WIN32)

// Narrow -> ancho con la codificacion nativa del proceso (CP_ACP), la misma
// conversion que aplicaban std::system y las llamadas A de Win32.
std::wstring ancho(const std::string& texto) {
    if (texto.empty())
        return std::wstring();
    const int n = MultiByteToWideChar(CP_ACP, 0, texto.c_str(),
                                      static_cast<int>(texto.size()), nullptr, 0);
    if (n <= 0)
        return std::wstring();
    std::wstring salida(static_cast<std::size_t>(n), L'\0');
    MultiByteToWideChar(CP_ACP, 0, texto.c_str(), static_cast<int>(texto.size()),
                        &salida[0], n);
    return salida;
}

// Bloque de entorno = entorno actual del motor + los overrides pedidos,
// ordenado alfabeticamente (la forma que espera CreateProcessW).
std::wstring bloqueConExtra(
    const std::vector<std::pair<std::string, std::string>>& entornoExtra) {
    std::map<std::wstring, std::wstring> variables;
    LPWCH actual = GetEnvironmentStringsW();
    if (actual) {
        for (const wchar_t* p = actual; *p;) {
            const wchar_t* fin = p + wcslen(p);
            const wchar_t* igual = wcschr(p, L'=');
            // Las entradas de unidad ("=C:=...") tambien se conservan: vienen
            // de un bloque valido y se reemiten tal cual.
            if (igual && igual > p)
                variables[std::wstring(p, igual)] = std::wstring(igual + 1);
            p = fin + 1;
        }
        FreeEnvironmentStringsW(actual);
    }
    for (const auto& par : entornoExtra)
        variables[ancho(par.first)] = ancho(par.second);

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

// Lanza la linea ya armada y espera. El log se abre AQUI (inheritable) y se
// cierra en el padre en cuanto el proceso arranco: el hijo se lleva su copia.
// bloque == nullptr hereda el entorno del motor.
int lanzar(const std::wstring& linea,
           const std::string& log,
           const std::string& cwd,
           const wchar_t* bloque) {
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    HANDLE hLog = INVALID_HANDLE_VALUE;
    BOOL heredarHandles = FALSE;
    if (!log.empty()) {
        SECURITY_ATTRIBUTES atributos{};
        atributos.nLength = sizeof(atributos);
        atributos.bInheritHandle = TRUE; // el hijo lo recibe via STARTF
        hLog = CreateFileW(ancho(log).c_str(), GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE, &atributos,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hLog == INVALID_HANDLE_VALUE)
            return -1;
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = nullptr;
        si.hStdOutput = hLog;
        si.hStdError = hLog;
        heredarHandles = TRUE;
    }

    std::wstring lineaMutable = linea;
    const std::wstring cwdAncho = ancho(cwd);
    // CREATE_UNICODE_ENVIRONMENT: sin el, CreateProcessW asume que el bloque
    // es ANSI y lo lee byte a byte — un UTF-16 ("P","A","T","H",0,...) se
    // corta en el primer NUL, queda una clave sin '=' y la llamada devuelve
    // ERROR_INVALID_PARAMETER (87). Diagnosticado con la sonda de
    // FunshiEngineGL/sondas/sonda_bloque.cpp: con el bloque del sistema
    // (userenv) y hasta con una copia literal del padre daba 87, mientras
    // CreateProcessA (ANSI por defecto) y .NET (que si pasa el flag) andaban.
    const DWORD flags =
        bloque ? CREATE_UNICODE_ENVIRONMENT : static_cast<DWORD>(0);
    const BOOL ok = CreateProcessW(
        nullptr, &lineaMutable[0], nullptr, nullptr, heredarHandles, flags,
        const_cast<wchar_t*>(bloque),
        cwd.empty() ? nullptr : cwdAncho.c_str(), &si, &pi);

    if (hLog != INVALID_HANDLE_VALUE)
        CloseHandle(hLog);
    if (!ok)
        return -1;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD codigo = 1;
    GetExitCodeProcess(pi.hProcess, &codigo);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return static_cast<int>(codigo);
}

#else // ---------------------------------------------------------------- POSIX

// Punteros a los argv/envp, apuntando a copias que viven en el caller.
// Se construyen TODO en el padre: tras fork() el hijo no debe alocar memoria
// (otro hilo del motor podria tener el lock de malloc).
struct Arreglos {
    std::vector<std::string> copias;
    std::vector<char*> punteros;
};

void armar(Arreglos& salida, const std::vector<std::string>& valores) {
    salida.copias = valores;
    salida.punteros.clear();
    salida.punteros.reserve(salida.copias.size() + 1);
    for (std::string& s : salida.copias)
        salida.punteros.push_back(&s[0]);
    salida.punteros.push_back(nullptr);
}

#endif

} // namespace

namespace Proceso {

int ejecutar(const std::vector<std::string>& argv,
             const std::string& log,
             const std::string& cwd,
             const std::vector<std::pair<std::string, std::string>>&
                 entornoExtra) {
    if (argv.empty())
        return -1;

#if defined(_WIN32)
    const std::wstring linea = ancho(lineaDeArgumentos(argv));
    if (entornoExtra.empty())
        return lanzar(linea, log, cwd, nullptr);
    const std::wstring bloque = bloqueConExtra(entornoExtra);
    return lanzar(linea, log, cwd, bloque.c_str());
#else
    int fdLog = -1;
    if (!log.empty()) {
        fdLog = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fdLog < 0)
            return -1;
    }

    Arreglos args;
    armar(args, argv);

    Arreglos envio;
    const bool hayExtra = !entornoExtra.empty();
    if (hayExtra) {
        // Partir del entorno del motor (heredado por fork) + overrides.
        std::vector<std::string> variables;
        extern char** environ;
        for (char** e = environ; *e; ++e) {
            const std::string actual(*e);
            const std::size_t igual = actual.find('=');
            const std::string clave =
                igual == std::string::npos ? actual : actual.substr(0, igual);
            bool reemplazada = false;
            for (const auto& par : entornoExtra)
                if (par.first == clave)
                    reemplazada = true;
            if (!reemplazada)
                variables.push_back(actual);
        }
        for (const auto& par : entornoExtra)
            variables.push_back(par.first + "=" + par.second);
        armar(envio, variables);
    }

    const pid_t pid = fork();
    if (pid < 0) {
        if (fdLog >= 0)
            close(fdLog);
        return -1;
    }
    if (pid == 0) {
        // Hijo: solo operaciones async-signal-safe hasta el exec.
        if (fdLog >= 0) {
            dup2(fdLog, 1);
            dup2(fdLog, 2);
            close(fdLog);
        }
        if (!cwd.empty() && chdir(cwd.c_str()) != 0)
            _exit(127);
        if (hayExtra) {
            extern char** environ;
            environ = envio.punteros.data();
        }
        execvp(args.punteros[0], args.punteros.data());
        _exit(127);
    }

    if (fdLog >= 0)
        close(fdLog);
    int estado = 0;
    if (waitpid(pid, &estado, 0) < 0)
        return -1;
    if (WIFEXITED(estado))
        return WEXITSTATUS(estado);
    return -1;
#endif
}

#if defined(_WIN32)

int ejecutarConBloque(const std::vector<std::string>& argv,
                      const std::wstring& bloqueEntorno,
                      const std::string& log,
                      const std::string& cwd) {
    if (argv.empty())
        return -1;
    // Copia propia: CreateProcessW puede escribir en los buffers que recibe.
    std::wstring bloque = bloqueEntorno;
    return lanzar(ancho(lineaDeArgumentos(argv)), log, cwd, &bloque[0]);
}

int ejecutarCmdCrudo(const std::string& receta, const std::string& log) {
    // Sin citar: la receta viene armada a mano con sus comillas balanceadas
    // (cmd.exe no entiende el escape \" de la CRT).
    return lanzar(ancho("cmd.exe " + receta), log, std::string(), nullptr);
}

#endif

} // namespace Proceso
