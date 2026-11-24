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

// --- La pose reparentada sobrevive el guardado -------------------------------
// El local reescrito al reparentar debe quedar consistente en disco: al recargar,
// el objeto vuelve a su misma pose mundial y cuelga de la raiz.
void reparentarSobreviveElGuardado() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_escena_reparent");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);
    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);
        GameObject* raiz = registry.getRoot();

        auto padreObj = GameObjectFactory::createSimpleObject(raiz);
        GameObject* padre = editor.createGameObject(std::move(padreObj), raiz);
        auto hijoObj = GameObjectFactory::createSimpleObject(padre);
        std::snprintf(hijoObj->inputName, sizeof(hijoObj->inputName),
                      "Reubicado");
        GameObject* hijo = editor.createGameObject(std::move(hijoObj), padre);
        CHECK(padre && hijo, "se arma la escena a guardar");
        if (!padre || !hijo) return;

        padre->getComponent<Transform>()->setTranslatef(5, 0, 5);
        hijo->getComponent<Transform>()->setTranslatef(5, 0, 5);
        CHECK(editor.reparentGameObject(hijo, raiz),
              "el hijo se reparenta antes de guardar");
        serializer.save(prefijo);
    }

    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);
        serializer.load(pathTxt, semiPath);

        GameObject* raiz = registry.getRoot();
        GameObject* reubicado = nullptr;
        if (raiz) {
            for (auto* e : raiz->getChildEntities()) {
                auto* go = dynamic_cast<GameObject*>(e);
                if (go && std::string(go->inputName) == "Reubicado") {
                    reubicado = go;
                    break;
                }
            }
        }
        CHECK(reubicado != nullptr,
              "el objeto reparentado se recarga bajo la raiz");
        if (reubicado) {
            float* t = reubicado->getGlobalTransform()->getTranslatef();
            CHECK(std::abs(t[0] - 10) < 1e-3f && std::abs(t[2] - 10) < 1e-3f,
                  "la pose reparentada sobrevive el guardado");
        }
    }
}

