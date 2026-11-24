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
#include "ScriptsTestsDetalle.h"

// Emparejamiento de libjvm y javac: tienen que salir de la MISMA raiz. Con
// dos raices falsas (una con JVM y otra con compilador) el emparejamiento
// tiene que quedarse con la primera que tenga JVM y no mezclar, porque un
// javac de otra version genera bytecode que la JVM rechaza y el fallo sale
// como "no se encontro la clase".
void seccion02() {
    {
        // Arbol falso: jdkA trae las dos cosas, jdkB solo la JVM, jdkC solo
        // javac (no sirve de raiz de JVM).
        const std::string jdkA = "/opt/jdkA";
        const std::string jdkB = "/opt/jdkB";
        const std::string jdkC = "/opt/jdkC";
        auto libjvmEn = [&](const std::string& raiz) -> std::string {
            if (raiz == jdkA) return raiz + "/lib/server/libjvm.so";
            if (raiz == jdkB) return raiz + "/lib/server/libjvm.so";
            return {};
        };
        auto javacEn = [&](const std::string& raiz) -> std::string {
            if (raiz == jdkA) return raiz + "/bin/javac";
            if (raiz == jdkC) return raiz + "/bin/javac";
            return {};
        };

        const ResolucionJdk::Herramientas conPrimero =
            ResolucionJdk::desdeRaices({jdkA, jdkB}, libjvmEn, javacEn);
        CHECK(conPrimero.libjvm == jdkA + "/lib/server/libjvm.so" &&
                  conPrimero.javac == jdkA + "/bin/javac" &&
                  conPrimero.raiz == jdkA,
              "la primera raiz con las dos herramientas manda para ambas");

        const ResolucionJdk::Herramientas sinJavacPrimero =
            ResolucionJdk::desdeRaices({jdkB, jdkA}, libjvmEn, javacEn);
        CHECK(sinJavacPrimero.libjvm == jdkB + "/lib/server/libjvm.so" &&
                  sinJavacPrimero.javac.empty(),
              "una raiz sin javac no se completa con el javac de otra raiz");
        CHECK(sinJavacPrimero.raiz == jdkB,
              "la raiz elegida es la de la JVM, no la del compilador");

        // Una raiz con javac pero sin JVM no es candidata: el .class lo tiene que
        // ejecutar una JVM.
        const ResolucionJdk::Herramientas soloJavac =
            ResolucionJdk::desdeRaices({jdkC}, libjvmEn, javacEn);
        CHECK(soloJavac.libjvm.empty() && soloJavac.javac.empty(),
              "una raiz sin JVM no aporta nada");

        CHECK(ResolucionJdk::desdeRaices({}, libjvmEn, javacEn).libjvm.empty(),
              "sin raices no hay herramientas");
        CHECK(ResolucionJdk::desdeRaices({"", "/opt/vacio"}, libjvmEn, javacEn)
                      .libjvm.empty(),
              "una raiz vacia o sin artefactos se ignora");

        // Raiz deducida de la ruta de la biblioteca: sirve para el caso en que
        // la biblioteca viene dada suelta (FUNSHI_LIBJVM, valor horneado) y hay
        // que buscarle el javac al lado.
        CHECK(ResolucionJdk::raizDesdeLibjvm("/opt/jdk/lib/server/libjvm.so") ==
                  "/opt/jdk",
              "libjvm.so POSIX: la raiz es dos niveles arriba");
        CHECK(ResolucionJdk::raizDesdeLibjvm("C:/jdk/bin/server/jvm.dll") ==
                  "C:/jdk",
              "jvm.dll de Windows: la raiz sube de server y de bin");
        CHECK(ResolucionJdk::raizDesdeLibjvm("C:/jdk/lib/jvm.lib") == "C:/jdk",
              "el .lib de FindJNI cuelga de lib, no de bin/server");
        CHECK(ResolucionJdk::raizDesdeLibjvm("/opt/jdk") == "/opt/jdk",
              "una ruta que ya es de JDK se toma tal cual");
        CHECK(ResolucionJdk::raizDesdeLibjvm("").empty(),
              "sin ruta de biblioteca no hay raiz que deducir");
    }
}

