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
// Preambulo compartido por las unidades de ScriptGameObject.cpp (generado al dividirlo).
#ifndef SCRIPTGAMEOBJECTDETALLE_H
#define SCRIPTGAMEOBJECTDETALLE_H

#include "ScriptGameObject.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "ScriptAudioHandles.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/GameObjectFactory.h"
#include "../Objetos/ClonadorObjetos.h"
#include "../Objetos/NombreUnico.h"
#include "../Objetos/Componentes/Escena/Transform.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../Audio/AudioEngine.h"
#include "../Configuracion/EditorConfig.h"
#include "../Scenes/Registro/SceneRegistry.h"
#include "../Scenes/Editor/EditorController.h"
#include "../Objetos/Prefab.h"
#include "../Objetos/PrefabLibrary.h"
#include "../Fisicas/PhysicsEngine.h"
#include "../Input/InputScripts.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../Objetos/Componentes/Interfaz/UIComponent.h"
#include "../WebUI/EstadoInterfaces.h"

namespace MotorScript {

namespace {

const char* nombreDelObjeto(const void* objeto) {
    const GameObject* o = static_cast<const GameObject*>(objeto);
    return o ? o->inputName : "";
}

}  // namespace

namespace ScriptGameObjectDetalle {

// ============================================================================
// Gestion de objetos desde scripts (v4). Toda mutacion estructural es
// DIFERIDA al final del frame (procesarPeticionesObjetos): crear o borrar
// durante el recorrido de actualizacion reconstruiria la vista lineal
// (refreshGameObjectView) e invalidaria el iterador en curso.
// ============================================================================
struct AltaPendiente {
    std::unique_ptr<GameObject> objeto;
    GameObject* padre = nullptr;
};

struct ComponentePendiente {
    GameObject* objeto = nullptr;
    std::unique_ptr<Component> componente;
};

}  // namespace ScriptGameObjectDetalle
using namespace ScriptGameObjectDetalle;

}  // namespace MotorScript

// Auxiliares repartidas entre las unidades (enlace externo).

namespace MotorScript {

int serviciosReproducirSonido(const char* ruta, float volumen, bool bucle);
void serviciosDetenerSonido(int handle);
bool serviciosReproducirMusica(const char* ruta, float volumen, float fundido);
void serviciosDetenerMusica(float fundido);
void serviciosVolumenMusica(float volumen);
void* serviciosObjetoPorNombre(const char* nombre);
void* serviciosObjetoPorId(int id);
void* serviciosObjetoPorEtiqueta(const char* etiqueta);
bool serviciosTeclaSostiene(const char* tecla);
bool serviciosTeclaPresionada(const char* tecla);
bool serviciosTeclaSoltada(const char* tecla);
float serviciosDeltaMouseX();
float serviciosDeltaMouseY();
void serviciosFijarGravedadGlobal(float x, float y, float z);
float serviciosGravedadGlobalEje(int eje);
void* serviciosCrearObjeto(const char* nombre, void* padre);
bool serviciosDestruirObjeto(void* objeto);
void* serviciosClonarObjeto(const void* original, void* padre);
bool serviciosAgregarColliderEsfera(void* objeto, float radio);
bool serviciosAgregarColliderCubo(void* objeto, float radio);
bool serviciosAgregarRigidBody(void* objeto, float masa);
void* serviciosObjetoDeCollider(const void* collider);
bool serviciosRaycast(float ox, float oy, float oz, float dx, float dy, float dz, float alcance, float* distancia, float* px, float* py, float* pz, float* nx, float* ny, float* nz, void** collider);
float serviciosPuntoImpactoX();
float serviciosPuntoImpactoY();
float serviciosPuntoImpactoZ();
float serviciosNormalImpactoX();
float serviciosNormalImpactoY();
float serviciosNormalImpactoZ();
float serviciosDistanciaImpacto();
void* serviciosInstanciarPrefab(const char* nombre, void* padre);
const char** serviciosListarPrefabs();
void* serviciosInterfazDeObjeto(const char* nombreObjeto);
bool serviciosInterfazActiva(const void* interfaz);
void serviciosInterfazEstablecerActiva(void* interfaz, bool activa);
bool serviciosInterfazEstablecerEstado(void* interfaz, const char* jsonEstado);
bool serviciosInterfazEstablecerValor(void* interfaz, const char* clave, const char* valorJson);
const char* serviciosInterfazValor(void* interfaz, const char* rutaJson);
void* serviciosInterfazNativaDeObjeto(const char* nombreObjeto);
bool serviciosInterfazNativaVisible(const void* interfaz);
void serviciosInterfazNativaEstablecerVisible(void* interfaz, bool visible);
bool serviciosInterfazNativaEstablecerTexto(void* interfaz, const char* idWidget, const char* texto);
bool serviciosInterfazNativaEstablecerVisibleWidget(void* interfaz, const char* idWidget, bool visible);
bool serviciosInterfazNativaBotonPulsado(void* interfaz, const char* idWidget);

}  // namespace MotorScript

// Estado de la unidad original: una sola instancia (se define en ScriptGameObject.cpp).

namespace MotorScript {

namespace ScriptGameObjectDetalle {

extern AudioEngine* g_audio;
extern SceneRegistry* g_escena;
extern InputScripts* g_input;
extern PhysicsEngine* g_fisica;
extern EditorController* g_editor;
extern EstadoInterfaces* g_interfaces;
extern ScriptAudioHandles g_sonidosScript;
extern std::vector<AltaPendiente> altasPendientes_;
extern std::vector<ComponentePendiente> componentesPendientes_;
extern std::vector<GameObject*> bajasPendientes_;

}  // namespace ScriptGameObjectDetalle
using namespace ScriptGameObjectDetalle;

}  // namespace MotorScript

#endif
