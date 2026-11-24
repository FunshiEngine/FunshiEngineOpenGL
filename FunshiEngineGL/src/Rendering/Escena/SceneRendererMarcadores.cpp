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

// Gizmo visual de una luz: un octaedro alambre amarillo en la posicion del
// objeto, para poder ubicar y seleccionar luces que no tienen cuerpo.
void SceneRenderer::dibujarMarcadorLuz(GameObject* object) {
    Transform* transform = object->getGlobalTransform();
    if (!transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    const float size = 0.5f;
    const float v[6][3] = {
        { 1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f},
        { 0.f, 1.f, 0.f}, { 0.f,-1.f, 0.f},
        { 0.f, 0.f, 1.f}, { 0.f, 0.f,-1.f}};
    const int edges[12][2] = {
        {0,2},{0,3},{0,4},{0,5},
        {1,2},{1,3},{1,4},{1,5},
        {2,4},{2,5},{3,4},{3,5}};

    // Los vertices se escalan (octaedro chico) y el dibujo lo hace la capa de
    // Rendering con el batch de lineas.
    float vsize[6][3];
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j) vsize[i][j] = v[i][j] * size;

    const float color[4] = {1.f, 0.85f, 0.1f, 1.f};
    LineBuilder builder;
    builder.agregarAristas(&vsize[0][0], 6, &edges[0][0], 12, color);
    lineRenderer().dibujar(builder, marcadoresBatch_, modelArr, 2.0f);
}

