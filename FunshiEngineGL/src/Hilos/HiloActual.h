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
#ifndef HILOACTUAL_H
#define HILOACTUAL_H

namespace Hilos {

// Afinidad de hilo: que codigo puede correr en que hilo.
//
// En un motor grafico no todo puede ejecutarse en cualquier hilo:
//   - el contexto de OpenGL pertenece al hilo que lo creo,
//   - ImGui no es thread-safe,
//   - CEF usa el hilo principal como hilo de interfaz (con el bombeo de
//     mensajes externo, todos sus callbacks corren ahi),
//   - el JNIEnv solo es valido en el hilo que creo la JVM.
//
// Estas funciones son la forma de comprobarlo antes de tocar esas APIs: lo
// que no se puede hacer desde un hilo secundario, se hace en el principal (o
// se encola con el patron de mutacion diferida que ya usa el modulo de
// scripts: el worker pide, el hilo principal aplica).
//
// El hilo principal se registra al arrancar el motor (fijarHiloPrincipal) en
// el arranque del editor y en el del juego exportado. Si no se ha
// registrado, esHiloPrincipal responde false: es mas seguro dudar de estar
// en el hilo principal que creerlo y tocar APIs que no se pueden tocar.
void fijarHiloPrincipal();

// true si el hilo que llama es el hilo principal del motor.
bool esHiloPrincipal();

}  // namespace Hilos

#endif  // HILOACTUAL_H
