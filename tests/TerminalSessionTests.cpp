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
// Pruebas headless de TerminalSession: la E/S REAL del pseudo-terminal.
//
// - POSIX: forkpty()/read()/write() sobre un maestro de PTY (enlaza -lutil).
// - Windows: ConPTY (CreatePseudoConsole + CreateProcess) con cmd.exe.
//
// Los rechazos deterministicos (shell vacia, directorio inexistente) no
// dependen de una shell real. La prueba de E/S lanza una shell por plataforma,
// escribe un comando con una marca inequivoca y drena la salida hasta que la
// marca aparece o vence un deadline, para que la suite nunca se cuelgue. Si
// ConPTY no esta disponible el test sale con 77 (ctest lo reporta como skip).

#include "../FunshiEngineGL/src/Terminal/TerminalSession.h"

#include "TempPruebas.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

constexpr char kMarca[] = "FUNSHI_PTY_OK";

#ifndef _WIN32
// Marca que la shell solo imprime al recibir la senal de parada, para probar
// que la salida emitida DURANTE detener() no se pierde.
constexpr char kMarcaFinal[] = "FUNSHI_FIN_HUP";

std::string comandoFinal() {
    return std::string("trap 'printf \"") + kMarcaFinal + "\\n\"' HUP\n";
}
#endif

// La marca puede aparecer primero en el eco de la propia linea de entrada
// ("echo FUNSHI_PTY_OK"); esa ocurrencia va precedida por "echo ". La marca
// emitida como SALIDA del comando no lleva ese prefijo (la precede el prompt o
// un salto de linea). Buscar una ocurrencia sin el prefijo prueba que el
// comando corrio de verdad y su salida llego por el pseudo-terminal.
bool marcaEsSalida(const std::string& salida) {
    std::size_t pos = salida.find(kMarca);
    while (pos != std::string::npos) {
        const std::size_t ini = pos >= 5 ? pos - 5 : 0;
        if (salida.substr(ini, pos - ini) != "echo ") return true;
        pos = salida.find(kMarca, pos + 1);
    }
    return false;
}

std::string shellDeLaPlataforma() {
#if defined(_WIN32)
    return "cmd.exe";
#else
    return "/bin/sh";
#endif
}

std::string comandoDeMarca() {
    const std::string base = std::string("echo ") + kMarca;
#if defined(_WIN32)
    return base + "\r\n";
#else
    return base + "\n";
#endif
}

// Drena la sesion acumulando lo leido hasta que aparece la marca o vence el
// deadline (en milisegundos).
std::string leerHastaMarca(TerminalSession& sesion, int ms) {
    std::string acumulado;
    const auto limite =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (std::chrono::steady_clock::now() < limite) {
        acumulado += sesion.leerSalida();
        if (marcaEsSalida(acumulado)) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return acumulado;
}

// Corre detener() en su propio hilo y espera a un deadline. Si no regresa, se
// abandona el hilo: la implementacion puede quedar bloqueada cerrando el
// maestro del PTY con el hilo lector todavia bloqueado en read(). Quien llama
// debe terminar el proceso sin destructores cuando devuelve false, porque el
// hilo abandonado sigue usando la sesion.
bool detenerConTimeout(TerminalSession& sesion, int ms) {
    auto terminado = std::make_shared<std::atomic<bool>>(false);
    std::thread hilo([&sesion, terminado]() {
        sesion.detener();
        terminado->store(true, std::memory_order_release);
    });
    const auto limite =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (!terminado->load(std::memory_order_acquire) &&
           std::chrono::steady_clock::now() < limite) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    const bool completo = terminado->load(std::memory_order_acquire);
    if (completo)
        hilo.join();
    else
        hilo.detach();
    return completo;
}
} // namespace

