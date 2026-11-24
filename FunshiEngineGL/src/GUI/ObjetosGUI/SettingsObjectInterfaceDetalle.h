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
// Preambulo compartido por las unidades de SettingsObjectInterface.cpp (generado al dividirlo).
#ifndef SETTINGSOBJECTINTERFACEDETALLE_H
#define SETTINGSOBJECTINTERFACEDETALLE_H

#include "SettingsObjectInterface.h"
#include "SettingsComponent.h"
#include "Transform/SettingsTransform.h"
#include "Color/SettingsColor.h"
#include "Colliders/SettingsColliderEsfera.h"
#include "Colliders/SettingsColliderCubo.h"
#include "Colliders/SettingsColliderMalla.h"
#include "RigidBody/SettingsRigidBody.h"
#include "Script/SettingsScript.h"
#include "Model/SettingsModel.h"
#include "Material/SettingsMaterial.h"
#include "Light/SettingsLight.h"
#include "Camera/SettingsCamera.h"
#include "Audio/SettingsAudioSource.h"
#include "Interface/SettingsInterface.h"
#include "Grid/SettingsGrid.h"
#include "Skybox/SettingsSkybox.h"
#include "Sprite/SettingsSpriteRenderer.h"
#include "../../Objetos/Componentes/Nucleo/CatalogoComponentes.h"
#include "../../Scenes/Editor/ArrastreArchivos.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/TagRegistry.h"
#include "../../Objetos/Componentes/Escena/Light.h"
#include "../../Objetos/Componentes/Render/Material.h"
#include "../../Objetos/Componentes/Escena/CameraComponent.h"
#include "../../Objetos/Componentes/Audio/AudioSource.h"
#include "../../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../../Objetos/Componentes/Escena/Grid.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/Componentes/Render/Color.h"
#include "../../Objetos/Componentes/Scripting/Script.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../../Objetos/Componentes/Colliders/MallaCollider.h"
#include "../../Objetos/SimpleObject.h"
#include "../../Objetos/Componentes/Render/Skybox.h"
#include "../../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../../Scenes/Editor/EditorController.h"
#include "../../Comandos/Componentes/AgregarComponenteComando.h"
#include "../../Comandos/Componentes/QuitarComponenteComando.h"
#include "../../Comandos/GestorComandos.h"
#include "../../Events/EventBus.h"
#include "../../Herramientas/TypeUtils.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <typeinfo>
#include <algorithm>
#include <cstring>
#include <vector>

namespace {

// Tamano del icono del componente en el inspector (en pixeles). Se dibuja al
// 15% mas grande que antes (16 -> 18.4): a 16 se perdia el detalle del icono
// de los componentes pequenos.
constexpr float kTamanoIconoComponente = 18.4f;

}  // namespace

#endif
