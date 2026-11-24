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
#include "GizmoControllerDetalle.h"

void GizmoController::dibujarYRastrear(ImGuiIO& io, CameraComponent* camara,
                                       bool modo2D) {
    // Matrices de la vista activa: las necesita el gizmo (la pasada principal
    // ya las aplico por su cuenta; aca se recalculan para ImGuizmo). El aspect
    // se trunca igual que en GameScene cuando se pasa al renderer (fbW/fbH).
    float view[16], projection[16];
    camara->getViewMatrix(view);
    const int fbW = static_cast<int>(io.DisplaySize.x);
    const int fbH = static_cast<int>(io.DisplaySize.y);
    camara->getProjectionMatrix(
        projection, static_cast<float>(fbW) / static_cast<float>(fbH));

    listo_ = false;
    GameObject* selected =
        seleccion_ ? seleccion_->getReturnableEntity() : nullptr;

    // Gizmo generico: se edita el Transform que diga el GizmoTarget activo.
    // Default: el Transform del objeto seleccionado con el global de su padre
    // como contexto. Un GizmoTarget externo (SettingsCollider*) tiene
    // prioridad. Si no hay target externo, el transfor del collider con su
    // gizmo habilitado toma prioridad sobre el del objeto: asi el checkbox
    // "Gizmo activo" del transform del collider (via SettingsTransform)
    // activa/dormita el gizmo del offset del collider cuando quieras.
    GizmoTarget target;
    if (controlador_ && controlador_->hasGizmoTarget()) {
        target = controlador_->getGizmoTarget();
        // Refrescar el contexto global del duenio cada frame: si el objeto o
        // sus ancestros se movieron, el parentGlobal almacenado quedaria
        // desactualizado y el offset local se recompondria contra una base
        // vieja (el puntero en si es estable: globalTransformCache del owner).
        if (target.owner)
            target.parentGlobal = target.owner->getGlobalTransform();
    } else if (selected) {
        // Collider offset primero: si su transform local tiene el gizmo
        // encendido se edita el offset; si no, el transform del objeto.
        if (Collider* collider = selected->getComponent<Collider>()) {
            Transform* colliderTransform = collider->getTransform();
            if (colliderTransform && colliderTransform->gizmoHabilitado) {
                target.local = colliderTransform;
                target.parentGlobal = selected->getGlobalTransform();
                target.owner = selected;
            }
        }
        if (!target.local) {
            Transform* objectTransform = selected->getComponent<Transform>();
            if (objectTransform && objectTransform->gizmoHabilitado) {
                target.local = objectTransform;
                Entity* parentEnt = selected->getParentEntity();
                target.parentGlobal =
                    parentEnt ? parentEnt->getGlobalTransform() : nullptr;
                target.owner = selected;
            }
        }
    }

    // Guia de eje activa (X/Y/Z): en vez de apagar el gizmo, se limita a la
    // flecha del eje elegido. ImGuizmo modela la operacion como mascara de
    // bits, asi que pasar TRANSLATE_X/Y/Z (en vez de TRANSLATE, que son los tres)
    // hace que dibuje solo esa flecha y solo responda al arrastre de esa
    // flecha: el usuario agarra un unico punto y mueve el objeto por un unico
    // eje, que es justo lo que promete la guia. El resto de la maquinaria
    // (foto para undo, desescala y escritura al transform local) es la misma.
    int guiaEje = -1;
    if (controlador_) guiaEje = controlador_->getGuiaEje();

    // Operacion efectiva de este frame: con guia activa es traslacion de un
    // solo eje, sin importar cual era la operacion del gizmo (1/T, 2/R, 3/U).
    // En 2D la rotacion se limita a Z (el eje perpendicular al plano XY): el
    // anillo completo giraria fuera del plano, que en una vista ortografica no
    // se puede leer. La traslacion y la escala no cambian.
    int operacionBase = static_cast<int>(operation_);
    if (modo2D && operacionBase == ImGuizmo::ROTATE)
        operacionBase = ImGuizmo::ROTATE_Z;
    const int operacionEfectiva =
        guiaEje >= GuiaEje::kEjeX && guiaEje <= GuiaEje::kEjeZ
            ? static_cast<int>(ImGuizmo::TRANSLATE_X << guiaEje)
            : operacionBase;

    if (target.local && operacionEfectiva != 0) {
        // Matriz que maniula el gizmo: parentGlobal * local. Para translate y
        // rotate ImGuizmo EXPLOTA con matrices escaladas (no-ortonormales):
        // con el objeto o su padre escalado, el objeto sale disparado al usar
        // el gizmo. Por eso se desescala antes de pasarla y se reinserta la
        // escala al leer el resultado.
        float scaleVec[3] = {1.0f, 1.0f, 1.0f};
        const bool sinEscala = operacionEfectiva != ImGuizmo::SCALE;

        float localArr[16];
        buildMatrixFromTransform(target.local, localArr);
        glm::mat4 mFull = glm::make_mat4(localArr);
        if (target.parentGlobal) {
            float parentArr[16];
            buildMatrixFromTransform(target.parentGlobal, parentArr);
            mFull = glm::make_mat4(parentArr) * mFull;
        }

        // Si la matriz de entrada ya es no finita (transform del objeto o de
        // algun ancestro corrupto), NO se opera el gizmo con ella: un drag la
        // escribiria tal cual y quedaria -nan en el transform. Se sigue con el
        // resto del frame con el gizmo apagado (es seguro: solo se dibuja).
        if (matrizNoFinita(mFull)) {
            listo_ = false;
            return;
        }

        if (sinEscala) {
            // Ortonormalizar zoom: guardar escala por columna y normalizar.
            glm::vec4 c0 = mFull[0];
            glm::vec4 c1 = mFull[1];
            glm::vec4 c2 = mFull[2];
            scaleVec[0] = glm::length(c0);
            scaleVec[1] = glm::length(c1);
            scaleVec[2] = glm::length(c2);
            // Ojo: el guard < 0.0001 NO atrapa NaN (toda comparacion con NaN
            // es false). Sin isfinite, una escala NaN se divide por si misma y
            // contamina toda la matriz.
            for (int i = 0; i < 3; ++i) {
                if (!std::isfinite(scaleVec[i]) || scaleVec[i] < 0.0001f)
                    scaleVec[i] = 1.0f;
            }
            mFull[0] = c0 / scaleVec[0];
            mFull[1] = c1 / scaleVec[1];
            mFull[2] = c2 / scaleVec[2];
        }

        float matrix[16];
        const float* ptr = glm::value_ptr(mFull);
        for (int i = 0; i < 16; ++i) matrix[i] = ptr[i];

        // Sistema de coordenadas del gizmo: LOCAL (= ejercicio historico, los ejes
        // rotan con el objeto) o GLOBAL/WORLD (ejes del mundo fijos, el gizmo
        // NO rota con el objeto). En WORLD el redondeo de la matriz se hace
        // igual contra inv(parentGlobal), asi que ambos conviven sin tocar la
        // escritura de vuelta al local.
        const ImGuizmo::MODE modoGizmo =
            global_ ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
        ImGuizmo::Manipulate(view, projection,
                             static_cast<ImGuizmo::OPERATION>(operacionEfectiva),
                             modoGizmo, matrix, nullptr,
                             nullptr, nullptr, nullptr);
        listo_ = true;

        // Arrastre del gizmo: UNA foto del transform al iniciar el drag (antes
        // de escribir la matriz de este frame) y, al soltarlo, un solo
        // TransformComando con el estado inicial y el final. Asi el historial
        // del editor tiene una entrada por movimiento y no una por frame.
        // Solo aplica al transform del OBJETO: el offset local de un collider
        // se edita sobre el componente y no tiene comando propio.
        const bool usandoGizmo = ImGuizmo::IsUsing();
        if (usandoGizmo && !arrastrando_) {
            arrastrando_ = true;
            arrastreComando_.reset();
            const bool editaObjeto =
                target.owner &&
                target.owner->getComponent<Transform>() == target.local;
            if (editaObjeto && target.owner->getId() > 0 && target.local) {
                tomarFotoTransform(target.local, arrastreInicial_);
                arrastreFinal_ = arrastreInicial_;
                // El constructor captura el estado actual como "anterior": el
                // gizmo todavia no escribio este frame.
                arrastreComando_ = std::make_unique<TransformComando>(
                    controlador_, target.owner, registro_);
            }
        }
        if (usandoGizmo) {
            // Reinsertar la escala que quitamos: M = M' * diag(scale).
            glm::mat4 mManip = glm::make_mat4(matrix);
            if (sinEscala) {
                glm::mat4 sMat = glm::scale(
                    glm::mat4(1.0f), glm::vec3(scaleVec[0], scaleVec[1], scaleVec[2]));
                mManip = mManip * sMat;
            }

            // Escribir de vuelta AL local: newLocal = inv(parentGlobal) * matrix
            Transform* localTransform = target.local;
            glm::mat4 newLocal = mManip;
            if (target.parentGlobal) {
                float parentGlobalArr[16];
                buildMatrixFromTransform(target.parentGlobal, parentGlobalArr);
                glm::mat4 invParentGlobal = glm::inverse(glm::make_mat4(parentGlobalArr));
                // glm::inverse de una matriz singular/no finita produce Inf/NaN.
                if (matrizNoFinita(glm::make_mat4(parentGlobalArr)) ||
                    matrizNoFinita(invParentGlobal)) {
                    return;
                }
                newLocal = invParentGlobal * mManip;
            }

            // El resultado del drag no debe corromper el transform con -nan:
            // si la matriz manipulada quedo no finita, se descarta este frame.
            if (matrizNoFinita(mManip) || matrizNoFinita(newLocal)) return;
            float localMatArr[16];
            const float* ptr2 = glm::value_ptr(newLocal);
            for (int i = 0; i < 16; ++i) localMatArr[i] = ptr2[i];
            decomposeMatrixToTransform(localMatArr, localTransform);

            // Congelar hijos SOLO al editar el transform de un objeto; el
            // offset local de un componente (collider) no arrastra hijos.
            const bool esObjeto =
                target.owner && target.owner->getComponent<Transform>() == target.local;
            bool freeze = false;
            if (esObjeto && target.local) freeze = target.local->childsFreeze;

            std::vector<std::pair<Entity*, glm::mat4>> childSnapshots;
            if (freeze && target.owner) {
                for (auto* child : target.owner->getChildEntities()) {
                    if (child && child->getComponent<Transform>()) {
                        float m[16];
                        buildMatrixFromTransform(child->getGlobalTransform(), m);
                        childSnapshots.push_back({child, glm::make_mat4(m)});
                    }
                }
            }

            if (freeze && !childSnapshots.empty() && target.owner) {
                float pM[16];
                buildMatrixFromTransform(target.owner->getGlobalTransform(), pM);
                glm::mat4 invParent = glm::inverse(glm::make_mat4(pM));
                for (auto& snap : childSnapshots) {
                    glm::mat4 newLocal = invParent * snap.second;
                    float localArr2[16];
                    const float* ptr = glm::value_ptr(newLocal);
                    for (int i = 0; i < 16; ++i) localArr2[i] = ptr[i];
                    decomposeMatrixToTransform(localArr2, snap.first->getComponent<Transform>());
                }
            }

            // El gizmo movio el transform del target: empujarlo hacia el
            // cuerpo fisico para que la simulacion parta de donde quedo
            // visualmente (INCLUYE los hijos con RigidBody).
            if (target.owner) {
                if (RigidBody* body = target.owner->getComponent<RigidBody>())
                    body->syncGameObjectToPhysics();
                for (auto* child : target.owner->getChildEntities()) {
                    if (child && child->getComponent<RigidBody>())
                        child->getComponent<RigidBody>()->syncGameObjectToPhysics();
                }
            }

            // Estado final del arrastre: lo consumira el comando de undo.
            if (arrastreComando_ && target.local)
                tomarFotoTransform(target.local, arrastreFinal_);
        } else if (arrastrando_) {
            // Se solto el gizmo: se registra UN comando con el estado inicial
            // y el final. ejecutar() reaplica los valores finales (el gizmo ya
            // los escribio en disco/memoria) y deja el comando en la pila de
            // undo, que es lo que consume Ctrl+Z.
            arrastrando_ = false;
            if (arrastreComando_ &&
                transformDistinguible(arrastreInicial_, arrastreFinal_)) {
                arrastreComando_->setNuevoEstado(
                    arrastreFinal_.pos[0], arrastreFinal_.pos[1],
                    arrastreFinal_.pos[2], arrastreFinal_.rot[0],
                    arrastreFinal_.rot[1], arrastreFinal_.rot[2],
                    arrastreFinal_.rot[3], arrastreFinal_.esc[0],
                    arrastreFinal_.esc[1], arrastreFinal_.esc[2]);
                if (controlador_)
                    controlador_->getGestorComandos()->ejecutar(
                        std::move(arrastreComando_));
            }
            // Sin cambios (clic sin mover) o sin comando: no hay nada que
            // registrar y el historial queda como estaba.
            arrastreComando_.reset();
        }
    }
}
