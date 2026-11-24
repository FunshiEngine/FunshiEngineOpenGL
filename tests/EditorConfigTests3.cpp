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

// 6c. Cotejo de prefijos del explorador contra las rutas de la
//     escena. ProjectPaths arma la raiz con '/' y absolutizarRuta
//     concatena con '/', mientras que el explorador publica la ruta con
//     el separador nativo (std::filesystem: '\' en Windows). Antes del
//     fix, mover o renombrar una carpeta devolvia 0 cambios en silencio
//     y las referencias de la escena quedaban apuntando al lugar viejo.
void seccion18() {
    {
        EditorConfig::limpiarRaizAssets();
        const std::string raiz =
            "C:\\base\\FunshiEngine-DATA/Proyects/JuegoX/srcJuegoX";
        EditorConfig::fijarRaizAssets(raiz);

        // Ruta en memoria tal como queda al cargar la escena.
        const std::string enMemoria =
            EditorConfig::absolutizarRuta("Scripts/cpp.cpp");
        CHECK(enMemoria == raiz + "/Scripts/cpp.cpp",
              "absolutizar concatena raiz + '/' + relativa");

        // Prefijo tal como lo arma el explorador: raiz + separador nativo.
        const std::string prefijoExplorador = raiz + "\\Scripts";
        const std::string destino = raiz + "\\Assets\\Scripts";

        const std::string reescrita = EditorConfig::reemplazarPrefijoRuta(
            enMemoria, prefijoExplorador, destino);
#ifdef _WIN32
        CHECK(reescrita == destino + "/cpp.cpp",
              "prefijo con '\\' reescribe la ruta resuelta con '/'");
        // El prefijo con '/' (caso de 6a/6b) tiene que seguir funcionando.
        CHECK(EditorConfig::reemplazarPrefijoRuta(enMemoria, raiz + "/Scripts",
                                                  raiz + "/Assets/Scripts") ==
                  raiz + "/Assets/Scripts/cpp.cpp",
              "el prefijo con '/' no se rompe");
        // Frontera: el prefijo cierra solo si le sigue un separador.
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/ScriptsEx/cpp.cpp", prefijoExplorador, destino)
                  .empty(),
              "'Scripts' no cubre 'ScriptsEx' (frontera con separadores "
              "mixtos)");
        // '\\' cierra el prefijo tambien en rutas con '/' puro (raiz unix).
        CHECK(EditorConfig::reemplazarPrefijoRuta("/dato/a\\b/f.fbx", "/dato/a",
                                                  "/dato/z") ==
                  "/dato/z\\b/f.fbx",
              "en Windows '\\' es separador para el cotejo");
#else
        CHECK(reescrita.empty(),
              "en Linux '\\' NO es separador de rutas");
        CHECK(EditorConfig::reemplazarPrefijoRuta("/dato/a\\b/f.fbx", "/dato/a",
                                                  "/dato/z")
                  .empty(),
              "en Linux '\\' es un caracter normal de nombre");
#endif
        // Mayusculas y minusculas: el sistema de archivos de Windows no las
        // distingue, asi que "assets/scripts" y "Assets/Scripts" son la misma
        // carpeta y el cotejo tiene que reconocerlo (si no, al mover o
        // renombrar con distinta capitalizacion la referencia de la escena se
        // queda con la ruta vieja). En Linux SI se distinguen: "X" y "x" pueden
        // ser dos archivos distintos, asi que ahi el cotejo es sensible.
#ifdef _WIN32
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/assets/scripts/cpp.cpp", raiz + "/Assets/Scripts",
                  raiz + "/Assets/Cpp")
                  == raiz + "/Assets/Cpp/cpp.cpp",
              "en Windows el prefijo se coteja sin distinguir mayusculas");
        CHECK(EditorConfig::relativizarRuta(raiz + "\\ASSETS\\scripts\\a.dll") ==
                  "ASSETS\\scripts\\a.dll",
              "en Windows relativizar no depende de como este escrita la raiz");
