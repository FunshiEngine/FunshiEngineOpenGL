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
#include "ComandosTestsDetalle.h"

// --- Caso 10: duplicar con undo/redo ---
void probarDuplicar() {
    SceneRegistry scene;
    PhysicsEngine fisica;
    EditorController editor(&scene, &fisica);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearConCuerpo(&scene, &editor, "Original");
    const int idOriginal = obj->getId();

    cmds->ejecutar(std::make_unique<DuplicarObjetoComando>(&editor, obj,
                                                           nullptr, &scene));
    CHECK(contarObjetos(&scene) == 2, "Duplicar: 2 objetos tras duplicar");
    GameObject* clon = nullptr;
    {
        scene.refreshGameObjectView();
        for (auto* nodo = scene.getGameObjects()->first(); nodo;
             nodo = (nodo != scene.getGameObjects()->last())
                        ? scene.getGameObjects()->next(nodo)
                        : nullptr) {
            GameObject* candidato = nodo->getElement();
            if (candidato && candidato != obj) clon = candidato;
        }
    }
    CHECK(clon != nullptr, "Duplicar: el clon esta en la escena");
    if (!clon) return;
    CHECK(clon->getId() != idOriginal, "Duplicar: el clon tiene id propio");
    CHECK(std::string(clon->inputName) != "Original",
          "Duplicar: el nombre del clon se uniciza");
    RigidBody* cuerpoClon = clon->getComponent<RigidBody>();
    CHECK(cuerpoClon && cuerpoClon->getRigidBody()->isInWorld(),
          "Duplicar: el cuerpo clonado entra al mundo");
    const int idClon = clon->getId();

    editor.deshacer();
    CHECK(scene.getObjectByID(idClon) == nullptr,
          "Duplicar: el clon sale tras undo");
    CHECK(scene.getObjectByID(idOriginal) != nullptr,
          "Duplicar: el original sigue tras undo");
    CHECK(contarObjetos(&scene) == 1, "Duplicar: 1 objeto tras undo");

    editor.rehacer();
    CHECK(contarObjetos(&scene) == 2, "Duplicar: 2 objetos tras redo");
}

// --- Caso 11: agregar/quitar componente con fisica ---
void probarComponentesConFisica() {
    SceneRegistry scene;
    PhysicsEngine fisica;
    EditorController editor(&scene, &fisica);
    GestorComandos* cmds = editor.getGestorComandos();

    GameObject* obj = crearSimpleObject(&scene, "CompFisico");
    const int id = obj->getId();
    Transform* transform = obj->getComponent<Transform>();
    cmds->ejecutar(std::make_unique<AgregarComponenteComando>(
        &editor, obj,
        std::make_unique<EsfereCollider>(1.0f, transform, obj), &scene));
    CHECK(obj->getComponent<Collider>() != nullptr,
          "CompFisica: collider agregado via comando");

    cmds->ejecutar(std::make_unique<AgregarComponenteComando>(
        &editor, obj, std::make_unique<RigidBody>(
                          obj->getComponent<Collider>(), 2.0f),
        &scene));
    RigidBody* body = obj->getComponent<RigidBody>();
    CHECK(body != nullptr, "CompFisica: cuerpo agregado via comando");
    if (!body) return;
    CHECK(body->getRigidBody()->isInWorld(),
          "CompFisica: agregar registra el cuerpo en el mundo");

    editor.deshacer();
    CHECK(obj->getComponent<RigidBody>() == nullptr,
          "CompFisica: undo quita el cuerpo del objeto");
    CHECK(!body->getRigidBody()->isInWorld(),
          "CompFisica: undo saca el cuerpo del mundo");

    editor.rehacer();
    RigidBody* cuerpoRehecho = obj->getComponent<RigidBody>();
    CHECK(cuerpoRehecho && cuerpoRehecho->getRigidBody()->isInWorld(),
          "CompFisica: redo registra el cuerpo de nuevo");

    cmds->ejecutar(std::make_unique<QuitarComponenteComando>(
        &editor, obj, "RigidBody", &scene));
    CHECK(obj->getComponent<RigidBody>() == nullptr,
          "CompFisica: quitar saca el cuerpo del objeto");
    CHECK(!body->getRigidBody()->isInWorld(),
          "CompFisica: quitar saca el cuerpo del mundo");

    editor.deshacer();
    RigidBody* cuerpoDevuelto = obj->getComponent<RigidBody>();
    CHECK(cuerpoDevuelto && cuerpoDevuelto->getRigidBody()->isInWorld(),
          "CompFisica: undo de quitar devuelve el cuerpo al mundo");
    (void)id;
}

