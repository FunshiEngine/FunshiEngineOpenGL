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
#include "SceneSerializationTestsDetalle.h"

namespace SceneSerializationTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace SceneSerializationTestsDetalle
using namespace SceneSerializationTestsDetalle;

// --- Nombres por defecto ------------------------------------------------------
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

void restaurarBaselineDeEscena() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_escena_baseline");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    GameObject* raiz = registry.getRoot();
    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "Baseline");
    Transform* transformOriginal = objeto->getComponent<Transform>();
    CHECK(transformOriginal != nullptr, "el objeto baseline tiene Transform");
    if (transformOriginal) transformOriginal->setTranslatef(3.f, 4.f, 5.f);
    GameObject* original = editor.createGameObject(std::move(objeto), raiz);
    CHECK(original != nullptr, "se crea el objeto baseline");
    const int idOriginal = original ? original->getId() : -1;
    serializer.save(prefijo);

    if (original) {
        original->getComponent<Transform>()->setTranslatef(90.f, 80.f, 70.f);
        editor.deleteGameObject(original);
    }
    auto agregado = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(agregado->inputName, sizeof(agregado->inputName), "Temporal");
    CHECK(editor.createGameObject(std::move(agregado), raiz) != nullptr,
          "se agrega un objeto durante la simulacion");

    serializer.load(pathTxt, semiPath);

    GameObject* restaurado = nullptr;
    for (auto* entidad : registry.getRoot()->getChildEntities()) {
        auto* objetoEscena = dynamic_cast<GameObject*>(entidad);
        if (objetoEscena && std::string(objetoEscena->inputName) == "Baseline")
            restaurado = objetoEscena;
        CHECK(!objetoEscena ||
                  std::string(objetoEscena->inputName) != "Temporal",
              "la restauracion elimina objetos agregados durante la simulacion");
    }
    CHECK(restaurado != nullptr,
          "la restauracion recupera un objeto eliminado durante la simulacion");
    CHECK(restaurado && restaurado->getId() == idOriginal,
          "la restauracion conserva el id del objeto baseline");
    Transform* transformRestaurado =
        restaurado ? restaurado->getComponent<Transform>() : nullptr;
    CHECK(transformRestaurado &&
              std::abs(transformRestaurado->getTranslatef()[0] - 3.f) < 0.001f &&
              std::abs(transformRestaurado->getTranslatef()[1] - 4.f) < 0.001f &&
              std::abs(transformRestaurado->getTranslatef()[2] - 5.f) < 0.001f,
          "la restauracion recupera la pose inicial completa");
}

// --- Guardar con el arbol vacio -----------------------------------------------
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
    CHECK(arbol && arbol->isEmpty(), "se vacia el arbol al guardarlo vacio");

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
// por el que un objeto sin nombre pudo pasar sin que nada lo notara.
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
            creado->setTag("suelo");

            auto collider = std::make_unique<EsfereCollider>(
                1.0f, creado->getComponent<Transform>(), creado);
            EsfereCollider* colliderPtr = collider.get();
            colliderPtr->setVisibleEnEscena(true);
            CHECK(editor.addComponent(creado, std::move(collider)),
                  "se agrega un collider visible a la escena");
            auto rigidBody = std::make_unique<RigidBody>(colliderPtr, 1.0f);
            RigidBody* rigidBodyPtr = rigidBody.get();
            rigidBodyPtr->setActivo(false);
            CHECK(editor.addComponent(creado, std::move(rigidBody)),
                  "se agrega un RigidBody desactivado a la escena");

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
                CHECK(go->getTag() == "suelo",
                      "el tag del objeto sobrevive el guardado de escena");
                Collider* collider = go->getComponent<Collider>();
                CHECK(collider && collider->estaVisibleEnEscena(),
                      "la visibilidad del collider sobrevive la escena completa");
                RigidBody* rigidBody = go->getComponent<RigidBody>();
                CHECK(rigidBody && !rigidBody->estaActivo(),
                      "el estado del RigidBody sobrevive la escena completa");
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

