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
#include "Script.h"

#include "../Behaviour/ComportamientoCargado.h"
#include "../Behaviour/IScriptBehaviour.h"
#include "../Behaviour/ScriptGameObject.h"
#include "../Behaviour/ScriptRuntime.h"
#include "Configuracion/EditorConfig.h"
#include "../Objetos/GameObject.h"

namespace {
constexpr uint32_t MAGIC_SCRIPT = 0x31535346; // 'F','S','S','1'
constexpr uint32_t VERSION_SCRIPT = 1;
} // namespace

void Script::setDllPath(std::string dllPath) {
    this->dllPath = dllPath; // guarda el path completo del fuente

    size_t lastSlash = dllPath.find_last_of("/\\");
    size_t lastDot = dllPath.find_last_of('.');

    bool esFuenteValida = !dllPath.empty() &&
                          (dllPath.find(".cpp") != std::string::npos ||
                           dllPath.find(".java") != std::string::npos);

    if (lastSlash != std::string::npos && lastDot != std::string::npos &&
        lastDot > lastSlash && esFuenteValida) {
        this->nameClass =
            dllPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        this->nameClass = "Debe ser <ClassName>.cpp o .java";
    }

    // El fuente cambio: invalidar lo cargado para que recompile en play mode.
    comportamiento_ = ComportamientoCargado{};
    cargado_ = false;
    arrancado_ = false;
    error_.clear();
}

bool Script::cargarActual(std::string& error) {
    if (dllPath.empty()) {
        error = "No hay fuente de script asignado.";
        return false;
    }
    return ScriptRuntime::compilarYCargar(dllPath, nameClass, comportamiento_,
                                          error);
}

void Script::cargarSiNecesario() {
    if (cargado_ || dllPath.empty()) return;
    cargado_ = true;
    if (!cargarActual(error_)) return;

    // Servicios de escena (audio, busqueda, teclado): la tabla global se
    // entrega aca, en el motor (los tests de scripts usan su propio stub de
    // tablaApi y no enlazan ScriptGameObject.cpp). El contexto real de la
    // escena lo cablea GameScene via inyectarServiciosScript al entrar en
    // Play, antes del primer onStart. Solo para C++: en Java `instancia` es
    // un jobject, no un IScriptBehaviour*.
    if (comportamiento_.lenguaje == "cpp" && comportamiento_.instancia)
        static_cast<IScriptBehaviour*>(comportamiento_.instancia)->servicios =
            MotorScript::tablaServicios();

    // Restaurar los valores de SerializeField persistidos en la escena sobre
    // la instancia recien compilada (reemplazos en caliente o editados).
    ScriptRuntime::inyectar(comportamiento_, valores_);
    if (!comportamiento_.campos.empty() && valores_.empty())
        valores_ = ReflejoScripts::valoresPorDefecto(comportamiento_.campos);
}

void Script::extraerValores() {
    if (comportamiento_.valido())
        valores_ = ScriptRuntime::extraer(comportamiento_);
}

void Script::recargar(GameObject* owner) {
    extraerValores();
    ScriptRuntime::descargar(comportamiento_, owner); // llama onStop si invalido
    cargado_ = false;
    arrancado_ = false;
    cargarSiNecesario();
}

bool Script::necesitaCompilar() const {
    return !dllPath.empty() &&
           (!cargado_ || ScriptRuntime::cambioElFuente(comportamiento_));
}

bool Script::aplicarCarga(GameObject* owner) {
    if (!dllPath.empty() && !cargado_) {
        cargarSiNecesario();
        return comportamiento_.valido();
    }
    if (ScriptRuntime::cambioElFuente(comportamiento_)) recargar(owner);
    return comportamiento_.valido();
}

void Script::actualizar(GameObject* owner, float deltaTime) {
    if (dllPath.empty()) return;

    // La compilacion esta en la cola de GameScene: no compilar inline aca, la
    // cola aplica la carga en su turno (progreso visible en la barra).
    if (aplazarCarga_ && necesitaCompilar()) return;

    if (!cargado_) cargarSiNecesario();
    if (!comportamiento_.valido()) return;

    // Hot reload: si el fuente cambio en disco se recompila y se recarga.
    if (ScriptRuntime::cambioElFuente(comportamiento_)) recargar(owner);
    if (!comportamiento_.valido()) return;

    if (!arrancado_) {
        ScriptRuntime::llamarInicio(comportamiento_, owner);
        arrancado_ = true;
    }
    ScriptRuntime::llamarActualizar(comportamiento_, owner, deltaTime);
}

