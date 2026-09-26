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
#ifndef MESHRENDERER_H
#define MESHRENDERER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "../Iluminacion/LightSystem.h"

class ShaderProgram;
class MeshGPU;
class TextureGL;
class Mesh;
class Image;
class TextureManager;
class Modelos3D;

// Renderer de objetos con el pipeline moderno (VBO/VAO + shader). Sustituyo al
// dibujado de Modelos3D que vivia en glBegin/glEnd, que ya no existe. Replica
// la iluminacion que antes daba el estado fijo (LightSystem + Material) con
// uniforms, para que el pasaje no sea una regresion visual; la grilla y los
// gizmos van por LineRenderer, su propio batch en GPU.
// El MeshGPU se cachea por identidad de la Mesh CPU (que ya es compartida por
// AssetManager), asi dos objetos con la misma malla no re-suben geometria. La
// textura difusa del Material (path -> Image compartida via TextureManager)
// se sube a GPU una sola vez por imagen y se cachea por identidad igual que la
// geometria; si la malla no tiene UVs o el archivo no carga, el objeto se
// dibuja sin textura (se registra el fallo para no reintentar cada frame).
class MeshRenderer {
public:
    MeshRenderer();
    ~MeshRenderer();

    bool available() const noexcept { return shader_ != nullptr; }

    // Manager de imagenes compartidas (flyweight): si no se inyecta, los
    // materiales con textura se dibujan sin ellas (no hay loader de imagenes).
    void setTextureManager(TextureManager* textureManager) noexcept {
        textureManager_ = textureManager;
    }

    // Luces de la pasada actual (1 vez por pasada, desde LightSystem: la misma
    // semantica que antes tenía el estado fijo de luces, pero calculada en CPU).
    // globalAmbient = modelo de luz.
    void setLuces(const LightData* luces, int lucesCount,
                  const float* globalAmbient);

    // Dibuja el modelo con el pipeline moderno (update + material + malla).
    // Devuelve false si no se drew: shader/VAO no disponible, o malla vacía o
    // sin normales por vertice (en ese caso avisa una vez por malla). No hay
    // pipeline de respaldo:false significa "esta pasada no dibujo nada".
    bool intentarRender(Modelos3D* objeto, const float view[16],
                        const float projection[16], float deltaTime);

    // Libera los MeshGPU/TextureGL cacheados (por ejemplo tras recargar
    // mallas o texturas).
    void clearCache();

private:
    std::unique_ptr<ShaderProgram> shader_;
    TextureManager* textureManager_ = nullptr;
    std::unordered_map<const Mesh*, std::unique_ptr<MeshGPU>> gpu_;
    std::unordered_map<const Image*, std::unique_ptr<TextureGL>> gpuTexturas_;
    // Paths de textura que ya fallaron al cargar: no se reintentan por frame.
    std::unordered_set<std::string> texturasFallidas_;
    // Mallas sin normales por vertice (no dibujables): el aviso por consola se
    // emite una sola vez por malla, no por frame.
    std::unordered_set<const Mesh*> mallasSinNormales_;
    std::vector<LightData> luces_;
    float globalAmbient_[3] = {0.15f, 0.15f, 0.15f};

    // Compila el shader por defecto la primera vez. No lanza: si el pipeline
    // moderno no esta disponible, deja available() == false.
    bool inicializar();
    void aplicarLuces();
    void aplicarMaterial(Modelos3D* objeto);
    // Resuelve los 4 slots de textura del Material (difusa/especular/emision/
    // normal) contra el TextureManager inyectado y enlaza las unidades de
    // sampleo correspondientes. La normal exige que la malla tenga tangentes.
    void aplicarTexturas(Modelos3D* objeto, const Mesh* mesh);
    // Enlaza un slot: devuelve true si quedo bindeado en la unidad indicada.
    bool enlazarSlotTextura(const std::string& path, int unit,
                            const char* samplerUniform);
};

#endif