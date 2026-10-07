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
// Pruebas headless del motor de reflexion de comportamientos (SerializeField):
// declaracion de campos con macros REFLECT_*, lectura/escritura sobre la
// instancia (incl. grupos anidados, vectores de grupos, vectores de primitivas)
// y round-trip de la serializacion binaria (con reordenamiento/campos nuevos).
// Sin pila grafica: se ejercita el arbol ValorCampo/DefCampo directamente.
// Incluye ademas los CONTRATOS headless de BackendCpp: los flags con que se
// compila el script C++ (CRT, familia de compilador, citado de rutas) y el
// comando de sondeo de toolchain (dispositivo nulo de la plataforma).

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/ComandoCompilacionCpp.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/RutaCabecerasScript.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/ResolucionJdk.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/SondeoToolchain.h"
#include "../FunshiEngineGL/src/Behaviour/IScriptBehaviour.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptAudioHandles.h"
#include "../FunshiEngineGL/src/Input/InputScripts.h"

using namespace ReflejoScripts;
namespace fs = std::filesystem;

int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

static bool casiIgual(float a, float b) {
    return std::fabs(a - b) < 1e-5f;
}

static bool casiIgual(const vec3& a, const vec3& b) {
    return casiIgual(a.x, b.x) && casiIgual(a.y, b.y) && casiIgual(a.z, b.z);
}

// --- Tipos de prueba reproducibles por los scripteadores ---------------------

struct Misil {
    float velocidad = 4.0f;
    int dano = 10;
    std::string etiqueta = "cohete";
    std::vector<int> cargas{1, 2};
    REFLECT_INICIO(Misil)
    REFLECT_CAMPO(velocidad)
    REFLECT_CAMPO(dano)
    REFLECT_CAMPO(etiqueta)
    REFLECT_ARRAY(cargas)
    REFLECT_FIN
};

struct Oleada {
    int conteo = 1;
    float espaciado = 0.5f;
    REFLECT_INICIO(Oleada)
    REFLECT_CAMPO(conteo)
    REFLECT_CAMPO(espaciado)
    REFLECT_FIN
};

class PruebaComportamiento : public IScriptBehaviour {
public:
    int vidas = 3;
    float velocidad = 1.5f;
    double peso = 2.5;
    bool activo = true;
    std::string nombre = "Hola";
    vec3 direccion = vec3(1.0f, 2.0f, 3.0f);
    std::vector<int> puntos{1, 2, 3};
    std::vector<float> ratios{0.1f, 0.2f};
    std::vector<double> pesosD{0.5, 1.5};
    std::vector<bool> flags{true, false};
    std::vector<std::string> tags{"rojo", "verde"};
    std::vector<vec3> esquinas{vec3(0, 0, 0), vec3(1, 1, 1)};
    Misil misil;
    std::vector<Misil> munis;
    std::vector<Oleada> oleadas;
    GameObject* objetivo = nullptr;
    std::vector<GameObject*> enemigos;

    void onStart(GameObject*) override {}
    void onUpdate(GameObject*, float) override {}
    void onStop(GameObject*) override {}

    REFLECT_INICIO(PruebaComportamiento)
    REFLECT_CAMPO(vidas)
    REFLECT_CAMPO(velocidad)
    REFLECT_CAMPO(peso)
    REFLECT_CAMPO(activo)
    REFLECT_CAMPO(nombre)
    REFLECT_CAMPO(direccion)
    REFLECT_ARRAY(puntos)
    REFLECT_ARRAY(ratios)
    REFLECT_ARRAY(pesosD)
    REFLECT_ARRAY(flags)
    REFLECT_ARRAY(tags)
    REFLECT_ARRAY(esquinas)
    REFLECT_GRUPO(misil)
    REFLECT_GRUPOS(munis)
    REFLECT_GRUPOS(oleadas)
    REFLECT_CAMPO(objetivo)
    REFLECT_CAMPO(enemigos)
    REFLECT_FIN
};

// Orden esperado de los campos reflejados de PruebaComportamiento.
static const std::vector<DefCampo>& defsDePrueba() {
    static const std::vector<DefCampo> defs = PruebaComportamiento::reflexion();
    return defs;
}
// 0 vidas, 1 velocidad, 2 peso, 3 activo, 4 nombre, 5 direccion, 6 puntos,
// 7 ratios, 8 pesosD, 9 flags, 10 tags, 11 esquinas, 12 misil, 13 munis,
// 14 oleadas, 15 objetivo, 16 enemigos.
static constexpr int kCantidadCampos = 17;
static constexpr int kIndiceMisil = 12;
static constexpr int kIndiceMunis = 13;

