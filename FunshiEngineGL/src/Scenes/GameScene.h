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
#ifndef GAMESCENE_H
#define GAMESCENE_H

#include <memory>
#include <string>
#include <vector>

#include "../Events/EventBus.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Iluminacion/LightSystem.h"
#include "../Behaviour/ScriptRuntime.h"
#include "../Behaviour/ScriptGameObject.h"
#include "../Input/InputScripts.h"
#include "../Configuracion/Apariencia.h"
#include "../Audio/AudioClipsManager.h"

class CameraComponent;
class EditorController;
class Transform;
class GUIManager;
class GameObject;
class PhysicsEngine;
class SceneMenuBarInterface;
class SceneRegistry;
class SceneSelectedInterface;
class SceneSerializer;
class AssimpMeshLoader;
class AssetManager;
class TextureManager;
class Script;
class SceneRenderer;
class AudioEngine;
class CanvasInterface;
class CreadorDeInterfaces;
class MiniAudioBackend;
class GizmoController;

class GameScene {
private:
    GUIManager* managerGUI;
    SceneSelectedInterface* selecteableGUI = nullptr;
    SceneMenuBarInterface* menuBarGUI = nullptr;
    CameraComponent* activeCamera = nullptr;
    GameObject* activeCameraObject = nullptr;
    // Camara elegida con "Usar" en la ventana Camaras. Con nullptr la activa
    // es la primera camara de la escena (o la que se siembra al inicio).
    GameObject* requestedActiveCamera = nullptr;
    int contadorCamaras = 0;
    bool ventanaCamarasAbierta = true;
    std::unique_ptr<SceneRegistry> sceneRegistry;
    std::unique_ptr<PhysicsEngine> phisics;
    // Registro central de assets (meshes CPU compartidos). Se inyecta a
    // EditorController y SceneSerializer para que cada Modelos3D pida su
    // geometria al cache (Flyweight) en lugar de parsear Assimp por objeto.
    std::unique_ptr<AssetManager> assetManager;
    // Pasada de render de la escena (Fase 3): el SceneRenderer es dueno del
    // MeshRenderer (VBO/VAO + shader), la grilla y las vistas previas; GameScene
    // solo le pasa el FrameContext por frame (objetos, luces CPU, apariencia).
    // Si el pipeline moderno no esta disponible, cada objeto degrada al modo
    // inmediato (fallback preservado).
    std::unique_ptr<SceneRenderer> sceneRenderer;
    // Registro central de imagenes CPU compartidas (flyweight). Igual que el
    // AssetManager de meshes, se inyecta a quien resuelve recursos: aqui lo
    // usa el MeshRenderer para convertir el path de textura de cada Material
    // en una imagen subida una sola vez a GPU.
    std::unique_ptr<TextureManager> textureManager;
    EventBus events;
    // Los constructores de EditorController y SceneSerializer reciben
    // pointers a los miembros anteriores (assetManager/events): deben estar
    // declarados ANTES de ellos, porque C++ inicializa los miembros en orden
    // de declaracion (no de la lista de iniciadores).
    std::unique_ptr<EditorController> editorController;
    std::unique_ptr<SceneSerializer> sceneSerializer;
    LightSystem lightSystem;
    // Motor de audio (facade, hilo de trabajo propio). Siempre existe aunque
    // el backend de miniaudio no pueda iniciar (falla silenciosa -> mudo).
    std::unique_ptr<AudioEngine> audioEngine;
    // Consulta de teclado para scripts (GLFW key names). EditorInput vive en
    // main.cpp para la camara; este es un canal independiente de solo lectura
    // que se alimenta de los mismos callbacks.
    InputScripts inputScripts;
    // Catalogo de clips del proyecto (explora Sonidos/ y registra por nombre).
    AudioClipsManager clipsAudio;
    float deltaTime = 0.0f;
    bool start = false;
    // version anterior de start: detecta la transicion false->true para
    // sincronizar los cuerpos a la pose VISUAL del editor ANTES de que el
    // primer stepSimulation los dispare desde la pose vieja.
    bool previousStart = false;
    // Pausa de la simulacion (F6) dentro del play: congela fisica y scripts
    // sin salir de Playing ni tocar `start` (asi no dispara la limpieza de la
    // transicion play->editor). La gobierna main desde el orquestador de
    // estados (funcion de marco de F5/F6/F7).
    bool simulacionPausada = false;
    bool menuActivo = false;
    // Sensibilidad global del mouse look, sincronizada desde MenuGUI (vista
    // Opciones). La aplica main al offset del raton antes de updateYaw().
    float sensibilidadCamara = 0.15f;
    // Sensibilidad de MOVIMIENTO (WASD) del editor: multiplica la velocidad
    // base (speed) de la camara activa. Se configura SOLO en la vista Opciones
    // del menu (configuracion general); es INDEPENDIENTE del mouse look. La
    // aplica main a DeltaTime antes de trasladar.
    float sensibilidadMovimientoCamara = 1.0f;
    // Perfil de apariencia sincronizado desde MenuGUI. La escena solo usa el
    // fondo del viewport y el color de la grilla (modo B/N); el estilo ImGui
    // lo aplica main con TemaEditor.
    Apariencia apariencia;
    // Gizmo de manipulacion de transforms (ImGuizmo): seleccion por clic
    // (picking) y manipulación del transform del objetivo, extraido de
    // gameScene() a su propio modulo. Se construye en el cuerpo del ctor (la
    // seleccion la inyecta GUIManager en el cuerpo) y vive toda la escena.
    std::unique_ptr<GizmoController> gizmoController_;

