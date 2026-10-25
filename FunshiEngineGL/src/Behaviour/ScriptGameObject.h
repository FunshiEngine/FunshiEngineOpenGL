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
// La tabla de servicios tambien es APPEND-ONLY y comparte este numero: la
// v8 anadio las funciones de musica, asi que los scripts compilados contra
// el contrato viejo se recompilan con la cabecera nueva.
constexpr int versionRuntimeScript = 13;
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

    // --- v5: propiedades fisicas del RigidBody (si no hay cuerpo, los
    // getters devuelven 0/false y los setters devuelven false) ---
    // Masa (0 = estatico).
    float (*masa)(const void* objeto);
    bool (*fijarMasa)(void* objeto, float masa);
    // Gravedad por cuerpo: sin ella el cuerpo no cae; la escala multiplica
    // la gravedad global del mundo (1 = igual, 0 = flota).
    bool (*usaGravedad)(const void* objeto);
    bool (*fijarUsoGravedad)(void* objeto, bool usar);
    float (*escalaGravedad)(const void* objeto);
    bool (*fijarEscalaGravedad)(void* objeto, float escala);
    // Friccion de contacto (0 = desliza).
    float (*friccion)(const void* objeto);
    bool (*fijarFriccion)(void* objeto, float friccion);
    // Freeze por ejes (eje: 0 = X, 1 = Y, 2 = Z).
    bool (*posicionCongelada)(const void* objeto, int eje);
    bool (*fijarFreezePosicion)(void* objeto, bool x, bool y, bool z);
    bool (*rotacionCongelada)(const void* objeto, int eje);
    bool (*fijarFreezeRotacion)(void* objeto, bool x, bool y, bool z);

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
class EstadoInterfaces;
class PhysicsEngine;
class EditorController;

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
    // Reproduce un archivo de audio del proyecto (por RUTA). La ruta es
    // relativa a la raiz de assets (src<nombre>) o absoluta. Devuelve -1 si
    // no hay motor o el archivo no existe (tolerante, no bloquea); sino un
    // handle >= 0 usable con detenerSonido().
    // (v7: la ruta sustituye al nombre del clip del catalogo.)
    int (*reproducirSonido)(const char* ruta, float volumen, bool bucle);
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

    // --- v4: gravedad global y gestion de objetos ---
    // Gravedad del mundo fisico (la que escalan los cuerpos que la usan).
    void (*fijarGravedadGlobal)(float x, float y, float z);
    float (*gravedadGlobalX)();
    float (*gravedadGlobalY)();
    float (*gravedadGlobalZ)();
    // Crea un objeto vacio con ese nombre (o uno por defecto si es nulo/vacio)
    // como hijo de `padre` (raiz si es nulo). Devuelve el puntero de inmediato
    // para configurarlo, pero entra a la escena al final del frame: el
    // puntero es valido mientras el objeto viva. La fisica del frame en curso
    // no lo ve; la del siguiente, si.
    void* (*crearObjeto)(const char* nombre, void* padre);
    // Marca para borrar al final del frame (seguro incluso sobre si mismo).
    bool (*destruirObjeto)(void* objeto);
    // Copia profunda (componentes incluidos) como hija de `padre` (raiz si es
    // nulo). Misma validez diferida que crearObjeto. Los scripts copiados
    // compilan a demanda en su primera actualizacion.
    void* (*clonarObjeto)(const void* original, void* padre);
    // Agrega collider (falla si ya tiene uno) y cuerpo (falla si no hay
    // collider o ya tiene cuerpo). Entran en vigor al final del frame.
    bool (*agregarColliderEsfera)(void* objeto, float radio);
    bool (*agregarColliderCubo)(void* objeto, float radio);
    bool (*agregarRigidBody)(void* objeto, float masa);

    // --- v5: colisiones y raycast (hitscan) ---
    // Resuelve un Collider* a su GameObject duenio. Util en
    // onCollisionEnter/onCollisionStay/onCollisionExit para saber qué objeto
    // recibio el golpe.
    void* (*objetoDeCollider)(const void* collider);
    // Dispara un rayo (hitscan) desde origen en direccion con alcance dado.
    // Llena punto/normal/distancia/collider si hubo impacto.
    bool (*raycast)(float ox, float oy, float oz,
                    float dx, float dy, float dz,
                    float alcance,
                    float* distancia, float* px, float* py, float* pz,
                    float* nx, float* ny, float* nz,
                    void** collider);
    // Ultimo punto de impacto del rayo (valido tras raycast con impacto).
    float (*puntoImpactoX)();
    float (*puntoImpactoY)();
    float (*puntoImpactoZ)();
    // Ultima normal de impacto del rayo.
    float (*normalImpactoX)();
    float (*normalImpactoY)();
    float (*normalImpactoZ)();
    // Distancia al ultimo impacto.
    float (*distanciaImpacto)();

    // --- v6: sistema de prefabs ---
    // Instancia un prefab por nombre como hijo de `padre` (raiz si nulo).
    // Devuelve el puntero a la raiz de la instancia (valido al final del frame).
    void* (*instanciarPrefab)(const char* nombre, void* padre);
    // Lista los nombres de prefabs disponibles en la biblioteca.
    // Devuelve un array terminado en nullptr (el llamador no debe liberar).
    // El array y los nombres que apunta son validos solo hasta la llamada
    // siguiente: copiarlos en el acto si deben retenerse.
    const char** (*listarPrefabs)();

    // --- v7: interfaces web (HUD) ---
    // Manipulacion de las interfaces de la escena desde los scripts. El handle
    // (`interfaz`) es opaco: un puntero al InterfaceComponent del objeto que la
    // tiene. En C++ se obtiene con un campo REFLECT_COMPONENTE (arrastrando el
    // componente a la propiedad) o con interfazDeObjeto(); en Java, con
    // interfazDeObjeto. Toda funcion tolera un handle nulo (no hace nada).
    // Handle de la interfaz del objeto `nombreObjeto`, o nullptr si no existe
    // o no tiene InterfaceComponent.
    void* (*interfazDeObjeto)(const char* nombreObjeto);
    // Estado de visibilidad de la interfaz.
    bool (*interfazActiva)(const void* interfaz);
    void (*interfazEstablecerActiva)(void* interfaz, bool activa);
    // Reemplaza el arbol de estado que el motor entrega a la interfaz. `json`
    // debe ser un objeto JSON; devuelve false si el texto no es valido.
    bool (*interfazEstablecerEstado)(void* interfaz, const char* jsonEstado);
    // Fija un valor en la ruta indicada (JSON Pointer, p. ej. "/jugador/vida";
    // se acepta sin la barra inicial). Crea los objetos intermedios que falten.
    // `valorJson` es un literal JSON de cualquier tipo.
    bool (*interfazEstablecerValor)(void* interfaz, const char* clave,
                                    const char* valorJson);
    // Valor que la interfaz devolvio al motor en la ruta dada, como texto JSON,
    // o nullptr si no existe. El puntero vale hasta la proxima lectura de la
    // misma interfaz.
    const char* (*interfazValor)(void* interfaz, const char* rutaJson);

    // --- v8: musica (por RUTA) ---
    // Reproduce una pista de musica (por RUTA, como los efectos: relativa a la
    // raiz de assets del proyecto o absoluta). Devuelve false si no hay motor
    // o el archivo no existe (queda avisado en el log). Si la pista no esta en
    // la lista del componente `Musica` de la escena, suena sola: al acabarse
    // no hay pista siguiente.
    bool (*reproducirMusica)(const char* ruta, float volumen, float fundido);
    // Detiene la musica (con fundido de salida opcional).
    void (*detenerMusica)(float fundido);
    // Volumen del bus de musica (todas las pistas juntas, de 0 a 1).
    void (*volumenMusica)(float volumen);

    // --- v9: interfaz de juego NATIVA (HUD que dibuja el motor) ---
    // Mismas reglas que las interfaces web: el handle (`interfaz`) es opaco
    // (un puntero al UIComponent del objeto que la tiene) y toda funcion
    // tolera un handle nulo (no hace nada). Se distingue de la interfaz web
    // por el prefijo: son dos sistemas distintos que pueden convivir en la
    // misma escena.
    // Handle de la interfaz nativa del objeto `nombreObjeto`, o nullptr si no
    // existe o no tiene UIComponent.
    void* (*interfazNativaDeObjeto)(const char* nombreObjeto);
    // Si la interfaz nativa esta activa (visible).
    bool (*interfazNativaVisible)(const void* interfaz);
    void (*interfazNativaEstablecerVisible)(void* interfaz, bool visible);
    // Cambia el texto de un widget (por su identificador). Devuelve false si
    // no hay ningun widget con ese identificador.
    bool (*interfazNativaEstablecerTexto)(void* interfaz, const char* idWidget,
                                           const char* texto);
    // Muestra u oculta un widget. Devuelve false si no existe.
    bool (*interfazNativaEstablecerVisibleWidget)(void* interfaz,
                                                   const char* idWidget,
                                                   bool visible);
    // Si ese boton se pulso en el frame actual (flanco de pulsacion).
    bool (*interfazNativaBotonPulsado)(void* interfaz, const char* idWidget);

    // Version de la tabla (siempre al final).
    int version;
};

// Tabla de servicios de escena para scripts (audio, busqueda, teclado).
const ScriptServices* tablaServicios();

// Cablea el contexto real de la escena. GameScene la llama al entrar en
// simulacion y, despues de onStop, con todos los punteros nulos para
// desconectar; al desconectar tambien se detienen los handles de audio creados
// por scripts y se descartan las peticiones de objetos sin aplicar.
// ScriptGameObject no depende de las cabeceras de Audio/Scenes/Input.
void inyectarServiciosScript(AudioEngine* audio, SceneRegistry* escena,
                             InputScripts* input,
                             PhysicsEngine* fisica = nullptr,
                             EditorController* editor = nullptr,
                             EstadoInterfaces* interfaces = nullptr);

// Aplica las peticiones de crear/clonar/destruir/agregar acumuladas por los
// scripts durante el frame. La llama GameScene al terminar de actualizar los
// objetos (mutar la escena a mitad del recorrido invalidaria el iterador).
void procesarPeticionesObjetos();

} // namespace MotorScript

#endif // SCRIPTGAMEOBJECT_H