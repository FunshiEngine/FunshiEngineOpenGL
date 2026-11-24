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
#include "PropiedadesComponentesDetalle.h"

namespace MotorInspeccion {

// Propiedades de transform, luces, camara, color, grilla y skybox.
void CatalogoPropiedades::registrarEscena() {
    // --- Transform -------------------------------------------------------
    // El Transform guarda el estado en dos copias (la que usa el render y la
    // publica): por eso se escribe siempre por los setters.
    registrar("Transform", {
        propiedadVec3<Transform>(
            "position", "Posicion",
            [](Transform* t) {
                const float* p = t->getTranslatef();
                return std::array<float, 3>{p[0], p[1], p[2]};
            },
            [](Transform* t, float x, float y, float z) {
                t->setTranslatef(x, y, z);
            }),
        propiedadVec3<Transform>(
            "scale", "Escala",
            [](Transform* t) {
                const float* s = t->getScalef();
                return std::array<float, 3>{s[0], s[1], s[2]};
            },
            [](Transform* t, float x, float y, float z) {
                t->setScalef(x, y, z);
            }),
        // La rotacion del motor es un angulo global mas un eje: se expone
        // tal cual, sin convertir a angulos de Euler.
        propiedadFlotante<Transform>(
            "rotationAngle", "Angulo de rotacion (grados)",
            [](Transform* t) { return t->getRotatef()[0]; },
            [](Transform* t, float v) {
                const float* r = t->getRotatef();
                t->setRotatef(v, r[1], r[2], r[3]);
            }),
        propiedadVec3<Transform>(
            "rotationAxis", "Eje de rotacion",
            [](Transform* t) {
                const float* r = t->getRotatef();
                return std::array<float, 3>{r[1], r[2], r[3]};
            },
            [](Transform* t, float x, float y, float z) {
                const float* r = t->getRotatef();
                t->setRotatef(r[0], x, y, z);
            }),
    });

    // --- Light -----------------------------------------------------------
    registrar("Light", {
        propiedadEnumeracion<Light>(
            "type", "Tipo de luz",
            {"Direccional", "Puntual", "Spot"},
            [](Light* l) { return static_cast<int>(l->getType()); },
            [](Light* l, int v) { l->setType(static_cast<LightType>(v)); }),
        propiedadVec3<Light>(
            "ambient", "Ambiente",
            [](Light* l) {
                const float* v = l->getAmbient();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setAmbient(r, g, b); }),
        propiedadVec3<Light>(
            "diffuse", "Difuso",
            [](Light* l) {
                const float* v = l->getDiffuse();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setDiffuse(r, g, b); }),
        propiedadVec3<Light>(
            "specular", "Especular",
            [](Light* l) {
                const float* v = l->getSpecular();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setSpecular(r, g, b); }),
        // La atenuacion se fija de golpe en el motor: al escribir una parte se
        // conservan las otras dos.
        propiedadFlotante<Light>(
            "attenuationConstant", "Atenuacion constante",
            [](Light* l) { return l->getConstant(); },
            [](Light* l, float v) {
                l->setAttenuation(v, l->getLinear(), l->getQuadratic());
            }),
        propiedadFlotante<Light>(
            "attenuationLinear", "Atenuacion lineal",
            [](Light* l) { return l->getLinear(); },
            [](Light* l, float v) {
                l->setAttenuation(l->getConstant(), v, l->getQuadratic());
            }),
        propiedadFlotante<Light>(
            "attenuationQuadratic", "Atenuacion cuadratica",
            [](Light* l) { return l->getQuadratic(); },
            [](Light* l, float v) {
                l->setAttenuation(l->getConstant(), l->getLinear(), v);
            }),
        propiedadFlotante<Light>(
            "spotCutOff", "Apertura del foco (grados)",
            [](Light* l) { return l->getSpotCutOff(); },
            [](Light* l, float v) { l->setSpotCutOff(v); }),
    });

    // --- CameraComponent -------------------------------------------------
    registrar("CameraComponent", {
        propiedadFlotante<CameraComponent>(
            "fov", "Campo de vision (grados)",
            [](CameraComponent* c) { return c->getFov(); },
            [](CameraComponent* c, float v) { c->setFov(v); }),
        propiedadEnumeracion<CameraComponent>(
            "projection", "Proyeccion", {"Perspectiva", "Ortografica"},
            [](CameraComponent* c) {
                return static_cast<int>(c->getProjection());
            },
            [](CameraComponent* c, int v) {
                c->setProjection(static_cast<CameraComponent::Projection>(v));
            }),
        propiedadFlotante<CameraComponent>(
            "orthoSize", "Alto de la vista ortografica",
            [](CameraComponent* c) { return c->getOrthoSize(); },
            [](CameraComponent* c, float v) { c->setOrthoSize(v); }),
        propiedadFlotante<CameraComponent>(
            "nearPlane", "Plano cercano",
            [](CameraComponent* c) { return c->getNearPlane(); },
            [](CameraComponent* c, float v) { c->setNearPlane(v); }),
        propiedadFlotante<CameraComponent>(
            "farPlane", "Plano lejano",
            [](CameraComponent* c) { return c->getFarPlane(); },
            [](CameraComponent* c, float v) { c->setFarPlane(v); }),
        propiedadFlotante<CameraComponent>(
            "speed", "Velocidad de la camara",
            [](CameraComponent* c) { return c->getSpeed(); },
            [](CameraComponent* c, float v) { c->setSpeed(v); }),
    });

    // --- Color -----------------------------------------------------------
    registrar("Color", {
        propiedadVec3<Color>(
            "color", "Color",
            [](Color* c) {
                const float* v = c->getColor();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Color* c, float r, float g, float b) { c->setColor(r, g, b); }),
    });

    // --- Grid ------------------------------------------------------------
    registrar("Grid", {
        propiedadBooleana<Grid>(
            "visible", "Visible",
            [](Grid* g) { return g->getVisible(); },
            [](Grid* g, bool v) { g->setVisible(v); }),
        propiedadVec3<Grid>(
            "color", "Color",
            [](Grid* g) {
                const float* v = g->getColor();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Grid* g, float r, float gg, float b) { g->setColor(r, gg, b); }),
        propiedadFlotante<Grid>(
            "size", "Tamano de la celda",
            [](Grid* g) { return g->getTam(); },
            [](Grid* g, float v) { g->setTam(v); }),
        propiedadFlotante<Grid>(
            "separation", "Separacion",
            [](Grid* g) { return g->getSeparacion(); },
            [](Grid* g, float v) { g->setSeparacion(v); }),
    });

    // --- Skybox ----------------------------------------------------------
    registrar("Skybox", {
        propiedadBooleana<Skybox>(
            "visible", "Visible",
            [](Skybox* s) { return s->getVisible(); },
            [](Skybox* s, bool v) { s->setVisible(v); }),
        propiedadTexto<Skybox>(
            "facePositiveX", "Cara +X",
            [](Skybox* s) { return s->getCaraMasX(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasX(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeX", "Cara -X",
            [](Skybox* s) { return s->getCaraMenosX(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosX(v); }),
        propiedadTexto<Skybox>(
            "facePositiveY", "Cara +Y",
            [](Skybox* s) { return s->getCaraMasY(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasY(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeY", "Cara -Y",
            [](Skybox* s) { return s->getCaraMenosY(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosY(v); }),
        propiedadTexto<Skybox>(
            "facePositiveZ", "Cara +Z",
            [](Skybox* s) { return s->getCaraMasZ(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasZ(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeZ", "Cara -Z",
            [](Skybox* s) { return s->getCaraMenosZ(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosZ(v); }),
    });
}

}  // namespace MotorInspeccion
