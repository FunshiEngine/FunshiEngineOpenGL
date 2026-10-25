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
#ifndef OBJETOS_TILEMAP_H
#define OBJETOS_TILEMAP_H

#include <cstdint>
#include <string>
#include <vector>

#include "../../Assets/Mesh.h"
#include "../../Assets/Tileset.h"
#include "Component.h"

class GameObject;

// Una capa del tilemap: una grilla de ids de tile mas su visibilidad y flag de
// colision. El id 0 se reserva como celda vacia (convencion de Tiled donde el
// primer gid es 1); el resto indexa directo en el Tileset.
struct CapaTilemap {
    std::string nombre = "Capa";
    bool visible = true;
    bool colision = false;
    int ancho = 0;
    int alto = 0;
    std::vector<std::uint32_t> tiles;
};

// Componente de mapa de tiles 2D. Guarda la ruta de un tileset externo (JSON
// estilo Tiled), una o varias capas de ids y expone el pintado por celda. La
// geometria de las capas visibles se hornea en una unica malla (un draw call por
// tileset) que consume el SceneRenderer, y las capas con colision alimentan al
// backend de fisica. El componente no depende de OpenGL ni de Bullet: solo
// describe datos y geometria, de modo que los tests headless lo ejerciten.
class Tilemap : public Component {
public:
    Tilemap();

    const std::string& getRutaTileset() const { return rutaTileset_; }
    // Asigna la ruta del tileset (absoluta o relativa a la raiz de assets) y
    // recarga sus metadatos.
    void setRutaTileset(const std::string& ruta);

    const Tileset& getTileset() const { return tileset_; }
    bool tilesetValido() const { return tileset_.esValido(); }
    const std::string& getUltimoErrorTileset() const { return ultimoError_; }
    // Vuelve a leer el JSON del tileset desde disco.
    void recargarTileset();
    // Ruta absoluta de la imagen del atlas (para cargarla en el backend).
    std::string rutaImagenTileset() const;

    int cantidadCapas() const { return static_cast<int>(capas_.size()); }
    const CapaTilemap* capa(int indice) const;
    CapaTilemap* capa(int indice);
    // Agrega una capa vacia (todas las celdas en 0). Devuelve su indice, o -1 si
    // las dimensiones son invalidas.
    int agregarCapa(const std::string& nombre, int ancho, int alto,
                    bool colision);
    bool eliminarCapa(int indice);
    void setCapaVisible(int indice, bool visible);
    void setCapaColision(int indice, bool colision);
    void setCapaNombre(int indice, const std::string& nombre);

    // Pinta la celda (x, y) de la capa con el id dado (0 borra).
    void pintar(int indice, int x, int y, std::uint32_t id);
    std::uint32_t tileEn(int indice, int x, int y) const;

    // Identificador que cambia con cada edicion de tiles o capas. Los consumidores
    // (render, colision) lo usan para rehacer lo que cachean.
    std::uint64_t revision() const { return revision_; }

    // Hornea la geometria de todas las capas visibles en una malla en el espacio
    // local del objeto (1 tile = 1 unidad, fila 0 arriba). Devuelve false si no
    // hay tileset valido o ninguna celda pintada.
    bool construirGeometria(Mesh& out) const;

    // Malla horneada cacheada: se reconstruye solo cuando cambia la revision, de
    // modo que el bucle de dibujo no rehaga la geometria cada frame. Un mapa sin
    // celdas pintadas devuelve una malla vacia (isEmpty()).
    const Mesh& mallaHorneada() const;

    void onLoaded(GameObject& dueno) override;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

private:
    std::string rutaTileset_;
    Tileset tileset_;
    std::string ultimoError_;
    std::vector<CapaTilemap> capas_;
    std::uint64_t revision_ = 1;
    mutable Mesh mallaCache_;
    mutable std::uint64_t mallaRevision_ = 0;
    mutable bool mallaConstruida_ = false;
};

#endif
