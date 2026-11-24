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

void propiedadesFisicasDesdeScripts() {
    GameObject jugador;
    Transform* transform = jugador.getComponent<Transform>();
    CHECK(transform != nullptr, "el objeto fisico tiene Transform");
    if (!transform) return;
    auto collider = std::make_unique<EsfereCollider>(1.0f, transform, &jugador);
    EsfereCollider* colliderPtr = collider.get();
    jugador.addComponent(std::move(collider));
    auto cuerpo = std::make_unique<RigidBody>(colliderPtr, 1.0f);
    RigidBody* cuerpoPtr = cuerpo.get();
    jugador.addComponent(std::move(cuerpo));

    BulletPhysicsAdapter fisica;
    fisica.addRigidBody(cuerpoPtr);

    const MotorScript::ApiScriptGameObject* api = MotorScript::tablaApi();
    CHECK(api->version >= 5,
          "la API de scripts publica masa, gravedad, friccion y freeze");
    CHECK(api->masa(&jugador) == 1.0f, "la API lee la masa inicial");
    CHECK(api->fijarMasa(&jugador, 5.0f) && api->masa(&jugador) == 5.0f,
          "la API fija la masa del cuerpo");
    CHECK(std::abs(cuerpoPtr->getRigidBody()->getMass() - 5.0f) < 0.001f,
          "la masa llega a la inercia de Bullet");
    CHECK(api->fijarMasa(&jugador, 0.0f) && api->masa(&jugador) == 0.0f,
          "la masa cero deja el cuerpo estatico");
    api->fijarMasa(&jugador, 1.0f);

    CHECK(api->usaGravedad(&jugador),
          "el cuerpo usa la gravedad del mundo por defecto");
    CHECK(api->fijarEscalaGravedad(&jugador, 2.0f) &&
              api->escalaGravedad(&jugador) == 2.0f,
          "la API fija la escala de gravedad por cuerpo");
    CHECK(!api->fijarEscalaGravedad(&jugador,
                                    std::numeric_limits<float>::quiet_NaN()),
          "la escala no finita se rechaza");
    fisica.fijarGravedad(0.0f, -10.0f, 0.0f);
    const btVector3 gravedad =
        cuerpoPtr->getRigidBody()->getGravity();
    CHECK(std::abs(gravedad.y() + 20.0f) < 0.001f,
          "la gravedad del mundo por la escala llega al cuerpo");
    CHECK(api->fijarUsoGravedad(&jugador, false) &&
              !api->usaGravedad(&jugador),
          "la API desactiva la gravedad por cuerpo");
    const btVector3 sinGravedad =
        cuerpoPtr->getRigidBody()->getGravity();
    CHECK(sinGravedad.length() < 0.001f,
          "sin gravedad el cuerpo no acelera");
    api->fijarUsoGravedad(&jugador, true);

    CHECK(std::abs(api->friccion(&jugador) - 0.5f) < 0.001f,
          "la friccion por defecto es la historica de Bullet");
    CHECK(api->fijarFriccion(&jugador, 0.1f) &&
              std::abs(api->friccion(&jugador) - 0.1f) < 0.001f,
          "la API fija la friccion del cuerpo");
    CHECK(std::abs(cuerpoPtr->getRigidBody()->getFriction() - 0.1f) < 0.001f,
          "la friccion llega al contacto de Bullet");
    CHECK(!api->fijarFriccion(&jugador, -1.0f) ||
              api->friccion(&jugador) >= 0.0f,
          "la friccion negativa no entra al cuerpo");

    CHECK(api->fijarFreezePosicion(&jugador, true, false, true) &&
              api->posicionCongelada(&jugador, 0) &&
              !api->posicionCongelada(&jugador, 1) &&
              api->posicionCongelada(&jugador, 2),
          "la API congela la posicion por ejes");
    const btVector3 factorLineal =
        cuerpoPtr->getRigidBody()->getLinearFactor();
    CHECK(factorLineal.x() == 0.0f && factorLineal.y() == 1.0f &&
              factorLineal.z() == 0.0f,
          "el freeze de posicion llega al linearFactor de Bullet");
    CHECK(api->fijarFreezeRotacion(&jugador, false, true, false) &&
              !api->rotacionCongelada(&jugador, 0) &&
              api->rotacionCongelada(&jugador, 1) &&
              !api->rotacionCongelada(&jugador, 2),
          "la API congela la rotacion por ejes");
    const btVector3 factorAngular =
        cuerpoPtr->getRigidBody()->getAngularFactor();
    CHECK(factorAngular.x() == 1.0f && factorAngular.y() == 0.0f &&
              factorAngular.z() == 1.0f,
          "el freeze de rotacion llega al angularFactor de Bullet");

    GameObject vacio;
    CHECK(api->masa(&vacio) == 0.0f && !api->usaGravedad(&vacio) &&
              api->escalaGravedad(&vacio) == 0.0f &&
              api->friccion(&vacio) == 0.0f &&
              !api->posicionCongelada(&vacio, 0) &&
              !api->rotacionCongelada(&vacio, 0),
          "sin RigidBody los getters devuelven neutro");
    CHECK(!api->fijarMasa(&vacio, 1.0f) &&
              !api->fijarUsoGravedad(&vacio, true) &&
              !api->fijarEscalaGravedad(&vacio, 1.0f) &&
              !api->fijarFriccion(&vacio, 1.0f) &&
              !api->fijarFreezePosicion(&vacio, true, true, true) &&
              !api->fijarFreezeRotacion(&vacio, true, true, true),
          "sin RigidBody los setters informan que no aplican");
    fisica.removeRigidBody(cuerpoPtr);
}

