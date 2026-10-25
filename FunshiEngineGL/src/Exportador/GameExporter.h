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
#ifndef GAMEEXPORTER_H
#define GAMEEXPORTER_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <functional>
#include <thread>

class MotorHilos;

// Exportador de juegos standalone: genera un proyecto CMake temporal,
// compila el juego (recompila scripts de usuario) y empaqueta
// ejecutable + assets + dependencias en una carpeta de distribución.
class GameExporter {
public:
    enum class Plataforma {
        Linux,
        Windows
    };

    // Plataformas a las que se puede exportar desde esta maquina.
    //
    // En Windows el toolchain es el nativo (MinGW): lo que sale es un .exe
    // con sus DLL al lado. En Linux se puede ademas cruzar a Windows con el
    // toolchain de MinGW, si esta instalado.
    static std::vector<Plataforma> plataformasDisponibles();

    // Etiqueta de la plataforma para el menu del dialogo.
    static const char* etiquetaPlataforma(Plataforma plataforma);

    struct Config {
        std::string proyectoOrigen;       // ruta al proyecto en FunshiEngine-DATA/Proyects/
        std::string nombreProyecto;       // nombre de la carpeta del proyecto; si
                                          // viene vacio se deduce de proyectoOrigen
        std::string nombreEjecutable;     // nombre del .exe / binario (sin extensión)
        std::string nombreProyectoExportado; // nombre de la carpeta de exportación
        Plataforma plataforma = Plataforma::Linux;
        std::string directorioSalida;     // FunshiEngine-DATA/Exportaciones/<nombre>/
        // Callbacks de progreso (se ejecutan en hilo del editor)
        std::function<void(const std::string& mensaje)> onLog;
        std::function<void(float progreso, const std::string& etapa)> onProgreso;
        std::function<void(bool exito, const std::string& mensaje)> onFinalizado;
    };

    explicit GameExporter(const Config& cfg);
    ~GameExporter();

    // Inicia la exportación en un hilo separado (no bloquea el editor).
    void iniciar();

    // Verifica si la exportación terminó (para polling desde el editor).
    bool haTerminado() const noexcept;

    // Mensaje final de la exportación (vacío mientras no haya terminado).
    std::string mensajeFinal() const;

    // Pool de hilos donde corre la exportación. Si no se inyecta ninguno, la
    // exportación usa su propio hilo. El pool se usa para el trabajo pesado
    // en segundo plano; el editor sigue en su hilo y se enteran por los
    // callbacks (que encolan en las colas thread-safe del dialogo).
    void setMotorHilos(MotorHilos* motor) noexcept { motorHilos_ = motor; }

    // Carpetas donde puede estar una DLL: la del compilador con el que se
    // construyo el motor (el runtime y las bibliotecas de terceros), las del
    // PATH y las del build.
    static std::vector<std::string> directoriosDeDependencias(
        const std::string& buildDir, bool windows);
    // True si la DLL la trae el sistema (no hace falta copiarla).
    static bool esDllDelSistema(const std::string& dll);
    // Primera ruta donde existe la DLL, o vacia si no esta.
    static std::string buscarDependencia(
        const std::vector<std::string>& directorios, const std::string& dll);
    // De donde sale cada DLL que hay que copiar (vacio si no se encuentra).
    using BuscadorDll = std::function<std::string(const std::string&)>;
    // Cierre de las DLL que necesita un ejecutable: sus propias
    // importaciones y las de cada DLL de la que depende (recursivamente). Una
    // biblioteca que usa otra (assimp usa zlib) arrastra a la que usa.
    // Devuelve pares: nombre de la DLL y de donde sale (vacio si no esta).
    static std::vector<std::pair<std::string, std::string>>
    cierreDeDependencias(const std::string& rutaEjecutable,
                          const BuscadorDll& buscador);
    // Ruta del archivo donde esta exportacion deja su traza: la salida de la
    // compilacion (cmake, el enlazador, los avisos del compilador) se
    // escribe ahi, y no mezclada con el log general del motor. Vacia si no
    // se pudo crear la carpeta.
    static std::string rutaLogExportacion();
    // Directorio de compilacion de esta exportacion: uno propio, en la
    // carpeta temporal del sistema, para que dos exportaciones simultaneas
    // no compartan el mismo directorio de build.
    static std::string directorioBuildTemporal();

private:
    Config cfg_;
    std::thread hiloExportacion_;
    // El pool de hilos (opcional): si esta inyectado, la exportacion corre
    // en el y no en un hilo propio.
    MotorHilos* motorHilos_ = nullptr;
    // El hilo de exportacion escribe el estado y el hilo del editor lo lee:
    // los indicadores son atomicos y el mensaje va con un candado.
    std::atomic<bool> terminado_{false};
    std::atomic<bool> exito_{false};
    mutable std::mutex mutexEstado_;
    std::condition_variable cvEstado_;
    std::string mensajeFinal_;
    // Archivo de traza de esta exportacion (salida de la compilacion).
    std::string rutaLog_;

    void runExportacion();
    void runExportacionInterna();
    bool generarProyectoCMake(const std::string& buildDir);
    bool compilarEngineRuntime(const std::string& buildDir);
    bool compilarJuego(const std::string& buildDir);
    bool copiarAssetsYDependencias(const std::string& buildDir);
    bool copiarDependenciasRuntime(const std::string& buildDir);
    bool empaquetarDistribucion(const std::string& buildDir);
    std::string obtenerExtensionEjecutable() const;
    void log(const std::string& msg);
    void progreso(float p, const std::string& etapa);
    void finalizar(bool ok, const std::string& msg);
};

#endif // GAMEEXPORTER_H