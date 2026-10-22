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
#ifndef EMISORPARTICULAS_H
#define EMISORPARTICULAS_H

#include <cstddef>
#include <memory>
#include <string>

#include "../../Particulas/ParametrosEmisor.h"
#include "../../Particulas/PerfilParticula.h"
#include "../../Particulas/SistemaParticulas.h"
#include "Component.h"

// Componente que suelta particulas.
//
// Guarda los datos del emisor (los que edita el inspector) y, si tiene, el
// perfil de efecto que manda sobre ellos. El estado de ejecucion (el
// acumulador de emision) no se serializa: es lo mismo que hace Animador2D
// con su fotograma.
class EmisorParticulas : public Component {
public:
    EmisorParticulas();

    // Parametros del emisor (los edita el inspector).
    ParametrosEmisor& parametros() noexcept { return parametros_; }
    const ParametrosEmisor& parametros() const noexcept { return parametros_; }

    bool activa() const noexcept { return activa_; }
    void setActiva(bool activa) noexcept { activa_ = activa; }

    // Ruta del perfil de efecto, relativa al proyecto. Vacio = sin perfil.
    const std::string& rutaPerfil() const noexcept { return rutaPerfil_; }
    // Fija la ruta y vuelve a leer el perfil. Si el archivo no se puede leer,
    // el emisor se queda sin perfil y el motivo queda en 'errorPerfil'.
    void setRutaPerfil(const std::string& ruta);

    // Ruta opcional de la textura de particula, relativa al proyecto. Si no
    // esta vacia, sobreescribe la textura del perfil al renderizar.
    const std::string& texturaParticula() const noexcept { return texturaParticula_; }
    void setTexturaParticula(const std::string& ruta) noexcept { texturaParticula_ = ruta; }

    // Perfil cargado (nulo si el emisor no tiene o si no se pudo leer).
    const std::shared_ptr<const PerfilParticula>& perfil() const noexcept {
        return perfil_;
    }
    // Motivo del ultimo fallo al leer el perfil (vacio si no hubo).
    const std::string& errorPerfil() const noexcept { return errorPerfil_; }

    // Suelta las particulas que tocan segun la tasa del emisor y devuelve
    // cuantas emitio. Un frame largo no puede soltar una avalancha: lo
    // pendiente se acota.
    std::size_t emitir(SistemaParticulas& sistema, const float origen[3],
                       float deltaTime);

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;
    void onLoaded(GameObject& owner) override;

private:
    // Vuelve a leer el perfil desde la ruta. Si la ruta esta vacia, el
    // emisor se queda sin perfil.
    void recargarPerfil();

    ParametrosEmisor parametros_;
    std::string rutaPerfil_;
    std::string texturaParticula_;
    bool activa_ = true;
    // Estado de ejecucion: no se serializa.
    float acumulador_ = 0.0f;
    std::shared_ptr<const PerfilParticula> perfil_;
    std::string errorPerfil_;
};

#endif
