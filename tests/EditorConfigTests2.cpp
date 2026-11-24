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

// 4c. Colores del cielo (fondoSuperior/fondoInferior): compatibilidad con
//     "fondo" antiguo (se copia a ambos), modo B/N fuerza ambos a
//     blanco/negro segun el tema, y round-trip de colores personalizados.
void seccion10(const std::string& rutaGeneral) {
    {
        // Compatibilidad: archivo viejo con "fondo" -> se copia a superior e inferior.
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"fondo\": [0.1, 0.2, 0.3]}\n}\n";
        }
        EditorConfig cfgCompat;
        cfgCompat.cargarGeneral(rutaGeneral);
        CHECK(cfgCompat.datos().apariencia.fondoSuperior[0] == 0.1f &&
                  cfgCompat.datos().apariencia.fondoSuperior[1] == 0.2f &&
                  cfgCompat.datos().apariencia.fondoSuperior[2] == 0.3f,
              "compat: fondo -> fondoSuperior");
        CHECK(cfgCompat.datos().apariencia.fondoInferior[0] == 0.1f &&
                  cfgCompat.datos().apariencia.fondoInferior[1] == 0.2f &&
                  cfgCompat.datos().apariencia.fondoInferior[2] == 0.3f,
              "compat: fondo -> fondoInferior");

        // Modo B/N oscuro: ambos negros (en color efectivo).
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"blancoYNegro\": true, \"temaClaro\": false,"
                 "\"fondoSuperior\": [0.8, 0.8, 0.8],"
                 "\"fondoInferior\": [0.2, 0.2, 0.2]}\n}\n";
        }
        EditorConfig cfgBNOscuro;
        cfgBNOscuro.cargarGeneral(rutaGeneral);
        float sup[3], inf[3];
        AparienciaUtil::fondoEfectivoSuperior(cfgBNOscuro.datos().apariencia, sup);
        AparienciaUtil::fondoEfectivoInferior(cfgBNOscuro.datos().apariencia, inf);
        CHECK(sup[0] == 0.0f && inf[0] == 0.0f,
              "B/N oscuro: color efectivo superior/inferior negros");

        // Modo B/N claro: ambos blancos (en color efectivo).
        {
            std::ofstream f(rutaGeneral, std::ios::trunc);
            f << "{\n  \"version\": 1,\n  \"apariencia\": {"
                 "\"blancoYNegro\": true, \"temaClaro\": true,"
                 "\"fondoSuperior\": [0.1, 0.1, 0.1],"
                 "\"fondoInferior\": [0.5, 0.5, 0.5]}\n}\n";
        }
        EditorConfig cfgBNClaro;
        cfgBNClaro.cargarGeneral(rutaGeneral);
        AparienciaUtil::fondoEfectivoSuperior(cfgBNClaro.datos().apariencia, sup);
        AparienciaUtil::fondoEfectivoInferior(cfgBNClaro.datos().apariencia, inf);
        CHECK(sup[0] == 1.0f && inf[0] == 1.0f,
              "B/N claro: color efectivo superior/inferior blancos");

        // Colores personalizados round-trip (modo normal, no B/N).
        {
            EditorConfig cfgColor;
            cfgColor.datos().apariencia.fondoSuperior[0] = 0.2f;
            cfgColor.datos().apariencia.fondoSuperior[1] = 0.3f;
            cfgColor.datos().apariencia.fondoSuperior[2] = 0.8f;
            cfgColor.datos().apariencia.fondoInferior[0] = 0.8f;
            cfgColor.datos().apariencia.fondoInferior[1] = 0.4f;
            cfgColor.datos().apariencia.fondoInferior[2] = 0.2f;
            cfgColor.guardarGeneral(rutaGeneral);

            EditorConfig cfgColor2;
            cfgColor2.cargarGeneral(rutaGeneral);
            CHECK(cfgColor2.datos().apariencia.fondoSuperior[0] == 0.2f &&
                      cfgColor2.datos().apariencia.fondoSuperior[1] == 0.3f &&
                      cfgColor2.datos().apariencia.fondoSuperior[2] == 0.8f,
                  "roundtrip fondoSuperior personalizado");
            CHECK(cfgColor2.datos().apariencia.fondoInferior[0] == 0.8f &&
                      cfgColor2.datos().apariencia.fondoInferior[1] == 0.4f &&
                      cfgColor2.datos().apariencia.fondoInferior[2] == 0.2f,
                  "roundtrip fondoInferior personalizado");
        }
    }
}

