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

namespace {

// --- Lineas corruptas en SceneBBDDObjetos.txt ---------------------------------
// Una linea del indice que resuelve a id=0 dentro del bloque de hijos hacia que
// loadPreOrder lea ObjectN0.db (el contenido de la raiz): nace un hijo que se
// llama "Scene" como la raiz y, al guardarse con id propio, se auto-propaga para
// siempre. El fantasma observado por el usuario (4 hijos "Scene" de 181 bytes en
// ObjectN2..N5) era exactamente eso. Tres variantes de linea que hoy reproducen
// el fantasma; tras el fix tienen que saltarse con aviso y sin romper la
// estructura del arbol.
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

std::string rutaConSeparadorFinal(const fs::path& ruta) {
    std::string resultado = ruta.string();
    if (!resultado.empty() && resultado.back() != fs::path::preferred_separator)
        resultado.push_back(fs::path::preferred_separator);
    return resultado;
}

}  // namespace

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

void diagnosticosCargaEscena() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_diagnosticos_escena");
    const fs::path base = carpetaDir.ruta();

    // Una escena nueva se crea vacía sin presentarla como error.
    {
        const fs::path nueva = base / "nueva";
        fs::create_directories(nueva);
        const fs::path indice = nueva / "SceneBBDDObjetos.txt";
        RegistroDiagnosticos diagnosticos(nueva / "diagnosticos.jsonl");
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets, &diagnosticos);

        CHECK(serializer.load(indice.string(), (nueva / "Scene").string()),
              "una escena ausente se inicializa como escena nueva");
        CHECK(fs::is_regular_file(indice),
              "la inicialización crea el índice vacío de la escena");
        CHECK(diagnosticos.ultimos().empty(),
              "crear una escena nueva no genera un diagnóstico de error");
    }

    // Un archivo de índice válido reconstruye la escena sin publicar errores.
    {
        const fs::path proyecto = base / "valida";
        fs::create_directories(proyecto);
        const std::string indice = guardarEscenaVictima(proyecto);
        RegistroDiagnosticos diagnosticos(proyecto / "diagnosticos.jsonl");
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets, &diagnosticos);

        CHECK(serializer.load(indice,
                              rutaConSeparadorFinal(proyecto / "Scene")),
              "una escena válida se carga completamente");
        CHECK(diagnosticos.ultimos().empty(),
              "una carga válida no publica diagnósticos");
    }

    // Una línea malformada se omite y queda registrada como recuperación parcial.
    {
        const fs::path proyecto = base / "corrupta";
        fs::create_directories(proyecto);
        const std::string indice = guardarEscenaVictima(proyecto);
        {
            std::ofstream salida(indice, std::ios::trunc);
            salida << "ObjectN0.db\n=>\nObjectN1.db\nbasura\n<=\n";
        }
        RegistroDiagnosticos diagnosticos(proyecto / "diagnosticos.jsonl");
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets, &diagnosticos);

        CHECK(!serializer.load(indice,
                               rutaConSeparadorFinal(proyecto / "Scene")),
              "una línea corrupta indica que la carga fue parcial");
        const auto eventos = diagnosticos.ultimos(10);
        CHECK(eventos.size() == 1,
              "una línea corrupta genera un único diagnóstico");
        CHECK(eventos.size() == 1 &&
                  eventos[0].modulo == "Scenes/SceneSerializer" &&
                  eventos[0].tipoExcepcion == "SCENE_INDEX_INVALID" &&
                  eventos[0].nivel == NivelDiagnostico::Advertencia &&
                  eventos[0].mensaje.find("basura") != std::string::npos,
              "el diagnóstico identifica el índice, línea y nivel de advertencia");
    }

    // El índice referencia un binario que desapareció: se conserva la carga
    // parcial y se publica el fallo del objeto que no pudo reconstruirse.
    {
        const fs::path proyecto = base / "objeto_ausente";
        fs::create_directories(proyecto);
        const std::string indice = guardarEscenaVictima(proyecto);
        std::error_code error;
        fs::remove(proyecto / "Scene" / "ObjectN1.db", error);
        CHECK(!error, "se elimina el binario del objeto para simular el fallo");

        RegistroDiagnosticos diagnosticos(proyecto / "diagnosticos.jsonl");
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets, &diagnosticos);

        CHECK(!serializer.load(indice,
                               rutaConSeparadorFinal(proyecto / "Scene")),
              "un binario de objeto ausente hace que la carga sea parcial");
        const auto eventos = diagnosticos.ultimos(10);
        CHECK(eventos.size() == 1 &&
                  eventos[0].tipoExcepcion == "SCENE_OBJECT_LOAD_FAILED" &&
                  eventos[0].nivel == NivelDiagnostico::Error &&
                  eventos[0].mensaje.find("id=1") != std::string::npos,
              "el binario ausente publica un error único asociado al objeto");
    }
}

