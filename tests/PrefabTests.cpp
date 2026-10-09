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
// Pruebas headless del sistema de Prefabs: round-trip completo (guardar -> instanciar ->
// conservar nombre, tag, jerarquia, componentes, Transform) y validacion de archivos.
//
// Patron de autoria (ver AGENTS.md): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Objetos/Prefab.h"
#include "../FunshiEngineGL/src/Objetos/PrefabLibrary.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Color.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Model.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Transform.h"
#include "../FunshiEngineGL/src/Scenes/EditorController.h"
#include "../FunshiEngineGL/src/Scenes/SceneRegistry.h"
#include "../FunshiEngineGL/src/Events/EventBus.h"
#include "../FunshiEngineGL/src/Assets/AssetManager.h"

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

// --- Helpers -----------------------------------------------------------------

static GameObject* crearEscenaBasica(SceneRegistry& registry, EditorController*& editor,
                                     EventBus* events, AssetManager* assets) {
    editor = new EditorController(&registry, nullptr, events, assets);
    return registry.getRoot();
}

static void verificarObjetoBasico(GameObject* obj, const std::string& nombreEsperado,
                                   const std::string& tagEsperado,
                                   const float posEsperada[3]) {
    CHECK(obj != nullptr, "objeto no es nulo");
    if (!obj) return;
    CHECK(std::string(obj->inputName) == nombreEsperado,
          "nombre coincide: " + nombreEsperado);
    CHECK(obj->getTag() == tagEsperado,
          "tag coincide: " + tagEsperado);
    Transform* t = obj->getGlobalTransform();
    CHECK(t != nullptr, "tiene componente Transform (global)");
    if (t) {
        const float* pos = t->getTranslatef();
        CHECK(std::abs(pos[0] - posEsperada[0]) < 0.001f &&
              std::abs(pos[1] - posEsperada[1]) < 0.001f &&
              std::abs(pos[2] - posEsperada[2]) < 0.001f,
              "Transform posicion global correcta");
    }
}

static void verificarHijo(GameObject* padre, const std::string& nombreHijo,
                           const std::string& tagHijo,
                           const float posHijo[3]) {
    CHECK(padre != nullptr, "padre no es nulo");
    if (!padre) return;
    GameObject* hijo = nullptr;
    for (auto* e : padre->getChildEntities()) {
        auto* go = dynamic_cast<GameObject*>(e);
        if (go && std::string(go->inputName) == nombreHijo) {
            hijo = go;
            break;
        }
    }
    CHECK(hijo != nullptr, "hijo encontrado: " + nombreHijo);
    if (hijo) {
        verificarObjetoBasico(hijo, nombreHijo, tagHijo, posHijo);
    }
}
// --- Tests -------------------------------------------------------------------

// 1. Round-trip basico: guardar un prefab simple e instanciarlo
void roundTripPrefabBasico() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_basico");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EditorController* editor = nullptr;
    EventBus events;
    AssetManager assets;
    GameObject* raiz = crearEscenaBasica(registry, editor, &events, &assets);

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "MiPrefab");
    objeto->setTag("enemigo");
    Transform* t = objeto->getComponent<Transform>();
    t->setTranslatef(1.0f, 2.0f, 3.0f);

    auto color = std::make_unique<Color>();
    color->setColor(1.0f, 0.5f, 0.0f);
    objeto->addComponent(std::move(color));

    GameObject* creado = editor->createGameObject(std::move(objeto), raiz);
    CHECK(creado != nullptr, "objeto creado en la escena");

    Prefab prefab("MiPrefab");
    CHECK(prefab.guardarDesdeObjeto(creado, editor), "prefab se guarda correctamente");
    CHECK(fs::exists(prefab.getRutaArchivo()), "archivo .prefab existe en disco");

    delete editor;

    SceneRegistry registry2;
    EditorController* editor2 = nullptr;
    EventBus events2;
    AssetManager assets2;
    GameObject* raiz2 = crearEscenaBasica(registry2, editor2, &events2, &assets2);

    GameObject* instanciado = prefab.instanciar(editor2, raiz2);
    CHECK(instanciado != nullptr, "prefab se instancia correctamente");

    float posEsperada[3] = {1.0f, 2.0f, 3.0f};
    verificarObjetoBasico(instanciado, "MiPrefab", "enemigo", posEsperada);

    Color* colorInst = instanciado->getComponent<Color>();
    CHECK(colorInst != nullptr, "componente Color se instancia");
    if (colorInst) {
        const float* c = colorInst->getColor();
        CHECK(std::abs(c[0] - 1.0f) < 0.001f &&
              std::abs(c[1] - 0.5f) < 0.001f &&
              std::abs(c[2] - 0.0f) < 0.001f &&
              std::abs(c[3] - 1.0f) < 0.001f,
              "Color mantiene valores originales");
    }

    delete editor2;
    EditorConfig::limpiarRaizAssets();
}