#else
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "/assets/scripts/cpp.cpp", raiz + "/Assets/Scripts",
                  raiz + "/Assets/Cpp")
                  .empty(),
              "en Linux el prefijo se coteja distinguiendo mayusculas");
#endif

        // Sin falsos positivos en ninguna plataforma: el prefijo tiene que
        // cerrar en un separador (mismo contrato que 6a).
        CHECK(EditorConfig::reemplazarPrefijoRuta(
                  raiz + "Ab/cpp.cpp", prefijoExplorador, destino)
                  .empty(),
              "srcJuegoX no cubre srcJuegoXAb");

        EditorConfig::limpiarRaizAssets();
    }
}

// Reset (Fase 3): restablecer vuelve a los defaults de fabrica.
void seccion19() {
    {
        EditorConfig cfg;

        // Modifica varios campos a valores no default.
        auto& d = cfg.datos();
        d.idioma = "en";
        d.sensibilidadCamara = 2.5f;
        d.sensibilidadMovimientoCamara = 2.0f;
        d.apariencia.temaClaro = true;
        d.apariencia.blancoYNegro = true;
        d.ventanaCamarasAbierta = false;
        d.gizmoOperacion = 5;
        d.gizmoGlobal = true;
        d.camaraActivaId = 3;
        d.nombreProyecto = "MiProyecto";
        d.estadoVentanas["Estado"] = false;

        cfg.restablecer();

        CHECK(cfg.datos().idioma == "es", "restablecer vuelve el idioma");
        CHECK(cfg.datos().sensibilidadCamara == 0.15f,
              "restablecer vuelve la sensibilidad");
        CHECK(cfg.datos().sensibilidadMovimientoCamara == 1.0f,
              "restablecer vuelve la sensibilidad de movimiento");
        CHECK(!cfg.datos().apariencia.temaClaro,
              "restablecer vuelve el tema");
        CHECK(!cfg.datos().apariencia.blancoYNegro,
              "restablecer vuelve el modo B/N");
        CHECK(cfg.datos().ventanaCamarasAbierta, "restablecer abre la ventana camaras");
        CHECK(cfg.datos().gizmoOperacion == 7, "restablecer vuelve el gizmo");
        CHECK(cfg.datos().gizmoGlobal == false,
              "restablecer vuelve el gizmo a LOCAL");
        CHECK(cfg.datos().camaraActivaId == -1,
              "restablecer vuelve la camara a automatica");
        CHECK(cfg.datos().nombreProyecto.empty(),
              "restablecer deja el proyecto sin abrir (main conserva el actual)");
        CHECK(cfg.datos().estadoVentanas.empty(),
              "restablecer limpia el estado de ventanas");
    }
}

// Escritura atomica + guardado diferido de la config general:
//  - la escritura va a un ".tmp" y se renombra encima (un corte a mitad de
//    escritura deja el JSON original intacto y no deja temporales colgados);
//  - los cambios en vivo de Opciones se encolan y NO reescriben el archivo
//    en cada evento, solo cuando vence el intervalo o en un guardarGeneral()
//    explicito (Ctrl+S, salida, reset).
void seccion20(const fs::path& base) {
    {
        const std::string rutaDif = (base / "ConfiguracionDiferida.json").string();

        EditorConfig cfg;
        cfg.datos().idioma = "es";
        cfg.guardarGeneral(rutaDif);
        CHECK(!fs::exists(rutaDif + ".tmp"),
              "no queda el temporal tras una escritura correcta");

        // Cambio en vivo dentro del intervalo: queda pendiente, no se escribe.
        cfg.datos().idioma = "en";
        cfg.solicitarGuardadoGeneral(rutaDif);
        cfg.volcarGuardadoGeneral();
        EditorConfig lector1;
        lector1.cargarGeneral(rutaDif);
        CHECK(lector1.datos().idioma == "es",
              "dentro del intervalo el guardado diferido no reescribe");

        // Vencido el intervalo, el volcado si escribe (una sola vez basta).
        std::this_thread::sleep_for(EditorConfig::kIntervaloEscritura +
                                    std::chrono::milliseconds(30));
        cfg.volcarGuardadoGeneral();
        EditorConfig lector2;
        lector2.cargarGeneral(rutaDif);
        CHECK(lector2.datos().idioma == "en",
              "vencido el intervalo el volcado persiste el cambio");

        // guardarGeneral() no espera al intervalo: vuelca el pendiente.
        cfg.datos().idioma = "es";
        cfg.solicitarGuardadoGeneral(rutaDif);
        cfg.guardarGeneral(rutaDif);
        EditorConfig lector3;
        lector3.cargarGeneral(rutaDif);
        CHECK(lector3.datos().idioma == "es",
              "guardarGeneral vuelca el pendiente sin esperar al intervalo");
        CHECK(!fs::exists(rutaDif + ".tmp"),
              "ninguna de las escrituras dejo temporales");
    }
}

