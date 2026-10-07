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
#ifndef SCENE_OBJECT_TREE_H
#define SCENE_OBJECT_TREE_H

#include "../../Herramientas/TreeGUI/TreeGUI.h"

class GameObject;
class SceneRegistry;
class EditorController;
class EventBus;
class IconosGUI;

/**
 * Widget reutilizable que dibuja la jerarquia de la escena: una fila por
 * GameObject con icono, nombre, colapso/expansion con estado persistente,
 * renombrado en linea, menu contextual y drag & drop.
 *
 * Es un componente puro de contenido: se dibuja dentro de la ventana activa
 * de ImGui (no abre/cierra ventanas). Se enlaza a la escena con bindScene(),
 * igual que los paneles de GUI.
 *
 * El delete y el reparent se aplican de forma diferida (tras el recorrido)
 * para no invalidar los iteradores del arbol mientras se dibuja.
 */
class SceneObjectTree {
public:
    SceneObjectTree() = default;
    ~SceneObjectTree();

    // No se copia: posee una suscripcion al EventBus.
    SceneObjectTree(const SceneObjectTree&) = delete;
    SceneObjectTree& operator=(const SceneObjectTree&) = delete;

    void bindScene(SceneRegistry* value, EditorController* controller,
                   EventBus* bus);
    void setIconosGUI(IconosGUI* iconos);
    void draw();

private:
    TreeIG::RowResult drawRow(GameObject* object, bool wasOpen);
    void applyDeferredOperations();
    void resetState();
    void unbind();
    void dibujarDialogosModales();
    void manejarAtajos();
    static bool esDescendiente(GameObject* nodo, GameObject* ancestro);

    SceneRegistry* scene = nullptr;
    EditorController* editor = nullptr;
    EventBus* events = nullptr;
    size_t eventSubscription = 0;
    IconosGUI* iconosGUI = nullptr;

    // Nodos abiertos en la jerarquia (estado de colapso persistente).
    TreeIG::OpenState openNodes;
    // Estado de renombrado en linea (doble click o menu Renombrar).
    GameObject* renombrando = nullptr;
    GameObject* objetoSeleccionPendiente = nullptr;
    ImVec2 posicionInicioSeleccion = ImVec2(0.0f, 0.0f);
    GameObject* objetoBajoMouseAlSoltar_ = nullptr;
    // Operaciones diferidas: esperan al final del recorrido para mutar el
    // arbol con seguridad.
    GameObject* objetoAEliminar = nullptr;
    GameObject* objetoAReParentar = nullptr;
    GameObject* objetoPadreNuevo = nullptr;
    // Portapapeles de copiar/pegar (Ctrl+C/X/V): no es dueño, solo observa.
    // Pegar varias veces el mismo objeto conserva el portapapeles; cortar lo
    // vacia al pegar (es mover). El pegado tambien es diferido.
    GameObject* objetoCopiado = nullptr;
    bool cortePendiente = false;
    GameObject* objetoAPegar = nullptr;
    GameObject* padreDePegado = nullptr;
    // Desanidar a raiz diferido: el objeto a mover a la raiz.
    GameObject* objetoADesanidar = nullptr;

    // Estado de dialogos modales (click derecho -> Cambiar ID / Renombrar / Eliminar).
    enum class DialogoTipo { Ninguno, CambiarID, Renombrar, Eliminar };
    DialogoTipo dialogoActivo = DialogoTipo::Ninguno;
    GameObject* objetoEnDialogo = nullptr;
    char bufferDialogo[256] = "";
    bool dialogoRecienAbierto = false; // para abrir el popup solo la primera vez
};

#endif