// 2. Round-trip con jerarquia (padre + hijo)
void roundTripPrefabJerarquia() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_jerarquia");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EditorController* editor = nullptr;
    EventBus events;
    AssetManager assets;
    GameObject* raiz = crearEscenaBasica(registry, editor, &events, &assets);

    auto padreObj = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(padreObj->inputName, sizeof(padreObj->inputName), "PadrePrefab");
    padreObj->setTag("grupo");
    Transform* tp = padreObj->getComponent<Transform>();
    tp->setTranslatef(10.0f, 0.0f, 5.0f);

    GameObject* padre = editor->createGameObject(std::move(padreObj), raiz);
    CHECK(padre != nullptr, "padre creado");

    auto hijoObj = GameObjectFactory::createSimpleObject(padre);
    std::snprintf(hijoObj->inputName, sizeof(hijoObj->inputName), "HijoPrefab");
    hijoObj->setTag("miembro");
    Transform* th = hijoObj->getComponent<Transform>();
    th->setTranslatef(2.0f, 3.0f, 4.0f);

    GameObject* hijo = editor->createGameObject(std::move(hijoObj), padre);
    CHECK(hijo != nullptr, "hijo creado");

    Prefab prefab("PrefabJerarquia");
    CHECK(prefab.guardarDesdeObjeto(padre, editor), "prefab con jerarquia se guarda");

    delete editor;

    SceneRegistry registry2;
    EditorController* editor2 = nullptr;
    EventBus events2;
    AssetManager assets2;
    GameObject* raiz2 = crearEscenaBasica(registry2, editor2, &events2, &assets2);

    GameObject* instanciado = prefab.instanciar(editor2, raiz2);
    CHECK(instanciado != nullptr, "prefab con jerarquia se instancia");

    float posPadre[3] = {10.0f, 0.0f, 5.0f};
    float posHijo[3] = {12.0f, 3.0f, 9.0f};

    verificarObjetoBasico(instanciado, "PadrePrefab", "grupo", posPadre);
    verificarHijo(instanciado, "HijoPrefab", "miembro", posHijo);

    delete editor2;
    EditorConfig::limpiarRaizAssets();
}

// 3. Round-trip con componente Model (path)
void roundTripPrefabConModel() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_model");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EditorController* editor = nullptr;
    EventBus events;
    AssetManager assets;
    GameObject* raiz = crearEscenaBasica(registry, editor, &events, &assets);

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "PrefabConModelo");
    Transform* t = objeto->getComponent<Transform>();
    t->setTranslatef(5.0f, 5.0f, 5.0f);

    auto modelo = std::make_unique<Model>();
    modelo->setPath("Modelos/coche.fbx");
    objeto->addComponent(std::move(modelo));

    GameObject* creado = editor->createGameObject(std::move(objeto), raiz);
    CHECK(creado != nullptr, "objeto con Model creado");

    Prefab prefab("PrefabConModelo");
    CHECK(prefab.guardarDesdeObjeto(creado, editor), "prefab con Model se guarda");

    delete editor;

    SceneRegistry registry2;
    EditorController* editor2 = nullptr;
    EventBus events2;
    AssetManager assets2;
    GameObject* raiz2 = crearEscenaBasica(registry2, editor2, &events2, &assets2);

    GameObject* instanciado = prefab.instanciar(editor2, raiz2);
    CHECK(instanciado != nullptr, "prefab con Model se instancia");

    float posEsperada[3] = {5.0f, 5.0f, 5.0f};
    verificarObjetoBasico(instanciado, "PrefabConModelo", "Untagged", posEsperada);

    Model* modeloInst = instanciado->getComponent<Model>();
    CHECK(modeloInst != nullptr, "componente Model se instancia");
    if (modeloInst) {
        std::string path = modeloInst->getPath();
        CHECK(path.find("Modelos/coche.fbx") != std::string::npos,
              "path del Model contiene la ruta relativa esperada: " + path);
    }

    delete editor2;
    EditorConfig::limpiarRaizAssets();
}

