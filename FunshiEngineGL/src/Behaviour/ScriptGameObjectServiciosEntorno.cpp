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
#include "ScriptGameObjectDetalle.h"

namespace MotorScript {

int serviciosReproducirSonido(const char* ruta, float volumen, bool bucle) {
    if (!g_audio || !ruta) return -1;
    // La ruta se resuelve contra la raiz de assets del proyecto (src<nombre>):
    // el script pide "sonidos/disparo.wav" sin saber donde esta el proyecto.
    // Una ruta absoluta se deja tal cual.
    const std::string archivo = EditorConfig::absolutizarRuta(ruta);
    const int handle = g_audio->reproducir(archivo, volumen, bucle);
    g_sonidosScript.registrar(handle);
    return handle;
}

void serviciosDetenerSonido(int handle) {
    if (g_audio) g_audio->detener(handle);
    g_sonidosScript.retirar(handle);
}

bool serviciosReproducirMusica(const char* ruta, float volumen, float fundido) {
    if (!g_audio || !ruta) return false;
    // La ruta se resuelve contra la raiz de assets del proyecto (src<nombre>),
    // igual que los efectos de sonido.
    const std::string archivo = EditorConfig::absolutizarRuta(ruta);
    return g_audio->reproducirMusica(archivo, volumen, fundido);
}

void serviciosDetenerMusica(float fundido) {
    if (g_audio) g_audio->detenerMusica(fundido);
}

void serviciosVolumenMusica(float volumen) {
    if (g_audio) g_audio->setVolumenMusica(volumen);
}

void* serviciosObjetoPorNombre(const char* nombre) {
    if (!g_escena || !nombre) return nullptr;
    // Recorrido en preorden de la lista lineal del registro (ya ordenada por
    // jerarquia tras cada refreshTransformOrigins). next(last) lanza
    // (no devuelve null), asi que se avanza con el idiom seguro de ListaDE.
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        // Comparacion por texto: ambos son const char* y el == compararia
        // punteros (el buffer inputName nunca es el literal buscado).
        if (o && std::strcmp(nombreDelObjeto(o), nombre) == 0) return o;
    }
    return nullptr;
}

void* serviciosObjetoPorId(int id) {
    if (!g_escena) return nullptr;
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        if (o && o->getId() == id) return o;
    }
    return nullptr;
}

void* serviciosObjetoPorEtiqueta(const char* etiqueta) {
    if (!g_escena || !etiqueta) return nullptr;
    auto* lista = g_escena->getGameObjects();
    if (!lista || lista->isEmpty()) return nullptr;
    for (auto* nodo = lista->first(); nodo;
         nodo = (nodo != lista->last()) ? lista->next(nodo) : nullptr) {
        GameObject* o = nodo->getElement();
        if (o && o->getTag() == etiqueta) return o;
    }
    return nullptr;
}

bool serviciosTeclaSostiene(const char* tecla) {
    return g_input ? g_input->sostiene(tecla) : false;
}

bool serviciosTeclaPresionada(const char* tecla) {
    return g_input ? g_input->presionada(tecla) : false;
}

bool serviciosTeclaSoltada(const char* tecla) {
    return g_input ? g_input->soltada(tecla) : false;
}

float serviciosDeltaMouseX() {
    return g_input ? g_input->deltaMouseX() : 0.0f;
}

float serviciosDeltaMouseY() {
    return g_input ? g_input->deltaMouseY() : 0.0f;
}

void serviciosFijarGravedadGlobal(float x, float y, float z) {
    if (g_fisica) g_fisica->fijarGravedad(x, y, z);
}

float serviciosGravedadGlobalEje(int eje) {
    float x = 0.0f;
    float y = -1.0f;
    float z = 0.0f;
    if (g_fisica) g_fisica->gravedad(x, y, z);
    if (eje == 0) return x;
    if (eje == 2) return z;
    return y;
}

}  // namespace MotorScript