    void asegurarGrilla();
    // Reproduce/detiene los AudioSource de la escena en las transiciones de
    // modo play (entrar = autoplay de los marcados; salir = detener todo).
    void sincronizarAudioPlay(bool entrarEnPlay);
    // Muestra las vistas previas del SceneRenderer (textura FBO por camara con
    // "Vista previa" activo) como ventanas ImGui.
    void pintarViewportsGUI();

    // Cola de compilacion de scripts (play mode). Cada script se agenda y se
    // procesa en DOS fases para que la barra de estado muestre "Compilando X
    // (i de n)..." un frame antes de bloquear el hilo con g++/javac.
    struct CargaPendiente {
        Script* script = nullptr;
        GameObject* owner = nullptr;
    };
    std::vector<CargaPendiente> colaCompilacion_;
    enum class FaseCarga { Mostrar, Compilar };
    FaseCarga faseCarga_ = FaseCarga::Mostrar;
    std::size_t indiceCarga_ = 0;
    std::string cargaActual_;
    std::size_t cargaTotal_ = 0;
    std::size_t cargaHecha_ = 0;
    std::vector<ScriptRuntime::ResultadoCarga> resultadosCarga_;
    bool compilacionEnCurso_ = false;
    // Overlay de carga (se dispara al pulsar "Activar"): progreso con un
    // minimo visible aunque la compilacion venga de cache, y un aviso breve
    // del resultado al terminar (asi no se depende de la consola).
    float overlayProgresoTimer_ = 0.0f;
    float overlayResultadoTimer_ = 0.0f;
    bool overlayProgresoVisible_ = false;
    bool overlayResultadoVisible_ = false;
    bool overlayResultadoPendiente_ = false;
    bool overlayEnCursoPrev_ = false;
    void encolarScriptsIniciales();
    void procesarColaCompilacion();
    void limpiarColaCompilacion();

public:
    GameScene(GUIManager* managerGUI);
    ~GameScene();

    ListaDE<GameObject*>* getGameObjectsScene();
    void saveScene(const std::string& filename);
    bool isStart();
    // Fuente de verdad de la simulacion: la maquina de estados (F5/F7) la
    // refleja aca desde el input (EditorInput), compartida con el boton
    // Activar/Detener del menu de escena.
    void setStart(bool activo) noexcept;
    // Pausa (F6): congela la simulacion sin salir de play.
    bool isSimulacionPausada() const noexcept;
    void setSimulacionPausada(bool pausada) noexcept;
    void loadScene(const std::string& pathTxt, const std::string& semiPath);
    // Configura los assets de audio (Sonidos/) e interfaces (Memory/Interfaces)
    // del proyecto. La llama main al arrancar y al cambiar de proyecto.
    void configurarProyecto(const std::string& nombreProyecto);
    AudioEngine* getAudioEngine() const noexcept;
    void GUI();
    void pintarVentanaCamaras();
    void update(float deltaTime);
    void gameScene();
    void setGizmoOperation(int operation);
    int getGizmoOperation() const;
    // Sistema de coordenadas del gizmo (LOCAL con rotacion de los ejes del
    // objeto, o GLOBAL con los ejes del mundo fijos). Se persiste por proyecto.
    bool isGizmoGlobal() const noexcept;
    void setGizmoGlobal(bool global) noexcept;
    bool isGizmoCapturingInput() const;
    bool gizmoInUse() const;

