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
#ifndef GUIAEJE_H
#define GUIAEJE_H

#include "Difuminado.h"
#include "LineBuilder.h"

// Geometria de la GUIA DE EJE del editor: al seleccionar un objeto y apretar
// X, Y o Z se dibuja la recta sobre la que el usuario puede moverlo, tomando
// ese eje como variable y FIJANDO las otras dos coordenadas a las del objeto.
//
// Ejemplo (pulsar X sobre un objeto en (3, 2, -5)): la recta es
// (t, 2, -5) para todo t; es decir X varia y Y/Z quedan clavadas en la
// posicion del objeto. Ese es exactamente el compromiso del gizmo de
// traslacion restringido a un solo eje, pero dibujado como guia en la escena:
// la recta va hasta el horizonte y se difumina como la grilla, y el gizmo que
// se dibuja encima trae solo la flecha de ese eje para arrastrar.
//
// La geometria es CPU pura a proposito (nada de OpenGL ni glm, solo la matriz
// global como array de floats), igual que LineBuilder, para poder ejercitarse
// en los targets headless (tests/RenderingTests.cpp). El dibujado lo hace la
// capa de Rendering con LineBuilder/LineBatch.
namespace GuiaEje {

// Ejes validos. kSinGuia = ninguna guia activa.
constexpr int kSinGuia = -1;
constexpr int kEjeX = 0;
constexpr int kEjeY = 1;
constexpr int kEjeZ = 2;

// Origen (posicion del objeto en el mundo) y direccion unitaria de la recta.
struct Eje {
    float origen[3];
    float dir[3];
};

// El difuminado de la guia es el MISMO tipo que el de la grilla (struct
// Difuminado, en Difuminado.h) y lo arma el llamador con el radio que eligio el
// usuario, para que la guia se desvanezca en el mismo circulo que el piso y no
// se desincronice al cambiar ese radio.

// Calcula el origen y la direccion de la guia a partir de la matriz GLOBAL del
// objeto seleccionado (16 floats column-major, como la que produce
// buildMatrixFromTransform) y el eje pulsado.
//
// "coordenadasGlobales" decide el sistema de referencia, sincronizado con el
// toggle LOCAL/GLOBAL del gizmo (tecla G):
//  - true  (GLOBAL): la direccion es el eje del MUNDO, asi que las otras dos
//    coordenadas quedan fijadas a los valores del objeto. Es la lectura literal
//    de "fijar las otras dos coordenadas".
//  - false (LOCAL): la direccion es el eje LOCAL del objeto (rota con el), y se
//    toma de la columna correspondiente de la matriz global, ya normalizada
//    para que la escala del objeto no cambie el largo de la guia.
//
// Devuelve false (y no toca "out") si el eje no es valido o si la matriz trae
// NaN/Inf. Tambien si el eje local es degenerado (columna de amplitud ~0, que
// solo pasa con una escala nula): sin direccion utilizable no hay guia.
bool calcularEje(const float matrizGlobal[16], int eje, bool coordenadasGlobales,
                 Eje* out);

// Agrega a "out" la recta completa de la guia: desde el origen hacia atras y
// hacia adelante hasta el horizonte, recortada alradio "dif.fin" (fuera de el
// no se pinta, igual que la grilla) y subdividida en "dif.subdivisiones" trozos
// para que el difuminado quede suave. Cada vertice lleva su propio alpha y el
// batch lo interpola a lo largo del tramo.
//
// "camaraMundo" es la posicion de la camara en el mundo (la misma que usa la
// grilla para su difuminado). La distancia aqui es 3D, no horizontal: asi el
// eje Y tambien se desvanece al subir, y no solo los del plano.
void emitir(LineBuilder& out, const Eje& eje, const float camaraMundo[3],
            const float color[3], const Difuminado& dif);

// Color de la guia por eje, con la convencion de los ejes del gizmo y de la
// grilla: X rojo, Y verde, Z azul. Asi el usuario reconoce de un vistazo que
// guia es cual. "rgba" recibe 4 floats.
void colorEje(int eje, float rgba[4]);

// Color con el que se dibuja la guia: el color de base del eje ajustado por
// contraste contra "referencia" (el color EFECTIVO de la grilla, o el fondo del
// viewport cuando no hay grilla visible), con la misma regla que usa la grilla
// para sus ejes. Sin ese ajuste el eje se pierde sobre una grilla de su mismo
// color; con el, la guia se sigue leyendo como el eje que es.
//
// "referencia" en nullptr deja el color de base sin tocar. "rgba" en nullptr no
// hace nada.
void colorEfectivo(int eje, const float referencia[3], float rgba[4]);

} // namespace GuiaEje

#endif // GUIAEJE_H
