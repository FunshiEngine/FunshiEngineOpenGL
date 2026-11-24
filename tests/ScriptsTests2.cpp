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

// Escenario exacto del crash que se corrigio: la escena se carga SIN defs
// (el script todavia no esta compilado) y el emparejado con los campos reales
// ocurre despues, al compilar. Si ese emparejado no ocurre, el arbol guardado
// conserva el cardinal que tenia en el archivo y el inspector lo indexa por
// posicion contra los defs: lectura fuera de rango.
void testAlinearValores() {
    std::cout << "-- alinearValores: emparejar el arbol de la escena con los "
                 "campos del script"
              << std::endl;

    const std::vector<DefCampo>& defs = defsDePrueba();
    const std::size_t n = static_cast<std::size_t>(kCantidadCampos);

    // 1. Escena guardada con MENOS campos de los que expone el script ahora
    //    (se agrego un SerializeField despues). Este es el caso que crasheaba.
    {
        std::vector<ValorCampo> leidos = valoresPorDefecto(defs);
        CHECK(leidos.size() == n, "el arbol por defecto tiene un valor por campo");
        leidos.resize(3); // solo vidas, velocidad y peso

        const std::vector<ValorCampo> alineados = alinearValores(leidos, defs);
        CHECK(alineados.size() == n,
              "tras alinear hay un valor por campo, nunca mas corto");

        bool orden = true;
        for (std::size_t i = 0; i < n && i < alineados.size(); ++i)
            orden = orden && alineados[i].nombre == defs[i].nombre;
        CHECK(orden, "el resultado va en el mismo orden que los defs");

        // Los preexistentes conservan su tipo; los nuevos toman el de SU campo
        // (no el del ultimo leido), que es lo que evita el std::get sobre una
        // alternativa equivocada.
        if (alineados.size() == n) {
            CHECK(alineados[0].tag == TagTipo::Entero &&
                      alineados[1].tag == TagTipo::Flotante &&
                      alineados[2].tag == TagTipo::Doble,
                  "los campos preexistentes conservan su tipo");
            CHECK(alineados[3].tag == TagTipo::Booleano,
                  "un booleano faltante se rellena como booleano");
            CHECK(alineados[static_cast<std::size_t>(kIndiceMisil)].tag ==
                      TagTipo::Grupo,
                  "un grupo faltante se rellena como grupo");
            CHECK(alineados[15].tag == TagTipo::Objeto,
                  "un GameObject faltante se rellena como objeto");
            CHECK(alineados[16].tag == TagTipo::Objetos,
                  "un vector de objetos faltante se rellena como vector");
        }
    }

    // 2. Campo con valor EDITADO: debe sobrevivir al emparejado, no perder su
    //    dato porque el resto del arbol se haya rellenado.
    {
        std::vector<ValorCampo> leidos = valoresPorDefecto(defs);
        leidos[0].como<int>() = 42;
        leidos.resize(1);

        const std::vector<ValorCampo> alineados = alinearValores(leidos, defs);
        if (alineados.size() == n) {
            CHECK(alineados[0].como<int>() == 42,
                  "el valor editado del campo que si existe se conserva");
        } else {
            CHECK(false, "arbol de tamanho inesperado al conservar el valor");
        }
    }

    // 3. Reordenamiento: los nombres no coinciden con las posiciones.
    {
        std::vector<ValorCampo> leidos = valoresPorDefecto(defs);
        std::reverse(leidos.begin(), leidos.end());

        const std::vector<ValorCampo> alineados = alinearValores(leidos, defs);
        CHECK(alineados.size() == n, "reordenar no cambia el cardinal");
        if (alineados.size() == n) {
            bool nombres = true;
            for (std::size_t i = 0; i < n; ++i)
                nombres = nombres && alineados[i].nombre == defs[i].nombre;
            CHECK(nombres,
                  "tras reordenar cada valor vuelve a su campo por nombre");
        }
    }

    // 4. Campo RETIRADO del script: el valor guardado se descarta.
    {
        std::vector<ValorCampo> leidos = valoresPorDefecto(defs);
        std::vector<DefCampo> menosCampos(defs.begin(), defs.end() - 1);
        // El ultimo campo es 'enemigos' (vector de GameObject), no un texto:
        // hay que escribir en la alternativa que le corresponde, o el
        // std::get lanzaria bad_variant_access... que es exactamente el fallo que
        // esta suite verifica que ya no puede llegar al usuario.
        leidos.back().como<std::vector<std::string>>() = {"basura"};

        const std::vector<ValorCampo> alineados =
            alinearValores(leidos, menosCampos);
        CHECK(alineados.size() == menosCampos.size(),
              "un campo retirado no deja un valor huerfano");
        bool conservaRetirado = false;
        for (const ValorCampo& v : alineados)
            conservaRetirado =
                conservaRetirado || v.nombre == defs.back().nombre;
        CHECK(!conservaRetirado, "el valor del campo retirado se descarta");
    }

    // 5. Sin defs (script sin SerializeField): el arbol se devuelve intacto,
    //    sin inventar entradas ni recortar.
    {
        const std::vector<DefCampo> sinCampos;
        std::vector<ValorCampo> leidos = valoresPorDefecto(defs);
        leidos.resize(4);
        const std::vector<ValorCampo> alineados = alinearValores(leidos, sinCampos);
        CHECK(alineados.empty(),
              "sin campos reflejados el arbol alineado queda vacio");
    }
}

