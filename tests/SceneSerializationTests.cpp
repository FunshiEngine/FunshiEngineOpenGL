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
#include <iterator>
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

// --- Reporte de fallos en Binario (H-9) ---------------------------------------
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

// --- H-17: lineas corruptas en SceneBBDDObjetos.txt ----------------------------
// Una linea del indice que resuelve a id=0 dentro del bloque de hijos hacia que
// loadPreOrder lea ObjectN0.db (el contenido de la raiz): nace un hijo que se
// llama "Scene" como la raiz y, al guardarse con id propio, se auto-propaga para
// siempre. El fantasma observado por el usuario (4 hijos "Scene" de 181 bytes en
// ObjectN2..N5) era exactamente eso. Tres variantes de linea que hoy reproducen
// el fantasma; tras el fix tienen que saltarse con aviso y sin romper la
// estructura del arbol.
//
// Ver PLAN GENERAL DE FIX.md §20 (H-17).
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

// --- H-17 (variante de guardado): hijo con id=0 --------------------------------
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

// --- H-17 (causa raíz): el look-ahead pierde la posición con ≥2 hermanos -----
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

int main() {
    nombresPorDefecto();
    roundTripDeEscena();
    guardadoConArbolVacio();
    reporteFalloBinario();
    indiceCorruptoSinFantasmas();
    hijoConIdCeroSeReasignaAlGuardar();
    hermanosConsecutivosSinPerdida();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
