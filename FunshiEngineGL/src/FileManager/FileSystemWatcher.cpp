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
#include "FileSystemWatcher.h"

#include <filesystem>

#if defined(__linux__)
#include <sys/inotify.h>
#include <unistd.h>
#endif

namespace {
#if defined(__linux__)
// Eventos de estructura: crear/borrar/mover/renombrar entradas, y cambios
// sobre la propia carpeta vigilada. El kernel ya filtra por mask; un fd sin
// eventos es un fd normal hasta que algo pasa.
constexpr uint32_t MASCARA_VIGILANCIA =
    IN_CREATE | IN_DELETE | IN_MOVE | IN_MODIFY | IN_DELETE_SELF |
    IN_MOVE_SELF;
#endif

constexpr std::chrono::seconds INTERVALO_POLLING(3);
} // namespace

FileSystemWatcher::FileSystemWatcher(const std::string& raiz) : raiz_(raiz) {
#if defined(__linux__)
    fd_ = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (fd_ < 0) return; // sin inotify: el watcher queda inactivo (fd_ == -1)
    aplicarVigilanciaRecursiva();
#else
    ultimoPulso_ = std::chrono::steady_clock::now();
#endif
}

FileSystemWatcher::~FileSystemWatcher() {
#if defined(__linux__)
    if (fd_ >= 0) ::close(fd_);
#endif
}

void FileSystemWatcher::aplicarVigilanciaRecursiva() {
    agregarWatchRama(raiz_);
}

void FileSystemWatcher::agregarWatch(const std::string& rutaCarpeta) {
#if defined(__linux__)
    if (fd_ < 0) return;
    const int wd = inotify_add_watch(fd_, rutaCarpeta.c_str(), MASCARA_VIGILANCIA);
    if (wd >= 0) wdDeCarpeta_[wd] = rutaCarpeta;
#endif
}

// Vigila la carpeta y TODA la rama nueva debajo de ella. Una creacion puede
// traer arboles enteros de golpe (cp -r, create_directories multi-nivel): la
// rama interna no dispara eventos en watches que todavia no existen, asi que
// hay que descenderle (inotify_add_watch por ruta es idempotente: mismo wd).
void FileSystemWatcher::agregarWatchRama(const std::string& raizRama) {
    if (raizRama.empty() || fd_ < 0) return;
    agregarWatch(raizRama);
    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(raizRama, ec);
    if (ec) return;
    const std::filesystem::recursive_directory_iterator fin;
    for (; it != fin; it.increment(ec)) {
        if (ec) { ec.clear(); continue; }
        std::error_code ecDir;
        if (it->is_directory(ecDir) && !ecDir) agregarWatch(it->path().string());
    }
}

void FileSystemWatcher::leerEventos() {
#if defined(__linux__)
    if (fd_ < 0) return;
    char buffer[8192];
    for (;;) {
        const ssize_t leidos = ::read(fd_, buffer, sizeof(buffer));
        if (leidos <= 0) break; // EAGAIN (drenado) o error: no queda nada

        ssize_t desplazamiento = 0;
        while (desplazamiento < leidos) {
            const auto* evento = reinterpret_cast<const struct inotify_event*>(
                buffer + desplazamiento);
            desplazamiento += sizeof(struct inotify_event) + evento->len;

            if (evento->mask & IN_Q_OVERFLOW) {
                // Cola desbordada: eventos perdidos; ante la duda, refrescar.
                huboCambios_ = true;
                continue;
            }

            const auto it = wdDeCarpeta_.find(evento->wd);
            if (it == wdDeCarpeta_.end()) continue;

            if (evento->mask & IN_IGNORED) {
                // El kernel removio el watch solo (carpeta borrada o movida).
                wdDeCarpeta_.erase(it);
                huboCambios_ = true;
                continue;
            }

            huboCambios_ = true;
            // Vigilar desde YA la rama que aparece al vuelo, para no perder
            // cambios mas profundos entre rescaneos (agregarWatchRama baja
            // por subcarpetas que aun no tienen watch).
            if ((evento->mask & IN_CREATE) && (evento->mask & IN_ISDIR)) {
                const std::string rutaNueva =
                    it->second + "/" + (evento->len ? evento->name : "");
                agregarWatchRama(rutaNueva);
            }
        }
    }
#endif
}

bool FileSystemWatcher::huboCambiosYConsumir() {
#if defined(__linux__)
    leerEventos();
    if (huboCambios_) {
        huboCambios_ = false;
        return true;
    }
    return false;
#else
    // Fallback sin inotify: dimensionar por el mtime de la raiz evita el
    // rescaneo completo cada 3s aun sin ningun cambio. Detecta choques
    // directos de la raiz; los cambios anidados fuera de la carpeta visible
    // quedan para el grid (que se auto-invalida por mtime) o un rescaneo
    // manual. Es el mejor esfuerzo en una plataforma sin watch nativo aqui.
    const auto ahora = std::chrono::steady_clock::now();
    if (ahora - ultimoPulso_ >= INTERVALO_POLLING) {
        ultimoPulso_ = ahora;
        std::error_code ec;
        const auto mtime = std::filesystem::last_write_time(raiz_, ec);
        if (!ec && mtime != ultimoMtimeRaiz_) {
            ultimoMtimeRaiz_ = mtime;
            return true;
        }
    }
    return false;
#endif
}