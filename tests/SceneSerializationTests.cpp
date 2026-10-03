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
// round-trip de escena, un objeto sin nombre o cualquier regresion de
// guardado pasaban sin que nada lo notara.
//
// Tambien cubre el vinculo del Inspector (Settings) con el objeto que muestra:
// es el estado de GUI que mas depende de la vida del objeto, y su ciclo de vida
// se puede verificar headless sin dibujar (borrar el objeto -> el inspector
// tiene que desvincularse).
//
// Patron de autoria (ver AGENTS.md): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>

#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Assets/AssetManager.h"
#include "../FunshiEngineGL/src/Assets/Mesh.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Events/EventBus.h"
#include "../FunshiEngineGL/src/GUI/ObjetosGUI/SettingsObjectInterface.h"
#include "../FunshiEngineGL/src/Herramientas/PathUtils.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Material.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Model.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Skybox.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Transform.h"
#include "../FunshiEngineGL/src/Objetos/SimpleObject.h"
#include "../FunshiEngineGL/src/Rendering/DibujoModelo.h"
#include "../FunshiEngineGL/src/Scenes/EditorController.h"
#include "../FunshiEngineGL/src/Scenes/RutasReescritura.h"
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

// --- Lineas corruptas en SceneBBDDObjetos.txt ---------------------------------
// Una linea del indice que resuelve a id=0 dentro del bloque de hijos hacia que
// loadPreOrder lea ObjectN0.db (el contenido de la raiz): nace un hijo que se
// llama "Scene" como la raiz y, al guardarse con id propio, se auto-propaga para
// siempre. El fantasma observado por el usuario (4 hijos "Scene" de 181 bytes en
// ObjectN2..N5) era exactamente eso. Tres variantes de linea que hoy reproducen
// el fantasma; tras el fix tienen que saltarse con aviso y sin romper la
// estructura del arbol.
namespace {

// Guarda la escena base (raiz + hijo "Victima") y devuelve la ruta del indice.
std::string guardarEscenaVictima(const fs::path& base) {
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    GameObject* raiz = registry.getRoot();
    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    std::snprintf(objeto->inputName, sizeof(objeto->inputName), "Victima");
    GameObject* creado = editor.createGameObject(std::move(objeto), raiz);
    CHECK(creado != nullptr, "se crea la escena base con su hijo");

    serializer.save(prefijo);
    return (base / "SceneBBDDObjetos.txt").string();
}

struct ResultadoCarga {
    bool fantasma = false;      // hijo llamado "Scene" (ademas de la raiz)
    bool victimaPresente = true; // el hijo real sobrevivio
    std::string aviso;          // lo que dijo std::cerr durante la carga
};

// Carga un indice (corrupto o no) en una escena nueva y mide el efecto.
ResultadoCarga cargarYMedir(const std::string& pathTxt,
                            const std::string& semiPath) {
    ResultadoCarga r;

    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    serializer.load(pathTxt, semiPath);
    std::cerr.rdbuf(buferAnterior);
    r.aviso = aviso.str();

    GameObject* raiz = registry.getRoot();
    r.victimaPresente = false;
    if (raiz) {
        for (auto* hijo : raiz->getChildEntities()) {
            auto* go = dynamic_cast<GameObject*>(hijo);
            if (!go) continue;
            const std::string nombre = go->inputName;
            if (nombre == "Scene") r.fantasma = true;
            if (nombre == "Victima") r.victimaPresente = true;
        }
    }
    return r;
}

std::string leerTodo(const std::string& ruta) {
    std::ifstream archivo(ruta, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(archivo),
                       std::istreambuf_iterator<char>());
}

} // namespace

