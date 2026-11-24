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
#ifndef MAPASOMBRAS_H
#define MAPASOMBRAS_H

#include <glm/glm.hpp>
#include <array>
#include <vector>

#include "../Backend/IRenderBackend.h"

namespace Rendering {

// Cuantas luces pueden proyectar sombra a la vez. El limite viene del numero
// de mapas que consulta el shader de mallas (MAX_SHADOWS) y del trabajo de GPU
// de la pasada: cada proyector es un mapa (2D o cubemap de 6 caras).
constexpr int kMaxProyectoresSombra = 4;

// Tipo de la luz que puede proyectar sombra.
enum class TipoLuzProyector {
    Direccional,
    Spot,
    Puntual,
};

// Caras de un cubemap de sombra (luz puntual), en el orden de OpenGL:
// +X, -X, +Y, -Y, +Z, -Z.
constexpr int kCarasCubemap = 6;

// Luz de la escena que podria proyectar sombra, con los ajustes del
// componente Sombras de su objeto.
struct CandidatoProyector {
    glm::vec3 posicion{0.f, 10.f, 0.f};
    glm::vec3 direccion{0.f, -1.f, 0.f};
    TipoLuzProyector tipo = TipoLuzProyector::Direccional;
    // Indice de la luz en los uniforms de iluminacion: la sombra atenua la
    // contribucion de esa luz y no la de las demas.
    int indiceLuz = 0;
    int resolucion = 1024;
    // Semi-lado del frustum ortografico (solo direccional).
    float radio = 20.f;
    // Alcance de la sombra (plano lejano).
    float distancia = 60.f;
    float sesgo = 0.0015f;
    float fuerza = 1.f;
    // Angulo de corte del cono (solo spot), en grados desde el eje.
    float anguloSpotGrados = 45.f;
};

// Proyector elegido: matrices con las que se dibuja su mapa y los datos con
// los que el shader de mallas lo consulta. El indice del proyector en la
// lista elegida es su SLOT en el shader (uShadowMap<slot>/uShadowCube<slot>).
struct ProyectorSombra {
    // Matriz vista x proyeccion del mapa 2D (direccional/spot).
    glm::mat4 matrizLuz{1.f};
    // Para luces puntuales: una matriz por cara del cubemap.
    std::array<glm::mat4, kCarasCubemap> matricesCara{};
    bool esCubemap = false;
    int indiceLuz = 0;
    int resolucion = 1024;
    float sesgo = 0.0015f;
    float fuerza = 1.f;
    // Posicion de la luz y alcance: el mapa de una puntual guarda la distancia
    // a la luz dividida por el alcance.
    glm::vec3 posicionLuz{0.f};
    float distancia = 60.f;
    // Tamano aproximado de un texel del mapa en unidades del mundo: el shader
    // desplaza el punto consultado a lo largo de la normal en esa medida
    // (normal offset) para evitar el acne sin despegar la sombra del objeto.
    float tamanoTexelMundo = 0.04f;
    // Textura de profundidad del mapa (la consulta el shader de mallas).
    Backend::Handle textura = Backend::kInvalidHandle;
};

// Cuantos proyectores se eligieron y cuantos se quedaron fuera por el limite.
struct ResultadoSeleccion {
    int elegidos = 0;
    int porLimite = 0;
};

// Elige los proyectores de sombra entre los candidatos (en orden, hasta el
// limite) y calcula sus matrices. 'centroEscena' es el punto alrededor del que
// se encuadra la luz direccional (lo que la camara mira).
ResultadoSeleccion seleccionarProyectores(const CandidatoProyector* candidatos,
                                           int cantidad, int limite,
                                           const glm::vec3& centroEscena,
                                           ProyectorSombra* salida);

// Duenio de los objetivos de profundidad (los mapas de sombra). Los crea
// cuando hace falta (una luz nueva, una resolucion o un tipo cambiado) y suelta
// los que dejan de usarse, para no dejar recursos de GPU colgando.
class MapaSombras {
public:
    // Un mapa en la GPU: el objetivo de profundidad (2D o cubemap) y su textura.
    struct Mapa {
        Backend::Handle target = Backend::kInvalidHandle;
        Backend::Handle textura = Backend::kInvalidHandle;
        bool esCubemap = false;
        int resolucion = 0;
    };

    explicit MapaSombras(Backend::IRenderBackend& backend);
    ~MapaSombras();

    MapaSombras(const MapaSombras&) = delete;
    MapaSombras& operator=(const MapaSombras&) = delete;

    // Prepara el mapa del slot 'indice'. Si la resolucion o el tipo cambiaron,
    // el mapa anterior se rehace. Devuelve false si el backend no puede crear
    // el objetivo (el proyector se dibuja sin sombra).
    bool preparar(int indice, int resolucion, bool esCubemap = false);

    // El mapa del slot. Si todavia no se preparo, sale vacio.
    const Mapa& mapa(int indice) const;

    // Suelta los mapas de los slots desde 'indice' en adelante.
    void liberarDesde(int indice);

    int activos() const { return static_cast<int>(entradas_.size()); }

private:
    void liberar(Mapa& entrada);

    Backend::IRenderBackend& backend_;
    std::vector<Mapa> entradas_;
};

}  // namespace Rendering

#endif
