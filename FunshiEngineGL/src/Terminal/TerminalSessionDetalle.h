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
// Preambulo compartido por las unidades de TerminalSession.cpp (generado al dividirlo).
#ifndef TERMINALSESSIONDETALLE_H
#define TERMINALSESSIONDETALLE_H

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

}  // namespace

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

#endif
