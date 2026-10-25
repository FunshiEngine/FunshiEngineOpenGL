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
#ifndef TRANSFORM_H
#define TRANSFORM_H

#include "../Componentes/Component.h"

// Las funciones libres de matrices se declaran aqui y se definen en
// Transform.cpp (evitan exponer glm en el header).
void buildMatrixFromTransform(class Transform* t, float outMatrix[16]);
void decomposeMatrixToTransform(float inMatrix[16], class Transform* t);

class Transform : public Component {
private:
	float objectTranslatef[3] = { 0.0f, 0.0f, 0.0f };
	float objectScalef[3] = { 1.0f, 1.0f, 1.0f };
	float objectRotatef[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	void serializeComponent(std::ofstream* fileNamePathContentObject) override;
	void deserializeComponent(std::ifstream* fileNamePathContentObject) override;

public:
	Transform();

	float arrTranslatef[3], arrScalef[3], arrRotatef[4];
	bool childsFreeze = false;

	// Gizmo "activo/dormido" (solo editor, NO se serializa: cambia el formato
	// binario de las escenas guardadas). true = el gizmo edita este transform
	// cuando corresponde (objeto seleccionado u offset de collider habilitado);
	// false = el gizmo se duerme y no aparece para este transform.
	bool gizmoHabilitado = true;

	void setTranslatef(float x, float y, float z);
	void setRotatef(float angle, float x, float y, float z);
	void setScalef(float x, float y, float z);

	float* getTranslatef() { return arrTranslatef; }
	float* getScalef() { return arrScalef; }
	float* getRotatef() { return arrRotatef; }

};
#endif