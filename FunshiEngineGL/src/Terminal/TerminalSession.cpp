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
#if defined(_WIN32)
#define _HAS_STD_BYTE 0
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
#include <fcntl.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "TerminalSession.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <climits>
#include <condition_variable>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace {
constexpr std::size_t kMaxSalida = 1024 * 1024;

// Retrocede desde 'pos' hasta encontrar el inicio de un codepoint UTF-8 válido
// (byte que NO sea byte de continuación 0x80-0xBF). Si no hay codepoint
// completo, devuelve 0.
std::size_t inicioCodepointUtf8(const std::string& s, std::size_t pos) {
    while (pos > 0 && (static_cast<unsigned char>(s[pos - 1]) & 0xC0) == 0x80) {
        --pos;
    }
    return pos;
}

void agregarSalida(std::mutex& mutex, std::string& destino,
                   const char* datos, std::size_t cantidad) {
    std::lock_guard<std::mutex> bloqueo(mutex);
    destino.append(datos, cantidad);
    if (destino.size() > kMaxSalida) {
        // Cortar alineado al inicio de un codepoint UTF-8 para no dejar
        // bytes de continuación huérfanos que corromperían el emulador.
        const std::size_t corteBruto = destino.size() - kMaxSalida;
        const std::size_t corte = inicioCodepointUtf8(destino, corteBruto);
        destino.erase(0, corte);
    }
}

#if defined(_WIN32)
std::wstring convertirAncho(const std::string& texto) {
    if (texto.empty()) return {};
    const int cantidad = MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, texto.data(),
        static_cast<int>(texto.size()), nullptr, 0);
    if (cantidad <= 0) return {};
    std::wstring resultado(static_cast<std::size_t>(cantidad), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, texto.data(),
                        static_cast<int>(texto.size()), &resultado[0], cantidad);
    return resultado;
}
#endif
} // namespace

struct TerminalSession::Impl {
    mutable std::mutex mutexSalida;
    std::string salida;
    std::atomic<bool> activa{false};
    std::thread hiloLectura;
#if defined(_WIN32)
    HANDLE entrada = INVALID_HANDLE_VALUE;
    HANDLE salidaPipe = INVALID_HANDLE_VALUE;
    HANDLE proceso = nullptr;
    HANDLE trabajo = nullptr;
    HPCON pseudoconsola = nullptr;
#else
    int terminal = -1;
    pid_t proceso = -1;
    // Señal de parada solicitada por detener(): el lector la usa para acotar su
    // espera y drenar la salida final aunque la shell no muera de inmediato.
    std::atomic<bool> parada{false};
    // Se pone en true al salir del lector para que detener() pueda esperar el
    // hilo con un deadline acotado en lugar de un join() potencialmente eterno.
    std::atomic<bool> lecturaTerminada{false};
    // true si el proceso hijo ya fue reapeado (por el lector al detectar EOF).
    // Evita doble reap en detener().
    std::atomic<bool> hijoReapeado{false};
    std::mutex mutexLector;
    std::condition_variable condicionLector;
#endif

