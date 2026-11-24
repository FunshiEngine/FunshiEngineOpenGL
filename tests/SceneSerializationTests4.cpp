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

    // El archivo de audio roto se repara con la misma regla.
    fs::create_directories(raiz / "Assets" / "Audio", ec);
    {
        std::ofstream clip((raiz / "Assets" / "Audio" / "disparo.wav").string());
        clip << "clip\n";
    }
    const fs::path audioReal = raiz / "Assets" / "Audio" / "disparo.wav";
    auto audio = std::make_unique<AudioSource>();
    audio->setRutaClip(EditorConfig::absolutizarRuta("Audio/disparo.wav"));
    AudioSource* refAudio = audio.get();
    objeto.addComponent(std::move(audio));

    ListaDE<GameObject*> escena;
    escena.addLast(&objeto);

    const int reparadas = RutasReescritura::sanarRutasInexistentes(&escena);
    CHECK(reparadas == 2, "con una coincidencia unica se reparan las dos");
    CHECK(fs::path(refScript->getPath()).generic_string() ==
              real.generic_string(),
          "la fuente del script queda apuntando al archivo que existe");
    CHECK(fs::path(refAudio->getRutaClip()).generic_string() ==
              audioReal.generic_string(),
          "el archivo de audio queda apuntando al archivo que existe");
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

void variosScriptsPorObjeto() {
    GameObject objeto;
    auto primero = std::make_unique<Script>();
    primero->setDllPath("Scripts/UserController.cpp");
    Script* refPrimero = primero.get();
    objeto.addComponent(std::move(primero));

    auto segundo = std::make_unique<Script>();
    segundo->setDllPath("Scripts/CameraController.cpp");
    Script* refSegundo = segundo.get();
    objeto.addComponent(std::move(segundo));

    int cantidadScripts = 0;
    ListaDE<Component*>* componentes = objeto.getComponents();
    Position<Component*>* posicion = componentes->first();
    while (posicion) {
        if (dynamic_cast<Script*>(posicion->getElement()))
            ++cantidadScripts;
        posicion = posicion != componentes->last()
                       ? componentes->next(posicion)
                       : nullptr;
    }

    CHECK(cantidadScripts == 2,
          "un GameObject conserva mas de un componente Script");
    CHECK(refPrimero->getPath() == "Scripts/UserController.cpp",
          "el primer componente Script mantiene su fuente");
    CHECK(refSegundo->getPath() == "Scripts/CameraController.cpp",
          "el segundo componente Script mantiene su fuente");
}

void nombreDeComponenteScriptSeSerializa() {
    TempPruebas::CarpetaPrueba carpeta("funshi_nombre_script");
    const fs::path archivo = carpeta.ruta() / "script.bin";

    Script original;
    original.setDllPath("Scripts/CameraController.cpp");
    original.setNombreComponente("Camara secundaria");
    ReflejoScripts::ValorCampo campoVelocidad;
    campoVelocidad.nombre = "velocidad";
    campoVelocidad.tag = ReflejoScripts::TagTipo::Flotante;
    campoVelocidad.contenido = 3.5f;
    original.obtenerValores().push_back(campoVelocidad);
    {
        std::ofstream salida(archivo, std::ios::binary);
        original.saveComponent(&salida);
    }

    Script recuperado;
    {
        std::ifstream entrada(archivo, std::ios::binary);
        recuperado.loadComponent(&entrada);
    }
    CHECK(recuperado.getNombreComponente() == "Camara secundaria",
          "el nombre personalizado del componente Script sobrevive al guardado");
    CHECK(recuperado.nombreParaMostrar() == "Camara secundaria",
          "el nombre personalizado identifica el panel Script");
    CHECK(recuperado.getNameClass() == "CameraController",
          "la identidad del script sigue siendo el nombre de la clase");
    CHECK(recuperado.obtenerCampos().size() == 1 &&
              recuperado.obtenerCampos()[0].nombre == "velocidad" &&
              recuperado.obtenerCampos()[0].tag ==
                  ReflejoScripts::TagTipo::Flotante,
          "los campos SerializeField siguen visibles tras recargar la escena");
}

void rotacionApiUsaRadianes() {
    GameObject objeto;
    const MotorScript::ApiScriptGameObject* api = MotorScript::tablaApi();
    api->fijarRotacionEjes(&objeto, 1.57079632679f, 0.0f, 1.0f, 0.0f);
    CHECK(std::fabs(objeto.getComponent<Transform>()->getRotatef()[0] -
                    90.0f) < 0.001f,
          "la API convierte radianes a grados al escribir el Transform");
    CHECK(std::fabs(api->rotacionAngulo(&objeto) - 1.57079632679f) < 0.001f,
          "la API devuelve el angulo de rotacion en radianes");
    CHECK(std::fabs(api->rotacionEjeY(&objeto) - 1.0f) < 0.001f,
          "el eje de rotacion conserva su valor");
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

// La ventana Camaras puede borrar el objeto activo durante GUI(). El puntero
// capturado al inicio del frame debe rechazarse antes de pasarlo al gizmo.
void laCamaraBorradaNoSeUsaEnElRestoDelFrame() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();
    CHECK(raiz != nullptr, "la escena tiene raiz para la prueba de camara");
    if (!raiz) return;

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    auto componente = std::make_unique<CameraComponent>();
    CameraComponent* capturada = componente.get();
    objeto->addComponent(std::move(componente));
    GameObject* camara = editor.createGameObject(std::move(objeto), raiz);
    CHECK(camara != nullptr, "la camara entra en la escena");
    if (!camara) return;

    CHECK(CameraFrameSafety::validarCamara(&registry, camara, capturada) ==
              capturada,
          "la camara activa viva puede continuar en el frame");
    CHECK(editor.deleteGameObject(camara),
          "la camara activa se elimina desde la interfaz");
    CHECK(CameraFrameSafety::validarCamara(&registry, camara, capturada) ==
              nullptr,
          "el puntero capturado se invalida antes de usarlo tras GUI");

    auto reemplazoObjeto = GameObjectFactory::createSimpleObject(raiz);
    auto reemplazoComponente = std::make_unique<CameraComponent>();
    CameraComponent* reemplazoCapturado = reemplazoComponente.get();
    reemplazoObjeto->addComponent(std::move(reemplazoComponente));
    GameObject* reemplazo =
        editor.createGameObject(std::move(reemplazoObjeto), raiz);
    CHECK(reemplazo != nullptr, "la camara de reemplazo entra en la escena");
    if (reemplazo) {
        CHECK(CameraFrameSafety::validarCamara(
                  &registry, reemplazo, reemplazoCapturado) ==
                  reemplazoCapturado,
              "la camara de reemplazo queda validada para el gizmo");
    }
}
