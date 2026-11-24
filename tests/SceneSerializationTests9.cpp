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
#include "../FunshiEngineGL/src/GUI/ObjetosGUI/SettingsComponent.h"

// Fisica: el paso es fijo (1/60) con subpasos, asi un frame largo avanza su
// tiempo completo; quitar un cuerpo no reinicia los contactos de los demas; y
// cada btRigidBody conoce su RigidBody (busqueda O(1) en los contactos).
void fisicaPasoFijoYContactosAlQuitar() {
    // 1. Caida libre de 0.1 s en un solo frame: v = g * t con g = -9.81.
    {
        GameObject cae;
        Transform* t = cae.getComponent<Transform>();
        t->setTranslatef(0.0f, 100.0f, 0.0f);
        auto col = std::make_unique<EsfereCollider>(0.5f, t, &cae);
        Collider* colPtr = col.get();
        cae.addComponent(std::move(col));
        auto cuerpo = std::make_unique<RigidBody>(colPtr, 1.0f);
        RigidBody* cuerpoPtr = cuerpo.get();
        cae.addComponent(std::move(cuerpo));
        CHECK(cuerpoPtr->getRigidBody()->getUserPointer() == cuerpoPtr,
              "el btRigidBody apunta a su RigidBody");

        BulletPhysicsAdapter fisica;
        fisica.addRigidBody(cuerpoPtr);
        fisica.stepSimulation(0.1f);
        const float vy = cuerpoPtr->getRigidBody()->getLinearVelocity().y();
        CHECK(std::abs(vy - (-9.81f * 0.1f)) < 0.05f,
              "un frame de 0.1 s simula 0.1 s completos (antes solo 1/60)");
        fisica.removeRigidBody(cuerpoPtr);
    }

    // 2. Un contacto que sigue no vuelve a ser "Inicio" porque salga del mundo
    //    otro cuerpo que no participa en el.
    {
        GameObject sensor, movil, lejano;
        Transform* tS = sensor.getComponent<Transform>();
        Transform* tM = movil.getComponent<Transform>();
        Transform* tL = lejano.getComponent<Transform>();
        tS->setTranslatef(0.0f, 10.0f, 0.0f);
        tM->setTranslatef(0.1f, 10.0f, 0.0f);
        tL->setTranslatef(50.0f, 10.0f, 0.0f);
        auto colS = std::make_unique<EsfereCollider>(1.0f, tS, &sensor);
        auto colM = std::make_unique<EsfereCollider>(1.0f, tM, &movil);
        auto colL = std::make_unique<EsfereCollider>(1.0f, tL, &lejano);
        Collider* pS = colS.get();
        Collider* pM = colM.get();
        Collider* pL = colL.get();
        sensor.addComponent(std::move(colS));
        movil.addComponent(std::move(colM));
        lejano.addComponent(std::move(colL));
        auto bS = std::make_unique<RigidBody>(pS, 0.0f);
        auto bM = std::make_unique<RigidBody>(pM, 1.0f);
        auto bL = std::make_unique<RigidBody>(pL, 0.0f);
        RigidBody* rS = bS.get();
        RigidBody* rM = bM.get();
        RigidBody* rL = bL.get();
        rS->fijarSensor(true);
        rM->fijarUsoGravedad(false);
        sensor.addComponent(std::move(bS));
        movil.addComponent(std::move(bM));
        lejano.addComponent(std::move(bL));

        BulletPhysicsAdapter fisica;
        fisica.addRigidBody(rS);
        fisica.addRigidBody(rM);
        fisica.addRigidBody(rL);
        auto tipoDelPar = [&](const std::vector<EventoContacto>& eventos, int& tipo) {
            tipo = -1;
            for (const EventoContacto& e : eventos)
                if ((e.colliderA == pS && e.colliderB == pM) || (e.colliderA == pM && e.colliderB == pS))
                    tipo = static_cast<int>(e.tipo);
        };
        int tipo = -1;
        for (int i = 0; i < 3; ++i) {
            fisica.stepSimulation(1.0f / 60.0f);
            tipoDelPar(fisica.tomarEventosContacto(), tipo);
        }
        CHECK(tipo == static_cast<int>(TipoContacto::Persistencia),
              "el sensor y el movil siguen en contacto");
        fisica.removeRigidBody(rL);
        CHECK(pM->cantidadContactos() > 0, "quitar otro cuerpo no borra los contactos del movil");
        fisica.stepSimulation(1.0f / 60.0f);
        tipoDelPar(fisica.tomarEventosContacto(), tipo);
        CHECK(tipo == static_cast<int>(TipoContacto::Persistencia),
              "el contacto sigue siendo persistente tras quitar un cuerpo ajeno");
        fisica.removeRigidBody(rS);
        CHECK(pM->cantidadContactos() == 0, "al quitar el sensor el movil lo olvida");
        fisica.removeRigidBody(rM);
    }
}

