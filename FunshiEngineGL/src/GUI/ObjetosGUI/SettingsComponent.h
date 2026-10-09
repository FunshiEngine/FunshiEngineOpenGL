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

public:
	SettingsComponent() = default;
	explicit SettingsComponent(const std::string& tipo) : tipoComponente_(tipo) {}
	virtual ~SettingsComponent() = default;
	virtual void showDataComponent() = 0;
	virtual Component* getComponent() = 0;
	const std::string& getTipoComponente() const noexcept { return tipoComponente_; }
	void setMostrarVisualesDepuracion(bool mostrar) noexcept {
		mostrarVisualesDepuracion_ = mostrar;
	}
};
#endif