// 4. PrefabLibrary: cache y validacion
void prefabLibraryCacheYValidacion() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_library");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "ParaLibrary");
    Transform* t = objeto->getComponent<Transform>();
    t->setTranslatef(7.0f, 8.0f, 9.0f);
    GameObject* creado = editor.createGameObject(std::move(objeto), raiz);
    CHECK(creado != nullptr, "objeto para library creado");

    Prefab prefab("ParaLibrary");
    CHECK(prefab.guardarDesdeObjeto(creado, &editor), "prefab se guarda para library");

    PrefabLibrary library;
    Prefab* p1 = library.obtener("ParaLibrary");
    CHECK(p1 != nullptr, "library obtiene prefab valido");
    CHECK(p1->getNombre() == "ParaLibrary", "nombre correcto en library");

    Prefab* p2 = library.obtener("ParaLibrary");
    CHECK(p2 != nullptr && p2 == p1, "segunda llamada usa cache (mismo puntero)");

    Prefab* p3 = library.obtener("NoExiste");
    CHECK(p3 == nullptr, "library devuelve nullptr para prefab inexistente");

    fs::path corrupto = fs::path(base) / "Assets" / "Prefabs" / "Corrupto.prefab";
    {
        std::ofstream out(corrupto, std::ios::binary);
        out.write("BASURA", 6);
    }

    Prefab* p4 = library.obtener("Corrupto");
    CHECK(p4 == nullptr, "library rechaza archivo con magic invalido");

    fs::path truncado = fs::path(base) / "Assets" / "Prefabs" / "Truncado.prefab";
    {
        std::ofstream out(truncado, std::ios::binary);
        std::uint32_t magic = 0x42415250;
        std::uint32_t version = 1;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
    }

    Prefab* p5 = library.obtener("Truncado");
    CHECK(p5 == nullptr, "library rechaza archivo truncado");

    fs::path vacio = fs::path(base) / "Assets" / "Prefabs" / "Vacio.prefab";
    {
        std::ofstream out(vacio, std::ios::binary);
        std::uint32_t magic = 0x42415250;
        std::uint32_t version = 1;
        std::uint32_t numObjetos = 0;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&numObjetos), sizeof(numObjetos));
    }

    Prefab* p6 = library.obtener("Vacio");
    CHECK(p6 == nullptr, "library rechaza archivo con 0 objetos");

    EditorConfig::limpiarRaizAssets();
}

// 5. Prefab::validarArchivo() estatico
void validarArchivoEstatico() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_validar");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "Valido");
    GameObject* creado = editor.createGameObject(std::move(objeto), raiz);

    Prefab prefab("Valido");
    CHECK(prefab.guardarDesdeObjeto(creado, &editor), "prefab valido guardado");

    CHECK(Prefab::validarArchivo(prefab.getRutaArchivo()),
          "validarArchivo devuelve true para archivo valido");

    CHECK(!Prefab::validarArchivo("/no/existe/archivo.prefab"),
          "validarArchivo devuelve false para archivo inexistente");

    fs::path badMagic = base / "Assets" / "Prefabs" / "BadMagic.prefab";
    {
        std::ofstream out(badMagic, std::ios::binary);
        out.write("XXXX", 4);
    }
    CHECK(!Prefab::validarArchivo(badMagic.string()),
          "validarArchivo rechaza magic incorrecto");

    fs::path badVersion = base / "Assets" / "Prefabs" / "BadVersion.prefab";
    {
        std::ofstream out(badVersion, std::ios::binary);
        std::uint32_t magic = 0x42415250;
        std::uint32_t version = 999;
        std::uint32_t numObjetos = 1;
        out.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
        out.write(reinterpret_cast<const char*>(&version), sizeof(version));
        out.write(reinterpret_cast<const char*>(&numObjetos), sizeof(numObjetos));
    }
    CHECK(!Prefab::validarArchivo(badVersion.string()),
          "validarArchivo rechaza version incorrecta");

    fs::path empty = base / "Assets" / "Prefabs" / "Empty.prefab";
    {
        std::ofstream out(empty, std::ios::binary);
    }
    CHECK(!Prefab::validarArchivo(empty.string()),
          "validarArchivo rechaza archivo vacio");

    EditorConfig::limpiarRaizAssets();
}

// 6. Instanciar prefab inexistente
void instanciarPrefabInexistente() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_noexiste");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    Prefab prefab("NoExiste");
    GameObject* resultado = prefab.instanciar(&editor, raiz);
    CHECK(resultado == nullptr, "instanciar devuelve nullptr para prefab inexistente");

    EditorConfig::limpiarRaizAssets();
}

