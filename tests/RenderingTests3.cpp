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

// Shader de sprites: los atributos tienen que coincidir con el layout fijo del
// VAO de createMesh (posicion en 0, UV en 2) y el fragment tiene que aplicar
// tinte, textura opcional y descarte por alpha.
void testShaderSprites() {
    const std::string vertex = kSpriteVertexShader;
    const std::string fragment = kSpriteFragmentShader;
    CHECK(vertex.find("layout(location = 0) in vec3 aPosition") !=
                  std::string::npos &&
              vertex.find("layout(location = 2) in vec2 aUv") !=
                  std::string::npos,
          "el shader de sprites lee posicion (0) y UV (2) del VAO de createMesh");
    CHECK(vertex.find("uUvRect") != std::string::npos &&
              vertex.find("mix(uUvRect.xy, uUvRect.zw, aUv)") !=
                  std::string::npos,
          "el shader recorta la region de la textura por vertice");
    CHECK(fragment.find("uUseTexture") != std::string::npos &&
              fragment.find("texture(uTexture, vUv)") != std::string::npos,
          "el fragment multiplica el texel cuando el sprite tiene textura");
    CHECK(fragment.find("uTint") != std::string::npos &&
              fragment.find("discard") != std::string::npos,
          "el fragment aplica el tinte y descarta los fragmentos transparentes");
}

int main() {
    testConstantes();
    testVacio();
    testSegmento();
    testColorPorExtremo();
    testPolilinea();
    testAristas();
    testCaja();
    testAcumulacion();
    testGuiaGlobal();
    testGuiaLocal();
    testGuiaIgnoraEscala();
    testOpacidad();
    testEmiteHastaElHorizonte();
    testDifuminadoPorVertice();
    testGuiaDefensiva();
    testEmiteDefensiva();
    testGuiaColor();
    testGuiaColorEfectivo();
    testDifuminadoDesdeRadio();
    testDifuminadoAcotaElRadio();
    testDifuminadoOpacidad();
    testShaderLineasRespetaHorizonteRadial();
    testCieloColoresEfectivos();
    testCacheCubemapClave();
    testPrimitivaQuad2D();
    testShaderSprites();

    std::cout << "Resultado: " << (total - fallos) << "/" << total
              << " OK" << std::endl;
    if (fallos > 0) {
        std::cout << fallos << " prueba(s) fallaron." << std::endl;
        return 1;
    }
    return 0;
}
