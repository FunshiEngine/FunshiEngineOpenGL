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
#ifndef SHADERLINEAS_H
#define SHADERLINEAS_H

// Programa de lineas de ancho constante en pixeles.
// Parte de ShaderSources.h (ver ahi las convenciones comunes).

// Shader de las lineas "gruesas" del engine (grilla, marcadores de luz/camara y
// wireframes de colliders). Sustituye a glBegin(GL_LINES)/glLineWidth, que en
// un contexto core 3.3 no existen como tales (glLineWidth solo garantiza 1 px).
//
// Cada segmento llega expandido a un quad por LineBuilder (atributos: inicio y
// fin del segmento, lado y avance). El vertex shader proyecta ambos extremos,
// calcula la perpendicular de la direccion en pantalla y desplaza el vertice
// hacia su "lado" un numero de pixeles dado por uWidth: asi el ancho es
// constante en pixeles, con cualquier distancia y en cualquier resolucion.
//
// Tambien resuelve el recorte por el plano cercano en espacio vista (uNear): un
// segmento con un extremo detras de la camara se recorta contra el plano en vez
// de estirarse desde una posicion invalida, y uno completamente detras se
// colapsa fuera del frustum (asi la grilla no "explota" al cruzarse con la
// camara). El tamano del viewport va en un vec4 (anchoPx, altoPx) y no en un
// vec2 porque el backend solo carga glUniform{1,3,4}fv.
inline const char* const kLineVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aStart;
layout(location = 1) in vec3 aEnd;
layout(location = 2) in float aSide;
layout(location = 3) in float aAlong;
layout(location = 4) in vec4 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform vec4 uViewport;  // (anchoPx, altoPx, -, -)
uniform float uWidth;    // ancho de la linea en pixeles
uniform float uNear;     // distancia al plano cercano (positivo)

out vec4 vColor;
out vec3 vWorldPosition;

void main() {
    // 1. Recorte del plano cercano en espacio vista (z hacia -near). La
    //    geometria viene en el espacio local del objeto (grilla, collider), asi
    //    que pasa por uModel antes que por la vista.
    vec3 worldS = (uModel * vec4(aStart, 1.0)).xyz;
    vec3 worldE = (uModel * vec4(aEnd, 1.0)).xyz;
    vec3 s = (uView * vec4(worldS, 1.0)).xyz;
    vec3 e = (uView * vec4(worldE, 1.0)).xyz;
    float nearZ = -uNear;
    bool sAdelante = s.z <= nearZ;
    bool eAdelante = e.z <= nearZ;
    if (!sAdelante && !eAdelante) {
        // Enteramente detras de la camara: vertice degenerado fuera de pantalla.
        gl_Position = vec4(0.0, 0.0, 2.0, 1.0);
        vColor = aColor;
        vWorldPosition = worldS;
        return;
    }
    if (!sAdelante) {
        float t = (nearZ - s.z) / (e.z - s.z);
        s = mix(s, e, t);
        worldS = mix(worldS, worldE, t);
    }
    if (!eAdelante) {
        float t = (nearZ - e.z) / (s.z - e.z);
        e = mix(e, s, t);
        worldE = mix(worldE, worldS, t);
    }

    // 2. A clip space y de ahí a NDC.
    vec4 clipS = uProjection * vec4(s, 1.0);
    vec4 clipE = uProjection * vec4(e, 1.0);
    vec2 ndcS = clipS.xy / clipS.w;
    vec2 ndcE = clipE.xy / clipE.w;

    // 3. Perpendicular en pixeles: se trabaja en pixeles y no en NDC para que el
    //    ancho sea isotropico (en NDC los ejes X e Y tienen escalas distintas).
    vec2 px = 0.5 * uViewport.xy;
    vec2 dir = (ndcE - ndcS) * px;
    float largo = length(dir);
    vec2 normal = (largo > 1e-6) ? vec2(-dir.y, dir.x) / largo : vec2(0.0);
    vec2 offsetPx = normal * (aSide * uWidth * 0.5);

    // 4. Interpola el punto sobre el segmento y le suma el desplazamiento.
    vec4 clip = mix(clipS, clipE, aAlong);
    clip.xy += (offsetPx / px) * clip.w;
    gl_Position = clip;
    vColor = aColor;
    vWorldPosition = mix(worldS, worldE, aAlong);
}
)";

inline const char* const kLineFragmentShader = R"(#version 330 core
in vec4 vColor;
in vec3 vWorldPosition;
out vec4 FragColor;

uniform vec3 uCameraPosition;
uniform float uDistanceFadeStart;
uniform float uDistanceFadeEnd;

void main() {
    float distanciaCamara = length(vWorldPosition - uCameraPosition);
    if (distanciaCamara >= uDistanceFadeEnd) discard;
    float rango = max(0.0001, uDistanceFadeEnd - uDistanceFadeStart);
    float t = clamp((distanciaCamara - uDistanceFadeStart) / rango, 0.0, 1.0);
    float opacidad = 1.0 - t * t;
    FragColor = vec4(vColor.rgb, vColor.a * opacidad);
}
)";

#endif
