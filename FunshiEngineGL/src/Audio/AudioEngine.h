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
#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

class IAudioBackend;

// Fachada del subsistema de audio (patron Facade, mismo rol que PhysicsEngine
// en Fisicas). Header liviano: no expone hilos, mutex ni el backend concreto.
//
// CONCURRENCIA: el motor tiene un HILO DE TRABAJO dedicado que procesa una
// cola de comandos (reproducir/detener/volumen). El hilo del juego (o la GUI)
// encola sin bloquear y sigue; quien de verdad habla con el hardware es el hilo
// de audio, y detras, miniaudio mezcla en el hilo de su device. Asi la
// reproduccion nunca frena el bucle del editor.
//
// Los clips se referencian por NOMBRE (registrados con registrarClip); la GUI y
// los componentes hablan en terminos de nombres, nunca de rutas.
class AudioEngine {
public:
    // Con nullptr monta NullAudioBackend (headless/tests/CI). El ctor sin
    // argumentos delega en este; el default arg con IAudioBackend incompleto
    // romperia a cualquier TU que lo invoque sin pare (unique_ptr exige el
    // tipo completo en el destructor del parametro).
    explicit AudioEngine(std::unique_ptr<IAudioBackend> backend);
    AudioEngine();
    ~AudioEngine();

    AudioEngine(const AudioEngine&) = delete;
    AudioEngine& operator=(const AudioEngine&) = delete;

    // Registro nombre->ruta de clips (hilo de juego). Sobrescribe si el nombre
    // ya existia; el playback de un clip no registrado devuelve -1.
    void registrarClip(const std::string& nombre, const std::string& ruta);
    bool clipRegistrado(const std::string& nombre) const;
    bool eliminarClip(const std::string& nombre);
    void limpiarClips();
    std::vector<std::string> nombresClips() const; // snapshot para la GUI

    // Comandos no bloqueantes (hilo de juego).
    // Devuelve -1 si el clip no existe, sino un handle >= 0 para detener().
    int reproducir(const std::string& clip, float volumen = 1.0f,
                   bool bucle = false);
    void detener(int handle);
    void detenerTodo();
    void setVolumenMaestro(float volumen);

    // Solo tests/cierre: bloquea hasta procesar toda la pila encolada.
    void esperarPilaVacia();
    std::size_t pendientes() const;

struct Impl; // pimpl publico: lo tocan las funciones libres del .cpp
    std::unique_ptr<Impl> impl_;
};

#endif // AUDIOENGINE_H