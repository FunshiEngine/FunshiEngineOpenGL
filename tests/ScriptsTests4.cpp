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

// --- Contrato de compilacion de los scripts C++ (CRT compartido) -------------
// El .dll del script comparte heap con el engine a traves de la reflexion
// (std::string, std::vector<DefCampo> y los std::function de cada campo se
// alocan de un lado y se liberan del otro), asi que los dos modulos tienen que
// usar el MISMO runtime de C++. Este test no puede observar la corrupcion de
// heap (haria falta MSVC y cargar el .dll en runtime), pero si verifica el
// contrato de flags: sin /MD el heap vuelve a separarse y el crash regresa.
void testContratoCompilacion() {
    const std::string msvc =
        CompilacionCpp::flagsFamilia(CompilacionCpp::Familia::Msvc, "MiClase");

    CHECK(CompilacionCpp::tieneFlag(msvc, "/MD") ||
              CompilacionCpp::tieneFlag(msvc, "/MDd"),
          "MSVC: el script se compila con el CRT dinamico (/MD), nunca con el "
          "estatico");
    CHECK(!CompilacionCpp::tieneFlag(msvc, "/MT") &&
              !CompilacionCpp::tieneFlag(msvc, "/MTd"),
          "MSVC: el CRT estatico (/MT) le da a la .dll su propio heap y rompe "
          "la ABI con el engine");
    CHECK(CompilacionCpp::tieneFlag(msvc, CompilacionCpp::runtimeFlag()),
          "MSVC: el flag de runtime es el mismo que usa el engine en esta "
          "configuracion (/MD o /MDd)");
    CHECK(CompilacionCpp::tieneFlag(msvc, "/EHsc"),
          "MSVC: sin /EHsc la primera excepcion del script mata el proceso");
    CHECK(msvc.find("/DFUNSHI_NOMBRE_CLASE=MiClase") != std::string::npos,
          "MSVC: el nombre de clase del script llega por define");
    CHECK(CompilacionCpp::tieneFlag(msvc, "/link"),
          "MSVC: la linea termina pasando opciones al linker");
    // H-15: el export de la fabrica queda pedido en el link para que los
    // fuentes con el template viejo (sin FUNSHI_COMPORTAMIENTO_EXPORT)
    // exporten igual el simbolo en MSVC, donde un extern "C" pelado no se
    // exporta solo.
    CHECK(msvc.find("/EXPORT:FUNSHI_CREAR_COMPORTAMIENTO") != std::string::npos,
          "MSVC: la linea pide exportar la fabrica en el link (H-15)");
    CHECK(!CompilacionCpp::tieneFlag(msvc, "-std=c++17"),
          "MSVC: la linea no lleva flags de GCC");

    const std::string gcc =
        CompilacionCpp::flagsFamilia(CompilacionCpp::Familia::Gcc, "MiClase");

    CHECK(CompilacionCpp::tieneFlag(gcc, "-shared"),
          "GCC: el script se compila como biblioteca compartida");
    CHECK(CompilacionCpp::tieneFlag(gcc, "-fPIC"),
          "GCC: -fPIC, porque el artefacto se carga con dlopen");
    CHECK(!CompilacionCpp::tieneFlag(gcc, "/MD") &&
              !CompilacionCpp::tieneFlag(gcc, "/nologo") &&
              !CompilacionCpp::tieneFlag(gcc, "/EHsc"),
          "GCC: la linea no lleva flags de MSVC");

    // H-2: la familia sale del TOOLCHAIN, no de la plataforma. Un build MinGW en
    // Windows recibia los flags de MSVC y el script no compilaba nunca.
    CHECK(CompilacionCpp::familiaDe("cl") == CompilacionCpp::Familia::Msvc,
          "cl.exe es familia MSVC");
    CHECK(CompilacionCpp::familiaDe(
              "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/"
              "MSVC/14.40.33807/bin/Hostx64/x64/cl.exe") ==
              CompilacionCpp::Familia::Msvc,
          "la ruta completa de cl.exe es familia MSVC");
    CHECK(CompilacionCpp::familiaDe("C:/msys64/mingw64/bin/c++.exe") ==
              CompilacionCpp::Familia::Gcc,
          "MinGW c++.exe en Windows es familia GCC, no MSVC");
    CHECK(CompilacionCpp::familiaDe("g++") == CompilacionCpp::Familia::Gcc,
          "g++ es familia GCC");
    CHECK(CompilacionCpp::familiaDe("/usr/bin/clang++") ==
              CompilacionCpp::Familia::Gcc,
          "clang++ es familia GCC");
    CHECK(CompilacionCpp::familiaDe("/usr/bin/g++-14") ==
              CompilacionCpp::Familia::Gcc,
          "un g++ con sufijo de version sigue siendo familia GCC");

    // Los ARGV del proceso hijo (H-3 nivel 2, sin shell): el include y la
    // salida son de la familia, no de la plataforma (MinGW recibia /I, /Fo
    // y /Fe, y no compilaba nunca). Cada token va crudo y separado — quien
    // cita al armar la linea de Windows es Proceso::citar, y en POSIX los
    // argumentos viajan tal cual.
    auto contiene = [](const std::vector<std::string>& valores,
                       const std::string& buscado) {
        return std::find(valores.begin(), valores.end(), buscado) !=
               valores.end();
    };
    auto arrancaCon = [](const std::vector<std::string>& valores,
                         const std::string& prefijo) {
        for (const std::string& tok : valores)
            if (tok.rfind(prefijo, 0) == 0) return true;
        return false;
    };
    auto tieneShell = [](const std::vector<std::string>& valores) {
        for (const std::string& tok : valores)
            if (tok.find('>') != std::string::npos ||
                tok.find('&') != std::string::npos)
                return true;
        return false;
    };

    CompilacionCpp::DatosComando datos;
    datos.nombreClase = "MiClase";
    datos.fuente = "C:/Proyectos/Nuevo Proyecto/src/Scripts/MiClase.cpp";
    datos.dirSrc = "C:/engine/src";
    datos.dirObjetos = "C:/Temp/funshi_scripts";
    datos.artefacto = "C:/Temp/funshi_scripts/script_1.dll";

    datos.compilador = "C:/msys64/mingw64/bin/c++.exe";
    const std::vector<std::string> argvMinGW =
        CompilacionCpp::argumentosCompilacion(datos);
    CHECK(contiene(argvMinGW, "-IC:/engine/src") && !arrancaCon(argvMinGW, "/I"),
          "MinGW: las cabeceras entran con -I, no con /I");
    CHECK(contiene(argvMinGW, "-o") && !arrancaCon(argvMinGW, "/Fo") &&
              !arrancaCon(argvMinGW, "/Fe"),
          "MinGW: la salida va con -o, no con /Fo ni /Fe");
    CHECK(!contiene(argvMinGW, "/nologo") && contiene(argvMinGW, "-shared"),
          "MinGW: sin flags de MSVC en ninguna parte de los argumentos");
    CHECK(contiene(argvMinGW, datos.fuente),
          "la ruta de un proyecto con espacios es UN SOLO argumento (el "
          "shell de antes la cortaba en el primer espacio)");

    datos.compilador = "cl.exe";
    const std::vector<std::string> argvMsvc =
        CompilacionCpp::argumentosCompilacion(datos);
    CHECK(contiene(argvMsvc, "/IC:/engine/src"),
          "MSVC: las cabeceras entran con /I");
    CHECK(contiene(argvMsvc, "/FoC:/Temp/funshi_scripts\\") &&
              contiene(argvMsvc, "/FeC:/Temp/funshi_scripts/script_1.dll"),
          "el /Fo conserva la barra final que cl.exe pide para el directorio "
          "y /Fe va pegado al artefacto");
    CHECK(!contiene(argvMsvc, "-o") && !contiene(argvMsvc, "-shared"),
          "MSVC: sin flags de GCC en ninguna parte de los argumentos");
    CHECK(!tieneShell(argvMinGW) && !tieneShell(argvMsvc),
          "los argumentos no llevan redireccion ni operadores de shell: eso "
          "lo hace Proceso por handles/fd");

    // El juego que usa ESTE build tambien cumple el contrato: la familia sale
    // del compilador horneado por CMake (FUNSHI_CXX_COMPILER), no del SO.
    const std::string actual = CompilacionCpp::flagsCompilador("MiClase");
    if (CompilacionCpp::familiaCompilador() == CompilacionCpp::Familia::Msvc) {
        CHECK(CompilacionCpp::tieneFlag(actual, CompilacionCpp::runtimeFlag()),
              "MSVC: el build en uso compila los scripts con el runtime de C++ "
              "del engine");
    } else {
        CHECK(CompilacionCpp::tieneFlag(actual, "-shared"),
              "GCC/Clang: el build en uso compila los scripts como biblioteca "
              "compartida");
    }

    // Comparacion por tokens: "/MD" no puede dar positivo sobre "/MDd".
    CHECK(!CompilacionCpp::tieneFlag("/MDd /EHsc", "/MD"),
          "el chequeo de flags compara tokens completos");

    seccion01();

    seccion02();

    seccion03();

    seccion04();

    seccion05();

    // vcvars64Ruta: el recorte por parent_path() tenia que terminar. En
    // MinGW/libstdc++ `path("C:\\\\").parent_path()` devuelve `C:\\\\` (nunca
    // vacio), asi que el bucle giraba para siempre y BackendCpp se colgaba al
    // compilar un script en Windows+MinGW. Si estas llamadas no vuelven, el
    // test se cuelga y ctest lo marque como timeout: esa es la senal.
    CHECK(CompilacionCpp::vcvars64Ruta("C:/msys64/mingw64/bin/c++.exe").empty(),
          "vcvars: un compilador GCC no busca vcvars64.bat (guarda de familia)");
    CHECK(CompilacionCpp::vcvars64Ruta("/ruta/de/prueba/cl.exe").empty(),
          "vcvars: una ruta MSVC inexistente recorta hasta la raiz y termina "
          "(sin guarda el bucle era infinito)");
    CHECK(CompilacionCpp::vcvars64Ruta("g++").empty(),
          "vcvars: un compilador bare de familia GCC ni recorre el filesystem");

    seccion06();

    seccion07();

    // --- Harvest del entorno de vcvars (H-3 nivel 2) --------------------------
    // La receta es CRUDA para cmd.exe (sin citar: cmd no entiende el escape
    // \" de la CRT) y no lleva datos de usuario: solo la ruta balanceada.
    CHECK(CompilacionCpp::comandoEntornoVcvars(
              "C:/VS/Auxiliary/Build/vcvars64.bat") ==
              "/U /d /c call \"C:/VS/Auxiliary/Build/vcvars64.bat\" && set",
          "el harvest arma la receta cruda `call ... && set` con /U (salida "
          "UTF-16) y /d (sin AutoRun)");
    // Parser de `set /U`: el banner se descarta, el valor puede contener '='
    // (se corta en el primero), las claves quedan ordenadas y el bloque es
    // multi-sz (NUL por par + NUL final). Sin >=3 variables no hay bloque.
    const std::wstring setDePrueba =
        L"Microsoft Visual Studio Version 17.0\r\n"
        L"VC variables configured for: x64 native tools\r\n"
        L"PATH=C:\\msys64\\mingw64\\bin;C:\\Windows\r\n"
        L"INCLUDE=C:\\VS\\include;D:\\con=igual\\inc\r\n"
        L"SONDA=ni\x00F1o caf\x00E9\r\n";
    const std::wstring bloque = CompilacionCpp::bloqueDesdeSet(setDePrueba);
    CHECK(!bloque.empty(), "el parseo del set /U devuelve un bloque");
    CHECK(bloque.find(L"PATH=C:\\msys64\\mingw64\\bin;C:\\Windows") !=
              std::wstring::npos,
          "cada CLAVE=valor del set entra en el bloque");
    CHECK(bloque.find(L"INCLUDE=C:\\VS\\include;D:\\con=igual\\inc") !=
              std::wstring::npos,
          "un valor con '=' se corta en el primer '=' de la clave");
    CHECK(bloque.find(L"Microsoft Visual") == std::wstring::npos &&
              bloque.find(L"configured") == std::wstring::npos,
          "el banner de vcvars se descarta (no tiene forma CLAVE=valor)");
    CHECK(bloque.find(L"SONDA=ni\x00F1o caf\x00E9") != std::wstring::npos,
          "el UTF-16 sobrevive byte a byte (sin round-trip por narrow)");
    CHECK(bloque.find(L"INCLUDE=") < bloque.find(L"PATH="),
          "el bloque sale ordenado alfabeticamente, como pide CreateProcess");
    CHECK(bloque.size() >= 2 && bloque[bloque.size() - 1] == L'\0' &&
              bloque[bloque.size() - 2] == L'\0',
          "el bloque termina doble NUL (multi-sz)");
    CHECK(CompilacionCpp::bloqueDesdeSet(L"una=variable\r\n").empty(),
          "con menos de 3 variables el bloque se toma vacio (cmd no corrio)");
    CHECK(CompilacionCpp::bloqueDesdeSet(std::wstring()).empty(),
          "una salida vacia da bloque vacio");
#if defined(_WIN32)
    // Fallback real: un vcvars inexistente no puede dar entorno.
    // BackendCpp avisa y compila con el entorno heredado en ese caso.
    CHECK(CompilacionCpp::entornoVcvars("C:/no/existe/vcvars64.bat").empty(),
          "un vcvars inexistente devuelve bloque vacio (modo heredado)");
#endif
}

