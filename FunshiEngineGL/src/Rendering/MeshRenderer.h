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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/glm.hpp>

#include "../Iluminacion/LightSystem.h"
#include "Sombras/MapaSombras.h"

class ShaderProgram;

namespace Rendering {
class GestorShaders;
}  // namespace Rendering
class MeshGPU;
class TextureGL;
struct Mesh;
struct Image;
class TextureManager;
class GameObject;
class IMaterial;
class IResolutorMateriales;

// Renderer de objetos con el pipeline moderno (VBO/VAO + shader). Replica
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

    // Backend de render: con el se crean y destruyen los recursos GPU (mallas
    // y texturas). Sin el, el renderer no dibuja.
    void setBackend(Rendering::Backend::IRenderBackend& backend) noexcept {
        backend_ = &backend;
    }

    // Gestor de shaders del motor (dueno de los programas): sin el, el
    // renderer no tiene programa y no dibuja.
    void setGestorShaders(Rendering::GestorShaders* gestorShaders) noexcept {
        gestorShaders_ = gestorShaders;
    }

    // Manager de imagenes compartidas (flyweight): si no se inyecta, los
    // materiales con textura se dibujan sin ellas (no hay loader de imagenes).
    void setTextureManager(TextureManager* textureManager) noexcept {
        textureManager_ = textureManager;
    }

    // Resolvedor de materiales por region: decide que material se dibuja en
    // cada region de la malla (una sub-malla con material propio). Si no se
    // inyecta, la malla se dibuja entera con el material unico del objeto.
    void setResolvedorMateriales(IResolutorMateriales* resolvedor) noexcept {
        resolvedorMateriales_ = resolvedor;
    }

    // Luces de la pasada actual (1 vez por pasada, desde LightSystem: la misma
    // semantica que antes tenía el estado fijo de luces, pero calculada en CPU).
    // globalAmbient = modelo de luz.
    void setLuces(const LightData* luces, int lucesCount,
                  const float* globalAmbient);
    void setHorizonteVisual(float inicio, float fin) noexcept {
        horizonteInicio_ = inicio;
        horizonteFin_ = fin;
    }

    // Dibuja una malla ya resuelta con su matriz mundo ya calculada. Es el
    // camino que usa un GameObject con componente Model: el contexto solo
    // aporta Material/Color para la iluminacion, y la transformacion viene
    // del Transform global del objeto, de modo que la jerarquia padre/hijo se
    // respeta sin fabricar un objeto temporal.
    bool intentarRenderMesh(GameObject* contexto, const Mesh* mesh,
                            const float modelo[16], const float view[16],
                            const float projection[16], float deltaTime);

    // Dibuja una malla con un material concreto (no resuelto por region) en las
    // matrices dadas. Es el camino de la vista previa de un material: la esfera
    // de muestra se dibuja con el material que se esta editando, sin las
    // regiones que pueda declarar la malla.
    bool renderMeshConMaterial(const Mesh* mesh, const IMaterial* material,
                               const float modelo[16], const float view[16],
                               const float projection[16]);

    // Pases de sombras.
    //
    // Dibuja la malla en el pase de profundidad: solo escribe la profundidad
    // vista desde la luz, con el programa de profundidad y la matriz de la
    // luz. No aplica materiales ni luces. Devuelve false si no hay programa
    // o si la malla no es dibujable.
    bool intentarRenderSombra(const Mesh* mesh, const float modelo[16],
                               const glm::mat4& matrizLuz);

    // Mapas de sombra de la pasada: los proyectores con los que el shader de
    // mallas consulta la profundidad. Los mapas se enlazan a las unidades de
    // textura reservadas para ellos (las del material ocupan las primeras).
    void setSombras(const Rendering::ProyectorSombra* proyectores, int cantidad);

    // Libera los MeshGPU/TextureGL cacheados (por ejemplo tras recargar
    // mallas o texturas).
    void clearCache();

