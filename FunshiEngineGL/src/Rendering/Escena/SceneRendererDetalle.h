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
// Preambulo compartido por las unidades de SceneRenderer.cpp (generado al dividirlo).
#ifndef SCENERENDERERDETALLE_H
#define SCENERENDERERDETALLE_H

#include "SceneRenderer.h"
#include "../../Diagnosticos/RegistroDiagnosticos.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include "../Backend/IRenderBackend.h"
#include "../Cielo/CacheCubemap.h"
#include "../Cielo/Cielo.h"
#include "../Mallas/DibujoModelo.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/Componentes/Render/Skybox.h"
#include "../../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../../Objetos/Componentes/Juego2D/Tilemap.h"
#include <stb_image/stb_image.h>
#include "../Lineas/LineBatch.h"
#include "../Lineas/LineBuilder.h"
#include "../Lineas/LineRenderer.h"
#include "../Mallas/MeshRenderer.h"
#include "RenderTarget.h"
#include "../Shaders/ShaderProgram.h"
#include "../Shaders/GestorShaders.h"
#include "../../Configuracion/Apariencia.h"
#include "../../Configuracion/EditorConfig.h"
#include "../../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../../Iluminacion/LightSystem.h"
#include "../../Objetos/Componentes/Escena/CameraComponent.h"
#include "../../Objetos/Componentes/Colliders/Collider.h"
#include "../../Objetos/Componentes/Escena/Grid.h"
#include "../../Objetos/Componentes/Escena/Light.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/Componentes/Render/Material.h"
#include "../../Assets/Mallas/AssetManager.h"

namespace {

// Media anchura de la banda de degradado del cielo, en unidades de la
// componente vertical de la direccion de vista. El degradado se reparte
// alrededor del horizonte y satura a los dos extremos, asi que 0.35 lleva el
// color inferior a toda la vista al mirar claramente hacia abajo y el superior
// al mirar claramente hacia arriba, dejando la mezcla solo en el horizonte.
constexpr float kCieloTransicion = 0.35f;

}  // namespace

// Ultima modificacion del archivo en unidades del reloj de la filesystem
// (segundos desde el epoch en la mayoria de los SO); 0 si el archivo no existe
// o no se puede consultar. La variante con error_code no lanza: una cara que
// falte es un dato para la clave, no una excepcion.
static std::int64_t mtimeSegundos(const std::string& ruta) {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(std::filesystem::path(ruta), ec);
    if (ec) return 0;
    return static_cast<std::int64_t>(t.time_since_epoch().count());
}

#endif
