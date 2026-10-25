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
#ifndef IPREVIEWMATERIAL_H
#define IPREVIEWMATERIAL_H

#include <string>

class IMaterial;

// Contrato de la vista previa de un material: la esfera de muestra que el
// editor dibuja para mostrar como queda.
//
// La GUI (el panel del material) no sabe nada de OpenGL: pide el descriptor
// de la textura y lo dibuja con ImGui::Image. Cada material editado tiene su
// propia vista, identificada por una clave, para que un panel con varias
// regiones pueda mostrarlas a la vez.
//
// El ciclo de vida de una vista es simple: el panel invalida la vista cuando
// el material cambia y pide redibujarla; el servicio la dibuja en su render
// target y expone la textura.
class IPreviewMaterial {
public:
    virtual ~IPreviewMaterial() = default;

    // Hay vista previa disponible (backend, renderer y render target).
    virtual bool disponible() const = 0;

    // Lado de la textura de la vista (en pixeles): la GUI la dibuja a ese
    // tamano para no deformarla.
    virtual int lado() const = 0;

    // Descriptor opaco de la textura de la vista `clave`, el que se pasa a
    // ImGui::Image. nullptr si esa vista no existe todavia.
    virtual const void* descriptorTextura(const std::string& clave) const = 0;

    // Dibuja la esfera de muestra con el material dado en la vista `clave`.
    // Devuelve false si no se pudo dibujar.
    virtual bool renderizar(const std::string& clave,
                            const IMaterial& material) = 0;

    // La vista `clave` esta sucia: hay que redibujarla.
    virtual bool sucia(const std::string& clave) const = 0;

    // Marca la vista `clave` como sucia (el material cambio).
    virtual void invalidar(const std::string& clave) = 0;
};

#endif // IPREVIEWMATERIAL_H