// 8. Resolucion de la raiz de datos (ProjectPaths). El motor instalado en
//    Program Files no puede escribir junto al ejecutable, asi que la raiz
//    cae a la carpeta de datos del usuario. Lo que se comprueba aqui es el
//    CONTRATO, no el entorno: que la raiz exista y sea escribible, que las
//    funciones derivadas cuelguen de ella y que la migracion no pise datos.
void seccion21(const fs::path& base) {
    {
        const std::string raizDatos = ProjectPaths::directorioBase();
        const std::string original = ProjectPaths::directorioBaseOriginal();

        CHECK(!raizDatos.empty(), "directorioBase no vacio");
        CHECK(!original.empty(), "directorioBaseOriginal no vacio");
        // Termina en FunshiEngine-DATA: las dos son la MISMA raiz, solo cambia donde
        // esta. Si esto se cumpliera, el resto de la comprobacion es coherente.
        const std::string sufijo = "FunshiEngine-DATA";
        auto terminaEn = [&sufijo](const std::string& ruta) {
            return ruta.size() >= sufijo.size() &&
                   ruta.compare(ruta.size() - sufijo.size(), sufijo.size(),
                                sufijo) == 0;
        };
        CHECK(terminaEn(raizDatos), "directorioBase termina en FunshiEngine-DATA");
        CHECK(terminaEn(original), "directorioBaseOriginal termina en FunshiEngine-DATA");

        // Coherencia de datosEnRutaDeUsuario con las dos rutas: solo puede ser
        // true si resolvio a OTRO sitio, y ese otro no puede ser el de siempre.
        const bool enUsuario = ProjectPaths::datosEnRutaDeUsuario();
        CHECK(enUsuario == (raizDatos != original),
              "datosEnRutaDeUsuario refleja si la raiz cambio de ubicacion");

        // La raiz debe existir y ser escribible: es la garantia que evita el
        // bug original (escrituras fallando en silencio en Program Files).
        CHECK(fs::is_directory(raizDatos), "la raiz de datos existe");

        // Toda ruta derivada cuelga de la raiz efectiva, no de la historica.
        const std::string proyecto = "PruebaResolucion";
        CHECK(ProjectPaths::directorioProyecto(proyecto) ==
                  raizDatos + "/Proyects/" + proyecto,
              "directorioProyecto cuelga de directorioBase");
        CHECK(ProjectPaths::directorioConfiguraciones() ==
                  raizDatos + "/Configuraciones",
              "directorioConfiguraciones cuelga de directorioBase");
        CHECK(ProjectPaths::rutaConfiguracionGeneral() ==
                  raizDatos + "/Configuraciones/Configuracion.json",
              "la config general cuelga de directorioBase");
        CHECK(ProjectPaths::rutaImguiIni(proyecto) ==
                  raizDatos + "/Proyects/" + proyecto + "/Memory/imgui.ini",
              "el imgui.ini del proyecto cuelga de directorioBase");

        // Eleccion de la raiz de datos con rutas de verdad. "No escribible" se
        // simula con una ruta debajo de un archivo regular: ahi crear carpetas
        // falla en cualquier sistema y sin depender de permisos, que en Windows
        // no se pueden tocar desde el test.
        const fs::path escribible = fs::path(base) / "carpeta_valida";
        std::error_code ecEs;
        fs::create_directories(escribible, ecEs);
        CHECK(!ecEs, "la carpeta escribible del test se pudo crear");

        const fs::path bloqueante = fs::path(base) / "bloque";
        {
            std::ofstream tapon(bloqueante);
            tapon << "soy un archivo, no una carpeta";
        }
        const std::string noEscribible = (bloqueante / "dentro").string();
        const std::string noEscribible2 = (bloqueante / "otro").string();

        CHECK(ProjectPathsDetalle::elegirRaizDeDatos(escribible.string(), noEscribible)
                  == escribible.string(),
              "si la ruta original admite escritura se usa, aunque haya alternativa");
        CHECK(ProjectPathsDetalle::elegirRaizDeDatos(noEscribible, escribible.string())
                  == escribible.string(),
              "si la ruta original no admite escritura se cae a la del usuario");
        CHECK(ProjectPathsDetalle::elegirRaizDeDatos(noEscribible, "") == noEscribible,
              "sin carpeta de usuario se conserva la ruta historica");
        CHECK(ProjectPathsDetalle::elegirRaizDeDatos(noEscribible, noEscribible2)
                  == noEscribible,
              "si ninguna de las dos sirve se conserva la ruta historica");

        // Migracion: nunca debe pisar datos que ya estan en destino. Se siembra
        // un archivo marcador y se comprueba que sobrevive a la llamada.
        const fs::path sembrado = fs::path(raizDatos) / "marcador_migracion.txt";
        {
            std::ofstream salida(sembrado);
            salida << "no pisar";
        }
        std::string mensajeMigracion = "sin tocar";
        const bool migro =
            ProjectPaths::migrarDatosDesdeRutaOriginal(mensajeMigracion);
        CHECK(fs::exists(sembrado), "la migracion no borra datos existentes");
        CHECK(!migro, "con destino poblado la migracion no copia nada");
        CHECK(mensajeMigracion.empty(),
              "sin migracion el mensaje al usuario queda vacio");
        {
            std::ifstream entrada(sembrado);
            std::string contenido;
            std::getline(entrada, contenido);
            CHECK(contenido == "no pisar", "el contenido del marcador no cambio");
        }
        std::error_code ec;
        fs::remove(sembrado, ec);
    }
}