// El valor horneado por CMake es la ruta del ARCHIVO de la biblioteca, no
// una raiz de JDK. Pasado por la lista de raices se descartaba siempre (buscaba
// "<archivo>/lib/server/libjvm.so"), de modo que ni en la maquina donde se
// construyo el motor aportaba nada. Se comprueba que, como ultimo recurso,
// solo cuenta si el archivo existe aqui, y que su raiz deduce el javac.
void seccion03() {
    {
        const std::string horneada = "/opt/jdkH/lib/server/libjvm.so";
        const std::string jdkH = "/opt/jdkH";
        auto javacEn = [&](const std::string& raiz) -> std::string {
            return raiz == jdkH ? raiz + "/bin/javac" : std::string();
        };

        const ResolucionJdk::Herramientas conHorneada =
            ResolucionJdk::conBibliotecaHorneada({}, horneada, true, javacEn);
        CHECK(conHorneada.libjvm == horneada && conHorneada.raiz == jdkH &&
                  conHorneada.javac == jdkH + "/bin/javac",
              "el valor horneado se usa si el archivo existe en esta maquina");

        const ResolucionJdk::Herramientas horneadaAusente =
            ResolucionJdk::conBibliotecaHorneada({}, horneada, false, javacEn);
        CHECK(horneadaAusente.libjvm.empty() &&
                  horneadaAusente.javac.empty() && horneadaAusente.raiz.empty(),
              "el valor horneado no aporta nada si su archivo no existe");

        CHECK(ResolucionJdk::conBibliotecaHorneada({}, "", true, javacEn)
                      .libjvm.empty(),
              "sin valor horneado no hay nada que anadir");

        // Lo que ya salio de una raiz del sistema manda: el horneado es el
        // ultimo recurso, no el primero.
        ResolucionJdk::Herramientas desdeSistema{
            "/usr/lib/jvm/java-17/lib/server/libjvm.so", "/usr/lib/jvm/java-17/bin/javac",
            "/usr/lib/jvm/java-17"};
        const ResolucionJdk::Herramientas sinPisar =
            ResolucionJdk::conBibliotecaHorneada(desdeSistema, horneada, true, javacEn);
        CHECK(sinPisar.libjvm == desdeSistema.libjvm && sinPisar.raiz == desdeSistema.raiz,
              "una raiz del sistema no se reemplaza por el valor horneado");

        // Una raiz sin javac al lado sigue sin javac: el emparejamiento es lo que
        // evita el bytecode que la JVM no entiende.
        CHECK(ResolucionJdk::conBibliotecaHorneada({}, "/opt/jreH/lib/server/libjvm.so",
                                                    true, javacEn)
                      .javac.empty(),
              "el valor horneado no inventa un javac que no tiene al lado");
    }
}

// Que se pueda ejecutar el javac que se paso a Proceso::ejecutar: un nombre
// suelto lo resuelve el PATH, una ruta tiene que existir. El fallo de "no se
// pudo ejecutar" no decia que lo que faltaba era un JDK.
void seccion04() {
    {
        TempPruebas::CarpetaPrueba carpetaJavac("funshi_scripts_javac");
        const auto& raiz = carpetaJavac.ruta();
        CHECK(ResolucionJdk::compiladorEjecutable("javac"),
              "un nombre suelto lo resuelve el PATH");
        CHECK(!ResolucionJdk::compiladorEjecutable(""),
              "sin compilador no hay nada que ejecutar");
        const auto javac = raiz / "javac";
        std::ofstream(javac) << "#!/bin/sh\n";
        CHECK(ResolucionJdk::compiladorEjecutable(javac.string()),
              "una ruta que existe es ejecutable");
        CHECK(!ResolucionJdk::compiladorEjecutable((raiz / "javac.exe").string()),
              "una ruta que no existe no es ejecutable ni con extension de Windows");
        CHECK(!ResolucionJdk::compiladorEjecutable(raiz.string()),
              "una carpeta no es un compilador");
    }
}

