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
#ifndef ATAJOSEDITOR_H
#define ATAJOSEDITOR_H

// Reglas de los atajos del editor que compiten con el teclado de ImGui.
//
// Vive en un header propio (mismo criterio que SoltarEnCarpeta.h) para poder
// fijar la decision headless: son dos lineas de logica, pero la diferencia
// entre "guarda siempre" y "guarda solo si no hay un campo de texto enfocado" es
// invisible hasta que el usuario pierde un cambio, y eso no se puede observar
// sin la UI.

namespace AtajosEditor {

// Ctrl+S: guardar no le quita nada al campo de texto (la "S" suelta si, y esa
// no lleva Ctrl), asi que el atajo tiene que funcionar SIEMPRE. Con un InputText
// enfocado, el guard anterior le devolvia la tecla al campo y el usuario perdia
// el cambio en silencio: el flujo reportado era renombrar un objeto y guardar
// sin hacer clic en otro lado.
inline bool debeGuardar(bool campoDeTextoActivo) {
    (void)campoDeTextoActivo;
    return true;
}

// Ctrl+Z (deshacer) y Ctrl+Y (rehacer) SI chocan con un campo de texto: dentro
// de un InputText esas combinaciones son el deshacer/rehacer del propio campo,
// asi que ahi el editor le cede la tecla.
inline bool cedeAlCampoDeTexto(bool campoDeTextoActivo) {
    return campoDeTextoActivo;
}

} // namespace AtajosEditor

#endif // ATAJOSEDITOR_H