// 7. Multiples instancias del mismo prefab
void multiplesInstanciasMismoPrefab() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_multiples");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "MultiPrefab");
    Transform* t = objeto->getComponent<Transform>();
    t->setTranslatef(1.0f, 1.0f, 1.0f);
    GameObject* creado = editor.createGameObject(std::move(objeto), raiz);

    Prefab prefab("MultiPrefab");
    CHECK(prefab.guardarDesdeObjeto(creado, &editor), "prefab guardado");

    // Instanciar 3 veces en la MISMA escena para verificar IDs unicos
    std::vector<int> ids;
    for (int i = 0; i < 3; ++i) {
        GameObject* inst = prefab.instanciar(&editor, raiz);
        CHECK(inst != nullptr, "instancia " + std::to_string(i+1) + " creada");
        if (inst) {
            float posEsp[3] = {1.0f, 1.0f, 1.0f};
            verificarObjetoBasico(inst, "MultiPrefab", "Untagged", posEsp);
            CHECK(std::find(ids.begin(), ids.end(), inst->getId()) == ids.end(),
                  "cada instancia tiene ID unico dentro de la escena");
            ids.push_back(inst->getId());
        }
    }

    EditorConfig::limpiarRaizAssets();
}

// 8. Prefab con hijo que tiene componente adicional
void prefabHijoConComponenteExtra() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_hijo_extra");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EditorController* editor = nullptr;
    EventBus events;
    AssetManager assets;
    GameObject* raiz = crearEscenaBasica(registry, editor, &events, &assets);

    auto padreObj = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(padreObj->inputName, sizeof(padreObj->inputName), "PadreConColor");
    auto colorPadre = std::make_unique<Color>();
    colorPadre->setColor(1.0f, 0.0f, 0.0f);
    padreObj->addComponent(std::move(colorPadre));
    GameObject* padre = editor->createGameObject(std::move(padreObj), raiz);

    auto hijoObj = GameObjectFactory::createSimpleObject(padre);
    std::snprintf(hijoObj->inputName, sizeof(hijoObj->inputName), "HijoConModelo");
    auto modelo = std::make_unique<Model>();
    modelo->setPath("Modelos/arma.obj");
    hijoObj->addComponent(std::move(modelo));
    GameObject* hijo = editor->createGameObject(std::move(hijoObj), padre);

    Prefab prefab("PadreConHijoCompleto");
    CHECK(prefab.guardarDesdeObjeto(padre, editor), "prefab con componentes variados guardado");

    delete editor;

    SceneRegistry registry2;
    EditorController* editor2 = nullptr;
    EventBus events2;
    AssetManager assets2;
    GameObject* raiz2 = crearEscenaBasica(registry2, editor2, &events2, &assets2);

    GameObject* inst = prefab.instanciar(editor2, raiz2);
    CHECK(inst != nullptr, "prefab complejo instanciado");

    Color* cp = inst->getComponent<Color>();
    CHECK(cp != nullptr, "padre tiene Color");
    if (cp) {
        const float* c = cp->getColor();
        CHECK(std::abs(c[0] - 1.0f) < 0.001f && c[1] == 0.0f && c[2] == 0.0f,
              "Color del padre correcto");
    }

    GameObject* hijoInst = nullptr;
    for (auto* e : inst->getChildEntities()) {
        auto* go = dynamic_cast<GameObject*>(e);
        if (go && std::string(go->inputName) == "HijoConModelo") {
            hijoInst = go;
            break;
        }
    }
    CHECK(hijoInst != nullptr, "hijo instanciado");
    if (hijoInst) {
        Model* mh = hijoInst->getComponent<Model>();
        CHECK(mh != nullptr, "hijo tiene Model");
        if (mh) {
            std::string path = mh->getPath();
            CHECK(path.find("Modelos/arma.obj") != std::string::npos,
                  "path del Model del hijo correcto: " + path);
        }
    }

    delete editor2;
    EditorConfig::limpiarRaizAssets();
}

// --- Main --------------------------------------------------------------------

int main() {
    roundTripPrefabBasico();
    roundTripPrefabJerarquia();
    roundTripPrefabConModel();
    prefabLibraryCacheYValidacion();
    validarArchivoEstatico();
    instanciarPrefabInexistente();
    multiplesInstanciasMismoPrefab();
    prefabHijoConComponenteExtra();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
