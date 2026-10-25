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

// Implementacion de stb_image en una sola unidad de traduccion: el resto del
// motor (el cargador de texturas y los iconos de la GUI) usa solo las
// declaraciones del header. Vive junto a la libreria, no en un modulo del
// motor, para que ningun modulo sea dueno de una libreria de terceros.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
