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
// Preambulo compartido por las unidades de MeshRenderer.cpp (generado al dividirlo).
#ifndef MESHRENDERERDETALLE_H
#define MESHRENDERERDETALLE_H

#include "MeshRenderer.h"
#include "../../Diagnosticos/RegistroDiagnosticos.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include "../../Assets/Mallas/Mesh.h"
#include "../../Assets/Imagenes/TextureManager.h"
#include "../../Iluminacion/LightSystem.h"
#include "../../Objetos/Componentes/Render/Color.h"
#include "../../Objetos/Componentes/Render/IMaterial.h"
#include "../../Objetos/Componentes/Render/IResolutorMateriales.h"
#include "../../Objetos/Componentes/Render/Material.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/GameObject.h"
#include "MeshGPU.h"
#include "../Shaders/ShaderProgram.h"
#include "../Shaders/GestorShaders.h"
#include "../Sombras/MapaSombras.h"
#include "../Sombras/UniformsSombras.h"
#include "../Texturas/TextureGL.h"
#include "../Backend/IRenderBackend.h"

namespace {

// Unidades fijas de textura para cada tipo de sampler: evita colisiones de
// tipo en la misma unidad de textura (lo cual causa GL_INVALID_OPERATION 1282
// en glDrawElements al mezclar sampler2D, sampler2DShadow y samplerCube en unidad 0).
constexpr int kUnidadTexturaDifusa = 0;

constexpr int kUnidadTexturaEspecular = 1;

constexpr int kUnidadTexturaEmision = 2;

constexpr int kUnidadTexturaNormal = 3;

// Las unidades 4..11 son de los mapas de sombra (ver UniformsSombras.h).

}  // namespace

#endif
