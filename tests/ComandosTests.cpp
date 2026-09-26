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
// Pruebas headless del sistema undo/redo basado en el patron Command.
// Cubren: CrearObjetoComando, BorrarObjetoComando, ReparentarComando,
// TransformComando, AgregarComponenteComando, QuitarComponenteComando,
// LimpiarEscenaComando y la cadena de redo multiple del GestorComandos.
//
// Cada test crea una SceneRegistry + EditorController frescos, ejecuta un
// comando via GestorComandos, verifica el estado, deshace y verifica, luego
// rehace y verifica. No se requiere pila grafica, fisica ni AudioManager.

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

#include "Scenes/SceneRegistry.h"
#include "Scenes/EditorController.h"
#include "Comandos/CrearObjetoComando.h"
#include "Comandos/BorrarObjetoComando.h"
#include "Comandos/ReparentarComando.h"
#include "Comandos/TransformComando.h"
#include "Comandos/AgregarComponenteComando.h"
#include "Comandos/QuitarComponenteComando.h"
#include "Comandos/LimpiarEscenaComando.h"
#include "Comandos/GestorComandos.h"

#include "Objetos/Modelos3D.h"
#include "Objetos/SimpleObject.h"
#include "Objetos/GameObject.h"
#include "Objetos/Componentes/Transform.h"
#include "Objetos/Componentes/Color.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Cuenta los objetos no-raiz en la vista plana de la escena.
int contarObjetos(SceneRegistry* scene) {
    scene->refreshGameObjectView();
    int count = 0;
    if (auto* objs = scene->getGameObjects()) {
        if (objs->isEmpty()) return 0;
        auto* pos = objs->first();
        while (pos) {
            ++count;
            pos = (pos != objs->last()) ? objs->next(pos) : nullptr;
        }
    }
    return count;
}

// Crea un SimpleObject en la escena y devuelve su puntero.
GameObject* crearSimpleObject(SceneRegistry* scene, const char* name = "Obj") {
    auto obj = std::make_unique<SimpleObject>();
    std::snprintf(obj->inputName, sizeof(obj->inputName), "%s", name);
    return scene->createObject(std::move(obj));
}

// --- Caso 1: CrearObjetoComando ---
void probarCrearObjeto() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    auto modelo = std::make_unique<Modelos3D>();
    cmds->ejecutar(std::make_unique<CrearObjetoComando>(
        &editor, &scene, std::move(modelo)));

    GameObject* obj = scene.getObjectByID(1);
    CHECK(obj != nullptr, "Crear: objeto creado existe (id=1)");
    CHECK(contarObjetos(&scene) == 1, "Crear: escena tiene 1 objeto");
    CHECK(editor.puedeDeshacer(), "Crear: puede deshacer tras ejecutar");
    CHECK(!editor.puedeRehacer(), "Crear: no puede rehacer antes de deshacer");

    // deshacer/rehacer devuelven la descripcion del comando aplicado para que la
    // UI pueda avisar en la barra de estado; vacias si la pila no tenia nada.
    const std::string descripcionUndo = editor.deshacer();
    CHECK(!descripcionUndo.empty(),
          "Crear: deshacer devuelve la descripcion del comando");
    CHECK(scene.getObjectByID(1) == nullptr, "Crear: objeto desaparece tras undo");
    CHECK(contarObjetos(&scene) == 0, "Crear: escena vacia tras undo");
    CHECK(editor.puedeRehacer(), "Crear: puede rehacer tras undo");

    const std::string descripcionRedo = editor.rehacer();
    CHECK(!descripcionRedo.empty(),
          "Crear: rehacer devuelve la descripcion del comando");
    CHECK(descripcionRedo == descripcionUndo,
          "Crear: undo y redo describen el mismo comando");
    CHECK(scene.getObjectByID(1) != nullptr, "Crear: objeto restaurado tras redo");
    CHECK(contarObjetos(&scene) == 1, "Crear: escena tiene 1 objeto tras redo");
    CHECK(editor.puedeDeshacer(), "Crear: puede deshacer tras redo");

    // Pila de redo agotada: la llamada es inocua y no inventa descripcion.
    cmds->limpiar();
    CHECK(editor.deshacer().empty(), "pila vacia: deshacer devuelve cadena vacia");
    CHECK(editor.rehacer().empty(), "pila vacia: rehacer devuelve cadena vacia");
}

