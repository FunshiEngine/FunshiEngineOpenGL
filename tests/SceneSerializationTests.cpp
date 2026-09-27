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
// Pruebas headless de serializacion de escena: round-trip completo (guardar ->
// recargar -> conservar nombre, id y jerarquia) y el nombre por defecto de los
// objetos nuevos.
//
// Es la prueba que faltaba: la unica suite de serializacion era
// model-serialization-tests, que cubre el componente Model AISLADO. Sin
// round-trip de escena, un objeto sin nombre (H-6) o cualquier regresion de
// guardado pasaban sin que nada lo notara.
//
// Patron de autoria (ver AGENTS.md): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Assets/AssetManager.h"
#include "../FunshiEngineGL/src/Events/EventBus.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Objetos/SimpleObject.h"
#include "../FunshiEngineGL/src/Scenes/EditorController.h"
#include "../FunshiEngineGL/src/Scenes/SceneRegistry.h"
#include "../FunshiEngineGL/src/Scenes/SceneSerializer.h"

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

// --- Nombres por defecto (H-6) ------------------------------------------------
// Un objeto creado desde la UI tiene que nacer con nombre no vacio y sin
// repetir el de otro objeto del arbol. Si nace vacio, el arbol muestra el
// nombre de la clase y el vacio se guarda y se recarga fielmente.
void nombresPorDefecto() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_escena_nombres");
    (void)carpetaDir;

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();
    CHECK(raiz != nullptr, "la escena tiene raiz");

    // La raiz ya se llama "Scene": el primer objeto no puede llamarse igual.
    auto primero = GameObjectFactory::createSimpleObject(raiz);
    CHECK(!std::string(primero->inputName).empty(),
          "un objeto nuevo nace con nombre no vacio");
    GameObject* p1 = editor.createGameObject(std::move(primero), raiz);
    CHECK(p1 != nullptr, "el objeto entra en la escena");
    const std::string nombre1 = p1 ? std::string(p1->inputName) : "";

    auto segundo = GameObjectFactory::createSimpleObject(raiz);
    const std::string nombre2(segundo->inputName);
    CHECK(!nombre2.empty(), "el segundo objeto tambien nace con nombre");
    CHECK(nombre2 != nombre1, "dos objetos seguidos no repiten nombre");

    // Sin arbol (creacion antes de tener escena) igual se consigue un nombre.
    auto suelto = GameObjectFactory::createSimpleObject(nullptr);
    CHECK(!std::string(suelto->inputName).empty(),
          "sin raiz el objeto igual nace con nombre");

    auto* p2 = editor.createGameObject(std::move(segundo), raiz);
    CHECK(p2 != nullptr, "el segundo objeto entra en la escena");
    if (p1 && p2) {
        // El filtro tiene que mirar el subarbol completo: el hijo no puede
        // heredar el nombre del padre.
        auto colgado = GameObjectFactory::createSimpleObject(p1);
        CHECK(std::string(colgado->inputName) != nombre1,
              "el nombre por defecto no repite el del padre");
    }
}

// --- Guardar con el arbol vacio (H-8) -----------------------------------------
// Decision: una escena vacia es un estado valido y se guarda vacia a PROPOSITO,
// pero con aviso en el log. Lo que no se permite es el retorno silencioso entre
// el trunc y la escritura: un BBDDObjetos.txt de 0 bytes sin explicar es
// indistinguible de una perdida de datos.
//
// Nota de alcance: hoy el registro siembra la raiz en su constructor y en
// clear(), asi que desde la UI el arbol casi no puede quedar vacio; el branch
// queda protegido para cuando aparezca un camino que lo deje asi (y por eso se
// vacia a mano aca, desde el arbol, que es lo que save() mira).
void guardadoConArbolVacio() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_escena_vacia");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();

    // Un archivo previo con contenido real: si el guardado vacio lo reemplaza,
    // tiene que decirlo.
    {
        std::ofstream previo(pathTxt);
        previo << "escena anterior\n";
    }

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    auto* arbol = registry.getEntitysTree();
    CHECK(arbol != nullptr, "el arbol existe");
    while (arbol && !arbol->isEmpty()) arbol->deleteRoot();
    CHECK(arbol && arbol->isEmpty(), "se vacia el arbol para el caso de H-8");

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    serializer.save(prefijo);
    std::cerr.rdbuf(buferAnterior);

    CHECK(fs::exists(pathTxt), "el archivo se escribe aunque el arbol este vacio");
    CHECK(fs::is_empty(pathTxt, ec),
          "una escena vacia deja el archivo vacio (estado valido)");
    CHECK(aviso.str().find("arbol vacio") != std::string::npos,
          "el guardado vacio se avisa en el log: nunca en silencio");
}

