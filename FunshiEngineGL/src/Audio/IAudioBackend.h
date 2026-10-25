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

// Contrato de musica (ver IMusicaBackend.h). Solo se necesita la
// declaracion adelantada: el backend de audio puede ofrecer tambien el
// contrato de musica, que es otro contrato distinto.
class IMusicaBackend;

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
    // `posicion` (opcional): si se pasa, el sonido es espacial (3D) y se situa
    // en esa coordenada del mundo; si es nullptr, el sonido es global (2D) y
    // suena igual desde cualquier punto.
    virtual int reproducirArchivo(const std::string& ruta, float volumen,
                                  bool bucle,
                                  const float* posicion = nullptr) = 0;
    virtual void detener(int id) = 0;
    virtual void detenerTodo() = 0;
    virtual void setVolumenMaestro(float volumen) = 0;

    // Reposiciona un sonido activo (para que siga a su objeto cada frame).
    virtual void setPosicionSonido(int id, float x, float y, float z) = 0;
    // Posiciona el listener (camara) para la spatializacion 3D.
    virtual void setListenerPosition(float x, float y, float z) = 0;

    // Recolecta las reproducciones que terminaron solas (free resources).
    // deltaTime: segundos desde la ultima llamada.
    virtual void despachar(float deltaTime) = 0;

    // Contrato de musica de este mismo backend, o nullptr si el backend no
    // ofrece musica. Lo consulta el AudioEngine para entregar el reproductor
    // de musica (GestorMusica) al backend real: la musica y los efectos de
    // sonido son contratos distintos porque necesitan cosas distintas
    // (volumen en curso, fundido, posicion y bus).
    virtual IMusicaBackend* backendMusica() { return nullptr; }
};

#endif // IAUDIO_BACKEND_H