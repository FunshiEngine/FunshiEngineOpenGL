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
#ifndef PARTICULASRENDERER_H
#define PARTICULASRENDERER_H

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../Particulas/ParticulasBuilder.h"
#include "../Particulas/SistemaParticulas.h"
#include "Backend/IRenderBackend.h"

class ShaderProgram;
class TextureGL;
class TextureManager;

namespace Rendering {
class GestorShaders;
}  // namespace Rendering

namespace Rendering {

// Pasada de particulas.
//
// El renderer no conoce el componente emisor (capa de entidades): recibe el
// sistema de particulas y dibuja lo que hay. Las particulas que comparten
// perfil comparten textura, asi que se agrupan por perfil y cada grupo es un
// lote con su propio VBO (los lotes se conservan entre frames: se vuelve a
// subir la geometria, no se recrea el recurso).
//
// Se dibuja con blend activo, depth test on y depth mask off: las particulas
// no escriben profundidad y el orden de dibujo decide el solapamiento.
class ParticulasRenderer {
private:
    // Lote de un grupo de particulas (uno por perfil).
    struct Lote {
        Backend::Handle handle = Backend::kInvalidHandle;
        std::size_t vertices = 0;
    };

    Backend::IRenderBackend* backend_ = nullptr;
    GestorShaders* gestorShaders_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    std::shared_ptr<ShaderProgram> shader_;
    // Geometria del frame: se reutiliza el buffer entre frames.
    ParticulasBuilder builder_;
    // Lotes por perfil (la clave es la direccion del perfil compartido).
    std::map<const PerfilParticula*, Lote> lotes_;
    // Texturas en GPU, cacheadas por identidad de la imagen.
    std::unordered_map<const void*, std::unique_ptr<TextureGL>> texturas_;
    // Rutas que no se pudieron cargar: no se reintentan cada frame.
    std::vector<std::string> texturasFallidas_;

    bool inicializar();
    // Enlaza la textura del perfil (o ninguna si el perfil no tiene). Si
    // texturaOverride no esta vacia, la usa en vez de la del perfil. Devuelve
    // false si hay que dibujar el lote sin textura.
    bool enlazarTextura(const PerfilParticula* perfil,
                        const std::string& texturaOverride = std::string());

public:
    // El constructor y el destructor estan en el .cpp: TextureGL esta
    // declarado adelante y alli se necesita el tipo completo para construir y
    // liberar el mapa de texturas.
    ParticulasRenderer();
    ~ParticulasRenderer();

    void setBackend(Backend::IRenderBackend* backend) { backend_ = backend; }
    void setGestorShaders(GestorShaders* gestor) { gestorShaders_ = gestor; }
    void setTextureManager(TextureManager* manager) { textureManager_ = manager; }

    // Dibuja las particulas vivas del sistema.
    void dibujar(const SistemaParticulas& sistema, const float view[16],
                 const float projection[16]);

    // Libera los recursos (lotes y texturas).
    void destruir();
};

}  // namespace Rendering

#endif