// 5. Nuevo sistema de guardado por proyecto:
// Cada proyecto genera su carpeta en FunshiEngine-DATA, con Memory (escena, config, imgui)
// y su hermano srcProyectName como raiz del explorador de archivos.
// La config general vive en la raiz de FunshiEngine-DATA (hermana de los proyectos).
void seccion11() {
    {
        const std::string baseMotor = EditorConfig::directorioBaseDatos();
        CHECK(!baseMotor.empty(), "directorioBaseDatos no vacio");

        const std::string proyNombre = "JuegoPrueba";
        const std::string proyDir  = EditorConfig::directorioProyecto(proyNombre);
        const std::string memDir   = EditorConfig::directorioMemory(proyNombre);
        const std::string srcDir   = EditorConfig::directorioSrc(proyNombre);
        const std::string rootName = EditorConfig::nombreRaizSrc(proyNombre);

        CHECK(rootName == "srcJuegoPrueba",              "nombreRaizSrc correcto");
        CHECK(proyDir == baseMotor + "/Proyects/" + proyNombre,   "directorioProyecto en Proyects/");
        CHECK(memDir  == proyDir + "/Memory",            "directorioMemory dentro del proyecto");
        CHECK(srcDir  == proyDir + "/srcJuegoPrueba",    "directorioSrc hermano de Memory");

        // Config general: en <base>/Configuraciones/Configuracion.json
        CHECK(EditorConfig::rutaConfiguracionGeneral() == baseMotor + "/Configuraciones/Configuracion.json",
              "rutaConfiguracionGeneral en Configuraciones/");
        // Config del proyecto: dentro de Memory del proyecto.
        CHECK(EditorConfig::rutaConfiguracionProyecto(proyNombre) ==
              memDir + "/ConfiguracionProyecto.json",
              "rutaConfiguracionProyecto dentro de Memory");

    // 5b. Rutas que el exportador copia a la distribucion del juego standalone.
    // El proyecto viaja COMPLETO con la disposicion del motor: el binario del
    // juego resuelve FunshiEngine-DATA/ junto a si mismo, asi que escenas, Memory y
    // src<nombre>/ tienen que quedar bajo Proyects/<nombre>/.
    {
        const std::string nombre = "JuegoExport";
        const std::string salida = EditorConfig::directorioExportacion("JuegoExportado");
        const std::vector<std::pair<std::string, std::string>> rutas =
            rutasDatosProyecto(nombre, salida);

        auto destinoDe = [&rutas](const std::string& origen) {
            for (const auto& par : rutas)
                if (par.first == origen) return par.second;
            return std::string();
        };

        CHECK(destinoDe(EditorConfig::directorioProyecto(nombre)) ==
                  salida + "/FunshiEngine-DATA/Proyects/" + nombre,
              "el proyecto se copia completo bajo Proyects/ del juego");
        // El material clave del juego queda cubierto por esa copia: todos sus
        // origenes son subcarpetas del proyecto.
        const std::string prefijo = EditorConfig::directorioProyecto(nombre) + "/";
        for (const std::string clave :
             {EditorConfig::directorioMemory(nombre),
              EditorConfig::rutaConfiguracionProyecto(nombre)}) {
            CHECK(clave.rfind(prefijo, 0) == 0,
                  "el origen vive dentro del proyecto: " + clave);
        }
        CHECK(directorioScriptsProyecto(nombre) ==
                  EditorConfig::directorioSrc(nombre) + "/Scripts",
              "los fuentes de script estan en src<nombre>/Scripts");
        CHECK(nombreProyectoDesdeRuta(EditorConfig::directorioProyecto(nombre)) ==
                  nombre,
              "el nombre del proyecto es el de su carpeta");
    }

        CHECK(EditorConfig::rutaSceneBBDD(proyNombre) ==
              memDir + "/Binarios/SceneBBDDObjetos.txt",
              "rutaSceneBBDD dentro de Memory/Binarios");
        CHECK(EditorConfig::rutaSceneDir(proyNombre) == memDir + "/Binarios/Scene/",
              "rutaSceneDir dentro de Memory/Binarios/Scene/");
        CHECK(EditorConfig::rutaImguiIni(proyNombre) == memDir + "/imgui.ini",
              "rutaImguiIni dentro de Memory");

        // Asegurar que la creacion de estructura crea las carpetas en disco
        EditorConfig::asegurarEstructuraProyecto(proyNombre);
        CHECK(fs::is_directory(memDir + "/Binarios/Scene"),
              "asegurarEstructuraProyecto creo Memory/Binarios/Scene");
        CHECK(fs::is_directory(srcDir),
              "asegurarEstructuraProyecto creo srcJuegoPrueba");
        // Las carpetas de assets las crea el usuario: el motor no las crea.
        CHECK(!fs::is_directory(srcDir + "/Sonidos"),
              "el motor no crea la carpeta de sonidos");
        CHECK(!fs::is_directory(srcDir + "/Scripts"),
              "el motor no crea la carpeta de scripts");
        CHECK(!fs::is_directory(proyDir + "/Mods"),
              "el motor no crea la carpeta de mods");

        // Limpieza de prueba
        std::error_code ec;
        fs::remove_all(proyDir, ec);
    }
}