void indiceCorruptoSinFantasmas() {
    // T1: linea basura (sin "ObjectN"): hoy el id queda en su default 0 y se
    // lee ObjectN0.db -> fantasma "Scene" SIN ningun aviso en el log.
    // T2: "ObjectN.db" (id vacio -> stoi falla): hoy setId(0) -> fantasma.
    // T3: "ObjectN0.db" duplicado en posicion de hijo: hoy lee la raiz ->
    // fantasma byte-identico al observado por el usuario.
    struct Caso {
        const char* descripcion;
        const char* nombreCarpeta;
        const char* indiceCorrupto;
    };
    const Caso casos[] = {
        {"T1: linea basura en el bloque de hijos", "funshi_h17_basura",
         "ObjectN0.db\n=>\nObjectN1.db\nbasura\n<=\n"},
        {"T2: ObjectN.db con id vacio", "funshi_h17_idvacio",
         "ObjectN0.db\n=>\nObjectN1.db\nObjectN.db\n<=\n"},
        {"T3: ObjectN0.db duplicado como hijo", "funshi_h17_duplicado",
         "ObjectN0.db\n=>\nObjectN1.db\nObjectN0.db\n<=\n"},
    };

    for (const Caso& c : casos) {
        TempPruebas::CarpetaPrueba carpetaDir(c.nombreCarpeta);
        const fs::path base = carpetaDir.ruta();

        const std::string pathTxt = guardarEscenaVictima(base);
        const std::string semiPath = (base / "Scene").string() + "/";

        {
            std::ofstream indice(pathTxt, std::ios::trunc);
            indice << c.indiceCorrupto;
        }

        const ResultadoCarga r = cargarYMedir(pathTxt, semiPath);

        CHECK(!r.fantasma,
              std::string(c.descripcion) + ": no nace un fantasma 'Scene'");
        CHECK(r.victimaPresente,
              std::string(c.descripcion) + ": el hijo real sigue cargado");
        CHECK(r.aviso.find("[escena]") != std::string::npos,
              std::string(c.descripcion) + ": la linea mala se avisa en el log");

        // Estructura: con el fantasma fuera, guardar y recargar otra vez
        // tiene que devolver exactamente la misma escena (sin propagacion).
        {
            SceneRegistry registry;
            EventBus events;
            AssetManager assets;
            EditorController editor(&registry, nullptr, &events, &assets);
            SceneSerializer serializer(&registry, &editor, &assets);
            serializer.load(pathTxt, semiPath);
            serializer.save((base / "Scene").string());
        }
        const ResultadoCarga segunda = cargarYMedir(pathTxt, semiPath);
        CHECK(!segunda.fantasma,
              std::string(c.descripcion) +
                  ": tras guardar de nuevo no aparece el fantasma");
        CHECK(segunda.victimaPresente,
              std::string(c.descripcion) +
                  ": el ciclo guardar-cargar es estable");
    }
}

// --- Variante de guardado: hijo con id=0 --------------------------------------
// El campo "Id" del inspector no validaba nada y un segundo "Confirmar"
// fijaba id=0 en cualquier hijo (SettingsObjectInterface). Al guardar, ese
// hijo escribia ObjectN0.db: pisaba el contenido de la RAIZ y dejaba una
// segunda linea "ObjectN0.db" en el indice (semilla del fantasma). El save
// tiene que reasignarle un id libre antes de escribir nada, con aviso.
void hijoConIdCeroSeReasignaAlGuardar() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_h17_idcero_guardar");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();

    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);

        GameObject* raiz = registry.getRoot();
        auto objeto = GameObjectFactory::createSimpleObject(raiz);
        std::snprintf(objeto->inputName, sizeof(objeto->inputName), "Victima");
        GameObject* creado = editor.createGameObject(std::move(objeto), raiz);
        CHECK(creado != nullptr, "se crea el hijo que quedara con id=0");
        if (creado) creado->setId(0);

        std::ostringstream aviso;
        std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
        serializer.save(prefijo);
        std::cerr.rdbuf(buferAnterior);

        CHECK(aviso.str().find("[escena]") != std::string::npos,
              "guardar un hijo con id=0 avisa en el log");
    }

    // El indice tiene que tener UNA sola linea ObjectN0.db (la raiz).
    int lineasCero = 0;
    std::string lineaDelHijo;
    {
        std::ifstream indice(pathTxt);
        std::string linea;
        while (std::getline(indice, linea)) {
            if (linea == "ObjectN0.db") ++lineasCero;
            else if (linea.rfind("ObjectN", 0) == 0) lineaDelHijo = linea;
        }
    }
    CHECK(lineasCero == 1,
          "el indice no contiene una segunda linea ObjectN0.db");
    CHECK(lineaDelHijo == "ObjectN1.db",
          "el hijo con id=0 se reasigna a un id libre antes de escribir");

    // ObjectN0.db conserva el contenido de la RAIZ (no el del hijo).
    const std::string bytesRaiz = leerTodo((base / "Scene" / "ObjectN0.db").string());
    CHECK(bytesRaiz.find("Victima") == std::string::npos,
          "ObjectN0.db sigue teniendo el contenido de la raiz");

    // Y el hijo reasignado tiene su propio archivo con su nombre.
    const std::string bytesHijo =
        leerTodo((base / "Scene" / "ObjectN1.db").string());
    CHECK(bytesHijo.find("Victima") != std::string::npos,
          "el hijo reasignado guarda su propio .db con su nombre");
}

