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
#include "RenderingTestsDetalle.h"

// El difuminado va POR VERTICE: el primer vertice (cerca de la camara) es opaco
// y el ultimo (en el horizonte) es transparente.
void testDifuminadoPorVertice() {
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeZ, true, &e), "eje calculado");

    const float camara[3] = {0.0f, 8.0f, 40.0f};
    const float color[3] = {0.3f, 0.5f, 1.0f};
    LineBuilder b;
    GuiaEje::emitir(b, e, camara, color, kDif);
    CHECK(!b.vacio(), "la guia emite geometria");

    // El vertice 0 es el extremo mas cercano a la camara; el ultimo de cada
    // tramo es el mas lejano. Con el difuminado, el alpha arranca en ~0 (el
    // origen esta lejos de la camara) y los dos extremos lejanos valen 0.
    const float* primero = vertice(b, 0);
    const float* ultimo = vertice(b, b.cantidadVertices() - 1);
    const int off = static_cast<int>(LineBuilder::offsetColor() / sizeof(float));
    CHECK(primero[off + 3] < 0.05f, "el extremo lejano arranca casi invisible");
    CHECK(ultimo[off + 3] < 0.05f, "el extremo del horizonte es invisible");

    // Y el color es el del eje en todos los vertices (solo varia el alpha).
    bool colorOk = true;
    for (std::size_t v = 0; v < b.cantidadVertices(); ++v) {
        const float* f = vertice(b, v);
        if (!cerca(f[off], color[0]) || !cerca(f[off + 1], color[1]) ||
            !cerca(f[off + 2], color[2]))
            colorOk = false;
    }
    CHECK(colorOk, "el color del eje se mantiene, solo cambia la opacidad");
}

// Casos de rechazo: sin guia valida no se dibuja nada.
void testGuiaDefensiva() {
    float m[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, m);
    GuiaEje::Eje e;
    e.origen[0] = 42.0f;  // se marca para verificar que no se toca
    e.dir[0] = 7.0f;

    CHECK(!GuiaEje::calcularEje(m, GuiaEje::kSinGuia, true, &e),
          "sin guia activa no hay eje");
    CHECK(!GuiaEje::calcularEje(m, 3, true, &e), "eje fuera de rango");
    CHECK(!GuiaEje::calcularEje(m, -2, true, &e), "eje negativo");
    CHECK(!GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, nullptr),
          "salida nula se rechaza");
    CHECK(!GuiaEje::calcularEje(nullptr, GuiaEje::kEjeX, true, &e),
          "matriz nula se rechaza");
    CHECK(cerca(e.origen[0], 42.0f) && cerca(e.dir[0], 7.0f),
          "el eje no se toca cuando se rechaza");

    // Matriz corrupta: no debe propagar NaN.
    float mala[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, mala);
    mala[12] = std::nanf("");
    CHECK(!GuiaEje::calcularEje(mala, GuiaEje::kEjeX, true, &e),
          "matriz con NaN se rechaza");
    float infinita[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, infinita);
    infinita[13] = HUGE_VALF;
    CHECK(!GuiaEje::calcularEje(infinita, GuiaEje::kEjeX, true, &e),
          "matriz con Inf se rechaza");

    // Escala nula en el eje pedido: la columna no da direccion utilizable.
    float degenerada[16];
    matrizTraslada(1.0f, 2.0f, 3.0f, degenerada);
    degenerada[4] = degenerada[5] = degenerada[6] = 0.0f;
    CHECK(!GuiaEje::calcularEje(degenerada, GuiaEje::kEjeY, false, &e),
          "eje local degenerado (escala 0) se rechaza");
}

// emitir() con parametros invalidos no debe generar geometria ni leer memoria.
void testEmiteDefensiva() {
    float m[16];
    matrizTraslada(0.0f, 0.0f, 0.0f, m);
    GuiaEje::Eje e;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &e), "eje calculado");

    const float camara[3] = {0.0f, 8.0f, 40.0f};
    const float color[3] = {1.0f, 0.25f, 0.25f};
    LineBuilder b;

    Difuminado dif;
    dif.fin = 0.0f;
    GuiaEje::emitir(b, e, camara, color, dif);
    CHECK(b.vacio(), "un horizonte de radio 0 no dibuja nada");

    GuiaEje::emitir(b, e, nullptr, color, kDif);
    CHECK(b.vacio(), "camara nula no dibuja nada");

    GuiaEje::emitir(b, e, camara, nullptr, kDif);
    CHECK(b.vacio(), "color nulo no dibuja nada");

    // Recta enteramente fuera del horizonte: no se pinta (mismo criterio que la
    // grilla con su circulo).
    GuiaEje::Eje lejos;
    CHECK(GuiaEje::calcularEje(m, GuiaEje::kEjeX, true, &lejos), "eje auxiliar");
    for (int k = 0; k < 3; ++k) lejos.origen[k] = 100000.0f;
    GuiaEje::emitir(b, lejos, camara, color, kDif);
    CHECK(b.vacio(), "una recta fuera del horizonte no se dibuja");
}

