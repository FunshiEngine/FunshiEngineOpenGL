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
#ifndef IAUDIO_BACKEND_H
#define IAUDIO_BACKEND_H

#include <string>

// Contrato del backend de audio (patron Strategy, mismo rol que
// IPhysicsBackend en Fisicas). El motor (AudioEngine) delega aqui TODA la
// maquinaria de sonido; un backend concreto se oculta detras de esta interfaz
// y se inyecta por constructor, con nullptr se monta el sin hardware/tests.
class IAudioBackend {
public:
    virtual ~IAudioBackend() = default;

    // Abre el dispositivo de audio. false si no hay hardware disponible
    // (headless, CI) o fallo el driver.
    virtual bool iniciar() = 0;
    virtual void cerrar() = 0;

    // Reproduce un archivo de audio; devuelve un id > 0 si arranco, 0 si fallo.
    // El resultado se consulta con detener()/recolectando reproducciones.
    virtual int reproducirArchivo(const std::string& ruta, float volumen,
                                  bool bucle) = 0;
    virtual void detener(int id) = 0;
    virtual void detenerTodo() = 0;
    virtual void setVolumenMaestro(float volumen) = 0;

    // Recolecta las reproducciones que terminaron solas (free resources).
    // deltaTime: segundos desde la ultima llamada.
    virtual void despachar(float deltaTime) = 0;
};

#endif // IAUDIO_BACKEND_H