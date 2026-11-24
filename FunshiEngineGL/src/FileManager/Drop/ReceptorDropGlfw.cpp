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
#include "ReceptorDropGlfw.h"

#include <GLFW/glfw3.h>

namespace {

// GLFW no pasa datos de usuario al callback: la instancia activa se guarda
// aqui. El editor tiene una sola ventana y un solo receptor.
ReceptorDropGlfw* g_receptorActivo = nullptr;

}  // namespace

ReceptorDropGlfw::ReceptorDropGlfw() = default;

ReceptorDropGlfw::~ReceptorDropGlfw() { desregistrar(); }

void ReceptorDropGlfw::registrar(void* ventana) {
    this->ventana = ventana;
    g_receptorActivo = this;
    glfwSetDropCallback(static_cast<GLFWwindow*>(ventana),
                        &ReceptorDropGlfw::alSoltar);
}

void ReceptorDropGlfw::desregistrar() {
    if (ventana != nullptr) {
        glfwSetDropCallback(static_cast<GLFWwindow*>(ventana), nullptr);
    }
    ventana = nullptr;
    if (g_receptorActivo == this) g_receptorActivo = nullptr;
}

std::vector<std::string> ReceptorDropGlfw::drenar() {
    std::lock_guard<std::mutex> bloque(mutex);
    std::vector<std::string> resultado;
    resultado.swap(pendientes);
    return resultado;
}

void ReceptorDropGlfw::encolar(const std::vector<std::string>& rutas) {
    std::lock_guard<std::mutex> bloque(mutex);
    pendientes.insert(pendientes.end(), rutas.begin(), rutas.end());
}

void ReceptorDropGlfw::alSoltar(GLFWwindow* /*ventana*/, int conteo,
                                 const char** rutas) {
    if (g_receptorActivo == nullptr || conteo <= 0 || rutas == nullptr) return;

    std::vector<std::string> nuevas;
    nuevas.reserve(static_cast<std::size_t>(conteo));
    for (int i = 0; i < conteo; ++i) {
        if (rutas[i] != nullptr) nuevas.emplace_back(rutas[i]);
    }
    if (nuevas.empty()) return;
    g_receptorActivo->encolar(nuevas);
}
