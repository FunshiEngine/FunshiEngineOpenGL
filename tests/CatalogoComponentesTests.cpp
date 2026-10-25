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
// Pruebas headless del catalogo de componentes: es la tabla unica que declara,
// por componente, su dimension (2D/3D), sus requisitos, su creador y su panel.
// De ella dependen tanto ComponentFactory (carga de escenas/prefabs) como el
// menu "agregar componente" del inspector, asi que se verifica que:
//   - todo descriptor este completo (nombre, etiqueta y creador),
//   - no haya nombres repetidos,
//   - ComponentFactory cree desde el catalogo, tolerando el prefijo de MSVC y
//     el alias retrocompatible,
//   - los tipos desconocidos y los requisitos incumplidos devuelvan nullptr,
//   - el filtro por dimension y los submenus sean los esperados.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo, sin
// dependencias de GUI, y salida final "OK/FALLOS: N comprobaciones".

#include <iostream>
#include <memory>
#include <string>

#include "../FunshiEngineGL/src/Objetos/Componentes/Animador2D.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/CatalogoComponentes.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/CameraComponent.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/ComponentFactory.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/RigidBody/RigidBody.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Sombras/Sombras.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/SpriteRenderer.h"
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

// Todo descriptor tiene los datos minimos para poder usarse desde la fabrica y
// desde el menu; los nombres son la clave de busqueda, asi que no se repiten.
void catalogoCompleto() {
    const auto& descriptores = CatalogoComponentes::instancia().descriptores();
    CHECK(!descriptores.empty(), "el catalogo tiene componentes registrados");

    bool todosConNombre = true;
    bool todosConEtiqueta = true;
    bool todosConCreador = true;
    for (const DescriptorComponente& d : descriptores) {
        if (d.nombre.empty()) todosConNombre = false;
        if (d.etiqueta.empty()) todosConEtiqueta = false;
        if (!d.crear) todosConCreador = false;
    }
    CHECK(todosConNombre, "todo descriptor tiene nombre");
    CHECK(todosConEtiqueta, "todo descriptor tiene etiqueta");
    CHECK(todosConCreador, "todo descriptor tiene creador");

    bool sinRepetidos = true;
    for (std::size_t i = 0; i < descriptores.size() && sinRepetidos; ++i) {
        for (std::size_t j = i + 1; j < descriptores.size(); ++j) {
            if (descriptores[i].nombre == descriptores[j].nombre) {
                sinRepetidos = false;
                break;
            }
        }
    }
    CHECK(sinRepetidos, "los nombres del catalogo son unicos");
}

// ComponentFactory::create() debe resolver todo tipo registrado, tolerar el
// prefijo que anteponia MSVC y el alias "Camera" -> "CameraComponent", y
// devolver nullptr ante un tipo ausente o un requisito incumplido.
void factoryCreaDesdeCatalogo() {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);

    auto transform = ComponentFactory::create("Transform", *obj);
    CHECK(dynamic_cast<Transform*>(transform.get()) != nullptr,
          "factory crea Transform desde el catalogo");

    auto camara = ComponentFactory::create("Camera", *obj);
    CHECK(dynamic_cast<CameraComponent*>(camara.get()) != nullptr,
          "factory resuelve el alias Camera -> CameraComponent");

    auto conPrefijo = ComponentFactory::create("class Transform", *obj);
    CHECK(dynamic_cast<Transform*>(conPrefijo.get()) != nullptr,
          "factory tolera el prefijo 'class ' de MSVC");

    auto desconocido = ComponentFactory::create("NoExiste", *obj);
    CHECK(desconocido == nullptr, "un tipo desconocido no crea nada");

    // Entity siembra un Transform en el constructor, asi que el requisito del
    // colisionador se cumple ya en un objeto recien creado.
    auto collider = ComponentFactory::create("EsfereCollider", *obj);
    CHECK(dynamic_cast<EsfereCollider*>(collider.get()) != nullptr,
          "un colisionador con Transform se crea");

    auto rigidBodySinCollider = ComponentFactory::create("RigidBody", *obj);
    CHECK(rigidBodySinCollider == nullptr,
          "un RigidBody sin Collider no se crea");

    obj->addComponent(std::move(collider));
    auto rigidBody = ComponentFactory::create("RigidBody", *obj);
    CHECK(dynamic_cast<RigidBody*>(rigidBody.get()) != nullptr,
          "un RigidBody con Collider se crea");
}

