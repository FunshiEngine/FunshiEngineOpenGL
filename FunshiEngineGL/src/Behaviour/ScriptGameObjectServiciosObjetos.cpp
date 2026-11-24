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
#include "ScriptGameObjectDetalle.h"

namespace MotorScript {

namespace {

// Nombre libre en la escena: contempla la escena y las altas pendientes (aun
// no insertadas). `tomados` se completa con el elegido.
std::string nombreLibrePara(const std::string& base,
                            std::vector<std::string>& tomados) {
    return ClonadorObjetos::nombreLibre(base, tomados);
}

std::vector<std::string> nombresTomados() {
    std::vector<std::string> tomados;
    if (g_escena) tomados = NombresUnicos::enUso(g_escena->getRoot());
    for (const AltaPendiente& alta : altasPendientes_) {
        if (alta.objeto && alta.objeto->inputName[0] != '\0')
            tomados.emplace_back(alta.objeto->inputName);
    }
    return tomados;
}

}  // namespace

void* serviciosCrearObjeto(const char* nombre, void* padre) {
    if (!g_editor || !g_escena) return nullptr;
    GameObject* padreObj = static_cast<GameObject*>(padre);
    if (padreObj && !g_escena->contains(padreObj)) padreObj = nullptr;
    std::unique_ptr<GameObject> objeto = GameObjectFactory::createSimpleObject(
        padreObj ? padreObj : g_escena->getRoot());
    if (nombre && *nombre) {
        std::vector<std::string> tomados = nombresTomados();
        const std::string unico = nombreLibrePara(nombre, tomados);
        std::snprintf(objeto->inputName, sizeof(objeto->inputName), "%s",
                      unico.c_str());
    }
    GameObject* crudo = objeto.get();
    altasPendientes_.push_back({std::move(objeto), padreObj});
    return crudo;
}

bool serviciosDestruirObjeto(void* objeto) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!g_editor || !g_escena || !o || !g_escena->contains(o)) return false;
    for (GameObject* marcado : bajasPendientes_) {
        if (marcado == o) return true;
    }
    bajasPendientes_.push_back(o);
    return true;
}

void* serviciosClonarObjeto(const void* original, void* padre) {
    GameObject* o = const_cast<GameObject*>(
        static_cast<const GameObject*>(original));
    if (!g_editor || !g_escena || !o || !g_escena->contains(o)) return nullptr;
    if (o == g_escena->getRoot()) return nullptr;
    GameObject* padreObj = static_cast<GameObject*>(padre);
    if (padreObj && !g_escena->contains(padreObj)) padreObj = nullptr;
    // Copia profunda (componentes y subarbol) sin insertar: entra a la escena
    // al final del frame con el resto de las altas pendientes.
    auto nodos =
        ClonadorObjetos::clonar(o, padreObj, nombresTomados());
    if (nodos.empty()) return nullptr;
    GameObject* crudo = nodos.front().first.get();
    for (auto& nodo : nodos)
        altasPendientes_.push_back(
            {std::move(nodo.first), nodo.second});
    return crudo;
}

namespace {

bool encolarComponente(GameObject* o, std::unique_ptr<Component> componente) {
    if (!g_editor || !g_escena || !o || !g_escena->contains(o) ||
        !componente)
        return false;
    componentesPendientes_.push_back({o, std::move(componente)});
    return true;
}

// El componente encolado aun no esta en el objeto: sin mirar la cola, dos
// pedidos del mismo frame entrarian igual y el segundo se descartaria mudo.
bool componentePendiente(GameObject* o, bool collider) {
    for (const ComponentePendiente& item : componentesPendientes_) {
        if (item.objeto != o || !item.componente) continue;
        if (collider && dynamic_cast<Collider*>(item.componente.get()))
            return true;
        if (!collider && dynamic_cast<RigidBody*>(item.componente.get()))
            return true;
    }
    return false;
}

}  // namespace

bool serviciosAgregarColliderEsfera(void* objeto, float radio) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(radio) || radio <= 0.0f) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    if (o->getComponent<Collider>() || componentePendiente(o, true))
        return false;
    Transform* t = o->getComponent<Transform>();
    if (!t) return false;
    return encolarComponente(
        o, std::make_unique<EsfereCollider>(radio, t, o));
}

