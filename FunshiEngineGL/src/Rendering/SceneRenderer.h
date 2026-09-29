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
#include <string>
#include <vector>

#include "Backend/IRenderBackend.h"
#include "Cielo.h"
#include "GrillaRenderer.h"
#include "GuiaEje.h"

// Clases de otras capas que la pasada de render necesita (solo data CPU).
class GameObject;
class CameraComponent;
class RenderTarget;
class TextureManager;
class ShaderProgram;
struct Apariencia;
struct LightData;
class Skybox;

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
        // Guia de eje activa: 0 = X, 1 = Y, 2 = Z, -1 = ninguna (GuiaEje.h).
        // Es la recta que el editor dibuja sobre el objeto seleccionado para
        // marcarle sobre que eje puede moverse. Solo se dibuja en la pasada
        // principal (las vistas previas de camara no la muestran).
        int guiaEje = GuiaEje::kSinGuia;
        // Sistema de referencia de la guia, sincronizado con el toggle
        // LOCAL/GLOBAL del gizmo (tecla G): en true la recta sigue el eje del
        // mundo, en false el eje local rotado del objeto.
        bool guiaCoordenadasGlobales = true;
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
                       const float projection[16], GameObject* camaraOjo,
                       int viewportAncho, int viewportAlto,
                       bool esPasadaPrincipal);
    void prepararLucesFrame(const FrameContext& ctx);
    void dibujarGameObjectsConOjo(const FrameContext& ctx, GameObject* camaraOjo,
                                  const float view[16],
                                  const float projection[16]);
    void dibujarObjectConOjo(const FrameContext& ctx, GameObject* object,
                             GameObject* camaraOjo, const float view[16],
                             const float projection[16]);
    void dibujarMarcadorLuz(GameObject* object);
    void dibujarMarcadorCamara(GameObject* object);
    // Cielo degradado (fullscreen triangle): se dibuja ANTES que la grilla y
    // los objetos, con depth test habilitado y depth mask deshabilitado, para
    // que quede "detras" de todo sin escribir profundidad. El color sale de la
    // direccion de vista de cada pixel, asi que el cielo acompanha a la camara
    // (mirar abajo da el color inferior, arriba el superior, y el horizonte
    // queda en la transicion) en vez de quedar clavado a la pantalla.
    void dibujarCielo(const FrameContext& ctx, const float view[16],
                      const float projection[16], const float camaraMundo[3]);
    // Skybox cubemap: se dibuja en lugar del degradado si hay un componente
    // Skybox visible con 6 caras validas. Mismo estado de depth que el cielo.
    //
    // Dibuja el cubemap del Skybox alrededor de la camara. Devuelve si se
    // dibujo de verdad: cuando no hay cubemap utilizable (falta una cara, no
    // decodifica, el backend no crea la textura o no hay malla) el llamador
    // tiene que caer al degradado. Antes devolvia void y el llamador hacia
    // return igual, con lo que esos casos dejaban el fondo en negro.
    bool dibujarSkyboxCubemap(const Skybox* skybox, const float view[16],
                              const float projection[16]);
    // Sube a GPU las 6 caras dadas y deja el handle en skyboxCubemap_. Devuelve
    // false si alguna cara no se pudo decodificar o la textura no se pudo crear;
    // en ese caso no queda nada cacheado y la pasada cae al degradado.
    bool cargarCubemap(const std::string rutas[6]);
    // Recta de la guia de eje (X/Y/Z) sobre el objeto seleccionado: va hasta el
    // horizonte con el difuminado de la grilla y el color del eje.
    void dibujarGuiaEje(const FrameContext& ctx, const float camaraMundo[3]);
    void dibujarGrillaEditor(const FrameContext& ctx,
                             const float camaraMundo[3]);
    void dibujarGrilla(const FrameContext& ctx, GameObject* object,
                       const float camaraMundo[3]);
    // Radio del difuminado del piso para este frame: el que eligio el usuario
    // en Opciones (Apariencia::radioDifuminado), acotado por el propio Difuminado
    // y con el valor por defecto si la pasada no trae perfil de apariencia. Lo
    // leen la grilla y la guia de eje para desvanecerse en el MISMO circulo.
    static float radioDifuminado(const FrameContext& ctx);
    // Color contra el que se mide el contraste de la guia de eje: el color
    // EFECTIVO de la grilla del suelo (el mismo que usa su propio dibujado, con
    // el modo blanco y negro ya resuelto) y, cuando no hay grilla visible, el
    // fondo del viewport. Devuelve false si la pasada no trae perfil de
    // apariencia.
    bool colorReferenciaGuia(const FrameContext& ctx, float out[3]) const;

    std::unique_ptr<class MeshRenderer> meshRenderer_;
    GrillaRenderer grillaRenderer_;
    // Programas del cielo: el del degradado (triangulo a pantalla completa) y el
    // del cubemap del componente Skybox. Se crean en la primera pasada que los
    // necesita y se reusan: ShaderProgram cachea las locations de los uniforms
    // (buscarlas por pasada era trabajo repetido) y libera el handle al
    // destruirse. El flag de fallo evita reintentar compilar un shader roto
    // frame a frame (el texto de origen no cambia, no puede pasar de rojo a
    // verde) y con el, tambien, repetir su mensaje de error.
    std::unique_ptr<ShaderProgram> cieloProgram_;
    bool cieloShaderFallado_ = false;
    std::unique_ptr<ShaderProgram> skyboxProgram_;
    bool skyboxShaderFallado_ = false;
    // Cubemap cacheado del componente Skybox: la textura GPU, la malla del cubo
    // y la clave (rutas + fechas de modificacion de las 6 caras) que valido la
    // textura. Se sube una sola vez por conjunto de caras y se reemplaza solo si
    // cambia algun archivo; la malla del cubo no depende de las caras y se crea
    // una sola vez. Se liberan en destruir().
    Rendering::Backend::Handle skyboxCubemap_ = Rendering::Backend::kInvalidHandle;
    Rendering::Backend::Handle skyboxCuboMalla_ = Rendering::Backend::kInvalidHandle;
    std::string skyboxClave_;
    // Batch de lineas compartido por los marcadores de luz y de camara (ambos
    // son 12 aristas): se sube y se dibuja por gizmo, en un solo draw cada uno.
    LineBatch marcadoresBatch_;
    // Vista previa viva por camara con el checkbox "Vista previa" (Fase 2).
    // Se reconstruye cada frame: texturas FBO + el objeto que las genera.
    std::vector<std::unique_ptr<RenderTarget>> viewportsCamaras_;
    std::vector<GameObject*> viewportsObjetos_;
    static constexpr int kPreviewW = 400;
    static constexpr int kPreviewH = 250;
};

#endif