// 5b. Las carpetas de assets las crea el usuario: el motor no mueve lo que
//     el usuario tenga en la raiz del proyecto.
void seccion12() {
    {
        const std::string proyNombre = "JuegoMigracion";
        const std::string proyDir = EditorConfig::directorioProyecto(proyNombre);
        const std::string sonidosRaiz = proyDir + "/Sonidos";
        std::error_code ec;
        fs::create_directories(sonidosRaiz, ec);
        { std::ofstream out(sonidosRaiz + "/tema.wav"); out << "clip"; }

        EditorConfig::asegurarEstructuraProyecto(proyNombre);

        CHECK(fs::is_directory(sonidosRaiz),
              "el motor deja la carpeta de assets donde la puso el usuario");
        CHECK(fs::exists(sonidosRaiz + "/tema.wav"),
              "el motor no mueve los archivos de assets del usuario");

        fs::remove_all(proyDir, ec);
    }
}

// 5c. Renombre fisico de proyecto: mueve la carpeta raiz y la raiz src
//     (src<viejo> -> src<nuevo>); no hace nada si falta el origen o si el
//     destino ya existe (main conmuta en ese caso).
void seccion13() {
    {
        const std::string viejo = "JuegoRenombrado";
        const std::string nuevo = "JuegoRenombradoV2";
        const std::string dirViejo = EditorConfig::directorioProyecto(viejo);
        const std::string dirNuevo = EditorConfig::directorioProyecto(nuevo);
        std::error_code ec;
        EditorConfig::asegurarEstructuraProyecto(viejo);
        CHECK(EditorConfig::renombrarProyecto("", nuevo) == false,
              "renombrar con origen vacio falla");
        CHECK(EditorConfig::renombrarProyecto(viejo, viejo) == false,
              "renombrar al mismo nombre falla");
        CHECK(EditorConfig::renombrarProyecto(viejo, nuevo),
              "renombrar mueve la carpeta del proyecto");
        CHECK(!fs::exists(dirViejo), "la carpeta vieja desaparece");
        CHECK(fs::is_directory(dirNuevo), "la carpeta nueva existe");
        CHECK(fs::is_directory(dirNuevo + "/src" + nuevo),
              "la raiz src tambien cambia de nombre");
        CHECK(!fs::exists(dirNuevo + "/src" + viejo),
              "la raiz src vieja no queda como fantasma");
        CHECK(EditorConfig::renombrarProyecto(viejo, nuevo) == false,
              "sin origen ya no se puede renombrar de nuevo");
        fs::remove_all(dirNuevo, ec);
    }
}