void opcionesDeRenderYFisicaSePersisten() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_opciones_componentes");
    const fs::path archivoCollider = carpetaDir.ruta() / "collider.bin";
    const fs::path archivoRigidBody = carpetaDir.ruta() / "rigidbody.bin";

    Transform transformCollider;
    EsfereCollider collider(2.0f, &transformCollider);
    collider.setVisibleEnEscena(true);
    {
        std::ofstream salida(archivoCollider, std::ios::binary);
        collider.saveComponent(&salida);
    }
    Transform transformColliderRecuperado;
    EsfereCollider colliderRecuperado(1.0f, &transformColliderRecuperado);
    {
        std::ifstream entrada(archivoCollider, std::ios::binary);
        colliderRecuperado.loadComponent(&entrada);
    }
    CHECK(colliderRecuperado.estaVisibleEnEscena(),
          "la visibilidad del collider se conserva al serializar");
    CHECK(std::abs(colliderRecuperado.getRadio() - 2.0f) < 0.001f,
          "el formato nuevo conserva tambien el radio del collider");

    const fs::path archivoColliderLegacy =
        carpetaDir.ruta() / "collider-legacy.bin";
    Transform transformLegacy;
    EsfereCollider colliderLegacy(3.0f, &transformLegacy);
    {
        std::ofstream salida(archivoColliderLegacy, std::ios::binary);
        const float radio = colliderLegacy.getRadio();
        salida.write(reinterpret_cast<const char*>(&radio), sizeof(radio));
        transformLegacy.saveComponent(&salida);
        colliderLegacy.getTransform()->saveComponent(&salida);
    }
    Transform transformLegacyRecuperado;
    EsfereCollider colliderLegacyRecuperado(1.0f,
                                             &transformLegacyRecuperado);
    {
        std::ifstream entrada(archivoColliderLegacy, std::ios::binary);
        colliderLegacyRecuperado.loadComponent(&entrada);
    }
    CHECK(!colliderLegacyRecuperado.estaVisibleEnEscena(),
          "un collider de escena anterior carga oculto");
    CHECK(std::abs(colliderLegacyRecuperado.getRadio() - 3.0f) < 0.001f,
          "un collider de escena anterior conserva el radio");

    Transform transformFisica;
    EsfereCollider colliderFisico(1.0f, &transformFisica);
    RigidBody rigidBody(&colliderFisico, 1.0f);
    rigidBody.setActivo(false);
    {
        std::ofstream salida(archivoRigidBody, std::ios::binary);
        rigidBody.saveComponent(&salida);
    }
    Transform transformFisicaRecuperada;
    EsfereCollider colliderFisicoRecuperado(1.0f,
                                             &transformFisicaRecuperada);
    RigidBody rigidBodyRecuperado(&colliderFisicoRecuperado, 1.0f);
    {
        std::ifstream entrada(archivoRigidBody, std::ios::binary);
        rigidBodyRecuperado.loadComponent(&entrada);
    }
    CHECK(!rigidBodyRecuperado.estaActivo(),
          "el estado inactivo del RigidBody se conserva al serializar");
    CHECK(rigidBodyRecuperado.getRigidBody() &&
              rigidBodyRecuperado.getRigidBody()->getActivationState() ==
                  DISABLE_SIMULATION,
          "un RigidBody inactivo no participa en la simulacion de Bullet");
    BulletPhysicsAdapter fisica;
    fisica.addRigidBody(&rigidBodyRecuperado);
    CHECK(rigidBodyRecuperado.getRigidBody()->getActivationState() ==
              DISABLE_SIMULATION,
          "el registro en Bullet conserva el estado inactivo");
    rigidBodyRecuperado.setActivo(true);
    CHECK(rigidBodyRecuperado.getRigidBody()->getActivationState() !=
              DISABLE_SIMULATION,
          "activar el RigidBody lo incorpora a la simulacion");
    rigidBodyRecuperado.setActivo(false);
    CHECK(rigidBodyRecuperado.getRigidBody()->getActivationState() ==
              DISABLE_SIMULATION,
          "desactivar el RigidBody registrado pausa la simulacion");
    fisica.removeRigidBody(&rigidBodyRecuperado);

    const fs::path archivoRigidBodyLegacy =
        carpetaDir.ruta() / "rigidbody-legacy.bin";
    {
        std::ofstream salida(archivoRigidBodyLegacy, std::ios::binary);
        const float masa = 1.0f;
        const float posicion[3] = {0.0f, 0.0f, 0.0f};
        const float rotacion[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        salida.write(reinterpret_cast<const char*>(&masa), sizeof(masa));
        salida.write(reinterpret_cast<const char*>(posicion),
                     sizeof(posicion));
        salida.write(reinterpret_cast<const char*>(rotacion),
                     sizeof(rotacion));
    }
    Transform transformRigidBodyLegacy;
    EsfereCollider colliderRigidBodyLegacy(1.0f, &transformRigidBodyLegacy);
    RigidBody rigidBodyLegacy(&colliderRigidBodyLegacy, 1.0f);
    {
        std::ifstream entrada(archivoRigidBodyLegacy, std::ios::binary);
        rigidBodyLegacy.loadComponent(&entrada);
    }
    CHECK(rigidBodyLegacy.estaActivo(),
          "un RigidBody de escena anterior carga activo");

    CameraComponent camara;
    camara.setNearPlane(0.1f);
    camara.setFarPlane(1000.0f);
    float proyeccion[16];
    camara.getProjectionMatrix(proyeccion, 1.0f, 150.0f);
    const float m10Para150 = -(150.0f + 0.1f) / (150.0f - 0.1f);
    const float m14Para150 = -(2.0f * 150.0f * 0.1f) / (150.0f - 0.1f);
    CHECK(std::abs(proyeccion[10] - m10Para150) < 1e-6f &&
              std::abs(proyeccion[14] - m14Para150) < 1e-6f,
          "la proyeccion limita el plano lejano al horizonte solicitado");
    camara.getProjectionMatrix(proyeccion, 1.0f);
    const float m10Para1000 = -(1000.0f + 0.1f) / (1000.0f - 0.1f);
    const float m14Para1000 =
        -(2.0f * 1000.0f * 0.1f) / (1000.0f - 0.1f);
    CHECK(std::abs(proyeccion[10] - m10Para1000) < 1e-6f &&
              std::abs(proyeccion[14] - m14Para1000) < 1e-6f,
          "la proyeccion ordinaria conserva el far plane configurable");
}

