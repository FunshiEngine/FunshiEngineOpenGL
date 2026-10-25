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
#ifndef SCENESELECTEDINTERFACE_H
#define SCENESELECTEDINTERFACE_H
#include <iostream>
#include <string>
#include <memory>
#include "../ObjetosGUI/SettingsObjectInterface.h"
#include "../../Estructuras/Trees/ArbolesEnlazados/ArbolEnlazado.h"
#include "../../Objetos/GameObject.h"
#include "SceneObjectTree.h"

using namespace std;

// Forward declarations
class SceneRegistry;
class EditorController;
class EventBus;
class IconosGUI;

// Ventana "SelectedObjects": jerarquia de la escena.
//
// Es una ventana fina: la logica del arbol vive en el widget reutilizable
// SceneObjectTree y la seleccion en el EditorController (unica fuente de
// verdad, publicada via EventBus).
class SceneSelectedInterface : public GeneralUserInterface {
protected:
	// Scene integration
	SceneRegistry* scene = nullptr;
	EditorController* editor = nullptr;

	// Widget reutilizable de jerarquia (contenido de la ventana)
	SceneObjectTree sceneTree;

	IconosGUI* iconosGUI = nullptr;

public:
	SceneSelectedInterface(bool stateGUI);

	void bindScene(SceneRegistry* value, EditorController* controller, EventBus* bus);
	void setIconosGUI(IconosGUI* iconosG) {
		iconosGUI = iconosG;
		sceneTree.setIconosGUI(iconosG);
	}

	virtual ArbolEnlazado<GameObject*>* getEntitysTree();
	virtual void setEntitys(ListaDE<GameObject*>* gameObjects);
	virtual GameObject* getReturnableEntity();
	virtual void setReturnableEntity(GameObject* object);
	virtual ListaDE<GameObject*>* getGameObjects();

	virtual void initGUI() override;
	virtual void contentGUI() override;
	virtual void endGUI() override;
	virtual void printGUI() override;

	void createGameObject(GameObject* newGameObject);
	void createGameObject(std::unique_ptr<GameObject> object);
	void addChildGameObject(GameObject* parent, GameObject* object);
	void replaceRootGameObject(GameObject* newRoot);
	void clearGameObjects();
	void refreshGameObjectView();
	bool deleteObjectByID(int id);
};
#endif