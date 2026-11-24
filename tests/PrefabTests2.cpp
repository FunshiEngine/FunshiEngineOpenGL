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
#include "PrefabTestsDetalle.h"

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

// 7. La API de scripts lista los prefabs con punteros validos.
// `servicios->listarPrefabs()` devolvia los c_str() de un vector<string>
// local que se destruia al salir: los punteros quedaban colgantes (los lee
// bajo ASan el ctest). El contrato los hace validos hasta la llamada
// siguiente, asi que cada llamada se copia en el acto y se compara copias.
void serviciosListarPrefabs() {
    TempPruebas::CarpetaPrueba carpeta("funshi_prefab_servicios");
    const fs::path base = carpeta.ruta();

    EditorConfig::limpiarRaizAssets();
    EditorConfig::fijarRaizAssets(base.string());

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "ParaServicios");
    GameObject* creado = editor.createGameObject(std::move(objeto), raiz);
    CHECK(creado != nullptr, "objeto para servicios creado");

    Prefab prefab("ParaServicios");
    CHECK(prefab.guardarDesdeObjeto(creado, &editor), "prefab se guarda para servicios");

    PrefabLibrary* biblioteca = editor.getPrefabLibrary();
    CHECK(biblioteca != nullptr, "el editor expone su biblioteca de prefabs");
    if (!biblioteca) {
        EditorConfig::limpiarRaizAssets();
        return;
    }
    biblioteca->recargar();

    MotorScript::inyectarServiciosScript(nullptr, &registry, nullptr, nullptr,
                                         &editor);
    const MotorScript::ScriptServices* servicios = MotorScript::tablaServicios();
    const char** primera = servicios->listarPrefabs();
    CHECK(primera != nullptr, "listarPrefabs devuelve el array");
    std::set<std::string> antes;
    if (primera) {
        int cantidad = 0;
        for (const char** p = primera; *p; ++p) {
            ++cantidad;
            antes.insert(*p);
        }
        CHECK(cantidad >= 1, "listarPrefabs incluye al menos un prefab");
        CHECK(antes.count("ParaServicios") == 1,
              "listarPrefabs incluye el prefab guardado");
    }

    // Una segunda llamada reconstruye los mismos nombres; se comparan copias
    // porque el array es valido solo hasta la llamada siguiente.
    const char** segunda = servicios->listarPrefabs();
    CHECK(segunda != nullptr, "segunda llamada a listarPrefabs devuelve el array");
    if (segunda) {
        std::set<std::string> despues;
        for (const char** p = segunda; *p; ++p) despues.insert(*p);
        CHECK(despues == antes, "los nombres se mantienen entre llamadas");
    }

    MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
    EditorConfig::limpiarRaizAssets();
}

// --- Main --------------------------------------------------------------------

// El nombre con el que se persiste un componente tiene que ser el mismo en
// Windows y en Linux, porque es la clave con la que el catalogo lo reconoce al
// leer. MSVC deja el prefijo de class-key en typeid().name() ("class
// Transform"): con ese nombre, el Transform del objeto no era el que se
// cargaba (se creaba un segundo) y la posicion guardada se perdia.
void nombresDeComponentePortables() {
    CHECK(nombreSimpleTipo(typeid(Transform).name()) == "Transform",
          "el nombre simple de Transform no lleva namespace ni prefijo");
    CHECK(nombreSimpleTipo(typeid(Color).name()) == "Color",
          "el nombre simple de Color no lleva namespace ni prefijo");

    CHECK(nombreSimpleTipo("class Transform") == "Transform",
          "el prefijo de class-key de MSVC se quita");
    CHECK(nombreSimpleTipo("struct Objetos::Componentes::Color") == "Color",
          "del nombre con namespace se queda el ultimo componente");

    // Nombres con 'E' o que empiezan por mayuscula: el recorte por 'E' del
    // mangling de GCC los truncaba ("SpriteRenderer" -> "SpriteRender",
    // "EsfereCollider" -> ""), y el nombre vacio hacia fallar la carga entera.
    CHECK(nombreSimpleTipo("SpriteRenderer") == "SpriteRenderer",
          "un nombre con 'E' no se trunca");
    CHECK(nombreSimpleTipo("EsfereCollider") == "EsfereCollider",
          "un nombre que empieza por 'E' no queda vacio");

    // El catalogo reconoce el tipo con cualquiera de las formas del nombre.
    const auto& catalogo = CatalogoComponentes::instancia();
    CHECK(catalogo.buscar("Transform") != nullptr,
          "el catalogo encuentra Transform por su nombre simple");
    CHECK(catalogo.buscar("class Transform") != nullptr,
          "el catalogo encuentra Transform por el nombre de MSVC");
    CHECK(catalogo.buscar("Objetos::Componentes::Transform") != nullptr,
          "el catalogo encuentra Transform por su nombre con namespace");
}

int main() {
    nombresDeComponentePortables();
    roundTripPrefabBasico();
    roundTripPrefabJerarquia();
    roundTripPrefabConModel();
    prefabLibraryCacheYValidacion();
    recargarNoCreaCarpeta();
    validarArchivoEstatico();
    instanciarPrefabInexistente();
    multiplesInstanciasMismoPrefab();
    prefabHijoConComponenteExtra();
    serviciosListarPrefabs();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
