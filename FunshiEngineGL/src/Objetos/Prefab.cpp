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
#include "Prefab.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

#include "GameObject.h"
#include "SimpleObject.h"
#include "Scenes/EditorController.h"
#include "ClonadorObjetos.h"
#include "Configuracion/EditorConfig.h"
#include "Objetos/Componentes/ComponentFactory.h"
#include "Objetos/Componentes/Material.h"
#include "Objetos/Componentes/Light.h"
#include "Objetos/Componentes/Model.h"
#include "Objetos/Componentes/Skybox.h"
#include "Objetos/Componentes/AudioSource.h"
#include "Objetos/Componentes/InterfaceComponent.h"
#include "Objetos/Componentes/CameraComponent.h"
#include "Objetos/Componentes/Grid.h"
#include "Objetos/Componentes/Transform.h"
#include "Objetos/Componentes/Color.h"
#include "Objetos/Componentes/Script.h"
#include "Objetos/Componentes/Colliders/EsfereCollider.h"
#include "Objetos/Componentes/Colliders/CubeCollider.h"
#include "Objetos/Componentes/Colliders/MallaCollider.h"
#include "Objetos/Componentes/RigidBody/RigidBody.h"

namespace fs = std::filesystem;

// Formato .prefab:
//   uint32_t magic = 0x42415250 ("PREB")
//   uint32_t version = 1
//   uint32_t numObjetos
//   por cada objeto:
//     uint32_t numComponentes
//     por cada componente: serializacion binaria del componente (saveComponent)
//     uint32_t numHijos
//     (recursivo)

