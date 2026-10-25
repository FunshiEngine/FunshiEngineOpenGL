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
#include "Transform.h"

#include <cmath>
#include <cstdint>
#include <ios>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/type_ptr.hpp>

// Marca de version del bloque Transform. Al inicio del bloque permite:
//  - escribir childsFreeze (que antes se perdia al guardar), y
//  - leer escenas viejas (sin marca) retrocediendo el cursor y asumiendo el
//    formato original (solo translate/scale/rotate, childsFreeze por defecto).
static constexpr std::uint32_t kTransformMagic = 0x54524632u; // "TRF2"

Transform::Transform() {
    // Inicializar arrays auxiliares
    for (int i = 0; i < 3; i++) {
        arrTranslatef[i] = objectTranslatef[i];
        arrScalef[i] = objectScalef[i];
    }
    for (int i = 0; i < 4; i++) {
        arrRotatef[i] = objectRotatef[i];
    }
}

void Transform::serializeComponent(std::ofstream* fileNamePathContentObject) {
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&kTransformMagic),
                                     sizeof(kTransformMagic));
    fileNamePathContentObject->write(
        reinterpret_cast<const char*>(&objectTranslatef), sizeof(float) * 3);
    fileNamePathContentObject->write(
        reinterpret_cast<const char*>(&objectScalef), sizeof(float) * 3);
    fileNamePathContentObject->write(
        reinterpret_cast<const char*>(&objectRotatef), sizeof(float) * 4);
    const unsigned char freeze = childsFreeze ? 1u : 0u;
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&freeze),
                                     sizeof(freeze));
}

void Transform::deserializeComponent(std::ifstream* fileNamePathContentObject) {
    // Formato nuevo: marca + datos + childsFreeze. Formato viejo: arranca
    // directo con translate. Se detecta por la marca; si no esta, se retrocede
    // el cursor y se lee el layout original.
    std::uint32_t marca = 0;
    fileNamePathContentObject->read(reinterpret_cast<char*>(&marca),
                                    sizeof(marca));

    if (marca == kTransformMagic) {
        fileNamePathContentObject->read(
            reinterpret_cast<char*>(&objectTranslatef), sizeof(float) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(&objectScalef),
                                        sizeof(float) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(&objectRotatef),
                                        sizeof(float) * 4);
        unsigned char freeze = 0;
        fileNamePathContentObject->read(reinterpret_cast<char*>(&freeze),
                                        sizeof(freeze));
        childsFreeze = freeze != 0u;
    } else {
        fileNamePathContentObject->seekg(-static_cast<std::streamoff>(sizeof(marca)),
                                         std::ios::cur);
        fileNamePathContentObject->read(
            reinterpret_cast<char*>(&objectTranslatef), sizeof(float) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(&objectScalef),
                                        sizeof(float) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(&objectRotatef),
                                        sizeof(float) * 4);
        childsFreeze = false;
    }

    // Actualizar los arrays auxiliares
    for (int i = 0; i < 3; i++) {
        arrTranslatef[i] = objectTranslatef[i];
        arrScalef[i] = objectScalef[i];
    }
    for (int i = 0; i < 4; i++) {
        arrRotatef[i] = objectRotatef[i];
    }
}

void Transform::setTranslatef(float x, float y, float z) {
    objectTranslatef[0] = x;
    objectTranslatef[1] = y;
    objectTranslatef[2] = z;

    // Actualizar el array auxiliar
    for (int i = 0; i < 3; i++) {
        arrTranslatef[i] = objectTranslatef[i];
    }
}

void Transform::setRotatef(float angle, float x, float y, float z) {
    objectRotatef[0] = angle;
    objectRotatef[1] = x;
    objectRotatef[2] = y;
    objectRotatef[3] = z;

    // Actualizar el array auxiliar
    for (int i = 0; i < 4; i++) {
        arrRotatef[i] = objectRotatef[i];
    }
}

void Transform::setScalef(float x, float y, float z) {
    objectScalef[0] = x;
    objectScalef[1] = y;
    objectScalef[2] = z;

    // Actualizar el array auxiliar
    for (int i = 0; i < 3; i++) {
        arrScalef[i] = objectScalef[i];
    }
}

void buildMatrixFromTransform(Transform* t, float outMatrix[16]) {
    glm::mat4 mat(1.0f);
    mat = glm::translate(
        mat, glm::vec3(t->getTranslatef()[0], t->getTranslatef()[1],
                       t->getTranslatef()[2]));

    float angle = t->getRotatef()[0];
    glm::vec3 axis(t->getRotatef()[1], t->getRotatef()[2], t->getRotatef()[3]);
    if (glm::length(axis) > 0.0001f)
        mat = glm::rotate(mat, glm::radians(angle), glm::normalize(axis));

    mat = glm::scale(
        mat, glm::vec3(t->getScalef()[0], t->getScalef()[1], t->getScalef()[2]));

    const float* ptr = glm::value_ptr(mat);
    for (int i = 0; i < 16; i++) outMatrix[i] = ptr[i];
}

void decomposeMatrixToTransform(float inMatrix[16], Transform* t) {
    glm::mat4 mat = glm::make_mat4(inMatrix);

    // Defensa contra NaN/Inf: si la matriz de entrada tiene valores no
    // finitos (p.ej. por glm::inverse de un padre singular, o un drag del
    // gizmo sobre datos corruptos), glm::decompose los propaga y el objeto
    // queda en -nan -> desaparece de la escena. Ante eso se ABORTA sin tocar
    // el transform (se conserva el valor anterior en vez de corromperlo).
    const float* M = inMatrix;
    for (int i = 0; i < 16; ++i) {
        if (!std::isfinite(M[i])) return;
    }

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(mat, scale, rotation, translation, skew, perspective);

    // Cualquier componente no finito en el resultado: abortar sin tocar.
    if (!std::isfinite(translation.x) || !std::isfinite(translation.y) ||
        !std::isfinite(translation.z) || !std::isfinite(scale.x) ||
        !std::isfinite(scale.y) || !std::isfinite(scale.z))
        return;

    t->setTranslatef(translation.x, translation.y, translation.z);

    rotation = glm::normalize(rotation);
    float angle = glm::degrees(glm::angle(rotation));
    glm::vec3 axis = glm::axis(rotation);
    if (std::isnan(angle) || !std::isfinite(angle) ||
        glm::length(axis) < 0.0001f || !std::isfinite(axis.x) ||
        !std::isfinite(axis.y) || !std::isfinite(axis.z)) {
        angle = 0.0f;
        axis = glm::vec3(0, 1, 0);
    }
    t->setRotatef(angle, axis.x, axis.y, axis.z);

    // No se admite escala no finita: 0 (ejes aplanados) y NaN/Inf corrompen
    // toda la cadena de matrices posterior.
    if (std::isfinite(scale.x) && std::isfinite(scale.y) &&
        std::isfinite(scale.z))
        t->setScalef(scale.x, scale.y, scale.z);
}