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
#ifndef SHADER2D_H
#define SHADER2D_H

// Programas de sprites 2D y de particulas.
// Parte de ShaderSources.h (ver ahi las convenciones comunes).

// Shader de sprites 2D (componente SpriteRenderer): quad texturizado sin
// iluminacion, dibujado con el pipeline moderno sobre la malla de
// Primitivas2D::quad(). El layout del VAO es el de createMesh: posicion en el
// atributo 0 y UV en el atributo 2 (el 1 queda para la normal, que un sprite no
// usa). No expone tangentes ni normales.
//
// El recorte de la region de la textura (spritesheet) viaja en uUvRect
// (u0,v0,u1,v1) y se mezcla por vertice con la UV base del quad; asi una sola
// malla compartida sirve para cualquier region, sin duplicar geometria. El tinte
// y la opacidad (fade del componente) van en uTint y se multiplican sobre el
// texel (modo MODULATE, igual que el material 3D).
inline const char* const kSpriteVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 2) in vec2 aUv;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec4 uUvRect;  // (u0, v0, u1, v1)

out vec2 vUv;

void main() {
    vUv = mix(uUvRect.xy, uUvRect.zw, aUv);
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
}
)";

inline const char* const kSpriteFragmentShader = R"(#version 330 core
in vec2 vUv;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform vec4 uTint;

void main() {
    vec4 color = uTint;
    if (uUseTexture == 1) color *= texture(uTexture, vUv);
    if (color.a <= 0.0001) discard;
    FragColor = color;
}
)";

// Particulas: el quad de cada particula ya viene expandido en el mundo (con
// su tamano y su rotacion aplicados), asi que el vertex shader solo proyecta.
// El color va por vertice (cada particula tiene el suyo, y lo cambia la curva
// del perfil a lo largo de la vida) y la textura es opcional.
inline const char* const kParticulasVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosicion;
layout(location = 1) in vec2 aUv;
layout(location = 2) in vec4 aColor;

uniform mat4 uView;
uniform mat4 uProjection;

out vec2 vUv;
out vec4 vColor;

void main() {
    vUv = aUv;
    vColor = aColor;
    gl_Position = uProjection * uView * vec4(aPosicion, 1.0);
}
)";

inline const char* const kParticulasFragmentShader = R"(#version 330 core
in vec2 vUv;
in vec4 vColor;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;

void main() {
    vec4 color = vColor;
    if (uUseTexture == 1) color *= texture(uTexture, vUv);
    if (color.a <= 0.0001) discard;
    FragColor = color;
}
)";

#endif