// Paridad instalador/motor al detectar un JDK: el .iss solo miraba
// bin\server\jvm.dll, asi que daba por bueno un JRE (que no trae javac) y el
// motor luego no podia compilar ningun .java. El predicado del instalador es
// Pascal Script y no se puede ejecutar fuera de Windows, asi que el contrato
// se comprueba sobre el propio .iss: cada deteccion de JDK pasa por el
// predicado que exige LAS DOS piezas, y no quedan comprobaciones sueltas de
// jvm.dll que puedan reintroducir el falso positivo.
void seccion05() {
    {
        const std::filesystem::path raizRepo =
            std::filesystem::path(__FILE__).parent_path().parent_path();
        const std::filesystem::path iss = raizRepo / "FunshiEngineGL" / "packaging" /
                             "FunshiEngineGL_setup.iss";
        std::ifstream f(iss);
        CHECK(f.good(), "se encuentra FunshiEngineGL_setup.iss para auditarlo");
        std::string texto((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());

        // Se cuentan LINEAS DE CODIGO, no texto: los comentarios del .iss
        // mencionan jvm.dll y javac.exe al explicar el requisito.
        int lineasJdk = 0;
        int lineasJavac = 0;
        int usosPredicado = 0;
        std::istringstream flujo(texto);
        std::string linea;
        while (std::getline(flujo, linea)) {
            if (linea.find("FileExists(") != std::string::npos &&
                linea.find("jvm.dll") != std::string::npos)
                ++lineasJdk;
            if (linea.find("\\bin\\javac.exe") != std::string::npos)
                ++lineasJavac;
            if (linea.find("JdkCompletoEn(") != std::string::npos &&
                linea.find("function") == std::string::npos)
                ++usosPredicado;
        }

        CHECK(usosPredicado >= 4,
              "el instalador detecta el JDK siempre por el mismo predicado "
              "(carpetas comunes, JAVA_HOME del proceso, JAVA_HOME de la maquina "
              "y jre embebido)");
        CHECK(lineasJdk == 1 && lineasJavac == 1,
              "una sola comprobacion de jvm.dll, y es la que exige tambien "
              "bin\\javac.exe (si no, un JRE pasa por JDK y el motor no compila)");
        CHECK(lineasJdk >= lineasJavac,
              "el predicado del instalador no puede exigir javac y olvidar la JVM");
    }
}

// Descubrimiento del toolset en la maquina del usuario: la ruta del
// compilador que hornea el build es la del runner que publico el paquete, y
// en el equipo del usuario no existe. El barrido se prueba con un arbol
// falso porque 'existe' no necesita Windows: se comprueba el contenido, no
// la plataforma.
void seccion06() {
    {
        TempPruebas::CarpetaPrueba carpetaVS("funshi_vcvars_maquina");
        const fs::path raizVS = carpetaVS.ruta() / "VS";
        const auto crearVcvars = [](const fs::path& visualStudio) {
            const fs::path bat =
                visualStudio / "VC" / "Auxiliary" / "Build" / "vcvars64.bat";
            fs::create_directories(bat.parent_path());
            std::ofstream(bat) << "rem falso";
        };

        CHECK(CompilacionCpp::vcvars64EnRaices(std::vector<std::string>{raizVS.string()}).empty(),
              "vcvars: sin arbol de Visual Studio devuelve vacio (no inventa ruta)");

        crearVcvars(raizVS / "2022" / "Community");
        CHECK(CompilacionCpp::vcvars64EnRaices(std::vector<std::string>{raizVS.string()}) ==
                  (raizVS / "2022" / "Community" / "VC" / "Auxiliary" / "Build" /
                   "vcvars64.bat").string(),
              "vcvars: encuentra la edicion completa bajo <raiz>/<anio>/<edicion>");

        // Las Build Tools cuelgan un nivel mas abajo; si el barrido solo mirase
        // dos niveles, el caso mas comun de toolset en una maquina de usuario
        // no se encontraria.
        fs::remove_all(raizVS);
        crearVcvars(raizVS / "2022" / "BuildTools");
        CHECK(CompilacionCpp::vcvars64EnRaices(std::vector<std::string>{raizVS.string()}) ==
                  (raizVS / "2022" / "BuildTools" / "VC" / "Auxiliary" / "Build" /
                   "vcvars64.bat").string(),
              "vcvars: encuentra las Build Tools, que cuelgan igual de dos niveles");

        // Con varias instalaciones gana la primera raiz: por eso el orden de
        // raicesVisualStudio() (entorno, despues Program Files) es el orden de
        // preferencia.
        const fs::path raizA = fs::path(carpetaVS.ruta()) / "VS_A";
        const fs::path raizB = fs::path(carpetaVS.ruta()) / "VS_B";
        crearVcvars(raizB / "2022" / "Community");
        crearVcvars(raizA / "2022" / "Enterprise");
        CHECK(CompilacionCpp::vcvars64EnRaices(std::vector<std::string>{raizA.string(), raizB.string()}) ==
                  (raizA / "2022" / "Enterprise" / "VC" / "Auxiliary" / "Build" /
                   "vcvars64.bat").string(),
              "vcvars: con varias instalaciones gana la primera raiz candidata");

        CHECK(CompilacionCpp::vcvars64EnRaices(std::vector<std::string>{(fs::path(carpetaVS.ruta()) /
                                                  "no_existe").string()})
                  .empty(),
              "vcvars: una raiz inexistente no lanza excepcion ni devuelve ruta");
    }
}

// Que compilador se invoca. El caso que motiva esto es el paquete publicado:
// la ruta horneada es la del runner que compilo y no existe en el equipo del
// usuario, asi que invocarla era garantir un fallo. "No existe" se simula
// con una ruta debajo de un archivo regular (falla en cualquier SO).
void seccion07() {
    {
        TempPruebas::CarpetaPrueba carpetaComp("funshi_elegir_compilador");
        const fs::path existente = carpetaComp.ruta() / "compilador_real.exe";
        { std::ofstream(existente) << "x"; }
        const fs::path tapon = carpetaComp.ruta() / "bloque";
        { std::ofstream(tapon) << "x"; }
        const std::string inexistente = (tapon / "dentro").string();

        CHECK(CompilacionCpp::elegirCompilador("", existente.string(), false) ==
                  existente.string(),
              "compilador: con la ruta horneada presente se respeta (build de desarrollo)");
        CHECK(CompilacionCpp::elegirCompilador("", existente.string(), true) ==
                  existente.string(),
              "compilador: la ruta horneada presente gana a descubrir el toolset");
        CHECK(CompilacionCpp::elegirCompilador("", inexistente, true) == "cl",
              "compilador: sin ruta horneada utilizable y con toolset, usa cl del PATH");
        CHECK(CompilacionCpp::elegirCompilador("", inexistente, false) == "g++",
              "compilador: sin ruta horneada utilizable ni toolset, g++ del PATH");
        CHECK(CompilacionCpp::elegirCompilador("", "", false) == "g++",
              "compilador: sin nada horneado se va al nombre standard");
        CHECK(CompilacionCpp::elegirCompilador("C:/mio/cl.exe", existente.string(), true) ==
                  "C:/mio/cl.exe",
              "compilador: el override del entorno gana siempre, exista o no");
        CHECK(CompilacionCpp::elegirCompilador("\"C:/mio/cl.exe\"", inexistente, false) ==
                  "C:/mio/cl.exe",
              "compilador: al override se le quitan las comillas del literal");
    }
}