// El color identifica el eje con la convencion del gizmo y de la grilla.
void testGuiaColor() {
    float x[4], y[4], z[4];
    GuiaEje::colorEje(GuiaEje::kEjeX, x);
    GuiaEje::colorEje(GuiaEje::kEjeY, y);
    GuiaEje::colorEje(GuiaEje::kEjeZ, z);

    CHECK(x[0] > x[1] && x[0] > x[2], "la guia de X es roja");
    CHECK(y[1] > y[0] && y[1] > y[2], "la guia de Y es verde");
    CHECK(z[2] > z[0] && z[2] > z[1], "la guia de Z es azul");
    CHECK(x[3] == 1.0f && y[3] == 1.0f && z[3] == 1.0f,
          "los colores base son opacos (el fade va aparte)");
}

// El color con el que se dibuja la guia es el del eje AJUSTADO por contraste
// contra el color de referencia (el de la grilla): sin ese ajuste el eje se
// pierde sobre una grilla de su mismo color, y el ajuste no puede cambiar de
// eje, porque reconocer el eje de un vistazo es justamente lo que aporta el
// color.
//
// El ajuste escala el brillo, asi que con un color de eje saturado no siempre
// puede llegar al contraste minimo (el canal se recorta en 1.0 y no hay mas
// margen): lo que se exige aqui es que la diferencia CREZCA y que se aleje en
// la direccion correcta.
void testGuiaColorEfectivo() {
    float baseX[4];
    GuiaEje::colorEje(GuiaEje::kEjeX, baseX);

    // Referencia que ya contrasta con el eje: el color base se respeta tal cual.
    const float oscura[3] = {0.0f, 0.0f, 0.0f};
    float sobreOscura[4];
    GuiaEje::colorEfectivo(GuiaEje::kEjeX, oscura, sobreOscura);
    CHECK(cerca(sobreOscura[0], baseX[0]) && cerca(sobreOscura[1], baseX[1]) &&
              cerca(sobreOscura[2], baseX[2]),
          "con una grilla que ya contrasta, el eje conserva su color base");
    CHECK(sobreOscura[3] == 1.0f, "el color ajustado sigue siendo opaco");

    // Referencia con la MISMA luminancia que el eje (grilla del mismo color):
    // sin ajuste la guia se pierde, con ajuste tiene que alejarse.
    const float misma[3] = {baseX[0], baseX[1], baseX[2]};
    const float lBase = AparienciaUtil::luminancia(baseX[0], baseX[1], baseX[2]);
    float ajustada[4];
    GuiaEje::colorEfectivo(GuiaEje::kEjeX, misma, ajustada);
    const float lAjustada =
        AparienciaUtil::luminancia(ajustada[0], ajustada[1], ajustada[2]);
    CHECK(std::fabs(lAjustada - lBase) > kEps,
          "con una grilla del mismo color el eje se aleja de ella");
    CHECK(lAjustada > lBase,
          "el ajuste aleja el eje hacia el lado mas claro que la grilla");
    CHECK(ajustada[0] > ajustada[1] && ajustada[0] > ajustada[2],
          "el ajuste no cambia de eje: X sigue siendo roja");

    // Los tres ejes contra la misma referencia: cada uno conserva su convencion
    // (X roja, Y verde, Z azul), que es la del gizmo y la de la grilla.
    const int ejes[3] = {GuiaEje::kEjeX, GuiaEje::kEjeY, GuiaEje::kEjeZ};
    const int dominante[3] = {0, 1, 2};
    for (int i = 0; i < 3; ++i) {
        float color[4];
        GuiaEje::colorEfectivo(ejes[i], misma, color);
        CHECK(color[dominante[i]] >= color[(dominante[i] + 1) % 3] &&
                  color[dominante[i]] >= color[(dominante[i] + 2) % 3],
              "el ajuste por contraste conserva la convencion de cada eje");
    }

    // Sin referencia no hay contraste que aplicar: el eje conserva su color
    // base (y no se lee memoria).
    float sinReferencia[4] = {-1.0f, -1.0f, -1.0f, -1.0f};
    GuiaEje::colorEfectivo(GuiaEje::kEjeY, nullptr, sinReferencia);
    float baseY[4];
    GuiaEje::colorEje(GuiaEje::kEjeY, baseY);
    CHECK(cerca(sinReferencia[0], baseY[0]) && cerca(sinReferencia[1], baseY[1]) &&
              cerca(sinReferencia[2], baseY[2]),
          "sin color de referencia el eje conserva su color base");
    CHECK(sinReferencia[3] == 1.0f,
          "sin color de referencia el eje sigue siendo opaco");
}

