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
// Pruebas headless del cache de texturas (TextureManager): imagen compartida,
// decode unico por path, fallos y eviccion, con un loader artificial. Sin la
// pila grafica (el upload a GPU TextureGL no vive aqui) ni stb_image. Solo std
// C++17.

#include <iostream>
#include <memory>
#include <string>

#include "../FunshiEngineGL/src/Assets/Image.h"
#include "../FunshiEngineGL/src/Assets/StbImageLoader.h"
#include "../FunshiEngineGL/src/Assets/TextureException.h"
#include "../FunshiEngineGL/src/Assets/TextureManager.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Imagen 2x2 RGBA con un patron: util para validar pixels compartidos.
std::shared_ptr<Image> imagenPatron() {
    auto image = std::make_shared<Image>();
    image->width = 2;
    image->height = 2;
    image->pixels = {255, 0,   0, 255, 0,   255, 0,   255,
                     0,   0,   255, 255, 255, 255, 255, 255}; // 4 px RGBA
    return image;
}

// Loader artificial: crea imagenes cuyo tamano codifica el path, cuenta cargas
// y falla para el path marcado como corrupto.
class LoaderStub : public ITextureLoader {
public:
    int cargas = 0;
    std::shared_ptr<Image> load(const std::string& path) override {
        ++cargas;
        // Conviene el mismo idioma de AssetManagerTests: cualquier path que
        // termine en ".corrupto" falla como si el archivo no decodificara.
        if (path.size() >= 9 && path.compare(path.size() - 9, 9, ".corrupto") == 0)
            throw TextureLoadException(path, "marcado como corrupto");
        auto image = std::make_shared<Image>();
        image->width = static_cast<int>(path.size());
        image->height = 1;
        image->pixels = {1, 2, 3, 4};
        return image;
    }
};

void testTextureManager() {
    TextureManager sinLoader;
    CHECK(sinLoader.textureCount() == 0, "manager nuevo vacio");

    // Sin loader, pedir una imagen ausente debe lanzar.
    try {
        sinLoader.getTexture("texturas/muro.png");
        CHECK(false,
              "getTexture sin loader deberia lanzar TextureNotFoundException");
    } catch (const TextureNotFoundException& e) {
        CHECK(std::string(e.what()).find("Textura") != std::string::npos,
              "mensaje de TextureNotFoundException");
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para imagen ausente");
    }

    auto loader = std::make_unique<LoaderStub>();
    LoaderStub* loaderRaw = loader.get();
    TextureManager manager(std::move(loader));

    // Carga y cache: el MISMO path solo se decodifica una vez.
    auto a = manager.getTexture("Texturas/Muro.png");
    auto b = manager.getTexture("Texturas/Muro.png");
    CHECK(a.get() == b.get(),
          "getTexture con el mismo path devuelve la misma imagen");
    CHECK(loaderRaw->cargas == 1, "la imagen se decodifico una sola vez");
    CHECK(manager.textureCount() == 1, "una entrada en el cache");

    // Claves normalizadas: distintos separadores -> mismo recurso.
    auto c = manager.getTexture("Texturas\\Muro.png");
    CHECK(c.get() == a.get(), "separadores distintos resuelven al mismo asset");
    CHECK(loaderRaw->cargas == 1, "sin recarga por solo cambiar separadores");

    // Paths diferentes -> imagenes diferentes.
    auto d = manager.getTexture("Texturas/Piso.png");
    CHECK(d.get() != a.get(), "paths distintos no comparten imagen");
    CHECK(manager.textureCount() == 2, "una entrada por path distinto");

    // Loader que falla: la excepcion debe propagarse y NO quedar cacheada.
    try {
        manager.getTexture("Texturas/Rota.png.corrupto");
        CHECK(false, "getTexture con loader fallido deberia lanzar");
    } catch (const TextureLoadException&) {
        // esperado
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para decode fallido");
    }
    CHECK(!manager.containsTexture("Texturas/Rota.png.corrupto"),
          "una imagen fallida no se cachea");

    // putTexture/contains/remove.
    manager.putTexture("Procedural/Patron.png", imagenPatron());
    CHECK(manager.containsTexture("Procedural/Patron.png"),
          "putTexture registra");
    CHECK(manager.textureCount() == 3, "una entrada mas tras putTexture");
    try {
        manager.putTexture("vacio.png", std::make_shared<Image>());
        CHECK(false, "putTexture con imagen vacia deberia lanzar");
    } catch (const TextureLoadException&) {
        // esperado
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para imagen vacia");
    }
    manager.removeTexture("Procedural/Patron.png");
    CHECK(!manager.containsTexture("Procedural/Patron.png"),
          "removeTexture elimina");

    // clearUnusedTextures: evicta solo lo que nadie usa (use_count == 1).
    // Suelta primero los holders del bloque de cache para dejarlas sin usuarios.
    a.reset();
    b.reset();
    c.reset();
    d.reset();
    {
        auto retenido = manager.getTexture("Texturas/Muro.png"); // use_count 2
        manager.clearUnusedTextures();
        CHECK(manager.containsTexture("Texturas/Muro.png"),
              "imagen retenida sobrevive a clearUnusedTextures");
    }
    manager.clearUnusedTextures();
    CHECK(!manager.containsTexture("Texturas/Muro.png"),
          "imagen sin usuarios externos se evicta");
}

} // namespace

// El volteo vertical del loader de imagenes invierte las filas: es lo que
// espera OpenGL (la primera fila abajo) y se hace sobre la imagen decodificada
// para no depender del estado global de la libreria.
void testVolteoVertical() {
    Image imagen;
    imagen.width = 2;
    imagen.height = 2;
    imagen.channels = 4;
    imagen.pixels = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

    StbImageLoader::voltearVertical(imagen);

    CHECK(imagen.pixels[0] == 9 && imagen.pixels[1] == 10 &&
              imagen.pixels[2] == 11 && imagen.pixels[3] == 12,
          "tras el volteo la primera fila es la ultima de la imagen");
    CHECK(imagen.pixels[8] == 1 && imagen.pixels[9] == 2 &&
              imagen.pixels[10] == 3 && imagen.pixels[11] == 4,
          "tras el volteo la ultima fila es la primera de la imagen");
    CHECK(imagen.pixels[4] == 13 && imagen.pixels[5] == 14 &&
              imagen.pixels[6] == 15 && imagen.pixels[7] == 16,
          "el volteo no altera el orden de los pixeles dentro de una fila");
    CHECK(imagen.pixels.size() == 16, "el volteo no cambia el tamano de la imagen");
}

int main() {
    testTextureManager();
    testVolteoVertical();

    std::cout << "Resultado: " << (total - fallos) << "/" << total << " OK"
              << std::endl;
    if (fallos > 0) {
        std::cout << fallos << " prueba(s) fallaron." << std::endl;
        return 1;
    }
    return 0;
}