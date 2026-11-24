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

// --- Un cambio de propiedad no reconstruye el inspector -----------------------
// El bus publicaba el mismo evento para "cambio de propiedad" (renombrar,
// editar un campo) y para "alta/baja de componente". El inspector reaccionaba a
// ambos borrando y recreando todos los Settings, con lo que se perdia el estado
// local de cada panel (radio sin confirmar, preset de material, header abierto)
// y el puntero de identidad cambiaba en cada edicion. Ahora solo el cambio
// estructural reconcilia, y ademas reutiliza los paneles vigentes.
void elEventoDePropiedadNoReconstruyeElInspector() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    GameObject* a = editor.createGameObject(std::move(objeto), raiz);
    CHECK(a != nullptr, "el objeto para el inspector entra en la escena");
    if (!a) return;

    SettingsObjectInterface inspector(a, true);
    inspector.setEditor(&editor);
    inspector.setEventBus(&events);

    CHECK(inspector.reconstruccionesSettings() == 0,
          "la carga inicial no cuenta como reconciliacion");
    SettingsComponent* transformAntes = inspector.settingsEnIndice(0);
    CHECK(transformAntes != nullptr,
          "el inspector lista el Transform del objeto");

    // Cambio de PROPIEDAD: no debe reconstruir ni crear paneles nuevos.
    const size_t antes = inspector.reconstruccionesSettings();
    const size_t creadosAntes = inspector.settingsCreados();
    events.publish({SceneEventType::ComponentChanged, a, nullptr});
    CHECK(inspector.reconstruccionesSettings() == antes,
          "un cambio de propiedad no reconcilia el inspector");
    CHECK(inspector.settingsCreados() == creadosAntes,
          "un cambio de propiedad no crea paneles nuevos");
    CHECK(inspector.settingsEnIndice(0) == transformAntes,
          "el panel del Transform sobrevive a un cambio de propiedad");

    // Cambio ESTRUCTURAL: si reconcilia y conserva el panel vigente.
    CHECK(editor.addComponent(a, std::make_unique<Color>()),
          "se agrega un Color al objeto");
    CHECK(inspector.reconstruccionesSettings() == antes + 1,
          "agregar un componente dispara exactamente una reconciliacion");
    CHECK(inspector.settingsCreados() == creadosAntes + 1,
          "agregar un componente crea exactamente un panel");
    CHECK(inspector.settingsEnIndice(0) == transformAntes,
          "el panel del Transform sobrevive a agregar otro componente");
    CHECK(inspector.settingsEnIndice(1) != nullptr,
          "el componente nuevo tiene su panel en la lista");
}

// --- El inspector dibuja un frame completo sin arrastre activo ---------------
// contentGUI() recorre los paneles con un PushID por panel y cierra el recorrido
// con el PopID y el avance de la lista. Si ese cierre quedara dentro de un
// `if (ImGui::BeginDragDropTarget())` (que solo es verdadero mientras hay un
// arrastre encima del encabezado), entonces en el caso normal -sin arrastre-
// el recorrido no avanza nunca: el inspector gira al 100% de CPU con la ventana
// congelada al seleccionar un objeto, y el ID de ImGui crece un nivel por
// iteracion.
//
// La prueba arma un frame headless (contexto ImGui sin backend, sin ventana) y
// exige que printGUI() vuelva. Si el bucle no termina, la prueba se cuelga y
// CTest la da por fallida por timeout.
void elInspectorDibujaUnFrameSinArrastre() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto objeto = GameObjectFactory::createSimpleObject(raiz);
    GameObject* a = editor.createGameObject(std::move(objeto), raiz);
    CHECK(a != nullptr, "el objeto del frame del inspector entra en la escena");
    if (!a) return;

    SettingsObjectInterface inspector(a, true);
    inspector.setEditor(&editor);
    inspector.setEventBus(&events);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // Mismo contrato que el backend OpenGL del motor: el atlas de fuentes lo
    // construye ImGui en NewFrame() (sin backend, TexIsBuilt arranca en falso y
    // ImFontAtlasUpdateNewFrame() desreferencia un Builder nulo).
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures;
    io.DisplaySize = ImVec2(1280.0f, 720.0f);
    io.DeltaTime = 1.0f / 60.0f;
    // No dejar imgui.ini en el directorio de trabajo de la prueba.
    io.IniFilename = nullptr;

    ImGui::NewFrame();
    inspector.printGUI();
    ImGui::EndFrame();

    CHECK(inspector.settingsEnIndice(0) != nullptr,
          "el frame termina y el panel del Transform sigue en el inspector");

    ImGui::DestroyContext();
}