static void testValoresPorDefecto() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    CHECK(defs.size() == kCantidadCampos, "17 campos reflejados");
    const std::vector<ValorCampo> valores = valoresPorDefecto(defs);
    CHECK(valores.size() == kCantidadCampos, "hay un valor por campo");

    CHECK(valores[0].tag == TagTipo::Entero &&
              valores[0].como<int>() == 0,
          "default entero");
    CHECK(valores[1].tag == TagTipo::Flotante &&
              casiIgual(valores[1].como<float>(), 0.0f),
          "default flotante");
    CHECK(valores[3].tag == TagTipo::Booleano &&
              valores[3].como<bool>() == false,
          "default booleano");
    CHECK(valores[6].tag == TagTipo::Enteros &&
              valores[6].como<std::vector<int>>().empty(),
          "default enteros");
    CHECK(valores[12].tag == TagTipo::Grupo &&
              valores[12].como<std::vector<ValorCampo>>().size() == 4,
          "grupo default con 4 subcampos");
    CHECK(valores[13].tag == TagTipo::Grupos &&
              valores[13].como<std::vector<std::vector<ValorCampo>>>().empty(),
          "grupos default vacio");
    CHECK(valores[15].tag == TagTipo::Objeto &&
              valores[15].como<std::string>().empty(),
          "objeto default sin referencia");
    CHECK(valores[16].tag == TagTipo::Objetos,
          "vector de objetos default");
}

static void testLecturaEscritura() {
    PruebaComportamiento b;
    const std::vector<DefCampo>& defs = defsDePrueba();

    // Lectura de primitivas.
    CHECK(leerCampo(defs[0], &b).como<int>() == 3, "leer vidas");
    CHECK(casiIgual(leerCampo(defs[1], &b).como<float>(), 1.5f), "leer velocidad");
    CHECK(leerCampo(defs[4], &b).como<std::string>() == "Hola", "leer nombre");
    CHECK(casiIgual(leerCampo(defs[5], &b).como<vec3>(), vec3(1, 2, 3)),
          "leer direccion");
    CHECK(leerCampo(defs[9], &b).como<std::vector<bool>>().size() == 2,
          "leer flags");

    // Escritura de primitivas.
    ValorCampo v = valorPorDefecto(defs[0]);
    v.contenido = 99;
    escribirCampo(defs[0], &b, v);
    CHECK(b.vidas == 99, "escribir vidas");

    // Escritura de un vector completo.
    ValorCampo vLista = leerCampo(defs[7], &b);
    vLista.contenido = std::vector<float>{9.0f, 8.0f};
    escribirCampo(defs[7], &b, vLista);
    CHECK(b.ratios.size() == 2 && casiIgual(b.ratios[0], 9.0f) &&
              casiIgual(b.ratios[1], 8.0f),
          "escribir vector de flotantes");

    // Grupo: leer subcampos y tocar uno por su nombre.
    ValorCampo vGrupo = leerCampo(defs[kIndiceMisil], &b);
    std::vector<ValorCampo>& subs = vGrupo.como<std::vector<ValorCampo>>();
    CHECK(subs.size() == 4, "grupo misil con 4 subcampos");
    bool encontroDano = false;
    for (ValorCampo& sub : subs) {
        if (sub.nombre == "dano") {
            sub.contenido = 77;
            encontroDano = true;
        }
    }
    CHECK(encontroDano, "el grupo expone 'dano'");
    escribirCampo(defs[kIndiceMisil], &b, vGrupo);
    CHECK(b.misil.dano == 77, "escribir subcampo del grupo");
    CHECK(casiIgual(b.misil.velocidad, 4.0f), "resto del grupo intacto");
}

static void testGruposVector() {
    PruebaComportamiento b;
    const std::vector<DefCampo>& defs = defsDePrueba();
    b.munis.push_back(Misil{});
    b.munis.push_back(Misil{});
    b.munis[0].dano = 5;
    b.munis[0].etiqueta = "pesado";

    ValorCampo vG = leerCampo(defs[kIndiceMunis], &b);
    std::vector<std::vector<ValorCampo>>& grupos =
        vG.como<std::vector<std::vector<ValorCampo>>>();
    CHECK(grupos.size() == 2, "leer 2 munis");
    CHECK(grupos[0].size() == 4, "cada mun se lee completo");
    CHECK(grupos[0][1].nombre == "dano" &&
              grupos[0][1].como<int>() == 5,
          "valores iniciales por nombre");

    // Agregar un tercer elemento y escribir: el vector del objeto se redimensiona.
    grupos.push_back(valoresPorDefecto(defs[kIndiceMunis].subcampos));
    escribirCampo(defs[kIndiceMunis], &b, vG);
    CHECK(b.munis.size() == 3, "grupos vector redimensionado a 3");
    CHECK(b.munis[0].dano == 5 && b.munis[0].etiqueta == "pesado",
          "elementos previos preservados");
}

