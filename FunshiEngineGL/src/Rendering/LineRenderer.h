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
#ifndef LINERENDERER_H
#define LINERENDERER_H

#include <memory>

#include <glm/glm.hpp>

class LineBatch;
class LineBuilder;
class ShaderProgram;

// Renderer de las lineas del engine con el pipeline moderno (VBO/VAO + shader
// de lineas gruesas): cubre la grilla, los marcadores de luz/camara y los
// wireframes de los colliders, y es la UNICA via de dibujo de lineas (el
// glBegin/glEnd + glLineWidth del modo inmediato quedo eliminado al migrar a
// OpenGL 3.3 core). El ancho se resuelve en pixeles dentro del shader, asi que
// las lineas se ven igual de gruesas a cualquier distancia (GL_LINES solo
// garantiza 1 px en un contexto core).
//
// El estado de la pasada (vista, proyeccion y viewport) lo fija una sola vez
// quien abre el paso 3D: SceneRenderer::dibujarEscena, que es el paso por el
// que entran tanto la pasada principal como cada vista previa. Asi dibujar()
// solo necesita la matriz del objeto y el ancho, y los consumidores
// (GrillaRenderer, los colliders, los marcadores) no arrastran las matrices de
// la camara en sus firmas.
//
// El gizmo de un collider se dibuja desde la GUI, fuera del paso 3D: reusa el
// estado de la ULTIMA pasada (la principal, que se dibuja justo antes que la
// GUI). Si todavia no corrio ninguna pasada, el dibujo es un no-op en vez de
// paintar geometria con matrices sin sentido.
//
// El batch se mantiene fuera del renderer: cada consumidor es dueño de su
// LineBuilder + LineBatch y los reutiliza frame a frame, de modo que por pase
// hay un solo draw por consumidor.
class LineRenderer {
public:
    LineRenderer();
    ~LineRenderer();

    bool available() const noexcept { return shader_ != nullptr; }

    // Matrices de vista y proyeccion del paso actual (las produce la camara).
    // Sin ellas no se dibuja nada.
    void setVista(const float view[16], const float projection[16]);

    // Tamano en pixeles del framebuffer donde se dibuja. El shader lo necesita
    // para convertir el ancho de linea de pixeles a NDC; hay que informarlo
    // cuando cambia el viewport (la pasada principal y cada vista previa).
    void setViewport(int width, int height) noexcept;

    // Dibuja un batch ya subido con el ancho dado en pixeles. "model" es la
    // matriz del objeto que aporta la geometria (la del objeto "Grilla", la del
    // collider) o null para geometria ya en mundo (marcadores). No-op si el
    // shader no esta disponible, falta el estado de la pasada o el batch esta
    // vacio.
    void dibujar(const LineBatch& batch, const float model[16],
                 float anchoPx);

    // Atajo para la geometria dinamica: sube el builder al batch (reutilizando
    // el recurso si ya habia uno) y lo dibuja. Es el camino de la grilla, los
    // marcadores y los wireframes de los colliders.
    void dibujar(LineBuilder& builder, LineBatch& batch,
                 const float model[16], float anchoPx);

    // Libera el shader (por ejemplo al reiniciar el contexto grafico).
    void reset();

private:
    std::unique_ptr<ShaderProgram> shader_;
    glm::mat4 view_ = glm::mat4(1.0f);
    glm::mat4 projection_ = glm::mat4(1.0f);
    bool vistaValida_ = false;
    int viewportWidth_ = 0;
    int viewportHeight_ = 0;

    // Compila el shader de lineas la primera vez. No lanza: si el pipeline
    // moderno no esta disponible, deja available() == false.
    bool inicializar();
    // Estado comun a los dos dibujar(): sube los uniforms y deja el programa
    // activo. Devuelve false si no se puede dibujar todavia.
    bool preparar(const float model[16], float anchoPx);
};

// LineRenderer activo del engine (un solo contexto GL), con el mismo criterio
// singleton que Backend::activeBackend(): los componentes de colisión dibujan
// sus gizmos desde la GUI sin recibir el renderer por parametro.
LineRenderer& lineRenderer();

#endif
