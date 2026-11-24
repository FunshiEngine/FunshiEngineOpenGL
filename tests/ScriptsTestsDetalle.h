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
// Preambulo compartido por las unidades de ScriptsTests.cpp (generado al dividirlo).
#ifndef SCRIPTSTESTSDETALLE_H
#define SCRIPTSTESTSDETALLE_H

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

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

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

// Tipo destino de una referencia a componente (TagTipo::Componente). La
// reflexion lo trata como puntero opaco: solo guarda el nombre del objeto dueno.
struct RecursoHUD {};

class ComportamientoConHUD : public IScriptBehaviour {
public:
    RecursoHUD* panel = nullptr;

    void onStart(GameObject*) override {}
    void onUpdate(GameObject*, float) override {}
    void onStop(GameObject*) override {}

    REFLECT_INICIO(ComportamientoConHUD)
    REFLECT_COMPONENTE(panel, "InterfaceComponent")
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

// Estado de la unidad original: una sola instancia (se define en ScriptsTests.cpp).

namespace ScriptsTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ScriptsTestsDetalle
using namespace ScriptsTestsDetalle;

// Funciones repartidas entre las unidades.

bool casiIgual(float a, float b);
bool casiIgual(const vec3& a, const vec3& b);
void testValoresPorDefecto();
void testLecturaEscritura();
void testGruposVector();
void testObjetoResolucion();
void testComponenteResolucion();
void testSerializacionBinaria();
void testAlinearValores();
void testAlinearValoresConTipoDistinto();
void testLecturaCorrupta();
void testEntradaDeScripts();
void testAudioIniciadoPorScripts();
void seccion01();
void seccion02();
void seccion03();
void seccion04();
void seccion05();
void seccion06();
void seccion07();
void testContratoCompilacion();
void testSondeoToolchain(const std::string& exePropio);

#endif