void Script::detener(GameObject* owner) {
    if (!arrancado_) return;
    ScriptRuntime::llamarDetener(comportamiento_, owner);
    arrancado_ = false;
    extraerValores(); // que la GUI conserve los ultimos valores editados
}

void Script::liberarComportamiento() {
    if (!comportamiento_.cargado) return;
    ScriptRuntime::descargar(comportamiento_); // sin owner: no dispara onStop
    cargado_ = false;
    arrancado_ = false;
    aplazarCarga_ = false;
}

void Script::escribirCampo(int indice,
                           const ReflejoScripts::ValorCampo& valor) {
    if (indice < 0 || indice >= static_cast<int>(valores_.size())) return;
    valores_[indice] = valor;
    // Si la instancia esta viva (play mode) reflejar el cambio inmediato.
    // Se despacha por backend: en Java los DefCampo no exponen `acceder` (las
    // variables viven en la JVM), invocar ReflejoScripts::escribirCampo directo
    // lanzaria std::bad_function_call al mover un slider.
    if (comportamiento_.valido()) ScriptRuntime::inyectar(comportamiento_, valores_);
}

void Script::serializeComponent(std::ofstream* f) {
    // 1. Magic + version para distinguir el formato nuevo (con SerializeField)
    uint32_t magic = MAGIC_SCRIPT;
    uint32_t version = VERSION_SCRIPT;
    f->write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    f->write(reinterpret_cast<const char*>(&version), sizeof(version));

    // 2. Campos historicos: path + nombre de clase. La ruta del fuente se
    // persiste relativa a la raiz de assets del proyecto (si esta fijada): los
    // fuentes viven bajo src<nombre>/Scripts, y asi la escena sigue valida al
    // mover/renombrar el proyecto entero.
    const std::string pathGuardado = EditorConfig::relativizarRuta(dllPath);
    size_t pathLength = pathGuardado.size();
    f->write(reinterpret_cast<const char*>(&pathLength), sizeof(size_t));
    f->write(pathGuardado.c_str(), pathLength);

    size_t nameLength = nameClass.size();
    f->write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
    f->write(nameClass.c_str(), nameLength);

    // 3. Valores de SerializeField (arbol autodescriptivo)
    extraerValores();
    ReflejoScripts::guardarValoresCampos(*f, valores_);
}

void Script::deserializeComponent(std::ifstream* f) {
    std::streampos inicio = f->tellg();

    uint32_t magic = 0;
    f->read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic == MAGIC_SCRIPT) {
        uint32_t version = 0;
        f->read(reinterpret_cast<char*>(&version), sizeof(version));

        size_t pathLength = 0;
        f->read(reinterpret_cast<char*>(&pathLength), sizeof(size_t));
        dllPath.resize(pathLength);
        f->read(&dllPath[0], pathLength);
        // Escena nueva: relativa a la raiz de assets; legacy: absoluta intacta.
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        f->read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
        nameClass.resize(nameLength);
        f->read(&nameClass[0], nameLength);

        if (version >= 1)
            valores_ = ReflejoScripts::cargarValoresCampos(*f);

        setDllPath(dllPath); // valida nombre clase + invalida lo cargado
    } else {
        // Formato legacy: solo path + nombre de clase (sin magic).
        f->seekg(inicio); // rebobinar para releer por el camino viejo

        size_t pathLength = 0;
        f->read(reinterpret_cast<char*>(&pathLength), sizeof(size_t));
        dllPath.resize(pathLength);
        f->read(&dllPath[0], pathLength);
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        f->read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
        nameClass.resize(nameLength);
        f->read(&nameClass[0], nameLength);
        setDllPath(dllPath);
    }

    cargado_ = false;
    comportamiento_ = ComportamientoCargado{};
}

void Script::saveComponent(std::ofstream* fileNamePathContentObject) {
    serializeComponent(fileNamePathContentObject);
}

void Script::loadComponent(std::ifstream* fileNamePathContentObject) {
    deserializeComponent(fileNamePathContentObject);
}