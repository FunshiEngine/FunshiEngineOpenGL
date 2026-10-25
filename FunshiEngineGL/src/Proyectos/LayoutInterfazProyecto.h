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
#ifndef LAYOUTINTERFAZPROYECTO_H
#define LAYOUTINTERFAZPROYECTO_H

#include <string>

struct ImGuiIO;

// Layout de la interfaz (imgui.ini) del proyecto activo.
//
// El archivo guarda la disposicion de las ventanas y de los docks, y vive en
// la carpeta Memory del proyecto. Su unico trabajo es decidir que archivo usa
// ImGui, volcar el layout del proyecto que se abandona y cargar el del
// entrante; no tiene nada que ver con el ciclo de vida del proyecto (crear,
// abrir, renombrar, borrar), que vive en el gestor de proyectos.
//
// La carga del entrante es diferida: ImGui la ejecuta en el frame siguiente
// al cambio, cuando las ventanas del proyecto nuevo ya existen y se les puede
// re-aplicar el dock.
class LayoutInterfazProyecto {
public:
    // El ImGuiIO se crea con ImGui::CreateContext() en el arranque del editor,
    // mas tarde que este objeto: se fija despues.
    void fijarImguiIO(ImGuiIO* io) noexcept { io_ = io; }

    // Aplica el layout del proyecto indicado. Sin proyecto (nombre vacio) la
    // interfaz se queda sin archivo de layout: no se crea ninguna carpeta
    // fantasma.
    void aplicarProyecto(const std::string& nombreProyecto);

    // Conmuta al layout del proyecto indicado: vuelca el vigente (el del
    // proyecto que se abandona) y deja preparado el del entrante.
    void conmutarProyecto(const std::string& nombreProyecto);

    // Vuelca el layout vigente al archivo del proyecto que se abandona.
    // ImGui solo escribe el archivo que tiene activo, asi que hay que hacerlo
    // antes de conmutar.
    void volcarLayoutVigente();

    // Ejecuta la carga pendiente. Llamar DESPUES de ImGui::NewFrame() del frame
    // siguiente al cambio de proyecto.
    void procesarCargaDiferida();

private:
    // Ruta del layout vigente. ImGui guarda el puntero a este string, asi que
    // tiene que vivir toda la aplicacion (miembro de un objeto de vida
    // completa).
    std::string ini_;
    ImGuiIO* io_ = nullptr;
    bool cargaPendiente_ = false;
    std::string rutaCarga_;
};

#endif // LAYOUTINTERFAZPROYECTO_H