    void leer() {
        char bloque[4096];
#if defined(_WIN32)
        // Se lee hasta que el extremo de salida de ConPTY se cierre (ReadFile
        // falla o devuelve 0 tras ClosePseudoConsole), de modo que la salida
        // final emitida por la shell no se descarte.
        for (;;) {
            DWORD leidos = 0;
            if (!ReadFile(salidaPipe, bloque, sizeof(bloque), &leidos, nullptr) ||
                leidos == 0)
                break;
            agregarSalida(mutexSalida, salida, bloque, leidos);
        }
#else
        // El maestro del PTY es no bloqueante; poll() acota la espera para que
        // detener() nunca dependa de close() para despertar el read().
        struct pollfd descriptor {};
        descriptor.fd = terminal;
        descriptor.events = POLLIN;
        // Ventana de gracia: al pedir la parada, el lector sigue leyendo 500 ms
        // mas para no descartar la salida final de la shell (la que se emite
        // al recibir la senal de parada). El plazo se calcula aqui, en el
        // hilo que lo usa: compartir el instante con el hilo que pide la
        // parada lo hacia escribir por un lado y leer por otro, y ademas se
        // fijaba antes de que existiera (la ventana nunca llegaba a aplicar).
        std::chrono::steady_clock::time_point limiteGracia{};
        bool graciaIniciada = false;
        for (;;) {
            const int listo = ::poll(&descriptor, 1, 100);
            if (listo < 0) {
                if (errno == EINTR) continue;
                break;
            }
            if (parada.load(std::memory_order_acquire)) {
                if (!graciaIniciada) {
                    limiteGracia = std::chrono::steady_clock::now() +
                                   std::chrono::milliseconds(500);
                    graciaIniciada = true;
                }
                if (std::chrono::steady_clock::now() >= limiteGracia) {
                    drenarResto(bloque, sizeof(bloque));
                    break;
                }
            }
            if (listo == 0) continue;
            if (!(descriptor.revents & (POLLIN | POLLHUP | POLLERR))) continue;
            const ssize_t leidos = ::read(terminal, bloque, sizeof(bloque));
            if (leidos > 0) {
                agregarSalida(mutexSalida, salida, bloque,
                              static_cast<std::size_t>(leidos));
            } else if (leidos < 0 && errno == EINTR) {
                continue;
            } else if (leidos < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                continue;
            } else {
                // EOF o error terminal: reapear el hijo para evitar zombie
                if (proceso > 0) {
                    int estado = 0;
                    const pid_t reaped = waitpid(proceso, &estado, WNOHANG);
                    // Solo se marca como reapeado si el hijo termino de verdad:
                    // waitpid devuelve 0 cuando el hijo sigue vivo (aun no
                    // fue reapeado) y -1 si ya no es hijo nuestro. Marcarlo
                    // en cualquiera de los dos casos dejaba el proceso como
                    // zombie para siempre.
                    if (reaped == proceso)
                        hijoReapeado.store(true, std::memory_order_release);
                }
                break;
            }
        }
#endif
        activa.store(false, std::memory_order_release);
#if !defined(_WIN32)
        lecturaTerminada.store(true, std::memory_order_release);
        condicionLector.notify_all();
#endif
    }

#if !defined(_WIN32)
    void drenarResto(char* bloque, std::size_t tam) {
        for (;;) {
            const ssize_t leidos = ::read(terminal, bloque, tam);
            if (leidos > 0) {
                agregarSalida(mutexSalida, salida, bloque,
                              static_cast<std::size_t>(leidos));
            } else if (leidos < 0 && errno == EINTR) {
                continue;
            } else {
                break;
            }
        }
    }
#endif
};

TerminalSession::TerminalSession() : impl_(std::make_unique<Impl>()) {}

TerminalSession::~TerminalSession() { detener(); }