int main() {
    TempPruebas::CarpetaPrueba carpeta("funshi_terminal_session_tests");
    const std::string directorio = carpeta.ruta().string();

    // 1. Shell vacia: rechazo determinista, sin shell de por medio.
    {
        TerminalSession sesion;
        std::string error;
        const bool ok = sesion.iniciar("", directorio, 80, 24, error);
        CHECK(!ok, "iniciar con shell vacia devuelve false");
        CHECK(!error.empty(), "iniciar con shell vacia deja error no vacio");
    }

    // 2. Directorio inexistente: rechazo determinista.
    {
        TerminalSession sesion;
        std::string error;
        const std::string inexistente =
            (carpeta.ruta() / "no" / "existe" / "funshi").string();
        const bool ok =
            sesion.iniciar(shellDeLaPlataforma(), inexistente, 80, 24, error);
        CHECK(!ok, "iniciar con directorio inexistente devuelve false");
        CHECK(!error.empty(),
              "iniciar con directorio inexistente deja error no vacio");
    }

    // 3. E/S real sobre el pseudo-terminal.
    TerminalSession sesion;
    std::string error;
    const bool ok =
        sesion.iniciar(shellDeLaPlataforma(), directorio, 80, 24, error);
    if (!ok && error.find("ConPTY no está disponible") != std::string::npos) {
        std::cout << "SKIP: " << error << std::endl;
        return 77;
    }
    CHECK(ok, "iniciar E/S real: la sesion arranca (" + error + ")");
    if (ok) {
        CHECK(sesion.estaActiva(), "estaActiva() es true tras iniciar");
        CHECK(sesion.escribir(comandoDeMarca()),
              "escribir devuelve true con la sesion activa");

        const std::string salida = leerHastaMarca(sesion, 10000);
        CHECK(marcaEsSalida(salida),
              "la marca de la salida del comando aparece en el PTY");

        // 5. detener() acotado: no debe colgarse.
        const bool detenida = detenerConTimeout(sesion, 10000);
        CHECK(detenida, "detener() regresa dentro del deadline");
        if (!detenida) {
            std::cout << "FALLOS: detener() no regreso en el deadline; "
                         "terminando sin esperar al hilo bloqueado\n";
            std::cout.flush();
            std::_Exit(1);
        }

        // 6. leerSalida() despues de detener() no debe crashear.
        const std::string sobrante = sesion.leerSalida();
        (void)sobrante;
        CHECK(true, "leerSalida() tras detener() no crashea");

        // 4. Escribir tras detener() es rechazado.
        CHECK(!sesion.escribir("x"), "escribir tras detener() devuelve false");
    }

#ifndef _WIN32
    // 7. La salida emitida DURANTE detener() no se pierde. La shell instala un
    //    trap que imprime una marca al recibir la senal de parada; el lector
    //    debe drenar ese ultimo tramo en lugar de descartarlo, y detener() debe
    //    regresar sin colgarse pese a la salida en vuelo. cmd.exe no tiene un
    //    equivalente a trap/senales, por eso la comprobacion es POSIX.
    {
        TerminalSession sesionParada;
        std::string errorParada;
        const bool okParada = sesionParada.iniciar(shellDeLaPlataforma(),
                                                   directorio, 80, 24,
                                                   errorParada);
        CHECK(okParada, "iniciar sesion de salida final (" + errorParada + ")");
        if (okParada) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sesionParada.escribir(comandoFinal());
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            const bool detenida = detenerConTimeout(sesionParada, 10000);
            CHECK(detenida,
                  "detener() con salida en vuelo regresa dentro del deadline");
            if (!detenida) {
                std::cout << "FALLOS: detener() no regreso con salida en "
                             "vuelo; terminando sin esperar al hilo\n";
                std::cout.flush();
                std::_Exit(1);
            }

            const std::string restante = sesionParada.leerSalida();
            CHECK(restante.find(kMarcaFinal) != std::string::npos,
                  "la salida emitida durante detener() no se pierde");
        }
    }

    // 8. detener() con emision continua que ignora SIGHUP: el lector tiene una
    //    ventana absoluta de gracia y el join espera con deadline escalando a
    //    SIGKILL, asi que detener() regresa aunque la shell no muera con
    //    SIGHUP y siga escribiendo. Contra el codigo previo a la ventana
    //    absoluta, el join se colgaba porque cada tramo de salida reiniciaba
    //    la espera.
    {
        TerminalSession sesionSorda;
        std::string errorSorda;
        const bool okSorda = sesionSorda.iniciar(shellDeLaPlataforma(),
                                                 directorio, 80, 24,
                                                 errorSorda);
        CHECK(okSorda, "iniciar sesion de emision continua (" + errorSorda + ")");
        if (okSorda) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sesionSorda.escribir(
                "trap '' HUP\nwhile :; do echo FLUJO_CONTINUO; done\n");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            const bool detenida = detenerConTimeout(sesionSorda, 15000);
            CHECK(detenida,
                  "detener() con shell sorda que emite sin parar regresa "
                  "dentro del deadline");
            if (!detenida) {
                std::cout << "FALLOS: detener() no regreso con shell sorda; "
                             "terminando sin esperar al hilo\n";
                std::cout.flush();
                std::_Exit(1);
            }
        }
    }
#endif

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones\n";
    return fallos == 0 ? 0 : 1;
}
