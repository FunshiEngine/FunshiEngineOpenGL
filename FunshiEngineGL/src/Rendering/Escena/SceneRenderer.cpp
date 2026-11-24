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

SceneRenderer::SceneRenderer(Rendering::Backend::IRenderBackend& backend)
    : backend_(backend),
      meshRenderer_(std::make_unique<MeshRenderer>()),
      grillaRenderer_(backend),
      pasadaSombras_(backend),
      marcadoresBatch_(backend) {
    meshRenderer_->setBackend(backend);
    spriteRenderer_.setBackend(backend);
    tilemapRenderer_.setBackend(backend);
    particulasRenderer_.setBackend(&backend);
}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::setTextureManager(TextureManager* textureManager) noexcept {
    if (meshRenderer_) meshRenderer_->setTextureManager(textureManager);
    spriteRenderer_.setTextureManager(textureManager);
    tilemapRenderer_.setTextureManager(textureManager);
    particulasRenderer_.setTextureManager(textureManager);
}

void SceneRenderer::setResolvedorMateriales(
    IResolutorMateriales* resolvedor) noexcept {
    if (meshRenderer_) meshRenderer_->setResolvedorMateriales(resolvedor);
}

void SceneRenderer::setGestorShaders(Rendering::GestorShaders* gestorShaders) noexcept {
    gestorShaders_ = gestorShaders;
    if (meshRenderer_) meshRenderer_->setGestorShaders(gestorShaders);
    // El renderer de lineas es un unico para el motor (los gizmos de colision
    // lo usan sin recibirlo por parametro).
    lineRenderer().setGestorShaders(gestorShaders);
    spriteRenderer_.setGestorShaders(gestorShaders);
    tilemapRenderer_.setGestorShaders(gestorShaders);
    particulasRenderer_.setGestorShaders(gestorShaders);
}

void SceneRenderer::destruir() {
    grillaRenderer_.destruir();
    // Recursos GPU del Skybox cacheados (malla del cubo y textura del cubemap).
    // Se libera aca con el contexto vivo y se vacia la clave, para que si la
    // escena vuelve a dibujar la pasada suba el cubemap de nuevo en lugar de
    // dar por buena una textura ya borrada.
    auto& backend = backend_;
    if (skyboxCubemap_ != Rendering::Backend::kInvalidHandle) {
        backend.destroyTextureCube(skyboxCubemap_);
        skyboxCubemap_ = Rendering::Backend::kInvalidHandle;
    }
    if (skyboxCuboMalla_ != Rendering::Backend::kInvalidHandle) {
        backend.destroyMesh(skyboxCuboMalla_);
        skyboxCuboMalla_ = Rendering::Backend::kInvalidHandle;
    }
    skyboxClave_.clear();
    // Malla, shader y texturas del renderer de sprites (mismo criterio: el
    // contexto GL todavia vive).
    spriteRenderer_.destruir();
    particulasRenderer_.destruir();
    // Mallas horneadas por tilemap, su shader y sus texturas de atlas.
    tilemapRenderer_.destruir();
}

// ---------------------------------------------------------------------------
// Pasada principal
// ---------------------------------------------------------------------------

void SceneRenderer::render(const FrameContext& ctx, GameObject* activeCameraObject,
                           CameraComponent* camara) {
    // Sombras: el mapa se calcula una vez por frame y lo usan todas las
    // pasadas (el mapa depende de la luz, no de la camara). Va antes que las
    // vistas previas para que estas tambien vean la sombra.
    prepararSombras(ctx, camara);

    dibujarViewportsPrevios(ctx);

    auto& backend = backend_;

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
            static_cast<float>(ctx.framebufferHeight),
        radioDifuminado(ctx));

    dibujarEscena(ctx, view, projection, activeCameraObject, ctx.framebufferWidth,
                  ctx.framebufferHeight, true);
}

// ---------------------------------------------------------------------------
// Dibujo de la escena (grilla + luces + objetos) desde una vista/proyeccion.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarEscena(const FrameContext& ctx,
                                  const float view[16],
                                  const float projection[16],
                                  GameObject* camaraOjo, int viewportAncho,
                                  int viewportAlto, bool esPasadaPrincipal) {
    auto& backend = backend_;

    // Estado de la pasada de lineas: el shader de lineas grosses necesita las
    // matrices de la camara y el tamano del viewport para pasar el ancho de
    // pixeles a NDC. Se fija aca porque TODA pasada (principal y vistas previas)
    // entra por esta funcion.
    lineRenderer().setVista(view, projection);
    lineRenderer().setViewport(viewportAncho, viewportAlto);
    const Difuminado horizonte =
        Difuminado::desdeRadio(radioDifuminado(ctx), 1);
    if (ctx.modo2D) {
        // En 2D la vista es ortografica y mira el plano XY de frente: no hay
        // horizonte de piso que difuminar. Se fija el rango completo para que
        // la grilla/lineas no se apaguen por distancia.
        lineRenderer().setHorizonteVisual(0.0f, 1.0e9f);
    } else {
        lineRenderer().setHorizonteVisual(horizonte.inicio, horizonte.fin);
    }

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

    // Cielo degradado: primera pasada, con depth test on + depth mask off para
    // que quede "detras" de todo sin escribir en el z-buffer. Recibe la
    // posicion de camara en el mundo (ya calculada arriba) porque el color del
    // cielo sale de la direccion de vista, no de la posicion del pixel. En 2D
    // se omite: el fondo del plano de juego es el color de limpieza.
    if (!ctx.modo2D) dibujarCielo(ctx, view, projection, camaraMundo);

    // La grilla se dibuja como una pasada independiente del renderer de
    // modelos: no depende de Modelos3D ni del recorrido normal de las
    // entidades.
    dibujarGrillaEditor(ctx, camaraMundo);
    if (meshRenderer_) {
        if (ctx.modo2D) {
            meshRenderer_->setHorizonteVisual(0.0f, 1.0e9f);
        } else {
            meshRenderer_->setHorizonteVisual(horizonte.inicio, horizonte.fin);
        }
    }

    // Luces de la pasada: van como uniforms del shader (MeshRenderer), que es
    // la unica via de iluminacion que queda.
    prepararLucesFrame(ctx);

    dibujarGameObjectsConOjo(ctx, camaraOjo, view, projection);

    // Tilemaps 2D: fondo del plano de juego; van antes que los sprites para
    // que estos queden por encima.
    dibujarTilemaps(ctx, view, projection);

    // Sprites 2D: despues de las mallas (que ya escribieron profundidad) y
    // antes de la guia de eje, para que la recta del editor quede por encima.
    dibujarSprites(ctx, view, projection);

    // Particululas: despues de los sprites, para que el humo y las chispas
    // queden por encima del plano de juego.
    dibujarParticulas(ctx, view, projection);

    // Guia de eje: despues de los objetos para que la recta se vea por encima
    // de la malla, y solo en la pasada principal.
    if (esPasadaPrincipal) dibujarGuiaEje(ctx, camaraMundo);

    ShaderProgram::unbind(backend);
}
