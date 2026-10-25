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
#include "CatalogoComponentes.h"

#include "Animador2D.h"
#include "AudioSource.h"
#include "CameraComponent.h"
#include "Color.h"
#include "EmisorParticulas.h"
#include "Colliders/CubeCollider.h"
#include "Colliders/EsfereCollider.h"
#include "Colliders/MallaCollider.h"
#include "Colliders/TilemapCollider.h"
#include "Grid.h"
#include "InterfaceComponent.h"
#include "Light.h"
#include "Material.h"
#include "Materiales.h"
#include "Model.h"
#include "Musica.h"
#include "RigidBody/RigidBody.h"
#include "Script.h"
#include "Skybox.h"
#include "Sombras/Sombras.h"
#include "SpriteRenderer.h"
#include "Tilemap.h"
#include "Transform.h"
#include "UIComponent.h"
#include "../GameObject.h"

#include "../../Herramientas/TypeUtils.h"

#include "../../GUI/ObjetosGUI/SettingsComponent.h"
#include "../../GUI/ObjetosGUI/Animador2D/SettingsAnimador2D.h"
#include "../../GUI/ObjetosGUI/Audio/SettingsAudioSource.h"
#include "../../GUI/ObjetosGUI/Audio/SettingsMusica.h"
#include "../../GUI/ObjetosGUI/Camera/SettingsCamera.h"
#include "../../GUI/ObjetosGUI/Color/SettingsColor.h"
#include "../../GUI/ObjetosGUI/Colliders/SettingsColliderCubo.h"
#include "../../GUI/ObjetosGUI/Colliders/SettingsColliderEsfera.h"
#include "../../GUI/ObjetosGUI/Colliders/SettingsColliderMalla.h"
#include "../../GUI/ObjetosGUI/Colliders/SettingsColliderTilemap.h"
#include "../../GUI/ObjetosGUI/Grid/SettingsGrid.h"
#include "../../GUI/ObjetosGUI/Interface/SettingsInterface.h"
#include "../../GUI/ObjetosGUI/Particulas/SettingsEmisorParticulas.h"
#include "../../GUI/ObjetosGUI/Light/SettingsLight.h"
#include "../../GUI/ObjetosGUI/Material/SettingsMaterial.h"
#include "../../GUI/ObjetosGUI/Materiales/SettingsMateriales.h"
#include "../../GUI/ObjetosGUI/Model/SettingsModel.h"
#include "../../GUI/ObjetosGUI/RigidBody/SettingsRigidBody.h"
#include "../../GUI/ObjetosGUI/Script/SettingsScript.h"
#include "../../GUI/ObjetosGUI/Skybox/SettingsSkybox.h"
#include "../../GUI/ObjetosGUI/Sombras/SettingsSombras.h"
#include "../../GUI/ObjetosGUI/Sprite/SettingsSpriteRenderer.h"
#include "../../GUI/ObjetosGUI/Tilemap/SettingsTilemap.h"
#include "../../GUI/ObjetosGUI/Transform/SettingsTransform.h"
#include "../../GUI/ObjetosGUI/Interfaz/SettingsUIComponent.h"