void rigidBodySerializaPropiedadesNuevas() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_rigidbody_v2");
    const fs::path archivo = carpetaDir.ruta() / "rigidbody-v2.bin";

    Transform transform;
    EsfereCollider collider(1.0f, &transform);
    RigidBody cuerpo(&collider, 2.0f);
    cuerpo.fijarUsoGravedad(false);
    cuerpo.fijarEscalaGravedad(0.5f);
    cuerpo.fijarFriccion(0.9f);
    cuerpo.fijarFreezePosicion(true, false, false);
    cuerpo.fijarFreezeRotacion(false, false, true);
    cuerpo.fijarSensor(true);
    {
        std::ofstream salida(archivo, std::ios::binary);
        cuerpo.saveComponent(&salida);
    }
    Transform transformRecuperado;
    EsfereCollider colliderRecuperado(1.0f, &transformRecuperado);
    RigidBody recuperado(&colliderRecuperado, 1.0f);
    {
        std::ifstream entrada(archivo, std::ios::binary);
        recuperado.loadComponent(&entrada);
        CHECK(static_cast<bool>(entrada),
              "el formato v3 carga sin error");
    }
    CHECK(recuperado.masa() == 2.0f, "la masa se conserva al serializar");
    CHECK(!recuperado.usaGravedad(), "el uso de gravedad se conserva");
    CHECK(recuperado.escalaGravedad() == 0.5f,
          "la escala de gravedad se conserva");
    CHECK(std::abs(recuperado.friccion() - 0.9f) < 0.001f,
          "la friccion se conserva al serializar");
    CHECK(recuperado.posicionCongelada(0) &&
              !recuperado.posicionCongelada(1) &&
              !recuperado.posicionCongelada(2),
          "el freeze de posicion se conserva");
    CHECK(!recuperado.rotacionCongelada(0) &&
              !recuperado.rotacionCongelada(1) &&
              recuperado.rotacionCongelada(2),
          "el freeze de rotacion se conserva");
    CHECK(recuperado.esSensor(), "el flag de sensor se conserva al serializar");

    // Compatibilidad hacia atras: blob v2 escrito a mano (magia + version 2
    // + activo + masa + pos + rot + gravedad + friccion + freeze) carga con
    // el sensor en falso.
    const fs::path archivoV2 = carpetaDir.ruta() / "rigidbody-v2.bin";
    {
        std::ofstream salida(archivoV2, std::ios::binary);
        const std::uint32_t magia = 0x32444252;
        const std::uint32_t version = 2;
        const bool activo = true;
        const float masa = 4.0f;
        const float pos[3] = {0.0f, 0.0f, 0.0f};
        const float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        const bool usarGravedad = true;
        const float escala = 1.0f;
        const float friccion = 0.5f;
        const bool freezePos[3] = {false, false, false};
        const bool freezeRot[3] = {false, false, false};
        salida.write(reinterpret_cast<const char*>(&magia), sizeof(magia));
        salida.write(reinterpret_cast<const char*>(&version), sizeof(version));
        salida.write(reinterpret_cast<const char*>(&activo), sizeof(activo));
        salida.write(reinterpret_cast<const char*>(&masa), sizeof(masa));
        salida.write(reinterpret_cast<const char*>(pos), sizeof(pos));
        salida.write(reinterpret_cast<const char*>(&rot), sizeof(rot));
        salida.write(reinterpret_cast<const char*>(&usarGravedad),
                     sizeof(usarGravedad));
        salida.write(reinterpret_cast<const char*>(&escala), sizeof(escala));
        salida.write(reinterpret_cast<const char*>(&friccion), sizeof(friccion));
        salida.write(reinterpret_cast<const char*>(freezePos), sizeof(freezePos));
        salida.write(reinterpret_cast<const char*>(freezeRot), sizeof(freezeRot));
    }
    Transform transformV2;
    EsfereCollider colliderV2(1.0f, &transformV2);
    RigidBody cuerpoV2(&colliderV2, 1.0f);
    {
        std::ifstream entrada(archivoV2, std::ios::binary);
        cuerpoV2.loadComponent(&entrada);
        CHECK(static_cast<bool>(entrada), "el formato v2 sigue cargando");
    }
    CHECK(!cuerpoV2.esSensor(), "una escena v2 no trae sensores");

    // Compatibilidad hacia atras: blob v1 escrito a mano (magia + version 1
    // + activo + masa + pos + rot) carga con los valores por defecto.
    const fs::path archivoV1 = carpetaDir.ruta() / "rigidbody-v1.bin";
    {
        std::ofstream salida(archivoV1, std::ios::binary);
        const std::uint32_t magia = 0x32444252;
        const std::uint32_t version = 1;
        const bool activo = true;
        const float masa = 3.0f;
        const float pos[3] = {0.0f, 0.0f, 0.0f};
        const float rot[4] = {0.0f, 0.0f, 0.0f, 1.0f};
        salida.write(reinterpret_cast<const char*>(&magia), sizeof(magia));
        salida.write(reinterpret_cast<const char*>(&version), sizeof(version));
        salida.write(reinterpret_cast<const char*>(&activo), sizeof(activo));
        salida.write(reinterpret_cast<const char*>(&masa), sizeof(masa));
        salida.write(reinterpret_cast<const char*>(pos), sizeof(pos));
        salida.write(reinterpret_cast<const char*>(rot), sizeof(rot));
    }
    Transform transformV1;
    EsfereCollider colliderV1(1.0f, &transformV1);
    RigidBody cuerpoV1(&colliderV1, 1.0f);
    {
        std::ifstream entrada(archivoV1, std::ios::binary);
        cuerpoV1.loadComponent(&entrada);
        CHECK(static_cast<bool>(entrada), "el formato v1 sigue cargando");
    }
    CHECK(cuerpoV1.masa() == 3.0f, "v1 conserva la masa");
    CHECK(cuerpoV1.usaGravedad() && cuerpoV1.escalaGravedad() == 1.0f &&
              std::abs(cuerpoV1.friccion() - 0.5f) < 0.001f &&
              !cuerpoV1.posicionCongelada(0) &&
              !cuerpoV1.rotacionCongelada(0),
          "v1 toma gravedad, friccion y freeze por defecto");
}