// El radio de difuminado es un valor de configuracion del usuario (arriba y
// abajo se lo elige en Opciones) y de el sale TODO: el tramo opaco, el
// horizonte que hace de limite de dibujado y la caida en el medio. Grilla y
// guia de eje parten del MISMO radio, asi que un cambio se ve en los dos a la
// vez; el inicio del difuminado se deriva del radio en proporcion constante
// (la misma que el radio por defecto), de modo que agrandar el circulo agranda
// el degradado en vez de estirarlo.
void testDifuminadoDesdeRadio() {
    const Difuminado porDefecto =
        Difuminado::desdeRadio(AparienciaUtil::kRadioDifuminadoPorDefecto, 6);
    CHECK(cerca(porDefecto.fin, 150.0f),
          "el radio por defecto deja el horizonte en 150");
    CHECK(cerca(porDefecto.inicio, 40.0f),
          "el inicio se deriva del radio con la proporcion historica");
    CHECK(cerca(porDefecto.inicio,
                porDefecto.fin * Difuminado::kProporcionInicio),
          "el inicio es siempre la misma fraccion del radio");
    CHECK(porDefecto.subdivisiones == 6,
          "las subdivisiones las elige el llamador (grilla o guia)");

    const Difuminado amplio = Difuminado::desdeRadio(600.0f, 24);
    CHECK(cerca(amplio.fin, 600.0f), "un radio mayor aleja el horizonte");
    CHECK(cerca(amplio.inicio, 160.0f),
          "un radio mayor Tambien aleja el inicio del difuminado");
    CHECK(amplio.subdivisiones == 24,
          "un radio con mas subdivisiones no se pierde al derivarlo");

    const Difuminado chico = Difuminado::desdeRadio(20.0f, 6);
    CHECK(cerca(chico.fin, 20.0f), "un radio menor acerca el horizonte");
    CHECK(chico.inicio < chico.fin, "con radio pequeno el tramo opaco es corto");
    CHECK(chico.inicio > 0.0f, "el inicio nunca se sale del circulo");
}

// El radio se acota en el mismo lugar donde se deriva, para que un valor
// corrupto en la configuracion (o un radio negativo) no pueda dejar la grilla o
// la guia sin horizonte o con una division por cero en la curva.
void testDifuminadoAcotaElRadio() {
    CHECK(cerca(Difuminado::desdeRadio(1e6f, 6).fin,
                AparienciaUtil::kRadioDifuminadoMaximo),
          "un radio enorme se acota al maximo");
    CHECK(cerca(Difuminado::desdeRadio(-50.0f, 6).fin,
                AparienciaUtil::kRadioDifuminadoMinimo),
          "un radio negativo se acota al minimo");
    CHECK(cerca(Difuminado::desdeRadio(0.0f, 6).fin,
                AparienciaUtil::kRadioDifuminadoMinimo),
          "un radio nulo se acota al minimo");

    const Difuminado acotado = Difuminado::desdeRadio(1e6f, 6);
    CHECK(acotado.fin > acotado.inicio,
          "radio acotado: el horizonte sigue dejando tramo opaco");
    CHECK(std::isfinite(Difuminado::desdeRadio(NAN, 6).fin) &&
              std::isfinite(Difuminado::desdeRadio(NAN, 6).inicio),
          "un radio NaN no deja un horizonte invalido");
    CHECK(std::isfinite(acotado.opacidad(0.0f)) &&
              std::isfinite(acotado.opacidad(acotado.fin)) &&
              std::isfinite(acotado.opacidad(acotado.fin * 4.0f)),
          "la curva da numeros finitos en todo el rango");
}