bool serviciosAgregarColliderCubo(void* objeto, float radio) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(radio) || radio <= 0.0f) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    if (o->getComponent<Collider>() || componentePendiente(o, true))
        return false;
    Transform* t = o->getComponent<Transform>();
    if (!t) return false;
    return encolarComponente(o,
                             std::make_unique<CubeCollider>(radio, t, o));
}

bool serviciosAgregarRigidBody(void* objeto, float masa) {
    GameObject* o = static_cast<GameObject*>(objeto);
    if (!o || !std::isfinite(masa)) return false;
    if (!g_escena || !g_escena->contains(o)) return false;
    Collider* collider = o->getComponent<Collider>();
    if (!collider || o->getComponent<RigidBody>() ||
        componentePendiente(o, false))
        return false;
    if (masa < 0.0f) masa = 0.0f;
    return encolarComponente(o,
                             std::make_unique<RigidBody>(collider, masa));
}

// --- v5: colisiones y raycast ---
void* serviciosObjetoDeCollider(const void* collider) {
    const Collider* c = static_cast<const Collider*>(collider);
    return c ? c->getOwner() : nullptr;
}

namespace {

// Ultimo resultado de raycast (almacenado para consultas posteriores).
ResultadoRaycast g_ultimoRaycast;

}  // namespace

bool serviciosRaycast(float ox, float oy, float oz,
                      float dx, float dy, float dz,
                      float alcance,
                      float* distancia, float* px, float* py, float* pz,
                      float* nx, float* ny, float* nz,
                      void** collider) {
    g_ultimoRaycast = ResultadoRaycast{};
    if (!g_fisica) return false;

    ResultadoRaycast r = g_fisica->raycast(ox, oy, oz, dx, dy, dz, alcance);
    g_ultimoRaycast = r;

    if (!r.impacto) return false;

    if (distancia) *distancia = r.distancia;
    if (px) *px = r.puntoX;
    if (py) *py = r.puntoY;
    if (pz) *pz = r.puntoZ;
    if (nx) *nx = r.normalX;
    if (ny) *ny = r.normalY;
    if (nz) *nz = r.normalZ;
    if (collider) *collider = r.collider;
    return true;
}

float serviciosPuntoImpactoX() { return g_ultimoRaycast.puntoX; }

float serviciosPuntoImpactoY() { return g_ultimoRaycast.puntoY; }

float serviciosPuntoImpactoZ() { return g_ultimoRaycast.puntoZ; }

float serviciosNormalImpactoX() { return g_ultimoRaycast.normalX; }

float serviciosNormalImpactoY() { return g_ultimoRaycast.normalY; }

float serviciosNormalImpactoZ() { return g_ultimoRaycast.normalZ; }

float serviciosDistanciaImpacto() { return g_ultimoRaycast.distancia; }

void* serviciosInstanciarPrefab(const char* nombre, void* padre) {
    if (!g_editor || !g_escena || !nombre || !*nombre) return nullptr;
    GameObject* padreObj = static_cast<GameObject*>(padre);
    if (padreObj && !g_escena->contains(padreObj)) padreObj = nullptr;
    if (!g_editor->getPrefabLibrary()) return nullptr;
    Prefab* prefab = g_editor->getPrefabLibrary()->obtener(nombre);
    if (!prefab) return nullptr;
    GameObject* instancia = prefab->instanciar(g_editor, padreObj);
    return instancia;
}

const char** serviciosListarPrefabs() {
    if (!g_editor || !g_editor->getPrefabLibrary()) return nullptr;
    // Los nombres son copias estables hasta la llamada siguiente: listarNombres()
    // devuelve un vector por valor, asi que sus c_str() se copian a un storage
    // estatico y recien despues se construye la vista que se devuelve.
    static std::vector<std::string> nombresEstables;
    static std::vector<const char*> vista;
    nombresEstables = g_editor->getPrefabLibrary()->listarNombres();
    vista.clear();
    vista.reserve(nombresEstables.size() + 1);
    for (const auto& nombre : nombresEstables)
        vista.push_back(nombre.c_str());
    vista.push_back(nullptr);
    return vista.data();
}

}  // namespace MotorScript
