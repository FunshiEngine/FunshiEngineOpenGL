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
#ifndef SCRIPTGAMEOBJECT_H
#define SCRIPTGAMEOBJECT_H

// Interfaz que ven los scripts sobre GameObject. Para que el .so/.dll de un
// script NO tenga que enlazar contra el motor (ni depender de las cabeceras que
// arrastran Bullet/Assimp), el motor inyecta una TABLA DE PUNTEROS A FUNCION:
// IScriptBehaviour::conectarApi(). Los scripts la usan via `this->api->...`.

class GameObject;

namespace MotorScript {

// Tabla de acceso a transform/nombre/log. Implementada por el motor
// (ScriptGameObject.cpp) y entregada al comportamiento en su creacion.
//
// Convencion de versionado: APPEND-ONLY. Los campos nuevos se agregan SIEMPRE
// al final y los existentes no se reordenan ni se cambian de tipo. Esto permite
// versionar la tabla; no promete compatibilidad binaria entre versiones del
// motor. Incrementar esta version cuando cambie el contrato/API/runtime: el
// cache la incorpora y recompila los scripts con el motor actualizado.
constexpr int versionRuntimeScript = 8;
struct ApiScriptGameObject {
    // --- v1 (original) ---
    const char* (*nombre)(const void* objeto);
    // Posicion y escala son valores locales del componente Transform.
    float (*posicionX)(const void* objeto);
    float (*posicionY)(const void* objeto);
    float (*posicionZ)(const void* objeto);
    void (*fijarPosicion)(void* objeto, float x, float y, float z);
    void (*fijarEscala)(void* objeto, float x, float y, float z);
    void (*fijarRotacionEjes)(void* objeto, float angulo, float x, float y,
                              float z);
    void (*imprimirConsola)(const char* texto);

    // --- v2: getters de transform que faltaban (rotacion y escala) ---
    // El angulo se expone en radianes y el eje es unitario. El componente
    // Transform conserva internamente el angulo en grados.
    float (*rotacionAngulo)(const void* objeto);
    float (*rotacionEjeX)(const void* objeto);
    float (*rotacionEjeY)(const void* objeto);
    float (*rotacionEjeZ)(const void* objeto);
    // Escala por eje.
    float (*escalaX)(const void* objeto);
    float (*escalaY)(const void* objeto);
    float (*escalaZ)(const void* objeto);

    // --- v3: movimiento de cuerpos fisicos ---
    // Devuelve true si el objeto tiene RigidBody; la velocidad es mundial.
    bool (*fijarVelocidadHorizontal)(void* objeto, float x, float z);
    // Solo salta si el cuerpo activo esta apoyado.
    bool (*saltar)(void* objeto, float velocidad);

    // --- v4: tags y datos de contacto ---
    const char* (*etiqueta)(const void* objeto);
    bool (*tieneEtiqueta)(const void* objeto, const char* etiqueta);
    void* (*objetoDeCollider)(const void* collider);

    // Version de la tabla (siempre al final).
    int version;
};

// Tabla implementada por el motor (ScriptGameObject.cpp). No la usan los
// scripts directamente; el backend se la entrega a IScriptBehaviour::conectarApi.
const ApiScriptGameObject* tablaApi();

} // namespace MotorScript

// Declaraciones adelantadas en el namespace global (los sistemas reales —
// Audio/AudioEngine.h, Scenes/SceneRegistry.h, Input/InputScripts.h — viven
// fuera de MotorScript). El modulo de scripts solo ve punteros opacos.
class AudioEngine;
class SceneRegistry;
class InputScripts;

namespace MotorScript {

// Servicios del motor disponibles para scripts, inyectados por GameScene al
// cargar los comportamientos (via ScriptRuntime). Separada de
// ApiScriptGameObject para mantener esa tabla centrada en el GameObject y
// modularidad: audio y busqueda de objetos son servicios de escena, no del
// objeto. Los scripts acceden via `this->servicios->...` (IScriptBehaviour
// expone el puntero `servicios`).
//
// Convencion APPEND-ONLY igual que ApiScriptGameObject, con campo `version`
// final para guardas `if (servicios->version >= N)`.
struct ScriptServices {
    // --- v1 ---
    // Reproduce un clip de la carpeta Sonidos/ del proyecto (por nombre).
    // Devuelve -1 si el clip no existe o no hay motor (tolerante, no
    // bloquea); sino un handle >= 0 usable con detenerSonido().
    int (*reproducirSonido)(const char* clip, float volumen, bool bucle);
    void (*detenerSonido)(int handle);

    // Busca un GameObject por nombre en la escena (recorrido en preorden del
    // registro). Devuelve nullptr si no existe; el puntero es valido mientras
    // el objeto viva (no crear/destruir objetos desde scripts aun).
    void* (*objetoPorNombre)(const char* nombre);

    // --- v3: otras formas de resolver referencias a objetos ---
    // Busca por id de escena (GameObject::getId); primer coincidente o nullptr.
    void* (*objetoPorId)(int id);
    // Busca por tag el primer objeto que lo tenga; nullptr si ninguno.
    // Util para referencias opcionales (ej. la camara de un controlador).
    void* (*objetoPorEtiqueta)(const char* etiqueta);

    // Consulta de teclado (nombres GLFW, ej. "W", "D0", "SPACE").
    bool (*teclaSostiene)(const char* tecla);      // mantenida apretada
    bool (*teclaPresionada)(const char* tecla);    // este frame (edge press)
    bool (*teclaSoltada)(const char* tecla);       // este frame (edge release)

    // Delta acumulado del mouse en pixeles durante el frame actual (v2).
    float (*deltaMouseX)();
    float (*deltaMouseY)();

    // Version de la tabla (siempre al final).
    int version;
};

// Tabla de servicios de escena para scripts (audio, busqueda, teclado).
const ScriptServices* tablaServicios();

// Cablea el contexto real de la escena. GameScene la llama al entrar en
// simulacion y, despues de onStop, con todos los punteros nulos para
// desconectar; al desconectar tambien se detienen los handles de audio creados
// por scripts. ScriptGameObject no depende de las cabeceras de Audio/Scenes/Input.
void inyectarServiciosScript(AudioEngine* audio, SceneRegistry* escena,
                             InputScripts* input);

} // namespace MotorScript

#endif // SCRIPTGAMEOBJECT_H