// --- Caso 2: BorrarObjetoComando ---
void probarBorrarObjeto() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearSimpleObject(&scene, "Borrar");
    int id = obj->getId();
    CHECK(contarObjetos(&scene) == 1, "Borrar: 1 objeto antes de borrar");

    cmds->ejecutar(std::make_unique<BorrarObjetoComando>(&editor, obj, &scene));

    CHECK(scene.getObjectByID(id) == nullptr, "Borrar: objeto eliminado");
    CHECK(contarObjetos(&scene) == 0, "Borrar: escena vacia tras borrar");
    CHECK(editor.puedeDeshacer(), "Borrar: puede deshacer");

    editor.deshacer();
    GameObject* restored = scene.getObjectByID(id);
    CHECK(restored != nullptr, "Borrar: objeto restaurado tras undo");
    CHECK(contarObjetos(&scene) == 1, "Borrar: 1 objeto tras undo");
    CHECK(restored->getParentEntity() == scene.getRoot(),
          "Borrar: restaurado bajo root tras undo");

    editor.rehacer();
    CHECK(scene.getObjectByID(id) == nullptr, "Borrar: objeto borrado tras redo");
    CHECK(contarObjetos(&scene) == 0, "Borrar: escena vacia tras redo");
}

// --- Caso 3: ReparentarComando ---
void probarReparentar() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* parentA = crearSimpleObject(&scene, "ParentA");
    GameObject* childObj = crearSimpleObject(&scene, "Child");

    CHECK(childObj->getParentEntity() == scene.getRoot(),
          "Reparentar: child inicialmente bajo root");
    CHECK(parentA->getChildEntities().size() == 0,
          "Reparentar: parentA sin hijos inicialmente");

    cmds->ejecutar(std::make_unique<ReparentarComando>(
        &editor, childObj, parentA, &scene));

    CHECK(childObj->getParentEntity() == parentA,
          "Reparentar: child bajo parentA tras execute");
    CHECK(parentA->getChildEntities().size() == 1,
          "Reparentar: parentA tiene 1 hijo tras execute");

    editor.deshacer();
    CHECK(childObj->getParentEntity() == scene.getRoot(),
          "Reparentar: child vuelve a root tras undo");
    CHECK(parentA->getChildEntities().size() == 0,
          "Reparentar: parentA sin hijos tras undo");

    editor.rehacer();
    CHECK(childObj->getParentEntity() == parentA,
          "Reparentar: child bajo parentA tras redo");
    CHECK(parentA->getChildEntities().size() == 1,
          "Reparentar: parentA tiene 1 hijo tras redo");
}

// --- Caso 4: TransformComando ---
void probarTransform() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearSimpleObject(&scene, "Transform");
    Transform* transform = obj->getComponent<Transform>();
    CHECK(transform != nullptr, "Transform: objeto tiene componente Transform");

    float* pos = transform->getTranslatef();
    CHECK(pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f,
          "Transform: posicion inicial (0,0,0)");
    float* scale = transform->getScalef();
    CHECK(scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f,
          "Transform: escala inicial (1,1,1)");

    auto cmd = std::make_unique<TransformComando>(&editor, obj, &scene);
    cmd->setNuevoEstado(5.0f, 10.0f, 15.0f, 45.0f, 1.0f, 2.0f, 3.0f,
                        2.0f, 3.0f, 4.0f);
    cmds->ejecutar(std::move(cmd));

    pos = transform->getTranslatef();
    CHECK(pos[0] == 5.0f && pos[1] == 10.0f && pos[2] == 15.0f,
          "Transform: posicion (5,10,15) tras execute");
    scale = transform->getScalef();
    CHECK(scale[0] == 2.0f && scale[1] == 3.0f && scale[2] == 4.0f,
          "Transform: escala (2,3,4) tras execute");

    editor.deshacer();
    pos = transform->getTranslatef();
    CHECK(pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f,
          "Transform: posicion restaurada (0,0,0) tras undo");
    scale = transform->getScalef();
    CHECK(scale[0] == 1.0f && scale[1] == 1.0f && scale[2] == 1.0f,
          "Transform: escala restaurada (1,1,1) tras undo");

    editor.rehacer();
    pos = transform->getTranslatef();
    CHECK(pos[0] == 5.0f && pos[1] == 10.0f && pos[2] == 15.0f,
          "Transform: posicion (5,10,15) tras redo");
    scale = transform->getScalef();
    CHECK(scale[0] == 2.0f && scale[1] == 3.0f && scale[2] == 4.0f,
          "Transform: escala (2,3,4) tras redo");
}

// --- Caso 5: AgregarComponenteComando ---
void probarAgregarComponente() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearSimpleObject(&scene, "CompObj");
    int id = obj->getId();
    CHECK(!obj->hasComponent("Color"),
          "AgregarComp: objeto sin Color inicialmente");

    cmds->ejecutar(std::make_unique<AgregarComponenteComando>(
        &editor, obj, std::make_unique<Color>(), &scene));

    GameObject* fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "AgregarComp: objeto existe tras execute");
    CHECK(fetched->hasComponent("Color"),
          "AgregarComp: objeto tiene Color tras execute");

    editor.deshacer();
    fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "AgregarComp: objeto existe tras undo");
    CHECK(!fetched->hasComponent("Color"),
          "AgregarComp: objeto sin Color tras undo");

    editor.rehacer();
    fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "AgregarComp: objeto existe tras redo");
    CHECK(fetched->hasComponent("Color"),
          "AgregarComp: objeto tiene Color tras redo");
}