void diagnosticosCargaMalla() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_diagnosticos_malla");
    RegistroDiagnosticos diagnosticos(carpetaDir.ruta() / "diagnosticos.jsonl");
    auto loader = std::make_unique<LoaderDiagnostico>();
    LoaderDiagnostico* loaderRaw = loader.get();
    AssetManager assets(std::move(loader));
    GameObject modeloObj;
    Model* modelo = new Model();
    modeloObj.addComponent(modelo);
    modelo->setAssetManager(&assets);
    modelo->setRegistroDiagnosticos(&diagnosticos);

    loaderRaw->falla = false;
    modelo->setPath("Assets/valido.obj");
    CHECK(modelo->getMesh() != nullptr && diagnosticos.ultimos().empty(),
          "cargar una malla válida no publica un diagnóstico");
    CHECK(loaderRaw->cargas == 1, "el loader recibe la carga válida");

    loaderRaw->falla = true;
    modelo->setPath("Assets/modelo.obj");
    CHECK(modelo->getMesh() == nullptr && assets.hasFailedMesh("Assets/modelo.obj"),
          "el fallo del loader deja el modelo vacío y cacheado");
    CHECK(loaderRaw->cargas == 2, "el loader intenta cargar el modelo fallido");

    modelo->setPath("Assets/modelo.obj");
    CHECK(loaderRaw->cargas == 2,
          "la repetición del fallo usa el error cacheado");
    auto eventos = diagnosticos.ultimos(10);
    CHECK(eventos.size() == 1 &&
              eventos[0].tipoExcepcion == "AssetLoadException" &&
              eventos[0].modulo == "Objetos/Model" &&
              eventos[0].nivel == NivelDiagnostico::Error,
          "RuntimeException de assets llega una sola vez al registro común");

    loaderRaw->falla = false;
    assets.clearFailedMeshes();
    modelo->setPath("Assets/modelo.obj");
    CHECK(modelo->getMesh() != nullptr && loaderRaw->cargas == 3,
          "limpiar el fallo permite que el modelo se recupere");
    CHECK(diagnosticos.ultimos(10).size() == 1,
          "la recuperación exitosa no duplica el diagnóstico histórico");
}

void diagnosticosCargaScript() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_diagnosticos_script");
    RegistroDiagnosticos diagnosticos(carpetaDir.ruta() / "diagnosticos.jsonl");
    Script script;
    script.setRegistroDiagnosticos(&diagnosticos);
    const fs::path fuente = carpetaDir.ruta() / "Scripts con espacios" /
                            "Controlador Ausente.cpp";
    script.setDllPath(fuente.string());

    CHECK(!script.aplicarCarga(nullptr),
          "un fuente de script ausente no se carga");
    CHECK(!script.ultimoError().empty(),
          "el componente conserva el error devuelto por el backend");
    auto eventos = diagnosticos.ultimos(10);
    CHECK(eventos.size() == 1 &&
              eventos[0].tipoExcepcion == "SCRIPT_LOAD_FAILED" &&
              eventos[0].mensaje.find(fuente.string()) != std::string::npos,
          "el fallo de carga se publica una vez con la ruta del fuente");

    CHECK(!script.aplicarCarga(nullptr),
          "una carga ya intentada conserva el estado de fallo");
    CHECK(diagnosticos.ultimos(10).size() == 1,
          "consultar de nuevo el fallo no duplica el diagnóstico");

    script.setDllPath(fuente.string());
    CHECK(!script.aplicarCarga(nullptr),
          "reasignar el fuente permite un nuevo intento de carga");
    CHECK(diagnosticos.ultimos(10).size() == 2,
          "un nuevo intento fallido genera un evento nuevo");
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
