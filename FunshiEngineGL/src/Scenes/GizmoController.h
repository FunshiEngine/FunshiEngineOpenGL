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
#ifndef GIZMO_CONTROLLER_H
#define GIZMO_CONTROLLER_H

#include <memory>

#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"

struct ImGuiIO;
class CameraComponent;
class EditorController;
class GameObject;
class SceneRegistry;
class SceneSelectedInterface;
class Transform;
class TransformComando;

// Gizmo del editor (ImGuizmo) extraido de GameScene::gameScene() para modularizar
// la escena: seleccion por clic (picking) + dibujado/manipulacion del gizmo de
// transform con el sistema de fotos/historial (TransformComando) y la
// sincronizacion RigidBody. No acopla con GameScene: recibe sus dependencias por
// inyeccion en el constructor y los datos por frame en cada llamada.
//
// La escena corre dos fases por frame, igual que antes:
//   1. procesarSeleccion() SIEMPRE (es el disparador que enciende/limpia la
//      seleccion y, con ella, las interfaces de edicion). Prepara ImGuizmo y
//      atiende el clic sobre la escena 3D.
//   2. Si el editor sigue activo, GUI() y luego dibujarYRastrear() (objetivo,
//      Manipulate y arrastre con foto inicial/final).
class GizmoController {
private:
    // Sistema de coordenadas del gizmo: false = LOCAL (rotacion de los ejes con
    // el objeto, comportamiento historico); true = GLOBAL/WORLD (ejes del mundo,
    // el gizmo NO rota con el objeto). Alternable con G o el menu "Gizmo".
    bool listo_ = false;
    bool global_ = false;
    int operation_ = 7; // ImGuizmo::TRANSLATE

    // Arrastre del gizmo en curso: se toma una foto del transform al iniciar el
    // arrastre y otra al terminar, para registrar UN TransformComando por
    // movimiento del usuario (y no uno por frame). El comando queda pendiente
    // hasta que el gizmo se suelta; si el transform no cambio, se descarta.
    struct EstadoTransform {
        float pos[3] = {0, 0, 0};
        float rot[4] = {0, 0, 0, 0};
        float esc[3] = {1, 1, 1};
    };
    bool arrastrando_ = false;
    EstadoTransform arrastreInicial_;
    EstadoTransform arrastreFinal_;
    std::unique_ptr<TransformComando> arrastreComando_;

    EditorController* controlador_ = nullptr;
    SceneRegistry* registro_ = nullptr;
    SceneSelectedInterface* seleccion_ = nullptr;

    // El puntero no es const porque los getters del Transform (getTranslatef,
    // getRotatef, getScalef) no son const en el componente.
    void tomarFotoTransform(Transform* t, EstadoTransform& destino) const;
    static bool transformDistinguible(const EstadoTransform& a,
                                      const EstadoTransform& b);

public:
    // Inyeccion explicita: el EditorController (gizmo target/comandos), el
    // SceneRegistry (para el TransformComando) y la seleccion (lectura/escritura
    // del objeto seleccionado). Todos viven mientras exista la escena.
    GizmoController(EditorController* controlador, SceneRegistry* registro,
                    SceneSelectedInterface* seleccion) noexcept;
    // Definido fuera de linea en el cpp: destruye el arrastreComando_
    // (unique_ptr<TransformComando>) con el tipo completo.
    ~GizmoController();

    // Estado del gizmo (lo persisten los proyectos / lo alternan G y el menu).
    void setGizmoOperation(int operation);
    int getGizmoOperation() const noexcept { return operation_; }
    void setGizmoGlobal(bool global) noexcept { global_ = global; }
    bool isGizmoGlobal() const noexcept { return global_; }

    // El gizmo captura el input del raton (no deseleccionar al arrastrarlo / al
    // mover la camara con el clic sobre el). gizmoInUse() es SOLO arrastre.
    bool isCapturingInput() const noexcept;
    bool gizmoInUse() const noexcept;

    // Fase 1 (siempre): prepara ImGuizmo para el frame y resuelve el clic de
    // seleccion/deseleccion sobre la escena 3D (disparador del modo editor).
    void procesarSeleccion(ImGuiIO& io, CameraComponent* camara,
                           ListaDE<GameObject*>* objetos);

    // Navegacion libre (sin E y sin objeto seleccionado): la escena apaga el
    // gizmo y no dibuja GUI (solo la escena 3D).
    void apagar() noexcept { listo_ = false; }

    // Fase 2 (solo con el editor activo): resuelve el objetivo (GizmoTarget
    // externo o transform del objeto/collider seleccionado) y dibuja/manipula
    // el gizmo, tomando la foto inicial al empezar el arrastre y registrando un
    // TransformComando al soltarlo. Sincroniza RigidBody al terminar.
    void dibujarYRastrear(ImGuiIO& io, CameraComponent* camara);

    // Picking sobre la escena 3D: devuelve el GameObject bajo un punto de
    // pantalla (o nullptr). API publica preservada de GameScene.
    GameObject* pickObject(float mouseX, float mouseY, CameraComponent* camara,
                           ListaDE<GameObject*>* objetos);

    // Direccion del bool LOCAL/GLOBAL para el menu del editor: la barra edita
    // el bool directamente (mismo patron que el boton play/stop con toggleBool).
    bool* direccionGizmoGlobal() noexcept { return &global_; }
};

#endif