// Un cuerpo sensor detecta el contacto (evento de inicio) pero no genera
// respuesta fisica: el otro cuerpo lo atraviesa. Se compara contra un
// colisionador solido como bloque de control (una sola variable: el flag).
void sensorDetectaSinRespuestaYPlano2D() {
    auto ensayar = [](bool comoSensor, float& posXFinal, int& eventosInicio) {
        GameObject sensor;
        GameObject movil;
        Transform* tSensor = sensor.getComponent<Transform>();
        Transform* tMovil = movil.getComponent<Transform>();
        tSensor->setTranslatef(0.0f, 10.0f, 0.0f);
        tMovil->setTranslatef(0.1f, 10.0f, 0.0f);

        auto colSensor = std::make_unique<EsfereCollider>(1.0f, tSensor, &sensor);
        Collider* colSensorPtr = colSensor.get();
        sensor.addComponent(std::move(colSensor));
        auto bodySensor = std::make_unique<RigidBody>(colSensorPtr, 0.0f);
        RigidBody* bodySensorPtr = bodySensor.get();
        if (comoSensor) bodySensorPtr->fijarSensor(true);
        sensor.addComponent(std::move(bodySensor));

        auto colMovil = std::make_unique<EsfereCollider>(1.0f, tMovil, &movil);
        Collider* colMovilPtr = colMovil.get();
        movil.addComponent(std::move(colMovil));
        auto bodyMovil = std::make_unique<RigidBody>(colMovilPtr, 1.0f);
        RigidBody* bodyMovilPtr = bodyMovil.get();
        bodyMovilPtr->fijarUsoGravedad(false);
        movil.addComponent(std::move(bodyMovil));

        BulletPhysicsAdapter fisica;
        fisica.addRigidBody(bodySensorPtr);
        fisica.addRigidBody(bodyMovilPtr);

        eventosInicio = 0;
        for (int i = 0; i < 30; ++i) {
            fisica.stepSimulation(1.0f / 60.0f);
            for (const EventoContacto& evento :
                 fisica.tomarEventosContacto()) {
                if (evento.tipo != TipoContacto::Inicio) continue;
                if ((evento.colliderA == colSensorPtr &&
                     evento.colliderB == colMovilPtr) ||
                    (evento.colliderA == colMovilPtr &&
                     evento.colliderB == colSensorPtr))
                    ++eventosInicio;
            }
        }
        posXFinal =
            bodyMovilPtr->getRigidBody()->getWorldTransform().getOrigin().x();

        fisica.removeRigidBody(bodySensorPtr);
        fisica.removeRigidBody(bodyMovilPtr);
    };

    float xSensor = 0.0f;
    float xSolido = 0.0f;
    int eventosSensor = 0;
    int eventosSolido = 0;
    ensayar(true, xSensor, eventosSensor);
    ensayar(false, xSolido, eventosSolido);

    CHECK(eventosSensor > 0, "el sensor detecta el contacto que atraviesa");
    CHECK(eventosSolido > 0, "el colisionador solido tambien detecta el contacto");
    CHECK(xSensor < xSolido,
          "el sensor no empuja: el movil lo atraviesa; el solido lo frena");

    // El flag del sensor y las restricciones 2D son propiedades del RigidBody
    // que deben llegar a la instancia de Bullet.
    GameObject objeto;
    Transform* t = objeto.getComponent<Transform>();
    auto col = std::make_unique<EsfereCollider>(1.0f, t, &objeto);
    Collider* colPtr = col.get();
    objeto.addComponent(std::move(col));
    auto cuerpo = std::make_unique<RigidBody>(colPtr, 1.0f);
    RigidBody* cuerpoPtr = cuerpo.get();
    objeto.addComponent(std::move(cuerpo));

    CHECK(!cuerpoPtr->esSensor() &&
              cuerpoPtr->getRigidBody()->hasContactResponse(),
          "un cuerpo normal responde al contacto");
    cuerpoPtr->fijarSensor(true);
    CHECK(!cuerpoPtr->getRigidBody()->hasContactResponse(),
          "el flag de sensor llega a Bullet (sin respuesta de contacto)");

    cuerpoPtr->fijarRestricciones2D();
    const btVector3 factorLineal =
        cuerpoPtr->getRigidBody()->getLinearFactor();
    const btVector3 factorAngular =
        cuerpoPtr->getRigidBody()->getAngularFactor();
    CHECK(factorLineal.z() == 0.0f && factorAngular.x() == 0.0f &&
              factorAngular.y() == 0.0f,
          "el modo 2D llega al linear/angular factor de Bullet");
}
