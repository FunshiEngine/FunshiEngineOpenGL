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

// Propiedades de materiales, sprites, animacion, modelos, tilemaps y particulas.
void CatalogoPropiedades::registrarRender() {
    // --- Material --------------------------------------------------------
    // Los colores del material son RGBA, pero el setter fuerza alpha = 1: se
    // expone solo la parte RGB.
    registrar("Material", {
        propiedadVec3<Material>(
            "ambient", "Ambiente",
            [](Material* m) {
                const float* v = m->getAmbient();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setAmbient(r, g, b); }),
        propiedadVec3<Material>(
            "diffuse", "Difuso",
            [](Material* m) {
                const float* v = m->getDiffuse();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setDiffuse(r, g, b); }),
        propiedadVec3<Material>(
            "specular", "Especular",
            [](Material* m) {
                const float* v = m->getSpecular();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setSpecular(r, g, b); }),
        propiedadVec3<Material>(
            "emission", "Emision",
            [](Material* m) {
                const float* v = m->getEmission();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setEmission(r, g, b); }),
        propiedadFlotante<Material>(
            "shininess", "Brillo",
            [](Material* m) { return m->getShininess(); },
            [](Material* m, float v) { m->setShininess(v); }),
        propiedadTexto<Material>(
            "diffuseMap", "Textura difusa",
            [](Material* m) { return m->getDiffuseMapPath(); },
            [](Material* m, const std::string& v) { m->setDiffuseMapPath(v); }),
        propiedadTexto<Material>(
            "specularMap", "Textura especular",
            [](Material* m) { return m->getSpecularMapPath(); },
            [](Material* m, const std::string& v) { m->setSpecularMapPath(v); }),
        propiedadTexto<Material>(
            "normalMap", "Textura de normales",
            [](Material* m) { return m->getNormalMapPath(); },
            [](Material* m, const std::string& v) { m->setNormalMapPath(v); }),
        propiedadTexto<Material>(
            "emissionMap", "Textura de emision",
            [](Material* m) { return m->getEmissionMapPath(); },
            [](Material* m, const std::string& v) { m->setEmissionMapPath(v); }),
    });

    // --- EmisorParticulas ------------------------------------------------
    registrar("EmisorParticulas", {
        propiedadBooleana<EmisorParticulas>(
            "active", "Activo",
            [](EmisorParticulas* e) { return e->activa(); },
            [](EmisorParticulas* e, bool v) { e->setActiva(v); }),
        propiedadEntera<EmisorParticulas>(
            "rate", "Particulas por segundo",
            [](EmisorParticulas* e) { return e->parametros().cantidadPorSegundo; },
            [](EmisorParticulas* e, int v) {
                e->parametros().cantidadPorSegundo = v < 0 ? 0 : v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "lifeMin", "Vida minima (segundos)",
            [](EmisorParticulas* e) { return e->parametros().vidaMinima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().vidaMinima = v;
                if (e->parametros().vidaMaxima < v)
                    e->parametros().vidaMaxima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "lifeMax", "Vida maxima (segundos)",
            [](EmisorParticulas* e) { return e->parametros().vidaMaxima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().vidaMaxima = v;
                if (e->parametros().vidaMinima > v)
                    e->parametros().vidaMinima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "speedMin", "Velocidad minima",
            [](EmisorParticulas* e) { return e->parametros().velocidadMinima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadMinima = v;
                if (e->parametros().velocidadMaxima < v)
                    e->parametros().velocidadMaxima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "speedMax", "Velocidad maxima",
            [](EmisorParticulas* e) { return e->parametros().velocidadMaxima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadMaxima = v;
                if (e->parametros().velocidadMinima > v)
                    e->parametros().velocidadMinima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "sizeMin", "Tamano minimo",
            [](EmisorParticulas* e) { return e->parametros().tamanoMinimo; },
            [](EmisorParticulas* e, float v) {
                e->parametros().tamanoMinimo = v;
                if (e->parametros().tamanoMaximo < v)
                    e->parametros().tamanoMaximo = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "sizeMax", "Tamano maximo",
            [](EmisorParticulas* e) { return e->parametros().tamanoMaximo; },
            [](EmisorParticulas* e, float v) {
                e->parametros().tamanoMaximo = v;
                if (e->parametros().tamanoMinimo > v)
                    e->parametros().tamanoMinimo = v;
            }),
        propiedadVec4<EmisorParticulas>(
            "color", "Color",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().color;
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](EmisorParticulas* e, float r, float g, float b, float a) {
                float* v = e->parametros().color;
                v[0] = r; v[1] = g; v[2] = b; v[3] = a;
            }),
        propiedadVec3<EmisorParticulas>(
            "direction", "Direccion",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().direccion;
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](EmisorParticulas* e, float x, float y, float z) {
                float* v = e->parametros().direccion;
                v[0] = x; v[1] = y; v[2] = z;
            }),
        propiedadFlotante<EmisorParticulas>(
            "dispersion", "Dispersion del cono",
            [](EmisorParticulas* e) { return e->parametros().dispersion; },
            [](EmisorParticulas* e, float v) {
                e->parametros().dispersion = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            }),
        propiedadVec3<EmisorParticulas>(
            "gravity", "Gravedad",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().gravedad;
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](EmisorParticulas* e, float x, float y, float z) {
                float* v = e->parametros().gravedad;
                v[0] = x; v[1] = y; v[2] = z;
            }),
        propiedadFlotante<EmisorParticulas>(
            "drag", "Rozamiento",
            [](EmisorParticulas* e) { return e->parametros().rozamiento; },
            [](EmisorParticulas* e, float v) {
                e->parametros().rozamiento = v < 0.0f ? 0.0f : v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "rotation", "Rotacion inicial (radianes)",
            [](EmisorParticulas* e) { return e->parametros().rotacionInicial; },
            [](EmisorParticulas* e, float v) {
                e->parametros().rotacionInicial = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "rotationSpeed", "Velocidad de rotacion (rad/s)",
            [](EmisorParticulas* e) { return e->parametros().velocidadRotacion; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadRotacion = v;
            }),
        propiedadTexto<EmisorParticulas>(
            "profilePath", "Perfil de efecto",
            [](EmisorParticulas* e) { return e->rutaPerfil(); },
            [](EmisorParticulas* e, const std::string& v) {
                e->setRutaPerfil(v);
            }),
    });

    // --- Materiales (materiales por region) -------------------------------
    registrar("Materiales", {
        propiedadEntera<Materiales>(
            "selectedRegion", "Region seleccionada (-1 = ninguna)",
            [](Materiales* m) { return m->regionSeleccionada(); },
            [](Materiales* m, int v) { m->setRegionSeleccionada(v); }),
    });

    // --- SpriteRenderer --------------------------------------------------
    registrar("SpriteRenderer", {
        propiedadTexto<SpriteRenderer>(
            "texturePath", "Textura",
            [](SpriteRenderer* s) { return s->getRutaTextura(); },
            [](SpriteRenderer* s, const std::string& v) {
                s->setRutaTextura(v);
            }),
        propiedadVec4<SpriteRenderer>(
            "tint", "Tinte",
            [](SpriteRenderer* s) {
                const float* v = s->getTint();
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](SpriteRenderer* s, float r, float g, float b, float a) {
                s->setTint(r, g, b, a);
            }),
        propiedadVec2<SpriteRenderer>(
            "size", "Tamano en unidades de mundo",
            [](SpriteRenderer* s) {
                const float* v = s->getTamano();
                return std::array<float, 2>{v[0], v[1]};
            },
            [](SpriteRenderer* s, float w, float h) { s->setTamano(w, h); }),
        propiedadVec4<SpriteRenderer>(
            "uvRect", "Rectangulo UV",
            [](SpriteRenderer* s) {
                const float* v = s->getUvRect();
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](SpriteRenderer* s, float u0, float v0, float u1, float v1) {
                s->setUvRect(u0, v0, u1, v1);
            }),
    });

    // --- Animador2D ------------------------------------------------------
    registrar("Animador2D", {
        propiedadEntera<Animador2D>(
            "columns", "Columnas",
            [](Animador2D* a) { return a->getColumnas(); },
            [](Animador2D* a, int v) { a->setColumnas(v); }),
        propiedadEntera<Animador2D>(
            "rows", "Filas",
            [](Animador2D* a) { return a->getFilas(); },
            [](Animador2D* a, int v) { a->setFilas(v); }),
        propiedadEntera<Animador2D>(
            "firstFrame", "Primer frame",
            [](Animador2D* a) { return a->getPrimerFrame(); },
            [](Animador2D* a, int v) { a->setPrimerFrame(v); }),
        propiedadEntera<Animador2D>(
            "lastFrame", "Ultimo frame (-1 = el ultimo)",
            [](Animador2D* a) { return a->getUltimoFrame(); },
            [](Animador2D* a, int v) { a->setUltimoFrame(v); }),
        propiedadFlotante<Animador2D>(
            "fps", "Fotogramas por segundo",
            [](Animador2D* a) { return a->getFps(); },
            [](Animador2D* a, float v) { a->setFps(v); }),
        propiedadBooleana<Animador2D>(
            "loop", "En bucle",
            [](Animador2D* a) { return a->getBucle(); },
            [](Animador2D* a, bool v) { a->setBucle(v); }),
        propiedadBooleana<Animador2D>(
            "playing", "Reproduciendo",
            [](Animador2D* a) { return a->getReproduciendo(); },
            [](Animador2D* a, bool v) { a->setReproduciendo(v); }),
    });

    // --- Model -----------------------------------------------------------
    registrar("Model", {
        propiedadTexto<Model>(
            "path", "Ruta del modelo",
            [](Model* m) { return m->getPath(); },
            [](Model* m, const std::string& v) { m->setPath(v); }),
    });

    // --- Tilemap ---------------------------------------------------------
    registrar("Tilemap", {
        propiedadTexto<Tilemap>(
            "tilesetPath", "Tileset",
            [](Tilemap* t) { return t->getRutaTileset(); },
            [](Tilemap* t, const std::string& v) { t->setRutaTileset(v); }),
    });
}

}  // namespace MotorInspeccion
