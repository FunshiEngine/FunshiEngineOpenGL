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
#include "ScriptGameObjectDetalle.h"

namespace MotorScript {

namespace {

void detenerSonidosScript() {
    g_sonidosScript.detenerTodos(
        [](int handle) { if (g_audio) g_audio->detener(handle); });
}

void descartarPeticionesObjetos() {
    altasPendientes_.clear();
    componentesPendientes_.clear();
    bajasPendientes_.clear();
}

}  // namespace

const ScriptServices* tablaServicios() {
    static const ScriptServices tabla = {
        /* .reproducirSonido  = */ serviciosReproducirSonido,
        /* .detenerSonido     = */ serviciosDetenerSonido,
        /* .objetoPorNombre   = */ serviciosObjetoPorNombre,
        /* .objetoPorId       = */ serviciosObjetoPorId,
        /* .objetoPorEtiqueta = */ serviciosObjetoPorEtiqueta,
        /* .teclaSostiene     = */ serviciosTeclaSostiene,
        /* .teclaPresionada   = */ serviciosTeclaPresionada,
        /* .teclaSoltada      = */ serviciosTeclaSoltada,
        /* .deltaMouseX       = */ serviciosDeltaMouseX,
        /* .deltaMouseY       = */ serviciosDeltaMouseY,
        /* .fijarGravedadGlobal = */ serviciosFijarGravedadGlobal,
        /* .gravedadGlobalX   = */
        []() { return serviciosGravedadGlobalEje(0); },
        /* .gravedadGlobalY   = */
        []() { return serviciosGravedadGlobalEje(1); },
        /* .gravedadGlobalZ   = */
        []() { return serviciosGravedadGlobalEje(2); },
        /* .crearObjeto       = */ serviciosCrearObjeto,
        /* .destruirObjeto    = */ serviciosDestruirObjeto,
        /* .clonarObjeto      = */ serviciosClonarObjeto,
        /* .agregarColliderEsfera = */ serviciosAgregarColliderEsfera,
        /* .agregarColliderCubo = */ serviciosAgregarColliderCubo,
        /* .agregarRigidBody  = */ serviciosAgregarRigidBody,
        /* .objetoDeCollider  = */ serviciosObjetoDeCollider,
        /* .raycast           = */ serviciosRaycast,
        /* .puntoImpactoX     = */ serviciosPuntoImpactoX,
        /* .puntoImpactoY     = */ serviciosPuntoImpactoY,
        /* .puntoImpactoZ     = */ serviciosPuntoImpactoZ,
        /* .normalImpactoX    = */ serviciosNormalImpactoX,
        /* .normalImpactoY    = */ serviciosNormalImpactoY,
        /* .normalImpactoZ    = */ serviciosNormalImpactoZ,
        /* .distanciaImpacto  = */ serviciosDistanciaImpacto,
        /* .instanciarPrefab  = */ serviciosInstanciarPrefab,
        /* .listarPrefabs     = */ serviciosListarPrefabs,
        /* .interfazDeObjeto  = */ serviciosInterfazDeObjeto,
        /* .interfazActiva    = */ serviciosInterfazActiva,
        /* .interfazEstablecerActiva = */ serviciosInterfazEstablecerActiva,
        /* .interfazEstablecerEstado = */ serviciosInterfazEstablecerEstado,
        /* .interfazEstablecerValor  = */ serviciosInterfazEstablecerValor,
        /* .interfazValor     = */ serviciosInterfazValor,
        /* .reproducirMusica  = */ serviciosReproducirMusica,
        /* .detenerMusica     = */ serviciosDetenerMusica,
        /* .volumenMusica     = */ serviciosVolumenMusica,
        /* .interfazNativaDeObjeto = */ serviciosInterfazNativaDeObjeto,
        /* .interfazNativaVisible = */ serviciosInterfazNativaVisible,
        /* .interfazNativaEstablecerVisible = */
        serviciosInterfazNativaEstablecerVisible,
        /* .interfazNativaEstablecerTexto = */
        serviciosInterfazNativaEstablecerTexto,
        /* .interfazNativaEstablecerVisibleWidget = */
        serviciosInterfazNativaEstablecerVisibleWidget,
        /* .interfazNativaBotonPulsado = */
        serviciosInterfazNativaBotonPulsado,
        /* .version           = */ 10,
    };
    return &tabla;
}

// Llamada por GameScene al entrar en play (y al salir, con nullptrs) para
// cablear el contexto real de la escena a la tabla de servicios.
void inyectarServiciosScript(AudioEngine* audio, SceneRegistry* escena,
                             InputScripts* input, PhysicsEngine* fisica,
                             EditorController* editor,
                             EstadoInterfaces* interfaces) {
    if (!audio && !escena && !input) detenerSonidosScript();
    if (!escena) descartarPeticionesObjetos();
    g_audio = audio;
    g_escena = escena;
    g_input = input;
    g_fisica = fisica;
    g_editor = editor;
    g_interfaces = interfaces;
}

void procesarPeticionesObjetos() {
    if (!g_editor || !g_escena) {
        descartarPeticionesObjetos();
        return;
    }
    for (AltaPendiente& alta : altasPendientes_) {
        if (!alta.objeto) continue;
        GameObject* padre = (alta.padre && g_escena->contains(alta.padre))
                                ? alta.padre
                                : nullptr;
        GameObject* insertado =
            g_editor->createGameObject(std::move(alta.objeto), padre);
        // createGameObject no registra cuerpos (solo addComponent lo hace):
        // el clon o el objeto nuevo con RigidBody entraria al mundo jamas.
        if (insertado && insertado->getComponent<RigidBody>())
            g_editor->refreshRigidBody(insertado);
    }
    altasPendientes_.clear();
    for (ComponentePendiente& item : componentesPendientes_) {
        if (!item.objeto || !item.componente ||
            !g_escena->contains(item.objeto))
            continue;
        if (dynamic_cast<Collider*>(item.componente.get())) {
            if (item.objeto->getComponent<Collider>()) continue;
        }
        if (dynamic_cast<RigidBody*>(item.componente.get())) {
            if (!item.objeto->getComponent<Collider>() ||
                item.objeto->getComponent<RigidBody>())
                continue;
        }
        g_editor->addComponent(item.objeto, std::move(item.componente));
    }
    componentesPendientes_.clear();
    for (GameObject* o : bajasPendientes_) {
        if (o && g_escena->contains(o)) g_editor->deleteGameObject(o);
    }
    bajasPendientes_.clear();
}

}  // namespace MotorScript
