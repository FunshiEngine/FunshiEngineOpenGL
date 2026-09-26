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
#ifndef COMPONENT_H
#define COMPONENT_H
#include "../../GestorDeArchivos/Binario.h"
using namespace std;

// Declaracion adelantada: onLoaded() recibe el dueno sin crear un ciclo de
// includes (GameObject.h incluye los componentes; Component.h no lo incluye).
class GameObject;

class Component {
private:
	bool terminalSelectScript = false;
	char name[25] = "";
protected:
	virtual void serializeComponent(std::ofstream* fileNamePathContentObject) = 0;

	virtual void deserializeComponent(std::ifstream* fileNamePathContentObject) = 0;
public:
	bool settingsObjectComponent = false;
	virtual ~Component() {} // Destructor virtual para hacer que la clase sea polim�rfica.

	virtual void saveComponent(std::ofstream* fileNamePathContentObject) = 0;

	virtual void loadComponent(std::ifstream* fileNamePathContentObject) = 0;

	// Hook polimorfico de post-carga: se invoca desde deserializeEntityComponents
	// justo despues de loadComponent() y antes de entregar el componente al
	// objeto. Los componentes lo usan para aplicar su estado al dueno sin que
	// el GameObejct tenga que despachar por nombre de tipo.
	virtual void onLoaded(GameObject& /*owner*/) {}

};
#endif
