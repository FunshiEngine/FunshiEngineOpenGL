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
#ifndef PORTAPAPELES_SISTEMA_H
#define PORTAPAPELES_SISTEMA_H

#include <string>
#include <vector>

#include "IServicioPortapapeles.h"

// Lectura del portapapeles del sistema en la plataforma del host.
//
// Windows: el portapapeles de archivos es un CF_HDROP (una lista de rutas
// gestionada por el shell). Linux: no hay equivalente en GLFW, asi que se lee
// el texto del portapapeles y se interpretan las URIs de archivo que el
// explorador del sistema deja ahi.
//
// La ventana se recibe como handle opaco (GLFWwindow*) porque en Linux el
// portapapeles se consulta a traves de la ventana. En Windows se ignora.
class PortapapelesSistema : public IServicioPortapapeles {
public:
    explicit PortapapelesSistema(void* ventana = nullptr);
    ~PortapapelesSistema() override;

    PortapapelesSistema(const PortapapelesSistema&) = delete;
    PortapapelesSistema& operator=(const PortapapelesSistema&) = delete;

    std::vector<std::string> archivosEnPortapapeles() const override;
    bool disponible(std::string* motivo = nullptr) const override;

private:
    void* ventana = nullptr;
};

#endif
