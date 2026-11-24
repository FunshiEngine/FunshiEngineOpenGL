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
// Preambulo compartido por las unidades de ScriptsJavaTests.cpp (generado al dividirlo).
#ifndef SCRIPTSJAVATESTSDETALLE_H
#define SCRIPTSJAVATESTSDETALLE_H

// Prueba de punta a punta del backend Java: escribe un fuente .java, lo
// compila con javac, arranca el JVM dinamicamente (libjvm via dlopen), crea el
// objeto, refleja e inyecta campos publicos (SerializeField) y ejecuta el
// ciclo iniciar/actualizar/detener. Si no hay JDK/javac sale con SKIP (77).

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/BackendJava.h"
#include "../FunshiEngineGL/src/Behaviour/Backends/SondeoToolchain.h"
#include "../FunshiEngineGL/src/Behaviour/Reflection/BehaviourReflection.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptRuntime.h"

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

namespace MotorScript {

// Stub de la tabla de acceso: en el motor la implementa ScriptGameObject.cpp.

}  // namespace MotorScript

static const char* FUENTE_JAVA =
    "public class MiPruebaJava implements Comportamiento {\n"
    "    public float velocidad = 2.0f;\n"
    "    public int vidas = 3;\n"
    "    public boolean activo = true;\n"
    "    public String etiqueta = \"hola\";\n"
    "    public String nombreObjeto;\n"
    "    public float x, y, z, giro, ejeX, ejeY, ejeZ, escalaX, escalaY, escalaZ;\n"
    "    public int sonido;\n"
    "    public long meta;\n"
    "    public String nombreMeta;\n"
    "    public boolean sostenida, pulsada, soltada, servicioOnStop;\n"
    "    public boolean movimientoFisico, saltoFisico;\n"
    "    public String tagObjeto;\n"
    "    public boolean etiquetaSuelo, colliderResuelto;\n"
    "    public int contactosInicio, contactosPersistencia, contactosFin;\n"
    "    public long camara;\n"
    "    public boolean porIdOk, porTagOk;\n"
    "    public float mouseX, mouseY;\n"
    "    public float masaLeida, escalaGravLeida, friccionLeida, gravLeidaY;\n"
    "    public boolean masaOk, gravOk, friccionOk, freezeOk;\n"
    "    public boolean crearOk, destruirOk, clonarOk, agregarOk;\n"
    "    public long hud;\n"
    "    public boolean hudActiva, hudEstadoOk, hudValorOk;\n"
    "    public String hudSalida;\n"
    "    public boolean musicaOk;\n"
    "    @Override public void iniciar(long o) {\n"
    "        vidas = 100;\n"
    "        nombreObjeto = Nativo.nombre(o);\n"
    "        x = Nativo.posicionX(o); y = Nativo.posicionY(o); z = Nativo.posicionZ(o);\n"
    "        Nativo.fijarPosicion(o, x + 1, y + 2, z + 3);\n"
    "        Nativo.fijarEscala(o, 4, 5, 6);\n"
    "        Nativo.fijarRotacion(o, 0.5f, 1, 0, 0);\n"
    "        giro = Nativo.rotacionAngulo(o); ejeX = Nativo.rotacionEjeX(o);\n"
    "        ejeY = Nativo.rotacionEjeY(o); ejeZ = Nativo.rotacionEjeZ(o);\n"
    "        escalaX = Nativo.escalaX(o); escalaY = Nativo.escalaY(o); escalaZ = Nativo.escalaZ(o);\n"
    "        sonido = Nativo.reproducirSonido(\"disparo.wav\", 0.5f, false);\n"
    "        meta = Nativo.objetoPorNombre(\"Meta\");\n"
    "        nombreMeta = Nativo.nombre(meta);\n"
    "        sostenida = Nativo.teclaSostiene(\"W\");\n"
    "        pulsada = Nativo.teclaPresionada(\"SPACE\");\n"
    "        soltada = Nativo.teclaSoltada(\"D0\");\n"
    "        mouseX = Nativo.deltaMouseX(); mouseY = Nativo.deltaMouseY();\n"
    "        movimientoFisico = Nativo.fijarVelocidadHorizontal(o, 2, -3);\n"
    "        saltoFisico = Nativo.saltar(o, 5);\n"
    "        tagObjeto = Nativo.etiqueta(meta);\n"
    "        etiquetaSuelo = Nativo.tieneEtiqueta(meta, \"suelo\");\n"
    "        colliderResuelto = Nativo.objetoDeCollider(0xBEEF) == meta;\n"
    "        camara = Nativo.objetoPorNombre(\"Meta\");\n"
    "        porIdOk = Nativo.objetoPorId(7) == meta;\n"
    "        porTagOk = Nativo.objetoPorEtiqueta(\"suelo\") == meta;\n"
    "        masaLeida = Nativo.masa(o); masaOk = Nativo.fijarMasa(o, 3.0f);\n"
    "        gravOk = Nativo.usaGravedad(o) && Nativo.fijarUsoGravedad(o, true);\n"
    "        escalaGravLeida = Nativo.escalaGravedad(o); Nativo.fijarEscalaGravedad(o, 2.0f);\n"
    "        friccionLeida = Nativo.friccion(o); friccionOk = Nativo.fijarFriccion(o, 0.1f);\n"
    "        freezeOk = Nativo.fijarFreezePosicion(o, true, false, true) && Nativo.posicionCongelada(o, 0) && !Nativo.posicionCongelada(o, 1) && Nativo.fijarFreezeRotacion(o, false, true, false) && Nativo.rotacionCongelada(o, 1);\n"
    "        Nativo.fijarGravedadGlobal(0, -2, 0); gravLeidaY = Nativo.gravedadGlobalY();\n"
    "        hud = Nativo.interfazDeObjeto(\"Meta\");\n"
    "        hudActiva = Nativo.interfazActiva(hud);\n"
    "        hudEstadoOk = Nativo.interfazEstablecerEstado(hud, \"{\\\"vida\\\":10}\");\n"
    "        hudValorOk = Nativo.interfazEstablecerValor(hud, \"/x\", \"1\");\n"
    "        Nativo.interfazEstablecerActiva(hud, false);\n"
    "        hudSalida = Nativo.interfazValor(hud, \"/boton\");\n"
    "        long nuevo = Nativo.crearObjeto(\"Enemigo\", 0); crearOk = nuevo != 0;\n"
    "        destruirOk = Nativo.destruirObjeto(nuevo);\n"
    "        clonarOk = Nativo.clonarObjeto(o, 0) != 0;\n"
    "        agregarOk = Nativo.agregarColliderEsfera(o, 1.0f) && Nativo.agregarColliderCubo(o, 1.0f) && Nativo.agregarRigidBody(o, 1.0f);\n"
    "        musicaOk = Nativo.reproducirMusica(\"tema.ogg\", 0.8f, 0.0f);\n"
    "        Nativo.detenerMusica(0.0f);\n"
    "        Nativo.volumenMusica(0.5f);\n"
    "        Nativo.detenerSonido(sonido);\n"
    "    }\n"
    "    @Override public void actualizar(long o, double dt) {\n"
    "        vidas += 1;\n"
    "        Nativo.imprimir(\"acci\\u00F3n\");\n"
    "    }\n"
    "    @Override public void detener(long o) { vidas = -1; servicioOnStop = Nativo.teclaSostiene(\"W\"); }\n"
    "    @Override public void colisionInicio(long o, long c, long otro) { contactosInicio++; }\n"
    "    @Override public void colisionPersistencia(long o, long c, long otro) { contactosPersistencia++; }\n"
    "    @Override public void colisionFin(long o, long c, long otro) { contactosFin++; }\n"
    "}\n";

// Estado de la unidad original: una sola instancia (se define en ScriptsJavaTests.cpp).

namespace ScriptsJavaTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ScriptsJavaTestsDetalle
using namespace ScriptsJavaTestsDetalle;

namespace MotorScript {

namespace ScriptsJavaTestsDetalle {

extern float posicion[3];
extern float escala[3];
extern float rotacion[4];
extern int handleSonidoDetenido;
extern std::string textoImpreso;
extern float masaCuerpo;
extern float escalaGrav;
extern float friccionCuerpo;
extern bool freezeP[3];
extern bool freezeR[3];
extern float gravMundo[3];
extern float fundidoSalida;
extern float volumenBus;

}  // namespace ScriptsJavaTestsDetalle
using namespace ScriptsJavaTestsDetalle;

}  // namespace MotorScript

// Funciones repartidas entre las unidades.

namespace MotorScript {

const ApiScriptGameObject* tablaApi();
const ScriptServices* tablaServicios();

}  // namespace MotorScript

bool hayJavac();

#endif