// --- Causa raíz: el look-ahead pierde la posición con ≥2 hermanos -------------
// loadPreOrder lee SceneBBDDObjetos.txt en modo texto y el look-ahead hace
// seekg(tellg()) tras getline; en MinGW/Windows eso no es idempotente: la
// releitura de la linea del siguiente hermano arranca en un offset erroneo
// (medido con sondas: +2 a +6, depende del bufer) y la linea sale truncada
// ("ectN2.db"). Con el codigo viejo find("ObjectN") fallaba -> id 0 ->
// fantasma "Scene" sin aviso; con solo la validacion estricta el hermano
// sano se saltearia. La correccion de fondo es leer el indice en binario.
void hermanosConsecutivosSinPerdida() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_h17_hermanos");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    // Escena: raiz + 4 hermanos consecutivos en el MISMO nivel (el caso que
    // el round-trip existente no cubria: ahi el look-ahead si hace seekg).
    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);

        GameObject* raiz = registry.getRoot();
        int creados = 0;
        for (int i = 1; i <= 4; ++i) {
            auto objeto = GameObjectFactory::createSimpleObject(raiz);
            std::snprintf(objeto->inputName, sizeof(objeto->inputName),
                          "Hermano%d", i);
            if (editor.createGameObject(std::move(objeto), raiz)) ++creados;
        }
        CHECK(creados == 4, "se crean los cuatro hermanos consecutivos");
        serializer.save(prefijo);
    }

    // Recargar: los cuatro tienen que volver completos.
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    serializer.load(pathTxt, semiPath);
    std::cerr.rdbuf(buferAnterior);

    int hermanosCargados = 0;
    int idsCorrectos = 0;
    if (GameObject* raiz = registry.getRoot()) {
        for (auto* hijo : raiz->getChildEntities()) {
            auto* go = dynamic_cast<GameObject*>(hijo);
            if (!go) continue;
            const std::string nombre = go->inputName;
            if (nombre.rfind("Hermano", 0) != 0) continue;
            ++hermanosCargados;
            const int esperado = nombre.back() - '0';
            if (go->getId() == esperado) ++idsCorrectos;
        }
    }
    CHECK(hermanosCargados == 4,
          "sobreviven los cuatro hermanos consecutivos (el look-ahead no "
          "pierde la posicion de ninguna linea)");
    CHECK(idsCorrectos == 4,
          "cada hermano conserva su id (ninguno se releyo truncado)");
    CHECK(aviso.str().find("linea invalida") == std::string::npos,
          "sin lineas invalidas: la releertura del look-ahead es exacta");
}

// --- Rutas de las caras del Skybox -------------------------------------------
// Las seis caras del cubemap son assets del proyecto, asi que se guardan
// relativas a la raiz de assets y se resuelven a absolutas al cargar, igual que
// la malla, las texturas y el script. Sin eso la ruta decia una cosa (relativa
// al proyecto) y hacia otra: se buscaba contra el directorio de trabajo del
// proceso, la fecha de modificacion daba 0 y el cache de la textura no se
// invalidaba nunca.

// Lectura de una ruta tal cual quedo escrita, sin pasar por absolutizar: es la
// forma de comprobar QUE se persisto, no de reconstruirla.
static std::string leerRutaPersistida(std::istream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    std::string s;
    if (len > 0) {
        s.resize(len);
        in.read(&s[0], len);
    }
    return s;
}

// Escritura cruda de una ruta, para armar a mano un archivo con el formato
// viejo (rutas absolutas tal cual, sin pasar por relativizar).
static void escribirRutaPersistida(std::ostream& out, const std::string& s) {
    const uint32_t len = static_cast<uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) out.write(s.data(), len);
}