// La curva es la misma que usaba la grilla y la guia por separado: opaca en el
// centro, caida cuadratica y exactamente 0 en el horizonte (y antes de cero,
// nunca negativa).
void testDifuminadoOpacidad() {
    const Difuminado dif = Difuminado::desdeRadio(150.0f, 6);
    CHECK(cerca(dif.opacidad(0.0f), 1.0f), "opaca en la camara");
    CHECK(cerca(dif.opacidad(dif.inicio), 1.0f),
          "opaca hasta el inicio del difuminado");
    CHECK(cerca(dif.opacidad(dif.fin), 0.0f), "invisible en el horizonte");
    CHECK(cerca(dif.opacidad(dif.fin * 5.0f), 0.0f),
          "invisible mas alla del horizonte (no se hace negativa)");
    const float mitad = dif.opacidad((dif.inicio + dif.fin) * 0.5f);
    CHECK(mitad < 1.0f && mitad > 0.0f, "a mitad de camino esta a medio opaca");
    CHECK(cerca(mitad, 0.75f),
          "la caida es cuadratica: a la mitad del tramo queda 3/4 de opacidad");
    // Un radio distinto no cambia la FORMA de la curva, solo la escala.
    const Difuminado otro = Difuminado::desdeRadio(300.0f, 6);
    CHECK(cerca(otro.opacidad((otro.inicio + otro.fin) * 0.5f), mitad),
          "la forma de la curva no depende del radio");
}

void testShaderLineasRespetaHorizonteRadial() {
    const std::string vertex = kLineVertexShader;
    const std::string fragment = kLineFragmentShader;
    CHECK(vertex.find("out vec3 vWorldPosition") != std::string::npos &&
              vertex.find("vWorldPosition = mix(worldS, worldE, aAlong)") !=
                  std::string::npos,
          "el shader de lineas entrega la posicion mundial de cada fragmento");
    CHECK(fragment.find("length(vWorldPosition - uCameraPosition)") !=
                  std::string::npos &&
              fragment.find("distanciaCamara >= uDistanceFadeEnd") !=
                  std::string::npos &&
              fragment.find("discard") != std::string::npos,
          "el shader descarta las lineas fuera del radio de horizonte");
    CHECK(fragment.find("uDistanceFadeStart") != std::string::npos &&
              fragment.find("1.0 - t * t") != std::string::npos,
          "el shader difumina las lineas hasta el borde radial");
}

// Cielo: los colores efectivos resuelven el modo B/N y el tema, devolviendo
// los dos extremos del degradado (superior e inferior).
void testCieloColoresEfectivos() {
    Apariencia ap;
    // Default: gris oscuro en ambas partes.
    float sup[3], inf[3];
    Cielo::coloresEfectivos(ap, sup, inf);
    CHECK(cerca(sup[0], 0.10f) && cerca(sup[1], 0.10f) && cerca(sup[2], 0.10f),
          "default: superior gris oscuro");
    CHECK(cerca(inf[0], 0.10f) && cerca(inf[1], 0.10f) && cerca(inf[2], 0.10f),
          "default: inferior gris oscuro");

    // Modo B/N con tema oscuro -> ambas negro.
    ap.blancoYNegro = true;
    ap.temaClaro = false;
    Cielo::coloresEfectivos(ap, sup, inf);
    CHECK(cerca(sup[0], 0.0f) && cerca(sup[1], 0.0f) && cerca(sup[2], 0.0f),
          "B/N oscuro: superior negro");
    CHECK(cerca(inf[0], 0.0f) && cerca(inf[1], 0.0f) && cerca(inf[2], 0.0f),
          "B/N oscuro: inferior negro");

    // Modo B/N con tema claro -> ambas blanco.
    ap.temaClaro = true;
    Cielo::coloresEfectivos(ap, sup, inf);
    CHECK(cerca(sup[0], 1.0f) && cerca(sup[1], 1.0f) && cerca(sup[2], 1.0f),
          "B/N claro: superior blanco");
    CHECK(cerca(inf[0], 1.0f) && cerca(inf[1], 1.0f) && cerca(inf[2], 1.0f),
          "B/N claro: inferior blanco");

    // Modo normal con colores personalizados.
    ap.blancoYNegro = false;
    ap.fondoSuperior[0] = 0.2f; ap.fondoSuperior[1] = 0.3f; ap.fondoSuperior[2] = 0.8f;
    ap.fondoInferior[0] = 0.8f; ap.fondoInferior[1] = 0.4f; ap.fondoInferior[2] = 0.2f;
    Cielo::coloresEfectivos(ap, sup, inf);
    CHECK(cerca(sup[0], 0.2f) && cerca(sup[1], 0.3f) && cerca(sup[2], 0.8f),
          "normal: superior personalizado");
    CHECK(cerca(inf[0], 0.8f) && cerca(inf[1], 0.4f) && cerca(inf[2], 0.2f),
          "normal: inferior personalizado");
}

