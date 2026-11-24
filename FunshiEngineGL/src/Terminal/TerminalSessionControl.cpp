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
#include "TerminalSessionDetalle.h"

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