    // Guia de eje (X/Y/Z): la recta que marca sobre que eje se mueve el objeto
    // seleccionado. "eje" es 0 = X, 1 = Y, 2 = Z. Es un interruptor (pulsar dos
    // veces la misma tecla la apaga) y se apaga sola al deseleccionar. Con la
    // guia activa el gizmo se oculta. El estado vive en EditorController; esto
    // es la puerta que usa el input.
    void alternarGuiaEje(int eje);
    int getGuiaEje() const noexcept;
    bool hayGuiaEje() const noexcept;
    void clearGuiaEje();

    // Aviso momentaneo en la barra de estado del editor (mismo mecanismo que el
    // "Proyecto guardado" de Ctrl+S). Lo usa el atajo de undo/redo para dejar
    // claro que cambio tomo el estado.
    void mostrarMensaje(const std::string& mensaje);
    GameObject* pickObject(float mouseX, float mouseY);

    // Camara de la escena como Component: devuelve el primer objeto que tenga
    // una CameraComponent (crea "CamaraPrincipal" si la escena no tiene).
    // The navigation FPS (WASD + mouse) opera sobre esta camara.
    CameraComponent* getActiveCamera();

    // Cambia la camara activa (por la que se navega y se ve la escena).
    void setActiveCamera(GameObject* object);

    // Persistencia de la camara activa (EditorConfig): id del GameObject
    // elegido con "Usar" (-1 = automatica) y restauracion tolerante por id
    // (si el objeto ya no existe o perdio su camara, se vuelve a automatica).
    int getActiveCameraId() const noexcept;
    void setActiveCameraById(int id);

    // Crea un GameObject vacio con camara en la posicion/orientacion de la
    // camara activa, activa su vista previa y la deja seleccionada para
    // ubicarla con el gizmo. Ventana "Camaras" -> "Agregar camara".
    GameObject* agregarCamaraEnVistaActiva();

    // Modo editor: interfaces (gizmo, jerarquia, settings, folders) activas
    // si se aprieta E (toggleEditorInterfaces) o hay un objeto seleccionado.
    void toggleEditorInterfaces();
    // Activa o desactiva el modo editor sin alternar (main la enciende al
    // entrar al editor desde el menu para que los paneles sean visibles de una;
    // E la alterna durante la sesion).
    void setMenuActivo(bool activo) noexcept;
    bool isEditorActivo() const;
    void clearSelection();

    // Acceso al controlador del editor para comandos (undo/redo)
    EditorController* getEditorController() noexcept { return editorController.get(); }
    const EditorController* getEditorController() const noexcept { return editorController.get(); }

    // Sensibilidad del mouse look (la setea main desde MenuGUI/Opciones).
    float getSensibilidadCamara() const noexcept;
    void setSensibilidadCamara(float sensibilidad) noexcept;

    // Sensibilidad de movimiento (WASD) del editor, configurable en la vista
    // Opciones del menu. Multiplica la velocidad base de la camara activa (main
    // lo aplica a DeltaTime en el callback de teclado).
    float getSensibilidadMovimientoCamara() const noexcept;
    void setSensibilidadMovimientoCamara(float sensibilidad) noexcept;

    // Perfil de apariencia (lo setea main desde MenuGUI/Opciones). Afecta el
    // fondo de la vista 3D y el color de la grilla.
    const Apariencia& getApariencia() const noexcept;
    void setApariencia(const Apariencia& valor) noexcept;

    // Estado de la ventana "Camaras" (persistido por EditorConfig).
    bool getVentanaCamarasAbierta() const noexcept;
    void setVentanaCamarasAbierta(bool abierta) noexcept;

    // Estado de la compilacion de scripts para la barra "Estado".
    bool compilacionEnCurso() const noexcept { return compilacionEnCurso_; }
    const std::string& cargaActual() const noexcept { return cargaActual_; }
    std::size_t cargaHecha() const noexcept { return cargaHecha_; }
    std::size_t cargaTotal() const noexcept { return cargaTotal_; }
    const std::vector<ScriptRuntime::ResultadoCarga>& resultadosCarga() const noexcept {
        return resultadosCarga_;
    }

    // Apagado de la aplicacion: descarga el comportamiento de todos los
    // scripts de la escena (libera referencias globales JNI / handles) sin
    // disparar onStop. main() la llama antes de apagar la JVM.
    void descargarScripts();
};

#endif
