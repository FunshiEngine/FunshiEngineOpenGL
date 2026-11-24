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
// Preambulo compartido por las unidades de SceneSerializationTests.cpp (generado al dividirlo).
#ifndef SCENESERIALIZATIONTESTSDETALLE_H
#define SCENESERIALIZATIONTESTSDETALLE_H

// Pruebas headless de serializacion de escena: round-trip completo (guardar ->
// recargar -> conservar nombre, id y jerarquia) y el nombre por defecto de los
// objetos nuevos.
//
// Es la prueba que faltaba: la unica suite de serializacion era
// model-serialization-tests, que cubre el componente Model AISLADO. Sin
// round-trip de escena, un objeto sin nombre o cualquier regresion de
// guardado pasaban sin que nada lo notara.
//
// Tambien cubre el vinculo del Inspector (Settings) con el objeto que muestra:
// es el estado de GUI que mas depende de la vida del objeto, y su ciclo de vida
// se puede verificar headless sin dibujar (borrar el objeto -> el inspector
// tiene que desvincularse).
//
// Patron de autoria (ver Skills): CHECK definido en este archivo,
// TempPruebas::CarpetaPrueba para la carpeta temporal que se limpia sola, y
// salida final "OK/FALLOS: N comprobaciones" saliendo con 0 o 1.

#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <imgui.h>
#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "TempPruebas.h"
#include "../FunshiEngineGL/src/Audio/AudioEngine.h"
#include "../FunshiEngineGL/src/Assets/AssetException.h"
#include "../FunshiEngineGL/src/Assets/Mallas/AssetManager.h"
#include "../FunshiEngineGL/src/Assets/Mallas/Mesh.h"
#include "../FunshiEngineGL/src/Behaviour/ScriptGameObject.h"
#include "../FunshiEngineGL/src/Configuracion/EditorConfig.h"
#include "../FunshiEngineGL/src/Diagnosticos/RegistroDiagnosticos.h"
#include "../FunshiEngineGL/src/Events/EventBus.h"
#include "../FunshiEngineGL/src/Fisicas/BulletPhysicsAdapter.h"
#include "../FunshiEngineGL/src/Fisicas/IPhysicsBackend.h"
#include "../FunshiEngineGL/src/Fisicas/PhysicsEngine.h"
#include "../FunshiEngineGL/src/GUI/ObjetosGUI/SettingsObjectInterface.h"
#include "../FunshiEngineGL/src/GUI/SceneGUI/JerarquiaArbol.h"
#include "../FunshiEngineGL/src/Herramientas/PathUtils.h"
#include "../FunshiEngineGL/src/Objetos/GameObject.h"
#include "../FunshiEngineGL/src/Objetos/GameObjectFactory.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Model.h"
#include "../FunshiEngineGL/src/Objetos/TagRegistry.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Color.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Audio/AudioSource.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Material.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Model.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/EsfereCollider.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Colliders/CubeCollider.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Escena/CameraComponent.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/RigidBody/RigidBody.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Scripting/Script.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/Skybox.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Render/SpriteRenderer.h"
#include "../FunshiEngineGL/src/Scenes/Registro/SceneRegistry.h"
#include "../FunshiEngineGL/src/Objetos/Componentes/Escena/Transform.h"
#include "../FunshiEngineGL/src/Objetos/SimpleObject.h"
#include "../FunshiEngineGL/src/Rendering/Mallas/DibujoModelo.h"
#include "../FunshiEngineGL/src/Rendering/Escena/ProyeccionCamara.h"
#include "../FunshiEngineGL/src/Scenes/Editor/EditorController.h"
#include "../FunshiEngineGL/src/Scenes/Juego/CameraFrameSafety.h"
#include "../FunshiEngineGL/src/Scenes/Serializacion/RutasReescritura.h"
#include "../FunshiEngineGL/src/Scenes/Registro/SceneRegistry.h"
#include "../FunshiEngineGL/src/Scenes/Serializacion/SceneSerializer.h"
#include "../FunshiEngineGL/src/Scenes/Registro/SiembraEscena.h"

namespace fs = std::filesystem;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        ++total;                                                               \
        if (!(cond)) {                                                         \
            ++fallos;                                                          \
            std::cout << "  [FALLO] " << msg << std::endl;                     \
        }                                                                      \
    } while (0)

namespace SceneSerializationTestsDetalle {

struct ResultadoCarga {
    bool fantasma = false;      // hijo llamado "Scene" (ademas de la raiz)
    bool victimaPresente = true; // el hijo real sobrevivio
    std::string aviso;          // lo que dijo std::cerr durante la carga
};

class LoaderDiagnostico : public IMeshLoader {
public:
    int cargas = 0;
    bool falla = true;

