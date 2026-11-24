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

GameObject* GizmoController::pickObject(float mouseX, float mouseY,
                                        CameraComponent* camara,
                                        ListaDE<GameObject*>* objetos) {
    if (!objetos || objetos->isEmpty() || !camara) return nullptr;

    ImGuiIO& io = ImGui::GetIO();
    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;
    if (screenW <= 0.0f || screenH <= 0.0f) return nullptr;

    float view[16], projection[16];
    camara->getViewMatrix(view);
    camara->getProjectionMatrix(projection, screenW / screenH);

    Rayo rayo;
    if (!RayoPantalla::desdePixel(mouseX, mouseY, screenW, screenH,
                                  glm::make_mat4(view), glm::make_mat4(projection), rayo))
        return nullptr;
    const glm::vec3 rayOrigin = rayo.origen;
    const glm::vec3 rayDir = rayo.direccion;

    GameObject* closestObject = nullptr;
    float minDistance = 1e30f;

    Position<GameObject*>* pos = objetos->first();
    while (pos && pos->getElement()) {
        GameObject* obj = pos->getElement();
        Transform* transform = obj->getGlobalTransform();
        if (transform) {
            float modelArr[16];
            buildMatrixFromTransform(transform, modelArr);
            glm::mat4 modelMat = glm::make_mat4(modelArr);
            glm::mat4 invModel = glm::inverse(modelMat);

            glm::vec3 localRayOrigin = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
            glm::vec3 localRayDir = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

            glm::vec3 boxMin(-1.0f, -1.0f, -1.0f);
            glm::vec3 boxMax(1.0f, 1.0f, 1.0f);

            if (auto* model = obj->getComponent<Model>()) {
                vec3 bMin, bMax;
                if (model->getBoundingBox(bMin, bMax)) {
                    boxMin = glm::vec3(bMin.x, bMin.y, bMin.z);
                    boxMax = glm::vec3(bMax.x, bMax.y, bMax.z);
                }
            } else if (auto* sc = obj->getComponent<EsfereCollider>()) {
                float r = sc->getRadio();
                boxMin = glm::vec3(-r, -r, -r);
                boxMax = glm::vec3(r, r, r);
            } else if (auto* cc = obj->getComponent<CubeCollider>()) {
                float r = cc->getRadio();
                boxMin = glm::vec3(-r, -r, -r);
                boxMax = glm::vec3(r, r, r);
            }

            for (int i = 0; i < 3; ++i) {
                if (boxMax[i] - boxMin[i] < 0.4f) {
                    boxMin[i] -= 0.2f;
                    boxMax[i] += 0.2f;
                }
            }

            float tHit = 0.0f;
            if (intersectRayAABB(localRayOrigin, localRayDir, boxMin, boxMax, tHit)) {
                glm::vec3 hitPointWorld = glm::vec3(modelMat * glm::vec4(localRayOrigin + localRayDir * tHit, 1.0f));
                float dist = glm::length(hitPointWorld - rayOrigin);
                if (glm::dot(hitPointWorld - rayOrigin, rayDir) > 0.0f && dist < minDistance) {
                    minDistance = dist;
                    closestObject = obj;
                }
            }
        }
        pos = (pos != objetos->last()) ? objetos->next(pos) : nullptr;
    }

    return closestObject;
}