// Gizmo visual de una camara secundaria: frustum de vision alambre cian. La
// camara activa no dibuja el suyo (seria visera en la propia vista).
void SceneRenderer::dibujarMarcadorCamara(GameObject* object) {
    CameraComponent* camara = object->getComponent<CameraComponent>();
    Transform* transform = object->getGlobalTransform();
    if (!camara || !transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    ImGuiIO& io = ImGui::GetIO();
    const float aspect = (io.DisplaySize.x > 0.f && io.DisplaySize.y > 0.f)
                             ? io.DisplaySize.x / io.DisplaySize.y
                             : 1.77f;

    const float tanHalf =
        std::tan(camara->getFov() * 0.5f * 3.14159265358979f / 180.f);
    const float nearDist = camara->getNearPlane();
    const float farDist = camara->getFarPlane();
    // En perspectiva el frustum se abre con la distancia (escalado por tanHalf);
    // en ortografica es una caja de media altura orthoSize constante.
    const bool ortografica =
        camara->getProjection() == CameraComponent::Projection::Orthographic;
    const float halfHNear = ortografica ? camara->getOrthoSize()
                                        : tanHalf * nearDist;
    const float halfWNear = ortografica ? camara->getOrthoSize() * aspect
                                        : halfHNear * aspect;
    const float halfHFar = ortografica ? camara->getOrthoSize()
                                       : tanHalf * farDist;
    const float halfWFar = ortografica ? camara->getOrthoSize() * aspect
                                       : halfHFar * aspect;

    // Frustum: 4 esquinas del plano near (0..3) + 4 del far (4..7). Los 12
    // bordes de "edges" indexan los 8 puntitos, por eso todo vive en un solo
    // array (antes far y near estaban separados y se leia fuera de rango).
    const float vFrustum[8][3] = {
        {-halfWNear, -halfHNear, -nearDist},
        { halfWNear, -halfHNear, -nearDist},
        {-halfWNear,  halfHNear, -nearDist},
        { halfWNear,  halfHNear, -nearDist},
        {-halfWFar,  -halfHFar,  -farDist},
        { halfWFar,  -halfHFar,  -farDist},
        {-halfWFar,   halfHFar,  -farDist},
        { halfWFar,   halfHFar,  -farDist}};
    const int edges[12][2] = {
        {0,1},{0,2},{3,1},{3,2},
        {4,5},{4,6},{7,5},{7,6},
        {0,4},{1,5},{2,6},{3,7}};

    const float color[4] = {0.3f, 0.8f, 0.9f, 1.f};
    LineBuilder builder;
    builder.agregarAristas(&vFrustum[0][0], 8, &edges[0][0], 12, color);
    lineRenderer().dibujar(builder, marcadoresBatch_, modelArr, 2.0f);
}

void SceneRenderer::dibujarGrillaEditor(const FrameContext& ctx,
                                        const float camaraMundo[3]) {
    if (!ctx.mostrarVisualesDepuracion) return;
    auto* gameObjects = ctx.gameObjects;

    if (!gameObjects || gameObjects->isEmpty()) return;

    Position<GameObject*>* pos = gameObjects->first();

    while (pos && pos->getElement()) {
        GameObject* object = pos->getElement();

        if (object->getComponent<Grid>() != nullptr) {
            dibujarGrilla(ctx, object, camaraMundo);
            return;
        }

        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}

void SceneRenderer::dibujarGrilla(const FrameContext& ctx, GameObject* object,
                                  const float camaraMundo[3]) {
    Grid* grid = object->getComponent<Grid>();
    Transform* transform = object->getGlobalTransform();
    if (!grid || !grid->getVisible() || !transform || !ctx.apariencia) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    // Color efectivo de la grilla segun el perfil de apariencia: en modo
    // blanco y negro se ignora el color del componente y se usa el contraste
    // puro (la geometria del frame se genera con el color efectivo).
    float colorGrilla[3];
    AparienciaUtil::grillaEfectiva(*ctx.apariencia, grid->getColor(),
                                   colorGrilla);

    // El dibujado (extent infinito del plano + difuminado del horizonte con
    // densidad fija + anchos) vive en la capa de Rendering; aqui se le pasa la
    // matriz del objeto "Grilla" y la posicion del ojo en el mundo.
    grillaRenderer_.dibujar(modelArr, colorGrilla, camaraMundo,
                            Difuminado::desdeRadio(radioDifuminado(ctx),
                                                   GrillaRenderer::kSubdivisiones));
}

float SceneRenderer::radioDifuminado(const FrameContext& ctx) {
    // Sin perfil de apariencia en la pasada (una vista previa, por ejemplo) se
    // usa el valor por defecto, que es el mismo que pone una configuracion
    // recien creada. El acotado al rango admitido lo hace Difuminado::desdeRadio.
    const float radio =
        ctx.apariencia ? ctx.apariencia->radioDifuminado
                       : AparienciaUtil::kRadioDifuminadoPorDefecto;
    return Difuminado::desdeRadio(radio, 1).fin;
}

bool SceneRenderer::colorReferenciaGuia(const FrameContext& ctx,
                                        float out[3]) const {
    if (!ctx.apariencia || !out) return false;

    // Mismo criterio que elegir la grilla que se dibuja: gana el PRIMER objeto
    // con componente Grid, y si ese no se ve no se dibuja ninguna (el fallback
    // al fondo es lo que le corresponde a la guia en ese caso).
    auto* objetos = ctx.gameObjects;
    if (objetos && !objetos->isEmpty()) {
        Position<GameObject*>* pos = objetos->first();
        while (pos && pos->getElement()) {
            GameObject* object = pos->getElement();
            Grid* grid = object->getComponent<Grid>();
            if (grid) {
                if (grid->getVisible() && object->getGlobalTransform()) {
                    AparienciaUtil::grillaEfectiva(*ctx.apariencia,
                                                   grid->getColor(), out);
                    return true;
                }
                break;
            }
            pos = objetos->last() == pos ? nullptr : objetos->next(pos);
        }
    }

    AparienciaUtil::fondoEfectivo(*ctx.apariencia, out);
    return true;
}

// ---------------------------------------------------------------------------
// Pasada de vista previa (Fase 2): por cada camara con "Vista previa" activo
// se pinta la escena a una textura FBO que luego muestra una ventana ImGui.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarViewportsPrevios(const FrameContext& ctx) {
    auto* gameObjects = ctx.gameObjects;
    std::vector<std::unique_ptr<RenderTarget>> nuevos;
    std::vector<GameObject*> nuevosObjetos;

    auto& backend = backend_;
    // Sin backend no hay nada que dibujar en la vista previa: se sale antes de
    // crear y enlazar ningun objetivo de render (enlazar uno sin backend
    // llamaria punteros de funcion nulos).
    if (!backend.available()) return;

    if (gameObjects && !gameObjects->isEmpty()) {
        // El avance del iterador va en la clausula de incremento del for: si
        // esta en el cuerpo, un `continue` puede saltarselo y el bucle queda
        // girando sobre el mismo elemento indefinidamente.
        for (Position<GameObject*>* pos = gameObjects->first();
             pos && pos->getElement();
             pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                                : nullptr) {
            GameObject* objeto = pos->getElement();
            CameraComponent* camara = objeto->getComponent<CameraComponent>();
            if (!camara || !camara->getPintar()) continue;

            camara->setUp(objeto);

            // Reutilizar el FBO del frame anterior del mismo objeto en
            // lugar de recrearlo (evita churn de texturas en el GPU).
            std::unique_ptr<RenderTarget> target;
            for (size_t i = 0; i < viewportsCamaras_.size(); ++i) {
                if (viewportsObjetos_[i] == objeto) {
                    target.reset(viewportsCamaras_[i].release());
                    break;
                }
            }
            if (!target) target = std::make_unique<RenderTarget>(backend_);

            target->resize(kPreviewW, kPreviewH);
            target->bind();

            backend.setViewport(0, 0, kPreviewW, kPreviewH);
            // El FBO hereda el estado GL; se fija el fondo del perfil para
            // que la vista previa use el mismo color que la pasada principal.
            float fondoPreview[3] = {0.f, 0.f, 0.f};
            if (ctx.apariencia)
                AparienciaUtil::fondoEfectivo(*ctx.apariencia, fondoPreview);
            backend.clearScreen(fondoPreview);

            float view[16], projection[16];
            camara->getViewMatrix(view);
            camara->getProjectionMatrix(
                projection,
                static_cast<float>(kPreviewW) / static_cast<float>(kPreviewH),
                radioDifuminado(ctx));
            // La vista previa de camara NO muestra la guia de eje: es una
            // ayuda del editor sobre la pasada principal (en el preview
            // ocuparia la imagen sin que el usuario la haya pedido).
            dibujarEscena(ctx, view, projection, objeto, kPreviewW, kPreviewH,
                          false);

            backend.bindDefaultFramebuffer();

            nuevos.push_back(std::move(target));
            nuevosObjetos.push_back(objeto);
        }
    }
    viewportsCamaras_ = std::move(nuevos);
    viewportsObjetos_ = std::move(nuevosObjetos);
}