void rutasDeCarasDeSkybox() {
    EditorConfig::limpiarRaizAssets();
    const std::string raiz = "/motor/MotorGrafico/Juego/srcJuego";
    EditorConfig::fijarRaizAssets(raiz);

    const std::string relativaPX = "Cielo/cielo_px.png";
    const std::string absolutaPX = raiz + "/" + relativaPX;
    const std::string relativaNY = "Cielo/cielo_ny.png";
    const std::string absolutaNY = raiz + "/" + relativaNY;

    // 1. Lo que queda EN DISCO son las rutas relativas, no las absolutas con las
    //    que se compuso el componente.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_rutas");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        {
            Skybox sky;
            sky.setCaraMasX(absolutaPX);
            sky.setCaraMenosY(absolutaNY);
            std::ofstream out(archivo, std::ios::binary);
            sky.saveComponent(&out);
        }
        std::ifstream in(archivo, std::ios::binary);
        CHECK(leerRutaPersistida(in) == relativaPX,
              "la cara +X se persiste relativa a la raiz de assets");
        CHECK(leerRutaPersistida(in) == "",
              "una cara sin asignar se persiste vacia");
        CHECK(leerRutaPersistida(in) == "",
              "otra cara sin asignar se persiste vacia");
        CHECK(leerRutaPersistida(in) == relativaNY,
              "la cara -Y se persiste relativa a la raiz de assets");
    }

    // 2. Un archivo con rutas RELATIVAS al cargar queda con rutas ABSOLUTAS en
    //    memoria, que es como las usa la pasada que dibuja el cubemap. Se arma
    //    el archivo a mano para comprobar la resolucion, no el viaje de ida.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_carga");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        const bool visible = true;
        {
            std::ofstream out(archivo, std::ios::binary);
            escribirRutaPersistida(out, relativaPX);
            for (int i = 0; i < 5; ++i) escribirRutaPersistida(out, "");
            out.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absolutaPX,
              "una ruta relativa se resuelve a absoluta al cargar");
        CHECK(sky.getCaraMenosY().empty(),
              "una cara sin asignar sigue vacia al cargar");
    }

    // 3. Ida y vuelta completa: componer con absolutas, guardar y recargar deja
    //    las mismas absolutas.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_ida_vuelta");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        {
            Skybox sky;
            sky.setCaraMasX(absolutaPX);
            sky.setCaraMenosY(absolutaNY);
            std::ofstream out(archivo, std::ios::binary);
            sky.saveComponent(&out);
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absolutaPX,
              "ida y vuelta: la cara +X conserva su ruta");
        CHECK(sky.getCaraMenosY() == absolutaNY,
              "ida y vuelta: la cara -Y conserva su ruta");
    }

    // 3. Escena legacy: una cara ABSOLUTA guardada por el formato viejo se carga
    //    intacta, sin volver a anteponerle la raiz.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_legacy");
        const fs::path archivo = carpeta.ruta() / "skybox_legacy.bin";
        const std::string absoluta = "/legacy/Cielo/absoluta.png";
        const bool visible = true;
        {
            std::ofstream out(archivo, std::ios::binary);
            for (int i = 0; i < 6; ++i) escribirRutaPersistida(out, absoluta);
            out.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absoluta,
              "una ruta absoluta legacy se deja intacta al cargar");
    }

    EditorConfig::limpiarRaizAssets();
}

// --- Reescritura de referencias al mover/renombrar ----------------------------
// El explorador publica la ruta con el separador nativo (std::filesystem) y la
// escena resuelve sus rutas con '/': el cotejo de prefijos tiene que tratar
// ambos como el mismo separador (en Windows) para que mover o renombrar una
// carpeta reescriba mallas, texturas y scripts en vez de devolver 0 cambios
// en silencio y dejar los .db apuntando al lugar viejo.
void reescrituraDeReferencias() {
    EditorConfig::limpiarRaizAssets();
    const std::string raiz = "C:\\base\\MotorGrafico/Proyects/JuegoX/srcJuegoX";
    EditorConfig::fijarRaizAssets(raiz);

    // Rutas en memoria tal como quedan al cargar la escena (absolutizar
    // concatena la raiz con '/').
    const std::string scriptEnEscena =
        EditorConfig::absolutizarRuta("Scripts/cpp.cpp");
    const std::string mallaEnEscena =
        EditorConfig::absolutizarRuta("Mallas/Auto.fbx");
    const std::string texturaEnEscena =
        EditorConfig::absolutizarRuta("Texturas/difuso.png");

    // 1. Mover `Scripts` dentro de `Assets`: solo la fuente esta bajo el
    //    prefijo movido; malla y textura quedan intactas.
    const std::string prefijoScripts = raiz + PATH_SEP + "Scripts";
    const std::string destinoScripts =
        raiz + PATH_SEP + "Assets" + PATH_SEP + "Scripts";

    // Componentes con las tres familias de asset que reescribe el modulo.
    GameObject objeto;
    auto script = std::make_unique<Script>();
    script->setDllPath(scriptEnEscena);
    Script* refScript = script.get();
    objeto.addComponent(std::move(script));

    auto modelo = std::make_unique<Model>();
    modelo->setPath(mallaEnEscena);
    Model* refModelo = modelo.get();
    objeto.addComponent(std::move(modelo));

    auto material = std::make_unique<Material>();
    material->setDiffuseMapPath(texturaEnEscena);
    Material* refMaterial = material.get();
    objeto.addComponent(std::move(material));

    ListaDE<GameObject*> escena;
    escena.addLast(&objeto);

    const int mover = RutasReescritura::reescribirEnEscena(
        &escena, prefijoScripts, destinoScripts);
    CHECK(mover == 1, "mover la carpeta reescribe la fuente del script");
    CHECK(refScript->getPath() == destinoScripts + "/cpp.cpp",
          "la fuente del script queda bajo la carpeta destino");
    CHECK(refModelo->getPath() == mallaEnEscena,
          "la malla (fuera del prefijo movido) no se toca");
    CHECK(refMaterial->getDiffuseMapPath() == texturaEnEscena,
          "la textura (fuera del prefijo movido) no se toca");

    // 2. Renombrar la carpeta de la malla: reescribe la malla y nada mas.
    const int renombrarMalla = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "Mallas", raiz + PATH_SEP + "MallasNuevas");
    CHECK(renombrarMalla == 1,
          "renombrar una carpeta reescribe la referencia de la malla");
    CHECK(refModelo->getPath() ==
              raiz + PATH_SEP + "MallasNuevas/Auto.fbx",
          "la malla queda bajo el nombre nuevo");

    // 3. Renombrar la carpeta de texturas: reescribe la textura.
    const int renombrarTextura = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "Texturas",
        raiz + PATH_SEP + "TexturasNuevas");
    CHECK(renombrarTextura == 1,
          "renombrar una carpeta reescribe la textura del material");
    CHECK(refMaterial->getDiffuseMapPath() ==
              raiz + PATH_SEP + "TexturasNuevas/difuso.png",
          "la textura queda bajo el nombre nuevo");

    // 4. Un prefijo que no corresponde a ninguna referencia no cambia nada.
    const int sinCambios = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "OtraCarpeta",
        raiz + PATH_SEP + "NadaCarpeta");
    CHECK(sinCambios == 0, "prefijo que no matchea no cambia nada");

    EditorConfig::limpiarRaizAssets();
}

