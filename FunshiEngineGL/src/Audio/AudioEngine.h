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
// Los archivos de audio se referencian por RUTA: el componente que lo pide
// guarda la ruta del archivo (la que el usuario arrastro al componente) y el
// motor reproduce ese archivo. No hay catalogo por nombre ni carpeta fija: el
// archivo puede estar en cualquier lado dentro del proyecto.
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

    // Comandos no bloqueantes (hilo de juego).
    // Reproduce el archivo de la ruta dada. Devuelve -1 si la ruta esta vacia o
    // el archivo no existe (queda avisado en el log), sino un handle >= 0 para
    // detener(). `posicion` (opcional): si se pasa, el sonido es espacial (3D)
    // y nace en esa coordenada del mundo; si es nullptr, el sonido es global
    // (2D).
    int reproducir(const std::string& ruta, float volumen = 1.0f,
                   bool bucle = false, const float* posicion = nullptr);
    void detener(int handle);
    void detenerTodo();
    void setVolumenMaestro(float volumen);

    // Reposiciona un sonido activo (para que siga a su objeto cada frame).
    void setPosicionSonido(int handle, float x, float y, float z);
    // Posiciona el listener (camara activa) para la spatializacion 3D.
    void setListenerPosition(float x, float y, float z);

    // --- Musica -------------------------------------------------------
    // La musica es un contrato aparte del de los efectos (IMusicaBackend):
    // una pista sona mucho rato, su volumen cambia mientras suena, se funde
    // con otra y se puede consultar su posicion. El reproductor (GestorMusica)
    // vive en el hilo de audio, asi que todo esto se encola igual que los
    // efectos y el hilo del juego nunca toca el hardware de audio.
    //
    // Las pistas se referencian por RUTA, igual que los efectos: no hay
    // catalogo por nombre ni carpeta fija. La lista es ordenada y es la que
    // se sigue al pasar a la pista siguiente.
    void establecerMusicas(const std::vector<std::string>& rutas,
                            bool repetir = true);
    // La lista de pistas actual (copia, para la GUI).
    std::vector<std::string> musicas() const;

    // Reproduce la pista de la RUTA dada (no hace falta que este en la lista:
    // se puede tocar cualquier archivo). Devuelve false si la ruta esta vacia
    // o el archivo no existe (queda avisado en el log). duracionFundido
    // negativo usa la duracion por defecto del reproductor; 0 cambia de pista
    // de inmediato.
    bool reproducirMusica(const std::string& ruta, float volumen = 1.0f,
                           float duracionFundido = -1.0f);
    // Detiene la musica (con fundido de salida opcional).
    void detenerMusica(float duracionFundido = 0.0f);
    // Volumen del bus de musica (todas las pistas juntas).
    void setVolumenMusica(float volumen);
    // Silencia el bus sin perder el volumen pedido.
    void silenciarMusica(bool silenciado);
    // Avanza el reproductor: el fundido en curso y el paso automatico a la
    // pista siguiente. Lo llama la escena con su deltaTime.
    void actualizarMusica(float deltaTime);

    // Solo tests/cierre: bloquea hasta procesar toda la pila encolada.
    void esperarPilaVacia();
    std::size_t pendientes() const;

struct Impl; // pimpl publico: lo tocan las funciones libres del .cpp
    std::unique_ptr<Impl> impl_;
};

#endif // AUDIOENGINE_H