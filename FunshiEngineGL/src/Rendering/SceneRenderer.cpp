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
#include "SceneRenderer.h"

#include <cmath>
#include <iostream>

#include <imgui.h>

#include "Backend/IRenderBackend.h"
#include "LineBatch.h"
#include "LineBuilder.h"
#include "LineRenderer.h"
#include "MeshRenderer.h"
#include "RenderTarget.h"
#include "Shaders/ShaderProgram.h"

#include "../Configuracion/Apariencia.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Iluminacion/LightSystem.h"
#include "../Objetos/Componentes/CameraComponent.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Objetos/Componentes/Grid.h"
#include "../Objetos/Componentes/Light.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Modelos3D.h"

SceneRenderer::SceneRenderer() : meshRenderer_(std::make_unique<MeshRenderer>()) {}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::setTextureManager(TextureManager* textureManager) noexcept {
    if (meshRenderer_) meshRenderer_->setTextureManager(textureManager);
}

void SceneRenderer::destruir() { grillaRenderer_.destruir(); }

// ---------------------------------------------------------------------------
// Pasada principal
// ---------------------------------------------------------------------------

void SceneRenderer::render(const FrameContext& ctx, GameObject* activeCameraObject,
                           CameraComponent* camara) {
    dibujarViewportsPrevios(ctx);

    auto& backend = Rendering::Backend::activeBackend();

    // Pass principal: vuelve al framebuffer de la ventana con su viewport.
    backend.bindDefaultFramebuffer();
    if (ctx.framebufferWidth <= 0 || ctx.framebufferHeight <= 0) {
        viewportsCamaras_.clear();
        viewportsObjetos_.clear();
        return;
    }
    backend.setViewport(0, 0, ctx.framebufferWidth, ctx.framebufferHeight);

    float view[16], projection[16];
    camara->getViewMatrix(view);
    camara->getProjectionMatrix(
        projection,
        static_cast<float>(ctx.framebufferWidth) /
            static_cast<float>(ctx.framebufferHeight));

    static bool diagMatricesPendiente = true;
    if (diagMatricesPendiente) {
        diagMatricesPendiente = false;
        bool noFinita = false;
        for (int i = 0; i < 16; ++i) {
            if (!std::isfinite(view[i]) || !std::isfinite(projection[i])) {
                noFinita = true;
                break;
            }
        }
        const float* diagPos = camara->getPosition();
        const float fwd[3] = {-view[2], -view[6], -view[10]};
        std::cout << "[diag] matrices camara finitas: "
                  << (noFinita ? "NO (NaN/Inf)" : "si") << "; pos camara = ("
                  << diagPos[0] << ", " << diagPos[1] << ", " << diagPos[2]
                  << ") fwd=(" << fwd[0] << ", " << fwd[1] << ", " << fwd[2]
                  << ") fov=" << camara->getFov()
                  << " near=" << camara->getNearPlane()
                  << " far=" << camara->getFarPlane() << std::endl;

        // Inventario de la escena: cuantas luces y cuantas mallas dibujables
        // (con normales) hay, para contrastar con lo que se ve en pantalla.
        std::cout << "[diag] luces_en_escena=";
        auto* diagObjects = ctx.gameObjects;
        int diagLuces = 0;
        int diagObjs = 0;
        int diagConMalla = 0;
        int diagConMallaYNormales = 0;
        if (diagObjects && !diagObjects->isEmpty()) {
            Position<GameObject*>* pos = diagObjects->first();
            while (pos && pos->getElement()) {
                GameObject* o = pos->getElement();
                ++diagObjs;
                if (o->getComponent<Light>()) ++diagLuces;
                auto* m = dynamic_cast<Modelos3D*>(o);
                if (m && m->getMesh() && !m->getMesh()->isEmpty()) {
                    ++diagConMalla;
                    if (m->getMesh()->hasNormals()) ++diagConMallaYNormales;
                }
                pos = (pos != diagObjects->last()) ? diagObjects->next(pos)
                                                   : nullptr;
            }
        }
        std::cout << diagLuces << " objetos=" << diagObjs
                  << " conMalla=" << diagConMalla
                  << " conMallaYNormales=" << diagConMallaYNormales
                  << std::endl;
    }

    dibujarEscena(ctx, view, projection, activeCameraObject, ctx.framebufferWidth,
                  ctx.framebufferHeight, true);

    static bool diagPostPassPendiente = true;
    if (diagPostPassPendiente) {
        diagPostPassPendiente = false;
        std::cout << "[diag] MeshRenderer moderno disponible="
                  << (meshRenderer_ && meshRenderer_->available() ? "si" : "no")
                  << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Dibujo de la escena (grilla + luces + objetos) desde una vista/proyeccion.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarEscena(const FrameContext& ctx,
                                  const float view[16],
                                  const float projection[16],
                                  GameObject* camaraOjo, int viewportAncho,
                                  int viewportAlto, bool esPasadaPrincipal) {
    auto& backend = Rendering::Backend::activeBackend();

    // Estado de la pasada de lineas: el shader de lineas grosses necesita las
    // matrices de la camara y el tamano del viewport para pasar el ancho de
    // pixeles a NDC. Se fija aca porque TODA pasada (principal y vistas previas)
    // entra por esta funcion.
    lineRenderer().setVista(view, projection);
    lineRenderer().setViewport(viewportAncho, viewportAlto);

    // Posicion de la camara en el mundo a partir de su matriz de vista:
    // view = [R | t] (column-major), ojo = -(R^T * t). La usa la grilla para
    // extender el plano hasta el horizonte y difuminarlo por distancia.
    float camaraMundo[3];
    camaraMundo[0] =
        -(view[0] * view[12] + view[1] * view[13] + view[2] * view[14]);
    camaraMundo[1] =
        -(view[4] * view[12] + view[5] * view[13] + view[6] * view[14]);
    camaraMundo[2] =
        -(view[8] * view[12] + view[9] * view[13] + view[10] * view[14]);

    // La grilla se dibuja como una pasada independiente del renderer de
    // modelos: no depende de Modelos3D ni del recorrido normal de las
    // entidades.
    dibujarGrillaEditor(ctx, camaraMundo);

    // Luces de la pasada: van como uniforms del shader (MeshRenderer), que es
    // la unica via de iluminacion que queda.
    prepararLucesFrame(ctx);

    dibujarGameObjectsConOjo(ctx, camaraOjo, view, projection);

    // Guia de eje: despues de los objetos para que la recta se vea por encima
    // de la malla, y solo en la pasada principal.
    if (esPasadaPrincipal) dibujarGuiaEje(ctx, camaraMundo);

    ShaderProgram::unbind();
}

// Recta guia del objeto seleccionado (teclas X/Y/Z): la recta sobre la que
// puede moverse, tomando el eje pulsado como variable y fijando las otras dos
// coordenadas a las del objeto. Va hasta el horizonte y se difumina con el
// MISMO criterio radial que la grilla (mismas constantes), asi que las dos se
// desvanecen en el mismo punto y la guia se lee como un eje que cruza el piso.
// Se dibuja con el batch de los marcadores (un solo draw) y el color del eje.
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

    float color[4];
    GuiaEje::colorEje(ctx.guiaEje, color);
    GuiaEje::Difuminado dif;
    dif.inicio = GrillaRenderer::kFadeInicio;
    dif.fin = GrillaRenderer::kFadeFin;
    // Mas subdivisiones que la grilla: la guia es mucho mas larga que una linea
    // de la grilla, asi que con los 6 trozos de aquella el degradado se veria
    // escalonado a lo largo de los 300 unidades.
    dif.subdivisiones = 24;

    LineBuilder builder;
    GuiaEje::emitir(builder, eje, camaraMundo, color, dif);
    // Geometria ya en mundo (la recta sale de la matriz global): model = null.
    lineRenderer().dibujar(builder, marcadoresBatch_, nullptr, 3.0f);
}

void SceneRenderer::prepararLucesFrame(const FrameContext& ctx) {
    if (!meshRenderer_) return;
    meshRenderer_->setLuces(ctx.lights, ctx.lightCount, ctx.globalAmbient);
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

    if (object->getComponent<Transform>()) {
        // El dibujado va siempre por el pipeline moderno (MeshRenderer: VBO/VAO
        // + shader). Si el objeto no tiene malla con normales, simplemente no
        // se dibuja (MeshRenderer lo avisa una vez por malla).
        auto* modelo = dynamic_cast<Modelos3D*>(object);
        if (modelo && meshRenderer_) {
            meshRenderer_->intentarRender(modelo, view, projection,
                                          ctx.deltaTime);
        }
    }

    if (object->getComponent<Light>()) dibujarMarcadorLuz(object);

    if (object->getComponent<CameraComponent>() && object != camaraOjo)
        dibujarMarcadorCamara(object);

    // Wireframe del collider en la escena 3D: SOLO mientras el gizmo del
    // offset del collider esta habilitado para este objeto (checkbox "Gizmo
    // activo" del transform del collider).
    if (ctx.editorActivo && object != camaraOjo && ctx.selectedObject) {
        Collider* collider = object->getComponent<Collider>();
        Transform* colliderTransform =
            collider ? collider->getTransform() : nullptr;

        if (collider && colliderTransform &&
            colliderTransform->gizmoHabilitado &&
            collider->getOwner() == ctx.selectedObject) {
            collider->dibujarCollider();
        }
    }
}

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
    const float halfHNear = tanHalf * nearDist;
    const float halfWNear = halfHNear * aspect;
    const float halfHFar = tanHalf * farDist;
    const float halfWFar = halfHFar * aspect;

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
    grillaRenderer_.dibujar(modelArr, colorGrilla, camaraMundo);
}

