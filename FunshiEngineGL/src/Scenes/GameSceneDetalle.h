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
// Preambulo compartido por las unidades de GameScene.cpp (generado al dividirlo).
#ifndef GAMESCENEDETALLE_H
#define GAMESCENEDETALLE_H

#include "GameScene.h"
#include "Juego/CameraFrameSafety.h"
#include "Registro/SiembraEscena.h"
#include "../GUIManager/GUIManager.h"
#include "../Events/EditorEventBus.h"
#include "../GUI/SceneGUI/SceneMenuBarInterface.h"
#include "../GUI/SceneGUI/SceneSelectedInterface.h"
#include "../GUI/WindowNames.h"
#include "../Objetos/Componentes/Escena/CameraComponent.h"
#include "../Fisicas/PhysicsEngine.h"
#include "../Fisicas/ContactoFisico.h"
#include "../Iluminacion/LightSystem.h"
#include "../Objetos/Componentes/Scripting/Script.h"
#include "../Objetos/Componentes/Escena/Transform.h"
#include "../Objetos/Componentes/Efectos/EmisorParticulas.h"
#include "../Objetos/Componentes/Escena/Grid.h"
#include "../Objetos/SimpleObject.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Objetos/Componentes/Render/Model.h"
#include "Editor/ArrastreArchivos.h"
#include "Editor/DropEscena.h"
#include "Editor/EditorController.h"
#include "Serializacion/ManifiestoAssets.h"
#include "Serializacion/RutasReescritura.h"
#include "Registro/SceneRegistry.h"
#include "Serializacion/SceneSerializer.h"
#include "../Assets/Mallas/AssetManager.h"
#include "../Assets/Mallas/AssimpMeshLoader.h"
#include "../Assets/Imagenes/StbImageLoader.h"
#include "../Assets/Imagenes/TextureManager.h"
#include "../Rendering/Backend/IRenderBackend.h"
#include "../Rendering/Preview/PreviewMaterial.h"
#include "../Rendering/Escena/RenderTarget.h"
#include "../Rendering/Escena/SceneRenderer.h"
#include "../Rendering/Shaders/GestorShaders.h"
#include "../Rendering/Lineas/GuiaEje.h"
#include "../Objetos/Prefab.h"
#include "../Objetos/PrefabLibrary.h"
#include "../Audio/AudioEngine.h"
#include "../Audio/MiniAudioBackend.h"
#include "../Objetos/Componentes/Audio/AudioSource.h"
#include "../Objetos/Componentes/Audio/Musica.h"
#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../Objetos/Componentes/Interfaz/UIComponent.h"
#include "../Configuracion/EditorConfig.h"
#include "Editor/GizmoController.h"
#include "../Objetos/Componentes/Juego2D/Tilemap.h"
#include "Juego/InterfacesJuego.h"
#include "Juego/OverlayInterfaces.h"
#include "../Rendering/Render2D/UIRenderer.h"
#include "../WebUI/WebUIRuntime.h"
#include <cmath>
#include <imgui.h>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <chrono>

// Duracion minima del overlay de carga de scripts: aunque la compilacion venga
// de cache (instantanea) la barra se ve un instante, y al terminar deja un
// aviso breve con el resultado para que el usuario no dependa de la consola.
static constexpr float kOverlayProgresoMin = 0.7f;

static constexpr float kOverlayResultadoSeg = 2.2f;

#endif