// --- Round-trip de escena -----------------------------------------------------
// Guardar y recargar tiene que conservar nombre, id y jerarquia: es el hueco
// por el que H-6 (objetos sin nombre) pudo pasar sin que nada lo notara.
void roundTripDeEscena() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_escena_roundtrip");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    std::string nombreGuardado;
    std::string nombreHijo;
    int idGuardado = -1;
    int idHijo = -1;

    // 1. Construir la escena con la factoria (para que el nombre por defecto
    //    entre en el round-trip) y guardar.
    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);

        GameObject* raiz = registry.getRoot();
        auto objeto = GameObjectFactory::createSimpleObject(raiz);
        std::snprintf(objeto->inputName, sizeof(objeto->inputName), "CosaUnica");
        GameObject* creado = editor.createGameObject(std::move(objeto), raiz);
        CHECK(creado != nullptr, "se crea el objeto de prueba");
        if (creado) {
            nombreGuardado = creado->inputName;
            idGuardado = creado->getId();

            auto hijo = GameObjectFactory::createSimpleObject(creado);
            std::snprintf(hijo->inputName, sizeof(hijo->inputName),
                          "HijoDelPrimero");
            GameObject* nacido =
                editor.createGameObject(std::move(hijo), creado);
            if (nacido) {
                nombreHijo = nacido->inputName;
                idHijo = nacido->getId();
            }
        }

        serializer.save(prefijo);
        CHECK(fs::exists(pathTxt), "el archivo de escena se escribio");
    }

    // 2. Escena nueva: recargar y comparar.
    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);

        serializer.load(pathTxt, semiPath);

        GameObject* raiz = registry.getRoot();
        CHECK(raiz != nullptr, "la raiz existe tras cargar");
        CHECK(std::string(raiz ? raiz->inputName : "") == "Scene",
              "la raiz sigue llamandose Scene");
        CHECK(!registry.getEntitysTree()->isEmpty(),
              "el arbol de la escena recargada no esta vacio");

        bool encontroPrimero = false;
        bool encontroHijo = false;
        if (raiz) {
            for (auto* hijo : raiz->getChildEntities()) {
                auto* go = dynamic_cast<GameObject*>(hijo);
                if (!go) continue;
                if (std::string(go->inputName) != nombreGuardado) continue;
                encontroPrimero = true;
                CHECK(go->getId() == idGuardado,
                      "el id del objeto sobrevive el guardado");
                for (auto* posibleHijo : go->getChildEntities()) {
                    auto* h2 = dynamic_cast<GameObject*>(posibleHijo);
                    if (h2 && std::string(h2->inputName) == nombreHijo) {
                        encontroHijo = true;
                        CHECK(h2->getId() == idHijo,
                              "el id del hijo sobrevive el guardado");
                    }
                }
            }
        }
        CHECK(encontroPrimero, "el nombre del objeto sobrevive el guardado");
        CHECK(encontroHijo, "la jerarquia (hijo dentro del padre) se conserva");
    }
}

int main() {
    nombresPorDefecto();
    roundTripDeEscena();
    guardadoConArbolVacio();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