// ---------------------------------------------------------------------------
// Pasada de vista previa (Fase 2): por cada camara con "Vista previa" activo
// se pinta la escena a una textura FBO que luego muestra una ventana ImGui.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarViewportsPrevios(const FrameContext& ctx) {
    auto* gameObjects = ctx.gameObjects;
    std::vector<std::unique_ptr<RenderTarget>> nuevos;
    std::vector<GameObject*> nuevosObjetos;
    if (gameObjects && !gameObjects->isEmpty()) {
        Position<GameObject*>* pos = gameObjects->first();
        while (pos && pos->getElement()) {
            GameObject* objeto = pos->getElement();
            CameraComponent* camara = objeto->getComponent<CameraComponent>();
            if (camara && camara->getPintar()) {
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
                if (!target) target = std::make_unique<RenderTarget>();

                target->resize(kPreviewW, kPreviewH);
                target->bind();

                auto& backend = Rendering::Backend::activeBackend();
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
                    static_cast<float>(kPreviewW) /
                        static_cast<float>(kPreviewH));
                // La vista previa de camara NO muestra la guia de eje: es una
                // ayuda del editor sobre la pasada principal (en el preview
                // ocuparia la imagen sin que el usuario la haya pedido).
                dibujarEscena(ctx, view, projection, objeto, kPreviewW,
                              kPreviewH, false);

                backend.bindDefaultFramebuffer();

                nuevos.push_back(std::move(target));
                nuevosObjetos.push_back(objeto);
            }
            pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                               : nullptr;
        }
    }
    viewportsCamaras_ = std::move(nuevos);
    viewportsObjetos_ = std::move(nuevosObjetos);
}
