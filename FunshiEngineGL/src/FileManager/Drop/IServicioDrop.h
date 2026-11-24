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
#ifndef ISERVICIODROP_H
#define ISERVICIODROP_H

#include <string>
#include <vector>

// Contrato de los archivos que el usuario suelta sobre la ventana del editor
// desde el explorador del sistema.
//
// El sistema entrega el drop FUERA del frame de la GUI (ocurre al procesar
// los eventos de la ventana), asi que el servicio lo encola y el explorador
// lo drena en su frame, cuando ya sabe que carpeta esta viendo: la carpeta
// visible es el destino.
//
// La ventana se pasa como handle opaco porque este contrato no debe depender
// de la biblioteca de ventanas: cada implementacion lo traduce a su propio
// tipo.
class IServicioDrop {
public:
    virtual ~IServicioDrop() = default;

    // Empieza a escuchar los drops en esa ventana del sistema.
    virtual void registrar(void* ventana) = 0;

    // Rutas sueltas desde la ultima llamada, y vacia la cola. La cola es la
    // frontera entre el hilo del sistema y el frame de la GUI.
    virtual std::vector<std::string> drenar() = 0;

    // Deja de escuchar drops en la ventana.
    virtual void desregistrar() = 0;
};

#endif