// --- Sanado de rutas rotas al cargar ------------------------------------------
// Una escena guardada con referencias que ya no resuelven (daño anterior al
// arreglo de los separadores, o archivos movidos fuera del motor) no puede
// quedarse asi: si el nombre base aparece UNA vez bajo la raiz de assets, la
// referencia se repara y se avisa en el log; con varias coincidencias o con
// ninguna no se adivina y la ruta queda como estaba. Sin raiz de assets no hay
// donde buscar, asi que no se hace nada.
void sanadoDeRutasRotas() {
    TempPruebas::CarpetaPrueba carpeta("funshi_sanado_rutas");
    const fs::path raiz = carpeta.ruta();
    std::error_code ec;

    // El asset real vive en Assets/Scripts, mientras que la escena cree que
    // sigue en Scripts: una sola coincidencia del nombre base.
    fs::create_directories(raiz / "Assets" / "Scripts", ec);
    {
        std::ofstream fuente(
            (raiz / "Assets" / "Scripts" / "cpp.cpp").string());
        fuente << "// fuente del script\n";
    }
    const fs::path real = raiz / "Assets" / "Scripts" / "cpp.cpp";

    EditorConfig::fijarRaizAssets(raiz.string());

    GameObject objeto;

    // Rota + unica coincidencia -> se repara.
    auto script = std::make_unique<Script>();
    script->setDllPath(EditorConfig::absolutizarRuta("Scripts/cpp.cpp"));
    Script* refScript = script.get();
    objeto.addComponent(std::move(script));

    // Rota + ninguna coincidencia -> se avisa y no se toca.
    auto material = std::make_unique<Material>();
    material->setDiffuseMapPath(
        EditorConfig::absolutizarRuta("Texturas/difuso.png"));
    Material* refMaterial = material.get();
    objeto.addComponent(std::move(material));

    // Sana desde el inicio -> no se toca.
    auto modelo = std::make_unique<Model>();
    const std::string existente = real.string();
    modelo->setPath(existente);
    Model* refModelo = modelo.get();
    objeto.addComponent(std::move(modelo));

    ListaDE<GameObject*> escena;
    escena.addLast(&objeto);

    const int reparadas = RutasReescritura::sanarRutasInexistentes(&escena);
    CHECK(reparadas == 1, "con una coincidencia unica se repara");
    CHECK(fs::path(refScript->getPath()).generic_string() ==
              real.generic_string(),
          "la fuente del script queda apuntando al archivo que existe");
    CHECK(refMaterial->getDiffuseMapPath() ==
              EditorConfig::absolutizarRuta("Texturas/difuso.png"),
          "sin ninguna coincidencia la ruta queda como estaba");
    CHECK(refModelo->getPath() == existente,
          "una referencia que ya resuelve no se toca");

    // Un segundo cpp.cpp en otra carpeta: ahora el nombre es ambiguo.
    fs::create_directories(raiz / "OtraCarpeta", ec);
    {
        std::ofstream otro((raiz / "OtraCarpeta" / "cpp.cpp").string());
        otro << "// otro\n";
    }
    auto duplicado = std::make_unique<Script>();
    const std::string rotaAmbigua =
        EditorConfig::absolutizarRuta("Scripts/duplicado.cpp");
    duplicado->setDllPath(rotaAmbigua);
    Script* refAmbigua = duplicado.get();
    objeto.addComponent(std::move(duplicado));

    const int conAmbiguedad =
        RutasReescritura::sanarRutasInexistentes(&escena);
    CHECK(conAmbiguedad == 0,
          "con dos coincidencias del mismo nombre no se adivina");
    CHECK(refAmbigua->getPath() == rotaAmbigua,
          "la referencia ambigua queda como estaba");

    // Sin raiz de assets no hay donde buscar.
    EditorConfig::limpiarRaizAssets();
    auto suelto = std::make_unique<Script>();
    const std::string rotaSinRaiz = "Scripts/definitivamente_no_existe.cpp";
    suelto->setDllPath(rotaSinRaiz);
    Script* refSuelto = suelto.get();
    objeto.addComponent(std::move(suelto));

    const int sinRaiz = RutasReescritura::sanarRutasInexistentes(&escena);
    CHECK(sinRaiz == 0,
          "sin raiz de assets no hay donde buscar y no se repara nada");
    CHECK(refSuelto->getPath() == rotaSinRaiz,
          "sin raiz la referencia queda como estaba");
}

