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
// Preambulo compartido por las unidades de PrefabTests.cpp (generado al dividirlo).
#ifndef PREFABTESTSDETALLE_H
#define PREFABTESTSDETALLE_H

// Pruebas headless del sistema de Prefabs: round-trip completo (guardar -> instanciar ->
// conservar nombre, tag, jerarquia, componentes, Transform) y validacion de archivos.
//
// Patron de autoria (ver Skills): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdio>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptGameObject.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Herramientas/TypeUtils.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Objetos/Prefab.h"
#include "../FunshiEngineGL/src/Objetos/PrefabLibrary.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Nucleo/CatalogoComponentes.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Color.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Model.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Escena/Transform.h"
#include "../FunshiEngineGL/src/Scenes/Editor/EditorController.h"
#include "../FunshiEngineGL/src/Scenes/Registro/SceneRegistry.h"
#include "../FunshiEngineGL/src/Events/EventBus.h"
#include "../FunshiEngineGL/src/Assets/Mallas/AssetManager.h"

namespace fs = std::filesystem;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

// Estado de la unidad original: una sola instancia (se define en PrefabTests.cpp).

namespace PrefabTestsDetalle {

extern int total;
extern int fallos;

}  // namespace PrefabTestsDetalle
using namespace PrefabTestsDetalle;

// Funciones repartidas entre las unidades.

GameObject* crearEscenaBasica(SceneRegistry& registry, EditorController*& editor, EventBus* events, AssetManager* assets);
void verificarObjetoBasico(GameObject* obj, const std::string& nombreEsperado, const std::string& tagEsperado, const float posEsperada[3]);
void verificarHijo(GameObject* padre, const std::string& nombreHijo, const std::string& tagHijo, const float posHijo[3]);
void roundTripPrefabBasico();
void roundTripPrefabJerarquia();
void roundTripPrefabConModel();
void prefabLibraryCacheYValidacion();
void recargarNoCreaCarpeta();
void validarArchivoEstatico();
void instanciarPrefabInexistente();
void multiplesInstanciasMismoPrefab();
void prefabHijoConComponenteExtra();
void serviciosListarPrefabs();
void nombresDeComponentePortables();

#endif