    std::shared_ptr<Mesh> load(const std::string& path) override {
        ++cargas;
        if (falla) throw AssetLoadException(path, "fallo controlado del loader");
        auto malla = std::make_shared<Mesh>();
        malla->vertices.emplace_back(0.0f, 0.0f, 0.0f);
        malla->vertices.emplace_back(1.0f, 0.0f, 0.0f);
        malla->vertices.emplace_back(0.0f, 1.0f, 0.0f);
        malla->indices = {0, 1, 2};
        return malla;
    }
};

}  // namespace SceneSerializationTestsDetalle
using namespace SceneSerializationTestsDetalle;

// --- Reparentar preserva la pose y refresca el cuerpo fisico ------------------
// SceneRegistry::reparent movia el nodo en el arbol y cambiaba parentEntity,
// pero no tocaba el transform local: como el local se interpreta contra el
// padre nuevo, el objeto "saltaba" a otra pose mundial. Ademas el btRigidBody
// seguia en la pose vieja y se teletransportaba de vuelta en el siguiente paso
// de simulacion. Ahora el local se reescribe como inverse(mundoPadre) *
// mundoHijo y el cuerpo se reconstruye.
// El contrato de fisicas no tiene comportamiento por defecto: un backend que
// no declare los contactos ni la gravedad no compila.
static_assert(std::is_abstract<IPhysicsBackend>::value,
              "el contrato de fisicas exige implementar todos sus metodos");

class BackendFisicoFalso : public IPhysicsBackend {
public:
    int agregados = 0;
    int quitados = 0;
    RigidBody* ultimo = nullptr;

    void stepSimulation(float) override {}
    void addRigidBody(RigidBody* body) override {
        ++agregados;
        ultimo = body;
    }
    void removeRigidBody(RigidBody*) override { ++quitados; }

    std::vector<EventoContacto> tomarEventosContacto() override { return {}; }
    void fijarGravedad(float, float, float) override {}
    void gravedad(float& x, float& y, float& z) const override {
        x = 0.0f;
        y = -1.0f;
        z = 0.0f;
    }
    // Sin impacto por defecto: el test no ejerce raycast.
    ResultadoRaycast raycast(float, float, float, float, float, float,
                             float) override {
        return {};
    }
};

// Estado de la unidad original: una sola instancia (se define en SceneSerializationTests.cpp).

namespace SceneSerializationTestsDetalle {

extern int total;
extern int fallos;

}  // namespace SceneSerializationTestsDetalle
using namespace SceneSerializationTestsDetalle;

// Funciones repartidas entre las unidades.

void nombresPorDefecto();
void restaurarBaselineDeEscena();
void guardadoConArbolVacio();
void roundTripDeEscena();
void tagsCompatiblesConEscenasAnteriores();
void reporteFalloBinario();
void indiceCorruptoSinFantasmas();
void diagnosticosCargaEscena();
void diagnosticosCargaMalla();
void diagnosticosCargaScript();
void hijoConIdCeroSeReasignaAlGuardar();
void hermanosConsecutivosSinPerdida();
std::string leerRutaPersistida(std::istream& in);
void escribirRutaPersistida(std::ostream& out, const std::string& s);
void rutasDeCarasDeSkybox();
void reescrituraDeReferencias();
void sanadoDeRutasRotas();
void variosScriptsPorObjeto();
void nombreDeComponenteScriptSeSerializa();
void rotacionApiUsaRadianes();
void elInspectorSeDesvinculaAlBorrar();
void laCamaraBorradaNoSeUsaEnElRestoDelFrame();
void borrarCrearBorrarNoDesalineaElBinario();
void elModeloSeResuelveConLaMatrizMundial();
void elEventoDePropiedadNoReconstruyeElInspector();
void elInspectorDibujaUnFrameSinArrastre();
void reparentarPreservaLaPoseYRefrescaElCuerpo();
void reparentarSobreviveElGuardado();
void opcionesDeRenderYFisicaSePersisten();
void movimientoFisicoYSaltoDesdeScripts();
void eventosDeContactoExponenColliderYPropietario();
void busquedaDeObjetosPorNombreIdYEtiqueta();
void propiedadesFisicasDesdeScripts();
void rigidBodySerializaPropiedadesNuevas();
void sensorDetectaSinRespuestaYPlano2D();
void gestionDeObjetosDesdeScripts();
void siembraPorDimension();
void proyeccionDeCamara();
void spriteRendererSerializaRoundTrip();
void musicaDesdeScripts();

#endif
