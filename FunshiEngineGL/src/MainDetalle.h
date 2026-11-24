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
// Piezas del arranque del editor (main.cpp) separadas por tamano: comparten
// los includes, el estado global de la aplicacion y el contexto del editor.
#ifndef MAINDETALLE_H
#define MAINDETALLE_H

// GLFW solo para la ventana/callbacks (GLFW_INCLUDE_NONE: ningun GL aca).
#define GLFW_INCLUDE_NONE
#include "Configuracion/Proyecto/ArchivoProyecto.h"
#include <GLFW/glfw3.h>

// _HAS_STD_BYTE=0 DEBE ir ANTES de cualquier include de stdlib en Windows
// para evitar colision con typedef 'byte' de rpcndr.h vs std::byte (C++17)
#ifdef _WIN32
#define _HAS_STD_BYTE 0
#endif

#include "../src/Scenes/GameScene.h"
#include "../src/Audio/AudioEngine.h"
#include "../src/GUIManager/GUIManager.h"
#include "../src/Configuracion/EditorConfig.h"
#include "../src/Configuracion/ProjectPaths.h"
#include "../src/Configuracion/ProyectoInicial.h"
#include "../src/Configuracion/RutasLog.h"
#include "../src/Exportador/ConfigJuegoExportado.h"
#include "../src/GUI/WindowNames.h"
#include "../src/GUI/Tema/TemaEditor.h"
#include <imgui.h>
#include "EngineTime.h"
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include "../src/Objetos/GameObject.h"
#include "Objetos/Componentes/Escena/CameraComponent.h"
#include "../src/Proyectos/GestorDeProyectos.h"
#include "../src/States/ApplicationStateMachine.h"
#include "../src/States/OrquestadorEstadoGUI.h"
#include "../src/Behaviour/ScriptRuntime.h"
#include "../src/Events/EditorEventBus.h"
#include "../src/Ventana.h"
#include "../src/Input/EditorInput.h"
#include "../src/Hilos/HiloActual.h"
#include "../src/Rendering/Backend/IRenderBackend.h"
#include "Rendering/Cielo/FondoEscena.h"
#include "../src/Diagnosticos/RegistroDiagnosticos.h"
#include "Mods/Carga/GestorMods.h"
#include "Mods/Carga/CargadorMods.h"
#include "Mods/Api/DatosEscenaMods.h"
#include "Mods/Api/ServiciosMods.h"
#include "../src/ExcepcionesCPP/RuntimeException.h"
#ifdef FUNSHI_CEF
#include "../src/WebUI/WebUIRuntime.h"
#endif
#include "ImGuizmo.h"
#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <exception>
#include <typeinfo>
#include <utility>
#include <filesystem>
#include <string>

// Al cerrar la app, LeakSanitizer reporta una fuguita de 128 bytes de una
// libreria externa sin simbolos (GLFW/X11 o el driver de video) que se reserva
// al crear la ventana y no es imputable al codigo del motor. El cheq queda
// pendiente en atexit y no se puede suprimir desde adentro (ni __lsan_disable:
// la fuga ya existia cuando se llama). La solucion es terminar sin pasar por
// los handlers de atexit una vez que todo el cleanup ya corrió de forma
// explicita (escena, scripts JNI, ImGui, glfw). ASan sigue detectando usos
// invalidos en ejecución; solo se descarta el contador de fugas al salir.
// Nota: __has_feature solo existe en Clang; en GCC el "__has_feature(...)"
// del #if falla al parsear, por eso se anida con defined() antes de usarlo.
#if defined(__SANITIZE_ADDRESS__)
#define FUNSHI_ASAN_ACTIVO 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define FUNSHI_ASAN_ACTIVO 1
#else
#define FUNSHI_ASAN_ACTIVO 0
#endif
#else
#define FUNSHI_ASAN_ACTIVO 0
#endif

//VENTANA

//INPUT

// Variables de estado
// El estado abierto/cerrado del menu de inicio lo gobierna el modelo del
// paquete MenuGUI (MenuModel), sincronizado por frame desde la maquina de
// estados (fuente de verdad); no un bool suelto de main.
// Fuente de verdad del estado de la aplicacion (MainMenu / Editing). El menu
// de inicio (paquete MenuGUI) informa su decision y aqui se refleja.

// Maneja el FILE* de la redireccion de salida; el puntero debe vivir toda la
// app para que el flush de cierre de main escriba en el log.
extern int ventanaHeightEjeY, ventanaWidthEjeX;
extern bool recFilesInit;
extern ApplicationStateMachine appStateMachine;
extern OrquestadorEstadoGUI orquestadorDeGUI;
extern float deltaTime;
extern FILE* g_logSalida;

// Argumentos y resultado de la fase previa a la ventana.
struct ArgumentosEditor {
    std::string proyectoCLI;
    std::string rutaFnepCLI;
    std::string errorFnep;
    std::string rutaLog;
    bool modoJuegoExportado = false;
};

// Estado del editor creado en EjecutarMotor y usado por las demas fases.
struct ContextoEditor {
    Ventana* auxVentana = nullptr;
    GLFWwindow* window = nullptr;
    GUIManager* managerOfGUI = nullptr;
    GameScene* scene = nullptr;
    MenuGUI* mainMenu = nullptr;
    TreeFilesInterface* treeFilesInterface = nullptr;
    ContentFolderInterface* contentFolderInterface = nullptr;
    EditorInput* input = nullptr;
    EditorConfig* editorConfig = nullptr;
    GestorDeProyectos* gestor = nullptr;
    std::string* proyectoActual = nullptr;
    const ProyectoInicial::Resolucion* arranque = nullptr;
    std::vector<ModCargado>* modsCargados = nullptr;
    bool* menuReflejadoEnFachada = nullptr;
    const ArgumentosEditor* argumentos = nullptr;
    float FPS = 60.0f;
    bool imguiGl3Listo = false;
};

int prepararArranque(int argc, char* argv[], ArgumentosEditor& a);
void configurarGUI(ContextoEditor& c);
void ejecutarBucle(ContextoEditor& c);
int cerrarEditor(ContextoEditor& c);

#endif