// --- Dos componentes del mismo tipo: cada panel edita el suyo ----------------
// El inspector decidia si un panel seguia vigente mirando solo el NOMBRE del
// tipo. Con dos Color en el objeto, borrar uno dejaba vivo su panel (seguia
// habiendo "un Color") y ese panel quedaba apuntando al componente liberado.
// Ahora el panel guarda su componente y la purga compara direcciones contra la
// lista viva del objeto.
void dosComponentesIgualesTienenPanelPropio() {
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    GameObject* raiz = registry.getRoot();

    GameObject* a =
        editor.createGameObject(GameObjectFactory::createSimpleObject(raiz), raiz);
    CHECK(a != nullptr, "el objeto con dos Color entra en la escena");
    if (!a) return;

    SettingsObjectInterface inspector(a, true);
    inspector.setEditor(&editor);
    inspector.setEventBus(&events);

    auto primero = std::make_unique<Color>();
    auto segundo = std::make_unique<Color>();
    Color* c1 = primero.get();
    Color* c2 = segundo.get();
    CHECK(editor.addComponent(a, std::move(primero)), "se agrega el primer Color");
    CHECK(editor.addComponent(a, std::move(segundo)), "se agrega el segundo Color");

    SettingsComponent* p1 = inspector.settingsEnIndice(1);
    SettingsComponent* p2 = inspector.settingsEnIndice(2);
    CHECK(p1 && p1->componenteDelPanel() == c1,
          "el primer panel de Color edita el primer Color");
    CHECK(p2 && p2->componenteDelPanel() == c2,
          "el segundo panel de Color edita el segundo Color (no el primero)");
    CHECK(p2 && p2->getComponent() == c2,
          "getComponent() del segundo panel devuelve su propio Color");

    // Borrar el PRIMER Color: su panel se va y el del segundo sobrevive.
    CHECK(editor.removeComponent(a, c1), "se borra el primer Color");
    CHECK(inspector.settingsEnIndice(1) == p2,
          "el panel del segundo Color sobrevive y ocupa el lugar del borrado");
    CHECK(inspector.settingsEnIndice(2) == nullptr,
          "el panel del Color borrado no queda en la lista");
    CHECK(inspector.settingsEnIndice(1) &&
              inspector.settingsEnIndice(1)->componenteDelPanel() == c2,
          "ningun panel queda apuntando al Color liberado");
}

int main() {
    fisicaPasoFijoYContactosAlQuitar();
    dosComponentesIgualesTienenPanelPropio();
    nombresPorDefecto();
    roundTripDeEscena();
    tagsCompatiblesConEscenasAnteriores();
    restaurarBaselineDeEscena();
    guardadoConArbolVacio();
    reporteFalloBinario();
    indiceCorruptoSinFantasmas();
    diagnosticosCargaEscena();
    diagnosticosCargaMalla();
    diagnosticosCargaScript();
    hijoConIdCeroSeReasignaAlGuardar();
    hermanosConsecutivosSinPerdida();
    rutasDeCarasDeSkybox();
    reescrituraDeReferencias();
    sanadoDeRutasRotas();
    variosScriptsPorObjeto();
    nombreDeComponenteScriptSeSerializa();
    rotacionApiUsaRadianes();
    elInspectorSeDesvinculaAlBorrar();
    laCamaraBorradaNoSeUsaEnElRestoDelFrame();
    borrarCrearBorrarNoDesalineaElBinario();
    elModeloSeResuelveConLaMatrizMundial();
    elEventoDePropiedadNoReconstruyeElInspector();
    elInspectorDibujaUnFrameSinArrastre();
    reparentarPreservaLaPoseYRefrescaElCuerpo();
    reparentarSobreviveElGuardado();
    opcionesDeRenderYFisicaSePersisten();
    movimientoFisicoYSaltoDesdeScripts();
    eventosDeContactoExponenColliderYPropietario();
    busquedaDeObjetosPorNombreIdYEtiqueta();
    musicaDesdeScripts();
    propiedadesFisicasDesdeScripts();
    rigidBodySerializaPropiedadesNuevas();
    sensorDetectaSinRespuestaYPlano2D();
    gestionDeObjetosDesdeScripts();
    siembraPorDimension();
    proyeccionDeCamara();
    spriteRendererSerializaRoundTrip();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
