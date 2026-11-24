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
#ifndef SETTINGSCOMPONENT_H
#define SETTINGSCOMPONENT_H
#include <string>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

class Component;

class SettingsComponent {
protected:
    bool mostrarVisualesDepuracion_ = true;
    std::string tipoComponente_; // nombre del tipo (demangle) para validacion segura
    // Componente concreto que edita el panel (lo fija el inspector al crearlo).
    // Con dos componentes del mismo tipo en el objeto, cada panel edita el
    // suyo y el inspector sabe cual quedo huerfano sin leer el puntero.
    Component* objetivo_ = nullptr;

    // El componente del panel como T: el fijado por el inspector o, si no hay
    // (paneles creados fuera del inspector), el primero de ese tipo del objeto.
    template <typename T, typename O>
    T* objetivo(O* objeto) const {
        if (objetivo_) return dynamic_cast<T*>(objetivo_);
        return objeto ? objeto->template getComponent<T>() : nullptr;
    }

public:
    SettingsComponent() = default;
    explicit SettingsComponent(const std::string& tipo) : tipoComponente_(tipo) {}
    virtual ~SettingsComponent() = default;
    virtual void showDataComponent() = 0;
    virtual Component* getComponent() = 0;
    // Los paneles que guardan un puntero a su componente lo toman de aqui.
    virtual void alFijarObjetivo() {}
    const std::string& getTipoComponente() const noexcept { return tipoComponente_; }
    void fijarObjetivo(Component* componente) {
        objetivo_ = componente;
        alFijarObjetivo();
    }
    // Componente del panel para el inspector: el fijado o, si no, getComponent().
    Component* componenteDelPanel() { return objetivo_ ? objetivo_ : getComponent(); }
    void setMostrarVisualesDepuracion(bool mostrar) noexcept {
        mostrarVisualesDepuracion_ = mostrar;
    }
};
#endif