void movimientoFisicoYSaltoDesdeScripts() {
    GameObject jugador;
    Transform* transform = jugador.getComponent<Transform>();
    CHECK(transform != nullptr, "el jugador tiene Transform");
    if (!transform) return;

    auto collider = std::make_unique<EsfereCollider>(
        1.0f, transform, &jugador);
    EsfereCollider* colliderPtr = collider.get();
    jugador.addComponent(std::move(collider));
    auto rigidBody = std::make_unique<RigidBody>(colliderPtr, 1.0f);
    RigidBody* rigidBodyPtr = rigidBody.get();
    jugador.addComponent(std::move(rigidBody));

    BulletPhysicsAdapter fisica;
    fisica.addRigidBody(rigidBodyPtr);
    for (int i = 0; i < 20; ++i) fisica.stepSimulation(1.0f / 60.0f);

    const MotorScript::ApiScriptGameObject* api = MotorScript::tablaApi();
    CHECK(api->version >= 3,
          "la API de scripts publica movimiento y salto con fisica");
    CHECK(api->fijarVelocidadHorizontal(&jugador, 2.0f, -3.0f),
          "la API detecta y mueve el RigidBody del GameObject");
    CHECK(api->saltar(&jugador, 5.0f),
          "un RigidBody apoyado acepta el salto");
    btVector3 velocidad = rigidBodyPtr->getRigidBody()->getLinearVelocity();
    CHECK(std::abs(velocidad.x() - 2.0f) < 0.001f &&
              std::abs(velocidad.y() - 5.0f) < 0.001f &&
              std::abs(velocidad.z() + 3.0f) < 0.001f,
          "el movimiento horizontal conserva la velocidad vertical del salto");
    CHECK(!api->saltar(&jugador, 5.0f),
          "el cuerpo no acepta un segundo salto en el aire");
    fisica.removeRigidBody(rigidBodyPtr);
}

void eventosDeContactoExponenColliderYPropietario() {
    GameObject jugador;
    jugador.setTag("jugador");
    GameObject suelo;
    suelo.setTag("suelo");
    Transform* transformJugador = jugador.getComponent<Transform>();
    Transform* transformSuelo = suelo.getComponent<Transform>();
    CHECK(transformJugador && transformSuelo,
          "los objetos de contacto tienen transform");
    if (!transformJugador || !transformSuelo) return;
    transformSuelo->setTranslatef(0.0f, -1.0f, 0.0f);

    auto colliderJugador = std::make_unique<EsfereCollider>(
        1.0f, transformJugador, &jugador);
    Collider* colliderJugadorPtr = colliderJugador.get();
    jugador.addComponent(std::move(colliderJugador));
    auto cuerpoJugador =
        std::make_unique<RigidBody>(colliderJugadorPtr, 1.0f);
    RigidBody* cuerpoJugadorPtr = cuerpoJugador.get();
    jugador.addComponent(std::move(cuerpoJugador));

    auto colliderSuelo = std::make_unique<EsfereCollider>(
        1.0f, transformSuelo, &suelo);
    Collider* colliderSueloPtr = colliderSuelo.get();
    suelo.addComponent(std::move(colliderSuelo));
    auto cuerpoSuelo = std::make_unique<RigidBody>(colliderSueloPtr, 0.0f);
    RigidBody* cuerpoSueloPtr = cuerpoSuelo.get();
    suelo.addComponent(std::move(cuerpoSuelo));

    BulletPhysicsAdapter fisica;
    fisica.addRigidBody(cuerpoJugadorPtr);
    fisica.addRigidBody(cuerpoSueloPtr);
    const auto* api = MotorScript::tablaApi();
    CHECK(api->version >= 4,
          "la API de scripts publica consultas de tags y colliders");
    CHECK(api->tieneEtiqueta(&suelo, "suelo") &&
              std::string(api->etiqueta(&suelo)) == "suelo",
          "la API consulta el tag registrado en el propietario");
    CHECK(api->objetoDeCollider(colliderSueloPtr) == &suelo,
          "la API resuelve el objeto propietario desde su collider");

    fisica.stepSimulation(1.0f / 60.0f);
    std::vector<EventoContacto> eventos = fisica.tomarEventosContacto();
    auto contieneTipo = [](const std::vector<EventoContacto>& lista,
                           TipoContacto tipo, Collider* a, Collider* b) {
        for (const EventoContacto& evento : lista) {
            if (evento.tipo == tipo &&
                ((evento.colliderA == a && evento.colliderB == b) ||
                 (evento.colliderA == b && evento.colliderB == a)))
                return true;
        }
        return false;
    };
    CHECK(contieneTipo(eventos, TipoContacto::Inicio, colliderJugadorPtr,
                       colliderSueloPtr),
          "el primer contacto produce un evento de inicio");

    fisica.stepSimulation(1.0f / 60.0f);
    eventos = fisica.tomarEventosContacto();
    CHECK(contieneTipo(eventos, TipoContacto::Persistencia,
                       colliderJugadorPtr, colliderSueloPtr),
          "el contacto continuo produce un evento de persistencia");
    CHECK(colliderJugadorPtr->cantidadContactos() > 0 &&
              colliderJugadorPtr->contactoEnIndice(0) == colliderSueloPtr,
          "el collider expone los colliders en contacto");

    btTransform separado;
    separado.setIdentity();
    separado.setOrigin(btVector3(100.0f, 10.0f, 0.0f));
    cuerpoJugadorPtr->getRigidBody()->setWorldTransform(separado);
    cuerpoJugadorPtr->getRigidBody()->getMotionState()->setWorldTransform(
        separado);
    cuerpoJugadorPtr->getRigidBody()->activate();
    fisica.stepSimulation(1.0f / 60.0f);
    eventos = fisica.tomarEventosContacto();
    CHECK(contieneTipo(eventos, TipoContacto::Fin, colliderJugadorPtr,
                       colliderSueloPtr),
          "al separarse los colliders se produce un evento de fin");

    fisica.removeRigidBody(cuerpoJugadorPtr);
    fisica.removeRigidBody(cuerpoSueloPtr);
}

