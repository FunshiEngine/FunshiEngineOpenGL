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
#ifndef SHADERSOURCES_H
#define SHADERSOURCES_H

// Fuentes de los shaders incorporados al motor.
//
// Van embebidas en el binario (no en archivos): el ejecutable exportado no
// puede depender de assets extra. Solo las incluye el gestor de shaders,
// que es el dueno de los programas: el resto del motor pide el programa por
// nombre y no necesita saber de donde sale el GLSL.
//
// Las variables son inline (C++17) para que exista una sola copia en todo el
// programa, no una por unidad de traduccion que las incluya.

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

    gl_Position = uProjection * uView * world;
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
// Cada mapa 2D (direccional/spot) tiene su propio sampler (uShadowMap0..3) y
// no un array: GLSL 3.30 no deja indexar un array de samplers con una variable.
// Las luces puntuales usan un cubemap de profundidad (samplerCube uShadowCube0..3)
// y sus 6 matrices viven en el array plano uShadowMatrixCubo[] (indice = luz*6 + cara).
// uShadowLightIndex dice a que luz de las de arriba pertenece cada mapa: la
// sombra oscurece solo la contribucion de esa luz, no la de las demas.
// uShadowTexel es 1/resolucion del mapa (el PCF necesita el tamano del texel).
// uShadowDistancia es el alcance de la luz (para puntuales).
const int MAX_SHADOWS = 4;
const int kCarasCubemap = 6;
uniform sampler2DShadow uShadowMap0;
uniform sampler2DShadow uShadowMap1;
uniform sampler2DShadow uShadowMap2;
uniform sampler2DShadow uShadowMap3;
uniform samplerCube uShadowCube0;
uniform samplerCube uShadowCube1;
uniform samplerCube uShadowCube2;
uniform samplerCube uShadowCube3;
uniform mat4 uShadowMatrix[MAX_SHADOWS];
uniform mat4 uShadowMatrixCubo[MAX_SHADOWS * kCarasCubemap];
uniform float uShadowBias[MAX_SHADOWS];
uniform float uShadowStrength[MAX_SHADOWS];
uniform float uShadowTexel[MAX_SHADOWS];
uniform float uShadowDistancia[MAX_SHADOWS];
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

// PCF 3x3 sobre un mapa: promedia nueve muestras alrededor del texel para que
// el borde de la sombra no sea un escalon.
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

// Cobertura del mapa que toca el indice dado. El sampler se elige con una
// cadena de if porque el indice del mapa no puede ser una variable.
float coberturaSombra(int indice, vec2 uv, float ref, float texel) {
    if (indice == 0) return pcf3x3(uShadowMap0, uv, ref, texel);
    if (indice == 1) return pcf3x3(uShadowMap1, uv, ref, texel);
    if (indice == 2) return pcf3x3(uShadowMap2, uv, ref, texel);
    return pcf3x3(uShadowMap3, uv, ref, texel);
}

// Profundidad guardada en el cubemap de una luz puntual, leida en la direccion
// desde la luz al fragmento. El sampler va como parametro: GLSL 3.30 no deja
// elegir el sampler con un if y guardarlo en una variable local (los samplers
// no son asignables ni declarables fuera de uniforms o parametros).
float profundidadCubemap(samplerCube cubo, vec3 direccion) {
    return texture(cubo, direccion).r;
}

// Factor de sombra de una luz: 1.0 = iluminado, 0.0 = en sombra.
//
// Las luces direccionales y spot usan un mapa 2D (sampler2DShadow) con
// comparacion por hardware y PCF. Las luces puntuales usan un cubemap de
// profundidad (samplerCube): se lee la profundidad almacenada en la direccion
// desde la luz al fragmento y se compara contra la distancia del fragmento.
float factorSombra(int indiceLuz, vec3 fragPos) {
    // --- Luz puntual: cubemap de profundidad -------------------------------
    if (uLightTypes[indiceLuz] == 1) {
        for (int s = 0; s < MAX_SHADOWS; ++s) {
            if (s >= uShadowCount) break;
            if (uShadowLightIndex[s] != indiceLuz) continue;
            vec3 dir = fragPos - uLightWorldPos[indiceLuz];
            float distancia = length(dir);
            if (distancia > uShadowDistancia[indiceLuz]) return 1.0;
            vec3 dirNorm = normalize(dir);
            // ref en [0,1]: distancia relativa al alcance de la luz.
            float ref = distancia / uShadowDistancia[indiceLuz];
            float profundidad;
            if (indiceLuz == 0) profundidad = profundidadCubemap(uShadowCube0, dirNorm);
            else if (indiceLuz == 1) profundidad = profundidadCubemap(uShadowCube1, dirNorm);
            else if (indiceLuz == 2) profundidad = profundidadCubemap(uShadowCube2, dirNorm);
            else profundidad = profundidadCubemap(uShadowCube3, dirNorm);
            float cobertura = (profundidad < ref) ? 0.0 : 1.0;
            return mix(1.0, cobertura, uShadowStrength[s]);
        }
        return 1.0;
    }

    // --- Luz direccional/spot: mapa 2D con comparacion por hardware --------
    //
    // El mapa es una sampler2DShadow en modo de comparacion: el hardware compara
    // la profundidad del fragmento con la del mapa y devuelve el porcentaje de
    // cobertura.
    //
    // Si el fragmento cae fuera del mapa, o detras del plano lejano, no hay nada
    // que compare y se queda iluminado: la sombra no se repite en el borde.
    for (int s = 0; s < MAX_SHADOWS; ++s) {
        if (s >= uShadowCount) break;
        if (uShadowLightIndex[s] != indiceLuz) continue;

        vec4 clip = uShadowMatrix[s] * vec4(fragPos, 1.0);
        if (clip.w <= 0.0) return 1.0;
        vec3 ndc = clip.xyz / clip.w;
        vec2 uv = ndc.xy * 0.5 + 0.5;
        float z = ndc.z * 0.5 + 0.5;
        if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return 1.0;
        if (z > 1.0) return 1.0;

        // El sesgo se resta en profundidad: acerca el fragmento a la luz para
        // que el mapa no lo de por tapado por su propia superficie.
        float ref = z - uShadowBias[s];
        float cobertura = coberturaSombra(s, uv, ref, uShadowTexel[s]);
        // La fuerza va de 0 (sin sombra) a 1 (sombra plena).
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
    float sombra = factorSombra(i, fragPos);
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

// Pases de profundidad del mapa de sombras.
//
// El vertex shader solo proyecta la geometria con la matriz de la luz: la
// posicion del vertice en el mapa es lo que se guarda en la textura de
// profundidad. No hay colores ni materiales, asi que solo usa el atributo de
// posicion (el resto del VAO queda sin usar).
inline const char* const kDepthVertexShader = R"(#version 330 core
layout(location = 0) in vec3 aPosition;

uniform mat4 uModel;
uniform mat4 uLightMatrix;

void main() {
    gl_Position = uLightMatrix * uModel * vec4(aPosition, 1.0);
}
)";

// El fragment shader no escribe nada util: el mapa es solo de profundidad y el
// destino de dibujo no tiene buffer de color (glDrawBuffer(GL_NONE)). Se
// declara la salida igualmente, para no depender de que el driver acepte un
// fragment shader sin color.
inline const char* const kDepthFragmentShader = R"(#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(1.0);
}
)";

#endif