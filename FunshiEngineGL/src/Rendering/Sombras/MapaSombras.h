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
// de mapas que consulta el shader de mallas y del trabajo de GPU de la
// pasada de sombras: cada proyector es un mapa (2D o cubemap de 6 caras) que
// se dibuja entero.
constexpr int kMaxProyectoresSombra = 4;

// Tipo de la luz que puede proyectar sombra.
enum class TipoLuzProyector {
    Direccional,
    Spot,
    Puntual,
};

// Caras de un cubemap de sombra para una luz puntual, en el orden que OpenGL
// espera: +X, -X, +Y, -Y, +Z, -Z.
constexpr int kCarasCubemap = 6;

// Luz de la escena que podria proyectar sombra, con los ajustes del
// componente Sombras de su objeto.
struct CandidatoProyector {
    glm::vec3 posicion{0.f, 10.f, 0.f};
    glm::vec3 direccion{0.f, -1.f, 0.f};
    TipoLuzProyector tipo = TipoLuzProyector::Direccional;
    // Indice de la luz en los uniforms de iluminacion: es lo que permite que
    // la sombra atenue la contribucion de esa luz y no la de las demas.
    int indiceLuz = 0;
    int resolucion = 1024;
    float radio = 20.f;
    float distancia = 60.f;
    float sesgo = 0.0015f;
    float fuerza = 1.f;
};

// Proyector elegido: la(s) matriz(es) que proyectan al mapa de sombras y los
// ajustes con los que se sombrea. Para luces direccionales y spot se usa una
// sola matriz (matricesLuz[0]); para puntuales se usan las 6 caras del
// cubemap (matricesLuz[0..5]) y esCubemap es true.
struct ProyectorSombra {
    // Matriz de vista x proyeccion de la primera cara (o la unica cara para
    // direccional/spot).
    glm::mat4 matrizLuz{1.f};
    // Para luces puntuales: las 6 matrices de las caras del cubemap.
    std::array<glm::mat4, kCarasCubemap> matricesLuz{
        glm::mat4(1.f), glm::mat4(1.f), glm::mat4(1.f),
        glm::mat4(1.f), glm::mat4(1.f), glm::mat4(1.f)};
    bool esCubemap = false;
    int indiceLuz = 0;
    int resolucion = 1024;
    float sesgo = 0.0015f;
    float fuerza = 1.f;
    float distancia = 60.f; // alcance de la luz (para puntuales, en el shader)
    // Textura de profundidad del mapa (la consulta el shader de mallas).
    Backend::Handle textura = Backend::kInvalidHandle;
};

// Cuantos proyectores se eligieron y cuantos se quedaron fuera, y por que.
struct ResultadoSeleccion {
    int elegidos = 0;
    // Candidatos que proyectan sombra pero se quedaron fuera porque ya
    // habia alcanzado el limite de mapas.
    int porLimite = 0;
    // Candidatos que no pueden proyectar sombra (deberia ser 0: todas las
    // luces admitidas tienen camino de sombra implementado).
    int noSoportados = 0;
};

// Elige los proyectores de sombra entre los candidatos. Se descartan los que
// no pueden proyectar sombra y los que se pasan del limite; el resto sale en
// 'salida' con su matriz ya calculada.
ResultadoSeleccion seleccionarProyectores(const CandidatoProyector* candidatos,
                                           int cantidad, int limite,
                                           const glm::vec3& centroEscena,
                                           ProyectorSombra* salida);

// Dueno de los objetivos de profundidad (los mapas de sombra). Los crea
// cuando hace falta (una luz nueva, o una resolucion cambiada) y suelta los
// que dejan de usarse, para no dejar recursos de GPU colgando.
class MapaSombras {
public:
    // Un mapa de sombra en la GPU: el framebuffer de profundidad y su textura.
    // Para luces direccionales/spot es un target 2D (target + textura).
    // Para luces puntuales es un cubemap de 6 caras: targetsCubo[0] es el FBO
    // compartido y textura es el handle del cubemap de profundidad.
    struct Mapa {
        Backend::Handle target = Backend::kInvalidHandle;
        Backend::Handle textura = Backend::kInvalidHandle;
        bool esCubemap = false;
        // Para cubemap: FBO compartido por las 6 caras (misma resolucion).
        std::array<Backend::Handle, kCarasCubemap> targetsCubo{
            Backend::kInvalidHandle, Backend::kInvalidHandle,
            Backend::kInvalidHandle, Backend::kInvalidHandle,
            Backend::kInvalidHandle, Backend::kInvalidHandle};
        int resolucion = 0;
    };

    explicit MapaSombras(Backend::IRenderBackend& backend);
    ~MapaSombras();

    MapaSombras(const MapaSombras&) = delete;
    MapaSombras& operator=(const MapaSombras&) = delete;

    // Prepara el mapa del proyector 'indice' con la resolucion pedida. Si
    // esCubemap es true, crea un cubemap de profundidad de 6 caras en lugar
    // de un target 2D. Si la resolucion cambio, el mapa anterior se rehace.
    // Devuelve false si el backend no puede crear el objetivo (el proyector
    // se dibuja sin sombra).
    bool preparar(int indice, int resolucion, bool esCubemap = false);

    // El mapa del proyector. Si todavia no se preparo, sale vacio.
    const Mapa& mapa(int indice) const;

    // Suelta los mapas de los proyectores que ya no se usan (los que estan
    // despues del indice dado).
    void liberarDesde(int indice);

    // Numero de mapas activos.
    int activos() const { return static_cast<int>(entradas_.size()); }

private:
    Backend::IRenderBackend& backend_;
    std::vector<Mapa> entradas_;
};

}  // namespace Rendering

#endif