// CacheCubemap: la clave que decide cuando hay que volver a subir el cubemap a
// GPU cambia exactamente cuando cambia algo que lo altera (una ruta o su fecha
// de modificacion), y solo entonces.
void testCacheCubemapClave() {
    const std::string rutas[6] = {"+x.png", "-x.png", "+y.png",
                                  "-y.png", "+z.png", "-z.png"};
    const std::int64_t mtimes[6] = {10, 20, 30, 40, 50, 60};
    const std::string base = CacheCubemap::claveDeCaras(rutas, mtimes);

    CHECK(!base.empty(), "clave no vacia");
    CHECK(base == CacheCubemap::claveDeCaras(rutas, mtimes),
          "mismas caras y mismo mtime -> misma clave (se reusa la textura)");

    std::int64_t mtimesOtraCara[6] = {10, 20, 31, 40, 50, 60};
    CHECK(CacheCubemap::claveDeCaras(rutas, mtimesOtraCara) != base,
          "una cara reescrita cambia la clave (se vuelve a subir)");

    std::int64_t mtimesAusentes[6] = {0, 0, 0, 0, 0, 0};
    CHECK(CacheCubemap::claveDeCaras(rutas, mtimesAusentes) != base,
          "caras inexistentes dan otra clave (mtime 0 != mtime real)");

    std::string rutasOtra[6] = {"+x.png", "-x.png", "+y.png",
                                "-y.png", "+z.png", "otro.png"};
    CHECK(CacheCubemap::claveDeCaras(rutasOtra, mtimes) != base,
          "ruta distinta en una cara cambia la clave");

    std::string rutasIntercambiadas[6] = {"-x.png", "+x.png", "+y.png",
                                          "-y.png", "+z.png", "-z.png"};
    CHECK(CacheCubemap::claveDeCaras(rutasIntercambiadas, mtimes) != base,
          "el orden de las caras importa (+X y -X intercambiadas no emparejan)");
}

// Quad compartido de los sprites 2D: el renderer lo sube a GPU una sola vez y
// el recorte de la region (uUvRect) y el tamano son por componente, asi que la
// geometria tiene que ser la unitaria centrada en el origen y con UVs 0..1.
void testPrimitivaQuad2D() {
    const Mesh quad = Primitivas2D::quad();
    CHECK(quad.vertices.size() == 4, "el quad tiene 4 vertices");
    CHECK(quad.uvs.size() == 4, "el quad tiene una UV por vertice");
    CHECK(quad.indices.size() == 6, "el quad tiene 6 indices (dos triangulos)");
    CHECK(!quad.isEmpty(), "el quad no esta vacio");
    CHECK(quad.hasUvs(), "el quad tiene UVs por vertice (necesarias para texturar)");

    bool centrado = true;
    bool planoZ = true;
    for (const vec3& v : quad.vertices) {
        if (v.x < -0.5f || v.x > 0.5f || v.y < -0.5f || v.y > 0.5f)
            centrado = false;
        if (v.z != 0.0f) planoZ = false;
    }
    CHECK(centrado, "los vertices van de -0.5 a 0.5 (quad 1x1 centrado)");
    CHECK(planoZ, "el quad esta en el plano Z=0");

    bool uvValidas = true;
    for (const vec2& uv : quad.uvs) {
        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
            uvValidas = false;
    }
    CHECK(uvValidas, "las UVs cubren [0,1]");

    bool indicesValidos = true;
    for (unsigned int i : quad.indices) {
        if (i >= quad.vertices.size()) indicesValidos = false;
    }
    CHECK(indicesValidos, "los indices referencian vertices existentes");
}