static void testObjetoResolucion() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    PruebaComportamiento b;

    // Sin resolver activo, escribir un nombre (objetivo inexistente) => nullptr.
    fijarResolverObjetos(nullptr);
    ValorCampo v = valorPorDefecto(defs[15]);
    v.contenido = std::string("Enemigo");
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == nullptr, "sin resolver el objetivo queda nulo");

    // Con resolver que no lo encuentra => nullptr.
    fijarResolverObjetos([](const std::string&) { return nullptr; });
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == nullptr, "resolver sin el objeto devuelve nulo");

    GameObject* camara =
        reinterpret_cast<GameObject*>(static_cast<std::uintptr_t>(0x1234));
    fijarResolverObjetos([camara](const std::string& nombre) {
        return nombre == "Camara" ? camara : nullptr;
    });
    v.contenido = std::string("Camara");
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == camara,
          "referencia a GameObject se resuelve por nombre");
    fijarResolverObjetos(nullptr);

    // Vector de objetos: nombres -> punteros nulos.
    std::vector<std::string> nombres{"A", "B"};
    ValorCampo vL = valorPorDefecto(defs[16]);
    vL.contenido = nombres;
    escribirCampo(defs[16], &b, vL);
    CHECK(b.enemigos.size() == 2 && b.enemigos[0] == nullptr &&
              b.enemigos[1] == nullptr,
          "vector de objetos resuelto a nulos");
}

static void testSerializacionBinaria() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    PruebaComportamiento b;
    b.vidas = 42;
    b.oleadas.push_back(Oleada{});
    b.oleadas[0].conteo = 8;
    b.oleadas[0].espaciado = 2.25f;
    std::vector<std::string> enemigosNombres{"E1", "E2"};

    std::vector<ValorCampo> valores;
    for (const DefCampo& d : defs) valores.push_back(leerCampo(d, &b));
    // El vector de objetos se guarda por nombre: reemplazamos el valor leido
    // (b.enemigos esta vacio en la prueba) para ejercitar la serializacion.
    valores[16].contenido = enemigosNombres;

    // Carpeta temporal unica por proceso (RAII): ademas el .bin ahora se
    // limpia al salir (antes quedaba en el sistema tras cada corrida).
    TempPruebas::CarpetaPrueba carpetaPrueba("funshi_reflexion_binaria");
    const std::filesystem::path ruta =
        carpetaPrueba.ruta() / "reflexion_binaria.bin";
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        CHECK(out.good(), "abrir archivo temporal de escritura");
        guardarValoresCampos(out, valores);
        out.close();
    }

    // Carga con los mismos defs: round-trip exacto.
    {
        std::ifstream in(ruta, std::ios::binary);
        std::vector<ValorCampo> cargados = cargarValoresCampos(in, defs);
        CHECK(cargados.size() == kCantidadCampos, "round-trip mantiene cantidad");
        CHECK(cargados[0].como<int>() == 42, "round-trip entero");
        CHECK(cargados[4].como<std::string>() == "Hola", "round-trip texto");
        CHECK(cargados[6].como<std::vector<int>>() ==
                  std::vector<int>({1, 2, 3}),
              "round-trip enteros");
        CHECK(cargados[14].como<std::vector<std::vector<ValorCampo>>>().size() ==
                  1,
              "round-trip grupos vector");
        CHECK(cargados[14].como<std::vector<std::vector<ValorCampo>>>()[0][0]
                          .como<int>() == 8,
              "round-trip subcampo de grupos");
        CHECK(cargados[15].como<std::string>().empty(), "round-trip objeto vacio");
        CHECK(cargados[16].como<std::vector<std::string>>() == enemigosNombres,
              "round-trip vector de objetos");
    }

    // Carga con defs REORDENADOS y con un campo nuevo: casa por nombre.
    {
        std::vector<DefCampo> defsMutados = {
            defs[4],  // nombre
            defs[9],  // flags
            defs[0],  // vidas
            defs[1],  // velocidad
            // campo nuevo inexistente en el archivo original:
            DefCampo{}};
        defsMutados[4].nombre = "campoNuevo";
        defsMutados[4].tag = TagTipo::Texto;

        std::ifstream in(ruta, std::ios::binary);
        std::vector<ValorCampo> cargados = cargarValoresCampos(in, defsMutados);
        CHECK(cargados.size() == 5, "carga con defs mutados mantiene cantidad");
        CHECK(cargados[0].nombre == "nombre" &&
                  cargados[0].como<std::string>() == "Hola",
              "casa por nombre aunque cambie el orden");
        CHECK(cargados[2].nombre == "vidas" && cargados[2].como<int>() == 42,
              "casa por nombre (vidas)");
        CHECK(cargados[4].nombre == "campoNuevo" &&
                  cargados[4].como<std::string>().empty(),
              "campo nuevo queda con default");
    }

    std::filesystem::remove(ruta);
}