void tagsCompatiblesConEscenasAnteriores() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_tags_legacy");
    std::error_code ec;
    GameObject original;
    original.setId(841);
    original.setTag("suelo");
    CHECK(original.saveEntity(carpetaDir.ruta().string()),
          "el objeto etiquetado se guarda");

    const fs::path archivo =
        carpetaDir.ruta() / "ObjectN841.db";
    const auto tamano = fs::file_size(archivo, ec);
    CHECK(!ec && tamano > 12 + std::string("suelo").size(),
          "el archivo contiene el bloque de tag al final");
    if (ec || tamano <= 12 + std::string("suelo").size()) return;

    GameObject recuperado;
    recuperado.setId(841);
    CHECK(recuperado.loadEntity(carpetaDir.ruta().string()),
          "el tag se lee desde el formato actual");
    CHECK(recuperado.getTag() == "suelo",
          "la lectura actual conserva el tag");
    const std::vector<std::string> tags = TagRegistry::registrados();
    CHECK(std::find(tags.begin(), tags.end(), "suelo") != tags.end(),
          "el tag cargado queda disponible en el registro del inspector");

    fs::resize_file(archivo, tamano - 12 - std::string("suelo").size(), ec);
    CHECK(!ec, "se prepara un objeto con formato anterior sin bloque de tag");
    if (ec) return;
    GameObject legacy;
    legacy.setId(841);
    CHECK(legacy.loadEntity(carpetaDir.ruta().string()),
          "el formato anterior se carga sin bloque de tag");
    CHECK(legacy.getTag() == "Untagged",
          "una escena anterior asigna el tag predeterminado");
}

// --- Reporte de fallos en Binario ---------------------------------------------
// Si el archivo no se pudo abrir para escritura (directorio inexistente, sin
// permisos), ofOpenBinary() debe devolver false y avisar en el log, en vez de
// dejar que cada write() falle en silencio. Lo mismo al leer: ifOpenBinary()
// devuelve false y avisa en vez de dejar los campos intactos (que es el
// mecanismo que hacia parecer que el objeto "perdia" el nombre o sus
// componentes).
void reporteFalloBinario() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_binario_fallo");
    const fs::path dir = carpetaDir.ruta();

    // 1. Escritura en directorio inexistente: falla, devuelve false y loguea.
    const std::string rutaInexistente =
        (dir / "carpeta_que_no_existe" / "archivo.db").string();
    Binario binEscritura(rutaInexistente);

    std::ostringstream logEscritura;
    std::streambuf* buferViejo = std::cerr.rdbuf(logEscritura.rdbuf());
    const bool okEscritura = binEscritura.ofOpenBinary();
    std::cerr.rdbuf(buferViejo);

    CHECK(!okEscritura, "ofOpenBinary devuelve false cuando no puede abrir");
    CHECK(logEscritura.str().find("[binario]") != std::string::npos,
          "ofOpenBinary avisa en el log cuando falla la apertura");

    // 2. Lectura de archivo inexistente: falla, devuelve false y loguea.
    const std::string rutaLecturaInexistente =
        (dir / "archivo_no_creado.db").string();
    Binario binLectura(rutaLecturaInexistente);

    std::ostringstream logLectura;
    buferViejo = std::cerr.rdbuf(logLectura.rdbuf());
    const bool okLectura = binLectura.ifOpenBinary();
    std::cerr.rdbuf(buferViejo);

    CHECK(!okLectura, "ifOpenBinary devuelve false si el archivo no existe");
    CHECK(logLectura.str().find("[binario]") != std::string::npos,
          "ifOpenBinary avisa en el log cuando no encuentra el archivo");

    // 3. Propagacion a GameObject::saveEntity / loadEntity: devuelven false.
    auto obj = GameObjectFactory::createSimpleObject(nullptr);
    CHECK(!obj->saveEntity((dir / "carpeta_que_no_existe").string()),
          "saveEntity propaga el fallo de ofOpenBinary");
    CHECK(!obj->loadEntity((dir / "carpeta_que_no_existe").string()),
          "loadEntity propaga el fallo de ifOpenBinary");

    // 4. Camino exitoso: devuelve true y no loguea error.
    const std::string rutaBuena = (dir / "bueno.db").string();
    Binario binBueno(rutaBuena);
    CHECK(binBueno.ofOpenBinary(), "ofOpenBinary devuelve true en ruta valida");
    binBueno.ofCloseBinary();
    CHECK(binBueno.ifOpenBinary(), "ifOpenBinary devuelve true sobre archivo existente");
    binBueno.ifCloseBinary();
}
