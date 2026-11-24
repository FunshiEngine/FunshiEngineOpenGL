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
#include "ScriptDetalle.h"

void Script::setDllPath(std::string dllPath) {
    this->dllPath = dllPath; // guarda el path completo del fuente
    camposInspector_.clear();

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
    error_.clear();
    if (!cargarActual(error_)) {
        EventoDiagnostico evento;
        evento.nivel = NivelDiagnostico::Error;
        evento.modulo = "Scripts";
        evento.tipoExcepcion = "SCRIPT_LOAD_FAILED";
        evento.mensaje = "No se pudo cargar '" + dllPath + "': " + error_;
        if (diagnosticos_)
            diagnosticos_->registrar(std::move(evento));
        else
            RegistroDiagnosticos::global().registrar(std::move(evento));
        return;
    }
    camposInspector_ = metadatosCampos(comportamiento_.campos);

    // Servicios de escena (audio, busqueda, teclado): la instancia C++ recibe
    // la tabla aca; Java consulta las mismas tablas desde sus metodos nativos
    // Nativo.*. GameScene cablea el contexto real al entrar en Play, antes del
    // primer onStart.
    if (comportamiento_.lenguaje == "cpp" && comportamiento_.instancia)
        static_cast<IScriptBehaviour*>(comportamiento_.instancia)->servicios =
            MotorScript::tablaServicios();

    if (!comportamiento_.campos.empty()) {
        // Emparejar por NOMBRE el arbol leido del .escena con los campos que
        // expone el script recien compilado, ANTES de inyectarlo. Sin esto el
        // cardinal de `valores_` era el que tuviera el archivo, no el de la
        // reflexion: una escena guardada antes de agregar un SerializeField
        // dejaba `valores_` mas corto que `campos` y el inspector leia fuera de
        // rango al editar (ver SettingsScript). Solo se llenaba cuando
        // `valores_` estaba COMPLETAMENTE vacio, que es un caso particular:
        // cualquier desajuste parcial pasaba.
        valores_ = ReflejoScripts::alinearValores(std::move(valores_),
                                                  comportamiento_.campos);
    }

    // Restaurar los valores de SerializeField persistidos en la escena sobre
    // la instancia recien compilada (reemplazos en caliente o editados).
    ScriptRuntime::inyectar(comportamiento_, valores_);
}

void Script::extraerValores() {
    if (comportamiento_.valido())
        valores_ = ScriptRuntime::extraer(comportamiento_);
}

void Script::recargar(GameObject* owner) {
    extraerValores();
    if (arrancado_) {
        ScriptRuntime::llamarDetener(comportamiento_, owner);
        if (comportamiento_.falloRuntime)
            error_ = comportamiento_.errorRuntime;
    }
    ScriptRuntime::descargar(comportamiento_);
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
    if (!comportamiento_.valido() || comportamiento_.falloRuntime) {
        if (comportamiento_.falloRuntime) error_ = comportamiento_.errorRuntime;
        return;
    }

    // Hot reload: si el fuente cambio en disco se recompila y se recarga.
    if (ScriptRuntime::cambioElFuente(comportamiento_)) recargar(owner);
    if (!comportamiento_.valido()) return;

    if (!arrancado_) {
        ScriptRuntime::llamarInicio(comportamiento_, owner);
        arrancado_ = true;
    }
    if (comportamiento_.falloRuntime) {
        error_ = comportamiento_.errorRuntime;
        return;
    }
    ScriptRuntime::llamarActualizar(comportamiento_, owner, deltaTime);
    if (comportamiento_.falloRuntime) error_ = comportamiento_.errorRuntime;
}

void Script::detener(GameObject* owner) {
    if (!arrancado_) return;
    ScriptRuntime::llamarDetener(comportamiento_, owner);
    if (comportamiento_.falloRuntime) error_ = comportamiento_.errorRuntime;
    arrancado_ = false;
    extraerValores(); // que la GUI conserve los ultimos valores editados
}

void Script::notificarContacto(GameObject* owner, Collider* propio,
                               Collider* otro, TipoContacto tipo) {
    if (!arrancado_ || !comportamiento_.valido()) return;
    ScriptRuntime::llamarContacto(comportamiento_, owner, propio, otro, tipo);
    if (comportamiento_.falloRuntime) error_ = comportamiento_.errorRuntime;
}

void Script::liberarComportamiento() {
    if (!comportamiento_.cargado) return;
    camposInspector_ = metadatosCampos(comportamiento_.campos);
    ScriptRuntime::descargar(comportamiento_); // sin owner: no dispara onStop
    cargado_ = false;
    arrancado_ = false;
    aplazarCarga_ = false;
}
