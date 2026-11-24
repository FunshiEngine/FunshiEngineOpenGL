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
#ifndef SHADERCIELO_H
#define SHADERCIELO_H

// Programas del cielo degradado y del skybox (cubemap).
// Parte de ShaderSources.h (ver ahi las convenciones comunes).

inline const char* const kSkyVertexShader = R"(#version 330 core
// Fullscreen triangle (no VBO needed): gl_VertexID 0..2 maps to the three
// corners of a triangle that covers the viewport. Solo emite la coordenada NDC
// del pixel: el color sale de la DIRECCION de vista de ese pixel, no de su
// posicion en pantalla.
out vec2 vNdc;
void main() {
    vec2 pos[3] = vec2[3](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );
    vec2 p = pos[gl_VertexID];
    vNdc = p;
    gl_Position = vec4(p, 0.0, 1.0);
}
)";

inline const char* const kSkyFragmentShader = R"(#version 330 core
in vec2 vNdc;
out vec4 FragColor;
uniform vec3 uColorTop;    // color del cenit (mirando bien hacia arriba)
uniform vec3 uColorBottom; // color del suelo (mirando bien hacia abajo)
uniform mat4 uInvViewProj; // inversa de projection*view
uniform vec3 uCamPos;      // posicion de la camara en el mundo
uniform float uTransicion; // media anchura de la transicion en unidades de dir.y
void main() {
    // Rayo de vista de ESTE pixel en espacio de mundo: se des-proyecta el NDC
    // al plano lejano y se resta la camara. Asi el color depende de hacia donde
    // se mira y no de donde cae el pixel: el cielo va con la camara.
    vec4 lejano = uInvViewProj * vec4(vNdc, 1.0, 1.0);
    vec3 dir = normalize(lejano.xyz / lejano.w - uCamPos);

    // dir.y va de -1 (suelo) a +1 (cenit) y vale 0 en el horizonte: el degradado
    // se reparte alrededor del horizonte y satura en los dos extremos.
    float t = clamp(dir.y / uTransicion * 0.5 + 0.5, 0.0, 1.0);
    FragColor = vec4(mix(uColorBottom, uColorTop, t), 1.0);
}
)";

// Cubemap del cielo (el cubo centrado en la camara, con las caras del
// Skybox). El vertice deja el cubo en el plano lejano (z = w).
inline const char* const kCubemapVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vTexCoord;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    vTexCoord = aPos;
    vec4 pos = uProjection * uView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // z = w para estar en el plano lejano
}
)";

inline const char* const kCubemapFragmentShader = R"(#version 330 core
in vec3 vTexCoord;
out vec4 FragColor;
uniform samplerCube uSkybox;
void main() {
    FragColor = texture(uSkybox, vTexCoord);
}
)";

#endif