bool TerminalSession::iniciar(const std::string& shell,
                              const std::string& directorio,
                              unsigned columnas, unsigned filas,
                              std::string& error) {
    detener();
    if (shell.empty() || directorio.empty()) {
        error = "La shell y el directorio de trabajo son obligatorios";
        return false;
    }
    std::error_code errorRuta;
    if (!std::filesystem::is_directory(std::filesystem::u8path(directorio), errorRuta)) {
        error = "El directorio del proyecto no está disponible";
        return false;
    }
    columnas = std::max(1u, columnas);
    filas = std::max(1u, filas);

#if defined(_WIN32)
    SECURITY_ATTRIBUTES seguridad{};
    seguridad.nLength = sizeof(seguridad);
    seguridad.bInheritHandle = FALSE;
    HANDLE entradaLectura = INVALID_HANDLE_VALUE;
    HANDLE salidaEscritura = INVALID_HANDLE_VALUE;
    HANDLE entradaEscritura = INVALID_HANDLE_VALUE;
    HANDLE salidaLectura = INVALID_HANDLE_VALUE;
    if (!CreatePipe(&entradaLectura, &entradaEscritura, &seguridad, 0) ||
        !CreatePipe(&salidaLectura, &salidaEscritura, &seguridad, 0)) {
        error = "No se pudieron crear los canales de ConPTY";
        goto fallo_windows;
    }
    SetHandleInformation(entradaEscritura, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(salidaLectura, HANDLE_FLAG_INHERIT, 0);

    {
        const COORD tamano{static_cast<SHORT>(std::min(columnas, 32767u)),
                           static_cast<SHORT>(std::min(filas, 32767u))};
        const HRESULT creado = CreatePseudoConsole(
            tamano, entradaLectura, salidaEscritura, 0,
            &impl_->pseudoconsola);
        if (FAILED(creado)) {
            error = "ConPTY no está disponible en este Windows";
            goto fallo_windows;
        }
    }
    {
        SIZE_T bytesAtributos = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &bytesAtributos);
        std::vector<unsigned char> almacenamiento(bytesAtributos);
        auto* atributos = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            almacenamiento.data());
        if (!InitializeProcThreadAttributeList(atributos, 1, 0,
                                              &bytesAtributos)) {
            error = "No se pudo preparar el inicio de la shell ConPTY";
            goto fallo_windows;
        }
        if (!UpdateProcThreadAttribute(
                atributos, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
                impl_->pseudoconsola, sizeof(impl_->pseudoconsola), nullptr,
                nullptr)) {
            DeleteProcThreadAttributeList(atributos);
            error = "No se pudo asociar la shell a ConPTY";
            goto fallo_windows;
        }

        std::wstring ejecutable = convertirAncho(shell);
        if (ejecutable.find_first_of(L"/\\") == std::wstring::npos) {
            std::vector<wchar_t> rutaEjecutable(32768, L'\0');
            const DWORD longitud = SearchPathW(nullptr, ejecutable.c_str(), nullptr,
                                               static_cast<DWORD>(rutaEjecutable.size()),
                                               rutaEjecutable.data(), nullptr);
            if (longitud == 0 || longitud >= rutaEjecutable.size()) {
                error = "No se encontro el ejecutable de la shell";
                DeleteProcThreadAttributeList(atributos);
                goto fallo_windows;
            }
            ejecutable.assign(rutaEjecutable.data(), longitud);
        }
        std::wstring directorioAncho = convertirAncho(directorio);
        std::wstring linea = L"\"" + ejecutable + L"\"";
        if (shell == "cmd.exe") linea += L" /Q";
        else if (shell == "powershell.exe") linea += L" -NoLogo -NoExit";
        else if (shell == "pwsh.exe") linea += L" -NoLogo -NoExit";
        STARTUPINFOEXW inicio{};
        inicio.StartupInfo.cb = sizeof(inicio);
        inicio.lpAttributeList = atributos;
        PROCESS_INFORMATION info{};
        const BOOL iniciado = CreateProcessW(
            nullptr, &linea[0], nullptr, nullptr, FALSE,
            EXTENDED_STARTUPINFO_PRESENT, nullptr, directorioAncho.c_str(),
            &inicio.StartupInfo, &info);
        DeleteProcThreadAttributeList(atributos);
        if (!iniciado) {
            error = "No se pudo iniciar la shell solicitada";
            goto fallo_windows;
        }
        CloseHandle(entradaLectura);
        entradaLectura = INVALID_HANDLE_VALUE;
        CloseHandle(salidaEscritura);
        salidaEscritura = INVALID_HANDLE_VALUE;
        CloseHandle(info.hThread);
        impl_->trabajo = CreateJobObjectW(nullptr, nullptr);
        if (!impl_->trabajo) {
            error = "No se pudo crear el grupo de procesos de la shell";
            TerminateProcess(info.hProcess, 1);
            CloseHandle(info.hProcess);
            goto fallo_windows;
        }
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limites{};
        limites.BasicLimitInformation.LimitFlags =
            JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(impl_->trabajo,
                                     JobObjectExtendedLimitInformation,
                                     &limites, sizeof(limites)) ||
            !AssignProcessToJobObject(impl_->trabajo, info.hProcess)) {
            error = "No se pudo asociar la shell a su grupo de procesos";
            TerminateProcess(info.hProcess, 1);
            CloseHandle(info.hProcess);
            CloseHandle(impl_->trabajo);
            impl_->trabajo = nullptr;
            goto fallo_windows;
        }
        impl_->proceso = info.hProcess;
    }
    impl_->entrada = entradaEscritura;
    impl_->salidaPipe = salidaLectura;
    impl_->activa.store(true, std::memory_order_release);
    impl_->hiloLectura = std::thread([this] { impl_->leer(); });
    return true;

fallo_windows:
    if (entradaLectura != INVALID_HANDLE_VALUE) CloseHandle(entradaLectura);
    if (entradaEscritura != INVALID_HANDLE_VALUE) CloseHandle(entradaEscritura);
    if (salidaLectura != INVALID_HANDLE_VALUE) CloseHandle(salidaLectura);
    if (salidaEscritura != INVALID_HANDLE_VALUE) CloseHandle(salidaEscritura);
    if (impl_->pseudoconsola) {
        ClosePseudoConsole(impl_->pseudoconsola);
        impl_->pseudoconsola = nullptr;
    }
    return false;
