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
// Preambulo compartido por las unidades de ComandosTests.cpp (generado al dividirlo).
#ifndef COMANDOSTESTSDETALLE_H
#define COMANDOSTESTSDETALLE_H

// Pruebas headless del sistema undo/redo basado en el patron Command.
// Cubren: CrearObjetoComando, BorrarObjetoComando, ReparentarComando,
// TransformComando, AgregarComponenteComando, QuitarComponenteComando,
// LimpiarEscenaComando, DuplicarObjetoComando, AjustarVariableComando,
// EstablecerTagComando y la cadena de redo multiple del GestorComandos.
//
// Cada test crea una SceneRegistry + EditorController frescos, ejecuta un
// comando via GestorComandos, verifica el estado, deshace y verifica, luego
// rehace y verifica. No se requiere pila grafica ni AudioManager (la fisica
// Bullet si se usa en los casos con cuerpo, que es headless).
//
// Los comandos mutan por la puerta del editor (EditorController): desregistran
// y re-registran los cuerpos en fisica y limpian la seleccion, asi que el
// undo nunca deja cuerpos fantasma en el mundo.

#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <btBulletDynamicsCommon.h>
#include "Scenes/Registro/SceneRegistry.h"
#include "Scenes/Editor/EditorController.h"
#include "Comandos/Objetos/CrearObjetoComando.h"
#include "Comandos/Objetos/BorrarObjetoComando.h"
#include "Comandos/Objetos/ReparentarComando.h"
#include "Comandos/Transform/TransformComando.h"
#include "Comandos/Componentes/AgregarComponenteComando.h"
#include "Comandos/Componentes/QuitarComponenteComando.h"
#include "Comandos/Objetos/LimpiarEscenaComando.h"
#include "Comandos/Objetos/DuplicarObjetoComando.h"
#include "Comandos/Componentes/AjustarVariableComando.h"
#include "Comandos/Objetos/EstablecerTagComando.h"
#include "Comandos/GestorComandos.h"
#include "Fisicas/PhysicsEngine.h"
#include "Objetos/Componentes/Render/Model.h"
#include "Objetos/SimpleObject.h"
#include "Objetos/GameObject.h"
#include "Objetos/Componentes/Escena/Transform.h"
#include "Objetos/Componentes/Render/Color.h"
#include "Objetos/Componentes/Colliders/EsfereCollider.h"
#include "Objetos/Componentes/RigidBody/RigidBody.h"
#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Estado de la unidad original: una sola instancia (se define en ComandosTests.cpp).

namespace ComandosTestsDetalle {

extern int total;
extern int fallos;

}  // namespace ComandosTestsDetalle
using namespace ComandosTestsDetalle;

// Funciones repartidas entre las unidades.

int contarObjetos(SceneRegistry* scene);
GameObject* crearSimpleObject(SceneRegistry* scene, const char* name = "Obj");
void probarCrearObjeto();
void probarBorrarObjeto();
void probarReparentar();
void probarTransform();
void probarAgregarComponente();
void probarQuitarComponente();
void probarLimpiarEscena();
void probarRedoCadena();
GameObject* crearConCuerpo(SceneRegistry* scene, EditorController* editor, const char* nombre, float masa = 1.0f);
void probarBorrarConFisica();
void probarDuplicar();
void probarComponentesConFisica();
void probarAjustarVariable();
void probarEstablecerTag();

#endif
