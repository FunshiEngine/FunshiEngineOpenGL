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
#ifndef IMUSICABACKEND_H
#define IMUSICABACKEND_H

#include <string>

// Estado de una pista de musica.
enum class EstadoPista {
    Detenida,      // no esta sonando
    Reproduciendo, // esta sonando
    Terminada      // llego al final y no esta en bucle
};

// Contrato del backend de musica: lo minimo que necesita el reproductor
// (GestorMusica) para sonar una pista, moverle el volumen, fundirla con otra
// y saber en que punto va.
//
// Es un contrato aparte del de los efectos de sonido (IAudioBackend) porque
// las necesidades son distintas: la musica suena mucho rato, su volumen
// cambia mientras suena, se funde con otra pista y se consulta su posicion.
// Cada backend de audio implementa el que le corresponde.
class IMusicaBackend {
public:
    virtual ~IMusicaBackend() = default;

    // Prepara una pista (abre el archivo). Devuelve un identificador > 0, o 0
    // si no se pudo abrir.
    virtual int abrirPista(const std::string& ruta) = 0;

    // Libera la pista: deja de sonar y suelta los recursos que ocupaba.
    virtual void cerrarPista(int id) = 0;

    // Empieza a sonar la pista con el volumen indicado (0..1). Devuelve false
    // si la pista no existe en el backend.
    virtual bool reproducirPista(int id, float volumen) = 0;

    // Detiene la pista.
    virtual void detenerPista(int id) = 0;

    // Volumen de una pista (0..1). Se puede cambiar mientras suena.
    virtual void volumenPista(int id, float volumen) = 0;

    // Volumen del bus: el de todas las pistas de musica juntas (0..1).
    virtual void volumenBus(float volumen) = 0;

    // Silencia el bus sin perder el volumen pedido (se aplica al alza).
    virtual void silenciarBus(bool silenciado) = 0;

    // Estado de la pista.
    virtual EstadoPista estadoPista(int id) = 0;

    // Posicion actual de la pista, en segundos (0 si no se sabe).
    virtual float posicionPista(int id) = 0;

    // Duracion total de la pista, en segundos (0 si no se sabe: una pista en
    // streaming puede no conocerla hasta que se ha decodificado bastante).
    virtual float duracionPista(int id) = 0;
};

#endif // IMUSICABACKEND_H
