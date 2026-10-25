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
#ifndef CATALOGO_COMPONENTES_H
#define CATALOGO_COMPONENTES_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "../../Configuracion/DimensionProyecto.h"

class Component;
class GameObject;
class SettingsComponent;
class EditorController;
class AudioEngine;
class IPreviewMaterial;

// Dimension del proyecto en la que un componente tiene sentido. El menu de
// "agregar componente" solo ofrece los que aplican a la dimension vigente;
// un componente ya presente en el objeto siempre muestra su panel, aunque la
// dimension del proyecto no coincida (asi se puede inspeccionar y quitar).
enum class DimensionesComponente {
    DosD,
    TresD,
    Ambas
};

inline bool dimensionesIncluyen(DimensionesComponente d, DimensionProyecto p) {
    switch (d) {
    case DimensionesComponente::Ambas:
        return true;
    case DimensionesComponente::DosD:
        return p == DimensionProyecto::DosD;
    case DimensionesComponente::TresD:
        return p == DimensionProyecto::TresD;
    default:
        return true;
    }
}

// Contexto que necesitan los paneles para construirse: el objeto y los
// servicios que la escena inyecta (editor para undo/redo y colliders, audio
// para la prueba de reproduccion, dimension para ocultar/limitar opciones).
struct ContextoPanel {
    GameObject* object = nullptr;
    EditorController* editor = nullptr;
    AudioEngine* audio = nullptr;
    DimensionProyecto dimension = DimensionProyecto::TresD;
    // Vista previa de material (la esfera de muestra). Puede ser nullptr:
    // entonces el panel del material se dibuja sin vista previa.
    IPreviewMaterial* preview = nullptr;
};

struct DescriptorComponente {
    // Nombre demangle del tipo (typeid().name()); es la clave del catalogo y
    // el nombre con el que el serializer persiste el componente.
    std::string nombre;
    // Etiqueta que se muestra en el menu "agregar componente".
    std::string etiqueta;
    // Alias retrocompatible con builds viejos (p. ej. "Camera" ->
    // "CameraComponent"). Vacio si no aplica.
    std::string alias;
    DimensionesComponente dimensiones = DimensionesComponente::Ambas;
    // Submenu en el que se agrupa dentro de "Agregar componente"; vacio = nivel
    // raiz.
    std::string submenu;
    // Icono del componente en el inspector, como ruta relativa a la carpeta de
    // imagenes del motor (por ejemplo
    // "GUI-Image/Component-Image/Transform/comp-transform-white.png"). Vacio si
    // el componente no tiene icono propio. Se deriva del tipo, nunca se
    // serializa: el nombre del tipo ya es el dato persistido.
    std::string icono;
    // Requisito para poder agregarlo (p. ej. colisionador exige Transform).
    std::function<bool(GameObject&)> prerequisito;
    // Texto que explica que falta para poder agregarlo ("necesita un
    // Transform"). Vacio si el componente no tiene requisitos. La API de
    // inspeccion lo usa para explicar por que no se pudo agregar.
    std::string requisito;
    // Crea el componente; nullptr si el requisito no se cumple.
    std::function<std::unique_ptr<Component>(GameObject&)> crear;
    // Crea el panel del inspector para un componente ya presente; vacio si el
    // componente no tiene panel.
    std::function<std::unique_ptr<SettingsComponent>(Component*, GameObject*,
                                                     const ContextoPanel&)>
        crearPanel;
};

// Catalogo unico de tipos de componente del motor. El campo se evalua una sola
// vez: cada componente se declara con sus metadatos (dimension, requisito,
// submenu), su creador y su panel. ComponentFactory y el inspector consumen
// esta tabla en vez de mantener cada uno su propia cadena de if.
class CatalogoComponentes {
public:
    static const CatalogoComponentes& instancia();

    const std::vector<DescriptorComponente>& descriptores() const {
        return items_;
    }

    // Busca por nombre demangle, tolerando el prefijo que MSVC anteponia
    // ("class X"/"struct X") y el alias retrocompatible. Devuelve nullptr si el
    // tipo no esta en el catalogo.
    const DescriptorComponente* buscar(const std::string& nombre) const;

private:
    CatalogoComponentes();
    std::vector<DescriptorComponente> items_;
};

#endif // CATALOGO_COMPONENTES_H
