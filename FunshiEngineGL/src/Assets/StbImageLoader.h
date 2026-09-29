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
#ifndef STBIMAGELOADER_H
#define STBIMAGELOADER_H

#include "TextureManager.h"

// Loader de imagenes con stb_image (vendoriado en Herramientas/IconosGUI).
// Decodifica a RGBA (4 canales, estandar para el upload a GPU) y vueltea
// verticalmente: OpenGL espera la primera fila abajo y los decoders comunes
// (PNG/JPG) entregan la primera fila arriba.
class StbImageLoader : public ITextureLoader {
public:
    // Lanza TextureLoadException si el archivo no existe o no decodifica.
    std::shared_ptr<Image> load(const std::string& path) override;

    // Dimensiones en pixeles sin decodificar la imagen: lee solo la cabecera
    // del archivo. Devuelve false si el archivo no existe o si su formato no se
    // puede sondear. A diferencia de load, no lanza excepcion.
    //
    // Existe para preguntar "¿miden todas las caras del cubemap lo mismo?"
    // ANTES de subirlas a la GPU. El criterio es el mismo que usa el render al
    // decodificar, asi que lo que el panel avisa y lo que el render acepta
    // coinciden; y sale mucho mas barato que decodificar las imagenes enteras
    // para tirar los pixeles que ya estan en disco. Comparar el peso del
    // archivo en bytes no sirve: dos imagenes identicas con distinta compresion
    // pesan distinto sin que midan distinto.
    static bool dimensiones(const std::string& path, int& ancho, int& alto);
};

#endif