private:
    // Resuelve el material de una region de la malla (por su nombre; vacio
    // = la malla entera). La escena lo resuelve con el resolvedor y el objeto
    // como contexto; la vista previa pasa el material que se esta editando.
    using ProveedorMaterial =
        std::function<const IMaterial*(const std::string& nombreRegion)>;

    std::shared_ptr<ShaderProgram> shader_;
    // Programa del pase de profundidad (mapa de sombras). Es aparte del de
    // las mallas: escribe profundidad y nada mas.
    std::shared_ptr<ShaderProgram> shaderSombra_;
    // Compilacion determinista: si el shader embebido fallo una vez, queda
    // marcado y inicializar() no vuelve a intentarlo (ni a loguearlo) por frame.
    bool falloShader_ = false;
    Rendering::GestorShaders* gestorShaders_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    // Resolvedor de materiales por region (inyectado).
    IResolutorMateriales* resolvedorMateriales_ = nullptr;
    // Backend de render inyectado (crea y destruye los recursos GPU).
    Rendering::Backend::IRenderBackend* backend_ = nullptr;
    std::unordered_map<const Mesh*, std::unique_ptr<MeshGPU>> gpu_;
    std::unordered_map<const Image*, std::unique_ptr<TextureGL>> gpuTexturas_;
    // Paths de textura que ya fallaron al cargar: no se reintentan por frame.
    std::unordered_set<std::string> texturasFallidas_;
    // Mallas sin normales por vertice (no dibujables): el aviso por consola se
    // emite una sola vez por malla, no por frame.
    std::unordered_set<const Mesh*> mallasSinNormales_;
    std::vector<LightData> luces_;
    // Mapas de sombra de la pasada actual (los consulta el shader de mallas).
    std::vector<Rendering::ProyectorSombra> sombras_;
    float globalAmbient_[3] = {0.15f, 0.15f, 0.15f};
    float horizonteInicio_ = 40.0f;
    float horizonteFin_ = 150.0f;

    // Compila el shader por defecto la primera vez. No lanza: si el pipeline
    // moderno no esta disponible, deja available() == false.
    bool inicializar();
    // Dibuja la malla con el material que resuelve el proveedor para cada
    // region (o para la malla entera si no tiene regiones). El contexto puede
    // ser nullptr (vista previa): entonces, sin material, se usan los valores
    // por defecto en vez de los del objeto.
    bool renderMesh(GameObject* contexto, const Mesh* mesh,
                    const float modelo[16], const float view[16],
                    const float projection[16],
                    const ProveedorMaterial& proveedor);
    void configurarSamplers();
    void aplicarLuces();
    // Sube los uniforms del material (contrato IMaterial). Si el material es
    // nullptr, se usa el color del objeto o los valores por defecto.
    void aplicarMaterial(GameObject* objeto, const IMaterial* material);
    // Sube los uniforms de los mapas de sombra (matriz, sesgo, fuerza, texel
    // y a que luz pertenece cada mapa) y enlaza sus texturas.
    void aplicarSombras();
    // Enlaza las texturas de los mapas de sombra a sus unidades (las del
    // material ocupan las cuatro primeras). Se hace en cada dibujado: otras
    // pasadas cambian el contenido de las unidades.
    void enlazarTexturasSombra();
    // Sube la malla a GPU (cache por identidad de la malla) y la devuelve.
    // nullptr si la malla no se puede dibujar.
    const MeshGPU* obtenerGpu(const Mesh* mesh);
    // Resuelve los 4 slots de textura del material (difusa/especular/emision/
    // normal) contra el TextureManager inyectado y enlaza las unidades de
    // sampleo correspondientes. La normal exige que la malla tenga tangentes.
    void aplicarTexturas(const IMaterial* material, const Mesh* mesh);
    // Enlaza un slot: devuelve true si quedo bindeado en la unidad indicada.
    bool enlazarSlotTextura(const std::string& path, int unit,
                            const char* samplerUniform);
};

#endif