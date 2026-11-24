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
#include "SceneRendererDetalle.h"

// Recta guia del objeto seleccionado (teclas X/Y/Z): la recta sobre la que
void SceneRenderer::dibujarGuiaEje(const FrameContext& ctx,
                                   const float camaraMundo[3]) {
    if (ctx.guiaEje < GuiaEje::kEjeX || ctx.guiaEje > GuiaEje::kEjeZ) return;
    GameObject* object = ctx.selectedObject;
    if (!object) return;

    Transform* transform = object->getGlobalTransform();
    if (!transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    GuiaEje::Eje eje;
    if (!GuiaEje::calcularEje(modelArr, ctx.guiaEje,
                              ctx.guiaCoordenadasGlobales, &eje))
        return;

    // El color del eje se mide contra el color de la grilla (misma regla que
    // sus ejes): si no, la guia se pierde sobre una grilla de su mismo color.
    float color[4];
    float referencia[3];
    if (colorReferenciaGuia(ctx, referencia)) {
        GuiaEje::colorEfectivo(ctx.guiaEje, referencia, color);
    } else {
        GuiaEje::colorEje(ctx.guiaEje, color);
    }
    // Mismo radio que la grilla (radioDifuminado lee el perfil de apariencia),
    // asi que guia y piso se desvanecen siempre en el mismo circulo.
    // Mas subdivisiones que la grilla: la guia es mucho mas larga que una linea
    // de la grilla, asi que con los 6 trozos de aquella el degradado se veria
    // escalonado a lo largo de toda la recta.
    const Difuminado dif = Difuminado::desdeRadio(
        radioDifuminado(ctx), 24);

    LineBuilder builder;
    GuiaEje::emitir(builder, eje, camaraMundo, color, dif);
    // Geometria ya en mundo (la recta sale de la matriz global): model = null.
    lineRenderer().dibujar(builder, marcadoresBatch_, nullptr, 3.0f);
}

void SceneRenderer::prepararLucesFrame(const FrameContext& ctx) {
    if (!meshRenderer_) return;
    meshRenderer_->setLuces(ctx.lights, ctx.lightCount, ctx.globalAmbient);
}

// Pasada de sombras: la logica vive en Rendering::PasadaSombras; aqui solo se
// arma su entrada con las luces del frame y la camara activa.
void SceneRenderer::prepararSombras(const FrameContext& ctx,
                                    CameraComponent* camara) {
    if (!meshRenderer_) return;

    Rendering::PasadaSombras::Entrada entrada;
    entrada.luces = ctx.lights;
    entrada.duenos = ctx.lightOwners;
    entrada.cantidadLuces = ctx.lightCount;
    entrada.objetos = ctx.gameObjects;
    entrada.framebufferAncho = ctx.framebufferWidth;
    entrada.framebufferAlto = ctx.framebufferHeight;
    if (camara) {
        const float* pos = camara->getPosition();
        if (pos) {
            entrada.hayCamara = true;
            entrada.posicionCamara = glm::vec3(pos[0], pos[1], pos[2]);
            // El frente de la camara es -Z de su vista (tercera fila de la
            // rotacion de la matriz de vista).
            float vista[16];
            camara->getViewMatrix(vista);
            const glm::vec3 frente(-vista[2], -vista[6], -vista[10]);
            if (glm::length(frente) > 1e-6f) entrada.frenteCamara = glm::normalize(frente);
        }
    }
    pasadaSombras_.ejecutar(entrada, *meshRenderer_);
}

void SceneRenderer::dibujarGameObjectsConOjo(const FrameContext& ctx,
                                             GameObject* camaraOjo,
                                             const float view[16],
                                             const float projection[16]) {
    auto* gameObjects = ctx.gameObjects;
    if (!gameObjects || gameObjects->isEmpty()) return;
    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        dibujarObjectConOjo(ctx, pos->getElement(), camaraOjo, view, projection);
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}

void SceneRenderer::dibujarObjectConOjo(const FrameContext& ctx,
                                        GameObject* object,
                                        GameObject* camaraOjo,
                                        const float view[16],
                                        const float projection[16]) {
    object->setTam(10);
    object->setColor(object->auxColor);

    if (static_cast<Transform*>(object->getComponentByName("Transform"))) {
        // El dibujado va siempre por el pipeline moderno (MeshRenderer: VBO/VAO
        // + shader). Si el objeto no tiene malla con normales, simplemente no
        // se dibuja (MeshRenderer lo avisa una vez por malla).
        // Todos los objetos con malla usan el componente Model; la malla se
        // resuelve via resolverDibujoModelo (que usa getGlobalTransform para
        // respetar la jerarquia padre/hijo).
        if (object->getComponent<Model>() && meshRenderer_ && ctx.assetManager) {
            const DibujoModelo dibujo =
                resolverDibujoModelo(object, *ctx.assetManager);
            if (dibujo.valido) {
                meshRenderer_->intentarRenderMesh(
                    object, dibujo.malla, dibujo.modelo, view, projection,
                    ctx.deltaTime);
            }
        }
    }

    if (ctx.mostrarVisualesDepuracion && object->getComponent<Light>())
        dibujarMarcadorLuz(object);

    if (ctx.mostrarVisualesDepuracion &&
        object->getComponent<CameraComponent>() && object != camaraOjo)
        dibujarMarcadorCamara(object);

    // La visibilidad persistente es independiente del gizmo de edicion del
    // offset, que solo dibuja el collider del objeto seleccionado.
    if (ctx.mostrarVisualesDepuracion && object != camaraOjo) {
        Collider* collider = object->getComponent<Collider>();
        Transform* colliderTransform =
            collider ? collider->getTransform() : nullptr;

        const bool gizmoSeleccionado =
            collider && colliderTransform && ctx.selectedObject == object &&
            collider->getOwner() == ctx.selectedObject &&
            colliderTransform->gizmoHabilitado;
        if (collider &&
            (collider->estaVisibleEnEscena() || gizmoSeleccionado)) {
            collider->dibujarCollider();
        }
    }
}

void SceneRenderer::dibujarParticulas(const FrameContext& ctx, const float view[16],
                                    const float projection[16]) {
    if (!ctx.sistemaParticulas) return;
    if (ctx.sistemaParticulas->particulas().empty()) return;
    particulasRenderer_.dibujar(*ctx.sistemaParticulas, view, projection);
}

void SceneRenderer::dibujarSprites(const FrameContext& ctx, const float view[16],
                                   const float projection[16]) {
    auto* gameObjects = ctx.gameObjects;
    if (!gameObjects || gameObjects->isEmpty()) return;

    struct Entrada {
        SpriteRenderer* sprite;
        float z;
        float model[16];
    };
    std::vector<Entrada> entradas;

    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        GameObject* object = pos->getElement();
        SpriteRenderer* sprite = object->getComponent<SpriteRenderer>();
        if (sprite) {
            Transform* transform = object->getGlobalTransform();
            if (transform) {
                Entrada entrada;
                entrada.sprite = sprite;
                buildMatrixFromTransform(transform, entrada.model);
                // La traslacion en Z de la matriz mundo (indice 14, columna 3
                // del layout col-major) es la clave de orden: el sprite no
                // tiene volumen propio y su orden en pantalla lo decide el Z
                // del Transform.
                entrada.z = entrada.model[14];
                entradas.push_back(entrada);
            }
        }
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
    if (entradas.empty()) return;

    // Atras->adelante: el primero en dibujarse queda debajo. Con depth mask off
    // en la pasada, el que se dibuja despues siempre gana el pixel, asi que
    // este orden define el solapamiento entre sprites.
    std::sort(entradas.begin(), entradas.end(),
              [](const Entrada& a, const Entrada& b) { return a.z < b.z; });

    for (const Entrada& e : entradas) {
        const std::string& ruta = e.sprite->getRutaTextura();
        Rendering::SpriteDraw draw;
        draw.model = e.model;
        draw.view = view;
        draw.projection = projection;
        draw.rutaTextura = ruta.empty() ? nullptr : ruta.c_str();
        draw.tint = e.sprite->getTint();
        draw.tamano = e.sprite->getTamano();
        draw.uvRect = e.sprite->getUvRect();
        spriteRenderer_.dibujar(draw);
    }
}

void SceneRenderer::dibujarTilemaps(const FrameContext& ctx,
                                    const float view[16],
                                    const float projection[16]) {
    if (!ctx.gameObjects) return;

    struct Entrada {
        Tilemap* tilemap = nullptr;
        float model[16] = {};
        float z = 0.0f;
        std::string ruta;
    };
    std::vector<Entrada> entradas;

    ListaDE<GameObject*>* gameObjects = ctx.gameObjects;
    if (!gameObjects || gameObjects->isEmpty()) return;

    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        GameObject* object = pos->getElement();
        Tilemap* tilemap = object->getComponent<Tilemap>();
        if (tilemap && !tilemap->mallaHorneada().isEmpty()) {
            Transform* transform = object->getGlobalTransform();
            if (transform) {
                Entrada entrada;
                entrada.tilemap = tilemap;
                buildMatrixFromTransform(transform, entrada.model);
                entrada.z = entrada.model[14];
                entrada.ruta = tilemap->rutaImagenTileset();
                entradas.push_back(std::move(entrada));
            }
        }
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
    if (entradas.empty()) return;

    std::sort(entradas.begin(), entradas.end(),
              [](const Entrada& a, const Entrada& b) { return a.z < b.z; });

    for (const Entrada& e : entradas) {
        Rendering::TilemapDraw draw;
        draw.id = e.tilemap;
        draw.revision = e.tilemap->revision();
        draw.malla = &e.tilemap->mallaHorneada();
        draw.rutaTextura = e.ruta.empty() ? nullptr : e.ruta.c_str();
        draw.model = e.model;
        draw.view = view;
        draw.projection = projection;
        draw.tint = nullptr;
        tilemapRenderer_.dibujar(draw);
    }
}
