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
#include "PreviewMaterial.h"

#include "../../Assets/Mesh.h"
#include "../../Assets/Primitivas3D.h"
#include "../RenderTarget.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

PreviewMaterial::PreviewMaterial(Rendering::Backend::IRenderBackend& backend,
                                 Rendering::GestorShaders* gestorShaders,
                                 TextureManager* texturas)
    : backend_(&backend),
      esfera_(Primitivas3D::esferaUV(24, 32, 1.0f)) {
    renderer_.setBackend(backend);
    renderer_.setGestorShaders(gestorShaders);
    renderer_.setTextureManager(texturas);
    configurarLuces();

    // Camara de producto: la esfera se ve de frente, ligeramente por encima.
    vista_ = glm::lookAt(glm::vec3(0.f, 0.6f, 3.2f), glm::vec3(0.f, 0.f, 0.f),
                          glm::vec3(0.f, 1.f, 0.f));
    proyeccion_ = glm::perspective(glm::radians(35.0f), 1.0f, 0.1f, 100.0f);
}

PreviewMaterial::~PreviewMaterial() = default;

bool PreviewMaterial::disponible() const {
    return backend_ != nullptr;
}

PreviewMaterial::Vista* PreviewMaterial::obtenerVista(const std::string& clave) {
    auto it = vistas_.find(clave);
    if (it != vistas_.end()) return &it->second;
    // Cada vista tiene su render target (con su textura): si se acumulan
    // demasiadas (muchos objetos inspeccionados con material), se sueltan y
    // se vuelven a crear bajo demanda.
    if (vistas_.size() >= kMaxVistas) vistas_.clear();
    Vista vista;
    if (backend_) vista.target = std::make_unique<RenderTarget>(*backend_);
    return &vistas_.emplace(clave, std::move(vista)).first->second;
}

const void* PreviewMaterial::descriptorTextura(const std::string& clave) const {
    if (!backend_) return nullptr;
    const auto it = vistas_.find(clave);
    if (it == vistas_.end() || !it->second.target) return nullptr;
    const Rendering::Backend::Handle textura =
        it->second.target->getColorTexture();
    if (textura == Rendering::Backend::kInvalidHandle) return nullptr;
    // El descriptor opaco del backend (en GL, el GLuint como puntero): la GUI
    // lo reenvia a ImGui::Image sin conocer el valor concreto.
    return backend_->imguiTextureId(textura);
}

bool PreviewMaterial::renderizar(const std::string& clave,
                                  const IMaterial& material) {
    if (!backend_) return false;
    Vista* vista = obtenerVista(clave);
    if (!vista || !vista->target) return false;

    vista->target->resize(kLado, kLado);
    vista->target->bind();
    backend_->setViewport(0, 0, kLado, kLado);
    // Fondo sobrio: la esfera es lo que se mira, no el fondo.
    const float fondo[3] = {0.10f, 0.11f, 0.13f};
    backend_->clearScreen(fondo);

    const glm::mat4 modelo(1.0f);
    const bool ok = renderer_.renderMeshConMaterial(
        &esfera_, &material, glm::value_ptr(modelo), glm::value_ptr(vista_),
        glm::value_ptr(proyeccion_));

    // Vuelve al framebuffer de la ventana: la vista previa se dibuja durante
    // la pasada de interfaz, y lo que viene despues (la escena) vuelve a
    // fijar su propio viewport.
    backend_->bindDefaultFramebuffer();

    // La vista solo queda limpia si se pudo dibujar: si fallo (sin programa,
    // sin backend utilizable) sigue sucia para reintentarlo.
    if (ok) vista->sucia = false;
    return ok;
}

bool PreviewMaterial::sucia(const std::string& clave) const {
    const auto it = vistas_.find(clave);
    // Una vista que todavia no existe hay que dibujarla: su textura no esta.
    if (it == vistas_.end()) return true;
    return it->second.sucia;
}

void PreviewMaterial::invalidar(const std::string& clave) {
    Vista* vista = obtenerVista(clave);
    if (vista) vista->sucia = true;
}

void PreviewMaterial::configurarLuces() {
    // Luz clave desde arriba-izquierda-frente y un relleno frio desde la
    // derecha: con las dos, se leen las formas y el brillo del material.
    LightData clave;
    clave.type = 0; // direccional
    clave.worldPos[0] = -2.0f;
    clave.worldPos[1] = 3.0f;
    clave.worldPos[2] = 2.0f;
    clave.direction[0] = 0.55f;
    clave.direction[1] = -0.75f;
    clave.direction[2] = -0.35f;
    clave.ambient[0] = 0.22f;
    clave.ambient[1] = 0.22f;
    clave.ambient[2] = 0.25f;
    clave.diffuse[0] = 1.0f;
    clave.diffuse[1] = 0.97f;
    clave.diffuse[2] = 0.92f;
    clave.specular[0] = 0.8f;
    clave.specular[1] = 0.8f;
    clave.specular[2] = 0.8f;

    LightData relleno;
    relleno.type = 0;
    relleno.worldPos[0] = 3.0f;
    relleno.worldPos[1] = 0.5f;
    relleno.worldPos[2] = 1.5f;
    relleno.direction[0] = -0.85f;
    relleno.direction[1] = -0.15f;
    relleno.direction[2] = -0.5f;
    relleno.ambient[0] = 0.08f;
    relleno.ambient[1] = 0.09f;
    relleno.ambient[2] = 0.11f;
    relleno.diffuse[0] = 0.35f;
    relleno.diffuse[1] = 0.38f;
    relleno.diffuse[2] = 0.45f;
    relleno.specular[0] = 0.2f;
    relleno.specular[1] = 0.2f;
    relleno.specular[2] = 0.25f;

    const float ambiente[3] = {0.16f, 0.16f, 0.18f};
    const LightData luces[2] = {clave, relleno};
    renderer_.setLuces(luces, 2, ambiente);
}