#else
    if (::access(shell.c_str(), X_OK) != 0) {
        error = "La shell seleccionada no está instalada: " + shell;
        return false;
    }
    struct winsize tamano{};
    tamano.ws_col = static_cast<unsigned short>(
        std::min(columnas, static_cast<unsigned>(USHRT_MAX)));
    tamano.ws_row = static_cast<unsigned short>(
        std::min(filas, static_cast<unsigned>(USHRT_MAX)));
    const pid_t hijo = forkpty(&impl_->terminal, nullptr, nullptr, &tamano);
    if (hijo < 0) {
        error = "No se pudo crear el pseudo-terminal: " +
                std::string(std::strerror(errno));
        return false;
    }
    if (hijo == 0) {
        if (::chdir(directorio.c_str()) != 0) _exit(126);
        char* argumentos[] = {const_cast<char*>(shell.c_str()),
                              const_cast<char*>("-i"), nullptr};
        ::execvp(argumentos[0], argumentos);
        static constexpr char mensaje[] =
            "No se pudo iniciar la shell seleccionada\n";
        ::write(STDERR_FILENO, mensaje, sizeof(mensaje) - 1);
        _exit(127);
    }
    impl_->proceso = hijo;
    const int banderas = ::fcntl(impl_->terminal, F_GETFL, 0);
    if (banderas >= 0) ::fcntl(impl_->terminal, F_SETFL, banderas | O_NONBLOCK);
    impl_->parada.store(false, std::memory_order_release);
#if !defined(_WIN32)
    impl_->lecturaTerminada.store(false, std::memory_order_release);
#endif
    impl_->hijoReapeado.store(false, std::memory_order_release);
    impl_->activa.store(true, std::memory_order_release);
    impl_->hiloLectura = std::thread([this] { impl_->leer(); });
    return true;
#endif
}

bool TerminalSession::escribir(const std::string& texto) {
    if (!impl_->activa.load(std::memory_order_acquire)) return false;
#if defined(_WIN32)
    std::size_t offset = 0;
    while (offset < texto.size()) {
        DWORD escritos = 0;
        const DWORD cantidad = static_cast<DWORD>(std::min<std::size_t>(
            texto.size() - offset, static_cast<std::size_t>(MAXDWORD)));
        if (!WriteFile(impl_->entrada, texto.data() + offset, cantidad,
                       &escritos, nullptr) || escritos == 0)
            return false;
        offset += escritos;
    }
    return true;
#else
    std::size_t offset = 0;
    // Reintentos acotados ante EAGAIN (buffer PTY lleno). poll(POLLOUT)
    // espera a que el descriptor esté listo para escritura.
    constexpr int kMaxReintentosEagain = 5;
    int reintentosEagain = 0;
    while (offset < texto.size()) {
        const ssize_t escritos = ::write(impl_->terminal, texto.data() + offset,
                                         texto.size() - offset);
        if (escritos < 0) {
            if (errno == EINTR) continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (reintentosEagain >= kMaxReintentosEagain) return false;
                struct pollfd fd = {impl_->terminal, POLLOUT, 0};
                if (::poll(&fd, 1, 200) <= 0) {
                    ++reintentosEagain;
                    continue;
                }
                ++reintentosEagain;
                continue;
            }
            return false;
        }
        if (escritos == 0) return false;
        offset += static_cast<std::size_t>(escritos);
        reintentosEagain = 0;
    }
    return true;
#endif
}

void TerminalSession::redimensionar(unsigned columnas, unsigned filas) {
    columnas = std::max(1u, columnas);
    filas = std::max(1u, filas);
#if defined(_WIN32)
    if (impl_->pseudoconsola) {
        const COORD tamano{static_cast<SHORT>(std::min(columnas, 32767u)),
                           static_cast<SHORT>(std::min(filas, 32767u))};
        ResizePseudoConsole(impl_->pseudoconsola, tamano);
    }
#else
    if (impl_->terminal >= 0) {
        struct winsize tamano{};
        tamano.ws_col = static_cast<unsigned short>(
            std::min(columnas, static_cast<unsigned>(USHRT_MAX)));
        tamano.ws_row = static_cast<unsigned short>(
            std::min(filas, static_cast<unsigned>(USHRT_MAX)));
        ioctl(impl_->terminal, TIOCSWINSZ, &tamano);
    }
#endif
}