void reparentarPreservaLaPoseYRefrescaElCuerpo() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    auto backend = std::make_unique<BackendFisicoFalso>();
    BackendFisicoFalso* espia = backend.get();
    PhysicsEngine physics(std::move(backend));
    EditorController editor(&registry, &physics, &events, &assets);
    GameObject* raiz = registry.getRoot();

    auto padreObj = GameObjectFactory::createSimpleObject(raiz);
    GameObject* padre = editor.createGameObject(std::move(padreObj), raiz);
    auto hijoObj = GameObjectFactory::createSimpleObject(padre);
    GameObject* hijo = editor.createGameObject(std::move(hijoObj), padre);
    auto nietoObj = GameObjectFactory::createSimpleObject(hijo);
    GameObject* nieto = editor.createGameObject(std::move(nietoObj), hijo);
    CHECK(padre && hijo && nieto, "se arma la jerarquia padre/hijo/nieto");
    if (!padre || !hijo || !nieto) return;

    padre->getComponent<Transform>()->setTranslatef(5, 0, 5);
    hijo->getComponent<Transform>()->setTranslatef(5, 0, 5);
    nieto->getComponent<Transform>()->setTranslatef(1, 0, 0);

    auto aprox = [](float a, float b) { return std::abs(a - b) < 1e-4f; };
    float* mundoHijoAntes = hijo->getGlobalTransform()->getTranslatef();
    CHECK(aprox(mundoHijoAntes[0], 10) && aprox(mundoHijoAntes[2], 10),
          "el hijo parte en (10,0,10)");

    EsfereCollider* col =
        new EsfereCollider(1.0f, hijo->getComponent<Transform>(), hijo);
    CHECK(editor.addComponent(hijo, std::unique_ptr<Component>(col)),
          "se agrega el collider del hijo");
    CHECK(editor.addComponent(hijo, std::make_unique<RigidBody>(col, 1.0f)),
          "se agrega el RigidBody del hijo");
    CHECK(espia->agregados == 1, "el cuerpo entra una vez al mundo");

    const int agregadosAntes = espia->agregados;
    const int quitadosAntes = espia->quitados;

    // Predicado del menu: un objeto anidado si es candidato a desanidar; un
    // hijo directo de la raiz ya esta al nivel superior.
    CHECK(esCandidatoADesanidar(hijo, raiz),
          "el hijo anidado es candidato a desanidar");
    CHECK(!esCandidatoADesanidar(padre, raiz),
          "un hijo directo de la raiz no es candidato");
    CHECK(!esCandidatoADesanidar(raiz, raiz), "la raiz no es candidata");

    CHECK(editor.reparentGameObject(hijo, raiz),
          "se reparenta el hijo a la raiz");
    CHECK(hijo->getParentEntity() == raiz, "el hijo cuelga de la raiz");

    float* mundoHijoDespues = hijo->getGlobalTransform()->getTranslatef();
    CHECK(aprox(mundoHijoDespues[0], 10) && aprox(mundoHijoDespues[2], 10),
          "la pose mundial del hijo se conserva al reparentar");
    float* localHijo = hijo->getComponent<Transform>()->getTranslatef();
    CHECK(aprox(localHijo[0], 10) && aprox(localHijo[2], 10),
          "el local del hijo pasa a inverse(padre)*mundo");

    float* mundoNieto = nieto->getGlobalTransform()->getTranslatef();
    CHECK(aprox(mundoNieto[0], 11) && aprox(mundoNieto[2], 10),
          "el descendiente conserva su pose mundial");

    CHECK(espia->quitados == quitadosAntes + 1,
          "reparentar saca el cuerpo viejo del mundo");
    CHECK(espia->agregados == agregadosAntes + 1,
          "reparentar reconstruye y reinserta el cuerpo");
    CHECK(espia->ultimo && espia->ultimo->getRigidBody() != nullptr,
          "el cuerpo reconstruido esta vivo");

    CHECK(!esCandidatoADesanidar(hijo, raiz),
          "tras desanidar, el hijo ya no es candidato");
}