// --- El Inspector se desvincula cuando se borra el objeto que muestra ---------
// El Inspector (Settings) guarda el GameObject que muestra y un Settings por
// cada componente, y solo se recarga cuando cambia el PUNTERO. Al borrar, el
// objeto se libera pero el Inspector seguia apuntando a el: si el allocator
// reutiliza el bloque para un objeto nuevo, la comparacion de punteros lo toma
// por el mismo objeto y se dibujan los Settings de componentes ya liberados
// (lectura de memoria liberada: valores absurdos y caida al interactuar).
//
// Por eso el vinculo tiene que caducar en el mismo acto en que el objeto deja
// de estar en la escena, y no cuando vuelve a tocarlo un objeto nuevo.
void elInspectorSeDesvinculaAlBorrar() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    // Entity ya crea un Transform por defecto; lo usamos en lugar de agregar
    // uno duplicado (lo que haria que getComponent<Transform>() devolviera el
    // primero con valores 0,0,0 y el segundo con los valores fijados).
    Transform* transform = objeto->getComponent<Transform>();
    transform->setTranslatef(7.f, -3.f, 11.f);
    GameObject* a = editor.createGameObject(std::move(objeto), raiz);
    CHECK(a != nullptr, "el objeto con Transform entra en la escena");
    if (!a) return;

    SettingsObjectInterface inspector(a, true);
    inspector.setEditor(&editor);
    inspector.setEventBus(&events);
    CHECK(inspector.getObjectInInspector() == a,
          "el inspector muestra el objeto recien creado");

    const GameObject* direccionLiberada = a;
    CHECK(editor.deleteGameObject(a), "el objeto inspeccionado se borra");
    CHECK(inspector.getObjectInInspector() == nullptr,
          "al borrar el objeto inspeccionado el inspector se desvincula");
    CHECK(!registry.contains(inspector.getObjectInInspector()),
          "el inspector no queda apuntando a memoria liberada");

    // Alta de un objeto nuevo. Si el allocator reutiliza el bloque del
    // borrado, una comparacion de punteros lo daria por el mismo objeto: por
    // eso la desvinculacion de arriba tiene que ocurrir en el borrado y no
    // aqui. El dato se imprime porque depende del allocator de cada maquina.
    auto nuevo = GameObjectFactory::createSimpleObject(raiz);
    GameObject* b = editor.createGameObject(std::move(nuevo), raiz);
    CHECK(b != nullptr, "el objeto nuevo entra en la escena");
    std::cout << "  [info] el allocator "
              << (b == direccionLiberada ? "reutilizo" : "no reutilizo")
              << " la direccion del bloque liberado" << std::endl;

    // El GUI revincula el inspector con la seleccion vigente cada frame.
    if (inspector.getObjectInInspector() != b) inspector.setTargetObject(b);
    CHECK(inspector.getObjectInInspector() == b,
          "el inspector muestra el objeto nuevo");

    // Borrar el objeto nuevo tambien lo desvincula (no solo el primero).
    CHECK(editor.deleteGameObject(b), "el objeto nuevo tambien se borra");
    CHECK(inspector.getObjectInInspector() == nullptr,
          "el inspector se desvincula al borrar el objeto nuevo");

    // Y al limpiar la escena entera.
    auto tercero = GameObjectFactory::createSimpleObject(raiz);
    GameObject* c = editor.createGameObject(std::move(tercero), raiz);
    CHECK(c != nullptr, "un tercer objeto entra en la escena");
    inspector.setTargetObject(c);
    CHECK(inspector.getObjectInInspector() == c,
          "el inspector muestra el tercer objeto");
    editor.clearScene();
    CHECK(inspector.getObjectInInspector() == nullptr,
          "limpiar la escena desvincula el inspector");
}

