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
#ifndef COLLIDER_H
#define COLLIDER_H
#include <memory>
#include "../Component.h"
#include "../Transform.h"

class btCollisionShape;
class GameObject;
class LineBatch;

// Collider: componente geometrico de colision.
//
// RAII: es duenio de su shape de Bullet (createCollisionShape la construye
// una sola vez, lazy) y de su transform local (myTransform). getGlobalTransform
// devuelve el Transform global POR VALOR: sin new/delete manuales.
//
// El collider pertenece a un GameObject (owner): getGlobalTransform compone
// owner->getGlobalTransform() (jerarquia completa con matrices) x myTransform.
// Sin owner se conserva el comportamiento antiguo (suma de posiciones, sin
// rotacion del padre) como respaldo.
class Collider : public Component {
protected:
	float radio;
	Transform* transformOfDadObject;
	std::unique_ptr<Transform> myTransform;
	std::unique_ptr<btCollisionShape> collisionShape;
	GameObject* owner = nullptr;

	// Batch de GPU con el wireframe del collider. Vive en la base para que los
	// tres tipos reutilicen el mismo recurso: la geometria casi no cambia (solo
	// el radio) y el dibujado es un unico draw por collider en vez de uno por
	// arista.
	// Va como puntero a incomplete type a proposito: Collider.h lo incluyen los
	// scripts, la serializacion y los targets headless, que no deben arrastrar
	// la pila de Rendering (LineBatch -> IRenderBackend -> glm). Se destruye en
	// ~Collider(), que esta definido en el .cpp.
	std::unique_ptr<LineBatch> wireBatch_;

	// Devuelve el batch del wireframe, creandolo la primera vez (el dibujo
	// ocurre con un contexto GL vivo).
	LineBatch& obtenerWireBatch();

	void serializeComponent(std::ofstream* fileNamePathContentObject) override;
	void deserializeComponent(std::ifstream* fileNamePathContentObject) override;

	// Construye la shape concreta del collider (se llama una sola vez).
	virtual std::unique_ptr<btCollisionShape> createCollisionShape() = 0;

public:
	Collider(float radio, Transform* transformOfDadObject,
	         GameObject* owner = nullptr);
	~Collider() override;

	void saveComponent(std::ofstream* fileNamePathContentObject) override;
	void loadComponent(std::ifstream* fileNamePathContentObject) override;

	// radio debe ser positivo
	virtual void setRadio(float radio);
	virtual float getRadio();

	Transform* getDadTransform() { return transformOfDadObject; }

	Transform* getTransform() { return myTransform.get(); }

	// GameObject dueño (escena). No duenio: vive en la lista de GameObject.
	GameObject* getOwner() const { return owner; }

	// Transform global POR VALOR. Composicion real con matrices si owner:
	// global = owner->getGlobalTransform() x myTransform. Sin owner = fallback
	// legacy (suma de posiciones, sin rotacion del padre).
	Transform getGlobalTransform() const;

	// Acceso no-duenio a la shape: la construye lazy y queda viva
	// mientras exista el collider.
	btCollisionShape* getCollisionShape();

	// Descarta la shape cacheada: se reconstruye lazy en el proximo
	// getCollisionShape() (se usa cuando cambia la malla del modelo).
	void invalidateCollisionShape();

	virtual void dibujarCollider() = 0;

	// seria ideal crear un metodo que recorra todos los objetos
	// obtenga sus collider y verifique si se chocan con el mio
	virtual bool isCollision(Collider* other);
};
#endif
