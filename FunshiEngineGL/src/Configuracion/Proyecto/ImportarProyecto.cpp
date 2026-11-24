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
#include "ImportarProyecto.h"

#include <filesystem>
#include <system_error>

#include "../ProjectPaths.h"
#include "ArchivoProyecto.h"

namespace fs = std::filesystem;

namespace ImportarProyecto {
namespace {

bool esDirectorio(const fs::path& p) {
    std::error_code ec;
    return fs::is_directory(p, ec);
}

// Una carpeta es proyecto <nombre> si tiene Memory/ y src<nombre>/.
bool tieneEstructura(const fs::path& carpeta, const std::string& nombre) {
    return !nombre.empty() && esDirectorio(carpeta / "Memory") &&
           esDirectorio(carpeta / ProjectPaths::nombreRaizSrc(nombre));
}

fs::path normalizada(const fs::path& p) {
    std::error_code ec;
    fs::path absoluta = fs::weakly_canonical(p, ec);
    if (ec) absoluta = fs::absolute(p, ec).lexically_normal();
    return absoluta;
}

// true si 'p' es 'raiz' o esta dentro de ella.
bool estaDentroDe(const fs::path& p, const fs::path& raiz) {
    const fs::path a = normalizada(p);
    const fs::path b = normalizada(raiz);
    auto ia = a.begin();
    for (auto ib = b.begin(); ib != b.end(); ++ib, ++ia) {
        if (ia == a.end() || *ia != *ib) return false;
    }
    return true;
}

// El .fnep de la carpeta: <nombreCarpeta>.fnep o, si no, el primero que haya.
bool leerFnep(const fs::path& carpeta, ArchivoProyecto::Datos& datos) {
    std::error_code ec;
    std::string error;
    const fs::path propio = carpeta / (carpeta.filename().string() + ".fnep");
    if (auto d = ArchivoProyecto::leer(propio.string(), error)) {
        datos = *d;
        return true;
    }
    for (fs::directory_iterator it(carpeta, ec), fin; !ec && it != fin; it.increment(ec)) {
        if (it->path().extension() != ".fnep") continue;
        if (auto d = ArchivoProyecto::leer(it->path().string(), error)) {
            datos = *d;
            return true;
        }
    }
    return false;
}

void borrarSinError(const fs::path& p) {
    std::error_code ec;
    fs::remove_all(p, ec);
}

}  // namespace

Origen analizar(const std::string& ruta, const std::string& raizProyectos) {
    Origen origen;
    if (ruta.empty()) return origen;
    fs::path carpeta(ruta);
    std::error_code ec;
    // Se acepta la carpeta del proyecto o su archivo .fnep.
    if (fs::is_regular_file(carpeta, ec) && carpeta.extension() == ".fnep")
        carpeta = carpeta.parent_path();
    if (!esDirectorio(carpeta)) return origen;  // NoExiste
    carpeta = normalizada(carpeta);
    origen.carpeta = carpeta.string();

    ArchivoProyecto::Datos fnep;
    const bool hayFnep = leerFnep(carpeta, fnep);
    if (hayFnep) origen.motor = fnep.motor;

    // El nombre es el de src<Nombre>: el de la carpeta o, si la carpeta se
    // renombro a mano, el que dice el .fnep.
    const std::string deCarpeta = carpeta.filename().string();
    if (tieneEstructura(carpeta, deCarpeta)) {
        origen.nombre = deCarpeta;
    } else if (hayFnep && tieneEstructura(carpeta, fnep.nombre)) {
        origen.nombre = fnep.nombre;
    } else {
        origen.problema = Problema::NoEsProyecto;
        return origen;
    }

    if (!raizProyectos.empty() && estaDentroDe(carpeta, raizProyectos)) {
        origen.problema = Problema::YaEstaEnProyectos;
        return origen;
    }
    origen.problema = Problema::Ninguno;
    return origen;
}

Origen analizar(const std::string& ruta) {
    return analizar(ruta, ProjectPaths::directorioProyects());
}

Problema validarNombre(const std::string& nombre, const std::string& raizProyectos) {
    // Un nombre que empieza con '.' quedaria oculto y choca con la carpeta
    // temporal de la copia.
    if (!ProjectPaths::esNombreValido(nombre) || nombre[0] == '.')
        return Problema::NombreInvalido;
    std::error_code ec;
    if (fs::exists(fs::path(raizProyectos) / nombre, ec)) return Problema::NombreRepetido;
    return Problema::Ninguno;
}

Problema importar(const Origen& origen, const std::string& nombreDestino,
                  const std::string& raizProyectos) {
    if (!origen.valido()) return origen.problema;
    const Problema nombre = validarNombre(nombreDestino, raizProyectos);
    if (nombre != Problema::Ninguno) return nombre;

    std::error_code ec;
    const fs::path raiz(raizProyectos);
    fs::create_directories(raiz, ec);
    const fs::path temporal = raiz / (".importando-" + nombreDestino);
    const fs::path destino = raiz / nombreDestino;
    borrarSinError(temporal);  // resto de una importacion cortada

    // 1. Copia completa a la carpeta oculta. Los enlaces se copian como
    //    enlaces: seguirlos podria copiar media maquina o entrar en un ciclo.
    fs::copy(origen.carpeta, temporal,
             fs::copy_options::recursive | fs::copy_options::copy_symlinks, ec);
    if (ec) {
        borrarSinError(temporal);
        return Problema::ErrorCopia;
    }

    // 2. Con otro nombre: src<Origen> -> src<Destino>.
    if (nombreDestino != origen.nombre) {
        fs::rename(temporal / ProjectPaths::nombreRaizSrc(origen.nombre),
                   temporal / ProjectPaths::nombreRaizSrc(nombreDestino), ec);
        if (ec) {
            borrarSinError(temporal);
            return Problema::ErrorCopia;
        }
    }

    // 3. Un solo .fnep, con el nombre con el que entra.
    for (fs::directory_iterator it(temporal, ec), fin; !ec && it != fin; it.increment(ec)) {
        if (it->path().extension() == ".fnep") {
            std::error_code ignorado;
            fs::remove(it->path(), ignorado);
        }
    }
    ArchivoProyecto::Datos datos;
    datos.nombre = nombreDestino;
    datos.motor = origen.motor;
    std::string error;
    if (!ArchivoProyecto::escribir((temporal / (nombreDestino + ".fnep")).string(), datos, error)) {
        borrarSinError(temporal);
        return Problema::ErrorCopia;
    }

    // 4. Recien ahora aparece en la carpeta de proyectos.
    if (fs::exists(destino, ec)) {
        borrarSinError(temporal);
        return Problema::NombreRepetido;
    }
    fs::rename(temporal, destino, ec);
    if (ec) {
        borrarSinError(temporal);
        return Problema::ErrorCopia;
    }
    return Problema::Ninguno;
}

Problema importar(const Origen& origen, const std::string& nombreDestino) {
    return importar(origen, nombreDestino, ProjectPaths::directorioProyects());
}

}  // namespace ImportarProyecto
