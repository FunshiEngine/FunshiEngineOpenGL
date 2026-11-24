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
#include "EditorConfigTestsDetalle.h"

// 11. La carpeta global de mods del motor NO es un proyecto legacy: la raiz
//     de datos la crea en cada arranque (asegurarEstructuraBase) y la
//     migracion de proyectos antiguos la movia a Proyects/, de modo que el
//     listado mostraba un proyecto falso llamado "Mods" y la carpeta real
//     de mods volvia a aparecer vacia en la raiz.
void seccion25() {
    {
        CHECK(!ProjectPaths::esNombreValido("Mods"),
              "'Mods' no es un proyecto: queda reservada en la raiz de datos");
        CHECK(ProjectPaths::esNombreValido("Mods del Juego"),
              "un proyecto del usuario puede llamarse parecido sin chocar");
        // La cache de mods (CargadorMods::directorioCacheMods) tambien vive en
        // la raiz de datos y se crea al cargar mods, antes de elegir proyecto:
        // sin reserva, la misma migracion la movia a Proyects/ como un
        // proyecto falso llamado "cache".
        CHECK(!ProjectPaths::esNombreValido("cache"),
              "'cache' no es un proyecto: es la cache de mods, reservada");

        // Migracion con rutas inyectadas: la carpeta reservada se queda en la
        // raiz y el proyecto legacy de verdad sigue migrando a Proyects/.
        TempPruebas::CarpetaPrueba carpetaMigracion("funshi_migracion_reservados");
        const fs::path raizMigracion = carpetaMigracion.ruta();
        fs::create_directories(raizMigracion / "Mods");
        fs::create_directories(raizMigracion / "cache" / "mods");
        fs::create_directories(raizMigracion / "MiJuego" / "Memory");
        fs::create_directories(raizMigracion / "MiJuego" / "srcMiJuego");

        ProjectManager::migrarProyectosAntiguosEn(
            raizMigracion.string(), (raizMigracion / "Proyects").string());

        CHECK(fs::is_directory(raizMigracion / "Mods"),
              "la carpeta global de mods se queda en la raiz de datos");
        CHECK(!fs::is_directory(raizMigracion / "Proyects" / "Mods"),
              "la migracion no crea un proyecto llamado Mods");
        CHECK(fs::is_directory(raizMigracion / "cache" / "mods"),
              "la cache de mods se queda en la raiz de datos");
        CHECK(!fs::is_directory(raizMigracion / "Proyects" / "cache"),
              "la migracion no crea un proyecto llamado cache");
        CHECK(fs::is_directory(raizMigracion / "Proyects" / "MiJuego"),
              "el proyecto legacy de verdad si migra a Proyects/");
        CHECK(!fs::exists(raizMigracion / "MiJuego"),
              "el proyecto legacy sale de la raiz de datos");
    }
}

int main() {
    // Carpeta temporal unica por proceso: crea y se limpia al salir (RAII).
    TempPruebas::CarpetaPrueba carpetaBase("funshi_editorconfig_tests");
    const fs::path base = carpetaBase.ruta();
    const std::string rutaGeneral  = (base / "Configuracion.json").string();
    const std::string proyNombreTest = "TestProyecto";
    const std::string rutaProyecto = (base / "ConfiguracionProyecto.json").string();

    seccion01(rutaGeneral, proyNombreTest, rutaProyecto);

    seccion02(rutaGeneral);

    seccion03();

    seccion04(rutaGeneral);

    seccion05(rutaGeneral, proyNombreTest, rutaProyecto);

    seccion06(base, proyNombreTest);

    seccion07(base, rutaGeneral, proyNombreTest);

    seccion08(proyNombreTest, rutaProyecto);

    seccion09(rutaGeneral);

    seccion10(rutaGeneral);

    seccion11();

    seccion12();

    seccion13();

    seccion14(base);

    seccion15();

    seccion16();

    seccion17();

    seccion18();

    seccion19();

    seccion20(base);

    seccion21(base);

    seccion22();

    seccion23();

    seccion24();

    seccion25();

    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "EDITORCONFIG TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
