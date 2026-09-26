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
#ifndef PROJECT_MANAGER_H
#define PROJECT_MANAGER_H

#include <string>
#include <vector>
#include <optional>

#include "ProjectPaths.h"

// Gestiona proyectos: descubrimiento, CRUD, migración de estructura antigua.
// Sin side effects en constructores; toda E/S es explícita.
class ProjectManager {
public:
    struct Proyecto {
        std::string nombre;
        std::string ruta;           // <Proyects>/<nombre>
        std::string rutaSrc;        // <Proyects>/<nombre>/src<nombre>
        bool valido = false;        // true si existe y tiene estructura mínima
    };

    // Asegura estructura base (Proyects/, Configuraciones/, Exportaciones/)
    // y ejecuta migración automática de estructura antigua -> nueva.
    // Idempotente: seguro llamar al arrancar.
    static void asegurarEstructuraBase();

    // Crea proyecto "NuevoProyecto" si Proyects/ está vacío.
    // Devuelve true si se creó, false si ya había proyectos o fallo.
    static bool crearProyectoPorDefecto();

    // Lista proyectos descubiertos en Proyects/ (solo directorios válidos).
    static std::vector<Proyecto> descubrirProyectos();

    // Busca proyecto por nombre exacto.
    static std::optional<Proyecto> buscarProyecto(const std::string& nombre);

    // Crea un nuevo proyecto con estructura completa.
    // Devuelve false si nombre inválido, ya existe, o fallo E/S.
    static bool crearProyecto(const std::string& nombre);

    // Renombra proyecto en disco: <viejo> -> <nuevo> (incluye src<viejo> -> src<nuevo>).
    // Devuelve false si origen no existe, destino existe, nombre inválido, o fallo E/S.
    static bool renombrarProyecto(const std::string& viejo, const std::string& nuevo);

    // Elimina proyecto completo (recursivo). Irreversible.
    // Devuelve false si no existe o fallo E/S.
    static bool eliminarProyecto(const std::string& nombre);

    // Verifica si un proyecto tiene estructura mínima válida.
    static bool esProyectoValido(const std::string& nombre);

private:
    // Migración interna: mueve carpetas sueltas bajo MotorGrafico/ a Proyects/
    static void migrarProyectosAntiguos();

    // Migración interna: mueve Configuracion.json raíz a Configuraciones/
    static void migrarConfiguracionGlobal();

    // Crea estructura de carpetas para un proyecto (sin validar nombre).
    static bool crearEstructuraProyecto(const std::string& nombre);
};

#endif // PROJECT_MANAGER_H