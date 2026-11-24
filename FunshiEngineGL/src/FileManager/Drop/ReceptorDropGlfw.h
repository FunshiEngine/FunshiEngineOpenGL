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
#ifndef RECEPTORDROPGLFW_H
#define RECEPTORDROPGLFW_H

#include <mutex>
#include <string>
#include <vector>

#include "IServicioDrop.h"

// GLFW solo se usa para el tipo de la ventana: se declara aqui para que la
// cabecera no dependa de la biblioteca de ventanas.
struct GLFWwindow;

// Recepcion de archivos soltados sobre la ventana del editor, con GLFW.
//
// GLFW entrega el drop desde su propio hilo de eventos, fuera del frame de
// la GUI: por eso el servicio encola las rutas y el explorador las drena en
// su frame, cuando ya sabe que carpeta esta viendo.
//
// GLFW no permite pasar datos de usuario al callback, asi que la instancia
// activa se guarda en una variable de archivo: el editor tiene una sola
// ventana y un solo receptor de drop.
class ReceptorDropGlfw : public IServicioDrop {
public:
    ReceptorDropGlfw();
    ~ReceptorDropGlfw() override;

    ReceptorDropGlfw(const ReceptorDropGlfw&) = delete;
    ReceptorDropGlfw& operator=(const ReceptorDropGlfw&) = delete;

    void registrar(void* ventana) override;
    std::vector<std::string> drenar() override;
    void desregistrar() override;

private:
    static void alSoltar(GLFWwindow* ventana, int conteo, const char** rutas);

    void encolar(const std::vector<std::string>& rutas);

    // El mutex protege la cola: el drop llega del hilo de eventos del
    // sistema y el drenado lo pide el frame de la GUI.
    std::mutex mutex;
    std::vector<std::string> pendientes;
    void* ventana = nullptr;
};

#endif
