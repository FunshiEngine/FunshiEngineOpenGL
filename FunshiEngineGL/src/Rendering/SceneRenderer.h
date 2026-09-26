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
#ifndef SCENERENDERER_H
#define SCENERENDERER_H

#include <memory>
#include <vector>

#include "GrillaRenderer.h"

// Clases de otras capas que la pasada de render necesita (solo data CPU).
class GameObject;
class CameraComponent;
class RenderTarget;
class TextureManager;
struct Apariencia;
struct LightData;

template <typename T>
class ListaDE;

// Pasada de render de la escena 3D (Fase 3 del desacoplamiento grafico).
//
// Extraida de GameScene: concentra en Rendering TODO el pasaje de dibujado de
// la escena (vistas previas de camara, pasada principal, grilla, objetos,
// marcadores y el diag del frame) y consume la capa de entidades SOLO como
// data (GameObjects via un FrameContext por llamada). El renderer es dueno del
// MeshRenderer (pipeline moderno VBO/VAO + shader), de la grilla y de los
// RenderTarget utilizados por las vistas previas. Ningun GL vive aqui: todo
// pasa por IRenderBackend.
class SceneRenderer {
public:
    SceneRenderer();
    ~SceneRenderer();

    // Manager de imagenes compartidas: se inyecta al MeshRenderer para que
    // los materiales con textura se suban una sola vez a GPU.
    void setTextureManager(TextureManager* textureManager) noexcept;

    // Datos CPU de la pasada que el renderer necesita de la escena. Los punteros
    // apuntan a memoria del llamador y solo valen durante render().
    struct FrameContext {
        ListaDE<GameObject*>* gameObjects = nullptr;
        const Apariencia* apariencia = nullptr;
        float deltaTime = 0.0f;
        bool editorActivo = false;
        GameObject* selectedObject = nullptr;
        // Luces ya recogidas (LightSystem::collectLights) con el modelo global.
        const LightData* lights = nullptr;
        int lightCount = 0;
        const float* globalAmbient = nullptr;
        // Tamano del framebuffer de la ventana (pasada principal).
        int framebufferWidth = 0;
        int framebufferHeight = 0;
    };

    // Renderiza las vistas previas y la pasada principal de la escena desde la
    // camara activa, con 'activeCameraObject' como objeto que observa (no
    // dibuja su propio marcador).
    void render(const FrameContext& ctx, GameObject* activeCameraObject,
                CameraComponent* camara);

    // Resultado de la ultima pasada de vistas previas (textura FBO por camara
    // con "Vista previa" activo + el objeto que la genero). La GUI las muestra
    // como ImGui::Image.
    const std::vector<std::unique_ptr<RenderTarget>>&
    viewportTargets() const noexcept {
        return viewportsCamaras_;
    }
    const std::vector<GameObject*>& viewportObjects() const noexcept {
        return viewportsObjetos_;
    }

    // Descarta la geometria cachead en la grilla (higiene defensiva al
    // apagar la aplicacion: el contexto GL puede cerrarse antes que la escena).
    void destruir();

private:
    void dibujarViewportsPrevios(const FrameContext& ctx);
    void dibujarEscena(const FrameContext& ctx, const float view[16],
                       const float projection[16], GameObject* camaraOjo);
    void prepararLucesFrame(const FrameContext& ctx);
    void dibujarGameObjectsConOjo(const FrameContext& ctx, GameObject* camaraOjo,
                                  const float view[16],
                                  const float projection[16]);
    void dibujarObjectConOjo(const FrameContext& ctx, GameObject* object,
                             GameObject* camaraOjo, const float view[16],
                             const float projection[16]);
    void dibujarMarcadorLuz(GameObject* object);
    void dibujarMarcadorCamara(GameObject* object);
    void dibujarGrillaEditor(const FrameContext& ctx,
                             const float camaraMundo[3]);
    void dibujarGrilla(const FrameContext& ctx, GameObject* object,
                       const float camaraMundo[3]);

    std::unique_ptr<class MeshRenderer> meshRenderer_;
    GrillaRenderer grillaRenderer_;
    // Vista previa viva por camara con el checkbox "Vista previa" (Fase 2).
    // Se reconstruye cada frame: texturas FBO + el objeto que las genera.
    std::vector<std::unique_ptr<RenderTarget>> viewportsCamaras_;
    std::vector<GameObject*> viewportsObjetos_;
    static constexpr int kPreviewW = 400;
    static constexpr int kPreviewH = 250;
};

#endif