// El script cambio el TIPO de un campo entre la guardado y la carga (por
// ejemplo, un texto que pasa a ser un entero). El valor guardado no se puede
// reinterpretar: la alternativa activa del variant no corresponde a la
// etiqueta del campo. Si se conservara tal cual, el inspector haria un
// std::get sobre la alternativa equivocada y lanzaria
// std::bad_variant_access DENTRO del render de ImGui (el motor aborta).
void testAlinearValoresConTipoDistinto() {
    std::cout << "-- alinearValores: un campo que cambio de tipo toma el "
                 "valor por defecto del campo actual"
              << std::endl;

    DefCampo def;
    def.nombre = "campo";
    def.tag = TagTipo::Entero;
    const std::vector<DefCampo> defs{def};

    ValorCampo leido;
    leido.nombre = "campo";
    leido.tag = TagTipo::Texto;
    leido.contenido = std::string("hola");

    const std::vector<ValorCampo> alineados =
        alinearValores({leido}, defs);
    CHECK(alineados.size() == 1, "el campo queda alineado");
    if (alineados.size() == 1) {
        CHECK(alineados[0].tag == TagTipo::Entero,
              "la etiqueta pasa a ser la del campo actual");
        // El acceso no puede lanzar: la etiqueta y el contenido coinciden.
        CHECK(alineados[0].como<int>() == 0,
              "el valor que no se puede reinterpretar pasa al valor por defecto");
    }
}

// --- Lectura de un archivo corrupto -------------------------------------------
// La longitud y la etiqueta las controla el archivo. Si estan corruptas, la
// lectura se corta con un error limpio: nunca se reserva una memoria absurda
// (que abortaba el motor) ni se deja el payload sin consumir (lo que venia
// detras era basura).

void testLecturaCorrupta() {
    std::cout << "-- lectura corrupta: una longitud o una etiqueta invalida se "
                 "corta sin abortar"
              << std::endl;

    TempPruebas::CarpetaPrueba carpeta("funshi_reflexion_corrupta");
    const std::filesystem::path ruta = carpeta.ruta() / "corrupto.bin";

    // 1. Cantidad de valores enorme.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const uint32_t n = 0xFFFFFFFFu;
        out.write(reinterpret_cast<const char*>(&n), sizeof(n));
        out.close();

        std::ifstream in(ruta, std::ios::binary);
        const std::vector<ValorCampo> leidos = cargarValoresCampos(in);
        CHECK(leidos.empty(), "una cantidad enorme de valores se rechaza");
        CHECK(!in.good(), "el stream queda marcado como fallido");
    }

    // 2. Longitud de texto enorme (el nombre del primer valor).
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const uint32_t n = 1;
        out.write(reinterpret_cast<const char*>(&n), sizeof(n));
        const uint32_t largo = 0xFFFFFFFFu;
        out.write(reinterpret_cast<const char*>(&largo), sizeof(largo));
        out.close();

        std::ifstream in(ruta, std::ios::binary);
        const std::vector<ValorCampo> leidos = cargarValoresCampos(in);
        CHECK(leidos.empty(), "un texto de longitud enorme se rechaza");
    }

    // 3. Etiqueta fuera de rango: el payload no se podria interpretar.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const uint32_t n = 1;
        out.write(reinterpret_cast<const char*>(&n), sizeof(n));
        const uint32_t largo = 1;
        out.write(reinterpret_cast<const char*>(&largo), sizeof(largo));
        out.write("x", 1);
        const uint8_t tag = 250;
        out.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
        out.close();

        std::ifstream in(ruta, std::ios::binary);
        const std::vector<ValorCampo> leidos = cargarValoresCampos(in);
        CHECK(leidos.empty(), "una etiqueta fuera de rango se rechaza");
    }

    // 4. Anidamiento profundo: el tope de profundidad corta la lectura en vez
    //    de desbordar la pila.
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        const uint32_t n = 1;
        out.write(reinterpret_cast<const char*>(&n), sizeof(n));
        const uint32_t largo = 1;
        out.write(reinterpret_cast<const char*>(&largo), sizeof(largo));
        out.write("x", 1);
        const uint8_t tagGrupo = static_cast<uint8_t>(TagTipo::Grupo);
        out.write(reinterpret_cast<const char*>(&tagGrupo), sizeof(tagGrupo));
        // El valor es un Grupo con un unico subcampo, que a su vez es un Grupo:
        // se repite el patron para superar el tope de profundidad.
        const uint32_t subcampos = 1;
        for (int nivel = 0; nivel < 200; ++nivel) {
            out.write(reinterpret_cast<const char*>(&subcampos),
                      sizeof(subcampos));
            out.write(reinterpret_cast<const char*>(&largo), sizeof(largo));
            out.write("x", 1);
            out.write(reinterpret_cast<const char*>(&tagGrupo), sizeof(tagGrupo));
        }
        out.close();

        std::ifstream in(ruta, std::ios::binary);
        const std::vector<ValorCampo> leidos = cargarValoresCampos(in);
        CHECK(leidos.empty(),
              "un anidamiento profundo se corta por el tope de profundidad");
    }
}