// --- Contrato del sondeo de toolchain (H-14 + H-3 nivel 2) -------------------
// El sondeo original pasaba por std::system con `> /dev/null`, un redirect de
// POSIX: cmd.exe lo toma como ruta inexistente y el chequeo fallaba aunque la
// herramienta este instalada (scripts-java-tests se saltaba en Windows con el
// JDK presente). Con H-3 nivel 2 no hay shell: SondeoToolchain::sondear()
// ejecuta la herramienta directamente y el log ES el dispositivo nulo de la
// plataforma, abierto por Proceso. Este test fija ese contrato y lo ejercita
// de verdad con un proceso real.
void testSondeoToolchain(const std::string& exePropio) {
#if defined(_WIN32)
    CHECK(std::string(SondeoToolchain::dispositivoNulo()) == "NUL",
          "H-14: en Windows el dispositivo nulo del sondeo es NUL");
#else
    CHECK(std::string(SondeoToolchain::dispositivoNulo()) == "/dev/null",
          "H-14: en POSIX el dispositivo nulo del sondeo es /dev/null");
#endif
    CHECK(!SondeoToolchain::sondear("", "-version"),
          "el sondeo de una herramienta vacia falla sin ejecutar nada");

    // Spawn REAL a traves del runner (sin shell): el propio binario, que con
    // el guard `--hijo` de main() termina en 0. Es la comprobacion de que
    // Proceso::ejecutar lanza, redirige y devuelve el codigo en esta misma
    // plataforma — sin depender de una herramienta externa ni de su exit
    // code (cl /? no es verificable donde no hay MSVC).
    CHECK(SondeoToolchain::sondear(exePropio, "--hijo"),
          "el sondeo corre un proceso real sin shell (el propio binario)");
}

int main(int argc, char** argv) {
    // Modo hijo: cuando SondeoToolchain::sondear() lanza este mismo binario
    // con `--hijo`, termina en 0 sin volver a sondear (evita la recursion).
    if (argc >= 2 && std::string(argv[1]) == "--hijo")
        return 0;

    testValoresPorDefecto();
    testLecturaEscritura();
    testGruposVector();
    testObjetoResolucion();
    testComponenteResolucion();
    testSerializacionBinaria();
    testAlinearValores();
    testAlinearValoresConTipoDistinto();
    testLecturaCorrupta();
    testEntradaDeScripts();
    testAudioIniciadoPorScripts();
    testContratoCompilacion();
    testSondeoToolchain(argv[0]);

    std::cout << "ScriptsTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}
