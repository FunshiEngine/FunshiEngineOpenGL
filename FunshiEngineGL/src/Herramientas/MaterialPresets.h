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
#ifndef MATERIALPRESETS_H
#define MATERIALPRESETS_H

#include "../Objetos/Componentes/Render/Material.h"

// Presets de materiales como DATOS (funciones inline), no como objetos
// globales: cada preset configura el Material que recibe. Reemplazan al viejo
// materiales.h que definia objetos static globales por Translation Unit.
namespace MaterialPresets {

static inline void bronze(Material& m) {
    m.setAmbient(0.2125f, 0.1275f, 0.054f);
    m.setDiffuse(0.714f, 0.4284f, 0.18144f);
    m.setSpecular(0.393548f, 0.271906f, 0.166721f);
    m.setShininess(0.2f);
}
static inline void copper(Material& m) {
    m.setAmbient(0.19125f, 0.0735f, 0.0225f);
    m.setDiffuse(0.7038f, 0.27048f, 0.0828f);
    m.setSpecular(0.256777f, 0.137622f, 0.086014f);
    m.setShininess(0.1f);
}
static inline void chrome(Material& m) {
    m.setAmbient(0.25f, 0.25f, 0.25f);
    m.setDiffuse(0.4f, 0.4f, 0.4f);
    m.setSpecular(0.774597f, 0.774597f, 0.774597f);
    m.setShininess(0.6f);
}
static inline void brass(Material& m) {
    m.setAmbient(0.329412f, 0.223529f, 0.027451f);
    m.setDiffuse(0.780392f, 0.568627f, 0.113725f);
    m.setSpecular(0.992157f, 0.941176f, 0.807843f);
    m.setShininess(0.21794872f);
}
static inline void jade(Material& m) {
    m.setAmbient(0.135f, 0.2225f, 0.1575f);
    m.setDiffuse(0.54f, 0.89f, 0.63f);
    m.setSpecular(0.316228f, 0.316228f, 0.316228f);
    m.setShininess(0.1f);
}
static inline void luz(Material& m) {
    m.setAmbient(0.f, 0.f, 0.f);
    m.setDiffuse(0.f, 0.f, 0.f);
    m.setSpecular(0.f, 0.f, 0.f);
    m.setShininess(0.f);
    m.setEmission(1.f, 1.f, 0.f);
}

} // namespace MaterialPresets

#endif