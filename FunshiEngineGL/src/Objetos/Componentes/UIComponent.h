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
#ifndef UICOMPONENT_H
#define UICOMPONENT_H

#include "Component.h"

#include "../../UI/LienzoUI.h"
#include "../../UI/SistemaUI.h"

#include <string>

// Interfaz de juego NATIVA como Component: se adjunta a un GameObject y, en
// modo juego, la escena dibuja delante de la camara los widgets que guarda
// (etiquetas, botones, imagenes y paneles), sin navegador web de por medio.
//
// El componente guarda dos cosas:
//   - el lienzo (LienzoUI): los widgets y su colocacion. Es lo que se
//     serializa en la escena.
//   - el estado de interaccion (SistemaUI): que widget tiene el raton encima
//     y cual se pulso. Es estado vivo: no se serializa, se reconstruye en cada
//     frame a partir de la entrada.
//
// Una escena puede tener varias interfaces nativas: se dibujan las activas
// ordenadas por `z`.
class UIComponent : public Component {
private:
	// Datos del HUD (se serializan).
	LienzoUI lienzo_;
	// Estado vivo de la interaccion (no se serializa).
	SistemaUI sistema_;
	bool activo = true;
	float z = 0.0f; // orden de apilado entre interfaces nativas

protected:
	void serializeComponent(std::ofstream* file) override;
	void deserializeComponent(std::ifstream* file) override;

public:
	UIComponent() = default;

	// Lienzo: la lista de widgets y su colocacion.
	LienzoUI& lienzo() noexcept { return lienzo_; }
	const LienzoUI& lienzo() const noexcept { return lienzo_; }

	// Estado de interaccion del frame.
	SistemaUI& sistema() noexcept { return sistema_; }
	const SistemaUI& sistema() const noexcept { return sistema_; }

	bool estaActivo() const noexcept { return activo; }
	void setActivo(bool valor) noexcept { activo = valor; }

	float getZ() const noexcept { return z; }
	void setZ(float valor) noexcept { z = valor; }

	// Coloca los widgets para una pantalla de ese tamano y actualiza el estado
	// de interaccion con la entrada del frame. Lo llama la escena antes de
	// dibujar y antes de que corran los scripts, para que un script vea el
	// clic en el mismo frame en que ocurre.
	void actualizarInteraccion(const EntradaUI& entrada, float anchoPantalla,
	                           float altoPantalla);
};

#endif // UICOMPONENT_H
