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
#ifndef SHADERSOMBRAS_H
#define SHADERSOMBRAS_H

// Programas de profundidad de los mapas de sombra (2D y cubemap).
// Parte de ShaderSources.h (ver ahi las convenciones comunes).

// Pases de profundidad del mapa de sombras.
//
// Mapas 2D (direccional/spot): el vertex shader proyecta la geometria con la
// matriz de la luz y la profundidad de hardware es lo que queda en el mapa. No
// hay colores ni materiales: solo se usa el atributo de posicion.
inline const char* const kDepthVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uModelLight;
void main() {
    gl_Position = uModelLight * vec4(aPosition, 1.0);
}
)";

// El destino no tiene buffer de color (glDrawBuffer(GL_NONE)); la salida se
// declara igual para no depender de que el driver acepte un fragment shader
// sin color.
inline const char* const kDepthFragmentShader = R"(#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0);
}
)";

// Cubemap de una luz puntual: en vez de la profundidad de la perspectiva (que
// no es lineal y depende de la cara) se guarda distancia(luz, fragmento) /
// alcance. Es lo que compara el shader de mallas en coberturaCubo.
inline const char* const kDepthCuboVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
uniform mat4 uModel;
uniform mat4 uModelLight;
out vec3 vWorldPos;
void main() {
    vWorldPos = vec3(uModel * vec4(aPosition, 1.0));
    gl_Position = uModelLight * vec4(aPosition, 1.0);
}
)";

inline const char* const kDepthCuboFragmentShader = R"(#version 330 core
in vec3 vWorldPos;
uniform vec3 uPosLuz;
uniform float uAlcance;
out vec4 FragColor;
void main() {
    gl_FragDepth = clamp(length(vWorldPos - uPosLuz) / uAlcance, 0.0, 1.0);
    FragColor = vec4(1.0);
}
)";

#endif
