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

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/IScriptBehaviour.h"

using namespace ReflejoScripts;

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

int main() {
    testValoresPorDefecto();
    testLecturaEscritura();
    testGruposVector();
    testObjetoResolucion();
    testSerializacionBinaria();

    std::cout << "ScriptsTests: " << total << " verificaciones, " << fallos
              << " fallos" << std::endl;
    return fallos > 0 ? 1 : 0;
}