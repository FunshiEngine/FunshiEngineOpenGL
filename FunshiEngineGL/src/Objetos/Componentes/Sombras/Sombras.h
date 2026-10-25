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
#ifndef SOMBRAS_H
#define SOMBRAS_H
#include "../Component.h"

// Ajustes de la sombra que proyecta una luz.
//
// Es un componente aparte y no una propiedad de Light a proposito: el formato
// binario de los componentes no tiene versionado, asi que anadir campos a un
// componente que ya existe invalidaria las escenas guardadas (al leerlas, los
// bytes de los campos nuevos se comerian los del siguiente componente). Un
// componente nuevo es aditivo: las escenas viejas no lo tienen y siguen
// leyendose igual.
//
// Se adjunta al mismo objeto que la luz: el motor busca el componente
// Sombras del dueno de cada luz para leer sus ajustes.
class Sombras : public Component {
private:
    // Si esta apagado, la luz no proyecta sombra (pero el resto de sus
    // ajustes se conservan).
    bool activado = true;
    // Lado del mapa de sombras en pixeles (potencia de dos). Un mapa mas
    // grande da bordes mas nitidos a cambio de mas memoria y mas tiempo de
    // dibujado en la pasada de sombras.
    int resolucion = 1024;
    // Desplazamiento de la profundidad al comparar: sin el, la propia
    // superficie que proyecta la sombra se auto-sombrea (acne).
    float sesgo = 0.0015f;
    // Cuanto se oscurece la zona en sombra: 0 deja la sombra sin efecto y 1
    // la deja completa.
    float fuerza = 1.f;
    // Semi-lado del frustum ortografico de la luz (la sombra abarca una caja
    // de lado 2*radio alrededor del foco).
    float radio = 20.f;
    // Alcance de la sombra: hasta que distancia desde la luz se dibujan los
    // proyectores.
    float distancia = 60.f;

protected:
    void serializeComponent(std::ofstream* file) override;
    void deserializeComponent(std::ifstream* file) override;

public:
    Sombras();

    bool estaActivado() const { return activado; }
    void setActivado(bool valor) { activado = valor; }

    int getResolucion() const { return resolucion; }
    // La resolucion se acota al rango util y se ajusta a una potencia de dos
    // (las texturas de profundidad lo son mucho mejor asi).
    void setResolucion(int valor);

    float getSesgo() const { return sesgo; }
    void setSesgo(float valor);

    float getFuerza() const { return fuerza; }
    void setFuerza(float valor);

    float getRadio() const { return radio; }
    void setRadio(float valor);

    float getDistancia() const { return distancia; }
    void setDistancia(float valor);
};
#endif
