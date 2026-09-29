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
#ifndef SELECTORARCHIVOCUBEMAP_H
#define SELECTORARCHIVOCUBEMAP_H

// Selector de archivo para las caras del cubemap del componente Skybox.
//
// Escribir la ruta de las seis caras a mano obliga al usuario a conocer la raiz
// de assets y el formato de las rutas, y el error no se ve: si la ruta no
// resuelve, el motor cae al degradado sin decir nada en la pantalla. Este modal
// deja elegir el archivo navegando, con las carpetas primero y filtrando por
// las extensiones que el motor de imagenes decodifica.
//
// La logica que decide (que extensiones valen, cuantas caras faltan, si dos
// caras miden distinto) queda en funciones `inline` puras FUERA de la clase del
// modal: asi se ejercita en los targets headless, que no crean contexto de
// ImGui ni pila grafica. Mismo criterio que SoltarEnCarpeta y RenombrarElemento
// en el modulo del explorador.
//
// Header-only a proposito: son pocas lineas y asi el helper no obliga a tocar
// la lista de fuentes de CMakeLists.txt.

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "imgui.h"

#include "../../../Assets/AssetPath.h"
#include "../../../Configuracion/EditorConfig.h"
#include "../../../FileManager/FileManager.h"

namespace SelectorArchivoCubemap {

// Orden de las caras, el mismo que usa la pasada que dibuja el cubemap.
constexpr int kCaras = 6;

// Extensiones que el motor de imagenes puede decodificar como cara. Se listan
// las que anuncia el inspector mas `.jpeg`, que es la misma imagen que `.jpg`.
inline bool esImagenCubemap(const std::string& ruta) {
    // Sin punto delante: AssetPath::hasExtension compara contra la extension
    // tal cual aparece despues del ultimo punto.
    static const char* kAceptadas[] = {"png",  "jpg", "jpeg", "tga",
                                       "bmp",  "psd", "hdr"};
    for (const char* ext : kAceptadas) {
        if (AssetPath::hasExtension(ruta, ext)) return true;
    }
    return false;
}

// Cuantas caras quedan sin asignar. Con seis asignadas y de igual tamano el
// cubemap se dibuja; con cualquiera de las dos condiciones faltando, el motor
// avisa una vez por el log y cae al degradado.
inline int carasSinAsignar(const std::string rutas[kCaras]) {
    int sinAsignar = 0;
    for (int i = 0; i < kCaras; ++i) {
        if (rutas[i].empty()) ++sinAsignar;
    }
    return sinAsignar;
}

// Dimensiones en pixeles de una cara. `ancho`/`alto` en 0 significan que el
// archivo no se pudo sondear (no existe o su formato no se pudo leer), en cuyo
// caso la cara queda fuera de la comparacion.
struct Dimensiones {
    int ancho = 0;
    int alto = 0;
};

// Devuelve el indice de la primera cara que no mide lo mismo que la +X, o -1 si
// todas coinciden (o si alguna no se pudo medir, que ya se informa por
// `carasSinAsignar`).
//
// El criterio es el MISMO que aplica el render al decodificar: compara los
// pixeles de cada cara contra los de la +X y descarta el cubemap entero si
// alguna difiere. Medir por peso de archivo en su lugar daria falsos positivos
// (dos imagenes identicas con distinta compresion pesan distinto sin que midan
// distinto) y mandaria a revisar algo que ya esta bien.
//
// Es pura a proposito: no toca disco. Quien la llama sondea los archivos con
// `StbImageLoader::dimensiones`, asi los targets headless la ejercitan sin
// enlazar stb_image.
inline int caraConDimensionDistinta(const Dimensiones d[kCaras]) {
    if (d[0].ancho <= 0 || d[0].alto <= 0) return -1;
    for (int i = 1; i < kCaras; ++i) {
        if (d[i].ancho <= 0 || d[i].alto <= 0) continue;
        if (d[i].ancho != d[0].ancho || d[i].alto != d[0].alto) return i;
    }
    return -1;
}

// Lo confirmado por el modal: la ruta absoluta del archivo elegido, o vacio si
// se cerro sin confirmar.
struct Resultado {
    bool confirmado = false;
    std::string ruta;
};

// Carpeta de la que parte el selector. Si la cara ya tiene ruta, se abre su
// propia carpeta (elegir la cara vecina es el caso normal); si esta vacia, la
// raiz de assets del proyecto; si tampoco, el directorio actual.
inline std::string carpetaInicial(const std::string rutaActual) {
    if (!rutaActual.empty()) {
        const std::filesystem::path padre =
            std::filesystem::path(rutaActual).parent_path();
        if (!padre.empty()) return padre.string();
    }
    const std::string& raiz = EditorConfig::raizAssetsFijada();
    if (!raiz.empty()) return raiz;
    return ".";
}

// Estado + dibujo del modal de seleccion. El panel lo abre desde el boton de la
// cara y lo dibuja junto a sus widgets; el resultado viene confirmado SOLO en
// el frame de la confirmacion.
class Modal {
public:
    // Registra la carpeta de la cara que se esta editando y cual es esa cara,
    // para que quien llama sepa a que campo aplicar el resultado. Se abre en el
    // dibujar() siguiente.
    void solicitar(const std::string& rutaActual, int cara) {
        carpeta_ = carpetaInicial(rutaActual);
        caraDestino_ = cara;
        elegida_.clear();
        filtro_.clear();
        abrir_ = true;
    }