// El filtro por dimension distingue lo propio del plano de lo propio de la
// profundidad y deja en "ambas" lo transversal.
void filtroDimensiones() {
    const auto& cat = CatalogoComponentes::instancia();

    const DescriptorComponente* transform = cat.buscar("Transform");
    CHECK(transform != nullptr, "Transform esta en el catalogo");
    CHECK(dimensionesIncluyen(transform->dimensiones, DimensionProyecto::DosD),
          "Transform aplica a 2D");
    CHECK(dimensionesIncluyen(transform->dimensiones, DimensionProyecto::TresD),
          "Transform aplica a 3D");

    const DescriptorComponente* modelo = cat.buscar("Model");
    CHECK(modelo != nullptr, "Model esta en el catalogo");
    CHECK(!dimensionesIncluyen(modelo->dimensiones, DimensionProyecto::DosD),
          "Model no se ofrece en 2D");
    CHECK(dimensionesIncluyen(modelo->dimensiones, DimensionProyecto::TresD),
          "Model si se ofrece en 3D");

    const DescriptorComponente* sprite = cat.buscar("SpriteRenderer");
    CHECK(sprite != nullptr, "SpriteRenderer esta en el catalogo");
    CHECK(dimensionesIncluyen(sprite->dimensiones, DimensionProyecto::DosD),
          "SpriteRenderer se ofrece en 2D");

    // Los colisionadores de volumen (esfera/cubo) y el RigidBody se reutilizan
    // en 2D restringiendo el cuerpo al plano; la malla sigue siendo solo 3D.
    const DescriptorComponente* collider = cat.buscar("EsfereCollider");
    CHECK(collider != nullptr, "EsfereCollider esta en el catalogo");
    CHECK(dimensionesIncluyen(collider->dimensiones, DimensionProyecto::DosD),
          "el colisionador de esfera se ofrece en 2D");
    CHECK(collider->submenu == "Add Collider",
          "los colisionadores se agrupan en su submenu");

    const DescriptorComponente* cubo = cat.buscar("CubeCollider");
    CHECK(cubo != nullptr &&
              dimensionesIncluyen(cubo->dimensiones, DimensionProyecto::DosD),
          "el colisionador de cubo se ofrece en 2D");

    const DescriptorComponente* malla = cat.buscar("MallaCollider");
    CHECK(malla != nullptr &&
              !dimensionesIncluyen(malla->dimensiones, DimensionProyecto::DosD),
          "el colisionador de malla sigue siendo solo 3D");

    const DescriptorComponente* body = cat.buscar("RigidBody");
    CHECK(body != nullptr &&
              dimensionesIncluyen(body->dimensiones, DimensionProyecto::DosD),
          "el RigidBody se ofrece en 2D");
}

// El modo 2D reutiliza el freeze por ejes del RigidBody: bloquea la posicion Z
// y las rotaciones X/Y sin pisar los freeze que el usuario ya haya fijado.
void restricciones2D() {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    obj->addComponent(ComponentFactory::create("EsfereCollider", *obj));
    obj->addComponent(ComponentFactory::create("RigidBody", *obj));
    RigidBody* rb = obj->getComponent<RigidBody>();
    CHECK(rb != nullptr, "el objeto tiene RigidBody");

    // Freeze propio del usuario en un eje fuera del plano: debe sobrevivir.
    rb->fijarFreezePosicion(true, false, false);
    rb->fijarRestricciones2D();

    CHECK(rb->posicionCongelada(2), "2D bloquea la posicion Z");
    CHECK(rb->rotacionCongelada(0) && rb->rotacionCongelada(1),
          "2D bloquea las rotaciones X e Y");
    CHECK(rb->posicionCongelada(0),
          "2D conserva el freeze de posicion X que fijo el usuario");
    CHECK(!rb->rotacionCongelada(2), "2D deja libre la rotacion Z");

    // Un cuerpo nace sin ser sensor; el flag se puede alternar.
    CHECK(!rb->esSensor(), "un RigidBody no es sensor por defecto");
    rb->fijarSensor(true);
    CHECK(rb->esSensor(), "el RigidBody queda como sensor");
    rb->fijarSensor(false);
    CHECK(!rb->esSensor(), "el RigidBody deja de ser sensor");
}

// El requisito del catalogo coincide con el del creador: el Transform que
// siembra Entity lo cumple; el Collider que pide RigidBody hay que agregarlo.
void requisitos() {
    auto obj = GameObjectFactory::createSimpleObject(nullptr);

    const DescriptorComponente* collider =
        CatalogoComponentes::instancia().buscar("EsfereCollider");
    CHECK(collider != nullptr && collider->prerequisito != nullptr,
          "el colisionador declara su requisito");
    CHECK(collider->prerequisito(*obj),
          "el Transform sembrado por Entity cumple el requisito del colisionador");

    const DescriptorComponente* rigidBody =
        CatalogoComponentes::instancia().buscar("RigidBody");
    CHECK(rigidBody != nullptr && rigidBody->prerequisito != nullptr,
          "el RigidBody declara su requisito");
    CHECK(!rigidBody->prerequisito(*obj),
          "sin Collider el requisito del RigidBody no se cumple");

    obj->addComponent(ComponentFactory::create("EsfereCollider", *obj));
    CHECK(rigidBody->prerequisito(*obj),
          "con Collider el requisito del RigidBody se cumple");
}

