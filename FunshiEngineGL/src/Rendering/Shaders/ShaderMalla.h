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
#ifndef SHADERMALLA_H
#define SHADERMALLA_H

// Programa de mallas: vertex/fragment con iluminacion, materiales, texturas y sombras.
// Parte de ShaderSources.h (ver ahi las convenciones comunes).

// Atributos: 0 = posicion (vec3), 1 = normal (vec3), 2 = uv (vec2),
// 3 = tangente (vec3), 4 = bitangente (vec3). Las UVs alimentan vUv y el muestreo
// de texturas; tangente/bitangente arman el marco TBN para el normal mapping
// cuando el Material define una mapa de normales.
inline const char* const kDefaultVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUv;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModelView;
uniform mat3 uNormalMatrix;

out vec3 vNormalWorld;
out vec3 vWorldPos;
out vec2 vUv;
out mat3 vTbn;

void main() {
    vec4 world = uModel * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vNormalWorld = uNormalMatrix * aNormal;
    vUv = aUv;

    // Marco TBN en espacio mundo: la tangente se re-ortogonaliza contra la
    // normal (Gram-Schmidt) y la bitangente compensa la orientacion (handedness)
    // con el signo del producto mixto para que las caras espejadas no se
    // quiebren. Sin normal map el shader no lo usa (todo es muestreo).
    vec3 t = normalize(uNormalMatrix * aTangent);
    vec3 n = normalize(vNormalWorld);
    t = normalize(t - dot(t, n) * n);
    vec3 b = cross(n, t);
    if (dot(cross(n, t), uNormalMatrix * aBitangent) < 0.0) b = -b;
    vTbn = mat3(t, b, n);

    gl_Position = uProjection * uModelView * vec4(aPosition, 1.0);
}
)";

inline const char* const kDefaultFragmentShader = R"(#version 330 core
in vec3 vNormalWorld;
in vec3 vWorldPos;
in vec2 vUv;
in mat3 vTbn;
out vec4 FragColor;

uniform vec3 uCameraPosition;
uniform float uDistanceFadeStart;
uniform float uDistanceFadeEnd;
uniform vec3 uGlobalAmbient;
uniform int uLightCount;
const int MAX_LIGHTS = 8;
uniform int uLightTypes[MAX_LIGHTS];       // 0 direccional, 1 punto, 2 spot
uniform vec3 uLightWorldPos[MAX_LIGHTS];   // dir para direccional, pos punto/spot
uniform vec3 uLightDir[MAX_LIGHTS];        // direccion forward de cara
uniform vec3 uLightAmbient[MAX_LIGHTS];
uniform vec3 uLightDiffuse[MAX_LIGHTS];
uniform vec3 uLightSpecular[MAX_LIGHTS];
uniform float uLightConstant[MAX_LIGHTS];
uniform float uLightLinear[MAX_LIGHTS];
uniform float uLightQuadratic[MAX_LIGHTS];
uniform float uSpotCutoffCos[MAX_LIGHTS];

// Sombras: un mapa de profundidad por luz que proyecta. MAX_SHADOWS tiene que
// coincidir con el limite de proyectores del motor (kMaxProyectoresSombra).
//
// Todo se indexa por SLOT de sombra (0..MAX_SHADOWS-1), no por indice de luz:
// uShadowLightIndex[slot] dice a que luz pertenece el mapa, y la sombra
// oscurece solo la contribucion de esa luz.
// Cada slot es un mapa 2D (direccional/spot, sampler2DShadow uShadowMap<slot>)
// o un cubemap (puntual, samplerCube uShadowCube<slot>) segun uShadowEsCubo.
// GLSL 3.30 no deja indexar samplers con una variable: se eligen con if.
// El cubemap guarda distancia(luz, fragmento) / alcance, en [0, 1].
// uShadowTexel es 1/resolucion; uShadowTexelMundo es el tamano de un texel
// en unidades del mundo (para el normal offset).
const int MAX_SHADOWS = 4;
uniform sampler2DShadow uShadowMap0;
uniform sampler2DShadow uShadowMap1;
uniform sampler2DShadow uShadowMap2;
uniform sampler2DShadow uShadowMap3;
uniform samplerCube uShadowCube0;
uniform samplerCube uShadowCube1;
uniform samplerCube uShadowCube2;
uniform samplerCube uShadowCube3;
uniform mat4 uShadowMatrix[MAX_SHADOWS];
uniform int uShadowEsCubo[MAX_SHADOWS];
uniform vec3 uShadowPosLuz[MAX_SHADOWS];
uniform float uShadowDistancia[MAX_SHADOWS];
uniform float uShadowBias[MAX_SHADOWS];
uniform float uShadowStrength[MAX_SHADOWS];
uniform float uShadowTexel[MAX_SHADOWS];
uniform float uShadowTexelMundo[MAX_SHADOWS];
uniform int uShadowLightIndex[MAX_SHADOWS];
uniform int uShadowCount;

