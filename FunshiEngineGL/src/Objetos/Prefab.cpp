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
#include "Objetos/Componentes/Material.h"
#include "Objetos/Componentes/Light.h"
#include "Objetos/Componentes/Model.h"
#include "Objetos/Componentes/Skybox.h"
#include "Objetos/Componentes/AudioSource.h"
#include "Objetos/Componentes/InterfaceComponent.h"
#include "Objetos/Componentes/CameraComponent.h"
#include "Objetos/Componentes/Grid.h"

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

void escribirArbol(GameObject* obj, std::ofstream& out) {
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
            if (Component* c = n->getElement()) c->saveComponent(&out);
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
    std::uint32_t numComp = 0;
    in.read(reinterpret_cast<char*>(&numComp), sizeof(numComp));
    if (!in) return nullptr;

    // Crear el objeto base
    auto obj = std::make_unique<SimpleObject>();
    GameObject* crudo = obj.get();

    // Leer componentes
    for (std::uint32_t i = 0; i < numComp; ++i) {
        std::streampos antes = in.tellg();
        uint32_t magic = 0;
        in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
        in.seekg(antes);
        if (!in) break;

        // Usar ComponentFactory para crear por magic number
        // Como no tenemos el nombre de tipo, mapeamos magic -> tipo.
        // Cada componente serializa su magic number al inicio.
        std::unique_ptr<Component> comp = nullptr;
        switch (magic) {
            case 0x544E5253: // Transform "SRNT"
                comp = std::make_unique<Transform>();
                break;
            case 0x524F4C43: // Color "OLOR"
                comp = std::make_unique<Color>();
                break;
            case 0x54534D4D: // Material "MATS"
                comp = std::make_unique<Material>();
                break;
            case 0x5448474C: // Light "LGHT"
                comp = std::make_unique<Light>();
                break;
            case 0x4D524143: // Camera "CAMR"
                comp = std::make_unique<CameraComponent>();
                break;
            case 0x44495247: // Grid "GRID"
                comp = std::make_unique<Grid>();
                break;
            case 0x52435345: // EsfereCollider "ESCR"
                comp = std::make_unique<EsfereCollider>(1.0f, crudo->getComponent<Transform>(), crudo);
                break;
            case 0x45554243: // CubeCollider "CUBE"
                comp = std::make_unique<CubeCollider>(1.0f, crudo->getComponent<Transform>(), crudo);
                break;
            case 0x414C4C4D: // MallaCollider "MLLA"
                comp = std::make_unique<MallaCollider>(1.0f, crudo->getComponent<Transform>(), crudo);
                break;
            case 0x44425242: // RigidBody "BRBD"
                if (Collider* col = crudo->getComponent<Collider>())
                    comp = std::make_unique<RigidBody>(col, 1.0f);
                break;
            case 0x54504353: // Script "SCPT"
                comp = std::make_unique<Script>();
                break;
            case 0x44454C4F: // Model "MODEL"
                comp = std::make_unique<Model>();
                break;
            case 0x5842534B: // Skybox "SKYB"
                comp = std::make_unique<Skybox>();
                break;
            case 0x45525541: // AudioSource "AUDS"
                comp = std::make_unique<AudioSource>();
                break;
            case 0x4546414E: // InterfaceComponent "NFEI"
                comp = std::make_unique<InterfaceComponent>();
                break;
            default:
                break;
        }

        if (comp) {
            comp->loadComponent(&in);
            crudo->addComponent(std::move(comp));
        } else {
            // Componente desconocido: saltar sus bytes (no sabemos su tamaño)
            // En una implementación robusta cada componente debería escribir su tamaño
            // Por ahora asumimos que no hay componentes desconocidos en prefabs válidos
        }
    }

    // Leer tag, nombre, etc. del GameObject
    // La serialización de GameObject guarda inputName, tag, id, state...
    // Pero SimpleObject no sobrescribe saveComponent, así que usamos la de GameObject
    // Para simplificar, guardamos/cargamos solo lo esencial:
    // (En una versión completa, GameObject tendría savePrefab/loadPrefab)
    
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