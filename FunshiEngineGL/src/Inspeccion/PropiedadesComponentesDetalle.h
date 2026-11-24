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
// Preambulo compartido por las unidades de PropiedadesComponentes.cpp.
#ifndef PROPIEDADESCOMPONENTESDETALLE_H
#define PROPIEDADESCOMPONENTESDETALLE_H

#include "PropiedadesComponentes.h"

#include <algorithm>
#include <array>
#include <cctype>

#include "../Objetos/Componentes/Juego2D/Animador2D.h"
#include "../Objetos/Componentes/Audio/AudioSource.h"
#include "../Objetos/Componentes/Escena/CameraComponent.h"
#include "../Objetos/Componentes/Colliders/CubeCollider.h"
#include "../Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../Objetos/Componentes/Colliders/MallaCollider.h"
#include "../Objetos/Componentes/Colliders/TilemapCollider.h"
#include "../Objetos/Componentes/Render/Color.h"
#include "../Objetos/Componentes/Efectos/EmisorParticulas.h"
#include "../Objetos/Componentes/Escena/Grid.h"
#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../Objetos/Componentes/Escena/Light.h"
#include "../Objetos/Componentes/Render/Material.h"
#include "../Objetos/Componentes/Render/Materiales.h"
#include "../Objetos/Componentes/Render/Model.h"
#include "../Objetos/Componentes/Audio/Musica.h"
#include "../Objetos/Componentes/RigidBody/RigidBody.h"
#include "../Objetos/Componentes/Scripting/Script.h"
#include "../Objetos/Componentes/Render/Skybox.h"
#include "../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../Objetos/Componentes/Juego2D/Tilemap.h"
#include "../Objetos/Componentes/Escena/Transform.h"
#include "../Objetos/Componentes/Interfaz/UIComponent.h"

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

}  // namespace MotorInspeccion

#endif