namespace {

constexpr const char* kSubmenuColliders = "Add Collider";

// Iconos de componente (ruta relativa a la carpeta de imagenes del motor).
// Los dibuja el inspector a la izquierda del nombre del componente. El icono
// se deriva del tipo de componente, no se serializa: el nombre del tipo ya es
// el dato que se guarda en la escena.
constexpr const char* kIconoTransform =
    "GUI-Image/Component-Image/Transform/comp-transform-white.png";
constexpr const char* kIconoGenerico =
    "GUI-Image/Component-Image/comp-empty-white.png";
constexpr const char* kIconoCamara =
    "GUI-Image/Component-Image/Camara/camera-white.png";
constexpr const char* kIconoMaterial =
    "GUI-Image/Component-Image/Material/material-white.png";
constexpr const char* kIconoLuz =
    "GUI-Image/Component-Image/Iluminacion/comp-light-directional-white.png";
constexpr const char* kIconoModelo =
    "GUI-Image/Component-Image/Model/model-white.png";
constexpr const char* kIconoSkybox =
    "GUI-Image/Component-Image/Skybox/skybox-white.png";
constexpr const char* kIconoSprite =
    "GUI-Image/Component-Image/Sprite/sprite-white.png";
constexpr const char* kIconoAnimacion =
    "GUI-Image/Component-Image/animation-white.png";
constexpr const char* kIconoTilemap =
    "GUI-Image/Component-Image/tilemap-white.png";
constexpr const char* kIconoColliderEsfera =
    "GUI-Image/Component-Image/Fisicas/comp-collider-sphere-white.png";
constexpr const char* kIconoColliderCubo =
    "GUI-Image/Component-Image/Fisicas/comp-collider-box-white.png";
constexpr const char* kIconoColliderMalla =
    "GUI-Image/Component-Image/Fisicas/comp-collider-mesh-white.png";
constexpr const char* kIconoColliderTilemap =
    "GUI-Image/Component-Image/Fisicas/comp-collider-box-white.png";
constexpr const char* kIconoRigidBody =
    "GUI-Image/Component-Image/Fisicas/comp-rigidbody-white.png";
constexpr const char* kIconoScript =
    "GUI-Image/Component-Image/Scripts/script-white.png";
constexpr const char* kIconoAudio =
    "GUI-Image/Component-Image/Sonidos/audio-white.png";
constexpr const char* kIconoMusica =
    "GUI-Image/Component-Image/Sonidos/audio-white.png";
constexpr const char* kIconoInterfaz =
    "GUI-Image/Component-Image/Interfaces/ui-white.png";
// El HUD nativo (widgets del motor) es un lienzo de pantalla: por eso usa el
// icono de canvas y no el de la interfaz web.
constexpr const char* kIconoInterfazNativa =
    "GUI-Image/Component-Image/comp-canvas-white.png";

} // namespace