// --- Renombrar -> borrar -> crear -> borrar: el binario no se desalinea -------
// Secuencia reportada en la que el Transform aparece con datos basura. Antes de
// mirar el ciclo de vida del Inspector, hay que descartar la otra hipotesis: que
// el binario quede desalineado y la carga lea basura en los componentes. Se
// reproduce la secuencia completa con guardar y recargar: si el Transform
// vuelve con sus valores, el problema no es del formato del archivo.
void borrarCrearBorrarNoDesalineaElBinario() {
    TempPruebas::CarpetaPrueba carpeta("funshi_secuencia_borrar_crear");
    const fs::path base = carpeta.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    const float tx = 5.f, ty = -2.f, tz = 13.f;

    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);
        GameObject* raiz = registry.getRoot();

        // 1. Un objeto con Transform (testigo: si el binario se desalinea, este
        //    tambien aparece con basura).
        auto testigo = GameObjectFactory::createSimpleObject(raiz);
        // Entity ya crea un Transform por defecto; lo modificamos en lugar de
        // agregar uno duplicado (getComponent devuelve el primero).
        Transform* transformTestigo = testigo->getComponent<Transform>();
        transformTestigo->setTranslatef(tx, ty, tz);
        GameObject* testigoVivo =
            editor.createGameObject(std::move(testigo), raiz);
        CHECK(testigoVivo != nullptr, "el objeto testigo entra en la escena");

        // 2. Otro objeto, renombrado sin confirmar, y borrado enseguida.
        auto doomed = GameObjectFactory::createSimpleObject(raiz);
        Transform* transformDoomed = doomed->getComponent<Transform>();
        transformDoomed->setTranslatef(-40.f, -50.f, -60.f);
        GameObject* paraBorrar = editor.createGameObject(std::move(doomed), raiz);
        CHECK(paraBorrar != nullptr, "el objeto a borrar entra en la escena");
        if (paraBorrar) {
            std::snprintf(paraBorrar->inputName,
                          sizeof(paraBorrar->inputName), "Renombrado");
        }
        CHECK(editor.deleteGameObject(paraBorrar), "el objeto renombrado se borra");

        // 3. Crear otro y borrarlo tambien.
        auto nuevo = GameObjectFactory::createSimpleObject(raiz);
        GameObject* creado = editor.createGameObject(std::move(nuevo), raiz);
        CHECK(creado != nullptr, "tras el borrado se crea otro objeto");
        CHECK(editor.deleteGameObject(creado),
              "el objeto creado despues tambien se borra");

        serializer.save(prefijo);
    }

    // Recargar y verificar que el testigo vuelve con sus valores intactos.
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    serializer.load(pathTxt, semiPath);
    std::cerr.rdbuf(buferAnterior);

    GameObject* raiz = registry.getRoot();
    CHECK(raiz != nullptr, "la escena recargada tiene raiz");
    CHECK(raiz && raiz->getChildEntities().size() == 1,
          "solo sobrevive el objeto testigo (los otros dos se borraron)");

    Transform* recargado = nullptr;
    if (raiz) {
        for (auto* hijo : raiz->getChildEntities()) {
            auto* go = dynamic_cast<GameObject*>(hijo);
            if (go) recargado = go->getComponent<Transform>();
        }
    }
    CHECK(recargado != nullptr,
          "el objeto superviviente conserva su componente Transform");
    if (recargado) {
        const float* t = recargado->getTranslatef();
        CHECK(t[0] == tx && t[1] == ty && t[2] == tz,
              "el Transform recargado conserva sus valores (el binario no se "
              "desalineo con la secuencia borrar-crear-borrar)");
    }
    CHECK(aviso.str().find("Nombre de componente invalido") ==
              std::string::npos,
          "ningun nombre de componente invalido al cargar");
    CHECK(aviso.str().find("linea invalida") == std::string::npos,
          "ninguna linea invalida en el indice");
}

