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
#ifndef GESTORMUSICA_H
#define GESTORMUSICA_H

#include <string>
#include <vector>

#include "IMusicaBackend.h"

// Reproductor de musica: que pista suena, cual es la siguiente, cuando hay
// que fundir una con otra y que volumen tiene el bus.
//
// Las pistas se referencian por RUTA (la del archivo), igual que los efectos
// de sonido: no hay catalogo por nombre ni carpeta fija, el archivo puede
// estar en cualquier sitio dentro del proyecto. La LISTA de pistas es ordenada
// y es la que se sigue al pasar a la pista siguiente.
//
// No sabe nada de hilos ni del motor: recibe el backend con el que sonar y lo
// avanza con el deltaTime de la escena. Todo su estado vive aqui, asi que se
// puede probar entero sin audio (con un backend falso).
class GestorMusica {
public:
    explicit GestorMusica(IMusicaBackend& backend);
    // Cierra las pistas que sigan abiertas (las que el reproductor abrio).
    ~GestorMusica();

    GestorMusica(const GestorMusica&) = delete;
    GestorMusica& operator=(const GestorMusica&) = delete;

    // --- lista de pistas ---
    // Fija la lista de pistas (rutas, en orden). El orden es el que se sigue
    // al pasar a la pista siguiente. Las rutas vacias se descartan.
    void establecerPistas(const std::vector<std::string>& rutas);
    // Anade una pista al final de la lista.
    void anadirPista(const std::string& ruta);
    // Vacia la lista.
    void limpiarPistas();
    // La lista actual (copia): el orden es el de reproduccion.
    std::vector<std::string> pistas() const;
    bool tienePista(const std::string& ruta) const;
    // Posicion de la ruta en la lista (-1 si no esta).
    int indiceDe(const std::string& ruta) const;
    // Al acabarse la ultima pista se vuelve a la primera (true, por defecto)
    // o la musica se detiene (false).
    void establecerRepetirLista(bool repetir);
    bool repetirLista() const;

    // --- reproduccion ---
    // Empieza a sonar la pista de la RUTA dada. La ruta no tiene que estar en
    // la lista: se puede tocar cualquier archivo; si no esta, suena sola (al
    // acabarse no hay pista siguiente). Si ya suena otra, se funden: la que
    // suena baja y la nueva sube durante el fundido. Con `duracionFundido`
    // negativa se usa la duracion por defecto del reproductor; con 0 el cambio
    // es inmediato. Devuelve false si la ruta esta vacia o el backend no pudo
    // abrirla.
    bool reproducir(const std::string& ruta, float volumen = 1.0f,
                    float duracionFundido = -1.0f);
    // Pasa a la pista siguiente de la lista (con fundido).
    bool reproducirSiguiente(float volumen = 1.0f, float duracionFundido = -1.0f);
    // Detiene la musica. Con un fundido > 0 el volumen baja a cero antes de
    // parar.
    void detener(float duracionFundido = 0.0f);

    // --- bus ---
    void volumenBus(float volumen);
    float volumenBusActual() const;
    void silenciarBus(bool silenciado);
    bool busSilenciado() const;

    // Duracion por defecto de los fundidos, en segundos.
    void duracionFundido(float segundos);
    float duracionFundidoActual() const;

    // --- avance ---
    // Avanza el reproductor: el fundido en curso y el paso automatico a la
    // pista siguiente cuando la actual se acaba (o cuando le queda justo el
    // fundido). Lo llama la escena con su deltaTime.
    void actualizar(float deltaTime);

    // --- estado ---
    // La ruta de la pista que suena ahora (vacia si no suena ninguna).
    const std::string& pistaActual() const;
    // La ruta de la pista que esta entrando en el fundido en curso (vacia si
    // no hay).
    const std::string& pistaSiguiente() const;
    bool sonando() const;
    bool enFundido() const;
    // Progreso del fundido en curso, de 0 a 1 (0 si no hay fundido).
    float progresoFundido() const;

private:
    // Estado de un fundido en curso. `idSaliente` es la pista que se va (0 si
    // no hay ninguna: cuando solo entra una pista) e `idEntrante` la que
    // entra (0 si solo se va una).
    struct Fundido {
        bool activo = false;
        int idSaliente = 0;
        int idEntrante = 0;
        float duracion = 0.0f;
        float transcurrido = 0.0f;
        float progreso = 0.0f;
        float volumenSaliente = 1.0f;
        float volumenEntrante = 0.0f;
        float volumenObjetivo = 1.0f;
        int indiceSaliente = -1;
        int indiceEntrante = -1;
    };

    bool iniciarCambio(const std::string& ruta, float volumen,
                       float duracionFundido);
    void avanzarFundido(float deltaTime);
    void completarFundido();
    void cancelarEntrada();
    std::string rutaSiguienteDeLista() const;

    IMusicaBackend& backend_;
    // La lista de pistas, en orden de reproduccion (cada una es su ruta).
    std::vector<std::string> pistas_;

    int idActual_ = 0;
    float volumenActual_ = 0.0f;
    int indiceActual_ = -1;
    std::string rutaActual_;

    int idSiguiente_ = 0;
    int indiceSiguiente_ = -1;
    std::string rutaSiguiente_;

    Fundido fundido_;
    float volumenBus_ = 1.0f;
    bool busSilenciado_ = false;
    float duracionFundido_ = 2.0f;
    bool repetirLista_ = true;
};

#endif // GESTORMUSICA_H