// 8.b La copia de la migracion, aislada. La migracion completa solo se
//     puede ejecutar cuando la carpeta del ejecutable no es escribible, y en
//     un runner de CI siempre lo es, asi que sin esto la parte que de verdad
//     se ejecuta en un fallo real (copiar el arbol) quedaria sin cubrir.
//     Se comprueba que copia, que trae la estructura anidada y que se niega a
//     pisar un destino poblado.
void seccion22() {
    {
        // Carpeta unica por proceso, como el resto de la suite, para que dos
        // ctest simultaneos no se pisen.
        TempPruebas::CarpetaPrueba carpetaCopia("funshi_copia_migracion");
        const fs::path raizCopia = carpetaCopia.ruta();

        const fs::path origen = raizCopia / "origen";
        fs::create_directories(origen / "Proyects" / "Ejemplo" / "Memory");
        { std::ofstream o(origen / "Configuracion.json"); o << "{}"; }
        { std::ofstream o(origen / "Proyects" / "Ejemplo" / "Memory" / "escena.bin");
          o << "escena"; }

        // Destino vacio: debe copiar, incluida la estructura de directorios.
        const fs::path destino = raizCopia / "destino";
        std::string error = "sin tocar";
        const bool copio = ProjectPaths::copiarArbolSiDestinoVacio(
            origen.string(), destino.string(), error);
        CHECK(copio, "copia el arbol cuando el destino esta vacio");
        CHECK(error.empty(), "sin error cuando la copia funciona");
        CHECK(fs::exists(destino / "Configuracion.json"),
              "llego el archivo de la raiz del arbol");
        CHECK(fs::exists(destino / "Proyects" / "Ejemplo" / "Memory" / "escena.bin"),
              "llego tambien la estructura anidada de directorios");

        // Destino poblado: no debe copiar ni pisar nada.
        const fs::path marcador = destino / "marcador.txt";
        { std::ofstream o(marcador); o << "intacto"; }
        std::string error2 = "sin tocar";
        const bool copio2 = ProjectPaths::copiarArbolSiDestinoVacio(
            origen.string(), destino.string(), error2);
        CHECK(!copio2, "no copia cuando el destino ya esta poblado");
        CHECK(error2.empty(),
              "no es un error que el destino este poblado: no hay nada que avisar");
        {
            std::ifstream e(marcador);
            std::string contenido;
            std::getline(e, contenido);
            CHECK(contenido == "intacto", "el archivo preexistente no se toco");
        }

        // Origen inexistente: se informa por `error` en vez de fingir exito.
        std::string error3;
        const bool copio3 = ProjectPaths::copiarArbolSiDestinoVacio(
            (raizCopia / "no_existe").string(),
            (raizCopia / "destino3").string(), error3);
        CHECK(!copio3, "falla si el origen no es un directorio");
        CHECK(!error3.empty(), "deja el motivo en error");
    }
}