// --- El componente Model se resuelve con la matriz MUNDIAL del objeto ---------
// El camino de render de un GameObject con componente Model armaba un
// Modelos3D temporal y le copiaba el Transform LOCAL. El temporal nacia sin
// padre y con un Transform identidad propio, asi que el modelo se dibujaba en
// el origen y, con jerarquia, en el sitio equivocado. resolverDibujoModelo()
// resuelve la malla y la matriz desde el Transform GLOBAL del propio objeto;
// esta prueba fija esa decision sin pila grafica.
void elModeloSeResuelveConLaMatrizMundial() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();
    CHECK(raiz != nullptr, "la escena tiene raiz para el caso de Model");
    if (!raiz) return;

    // Malla registrada sin loader: putMesh la deja en la cache compartida.
    auto malla = std::make_shared<Mesh>();
    malla->vertices = {vec3(0.f, 0.f, 0.f), vec3(1.f, 0.f, 0.f),
                       vec3(0.f, 1.f, 0.f)};
    malla->indices = {0, 1, 2};
    assets.putMesh("Modelos/cubo.obj", malla);

    // Padre desplazado a (2,0,0).
    auto padre = GameObjectFactory::createSimpleObject(raiz);
    padre->getComponent<Transform>()->setTranslatef(2.f, 0.f, 0.f);
    GameObject* nPadre = editor.createGameObject(std::move(padre), raiz);
    CHECK(nPadre != nullptr, "el objeto padre entra en la escena");
    if (!nPadre) return;

    CHECK(!resolverDibujoModelo(nPadre, assets).valido,
          "sin componente Model no hay dibujo");

    auto modeloPadre = std::make_unique<Model>();
    modeloPadre->setPath("Modelos/cubo.obj");
    nPadre->addComponent(std::move(modeloPadre));

    const DibujoModelo dibujoPadre = resolverDibujoModelo(nPadre, assets);
    CHECK(dibujoPadre.valido, "con Model y malla registrada el dibujo es valido");
    CHECK(dibujoPadre.malla == malla.get(),
          "se dibuja la malla resuelta por el AssetManager");
    CHECK(dibujoPadre.modelo[12] == 2.f && dibujoPadre.modelo[13] == 0.f &&
              dibujoPadre.modelo[14] == 0.f,
          "la matriz del objeto raiz lleva su traslacion (2,0,0)");

    // Hijo local en (10,0,10): el mundo tiene que combinar padre + local.
    auto hijo = GameObjectFactory::createSimpleObject(nPadre);
    hijo->getComponent<Transform>()->setTranslatef(10.f, 0.f, 10.f);
    GameObject* nHijo = editor.createGameObject(std::move(hijo), nPadre);
    CHECK(nHijo != nullptr, "el objeto hijo entra en la escena");
    if (!nHijo) return;

    auto modeloHijo = std::make_unique<Model>();
    modeloHijo->setPath("Modelos/cubo.obj");
    nHijo->addComponent(std::move(modeloHijo));

    const DibujoModelo dibujoHijo = resolverDibujoModelo(nHijo, assets);
    CHECK(dibujoHijo.valido, "el hijo con Model tambien resuelve dibujo");
    CHECK(dibujoHijo.modelo[12] == 12.f && dibujoHijo.modelo[13] == 0.f &&
              dibujoHijo.modelo[14] == 10.f,
          "la matriz del hijo combina el padre (2,0,0) con el local (10,0,10)");

    // Ruta que no carga: no propaga y deja el dibujo invalido, con aviso.
    auto malo = GameObjectFactory::createSimpleObject(raiz);
    auto modeloMalo = std::make_unique<Model>();
    modeloMalo->setPath("Modelos/no_existe.obj");
    malo->addComponent(std::move(modeloMalo));
    GameObject* nMalo = editor.createGameObject(std::move(malo), raiz);
    CHECK(nMalo != nullptr, "el objeto con ruta invalida entra en la escena");
    if (!nMalo) return;

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    DibujoModelo dibujoMalo;
    try {
        dibujoMalo = resolverDibujoModelo(nMalo, assets);
    } catch (...) {
        std::cerr.rdbuf(buferAnterior);
        CHECK(false, "una ruta que no carga no debe propagar excepcion");
    }
    std::cerr.rdbuf(buferAnterior);
    CHECK(!dibujoMalo.valido, "una ruta que no carga deja el dibujo invalido");
    CHECK(aviso.str().find("no se pudo cargar") != std::string::npos,
          "la ruta que no carga queda registrada en el log");
}

int main() {
    nombresPorDefecto();
    roundTripDeEscena();
    guardadoConArbolVacio();
    reporteFalloBinario();
    indiceCorruptoSinFantasmas();
    hijoConIdCeroSeReasignaAlGuardar();
    hermanosConsecutivosSinPerdida();
    rutasDeCarasDeSkybox();
    reescrituraDeReferencias();
    sanadoDeRutasRotas();
    elInspectorSeDesvinculaAlBorrar();
    borrarCrearBorrarNoDesalineaElBinario();
    elModeloSeResuelveConLaMatrizMundial();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