uniform vec4 uMaterialAmbient;
uniform vec4 uMaterialDiffuse;
uniform vec4 uMaterialSpecular;
uniform vec4 uMaterialEmission;
uniform float uMaterialShininess;

// Texturas opcionales del Material. Con su flag en 0 el material se colorea
// solo con los uniforms; cuando la textura esta, su texel multiplica
// (MODULA) al canal correspondiente, replicando el modo GL_MODULATE de la GL
// antigua. La normal map perturba la normal en espacio tangente (vTbn).
uniform sampler2D uDiffuseTex;
uniform int uUseTexture;
uniform sampler2D uSpecularTex;
uniform int uUseSpecularMap;
uniform sampler2D uEmissionTex;
uniform int uUseEmissionMap;
uniform sampler2D uNormalTex;
uniform int uUseNormalMap;

vec3 normalizarSeguro(vec3 v) {
    float len = length(v);
    return (len < 1e-8) ? vec3(0.0) : v / len;
}

// PCF 3x3 sobre un mapa 2D: promedia nueve comparaciones alrededor del texel
// para que el borde de la sombra no sea un escalon.
float pcf3x3(sampler2DShadow mapa, vec2 uv, float ref, float texel) {
    float suma = 0.0;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            vec2 offset = vec2(float(dx), float(dy)) * texel;
            suma += texture(mapa, vec3(uv + offset, ref));
        }
    }
    return suma / 9.0;
}

float coberturaMapa(int slot, vec2 uv, float ref, float texel) {
    if (slot == 0) return pcf3x3(uShadowMap0, uv, ref, texel);
    if (slot == 1) return pcf3x3(uShadowMap1, uv, ref, texel);
    if (slot == 2) return pcf3x3(uShadowMap2, uv, ref, texel);
    return pcf3x3(uShadowMap3, uv, ref, texel);
}

// Distancia normalizada guardada en el cubemap del slot, en la direccion dada.
float distanciaCubo(int slot, vec3 direccion) {
    if (slot == 0) return texture(uShadowCube0, direccion).r;
    if (slot == 1) return texture(uShadowCube1, direccion).r;
    if (slot == 2) return texture(uShadowCube2, direccion).r;
    return texture(uShadowCube3, direccion).r;
}

// PCF de un cubemap: compara en el centro y en 12 direcciones vecinas (las
// aristas de un cubo) separadas lo que mide un texel a esa distancia.
const vec3 kDiscoCubo[12] = vec3[](
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, 1, 0), vec3(-1, -1, 0),
    vec3(1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, 1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, 1, -1), vec3(0, -1, 1), vec3(0, -1, -1));

float coberturaCubo(int slot, vec3 direccion, float ref, float radio) {
    float suma = (distanciaCubo(slot, direccion) < ref) ? 0.0 : 1.0;
    for (int k = 0; k < 12; ++k) {
        float d = distanciaCubo(slot, direccion + kDiscoCubo[k] * radio);
        suma += (d < ref) ? 0.0 : 1.0;
    }
    return suma / 13.0;
}

// Factor de sombra de la luz 'indiceLuz': 1.0 = iluminado, 0.0 = en sombra.
//
// N es la normal del fragmento y L la direccion hacia la luz. El punto que se
// consulta se desplaza por la normal (normal offset) en proporcion a lo
// inclinada que esta la superficie respecto de la luz, y el sesgo de
// profundidad crece con la pendiente: asi no hay acne en superficies
// rasantes sin despegar la sombra en las que miran a la luz.
float factorSombra(int indiceLuz, vec3 fragPos, vec3 N, vec3 L) {
    for (int s = 0; s < MAX_SHADOWS; ++s) {
        if (s >= uShadowCount) break;
        if (uShadowLightIndex[s] != indiceLuz) continue;

        float ndotl = clamp(dot(N, L), 0.0, 1.0);
        float pendiente = 1.0 - ndotl;
        vec3 punto = fragPos + N * (uShadowTexelMundo[s] * (0.5 + 1.5 * pendiente));
        float sesgo = uShadowBias[s] * (1.0 + 2.0 * pendiente);

        if (uShadowEsCubo[s] == 1) {
            vec3 desdeLuz = punto - uShadowPosLuz[s];
            float distancia = length(desdeLuz);
            if (distancia >= uShadowDistancia[s]) return 1.0;
            float ref = distancia / uShadowDistancia[s] - sesgo;
            float radio = 3.0 * uShadowTexel[s];
            float cobertura = coberturaCubo(s, desdeLuz / max(distancia, 1e-5),
                                            ref, radio);
            return mix(1.0, cobertura, uShadowStrength[s]);
        }

        // Mapa 2D con comparacion por hardware (sampler2DShadow). Fuera del
        // mapa o detras del plano lejano no hay nada que comparar: iluminado.
        vec4 clip = uShadowMatrix[s] * vec4(punto, 1.0);
        if (clip.w <= 0.0) return 1.0;
        vec3 ndc = clip.xyz / clip.w;
        vec2 uv = ndc.xy * 0.5 + 0.5;
        float z = ndc.z * 0.5 + 0.5;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;
        if (z > 1.0) return 1.0;
        float cobertura = coberturaMapa(s, uv, z - sesgo, uShadowTexel[s]);
        return mix(1.0, cobertura, uShadowStrength[s]);
    }
    return 1.0;
}

