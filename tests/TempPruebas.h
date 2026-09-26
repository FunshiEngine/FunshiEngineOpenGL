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
#ifndef TEMPPRUEBAS_H
#define TEMPPRUEBAS_H

// Utilidad comun de las suites headless: carpetas temporales UNICAS por
// proceso.
//
// Antes cada test usaba una carpeta fija (p. ej.
// temp_directory_path()/funshi_filemanager_tests) con remove_all al inicio.
// Dos ejecuciones simultaneas de ctest en la misma maquina se pisaban: un
// proceso borraba mientras el otro escribia y la suite fallaba o abortaba con
// filesystem_error. Con este helper cada proceso trabaja en su propia carpeta
// y se limpia al salir (RAII), incluida la ruta de fallo (return != 0); solo
// un abort (terminate) puede dejar restos, que se descartan del sistema.

#include <chrono>
#include <filesystem>
#include <random>
#include <string>

namespace TempPruebas {

// Ruta temporal unica para esta ejecucion: base del sistema + nombre + sufijo
// (tiempo + aleatorio). No crea ni borra nada; es el camino para pruebas que
// solo necesitan un archivo suelto.
inline std::filesystem::path rutaUnica(const std::string& nombre) {
    const auto instantes =
        std::chrono::steady_clock::now().time_since_epoch().count();
    std::random_device azar;
    const std::string sufijo =
        std::to_string(instantes) + "-" + std::to_string(azar());
    return std::filesystem::temp_directory_path() / (nombre + "-" + sufijo);
}

// Carpeta temporal unica con limpieza automatica. Crea el directorio en el
// constructor (errores a la luz, igual que los tests originales) y lo elimina
// en el destructor, incluso si el test termina con error. No copiable para
// que dos scopes no compartan la misma carpeta.
class CarpetaPrueba {
public:
    explicit CarpetaPrueba(const std::string& nombre)
        : ruta_(rutaUnica(nombre)) {
        std::filesystem::create_directories(ruta_);
    }

    ~CarpetaPrueba() {
        std::error_code ec;
        std::filesystem::remove_all(ruta_, ec);
    }

    CarpetaPrueba(const CarpetaPrueba&) = delete;
    CarpetaPrueba& operator=(const CarpetaPrueba&) = delete;

    const std::filesystem::path& ruta() const { return ruta_; }

private:
    std::filesystem::path ruta_;
};

} // namespace TempPruebas

#endif // TEMPPRUEBAS_H