// 5c-bis. Archivo .fnep: se crea dentro de la carpeta del proyecto,
//     abrirlo resuelve ese proyecto, sigue al proyecto al renombrarlo y se
//     genera para proyectos viejos que no lo tienen.
void seccion14(const fs::path& base) {
    {
        const std::string nombre = "JuegoFnep";
        const std::string renombrado = "JuegoFnepV2";
        const std::string dir = EditorConfig::directorioProyecto(nombre);
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::remove_all(EditorConfig::directorioProyecto(renombrado), ec);
        CHECK(ProjectManager::crearProyecto(nombre), "se crea el proyecto del .fnep");
        const std::string fnep = ArchivoProyecto::rutaDe(nombre);
        CHECK(fs::exists(fnep) && fs::path(fnep).parent_path() == fs::path(dir),
              "el .fnep vive dentro de la carpeta del proyecto");
        std::string error;
        auto abierto = ArchivoProyecto::resolverApertura(fnep, error);
        CHECK(abierto && *abierto == nombre, "abrir el .fnep resuelve su proyecto");

        const fs::path afuera = base / "copia.fnep";
        fs::copy_file(fnep, afuera, fs::copy_options::overwrite_existing, ec);
        CHECK(!ArchivoProyecto::resolverApertura(afuera.string(), error) && !error.empty(),
              "un .fnep copiado fuera de su proyecto no abre otro por error");

        { std::ofstream out(afuera); out << R"({"formato":"fnep","version":99,"nombre":"X"})"; }
        CHECK(!ArchivoProyecto::leer(afuera.string(), error),
              "una version futura del formato se rechaza con un error claro");
        { std::ofstream out(afuera); out << "no es json"; }
        CHECK(!ArchivoProyecto::leer(afuera.string(), error), "un .fnep corrupto se rechaza");

        CHECK(ProjectManager::renombrarProyecto(nombre, renombrado), "se renombra el proyecto");
        const std::string fnepNuevo = ArchivoProyecto::rutaDe(renombrado);
        abierto = ArchivoProyecto::resolverApertura(fnepNuevo, error);
        CHECK(abierto && *abierto == renombrado, "el .fnep sigue al proyecto renombrado");
        CHECK(!fs::exists(EditorConfig::directorioProyecto(renombrado) + "/" + nombre + ".fnep"),
              "no queda el .fnep con el nombre viejo");

        fs::remove(fnepNuevo, ec);
        ProjectManager::asegurarEstructuraBase();
        CHECK(fs::exists(fnepNuevo), "un proyecto sin .fnep lo recibe al arrancar");
        fs::remove_all(EditorConfig::directorioProyecto(renombrado), ec);
    }
}

// 5d. Eliminacion de proyecto: borra la carpeta completa (escena, src,
//     config). No toca nada si el proyecto no existe o el nombre es vacio.
void seccion15() {
    {
        const std::string nombre = "JuegoAEliminar";
        const std::string dirProyecto = EditorConfig::directorioProyecto(nombre);
        std::error_code ec;
        EditorConfig::asegurarEstructuraProyecto(nombre);
        CHECK(EditorConfig::eliminarProyecto("") == false,
              "eliminar con nombre vacio falla");
        CHECK(EditorConfig::eliminarProyecto("Inexistente") == false,
              "eliminar un proyecto inexistente falla");
        CHECK(fs::is_directory(dirProyecto), "el proyecto existe antes de eliminar");
        CHECK(EditorConfig::eliminarProyecto(nombre),
              "eliminar retira la carpeta del proyecto");
        CHECK(!fs::exists(dirProyecto), "la carpeta del proyecto desaparece");
        CHECK(EditorConfig::eliminarProyecto(nombre) == false,
              "una vez eliminado ya no se puede eliminar de nuevo");
    }
}

