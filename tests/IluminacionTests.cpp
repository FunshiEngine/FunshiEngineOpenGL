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
// Pruebas headless del sistema de luces: la recogida de las luces de la
// escena (LightSystem::collectLights) y el paralelismo entre la lista de luces
// y la lista de duenos.
//
// Ese paralelismo no es cosmetico: el pase de sombras usa el indice de la luz
// para saber a que contribucion de iluminacion pertenece cada mapa de sombra
// (uShadowLightIndex). Si las dos listas se desalinearan, la sombra de una luz
// oscureceria la contribucion de otra, sin que nada fallara de forma visible.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo y salida
// final "OK/FALLOS: N comprobaciones".

#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

#include "../FunshiEngineGL/src/Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../FunshiEngineGL/src/Iluminacion/LightSystem.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Light.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Transform.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"

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

namespace {

// Objeto con una luz del tipo pedido. El objeto queda vivo mientras el
// unique_ptr lo posea.
std::unique_ptr<GameObject> conLuz(LightType tipo, float x = 0.f, float y = 0.f,
                                   float z = 0.f) {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    auto luz = std::make_unique<Light>();
    luz->setType(tipo);
    obj->addComponent(std::move(luz));
    if (Transform* transform = obj->getComponent<Transform>())
        transform->setTranslatef(x, y, z);
    return obj;
}

// Objeto sin luz: sirve para comprobar que no desplaza el indice de las
// luces que vienen despues.
std::unique_ptr<GameObject> sinLuz() {
    return GameObjectFactory::createSimpleObject(nullptr);
}

bool casiIgual(float a, float b) { return std::fabs(a - b) < 1e-4f; }

bool vectorCasiIgual(const float a[3], float x, float y, float z) {
    return casiIgual(a[0], x) && casiIgual(a[1], y) && casiIgual(a[2], z);
}

} // namespace

// Una escena sin luces (o sin lista) no recoge ninguna.
void escenaSinLuces() {
    LightSystem sistema;

    LightData luces[LightSystem::kMaxLights];
    int cantidad = -1;
    sistema.collectLights(nullptr, luces, cantidad);
    CHECK(cantidad == 0, "una lista nula no recoge luces");

    ListaDE<GameObject*> vacia;
    cantidad = -1;
    sistema.collectLights(&vacia, luces, cantidad);
    CHECK(cantidad == 0, "una escena vacia no recoge luces");

    auto objeto = sinLuz();
    ListaDE<GameObject*> escena;
    escena.addLast(objeto.get());
    cantidad = -1;
    sistema.collectLights(&escena, luces, cantidad);
    CHECK(cantidad == 0, "un objeto sin luz no aporta luces");
}

// Las luces se recogen en el orden de la escena y cada una viene con el
// objeto dueno que la lleva. Los objetos sin luz no desplazan el indice: el
// dueno de la luz i es siempre el objeto que tiene el componente Light.
void ordenYDuenos() {
    LightSystem sistema;

    auto sin = sinLuz();
    auto primera = conLuz(LightType::POINT, 1.f, 2.f, 3.f);
    auto otroSin = sinLuz();
    auto segunda = conLuz(LightType::SPOT, 4.f, 5.f, 6.f);

    ListaDE<GameObject*> escena;
    escena.addLast(sin.get());
    escena.addLast(primera.get());
    escena.addLast(otroSin.get());
    escena.addLast(segunda.get());

    LightData luces[LightSystem::kMaxLights];
    GameObject* duenos[LightSystem::kMaxLights] = {};
    int cantidad = 0;
    sistema.collectLights(&escena, luces, cantidad, duenos);

    CHECK(cantidad == 2, "se recogen las dos luces de la escena");
    if (cantidad != 2) return;

    CHECK(duenos[0] == primera.get(),
          "el primer dueno es el objeto de la primera luz");
    CHECK(duenos[1] == segunda.get(),
          "el segundo dueno es el objeto de la segunda luz");
    CHECK(luces[0].type == static_cast<int>(LightType::POINT),
          "la primera luz conserva su tipo");
    CHECK(luces[1].type == static_cast<int>(LightType::SPOT),
          "la segunda luz conserva su tipo");
}

// Los datos de cada luz salen del Transform de su dueno: una luz puntual
// toma la posicion del objeto y una direccional toma la direccion (hacia donde
// mira el objeto), porque su posicion no importa.
void datosDesdeElTransform() {
    LightSystem sistema;

    auto puntual = conLuz(LightType::POINT, 1.f, 2.f, 3.f);
    auto direccional = conLuz(LightType::DIRECTIONAL, 7.f, 8.f, 9.f);
    if (Transform* transform =
            direccional->getComponent<Transform>())
        transform->setRotatef(90.f, 0.f, 1.f, 0.f);

    ListaDE<GameObject*> escena;
    escena.addLast(puntual.get());
    escena.addLast(direccional.get());

    LightData luces[LightSystem::kMaxLights];
    GameObject* duenos[LightSystem::kMaxLights] = {};
    int cantidad = 0;
    sistema.collectLights(&escena, luces, cantidad, duenos);
    CHECK(cantidad == 2, "se recogen las dos luces");
    if (cantidad != 2) return;

    CHECK(vectorCasiIgual(luces[0].worldPos, 1.f, 2.f, 3.f),
          "la luz puntual usa la posicion de su objeto");
    CHECK(vectorCasiIgual(luces[0].direction, 0.f, 0.f, -1.f),
          "la luz puntual sin rotar mira hacia -Z");

    CHECK(vectorCasiIgual(luces[1].worldPos, -1.f, 0.f, 0.f),
          "la luz direccional usa la direccion de su objeto");
    CHECK(vectorCasiIgual(luces[1].direction, -1.f, 0.f, 0.f),
          "la direccion de la luz direccional es la de su objeto");
}

// El limite de luces (los slots GL_LIGHT0..7) se respeta, y los duenos que
// quedan son los de las primeras luces, en el mismo orden.
void limiteDeLuces() {
    LightSystem sistema;

    std::vector<std::unique_ptr<GameObject>> objetos;
    ListaDE<GameObject*> escena;
    for (int i = 0; i < LightSystem::kMaxLights + 4; ++i) {
        auto obj = conLuz(LightType::POINT, static_cast<float>(i), 0.f, 0.f);
        escena.addLast(obj.get());
        objetos.push_back(std::move(obj));
    }

    LightData luces[LightSystem::kMaxLights];
    GameObject* duenos[LightSystem::kMaxLights] = {};
    int cantidad = 0;
    sistema.collectLights(&escena, luces, cantidad, duenos);

    CHECK(cantidad == LightSystem::kMaxLights,
          "no se recogen mas luces que el limite de slots");
    CHECK(duenos[0] == objetos[0].get(), "el primer dueno es la primera luz");
    CHECK(duenos[cantidad - 1] == objetos[cantidad - 1].get(),
          "el ultimo dueno corresponde a la ultima luz recogida");
}

// La lista de duenos es opcional: quien solo quiere los datos de las luces
// puede llamar igual sin ella.
void duenosOpcionales() {
    LightSystem sistema;

    auto luz = conLuz(LightType::DIRECTIONAL);
    ListaDE<GameObject*> escena;
    escena.addLast(luz.get());

    LightData luces[LightSystem::kMaxLights];
    int cantidad = 0;
    sistema.collectLights(&escena, luces, cantidad);
    CHECK(cantidad == 1, "se recoge la luz sin pedir los duenos");
}

int main() {
    escenaSinLuces();
    ordenYDuenos();
    datosDesdeElTransform();
    limiteDeLuces();
    duenosOpcionales();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