void busquedaDeObjetosPorNombreIdYEtiqueta() {
    SceneRegistry registro;
    auto camara = std::make_unique<GameObject>();
    std::snprintf(camara->inputName, sizeof(camara->inputName), "%s",
                  "Camara");
    camara->setId(7);
    camara->setTag("principal");
    auto jugador = std::make_unique<GameObject>();
    std::snprintf(jugador->inputName, sizeof(jugador->inputName), "%s",
                  "Jugador");
    jugador->setTag("jugador");
    GameObject* camaraPtr = registro.createObject(std::move(camara));
    GameObject* jugadorPtr = registro.createObject(std::move(jugador));
    CHECK(camaraPtr && jugadorPtr,
          "el registro crea los objetos de busqueda");
    if (!camaraPtr || !jugadorPtr) return;

    MotorScript::inyectarServiciosScript(nullptr, &registro, nullptr);
    const MotorScript::ScriptServices* servicios =
        MotorScript::tablaServicios();
    CHECK(servicios->version >= 3,
          "los servicios publican busqueda por nombre, id y etiqueta");
    CHECK(servicios->objetoPorNombre("Camara") == camaraPtr,
          "la busqueda por nombre devuelve la camara");
    CHECK(servicios->objetoPorId(7) == camaraPtr,
          "la busqueda por id devuelve la camara");
    CHECK(servicios->objetoPorId(jugadorPtr->getId()) == jugadorPtr,
          "la busqueda por id asignado devuelve al jugador");
    CHECK(servicios->objetoPorEtiqueta("principal") == camaraPtr,
          "la busqueda por etiqueta devuelve la camara");
    CHECK(servicios->objetoPorEtiqueta("jugador") == jugadorPtr,
          "la busqueda por etiqueta devuelve al jugador");
    CHECK(servicios->objetoPorNombre("Inexistente") == nullptr,
          "el nombre desconocido no resuelve objeto");
    CHECK(servicios->objetoPorId(9999) == nullptr,
          "el id desconocido no resuelve objeto");
    CHECK(servicios->objetoPorEtiqueta("inexistente") == nullptr,
          "la etiqueta desconocida no resuelve objeto");
    MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
    CHECK(servicios->objetoPorNombre("Camara") == nullptr &&
              servicios->objetoPorId(7) == nullptr &&
              servicios->objetoPorEtiqueta("principal") == nullptr,
          "sin escena inyectada no hay busqueda");
}