// 6a. Contexto de rutas de la serializacion portable: sin raiz fijada las
//     rutas pasan tal cual (passthrough, igual que antes del cambio).
void seccion16() {
    {
        EditorConfig::limpiarRaizAssets();
        CHECK(EditorConfig::hayRaizAssets() == false,
              "sin proyecto no hay raiz de assets");
        CHECK(EditorConfig::relativizarRuta("/a/b/MiModelo.fbx") == "/a/b/MiModelo.fbx",
              "sin raiz no se relativiza");
        CHECK(EditorConfig::absolutizarRuta("Carpetas/MiModelo.fbx") ==
                  "Carpetas/MiModelo.fbx",
              "sin raiz no se absolutiza");
        CHECK(EditorConfig::reemplazarPrefijoRuta("x/UnArchivo.fbx", "x", "y") ==
                  "y/UnArchivo.fbx",
              "reemplazo de prefijo puro funciona sin contexto");
        CHECK(EditorConfig::reemplazarPrefijoRuta("zzz/UnArchivo.fbx", "x", "y")
                  .empty(),
              "prefijo que no cierra en separador no reemplaza (x vs zzz)");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/otro/a.fbx", "/ruta", "/ln")
                  .empty(),
              "sin match devuelve vacio");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/ruta/a.fbx", "/ruta", "/ruta")
                  .empty(),
              "reemplazo identico no hace nada");
    }
}

// 6b. Con raiz de assets fijada (src<nombre> del proyecto abierto): las
//     rutas de la escena se guardan relativas y se resuelven al cargar.
//     Las escenas legacy (absolutas) se dejan intactas.
void seccion17() {
    {
        const std::string raiz = "/dato/FunshiEngine-DATA/JuegoX/srcJuegoX";
        EditorConfig::fijarRaizAssets(raiz);
        CHECK(EditorConfig::hayRaizAssets(), "con proyecto hay raiz de assets");

        const std::string abs = raiz + "/Modelos/Auto/model.fbx";
        const std::string rel = EditorConfig::relativizarRuta(abs);
        CHECK(rel == "Modelos/Auto/model.fbx",
              "ruta bajo la raiz se guarda relativa");
        CHECK(EditorConfig::absolutizarRuta(rel) == abs,
              "la relativa vuelve a absoluta al cargar");

        CHECK(EditorConfig::relativizarRuta("/dato/Otro/fuera.fbx") ==
                  "/dato/Otro/fuera.fbx",
              "ruta fuera de la raiz se conserva absoluta");
        CHECK(EditorConfig::absolutizarRuta("C:\\escena\\legacy\\x.dds") ==
                  "C:\\escena\\legacy\\x.dds",
              "absoluta legacy (windows) no se toca");
        CHECK(EditorConfig::absolutizarRuta("/abs/legacy/x.dds") ==
                  "/abs/legacy/x.dds",
              "absoluta legacy (unix) no se toca");

        // reemplazarPrefijoRuta NO depende del contexto: reescribe el prefijo
        // de una ruta absoluta (base de la actualizacion tras mover/renombrar).
        CHECK(EditorConfig::reemplazarPrefijoRuta(abs, raiz + "/Modelos",
                                                  raiz + "/Assets/Modelos") ==
                  raiz + "/Assets/Modelos/Auto/model.fbx",
              "reescribe prefijo de carpeta movida");

        EditorConfig::limpiarRaizAssets();
        CHECK(EditorConfig::hayRaizAssets() == false,
              "limpiar deja de relativizar");
        CHECK(EditorConfig::absolutizarRuta("Modelos/Auto/model.fbx") ==
                  "Modelos/Auto/model.fbx",
              "sin raiz la relativa almacenada queda como estaba");
    }
}
