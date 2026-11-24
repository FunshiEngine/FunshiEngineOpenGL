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
#include "ScriptsRuntimeTestsDetalle.h"

int main() {
    // El skip es por FAMILIA de toolchain, no por SO (H-14): con MSVC el
    // compilador necesita el entorno de Visual Studio (vcvars: INCLUDE, LIB,
    // link.exe), asi que desde un shell normal no hay forma de correrlo. Con
    // GCC/Clang el test corre en cualquier plataforma, Windows incluido
    // (MinGW): la familia decide los flags, no el sistema operativo.
    if (CompilacionCpp::familiaCompilador() == CompilacionCpp::Familia::Msvc) {
        std::cout << "scripts-runtime-tests: SKIP (toolchain MSVC: requiere "
                     "el entorno de Visual Studio)."
                  << std::endl;
        return 77;
    }
    if (!hayCompilador()) {
        std::cout << "scripts-runtime-tests: SKIP (no hay compilador C++)."
                  << std::endl;
        return 77;
    }

    // 1. Escribir el fuente en un directorio temporal (unico por proceso,
    // limpieza automatica al salir); `ec` lo usan los remove_all posteriores.
    TempPruebas::CarpetaPrueba carpetaDir("funshi_scripts_runtime_test");
    const fs::path dir = carpetaDir.ruta();
    std::error_code ec;
    const std::string fuente = (dir / "MiPrueba.cpp").string();
    std::string error;

    ComportamientoCargado comportamiento;
    const std::string clase = "MiPrueba";

    // 2. Compilar y cargar por primera vez.
    escribirFuente(fuente, fuenteScript(clase));
    bool ok = ScriptRuntime::compilarYCargar(fuente, clase, comportamiento,
                                             error);
    CHECK(ok, "compilarYCargar exitoso");
    if (ok) {
        CHECK(comportamiento.valido(), "comportamiento valido (instancia creada)");
        CHECK(comportamiento.lenguaje == "cpp", "lenguaje = cpp");
        CHECK(comportamiento.campos.size() == 3,
              "exactamente 3 campos reflejados (velocidad/vidas/pasos)");
        CHECK(!error.empty() == false, "sin error al compilar");
    } else {
        std::cout << "  Error del backend: " << error << std::endl;
        CHECK(false, "no hubo error de compilacion (ver stdout)");
    }

    if (comportamiento.valido()) {
        // 3. Inyectar valores SerializeField y verificarlos por lectura.
        std::vector<ValorCampo> valores = extraerCampos(comportamiento);
        for (auto& v : valores) {
            if (v.nombre == "velocidad") v.contenido = 7.0f;
            if (v.nombre == "vidas") v.contenido = 9;
        }
        inyectarCampos(comportamiento, valores);

        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "velocidad")
                CHECK(casiIgual(v.como<float>(), 7.0f),
                      "velocidad inyectada = 7");
            if (v.nombre == "vidas")
                CHECK(v.como<int>() == 9, "vidas inyectadas = 9");
        }

        // 4. Ciclo de vida: onStart + onUpdate + onStop.
        ScriptRuntime::llamarInicio(comportamiento, nullptr);
        ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
        ScriptRuntime::llamarActualizar(comportamiento, nullptr, 0.016f);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 102, "onStart(100)+2x onUpdate(+1) = pasos 102");
        }
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Inicio);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores)
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 112,
                      "el callback de inicio de contacto llega al script C++");
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Persistencia);
        ScriptRuntime::llamarContacto(comportamiento, nullptr, nullptr, nullptr,
                                      TipoContacto::Fin);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores)
            if (v.nombre == "pasos")
                CHECK(v.como<int>() == 162,
                      "los callbacks de persistencia y fin llegan al script C++");
        ScriptRuntime::llamarDetener(comportamiento, nullptr);
        valores = extraerCampos(comportamiento);
        for (const auto& v : valores) {
            if (v.nombre == "vidas")
                CHECK(v.como<int>() == -1, "onStop deja vidas = -1");
        }
    }

    // Una excepción de código de usuario queda atrapada en la frontera del
    // backend, se publica una vez y bloquea callbacks posteriores.
    {
        const std::string fuenteFalla = (dir / "ScriptFalla.cpp").string();
        std::string contenido = fuenteScript("ScriptFalla");
        const std::string include = "#include \"Behaviour/IScriptBehaviour.h\"\n";
        contenido.insert(include.size(), "#include <stdexcept>\n");
        const std::string inicial = "pasos = 100;";
        const size_t posicion = contenido.find(inicial);
        if (posicion != std::string::npos)
            contenido.replace(posicion, inicial.size(),
                              "throw std::runtime_error(\"fallo callback\");");
        escribirFuente(fuenteFalla, contenido);
        ComportamientoCargado falla;
        std::string errorFalla;
        const bool cargada = ScriptRuntime::compilarYCargar(
            fuenteFalla, "ScriptFalla", falla, errorFalla);
        CHECK(cargada, "se carga el script que arroja desde onStart");
        if (cargada) {
            ScriptRuntime::llamarInicio(falla, nullptr);
            CHECK(falla.falloRuntime && falla.callbackFallido == "onStart" &&
                      falla.errorRuntime.find("fallo callback") !=
                          std::string::npos,
                  "la excepción estándar deja el fallo runtime en el callback");
            const auto eventos = RegistroDiagnosticos::global().ultimos(20);
            CHECK(!eventos.empty() &&
                      eventos.back().tipoExcepcion == "SCRIPT_CALLBACK_FAILED" &&
                      eventos.back().mensaje.find(fuenteFalla) !=
                          std::string::npos,
                  "la excepción del callback se publica con ruta y tipo");
            ScriptRuntime::llamarActualizar(falla, nullptr, 0.016f);
            CHECK(falla.falloRuntime && falla.callbackFallido == "onStart",
                  "el callback latched no vuelve a invocarse");
            ScriptRuntime::descargar(falla, nullptr);
        }
    }

    {
        const std::string fuenteFalla = (dir / "ScriptFallaNoEstandar.cpp").string();
        std::string contenido = fuenteScript("ScriptFallaNoEstandar");
        const std::string include = "#include \"Behaviour/IScriptBehaviour.h\"\n";
        contenido.insert(include.size(), "#include <stdexcept>\n");
        const std::string inicial = "pasos = 100;";
        const size_t posicion = contenido.find(inicial);
        if (posicion != std::string::npos)
            contenido.replace(posicion, inicial.size(), "throw 17;");
        escribirFuente(fuenteFalla, contenido);
        ComportamientoCargado falla;
        std::string errorFalla;
        const bool cargada = ScriptRuntime::compilarYCargar(
            fuenteFalla, "ScriptFallaNoEstandar", falla, errorFalla);
        CHECK(cargada, "se carga el script que arroja una excepción no estándar");
        if (cargada) {
            ScriptRuntime::llamarInicio(falla, nullptr);
            CHECK(falla.falloRuntime && falla.errorRuntime ==
                      "excepcion C++ no estandar",
                  "la excepción no estándar queda contenida en BackendCpp");
            ScriptRuntime::descargar(falla, nullptr);
        }
    }

    // 5. Segundo componente sobre el MISMO fuente: la cola de compilacion de
    //    la escena lo entrega con un ComportamientoCargado vacio (estado por
    //    componente), mientras el primero sigue cargado en este proceso. El
    //    artefacto ya esta al dia, asi que no hay que volver a enlazarlo:
    //    reescribir una imagen que esta cargada es un error de escritura que
    //    el enlazador reporta como permiso denegado.
    if (comportamiento.valido()) {
        std::error_code ecArtefacto;
        const auto mtimeAntes =
            fs::last_write_time(comportamiento.artefacto, ecArtefacto);
        CHECK(!ecArtefacto, "el artefacto del primer componente existe");

        const std::string claveCacheAnterior =
            fs::weakly_canonical(fuente).string() + "|" +
            BackendCpp::compiladorRuta() + "|" +
            CompilacionCpp::flagsCompilador(std::string());
        const fs::path artefactoCacheAnterior =
            fs::path(BackendCpp::cacheDir()) /
            ("script_" +
             std::to_string(std::hash<std::string>{}(claveCacheAnterior)) +
             fs::path(comportamiento.artefacto).extension().string());
        CHECK(artefactoCacheAnterior.string() != comportamiento.artefacto,
              "la clave actual del cache no coincide con la version sin API");
        if (artefactoCacheAnterior.string() != comportamiento.artefacto) {
            std::error_code ecCopia;
            fs::copy_file(comportamiento.artefacto, artefactoCacheAnterior,
                          fs::copy_options::overwrite_existing, ecCopia);
            CHECK(!ecCopia,
                  "se puede preparar un artefacto de cache sin version API");
            if (!ecCopia)
                fs::last_write_time(
                    artefactoCacheAnterior,
                    fs::file_time_type::clock::now() +
                        std::chrono::hours(1),
                    ecCopia);
        }

        ComportamientoCargado segundo;
        std::string errorSegundo;
        const bool okSegundo = ScriptRuntime::compilarYCargar(
            fuente, clase, segundo, errorSegundo);
        CHECK(okSegundo, "segundo componente sobre el mismo fuente carga bien");
        if (!okSegundo)
            std::cout << "  Error del backend: " << errorSegundo << std::endl;
        CHECK(segundo.valido(), "segundo comportamiento valido");
        CHECK(segundo.artefacto != artefactoCacheAnterior.string(),
              "un binario del cache anterior no se carga aunque este vigente");
        if (!ecArtefacto) {
            std::error_code ecDespues;
            const auto mtimeDespues =
                fs::last_write_time(comportamiento.artefacto, ecDespues);
            CHECK(!ecDespues && mtimeDespues == mtimeAntes,
                  "el artefacto no se volvio a escribir (sigue al dia)");
        }
        ScriptRuntime::descargar(segundo, nullptr);
    }

    // 6. Hot reload: reescribir el fuente agregando un campo nuevo y tocando
    // el mtime; descargar y recargar. Los valores conocidos se conservan.
    if (comportamiento.valido()) {
        std::string fuente2 =
            "#include \"Behaviour/IScriptBehaviour.h\"\n"
            "class FUNSHI_NOMBRE_CLASE : public IScriptBehaviour {\n"
            "public:\n"
            "    float velocidad = 2.0f;\n"
            "    int vidas = 3;\n"
            "    int pasos = 0;\n"
            "    int nuevos = 0;\n"
            "    REFLECT_INICIO(FUNSHI_NOMBRE_CLASE)\n"
            "        REFLECT_CAMPO(velocidad)\n"
            "        REFLECT_CAMPO(vidas)\n"
            "        REFLECT_CAMPO(pasos)\n"
            "        REFLECT_CAMPO(nuevos)\n"
            "    REFLECT_FIN\n"
            "    void onStart(GameObject* o) override { (void)o; }\n"
            "    void onUpdate(GameObject* o, float d) override { (void)o; (void)d; }\n"
            "    void onStop(GameObject* o) override { (void)o; }\n"
            "    std::vector<::ReflejoScripts::DefCampo>\n"
            "    camposReflejados() const override { return reflexion(); }\n"
            "};\n"
            "extern \"C\" IScriptBehaviour* FUNSHI_CREAR_COMPORTAMIENTO(\n"
            "    const MotorScript::ApiScriptGameObject* api) {\n"
            "    (void)api;\n"
            "    return new FUNSHI_NOMBRE_CLASE();\n"
            "}\n";
        escribirFuente(fuente, fuente2);
        tocarFuente(fuente);

        CHECK(ScriptRuntime::cambioElFuente(comportamiento),
              "cambioElFuente detecta el mtime nuevo");

        std::vector<ValorCampo> antes = extraerCampos(comportamiento);
        ScriptRuntime::descargar(comportamiento, nullptr);
        CHECK(!comportamiento.valido(), "descargar invalida el comportamiento");

        ComportamientoCargado reload;
        ok = ScriptRuntime::compilarYCargar(fuente, clase, reload, error);
        CHECK(ok, "recarga tras hot reload");
        if (ok) {
            CHECK(reload.campos.size() == 4, "4 campos tras agregar 'nuevos'");
            // Los valores antiguos se conservan (emparejado por nombre).
            inyectarCampos(reload, antes);
            std::vector<ValorCampo> despues = extraerCampos(reload);
            for (const auto& v : despues) {
                if (v.nombre == "velocidad")
                    CHECK(casiIgual(v.como<float>(), 7.0f),
                          "velocidad conservada tras hot reload");
                if (v.nombre == "nuevos")
                    CHECK(v.como<int>() == 0, "'nuevos' default = 0");
            }
            ScriptRuntime::descargar(reload, nullptr);
        }
    }

    // 7. Cabeceras del motor ausentes: el backend avisa con su propio mensaje
    //    en vez de lanzar el compilador contra un -I a una ruta inexistente
    //    (que solo daria el "No such file or directory" del toolchain).
    {
        const std::string fuenteSinCabeceras =
            (dir / "SinCabeceras.cpp").string();
        escribirFuente(fuenteSinCabeceras, fuenteScript("SinCabeceras"));
        const std::string rutaMuerta = (dir / "no_existe").string();
        fijarVariable("FUNSHI_SRC_DIR", rutaMuerta.c_str());
        ComportamientoCargado sinCabeceras;
        std::string errorCabeceras;
        const bool okSin = ScriptRuntime::compilarYCargar(
            fuenteSinCabeceras, "SinCabeceras", sinCabeceras, errorCabeceras);
        fijarVariable("FUNSHI_SRC_DIR", nullptr);
        CHECK(!okSin, "sin cabeceras del motor no se compila el script");
        CHECK(!sinCabeceras.valido(), "no queda comportamiento cargado");
        CHECK(errorCabeceras.find("cabeceras") != std::string::npos,
              "el aviso es del motor (habla de las cabeceras), no del "
              "compilador");
    }

    // 8. Limpieza.
    fs::remove_all(dir, ec);

    std::cout << "ScriptsRuntime: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos == 0 ? 0 : 1;
}