CatalogoComponentes::CatalogoComponentes() {
    using Creador = std::function<std::unique_ptr<Component>(GameObject&)>;
    using CreadorPanel = std::function<std::unique_ptr<SettingsComponent>(
        Component*, GameObject*, const ContextoPanel&)>;

    auto add = [this](std::string nombre, std::string etiqueta,
                      DimensionesComponente dim, std::string submenu,
                      std::string icono, Creador crear, CreadorPanel panel,
                      std::function<bool(GameObject&)> prereq = nullptr,
                      std::string alias = std::string(),
                      std::string requisito = std::string()) {
        DescriptorComponente d;
        d.nombre = std::move(nombre);
        d.etiqueta = std::move(etiqueta);
        d.alias = std::move(alias);
        d.dimensiones = dim;
        d.submenu = std::move(submenu);
        d.icono = std::move(icono);
        d.prerequisito = std::move(prereq);
        d.requisito = std::move(requisito);
        d.crear = std::move(crear);
        d.crearPanel = std::move(panel);
        items_.push_back(std::move(d));
    };

    // Nucleo: presentes en cualquier dimension.
    add("Transform", "Transform", DimensionesComponente::Ambas, "",
        kIconoTransform,
        [](GameObject&) { return std::unique_ptr<Component>(new Transform()); },
        [](Component* c, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsTransform(dynamic_cast<Transform*>(c), o);
            s->setDimension(ctx.dimension);
            return std::unique_ptr<SettingsComponent>(s);
        });
    add("Color", "Color", DimensionesComponente::Ambas, "",
        kIconoGenerico,
        [](GameObject&) { return std::unique_ptr<Component>(new Color()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsColor(o));
        });
    add("CameraComponent", "CameraComponent", DimensionesComponente::Ambas, "",
        kIconoCamara,
        [](GameObject&) {
            return std::unique_ptr<Component>(new CameraComponent());
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsCamera(o);
            s->setDimension(ctx.dimension);
            return std::unique_ptr<SettingsComponent>(s);
        },
        nullptr, "Camera");

    // Render 3D: solo tienen sentido con profundidad.
    add("Material", "Material", DimensionesComponente::TresD, "",
        kIconoMaterial,
        [](GameObject&) { return std::unique_ptr<Component>(new Material()); },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsMaterial(o, ctx.preview));
        });
    // Materiales por region: cada parte de la malla (la cara de un cubo, la
    // parte de un personaje) puede llevar su propio material.
    add("Materiales", "Materiales por region", DimensionesComponente::TresD, "",
        kIconoMaterial,
        [](GameObject&) { return std::unique_ptr<Component>(new Materiales()); },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsMateriales(o, ctx.editor, ctx.preview));
        });
    add("Light", "Light", DimensionesComponente::TresD, "",
        kIconoLuz,
        [](GameObject&) { return std::unique_ptr<Component>(new Light()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsLight(o));
        });
    add("Model", "Model", DimensionesComponente::TresD, "",
        kIconoModelo,
        [](GameObject&) { return std::unique_ptr<Component>(new Model()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsModel(o));
        });
    add("Grid", "Grid", DimensionesComponente::TresD, "",
        kIconoGenerico,
        [](GameObject&) { return std::unique_ptr<Component>(new Grid()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsGrid(o));
        });
    add("Skybox", "Skybox", DimensionesComponente::TresD, "",
        kIconoSkybox,
        [](GameObject&) { return std::unique_ptr<Component>(new Skybox()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsSkybox(o));
        });
    // Sombras: se agrega al objeto de la luz, asi que sin luz no tiene sentido.
    add("Sombras", "Sombras", DimensionesComponente::TresD, "",
        kIconoGenerico,
        [](GameObject& o) -> std::unique_ptr<Component> {
            if (!o.getComponent<Light>()) return nullptr;
            return std::unique_ptr<Component>(new Sombras());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsSombras(o));
        },
        [](GameObject& o) { return o.getComponent<Light>() != nullptr; },
        "", "necesita una Light");

    // Render 2D: el sprite se puede usar en ambas; el animador es propio del
    // plano.
    add("SpriteRenderer", "SpriteRenderer", DimensionesComponente::Ambas, "",
        kIconoSprite,
        [](GameObject&) {
            return std::unique_ptr<Component>(new SpriteRenderer());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsSpriteRenderer(o));
        });
    add("Animador2D", "Animador2D", DimensionesComponente::DosD, "",
        kIconoAnimacion,
        [](GameObject& o) -> std::unique_ptr<Component> {
            if (!o.getComponent<SpriteRenderer>()) return nullptr;
            return std::unique_ptr<Component>(new Animador2D());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsAnimador2D(o));
        },
        [](GameObject& o) {
            return o.getComponent<SpriteRenderer>() != nullptr;
        },
        "", "necesita un SpriteRenderer");
    add("Tilemap", "Tilemap", DimensionesComponente::DosD, "",
        kIconoTilemap,
        [](GameObject&) { return std::unique_ptr<Component>(new Tilemap()); },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsTilemap(o));
        });

    // Colision: requieren Transform y en el menu se agrupan en un submenu.
    auto exigeTransform = [](GameObject& o) {
        return o.getComponent<Transform>() != nullptr;
    };
    add("EsfereCollider", "EsfereCollider", DimensionesComponente::Ambas,
        kSubmenuColliders,
        kIconoColliderEsfera,
        [](GameObject& o) -> std::unique_ptr<Component> {
            Transform* t = o.getComponent<Transform>();
            if (!t) return nullptr;
            return std::unique_ptr<Component>(new EsfereCollider(5.0f, t, &o));
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsColliderEsfera(o);
            s->setEditor(ctx.editor);
            return std::unique_ptr<SettingsComponent>(s);
        },
        exigeTransform, "", "necesita un Transform");
    add("CubeCollider", "CubeCollider", DimensionesComponente::Ambas,
        kSubmenuColliders,
        kIconoColliderCubo,
        [](GameObject& o) -> std::unique_ptr<Component> {
            Transform* t = o.getComponent<Transform>();
            if (!t) return nullptr;
            return std::unique_ptr<Component>(new CubeCollider(5.0f, t, &o));
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsColliderCubo(o);
            s->setEditor(ctx.editor);
            return std::unique_ptr<SettingsComponent>(s);
        },
        exigeTransform, "", "necesita un Transform");
    add("MallaCollider", "MallaCollider", DimensionesComponente::TresD,
        kSubmenuColliders,
        kIconoColliderMalla,
        [](GameObject& o) -> std::unique_ptr<Component> {
            Transform* t = o.getComponent<Transform>();
            if (!t) return nullptr;
            return std::unique_ptr<Component>(new MallaCollider(5.0f, t, &o));
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsColliderMalla(o);
            s->setEditor(ctx.editor);
            return std::unique_ptr<SettingsComponent>(s);
        },
        exigeTransform, "", "necesita un Transform");
    add("TilemapCollider", "TilemapCollider", DimensionesComponente::DosD,
        kSubmenuColliders,
        kIconoColliderTilemap,
        [](GameObject& o) -> std::unique_ptr<Component> {
            Transform* t = o.getComponent<Transform>();
            if (!t) return nullptr;
            return std::unique_ptr<Component>(new TilemapCollider(5.0f, t, &o));
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsColliderTilemap(o);
            s->setEditor(ctx.editor);
            return std::unique_ptr<SettingsComponent>(s);
        },
        [](GameObject& o) {
            return o.getComponent<Transform>() != nullptr &&
                   o.getComponent<Tilemap>() != nullptr;
        },
        "", "necesita un Transform y un Tilemap");
    add("RigidBody", "RigidBody", DimensionesComponente::Ambas, "",
        kIconoRigidBody,
        [](GameObject& o) -> std::unique_ptr<Component> {
            Collider* col = o.getComponent<Collider>();
            if (!col) return nullptr;
            return std::unique_ptr<Component>(new RigidBody(col, 1.0f));
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsRigidBody(o);
            s->setEditor(ctx.editor);
            return std::unique_ptr<SettingsComponent>(s);
        },
        [](GameObject& o) { return o.getComponent<Collider>() != nullptr; },
        "", "necesita un Collider");

    // Script, audio e interfaz: disponibles en cualquier dimension.
    add("Script", "Script", DimensionesComponente::Ambas, "",
        kIconoScript,
        [](GameObject&) { return std::unique_ptr<Component>(new Script()); },
        [](Component* c, GameObject*, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsScript(dynamic_cast<Script*>(c)));
        });
    add("AudioSource", "AudioSource", DimensionesComponente::Ambas, "",
        kIconoAudio,
        [](GameObject&) {
            return std::unique_ptr<Component>(new AudioSource());
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsAudioSource(o);
            s->setAudioEngine(ctx.audio);
            return std::unique_ptr<SettingsComponent>(s);
        });
    add("Musica", "Musica", DimensionesComponente::Ambas, "",
        kIconoMusica,
        [](GameObject&) {
            return std::unique_ptr<Component>(new Musica());
        },
        [](Component*, GameObject* o, const ContextoPanel& ctx) {
            auto* s = new SettingsMusica(o);
            s->setAudioEngine(ctx.audio);
            return std::unique_ptr<SettingsComponent>(s);
        });
    // Emisor de particulas: funciona en cualquier dimension y no exige nada.
    add("EmisorParticulas", "Emisor de particulas",
        DimensionesComponente::Ambas, "",
        kIconoGenerico,
        [](GameObject&) {
            return std::unique_ptr<Component>(new EmisorParticulas());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsEmisorParticulas(o));
        });
    add("InterfaceComponent", "Interfaz de juego (HTML)",
        DimensionesComponente::Ambas, "",
        kIconoInterfaz,
        [](GameObject&) {
            return std::unique_ptr<Component>(new InterfaceComponent());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(new SettingsInterface(o));
        });
    // Interfaz de juego NATIVA: los widgets los dibuja el motor, sin
    // navegador web. Convive con la interfaz HTML.
    add("UIComponent", "Interfaz de juego (nativa)",
        DimensionesComponente::Ambas, "",
        kIconoInterfazNativa,
        [](GameObject&) {
            return std::unique_ptr<Component>(new UIComponent());
        },
        [](Component*, GameObject* o, const ContextoPanel&) {
            return std::unique_ptr<SettingsComponent>(
                new SettingsUIComponent(o));
        });
}

const CatalogoComponentes& CatalogoComponentes::instancia() {
    static const CatalogoComponentes catalogo;
    return catalogo;
}

const DescriptorComponente* CatalogoComponentes::buscar(
    const std::string& nombre) const {
    // El nombre se normaliza al simple (sin namespace y sin el prefijo de
    // class-key de MSVC) para que el mismo tipo se encuentre con el nombre
    // que se persistio en otra plataforma.
    const std::string normalizado = nombreSimpleTipo(nombre);
    for (const DescriptorComponente& d : items_) {
        if (d.nombre == normalizado ||
            (!d.alias.empty() && d.alias == normalizado))
            return &d;
    }
    return nullptr;
}