namespace {
constexpr std::uint32_t PREFAB_MAGIC = 0x42415250;
constexpr std::uint32_t PREFAB_VERSION = 1;

// Constantes para serializacion de tag (coinciden con GameObject.cpp)
constexpr std::uint32_t TAG_MAGIC = 0x31474154;
constexpr std::uint32_t TAG_VERSION = 1;
constexpr std::size_t TAG_MAX_LENGTH = 1024;

struct GameObjectProps {
    bool state = true;
    int id = 0;
    int tam = 1;
    char inputName[25] = "";
    std::string tag = "Untagged";
};

void escribirGameObjectProps(GameObject* obj, std::ofstream& out) {
    // state (bool)
    bool state = obj->getState();
    out.write(reinterpret_cast<const char*>(&state), sizeof(state));
    // id (int)
    int id = obj->getId();
    out.write(reinterpret_cast<const char*>(&id), sizeof(id));
    // tam (int)
    int tam = obj->getTam();
    out.write(reinterpret_cast<const char*>(&tam), sizeof(tam));
    // inputName (char[25])
    out.write(obj->inputName, sizeof(obj->inputName));
    // tag (magic, version, length, data)
    const std::string& tag = obj->getTag();
    std::uint32_t length = static_cast<std::uint32_t>(tag.size());
    out.write(reinterpret_cast<const char*>(&TAG_MAGIC), sizeof(TAG_MAGIC));
    out.write(reinterpret_cast<const char*>(&TAG_VERSION), sizeof(TAG_VERSION));
    out.write(reinterpret_cast<const char*>(&length), sizeof(length));
    if (length > 0) {
        out.write(tag.data(), static_cast<std::streamsize>(length));
    }
}

GameObjectProps leerGameObjectProps(std::ifstream& in) {
    GameObjectProps props;
    in.read(reinterpret_cast<char*>(&props.state), sizeof(props.state));
    in.read(reinterpret_cast<char*>(&props.id), sizeof(props.id));
    in.read(reinterpret_cast<char*>(&props.tam), sizeof(props.tam));
    in.read(props.inputName, sizeof(props.inputName));
    // tag
    std::uint32_t magic = 0, version = 0, length = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    in.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (in && magic == TAG_MAGIC && version == TAG_VERSION && length > 0 && length <= TAG_MAX_LENGTH) {
        props.tag.resize(length);
        in.read(props.tag.data(), static_cast<std::streamsize>(length));
    } else {
        props.tag = "Untagged";
        // Si el tag es invalido, intentar recuperar el stream
        if (magic != TAG_MAGIC || version != TAG_VERSION) {
            // Retroceder para no perder datos de componentes
            in.seekg(-static_cast<std::streamoff>(sizeof(magic) + sizeof(version) + sizeof(length)), std::ios::cur);
        }
    }
    return props;
}

// Obtiene el nombre de tipo demangleado del componente (consistente con ComponentFactory y serializaciÃ³n de escena)
namespace {
std::string obtenerNombreComponente(const Component* comp) {
    const std::type_info& ti = typeid(*comp);
    const char* name = ti.name();
    // RTTI names (GCC/Clang): "N8Objetos10Componentes9TransformE", "N8Objetos10Componentes5ColorE", etc.
    std::string typeName(name);
    // Extraer nombre simple (despuÃ©s de :: final)
    size_t pos = typeName.find_last_of(':');
    if (pos != std::string::npos) typeName = typeName.substr(pos + 1);
    // Eliminar prefijos de namespace comunes (GCC/Clang mangling)
    if (typeName.rfind("N8Objetos10Componentes", 0) == 0) {
        typeName = typeName.substr(21);
    }
    // Eliminar prefijo de longitud (dÃ­gitos iniciales que indican la longitud del nombre)
    // Ej: "9TransformE" -> "TransformE"
    while (!typeName.empty() && std::isdigit(typeName[0])) {
        typeName = typeName.substr(1);
    }
    // Eliminar sufijos de template/size
    size_t end = typeName.find('E');
    if (end != std::string::npos) typeName = typeName.substr(0, end);
    return typeName;
}
} // namespace

void escribirArbol(GameObject* obj, std::ofstream& out) {
    // Propiedades del GameObject (state, id, tam, inputName, tag)
    escribirGameObjectProps(obj, out);

    // Componentes
    std::uint32_t numComp = 0;
    if (obj->getComponents() && !obj->getComponents()->isEmpty()) {
        for (auto* n = obj->getComponents()->first(); n;
             n = (n != obj->getComponents()->last()) ? obj->getComponents()->next(n) : nullptr) {
            if (n->getElement()) ++numComp;
        }
    }
    out.write(reinterpret_cast<const char*>(&numComp), sizeof(numComp));
    if (numComp > 0) {
        for (auto* n = obj->getComponents()->first(); n;
             n = (n != obj->getComponents()->last()) ? obj->getComponents()->next(n) : nullptr) {
            if (Component* c = n->getElement()) {
                // Escribir nombre de tipo (length + string) antes de sus datos
                // Formato consistente con GameObject::serializeEntityComponents
                std::string typeName = obtenerNombreComponente(c);
                std::uint32_t length = static_cast<std::uint32_t>(typeName.size());
                out.write(reinterpret_cast<const char*>(&length), sizeof(length));
                out.write(typeName.c_str(), length);
                c->saveComponent(&out);
            }
        }
    }
    // Hijos
    std::uint32_t numHijos = 0;
    for (Entity* hijo : obj->getChildEntities()) {
        if (dynamic_cast<GameObject*>(hijo)) ++numHijos;
    }
    out.write(reinterpret_cast<const char*>(&numHijos), sizeof(numHijos));
    for (Entity* hijo : obj->getChildEntities()) {
        if (GameObject* go = dynamic_cast<GameObject*>(hijo)) {
            escribirArbol(go, out);
        }
    }
}

GameObject* leerArbol(std::ifstream& in, EditorController* editor, GameObject* padre) {
    // Leer propiedades del GameObject (state, id, tam, inputName, tag)
    GameObjectProps props = leerGameObjectProps(in);
    if (!in) return nullptr;

    // Leer número de componentes
    std::uint32_t numComp = 0;
    in.read(reinterpret_cast<char*>(&numComp), sizeof(numComp));
    if (!in) return nullptr;

    // Crear el objeto base
    auto obj = std::make_unique<SimpleObject>();
    GameObject* crudo = obj.get();

    // Aplicar propiedades del GameObject
    crudo->setState(props.state);
    // No preservar el ID original: al instanciar un prefab, el SceneRegistry
    // asigna un ID nuevo unico. El ID guardado es solo para referencia.
    crudo->setId(0);
    crudo->setTam(props.tam);
    std::snprintf(crudo->inputName, sizeof(crudo->inputName), "%s", props.inputName);
    crudo->setTag(props.tag);

    // Leer componentes
    for (std::uint32_t i = 0; i < numComp; ++i) {
        // Leer nombre de tipo (length + string)
        std::uint32_t length = 0;
        in.read(reinterpret_cast<char*>(&length), sizeof(length));
        if (!in || length == 0 || length > 256) {
            return nullptr;
        }
        std::string typeName(length, '\0');
        in.read(&typeName[0], static_cast<std::streamsize>(length));
        if (!in) return nullptr;

        // Crear componente usando ComponentFactory (consistente con deserializaciÃ³n de escena)
        std::unique_ptr<Component> comp = nullptr;

        // Caso especial: Transform - el objeto ya tiene uno de Entity
        if (typeName == "Transform") {
            if (Transform* t = crudo->getComponent<Transform>()) {
                t->loadComponent(&in);
            }
        } else {
            // Usar ComponentFactory para los demÃ¡s
            comp = ComponentFactory::create(typeName, *crudo);
            if (comp) {
                comp->loadComponent(&in);
                crudo->addComponent(std::move(comp));
            } else {
                return nullptr;
            }
        }
    }

    // Insertar en la escena
    GameObject* insertado = editor->createGameObject(std::move(obj), padre);
    if (!insertado) return nullptr;

    // Leer hijos recursivamente
    std::uint32_t numHijos = 0;
    in.read(reinterpret_cast<char*>(&numHijos), sizeof(numHijos));
    for (std::uint32_t i = 0; i < numHijos; ++i) {
        leerArbol(in, editor, insertado);
    }
    return insertado;
}
} // namespace