void testEntradaDeScripts() {
    InputScripts input;
    input.reset();
    input.onKey(87, 1);
    CHECK(input.sostiene("W"), "W queda sostenida tras el evento de pulsacion");
    CHECK(input.presionada("W"), "W publica el flanco de pulsacion");
    input.onKey(87, 2);
    CHECK(input.sostiene("W"), "GLFW_REPEAT conserva la tecla sostenida");
    input.avanzarFrame();
    CHECK(input.sostiene("W") && !input.presionada("W"),
          "avanzar el frame consume el flanco y conserva el estado sostenido");
    input.onKey(87, 0);
    CHECK(!input.sostiene("W") && input.soltada("W"),
          "el evento de liberacion publica el flanco correspondiente");
    input.onMouseMove(100.0, 100.0);
    CHECK(input.deltaMouseX() == 0.0f && input.deltaMouseY() == 0.0f,
          "la primera posicion del mouse solo establece el origen");
    input.onMouseMove(106.0, 97.0);
    CHECK(input.deltaMouseX() == 6.0f && input.deltaMouseY() == -3.0f,
          "el delta del mouse acumula movimiento en pixeles");
    input.avanzarFrame();
    CHECK(input.deltaMouseX() == 0.0f && input.deltaMouseY() == 0.0f,
          "avanzar el frame consume el delta del mouse");
    input.reset();
    CHECK(!input.sostiene("W") && !input.soltada("W") &&
              input.deltaMouseX() == 0.0f && input.deltaMouseY() == 0.0f,
          "reset elimina teclas, flancos y delta del mouse");
    for (int digito = 0; digito <= 9; ++digito) {
        const std::string nombre = "D" + std::to_string(digito);
        CHECK(InputScripts::codigoDe(nombre) == 48 + digito,
              "los digitos usan los identificadores D0-D9");
    }
    CHECK(InputScripts::codigoDe("0") == -1,
          "los identificadores simples 0-9 no son aliases");
}

void testAudioIniciadoPorScripts() {
    ScriptAudioHandles handlesScript;
    std::vector<int> activos = {12, 27, 33};
    handlesScript.registrar(27);
    handlesScript.registrar(33);
    handlesScript.registrar(-1);
    activos.erase(std::remove(activos.begin(), activos.end(), 33),
                  activos.end());
    handlesScript.retirar(33);
    handlesScript.detenerTodos([&activos](int handle) {
        activos.erase(std::remove(activos.begin(), activos.end(), handle),
                      activos.end());
    });
    CHECK(activos == std::vector<int>({12}),
          "al cerrar la simulacion se detienen solo los handles de scripts");
}

// Ruta de las cabeceras del script: absoluta en el checkout de desarrollo o
// relativa junto al ejecutable en la app instalada ("include"). La
// resolucion no toca el disco: `existe` simula que rutas son utilizables.
void seccion01() {
    {
        const std::string raiz = std::filesystem::temp_directory_path().string();
        const std::string fakeExe =
            (std::filesystem::temp_directory_path() / "funshi_fake_exe").string();
        const std::string absoluta = raiz;
        const std::string absolutaMala =
            (std::filesystem::temp_directory_path() / "funshi_no_existe").string();
        const std::string relativaBien = "include";
        const std::string relativaSoloJuntoAlExe = "solo_junto_al_exe";
        const std::string candidatoExe =
            (std::filesystem::path(fakeExe) / relativaSoloJuntoAlExe).string();
        const std::vector<std::string> existentes = {
            absoluta, relativaBien, candidatoExe};

        auto existe = [&](const std::string& ruta) {
            return std::find(existentes.begin(), existentes.end(), ruta) !=
                   existentes.end();
        };

        CHECK(RutaCabecerasScript::resolver("", fakeExe, existe).empty(),
              "sin valor configurado no hay carpeta de cabeceras");
        CHECK(RutaCabecerasScript::resolver(absoluta, fakeExe, existe) ==
                  absoluta,
              "una ruta absoluta existente se usa tal cual");
        CHECK(RutaCabecerasScript::resolver(absolutaMala, fakeExe, existe)
                  .empty(),
              "una ruta absoluta inexistente no se devuelve muerta");
        CHECK(RutaCabecerasScript::resolver(relativaBien, fakeExe, existe) ==
                  relativaBien,
              "una ruta relativa al directorio de trabajo se usa tal cual");
        CHECK(RutaCabecerasScript::resolver(relativaSoloJuntoAlExe, fakeExe,
                                            existe) == candidatoExe,
              "una ruta relativa se resuelve contra la carpeta del ejecutable");
CHECK(RutaCabecerasScript::resolver(relativaSoloJuntoAlExe, "",
                                            existe)
                   .empty(),
              "sin carpeta del ejecutable no se inventa una ruta");
    }
}