vec3 contribucionLuz(int i, vec3 N, vec3 V, vec3 fragPos, vec3 baseDiffuse,
                     vec3 baseSpecular) {
    int type = uLightTypes[i];

    vec3 L;
    float atenuacion = 1.0;
    if (type == 0) {
        L = -normalizarSeguro(uLightWorldPos[i]);
    } else {
        vec3 toLight = uLightWorldPos[i] - fragPos;
        float dist = length(toLight);
        if (dist < 1e-5) return uLightAmbient[i] * uMaterialAmbient.rgb;
        L = toLight / dist;
        atenuacion = 1.0 / max(1e-5,
            uLightConstant[i] + uLightLinear[i] * dist +
            uLightQuadratic[i] * dist * dist);
    }

    // El ambiente de cada luz NO se atenua con la distancia (GL: matAmbient *
    // lightAmbient, sin factor). El diffuse/specular si.
    vec3 resultado = uLightAmbient[i] * uMaterialAmbient.rgb;

    float factorSpot = 1.0;
    if (type == 2) {
        float cosAngulo = dot(-L, normalizarSeguro(uLightDir[i]));
        if (cosAngulo < uSpotCutoffCos[i]) {
            return resultado; // fuera del cono: solo ambiente
        }
        factorSpot = cosAngulo; // GL_SPOT_EXPONENT = 1 -> pow(cos, 1)
    }

    float ndotl = max(dot(N, L), 0.0);
    vec3 difuso = ndotl * uLightDiffuse[i] * baseDiffuse;

    vec3 especular = vec3(0.0);
    if (ndotl > 0.0 && uMaterialShininess > 0.0) {
        vec3 H = normalizarSeguro(L + V);
        float ndoth = max(dot(N, H), 0.0);
        especular = pow(ndoth, uMaterialShininess) *
                    uLightSpecular[i] * baseSpecular;
    }

    // La sombra de la luz (si esta la proyecta) atenua su difuso y su
    // especular; el ambiente no se toca.
    float sombra = factorSombra(i, fragPos, N, L);
    resultado += (difuso + especular) * (atenuacion * factorSpot * sombra);
    return resultado;
}

void main() {
    float distanciaCamara = length(vWorldPos - uCameraPosition);
    if (distanciaCamara >= uDistanceFadeEnd) discard;
    float rangoDifuminado = max(0.0001, uDistanceFadeEnd - uDistanceFadeStart);
    float t = clamp((distanciaCamara - uDistanceFadeStart) / rangoDifuminado,
                    0.0, 1.0);
    float opacidadDistancia = 1.0 - t * t;

    // Normal del fragmento: con normal map se perturba en espacio tangente
    // (vTbn); sin el, es la normal interpolada de la geometria.
    vec3 N = normalizarSeguro(vNormalWorld);
    if (uUseNormalMap == 1) {
        vec3 tn = normalize(texture(uNormalTex, vUv).rgb * 2.0 - 1.0);
        N = normalizarSeguro(vTbn * tn);
    }
    vec3 V = normalizarSeguro(uCameraPosition - vWorldPos);

    // El diffuse modulado que ven las luces: texel de textura por diffuse del
    // material (GL_MODULATE). Sin textura (o con la malla sin UVs) queda igual.
    vec3 baseDiffuse = uMaterialDiffuse.rgb;
    if (uUseTexture == 1) baseDiffuse *= texture(uDiffuseTex, vUv).rgb;
    vec3 baseSpecular = uMaterialSpecular.rgb;
    if (uUseSpecularMap == 1) baseSpecular *= texture(uSpecularTex, vUv).rgb;

    // GL_LIGHT_MODEL_AMBIENT * material ambiente + emision, una sola vez.
    vec3 color = uGlobalAmbient * uMaterialAmbient.rgb;
    for (int i = 0; i < MAX_LIGHTS; ++i) {
        if (i >= uLightCount) break;
        color += contribucionLuz(i, N, V, vWorldPos, baseDiffuse, baseSpecular);
    }
    vec3 emision = uMaterialEmission.rgb;
    if (uUseEmissionMap == 1) emision *= texture(uEmissionTex, vUv).rgb;
    color += emision;

    FragColor = vec4(color, uMaterialDiffuse.a * opacidadDistancia);
}
)";

#endif