// El animador es propio del plano, exige un SpriteRenderer hermano y su
// onUpdate recorre la grilla escribiendo la region UV del sprite.
void animador2D() {
    const auto& cat = CatalogoComponentes::instancia();
    const DescriptorComponente* anim = cat.buscar("Animador2D");
    CHECK(anim != nullptr, "Animador2D esta en el catalogo");
    CHECK(dimensionesIncluyen(anim->dimensiones, DimensionProyecto::DosD),
          "Animador2D aplica a 2D");
    CHECK(!dimensionesIncluyen(anim->dimensiones, DimensionProyecto::TresD),
          "Animador2D no se ofrece en 3D");

    auto sinSprite = GameObjectFactory::createSimpleObject(nullptr);
    CHECK(anim->prerequisito != nullptr && !anim->prerequisito(*sinSprite),
          "sin SpriteRenderer el requisito del Animador2D no se cumple");
    CHECK(ComponentFactory::create("Animador2D", *sinSprite) == nullptr,
          "sin SpriteRenderer el Animador2D no se crea");

    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    obj->addComponent(ComponentFactory::create("SpriteRenderer", *obj));
    CHECK(anim->prerequisito(*obj),
          "con SpriteRenderer el requisito del Animador2D se cumple");
    obj->addComponent(ComponentFactory::create("Animador2D", *obj));

    Animador2D* a = obj->getComponent<Animador2D>();
    SpriteRenderer* s = obj->getComponent<SpriteRenderer>();
    CHECK(a != nullptr && s != nullptr, "el objeto tiene Animador2D y sprite");

    a->setColumnas(2);
    a->setFilas(2);
    a->setFps(1.0f);
    a->setPrimerFrame(0);
    a->setUltimoFrame(-1);
    CHECK(a->getTotalFrames() == 4, "la grilla 2x2 tiene 4 fotogramas");

    // Primer fotograma: esquina superior izquierda de la planilla.
    a->onUpdate(*obj, 0.0f);
    CHECK(s->getUvRect()[0] == 0.0f && s->getUvRect()[1] == 0.0f &&
              s->getUvRect()[2] == 0.5f && s->getUvRect()[3] == 0.5f,
          "el primer fotograma recorta la esquina 0,0");

    // Un paso de 1/fps avanza una celda (fila 0, columna 1).
    a->onUpdate(*obj, 1.0f);
    CHECK(s->getUvRect()[0] == 0.5f && s->getUvRect()[1] == 0.0f,
          "el segundo fotograma recorta la columna 1");

    // Bucle: tras el ultimo vuelve al primero.
    a->onUpdate(*obj, 1.0f);
    a->onUpdate(*obj, 1.0f);
    CHECK(s->getUvRect()[0] == 0.5f && s->getUvRect()[1] == 0.5f,
          "el cuarto fotograma recorta la esquina 1,1");
    a->onUpdate(*obj, 1.0f);
    CHECK(s->getUvRect()[0] == 0.0f && s->getUvRect()[1] == 0.0f,
          "con bucle, tras el ultimo vuelve al primero");

    // Sin bucle, se detiene en el ultimo fotograma.
    a->setBucle(false);
    a->reiniciar();
    a->onUpdate(*obj, 1.0f);
    a->onUpdate(*obj, 1.0f);
    a->onUpdate(*obj, 1.0f);
    a->onUpdate(*obj, 1.0f);
    CHECK(s->getUvRect()[0] == 0.5f && s->getUvRect()[1] == 0.5f,
          "sin bucle la animacion se detiene en el ultimo fotograma");
}

// Las sombras se agregan al objeto de la luz: sin luz no se crean, y con luz
// se crean. El componente es propio (no una propiedad de Light) porque el
// formato binario de los componentes no tiene versionado.
void sombras() {
    const auto& cat = CatalogoComponentes::instancia();
    const DescriptorComponente* sombras = cat.buscar("Sombras");
    CHECK(sombras != nullptr, "Sombras esta en el catalogo");
    CHECK(dimensionesIncluyen(sombras->dimensiones, DimensionProyecto::TresD),
          "Sombras se ofrece en 3D");
    CHECK(!dimensionesIncluyen(sombras->dimensiones, DimensionProyecto::DosD),
          "Sombras no se ofrece en 2D");

    auto sinLuz = GameObjectFactory::createSimpleObject(nullptr);
    CHECK(sombras->prerequisito != nullptr && !sombras->prerequisito(*sinLuz),
          "sin luz el requisito de Sombras no se cumple");
    CHECK(ComponentFactory::create("Sombras", *sinLuz) == nullptr,
          "sin luz el componente de sombras no se crea");

    auto conLuz = GameObjectFactory::createSimpleObject(nullptr);
    conLuz->addComponent(ComponentFactory::create("Light", *conLuz));
    CHECK(sombras->prerequisito(*conLuz),
          "con luz el requisito de Sombras se cumple");
    auto creado = ComponentFactory::create("Sombras", *conLuz);
    auto* componente = dynamic_cast<Sombras*>(creado.get());
    CHECK(componente != nullptr, "con luz el componente de sombras se crea");
    CHECK(componente && componente->getResolucion() > 0,
          "el componente de sombras nace con mapa");
}

int main() {
    catalogoCompleto();
    factoryCreaDesdeCatalogo();
    filtroDimensiones();
    requisitos();
    restricciones2D();
    animador2D();
    sombras();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