// --- Caso 11: AjustarVariableComando ---
// Ajusta una variable de un componente y la restituye al deshacer. El
// comando no conoce el catalogo de propiedades: los accesores se inyectan.
void probarAjustarVariable() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    auto objeto = std::make_unique<SimpleObject>();
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s", "Caja");
    cmds->ejecutar(std::make_unique<CrearObjetoComando>(
        &editor, &scene, std::move(objeto)));

    GameObject* obj = scene.getObjectByID(1);
    CHECK(obj != nullptr, "AjustarVariable: el objeto existe");
    if (!obj) return;

    Transform* transform = obj->getComponent<Transform>();
    CHECK(transform != nullptr, "AjustarVariable: el objeto tiene transform");
    if (!transform) return;

    // Lector y escritor: el mismo contrato que usa el catalogo de
    // propiedades de la API de inspeccion.
    auto lector = [](void* componente) -> std::string {
        Transform* t = static_cast<Transform*>(componente);
        return std::to_string(t->getTranslatef()[0]);
    };
    auto escritor = [](void* componente, const std::string& valor,
                       std::string& error) -> bool {
        Transform* t = static_cast<Transform*>(componente);
        float x = 0.0f, y = 0.0f, z = 0.0f;
        char* fin = nullptr;
        x = std::strtof(valor.c_str(), &fin);
        if (fin == valor.c_str() || *fin != '\0') {
            error = "valor invalido";
            return false;
        }
        y = x;
        z = x;
        t->setTranslatef(x, y, z);
        return true;
    };

    auto comando = std::make_unique<AjustarVariableComando>(
        &editor, &scene, obj->getId(), "Transform", "position", lector,
        escritor, "5");
    AjustarVariableComando* ajuste = comando.get();
    cmds->ejecutar(std::move(comando));
    CHECK(ajuste->valorAnterior().find("0") != std::string::npos,
          "AjustarVariable: el valor anterior se captura al construir");
    CHECK(transform->getTranslatef()[0] == 5.0f,
          "AjustarVariable: el valor nuevo se escribe");
    CHECK(ajuste->ultimoError().empty(),
          "AjustarVariable: la escritura no dio error");

    editor.deshacer();
    CHECK(transform->getTranslatef()[0] == 0.0f,
          "AjustarVariable: el deshacer restituye el valor anterior");

    editor.rehacer();
    CHECK(transform->getTranslatef()[0] == 5.0f,
          "AjustarVariable: el rehacer vuelve a escribir el valor nuevo");
}

// --- Caso 12: EstablecerTagComando ---
void probarEstablecerTag() {
    SceneRegistry scene;
    EditorController editor(&scene);
    GestorComandos* cmds = editor.getGestorComandos();

    auto objeto = std::make_unique<SimpleObject>();
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s", "Enemigo");
    cmds->ejecutar(std::make_unique<CrearObjetoComando>(
        &editor, &scene, std::move(objeto)));

    GameObject* obj = scene.getObjectByID(1);
    CHECK(obj != nullptr, "EstablecerTag: el objeto existe");
    if (!obj) return;
    CHECK(obj->getTag() == "Untagged", "EstablecerTag: la etiqueta inicial");

    cmds->ejecutar(std::make_unique<EstablecerTagComando>(
        &editor, &scene, obj, "enemigo"));
    CHECK(obj->getTag() == "enemigo", "EstablecerTag: la etiqueta se fija");

    editor.deshacer();
    CHECK(obj->getTag() == "Untagged",
          "EstablecerTag: el deshacer restituye la etiqueta anterior");
}

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
    probarBorrarConFisica();
    probarDuplicar();
    probarComponentesConFisica();
    probarAjustarVariable();
    probarEstablecerTag();
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "COMANDOS TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