// 9. Carpetas candidatas del log: la de la raiz de datos va primero (ya cae
//    a la carpeta del usuario cuando el ejecutable no admite escritura), y
//    la temporal del sistema es el ultimo recurso. Sin esto, instalado en
//    Program Files el motor se queda sin log ni consola que lo muestre.
void seccion23() {
    {
        const std::string exe = "/opt/FunshiEngineGL";
        const std::string datosUsuario = "/home/usuario/.local/share/FunshiEngineGL/FunshiEngine-DATA";
        const std::string temp = "/tmp";

        const std::vector<std::string> rutas = RutasLog::candidatas(
            datosUsuario, exe, temp);
        CHECK(rutas.size() == 3, "tres candidatas: raiz de datos, ejecutable y temporal");
        CHECK(rutas[0] == (fs::path(datosUsuario) / "logs").string(),
              "la primera candidata es la raiz de datos");
        CHECK(rutas[1] == (fs::path(exe) / "logs").string(), "la segunda, la carpeta del ejecutable");
        CHECK(rutas[2] == (fs::path(temp) / "FunshiEngineGL" / "logs").string(),
              "la tercera, la temporal del motor");

        // Sin raiz de datos no se pierde la carpeta del ejecutable.
        const std::vector<std::string> sinDatos = RutasLog::candidatas("", exe, temp);
        CHECK(sinDatos.size() == 2 && sinDatos[0] == (fs::path(exe) / "logs").string(),
              "sin raiz de datos arranca por la carpeta del ejecutable");

        // Todo vacio: no hay candidatas, el arranque lo reporta como log vacio.
        CHECK(RutasLog::candidatas("", "", "").empty(),
              "sin ninguna ruta no hay candidatas");

        // Candidatas repetidas (raiz de datos y ejecutable iguales) se unifican:
        // el motor prueba la primera que acepte escritura, no la ultima.
        const std::vector<std::string> repetidas =
            RutasLog::candidatas(exe, exe, temp);
        CHECK(repetidas.size() == 2 && repetidas[0] == (fs::path(exe) / "logs").string() &&
                  repetidas[1] == (fs::path(temp) / "FunshiEngineGL" / "logs").string(),
              "una ruta repetida no se prueba dos veces");
    }
}

// 10. La carpeta "logs" es de la raiz de datos, no un proyecto legacy: si
//     no estuviera reservada, la migracion de proyectos antiguos la
//     arrastraba dentro de Proyects/ y el log se escribia a otro sitio del
//     que el motor anuncia.
void seccion24() {
    {
        CHECK(!ProjectPaths::esNombreValido("logs"),
              "'logs' no es un proyecto: queda reservada en la raiz de datos");
        CHECK(ProjectPaths::esNombreValido("Loges del Pueblo"),
              "un nombre de proyecto corriente sigue siendo valido");
    }
}