    bool activo() const noexcept { return !carpeta_.empty(); }

    // Indice de la cara que se esta editando. Solo tiene sentido mientras el
    // modal esta activo.
    int caraDestino() const noexcept { return caraDestino_; }

    Resultado dibujar() {
        Resultado resultado;
        if (carpeta_.empty()) return resultado;
        if (abrir_) {
            ImGui::OpenPopup("Seleccionar cara del cubemap");
            abrir_ = false;
        }
        if (!ImGui::BeginPopupModal("Seleccionar cara del cubemap", nullptr,
                                    ImGuiWindowFlags_AlwaysAutoResize)) {
            // No hay popup que editar (lo cerro el usuario): se descarta el
            // pedido en vez de dejarlo colgado esperando un dibujar() que ya no
            // puede volver a abrirlo.
            carpeta_.clear();
            return resultado;
        }

        dibujarRuta();
        ImGui::Separator();
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        char filtro[128];
        std::snprintf(filtro, sizeof(filtro), "%s", filtro_.c_str());
        if (ImGui::InputText("##filtroCubemap", filtro, sizeof(filtro))) {
            filtro_ = filtro;
        }

        if (!dibujarListado()) {
            ImGui::TextDisabled("No hay imagenes en esta carpeta.");
        }

        // El doble clic en un archivo confirma: el listado marca el pedido y el
        // cierre se resuelve aca, que es el unico sitio con el popup abierto.
        const bool porDobleClic = confirmarPorDobleClic_;
        confirmarPorDobleClic_ = false;

        const bool elegir =
            porDobleClic || (!elegida_.empty() &&
                             ImGui::Button("Elegir", ImVec2(120, 0)));
        const bool cancelar =
            !porDobleClic &&
            (ImGui::Button("Cancelar", ImVec2(120, 0)) ||
             ImGui::IsKeyPressed(ImGuiKey_Escape));
        if (!elegir) ImGui::SameLine();

        if (elegir) {
            resultado.confirmado = true;
            resultado.ruta = elegida_;
        }
        if (elegir || cancelar) {
            carpeta_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
        return resultado;
    }

private:
    // Encabezado con la carpeta actual y un boton para subir un nivel. Se
    // escribe como texto no editable: no hace falta una ruta escribible a mano
    // para este caso de uso.
    void dibujarRuta() {
        ImGui::TextUnformatted(carpeta_.c_str());
        if (std::filesystem::path(carpeta_).has_parent_path()) {
            if (ImGui::SmallButton("Arriba")) {
                elegida_.clear();
                carpeta_ =
                    std::filesystem::path(carpeta_).parent_path().string();
            }
        }
    }

    // Filas del contenido: carpetas primero y despues las imagenes que acepta
    // el cubemap, filtradas por el texto de busqueda. Devuelve si habia algo
    // que mostrar.
    bool dibujarListado() {
        std::vector<FileManager::EntradaDirectorio> entradas;
        if (!FileManager::listarDirectorio(carpeta_, entradas)) return false;

        // Carpetas primero y, dentro de cada grupo, por nombre: el orden de
        // disco no es estable y una lista que salta entreFrames hace fallar los
        // clics.
        std::sort(entradas.begin(), entradas.end(),
                  [](const FileManager::EntradaDirectorio& a,
                     const FileManager::EntradaDirectorio& b) {
                      if (a.esCarpeta != b.esCarpeta) return a.esCarpeta;
                      return a.nombre < b.nombre;
                  });

        bool hay = false;
        const float altoFila =
            ImGui::GetTextLineHeightWithSpacing();
        ImGui::BeginChild("##listaCubemap", ImVec2(380, 240 * altoFila /
                                                            16.0f),
                          ImGuiChildFlags_Borders);
        for (const auto& entrada : entradas) {
            if (!entrada.esCarpeta && !esImagenCubemap(entrada.nombre)) continue;
            if (!filtro_.empty() &&
                AssetPath::lowercase(entrada.nombre).find(
                    AssetPath::lowercase(filtro_)) == std::string::npos) {
                continue;
            }
            hay = true;
            const bool esLaElegida = (entrada.ruta == elegida_);
            if (ImGui::Selectable(entrada.nombre.c_str(), esLaElegida, 0,
                                  ImVec2(0, 0))) {
                elegida_ = entrada.ruta;
            }
            if (ImGui::IsItemHovered() &&
                ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                // Doble clic en una carpeta: se entra. En un archivo: se elige
                // y se pide cerrar, que es lo que espera cualquiera que ya
                // conoce el explorador.
                if (entrada.esCarpeta) {
                    elegida_.clear();
                    carpeta_ = entrada.ruta;
                } else {
                    elegida_ = entrada.ruta;
                    confirmarPorDobleClic_ = true;
                }
            }
        }
        ImGui::EndChild();
        return hay;
    }

    std::string carpeta_;
    std::string elegida_;
    std::string filtro_;
    int caraDestino_ = 0;
    bool confirmarPorDobleClic_ = false;
    bool abrir_ = false;
};

}  // namespace SelectorArchivoCubemap

#endif  // SELECTORARCHIVOCUBEMAP_H