std::string TerminalSession::leerSalida() {
    std::lock_guard<std::mutex> bloqueo(impl_->mutexSalida);
    std::string resultado;
    resultado.swap(impl_->salida);
    return resultado;
}

bool TerminalSession::estaActiva() const {
    return impl_->activa.load(std::memory_order_acquire);
}

void TerminalSession::detener() {
#if defined(_WIN32)
    // Cerrar la pseudoconsola hace que el extremo de salida se cierre y el
    // hilo lector drene la salida final; no se cierra salidaPipe hasta unir el
    // hilo para no cortar la lectura en curso.
    if (impl_->pseudoconsola) {
        ClosePseudoConsole(impl_->pseudoconsola);
        impl_->pseudoconsola = nullptr;
    }
    if (impl_->entrada != INVALID_HANDLE_VALUE) {
        CloseHandle(impl_->entrada);
        impl_->entrada = INVALID_HANDLE_VALUE;
    }
    if (impl_->hiloLectura.joinable()) impl_->hiloLectura.join();
    if (impl_->salidaPipe != INVALID_HANDLE_VALUE) {
        CloseHandle(impl_->salidaPipe);
        impl_->salidaPipe = INVALID_HANDLE_VALUE;
    }
    impl_->activa.store(false, std::memory_order_release);
    if (impl_->proceso) {
        if (WaitForSingleObject(impl_->proceso, 2000) == WAIT_TIMEOUT &&
            impl_->trabajo) {
            TerminateJobObject(impl_->trabajo, 1);
            WaitForSingleObject(impl_->proceso, INFINITE);
        }
        CloseHandle(impl_->proceso);
        impl_->proceso = nullptr;
    }
    if (impl_->trabajo) {
        CloseHandle(impl_->trabajo);
        impl_->trabajo = nullptr;
    }
#else
    // Pedir la parada y colgar el grupo de la shell. Si la shell muere, el
    // lector drena la salida final hasta el cierre del esclavo; si la ignora
    // y sigue emitiendo, la ventana absoluta de gracia lo saca igual.
    impl_->parada.store(true, std::memory_order_release);
    impl_->condicionLector.notify_all();
    if (impl_->proceso > 0) kill(-impl_->proceso, SIGHUP);
    {
        std::unique_lock<std::mutex> bloqueo(impl_->mutexLector);
        impl_->condicionLector.wait_for(
            bloqueo, std::chrono::seconds(5),
            [this] { return impl_->lecturaTerminada.load(std::memory_order_acquire); });
    }
    if (!impl_->lecturaTerminada.load(std::memory_order_acquire)) {
        if (impl_->proceso > 0) kill(-impl_->proceso, SIGKILL);
        std::unique_lock<std::mutex> bloqueo(impl_->mutexLector);
        impl_->condicionLector.wait_for(
            bloqueo, std::chrono::seconds(5),
            [this] { return impl_->lecturaTerminada.load(std::memory_order_acquire); });
    }
    if (impl_->hiloLectura.joinable()) impl_->hiloLectura.join();
    if (impl_->terminal >= 0) {
        ::close(impl_->terminal);
        impl_->terminal = -1;
    }
    if (impl_->proceso > 0) {
        // Evitar doble reap: si el lector ya lo reapeo, solo marcar. El lector
        // solo marca cuando waitpid devolvio el pid, asi que aqui el hijo
        // sigue vivo o ya no es hijo nuestro (ECHILD).
        if (!impl_->hijoReapeado.load(std::memory_order_acquire)) {
            int estado = 0;
            pid_t resultado = waitpid(impl_->proceso, &estado, WNOHANG);
            if (resultado == 0) {
                // El hijo sigue vivo: se mata al grupo y se espera a que
                // termine, para no dejar un zombie.
                kill(-impl_->proceso, SIGKILL);
                while (waitpid(impl_->proceso, &estado, 0) < 0 && errno == EINTR) {
                }
            }
        }
        impl_->proceso = -1;
    }
    impl_->activa.store(false, std::memory_order_release);
#endif
}
