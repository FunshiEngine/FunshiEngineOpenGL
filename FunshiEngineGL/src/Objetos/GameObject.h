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
#ifndef GAMEOBJECT_H
#define GAMEOBJECT_H

#include <cfloat>
#include <memory>
#include <string>

#include "../Herramientas/TypeUtils.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Estructuras/Comparable.h"
#include "../Entity/Entity.h"

// El objeto del dominio no conoce los componentes concretos: solo los declara
// para poder apuntarlos. Quien los usa incluye su propia cabecera (y el
// CatalogoComponentes es quien los crea por nombre).
class Color;

class GameObject : public Comparable<GameObject>, public Entity {
protected:
    bool state = true;
    int id = 0;
    int tam = 1;
    std::string tag_ = "Untagged";
    bool dirty_ = false;  // Para autosave de escena
public:
    explicit GameObject(Entity* origin);
    GameObject();
    ~GameObject() override;

    char inputName[25] = "";
    color auxColor = {0.0f, 0.0f, 0.0f, 1.0f};

    int compareTo(GameObject* other) override;
    float distanciaA(GameObject* other);
    void setId(int id);
    void setState(bool state);
    void setTam(int tam);
    void setColor(color cor);
    const float* getColor(float*& arr, int tam);
    Color* getColor();
    int getId();
    bool getState();
    int getTam();
    const std::string& getTag() const noexcept { return tag_; }
    void setTag(std::string tag);

    // El dibujado no es responsabilidad de la entidad: SceneRenderer recorre la
    // escena y dibuja cada Modelos3D con MeshRenderer (VBO/VAO + shader). Asi
    // la entidad no necesita conocer la capa de Rendering.
    virtual void update(float deltaTime);

    // Dirty tracking para autosave de escena
    bool isDirty() const { return dirty_; }
    void setDirty(bool value) { dirty_ = value; }

protected:
    // Atributos propios del objeto de juego (id, nombre, estado, tam y tag):
    // los escribe el objeto porque su formato es parte del binario de escena.
    void serializeLocalAtributes() override;
    void deserializeLocalAtributes() override;

public:
    bool saveEntity(std::string filename) override;
    bool loadEntity(std::string filename) override;
};

#endif
