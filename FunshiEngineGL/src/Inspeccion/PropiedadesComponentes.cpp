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
#include "PropiedadesComponentes.h"

#include <algorithm>
#include <array>
#include <cctype>

#include "../Objetos/Componentes/Animador2D.h"
#include "../Objetos/Componentes/AudioSource.h"
#include "../Objetos/Componentes/CameraComponent.h"
#include "../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../Objetos/Componentes/Colliders/MallaCollider.h"
#include "../Objetos/Componentes/Colliders/TilemapCollider.h"
#include "../Objetos/Componentes/Color.h"
#include "../Objetos/Componentes/EmisorParticulas.h"
#include "../Objetos/Componentes/Grid.h"
#include "../Objetos/Componentes/InterfaceComponent.h"
#include "../Objetos/Componentes/Light.h"
#include "../Objetos/Componentes/Material.h"
#include "../Objetos/Componentes/Materiales.h"
#include "../Objetos/Componentes/Model.h"
#include "../Objetos/Componentes/Musica.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Script.h"
#include "../Objetos/Componentes/Skybox.h"
#include "../Objetos/Componentes/SpriteRenderer.h"
#include "../Objetos/Componentes/Tilemap.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/Componentes/UIComponent.h"

namespace MotorInspeccion {

namespace {

// --- Constructores de propiedades ----------------------------------------
//
// Cada propiedad define como se interpreta el texto (validar) y como se
// escribe en el componente (escribir). El puntero que llega a las funciones
// es el del componente concreto: el catalogo esta indexado por el nombre
// canonico del tipo, que es el mismo criterio que usa
// Entity::getComponentByName.
//
// `validar` no toca el componente: sirve para rechazar un valor invalido
// antes de encolar el comando, de modo que un rechazo no deja nada a medias
// en la escena.

template <typename T>
DefPropiedad propiedadFlotante(std::string nombre, std::string etiqueta,
                                std::function<float(T*)> getter,
                                std::function<void(T*, float)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Flotante;
    p.leer = [getter](void* c) {
        return textoFlotante(getter(static_cast<T*>(c)));
    };
    p.validar = [](const std::string& texto, std::string& error) {
        float valor = 0.0f;
        return leerFlotante(texto, valor, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        float valor = 0.0f;
        if (!leerFlotante(texto, valor, error)) return false;
        setter(static_cast<T*>(c), valor);
        return true;
    };
    return p;
}

// Como la anterior, pero el setter devuelve false cuando el motor rechaza el
// valor (por ejemplo una masa negativa o un radio que no puede ser cero).
template <typename T>
DefPropiedad propiedadFlotanteValidado(std::string nombre, std::string etiqueta,
                                        std::function<float(T*)> getter,
                                        std::function<bool(T*, float)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Flotante;
    p.leer = [getter](void* c) {
        return textoFlotante(getter(static_cast<T*>(c)));
    };
    p.validar = [](const std::string& texto, std::string& error) {
        float valor = 0.0f;
        return leerFlotante(texto, valor, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        float valor = 0.0f;
        if (!leerFlotante(texto, valor, error)) return false;
        if (!setter(static_cast<T*>(c), valor)) {
            error = "el motor rechazo el valor";
            return false;
        }
        return true;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadBooleana(std::string nombre, std::string etiqueta,
                               std::function<bool(T*)> getter,
                               std::function<void(T*, bool)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Booleano;
    p.leer = [getter](void* c) {
        return textoBooleano(getter(static_cast<T*>(c)));
    };
    p.validar = [](const std::string& texto, std::string& error) {
        bool valor = false;
        return leerBooleano(texto, valor, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        bool valor = false;
        if (!leerBooleano(texto, valor, error)) return false;
        setter(static_cast<T*>(c), valor);
        return true;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadEntera(std::string nombre, std::string etiqueta,
                              std::function<int(T*)> getter,
                              std::function<void(T*, int)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Entero;
    p.leer = [getter](void* c) {
        return textoEntero(getter(static_cast<T*>(c)));
    };
    p.validar = [](const std::string& texto, std::string& error) {
        int valor = 0;
        return leerEntero(texto, valor, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        int valor = 0;
        if (!leerEntero(texto, valor, error)) return false;
        setter(static_cast<T*>(c), valor);
        return true;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadTexto(std::string nombre, std::string etiqueta,
                             std::function<std::string(T*)> getter,
                             std::function<void(T*, const std::string&)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Texto;
    p.leer = [getter](void* c) {
        return getter(static_cast<T*>(c));
    };
    // Un texto es siempre valido: no hay nada que interpretar.
    p.validar = [](const std::string&, std::string&) { return true; };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        (void)error;
        setter(static_cast<T*>(c), texto);
        return true;
    };
    return p;
}

// La enumeracion se expone por su nombre textual: el valor interno es un
// entero, pero el que ve y escribe el usuario es "Puntual", no "1".
template <typename T>
DefPropiedad propiedadEnumeracion(std::string nombre, std::string etiqueta,
                                  std::vector<std::string> admitidos,
                                  std::function<int(T*)> getter,
                                  std::function<void(T*, int)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Enumeracion;
    p.admitidos = admitidos;
    p.leer = [getter, admitidos](void* c) {
        const int valor = getter(static_cast<T*>(c));
        if (valor >= 0 && valor < static_cast<int>(admitidos.size()))
            return admitidos[static_cast<std::size_t>(valor)];
        return std::to_string(valor);
    };
    p.validar = [admitidos](const std::string& texto, std::string& error) {
        for (const std::string& admitido : admitidos) {
            if (admitido == texto) return true;
        }
        std::string lista;
        for (std::size_t i = 0; i < admitidos.size(); ++i) {
            if (i > 0) lista += ", ";
            lista += admitidos[i];
        }
        error = "'" + texto + "' no es un valor admitido (" + lista + ")";
        return false;
    };
    p.escribir = [setter, admitidos](void* c, const std::string& texto,
                                     std::string& error) {
        for (std::size_t i = 0; i < admitidos.size(); ++i) {
            if (admitidos[i] == texto) {
                setter(static_cast<T*>(c), static_cast<int>(i));
                return true;
            }
        }
        error = "'" + texto + "' no es un valor admitido";
        return false;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadVec2(std::string nombre, std::string etiqueta,
                           std::function<std::array<float, 2>(T*)> getter,
                           std::function<void(T*, float, float)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Vec2;
    p.leer = [getter](void* c) {
        const std::array<float, 2> v = getter(static_cast<T*>(c));
        return textoVec2(v[0], v[1]);
    };
    p.validar = [](const std::string& texto, std::string& error) {
        float x = 0.0f, y = 0.0f;
        return leerVec2(texto, x, y, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        float x = 0.0f, y = 0.0f;
        if (!leerVec2(texto, x, y, error)) return false;
        setter(static_cast<T*>(c), x, y);
        return true;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadVec3(std::string nombre, std::string etiqueta,
                           std::function<std::array<float, 3>(T*)> getter,
                           std::function<void(T*, float, float, float)> setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Vec3;
    p.leer = [getter](void* c) {
        const std::array<float, 3> v = getter(static_cast<T*>(c));
        return textoVec3(v[0], v[1], v[2]);
    };
    p.validar = [](const std::string& texto, std::string& error) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        return leerVec3(texto, x, y, z, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (!leerVec3(texto, x, y, z, error)) return false;
        setter(static_cast<T*>(c), x, y, z);
        return true;
    };
    return p;
}

template <typename T>
DefPropiedad propiedadVec4(std::string nombre, std::string etiqueta,
                           std::function<std::array<float, 4>(T*)> getter,
                           std::function<void(T*, float, float, float, float)>
                               setter) {
    DefPropiedad p;
    p.nombre = std::move(nombre);
    p.etiqueta = std::move(etiqueta);
    p.tipo = TipoValor::Vec4;
    p.leer = [getter](void* c) {
        const std::array<float, 4> v = getter(static_cast<T*>(c));
        return textoVec4(v[0], v[1], v[2], v[3]);
    };
    p.validar = [](const std::string& texto, std::string& error) {
        float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
        return leerVec4(texto, x, y, z, w, error);
    };
    p.escribir = [setter](void* c, const std::string& texto,
                          std::string& error) {
        float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
        if (!leerVec4(texto, x, y, z, w, error)) return false;
        setter(static_cast<T*>(c), x, y, z, w);
        return true;
    };
    return p;
}

}  // namespace

CatalogoPropiedades::CatalogoPropiedades() {
    // --- Transform -------------------------------------------------------
    // El Transform guarda el estado en dos copias (la que usa el render y la
    // publica): por eso se escribe siempre por los setters.
    registrar("Transform", {
        propiedadVec3<Transform>(
            "position", "Posicion",
            [](Transform* t) {
                const float* p = t->getTranslatef();
                return std::array<float, 3>{p[0], p[1], p[2]};
            },
            [](Transform* t, float x, float y, float z) {
                t->setTranslatef(x, y, z);
            }),
        propiedadVec3<Transform>(
            "scale", "Escala",
            [](Transform* t) {
                const float* s = t->getScalef();
                return std::array<float, 3>{s[0], s[1], s[2]};
            },
            [](Transform* t, float x, float y, float z) {
                t->setScalef(x, y, z);
            }),
        // La rotacion del motor es un angulo global mas un eje: se expone
        // tal cual, sin convertir a angulos de Euler.
        propiedadFlotante<Transform>(
            "rotationAngle", "Angulo de rotacion (grados)",
            [](Transform* t) { return t->getRotatef()[0]; },
            [](Transform* t, float v) {
                const float* r = t->getRotatef();
                t->setRotatef(v, r[1], r[2], r[3]);
            }),
        propiedadVec3<Transform>(
            "rotationAxis", "Eje de rotacion",
            [](Transform* t) {
                const float* r = t->getRotatef();
                return std::array<float, 3>{r[1], r[2], r[3]};
            },
            [](Transform* t, float x, float y, float z) {
                const float* r = t->getRotatef();
                t->setRotatef(r[0], x, y, z);
            }),
    });

    // --- RigidBody -------------------------------------------------------
    registrar("RigidBody", {
        propiedadFlotanteValidado<RigidBody>(
            "mass", "Masa (0 = estatico)",
            [](RigidBody* r) { return r->masa(); },
            [](RigidBody* r, float v) { return r->fijarMasa(v); }),
        propiedadBooleana<RigidBody>(
            "active", "Activo",
            [](RigidBody* r) { return r->estaActivo(); },
            [](RigidBody* r, bool v) { r->setActivo(v); }),
        propiedadBooleana<RigidBody>(
            "useGravity", "Usa gravedad",
            [](RigidBody* r) { return r->usaGravedad(); },
            [](RigidBody* r, bool v) { r->fijarUsoGravedad(v); }),
        propiedadFlotanteValidado<RigidBody>(
            "gravityScale", "Escala de gravedad",
            [](RigidBody* r) { return r->escalaGravedad(); },
            [](RigidBody* r, float v) { return r->fijarEscalaGravedad(v); }),
        propiedadFlotanteValidado<RigidBody>(
            "friction", "Friccion",
            [](RigidBody* r) { return r->friccion(); },
            [](RigidBody* r, float v) { return r->fijarFriccion(v); }),
        propiedadBooleana<RigidBody>(
            "sensor", "Sensor (trigger)",
            [](RigidBody* r) { return r->esSensor(); },
            [](RigidBody* r, bool v) { r->fijarSensor(v); }),
        // Los freeze son por eje: al escribir uno se releen los otros dos para
        // no borrar los que ya estaban puestos.
        propiedadBooleana<RigidBody>(
            "freezePositionX", "Congelar posicion X",
            [](RigidBody* r) { return r->posicionCongelada(0); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(v, r->posicionCongelada(1),
                                       r->posicionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezePositionY", "Congelar posicion Y",
            [](RigidBody* r) { return r->posicionCongelada(1); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(r->posicionCongelada(0), v,
                                       r->posicionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezePositionZ", "Congelar posicion Z",
            [](RigidBody* r) { return r->posicionCongelada(2); },
            [](RigidBody* r, bool v) {
                r->fijarFreezePosicion(r->posicionCongelada(0),
                                       r->posicionCongelada(1), v);
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationX", "Congelar rotacion X",
            [](RigidBody* r) { return r->rotacionCongelada(0); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(v, r->rotacionCongelada(1),
                                       r->rotacionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationY", "Congelar rotacion Y",
            [](RigidBody* r) { return r->rotacionCongelada(1); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(r->rotacionCongelada(0), v,
                                       r->rotacionCongelada(2));
            }),
        propiedadBooleana<RigidBody>(
            "freezeRotationZ", "Congelar rotacion Z",
            [](RigidBody* r) { return r->rotacionCongelada(2); },
            [](RigidBody* r, bool v) {
                r->fijarFreezeRotacion(r->rotacionCongelada(0),
                                       r->rotacionCongelada(1), v);
            }),
    });

    // --- Light -----------------------------------------------------------
    registrar("Light", {
        propiedadEnumeracion<Light>(
            "type", "Tipo de luz",
            {"Direccional", "Puntual", "Spot"},
            [](Light* l) { return static_cast<int>(l->getType()); },
            [](Light* l, int v) { l->setType(static_cast<LightType>(v)); }),
        propiedadVec3<Light>(
            "ambient", "Ambiente",
            [](Light* l) {
                const float* v = l->getAmbient();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setAmbient(r, g, b); }),
        propiedadVec3<Light>(
            "diffuse", "Difuso",
            [](Light* l) {
                const float* v = l->getDiffuse();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setDiffuse(r, g, b); }),
        propiedadVec3<Light>(
            "specular", "Especular",
            [](Light* l) {
                const float* v = l->getSpecular();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Light* l, float r, float g, float b) { l->setSpecular(r, g, b); }),
        // La atenuacion se fija de golpe en el motor: al escribir una parte se
        // conservan las otras dos.
        propiedadFlotante<Light>(
            "attenuationConstant", "Atenuacion constante",
            [](Light* l) { return l->getConstant(); },
            [](Light* l, float v) {
                l->setAttenuation(v, l->getLinear(), l->getQuadratic());
            }),
        propiedadFlotante<Light>(
            "attenuationLinear", "Atenuacion lineal",
            [](Light* l) { return l->getLinear(); },
            [](Light* l, float v) {
                l->setAttenuation(l->getConstant(), v, l->getQuadratic());
            }),
        propiedadFlotante<Light>(
            "attenuationQuadratic", "Atenuacion cuadratica",
            [](Light* l) { return l->getQuadratic(); },
            [](Light* l, float v) {
                l->setAttenuation(l->getConstant(), l->getLinear(), v);
            }),
        propiedadFlotante<Light>(
            "spotCutOff", "Apertura del foco (grados)",
            [](Light* l) { return l->getSpotCutOff(); },
            [](Light* l, float v) { l->setSpotCutOff(v); }),
    });

    // --- CameraComponent -------------------------------------------------
    registrar("CameraComponent", {
        propiedadFlotante<CameraComponent>(
            "fov", "Campo de vision (grados)",
            [](CameraComponent* c) { return c->getFov(); },
            [](CameraComponent* c, float v) { c->setFov(v); }),
        propiedadEnumeracion<CameraComponent>(
            "projection", "Proyeccion", {"Perspectiva", "Ortografica"},
            [](CameraComponent* c) {
                return static_cast<int>(c->getProjection());
            },
            [](CameraComponent* c, int v) {
                c->setProjection(static_cast<CameraComponent::Projection>(v));
            }),
        propiedadFlotante<CameraComponent>(
            "orthoSize", "Alto de la vista ortografica",
            [](CameraComponent* c) { return c->getOrthoSize(); },
            [](CameraComponent* c, float v) { c->setOrthoSize(v); }),
        propiedadFlotante<CameraComponent>(
            "nearPlane", "Plano cercano",
            [](CameraComponent* c) { return c->getNearPlane(); },
            [](CameraComponent* c, float v) { c->setNearPlane(v); }),
        propiedadFlotante<CameraComponent>(
            "farPlane", "Plano lejano",
            [](CameraComponent* c) { return c->getFarPlane(); },
            [](CameraComponent* c, float v) { c->setFarPlane(v); }),
        propiedadFlotante<CameraComponent>(
            "speed", "Velocidad de la camara",
            [](CameraComponent* c) { return c->getSpeed(); },
            [](CameraComponent* c, float v) { c->setSpeed(v); }),
    });

    // --- Material --------------------------------------------------------
    // Los colores del material son RGBA, pero el setter fuerza alpha = 1: se
    // expone solo la parte RGB.
    registrar("Material", {
        propiedadVec3<Material>(
            "ambient", "Ambiente",
            [](Material* m) {
                const float* v = m->getAmbient();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setAmbient(r, g, b); }),
        propiedadVec3<Material>(
            "diffuse", "Difuso",
            [](Material* m) {
                const float* v = m->getDiffuse();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setDiffuse(r, g, b); }),
        propiedadVec3<Material>(
            "specular", "Especular",
            [](Material* m) {
                const float* v = m->getSpecular();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setSpecular(r, g, b); }),
        propiedadVec3<Material>(
            "emission", "Emision",
            [](Material* m) {
                const float* v = m->getEmission();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Material* m, float r, float g, float b) { m->setEmission(r, g, b); }),
        propiedadFlotante<Material>(
            "shininess", "Brillo",
            [](Material* m) { return m->getShininess(); },
            [](Material* m, float v) { m->setShininess(v); }),
        propiedadTexto<Material>(
            "diffuseMap", "Textura difusa",
            [](Material* m) { return m->getDiffuseMapPath(); },
            [](Material* m, const std::string& v) { m->setDiffuseMapPath(v); }),
        propiedadTexto<Material>(
            "specularMap", "Textura especular",
            [](Material* m) { return m->getSpecularMapPath(); },
            [](Material* m, const std::string& v) { m->setSpecularMapPath(v); }),
        propiedadTexto<Material>(
            "normalMap", "Textura de normales",
            [](Material* m) { return m->getNormalMapPath(); },
            [](Material* m, const std::string& v) { m->setNormalMapPath(v); }),
        propiedadTexto<Material>(
            "emissionMap", "Textura de emision",
            [](Material* m) { return m->getEmissionMapPath(); },
            [](Material* m, const std::string& v) { m->setEmissionMapPath(v); }),
    });

    // --- EmisorParticulas ------------------------------------------------
    registrar("EmisorParticulas", {
        propiedadBooleana<EmisorParticulas>(
            "active", "Activo",
            [](EmisorParticulas* e) { return e->activa(); },
            [](EmisorParticulas* e, bool v) { e->setActiva(v); }),
        propiedadEntera<EmisorParticulas>(
            "rate", "Particulas por segundo",
            [](EmisorParticulas* e) { return e->parametros().cantidadPorSegundo; },
            [](EmisorParticulas* e, int v) {
                e->parametros().cantidadPorSegundo = v < 0 ? 0 : v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "lifeMin", "Vida minima (segundos)",
            [](EmisorParticulas* e) { return e->parametros().vidaMinima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().vidaMinima = v;
                if (e->parametros().vidaMaxima < v)
                    e->parametros().vidaMaxima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "lifeMax", "Vida maxima (segundos)",
            [](EmisorParticulas* e) { return e->parametros().vidaMaxima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().vidaMaxima = v;
                if (e->parametros().vidaMinima > v)
                    e->parametros().vidaMinima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "speedMin", "Velocidad minima",
            [](EmisorParticulas* e) { return e->parametros().velocidadMinima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadMinima = v;
                if (e->parametros().velocidadMaxima < v)
                    e->parametros().velocidadMaxima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "speedMax", "Velocidad maxima",
            [](EmisorParticulas* e) { return e->parametros().velocidadMaxima; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadMaxima = v;
                if (e->parametros().velocidadMinima > v)
                    e->parametros().velocidadMinima = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "sizeMin", "Tamano minimo",
            [](EmisorParticulas* e) { return e->parametros().tamanoMinimo; },
            [](EmisorParticulas* e, float v) {
                e->parametros().tamanoMinimo = v;
                if (e->parametros().tamanoMaximo < v)
                    e->parametros().tamanoMaximo = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "sizeMax", "Tamano maximo",
            [](EmisorParticulas* e) { return e->parametros().tamanoMaximo; },
            [](EmisorParticulas* e, float v) {
                e->parametros().tamanoMaximo = v;
                if (e->parametros().tamanoMinimo > v)
                    e->parametros().tamanoMinimo = v;
            }),
        propiedadVec4<EmisorParticulas>(
            "color", "Color",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().color;
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](EmisorParticulas* e, float r, float g, float b, float a) {
                float* v = e->parametros().color;
                v[0] = r; v[1] = g; v[2] = b; v[3] = a;
            }),
        propiedadVec3<EmisorParticulas>(
            "direction", "Direccion",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().direccion;
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](EmisorParticulas* e, float x, float y, float z) {
                float* v = e->parametros().direccion;
                v[0] = x; v[1] = y; v[2] = z;
            }),
        propiedadFlotante<EmisorParticulas>(
            "dispersion", "Dispersion del cono",
            [](EmisorParticulas* e) { return e->parametros().dispersion; },
            [](EmisorParticulas* e, float v) {
                e->parametros().dispersion = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
            }),
        propiedadVec3<EmisorParticulas>(
            "gravity", "Gravedad",
            [](EmisorParticulas* e) {
                const float* v = e->parametros().gravedad;
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](EmisorParticulas* e, float x, float y, float z) {
                float* v = e->parametros().gravedad;
                v[0] = x; v[1] = y; v[2] = z;
            }),
        propiedadFlotante<EmisorParticulas>(
            "drag", "Rozamiento",
            [](EmisorParticulas* e) { return e->parametros().rozamiento; },
            [](EmisorParticulas* e, float v) {
                e->parametros().rozamiento = v < 0.0f ? 0.0f : v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "rotation", "Rotacion inicial (radianes)",
            [](EmisorParticulas* e) { return e->parametros().rotacionInicial; },
            [](EmisorParticulas* e, float v) {
                e->parametros().rotacionInicial = v;
            }),
        propiedadFlotante<EmisorParticulas>(
            "rotationSpeed", "Velocidad de rotacion (rad/s)",
            [](EmisorParticulas* e) { return e->parametros().velocidadRotacion; },
            [](EmisorParticulas* e, float v) {
                e->parametros().velocidadRotacion = v;
            }),
        propiedadTexto<EmisorParticulas>(
            "profilePath", "Perfil de efecto",
            [](EmisorParticulas* e) { return e->rutaPerfil(); },
            [](EmisorParticulas* e, const std::string& v) {
                e->setRutaPerfil(v);
            }),
    });

    // --- Materiales (materiales por region) -------------------------------
    registrar("Materiales", {
        propiedadEntera<Materiales>(
            "selectedRegion", "Region seleccionada (-1 = ninguna)",
            [](Materiales* m) { return m->regionSeleccionada(); },
            [](Materiales* m, int v) { m->setRegionSeleccionada(v); }),
    });

    // --- Color -----------------------------------------------------------
    registrar("Color", {
        propiedadVec3<Color>(
            "color", "Color",
            [](Color* c) {
                const float* v = c->getColor();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Color* c, float r, float g, float b) { c->setColor(r, g, b); }),
    });

    // --- Grid ------------------------------------------------------------
    registrar("Grid", {
        propiedadBooleana<Grid>(
            "visible", "Visible",
            [](Grid* g) { return g->getVisible(); },
            [](Grid* g, bool v) { g->setVisible(v); }),
        propiedadVec3<Grid>(
            "color", "Color",
            [](Grid* g) {
                const float* v = g->getColor();
                return std::array<float, 3>{v[0], v[1], v[2]};
            },
            [](Grid* g, float r, float gg, float b) { g->setColor(r, gg, b); }),
        propiedadFlotante<Grid>(
            "size", "Tamano de la celda",
            [](Grid* g) { return g->getTam(); },
            [](Grid* g, float v) { g->setTam(v); }),
        propiedadFlotante<Grid>(
            "separation", "Separacion",
            [](Grid* g) { return g->getSeparacion(); },
            [](Grid* g, float v) { g->setSeparacion(v); }),
    });

    // --- SpriteRenderer --------------------------------------------------
    registrar("SpriteRenderer", {
        propiedadTexto<SpriteRenderer>(
            "texturePath", "Textura",
            [](SpriteRenderer* s) { return s->getRutaTextura(); },
            [](SpriteRenderer* s, const std::string& v) {
                s->setRutaTextura(v);
            }),
        propiedadVec4<SpriteRenderer>(
            "tint", "Tinte",
            [](SpriteRenderer* s) {
                const float* v = s->getTint();
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](SpriteRenderer* s, float r, float g, float b, float a) {
                s->setTint(r, g, b, a);
            }),
        propiedadVec2<SpriteRenderer>(
            "size", "Tamano en unidades de mundo",
            [](SpriteRenderer* s) {
                const float* v = s->getTamano();
                return std::array<float, 2>{v[0], v[1]};
            },
            [](SpriteRenderer* s, float w, float h) { s->setTamano(w, h); }),
        propiedadVec4<SpriteRenderer>(
            "uvRect", "Rectangulo UV",
            [](SpriteRenderer* s) {
                const float* v = s->getUvRect();
                return std::array<float, 4>{v[0], v[1], v[2], v[3]};
            },
            [](SpriteRenderer* s, float u0, float v0, float u1, float v1) {
                s->setUvRect(u0, v0, u1, v1);
            }),
    });

    // --- AudioSource -----------------------------------------------------
    registrar("AudioSource", {
        propiedadTexto<AudioSource>(
            "clipPath", "Archivo de sonido",
            [](AudioSource* a) { return a->getRutaClip(); },
            [](AudioSource* a, const std::string& v) { a->setRutaClip(v); }),
        propiedadFlotante<AudioSource>(
            "volume", "Volumen",
            [](AudioSource* a) { return a->getVolume(); },
            [](AudioSource* a, float v) { a->setVolume(v); }),
        propiedadBooleana<AudioSource>(
            "loop", "En bucle",
            [](AudioSource* a) { return a->isLoop(); },
            [](AudioSource* a, bool v) { a->setLoop(v); }),
        propiedadBooleana<AudioSource>(
            "autoplay", "Reproducir al iniciar",
            [](AudioSource* a) { return a->isReproduccionAutomatica(); },
            [](AudioSource* a, bool v) { a->setReproduccionAutomatica(v); }),
    });

    // --- Musica ----------------------------------------------------------
    registrar("Musica", {
        propiedadFlotante<Musica>(
            "volume", "Volumen",
            [](Musica* m) { return m->getVolume(); },
            [](Musica* m, float v) { m->setVolume(v); }),
        propiedadFlotante<Musica>(
            "fadeDuration", "Fundido (segundos)",
            [](Musica* m) { return m->getDuracionFundido(); },
            [](Musica* m, float v) { m->setDuracionFundido(v); }),
        propiedadBooleana<Musica>(
            "autoplay", "Reproducir al iniciar",
            [](Musica* m) { return m->isReproduccionAutomatica(); },
            [](Musica* m, bool v) { m->setReproduccionAutomatica(v); }),
        propiedadBooleana<Musica>(
            "repeatList", "Repetir la lista",
            [](Musica* m) { return m->isRepetirLista(); },
            [](Musica* m, bool v) { m->setRepetirLista(v); }),
    });

    // --- InterfaceComponent ----------------------------------------------
    registrar("InterfaceComponent", {
        propiedadTexto<InterfaceComponent>(
            "htmlPath", "Archivo HTML",
            [](InterfaceComponent* i) { return i->getRutaHtml(); },
            [](InterfaceComponent* i, const std::string& v) {
                i->setRutaHtml(v);
            }),
        propiedadBooleana<InterfaceComponent>(
            "active", "Activa",
            [](InterfaceComponent* i) { return i->estaActiva(); },
            [](InterfaceComponent* i, bool v) { i->setActivo(v); }),
        propiedadEnumeracion<InterfaceComponent>(
            "anchor", "Ancla",
            {"ArribaIzquierda", "ArribaCentro", "ArribaDerecha",
             "MedioIzquierda", "Centro", "MedioDerecha", "AbajoIzquierda",
             "AbajoCentro", "AbajoDerecha"},
            [](InterfaceComponent* i) { return static_cast<int>(i->getAncla()); },
            [](InterfaceComponent* i, int v) {
                i->setAncla(static_cast<AnclaInterfaz>(v));
            }),
        propiedadVec2<InterfaceComponent>(
            "offset", "Desplazamiento",
            [](InterfaceComponent* i) {
                const glm::vec2 v = i->getDesplazamiento();
                return std::array<float, 2>{v.x, v.y};
            },
            [](InterfaceComponent* i, float x, float y) {
                i->setDesplazamiento(glm::vec2(x, y));
            }),
        propiedadBooleana<InterfaceComponent>(
            "autoSize", "Tamano automatico",
            [](InterfaceComponent* i) { return i->usaTamanoAuto(); },
            [](InterfaceComponent* i, bool v) { i->setTamanoAuto(v); }),
        propiedadVec2<InterfaceComponent>(
            "size", "Tamano",
            [](InterfaceComponent* i) {
                const glm::vec2 v = i->getTamano();
                return std::array<float, 2>{v.x, v.y};
            },
            [](InterfaceComponent* i, float w, float h) {
                i->setTamano(glm::vec2(w, h));
            }),
        propiedadFlotante<InterfaceComponent>(
            "z", "Orden de apilado",
            [](InterfaceComponent* i) { return i->getZ(); },
            [](InterfaceComponent* i, float v) { i->setZ(v); }),
    });

    // --- UIComponent (interfaz de juego nativa) --------------------------
    // Como con el Tilemap (que solo expone el tileset), la lista de widgets es
    // un dato compuesto que se edita en el inspector: aqui se exponen las
    // propiedades simples del componente.
    registrar("UIComponent", {
        propiedadBooleana<UIComponent>(
            "active", "Activa",
            [](UIComponent* i) { return i->estaActivo(); },
            [](UIComponent* i, bool v) { i->setActivo(v); }),
        propiedadFlotante<UIComponent>(
            "z", "Orden de apilado",
            [](UIComponent* i) { return i->getZ(); },
            [](UIComponent* i, float v) { i->setZ(v); }),
    });

    // --- Colliders -------------------------------------------------------
    // El radio solo existe en la esfera y en el cubo: en la malla y en el
    // tilemap la forma la define la malla o el mapa, no un radio.
    registrar("EsfereCollider", {
        propiedadFlotanteValidado<EsfereCollider>(
            "radius", "Radio",
            [](EsfereCollider* c) { return c->getRadio(); },
            [](EsfereCollider* c, float v) {
                // Collider::setRadio ignora en silencio los radios que no son
                // positivos: se rechaza aqui para que el error sea visible.
                if (v <= 0.0f) return false;
                c->setRadio(v);
                return true;
            }),
        propiedadBooleana<EsfereCollider>(
            "visibleInScene", "Visible en la escena",
            [](EsfereCollider* c) { return c->estaVisibleEnEscena(); },
            [](EsfereCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("CubeCollider", {
        propiedadFlotanteValidado<CubeCollider>(
            "radius", "Semilado del cubo",
            [](CubeCollider* c) { return c->getRadio(); },
            [](CubeCollider* c, float v) {
                if (v <= 0.0f) return false;
                c->setRadio(v);
                return true;
            }),
        propiedadBooleana<CubeCollider>(
            "visibleInScene", "Visible en la escena",
            [](CubeCollider* c) { return c->estaVisibleEnEscena(); },
            [](CubeCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("MallaCollider", {
        propiedadBooleana<MallaCollider>(
            "visibleInScene", "Visible en la escena",
            [](MallaCollider* c) { return c->estaVisibleEnEscena(); },
            [](MallaCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });
    registrar("TilemapCollider", {
        propiedadBooleana<TilemapCollider>(
            "visibleInScene", "Visible en la escena",
            [](TilemapCollider* c) { return c->estaVisibleEnEscena(); },
            [](TilemapCollider* c, bool v) { c->setVisibleEnEscena(v); }),
    });

    // --- Skybox ----------------------------------------------------------
    registrar("Skybox", {
        propiedadBooleana<Skybox>(
            "visible", "Visible",
            [](Skybox* s) { return s->getVisible(); },
            [](Skybox* s, bool v) { s->setVisible(v); }),
        propiedadTexto<Skybox>(
            "facePositiveX", "Cara +X",
            [](Skybox* s) { return s->getCaraMasX(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasX(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeX", "Cara -X",
            [](Skybox* s) { return s->getCaraMenosX(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosX(v); }),
        propiedadTexto<Skybox>(
            "facePositiveY", "Cara +Y",
            [](Skybox* s) { return s->getCaraMasY(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasY(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeY", "Cara -Y",
            [](Skybox* s) { return s->getCaraMenosY(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosY(v); }),
        propiedadTexto<Skybox>(
            "facePositiveZ", "Cara +Z",
            [](Skybox* s) { return s->getCaraMasZ(); },
            [](Skybox* s, const std::string& v) { s->setCaraMasZ(v); }),
        propiedadTexto<Skybox>(
            "faceNegativeZ", "Cara -Z",
            [](Skybox* s) { return s->getCaraMenosZ(); },
            [](Skybox* s, const std::string& v) { s->setCaraMenosZ(v); }),
    });

    // --- Animador2D ------------------------------------------------------
    registrar("Animador2D", {
        propiedadEntera<Animador2D>(
            "columns", "Columnas",
            [](Animador2D* a) { return a->getColumnas(); },
            [](Animador2D* a, int v) { a->setColumnas(v); }),
        propiedadEntera<Animador2D>(
            "rows", "Filas",
            [](Animador2D* a) { return a->getFilas(); },
            [](Animador2D* a, int v) { a->setFilas(v); }),
        propiedadEntera<Animador2D>(
            "firstFrame", "Primer frame",
            [](Animador2D* a) { return a->getPrimerFrame(); },
            [](Animador2D* a, int v) { a->setPrimerFrame(v); }),
        propiedadEntera<Animador2D>(
            "lastFrame", "Ultimo frame (-1 = el ultimo)",
            [](Animador2D* a) { return a->getUltimoFrame(); },
            [](Animador2D* a, int v) { a->setUltimoFrame(v); }),
        propiedadFlotante<Animador2D>(
            "fps", "Fotogramas por segundo",
            [](Animador2D* a) { return a->getFps(); },
            [](Animador2D* a, float v) { a->setFps(v); }),
        propiedadBooleana<Animador2D>(
            "loop", "En bucle",
            [](Animador2D* a) { return a->getBucle(); },
            [](Animador2D* a, bool v) { a->setBucle(v); }),
        propiedadBooleana<Animador2D>(
            "playing", "Reproduciendo",
            [](Animador2D* a) { return a->getReproduciendo(); },
            [](Animador2D* a, bool v) { a->setReproduciendo(v); }),
    });

    // --- Model -----------------------------------------------------------
    registrar("Model", {
        propiedadTexto<Model>(
            "path", "Ruta del modelo",
            [](Model* m) { return m->getPath(); },
            [](Model* m, const std::string& v) { m->setPath(v); }),
    });

    // --- Script ----------------------------------------------------------
    // Cambiar la ruta del script descarta el comportamiento cargado: el
    // motor lo vuelve a compilar al cargar la escena.
    registrar("Script", {
        propiedadTexto<Script>(
            "sourcePath", "Codigo fuente del script",
            [](Script* s) { return s->getPath(); },
            [](Script* s, const std::string& v) { s->setDllPath(v); }),
        propiedadTexto<Script>(
            "componentName", "Nombre del componente",
            [](Script* s) { return s->getNombreComponente(); },
            [](Script* s, const std::string& v) { s->setNombreComponente(v); }),
        propiedadBooleana<Script>(
            "deferredLoad", "Aplazar la carga",
            [](Script* s) { return s->getAplazarCarga(); },
            [](Script* s, bool v) { s->setAplazarCarga(v); }),
    });

    // --- Tilemap ---------------------------------------------------------
    registrar("Tilemap", {
        propiedadTexto<Tilemap>(
            "tilesetPath", "Tileset",
            [](Tilemap* t) { return t->getRutaTileset(); },
            [](Tilemap* t, const std::string& v) { t->setRutaTileset(v); }),
    });

    for (auto& par : mapa_) orden_.push_back(par.first);
    std::sort(orden_.begin(), orden_.end());
}

void CatalogoPropiedades::registrar(const std::string& tipo,
                                     std::vector<DefPropiedad> propiedades) {
    if (propiedades.empty()) return;
    mapa_[tipo] = std::move(propiedades);
}

const std::vector<DefPropiedad>* CatalogoPropiedades::de(
    const std::string& tipo) const {
    const auto encontrado = mapa_.find(tipo);
    if (encontrado == mapa_.end()) return nullptr;
    return &encontrado->second;
}

const DefPropiedad* CatalogoPropiedades::buscar(const std::string& tipo,
                                                 const std::string& variable) const {
    const std::vector<DefPropiedad>* propiedades = de(tipo);
    if (!propiedades) return nullptr;
    for (const DefPropiedad& p : *propiedades) {
        if (p.nombre.size() != variable.size()) continue;
        bool iguales = true;
        for (std::size_t i = 0; i < p.nombre.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(p.nombre[i])) !=
                std::tolower(static_cast<unsigned char>(variable[i]))) {
                iguales = false;
                break;
            }
        }
        if (iguales) return &p;
    }
    return nullptr;
}

const CatalogoPropiedades& CatalogoPropiedades::instancia() {
    static const CatalogoPropiedades catalogo;
    return catalogo;
}

}  // namespace MotorInspeccion