// Escenario exacto del crash que se corrigio: la escena se carga SIN defs
// (el script todavia no esta compilado) y el emparejado con los campos reales
// ocurre despues, al compilar. Si ese emparejado no ocurre, el arbol guardado
// conserva el cardinal que tenia en el archivo y el inspector lo indexa por
// posicion contra los defs: lectura fuera de rango.
static void testAlinearValores() {
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

static void testEntradaDeScripts() {
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

static void testAudioIniciadoPorScripts() {
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

// --- Contrato de compilacion de los scripts C++ (CRT compartido) -------------
// El .dll del script comparte heap con el engine a traves de la reflexion
// (std::string, std::vector<DefCampo> y los std::function de cada campo se
// alocan de un lado y se liberan del otro), asi que los dos modulos tienen que
// usar el MISMO runtime de C++. Este test no puede observar la corrupcion de
// heap (haria falta MSVC y cargar el .dll en runtime), pero si verifica el
// contrato de flags: sin /MD el heap vuelve a separarse y el crash regresa.
static void testContratoCompilacion() {
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

    // Ruta de las cabeceras del script: absoluta en el checkout de desarrollo o
    // relativa junto al ejecutable en la app instalada ("include"). La
    // resolucion no toca el disco: `existe` simula que rutas son utilizables.
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

    // Emparejamiento de libjvm y javac: tienen que salir de la MISMA raiz. Con
    // dos raices falsas (una con JVM y otra con compilador) el emparejamiento
    // tiene que quedarse con la primera que tenga JVM y no mezclar, porque un
    // javac de otra version genera bytecode que la JVM rechaza y el fallo sale
    // como "no se encontro la clase".
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

    // El valor horneado por CMake es la ruta del ARCHIVO de la biblioteca, no
    // una raiz de JDK. Pasado por la lista de raices se descartaba siempre (buscaba
    // "<archivo>/lib/server/libjvm.so"), de modo que ni en la maquina donde se
    // construyo el motor aportaba nada. Se comprueba que, como ultimo recurso,
    // solo cuenta si el archivo existe aqui, y que su raiz deduce el javac.
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

    // Que se pueda ejecutar el javac que se paso a Proceso::ejecutar: un nombre
    // suelto lo resuelve el PATH, una ruta tiene que existir. El fallo de "no se
    // pudo ejecutar" no decia que lo que faltaba era un JDK.
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

    // Paridad instalador/motor al detectar un JDK: el .iss solo miraba
    // bin\server\jvm.dll, asi que daba por bueno un JRE (que no trae javac) y el
    // motor luego no podia compilar ningun .java. El predicado del instalador es
    // Pascal Script y no se puede ejecutar fuera de Windows, asi que el contrato
    // se comprueba sobre el propio .iss: cada deteccion de JDK pasa por el
    // predicado que exige LAS DOS piezas, y no quedan comprobaciones sueltas de
    // jvm.dll que puedan reintroducir el falso positivo.
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

    // Descubrimiento del toolset en la maquina del usuario: la ruta del
    // compilador que hornea el build es la del runner que publico el paquete, y
    // en el equipo del usuario no existe. El barrido se prueba con un arbol
    // falso porque 'existe' no necesita Windows: se comprueba el contenido, no
    // la plataforma.
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

    // Que compilador se invoca. El caso que motiva esto es el paquete publicado:
    // la ruta horneada es la del runner que compilo y no existe en el equipo del
    // usuario, asi que invocarla era garantir un fallo. "No existe" se simula
    // con una ruta debajo de un archivo regular (falla en cualquier SO).
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
static void testSondeoToolchain(const std::string& exePropio) {
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
    testSerializacionBinaria();
    testAlinearValores();
    testEntradaDeScripts();
    testAudioIniciadoPorScripts();
    testContratoCompilacion();
    testSondeoToolchain(argv[0]);

    std::cout << "ScriptsTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}