Prefab::Prefab(const std::string& nombre)
    : nombre_(nombre), rutaArchivo_(rutaParaNombre(nombre)) {}

std::string Prefab::rutaParaNombre(const std::string& nombre) const {
    fs::path base = EditorConfig::raizAssetsFijada();
    fs::path dir = base / "Prefabs";
    fs::create_directories(dir);
    return (dir / (nombre + ".prefab")).string();
}

bool Prefab::guardarDesdeObjeto(GameObject* raiz, EditorController* editor) {
    (void)editor; // no usado en esta versión simple
    std::ofstream out(rutaArchivo_, std::ios::binary);
    if (!out) return false;
    
    out.write(reinterpret_cast<const char*>(&PREFAB_MAGIC), sizeof(PREFAB_MAGIC));
    out.write(reinterpret_cast<const char*>(&PREFAB_VERSION), sizeof(PREFAB_VERSION));
    
    escribirArbol(raiz, out);
    return static_cast<bool>(out);
}

GameObject* Prefab::instanciar(EditorController* editor, GameObject* padre) {
    if (!editor) return nullptr;
    if (!existeArchivo()) return nullptr;
    
    std::ifstream in(rutaArchivo_, std::ios::binary);
    if (!in) return nullptr;
    
    std::uint32_t magic = 0, version = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (magic != PREFAB_MAGIC || version != PREFAB_VERSION) return nullptr;
    
    return leerArbol(in, editor, padre);
}

bool Prefab::existeArchivo() const noexcept {
    return fs::exists(rutaArchivo_);
}

bool Prefab::validarArchivo(const std::string& ruta) {
    std::ifstream in(ruta, std::ios::binary);
    if (!in) return false;

    // Verificar magic number
    std::uint32_t magic = 0, version = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (!in) return false;
    if (magic != PREFAB_MAGIC || version != PREFAB_VERSION) return false;

    // Verificar que hay al menos un objeto (numObjetos > 0)
    std::uint32_t numObjetos = 0;
    in.read(reinterpret_cast<char*>(&numObjetos), sizeof(numObjetos));
    if (!in || numObjetos == 0) return false;

    return true;
}