// --- Caso 6: QuitarComponenteComando ---
void probarQuitarComponente() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearSimpleObject(&scene, "QuitarObj");
    int id = obj->getId();
    obj->addComponent(std::make_unique<Color>());
    CHECK(obj->hasComponent("Color"),
          "QuitarComp: objeto tiene Color inicialmente");

    cmds->ejecutar(std::make_unique<QuitarComponenteComando>(
        &editor, obj, "Color", &scene));

    GameObject* fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "QuitarComp: objeto existe tras execute");
    CHECK(!fetched->hasComponent("Color"),
          "QuitarComp: objeto sin Color tras execute");

    editor.deshacer();
    fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "QuitarComp: objeto existe tras undo");
    CHECK(fetched->hasComponent("Color"),
          "QuitarComp: objeto tiene Color tras undo");

    editor.rehacer();
    fetched = scene.getObjectByID(id);
    CHECK(fetched != nullptr, "QuitarComp: objeto existe tras redo");
    CHECK(!fetched->hasComponent("Color"),
          "QuitarComp: objeto sin Color tras redo");
}

// --- Caso 7: LimpiarEscenaComando ---
void probarLimpiarEscena() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj1 = crearSimpleObject(&scene, "L1");
    GameObject* obj2 = crearSimpleObject(&scene, "L2");
    GameObject* obj3 = crearSimpleObject(&scene, "L3");
    int id1 = obj1->getId();
    int id2 = obj2->getId();
    int id3 = obj3->getId();
    CHECK(contarObjetos(&scene) == 3, "Limpiar: 3 objetos creados");

    cmds->ejecutar(std::make_unique<LimpiarEscenaComando>(&editor, &scene));

    CHECK(scene.getObjectByID(id1) == nullptr, "Limpiar: obj1 borrado");
    CHECK(scene.getObjectByID(id2) == nullptr, "Limpiar: obj2 borrado");
    CHECK(scene.getObjectByID(id3) == nullptr, "Limpiar: obj3 borrado");
    CHECK(contarObjetos(&scene) == 0, "Limpiar: escena vacia tras execute");

    editor.deshacer();
    CHECK(scene.getObjectByID(id1) != nullptr, "Limpiar: obj1 restaurado tras undo");
    CHECK(scene.getObjectByID(id2) != nullptr, "Limpiar: obj2 restaurado tras undo");
    CHECK(scene.getObjectByID(id3) != nullptr, "Limpiar: obj3 restaurado tras undo");
    CHECK(contarObjetos(&scene) == 3, "Limpiar: 3 objetos tras undo");

    editor.rehacer();
    CHECK(scene.getObjectByID(id1) == nullptr, "Limpiar: obj1 borrado tras redo");
    CHECK(scene.getObjectByID(id2) == nullptr, "Limpiar: obj2 borrado tras redo");
    CHECK(scene.getObjectByID(id3) == nullptr, "Limpiar: obj3 borrado tras redo");
    CHECK(contarObjetos(&scene) == 0, "Limpiar: escena vacia tras redo");
}

// --- Caso 8: Cadena de redo multiple ---
void probarRedoCadena() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    // Ejecutar 3 comandos de crear objeto
    for (int i = 0; i < 3; ++i) {
        auto modelo = std::make_unique<Modelos3D>();
        cmds->ejecutar(std::make_unique<CrearObjetoComando>(
            &editor, &scene, std::move(modelo)));
    }
    CHECK(contarObjetos(&scene) == 3, "RedoCadena: 3 objetos creados");

    // Undo todos (uno a la vez, LIFO)
    editor.deshacer();
    CHECK(contarObjetos(&scene) == 2, "RedoCadena: 2 objetos tras undo 1");
    editor.deshacer();
    CHECK(contarObjetos(&scene) == 1, "RedoCadena: 1 objeto tras undo 2");
    editor.deshacer();
    CHECK(contarObjetos(&scene) == 0, "RedoCadena: 0 objetos tras undo 3");
    CHECK(!editor.puedeDeshacer(), "RedoCadena: no puede deshacer mas");

    // Redo todos (uno a la vez, LIFO)
    editor.rehacer();
    CHECK(contarObjetos(&scene) == 1, "RedoCadena: 1 objeto tras redo 1");
    editor.rehacer();
    CHECK(contarObjetos(&scene) == 2, "RedoCadena: 2 objetos tras redo 2");
    editor.rehacer();
    CHECK(contarObjetos(&scene) == 3, "RedoCadena: 3 objetos tras redo 3");
    CHECK(!editor.puedeRehacer(), "RedoCadena: no puede rehacer mas");
}

} // namespace

int main() {
    std::cout << "== COMANDOS TESTS ==" << std::endl;
    probarCrearObjeto();
    probarBorrarObjeto();
    probarReparentar();
    probarTransform();
    probarAgregarComponente();
    probarQuitarComponente();
    probarLimpiarEscena();
    probarRedoCadena();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "COMANDOS TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
