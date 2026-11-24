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
// Preambulo compartido por las unidades de InspeccionEscenaTests.cpp (generado al dividirlo).
#ifndef INSPECCIONESCENATESTSDETALLE_H
#define INSPECCIONESCENATESTSDETALLE_H

// Pruebas headless de la API de inspeccion de escena (src/Inspeccion).
//
// Cubren: las conversiones texto <-> valor, el catalogo de propiedades de los
// componentes, los filtros de objetos, las vistas, las mutaciones (crear
// objeto, etiquetar, agregar y quitar componentes, asignar variables) y que
// todas las mutaciones se puedan deshacer.
//
// No se requiere pila grafica ni AudioManager. La fisica no se instancia: los
// comandos toleran que el editor no tenga mundo de fisica.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "Assets/Mallas/AssetManager.h"
#include "Behaviour/ScriptGameObject.h"
#include "Events/EventBus.h"
#include "Inspeccion/ApiEscena.h"
#include "Inspeccion/InspeccionEscena.h"
#include "Inspeccion/PropiedadesComponentes.h"
#include "Inspeccion/TextoValor.h"
#include "Objetos/Componentes/Escena/Light.h"
#include "Objetos/Componentes/Escena/Transform.h"
#include "Objetos/GameObject.h"
#include "Scenes/Editor/EditorController.h"
#include "Scenes/Registro/SceneRegistry.h"
#include "Scenes/Serializacion/SceneSerializer.h"
#include "TempPruebas.h"

namespace fs = std::filesystem;

using MotorInspeccion::FiltroObjetos;

using MotorInspeccion::VistaComponente;

using MotorInspeccion::VistaObjeto;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Escena de trabajo: registro de objetos, controlador del editor y la API
// encima. Sin fisica ni eventos: la API no los necesita para leer ni para
// mutar.
struct Contexto {
    SceneRegistry escena;
    EditorController editor;
    MotorInspeccion::InspeccionEscena api;

    Contexto() : escena(), editor(&escena), api(&escena, &editor) {}
};

// Estado de la unidad original: una sola instancia (se define en InspeccionEscenaTests.cpp).

namespace InspeccionEscenaTestsDetalle {

extern int total;
extern int fallos;

}  // namespace InspeccionEscenaTestsDetalle
using namespace InspeccionEscenaTestsDetalle;

// Funciones repartidas entre las unidades.

int crearObjeto(Contexto& ctx, const std::string& nombre, int idPadre = 0);
std::string valorVariable(const std::vector<VistaComponente>& componentes, const std::string& tipo, const std::string& variable);
void probarTextoValor();
void probarCatalogo();
void probarFiltros();
void probarPosiciones();
void probarMutaciones();
void probarObjetos();
void probarVersion();
void probarGuardarCargar();

#endif
