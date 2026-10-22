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
#ifndef ANIMACIONESPARTICULASINTERFACE_H
#define ANIMACIONESPARTICULASINTERFACE_H

#include <string>

#include "../GeneralUserInterface.h"
#include "../../Particulas/PerfilParticula.h"

class EditorController;

// Ventana dedicada a los efectos: el taller donde se configura un perfil de
// particulas (vida, velocidad, aceleracion, rozamiento y las curvas de tamano
// y color) y se genera como archivo .json dentro de los assets del proyecto.
// El archivo aparece en el explorador de contenido y desde ahi se arrastra
// sobre el campo de perfil de un EmisorParticulas. La pestana de prefabs
// gestiona la biblioteca de objetos guardados como .prefab: guardar la
// seleccion, instanciar (por lista o arrastrando desde el explorador),
// renombrar y borrar.
//
// La ventana edita SU PROPIO perfil: no esta atada a la seleccion de la
// escena, igual que el explorador de contenido. Para retocar un efecto ya
// generado se arrastra su .json sobre la zona de carga y el editor lo trae.
class AnimacionesParticulasInterface : public GeneralUserInterface {
public:
    AnimacionesParticulasInterface();

    void contentGUI() override;
    void printGUI() override;

    // El editor llega cuando la escena se vincula al gestor de GUI: la
    // pestana de prefabs usa su seleccion y su biblioteca de prefabs.
    void setEditor(EditorController* editor) noexcept { editor_ = editor; }

protected:
    // Pestana de particulas: el editor del perfil y el boton de generar.
    void pestanaParticulas();
    // Pestana de animaciones: donde vivira el taller de animaciones 2D.
    void pestanaAnimaciones();
    // Pestana de prefabs: la biblioteca de la raiz de assets del proyecto.
    void pestanaPrefabs();

    // Editor del perfil en curso: datos generales, curvas y generacion.
    void dibujarDatosGenerales();
    void dibujarCurvaTamanos();
    void dibujarCurvaColores();
    void dibujarZonaCarga();
    void generarPerfil();

    // Prefabs: instanciar por nombre o desde la ruta arrastrada, guardar la
    // seleccion y mantener el estado de la pestana.
    void instanciarPrefab(const std::string& nombre);
    void instanciarDesdeRuta(const std::string& ruta);
    void dibujarAccionesPrefab();

    EditorController* editor_ = nullptr;
    PerfilParticula perfil_;
    // Nombre con el que se genera el archivo (independiente del nombre del
    // perfil, que es solo la etiqueta que muestra el inspector).
    char nombreArchivo_[128] = "";
    // Nombre para renombrar o guardar un prefab (buffer de los popups).
    char nombrePrefabBuf_[128] = "";
    // Prefab elegido en la lista de la pestana de prefabs.
    std::string prefabSeleccionado_;
    // La biblioteca se escanea la primera vez que se abre la pestana y
    // despues de cada operacion; leer el disco cada frame seria un despilfarro.
    bool primeraVezPrefabs_ = true;
    // Resultado de la ultima operacion (generar o cargar), para feedback en
    // la ventana misma.
    std::string mensaje_;
    bool mensajeError_ = false;
};
#endif
