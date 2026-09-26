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

#include <string>
#include <functional>
#include <thread>

// Exportador de juegos standalone: genera un proyecto CMake temporal,
// compila el juego (recompila scripts de usuario) y empaqueta
// ejecutable + assets + dependencias en una carpeta de distribución.
class GameExporter {
public:
    enum class Plataforma {
        Linux,
        Windows
    };

    struct Config {
        std::string proyectoOrigen;       // ruta al proyecto en MotorGrafico/Proyects/
        std::string nombreEjecutable;     // nombre del .exe / binario (sin extensión)
        std::string nombreProyectoExportado; // nombre de la carpeta de exportación
        Plataforma plataforma = Plataforma::Linux;
        std::string directorioSalida;     // MotorGrafico/Exportaciones/<nombre>/
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

private:
    Config cfg_;
    std::thread hiloExportacion_;
    bool terminado_ = false;
    bool exito_ = false;
    std::string mensajeFinal_;

    void runExportacion();
    bool generarProyectoCMake(const std::string& buildDir);
    bool compilarEngineRuntime(const std::string& buildDir);
    bool compilarScriptsUsuario(const std::string& buildDir);
    bool compilarJuego(const std::string& buildDir);
    bool copiarAssetsYDependencias(const std::string& buildDir);
    bool copiarDependenciasRuntime(const std::string& buildDir);
    bool empaquetarDistribucion(const std::string& buildDir);
    std::string obtenerExtensionEjecutable() const;
    std::string obtenerToolchainCMake() const;
    void log(const std::string& msg);
    void progreso(float p, const std::string& etapa);
    void finalizar(bool ok, const std::string& msg);
};

#endif // GAMEEXPORTER_H