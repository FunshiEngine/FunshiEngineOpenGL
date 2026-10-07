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
#ifndef SCRIPT_H
#define SCRIPT_H

#include "../../Behaviour/ComportamientoCargado.h"
#include "../../Fisicas/ContactoFisico.h"
#include "Component.h"
#include <string>
#include <utility>
#include <vector>

// Componente de comportamiento de usuario. Apunta a un fuente (.cpp o .java);
// el backend correspondiente lo compila a un artefacto y lo carga en caliente
// (hot reload por mtime del fuente). EJECUTA el ciclo onStart/onUpdate/onStop.
class Script : public Component {
private:
    std::string dllPath; // fuente del script (.cpp / .java)
    std::string nameClass;
    std::string nombreComponente_;
    ComportamientoCargado comportamiento_;
    std::vector<ReflejoScripts::DefCampo> camposInspector_;
    std::vector<ReflejoScripts::ValorCampo> valores_;
    bool cargado_ = false;  // ya se intento cargar el fuente actual
    bool arrancado_ = false; // onStart ya fue invocado (en play mode)
    // En play mode la recompilacion la agenda GameScene (cola con progreso
    // visible en la barra de estado): mientras esta en cola, actualizar() no
    // compila inline y deja que la cola lo haga.
    bool aplazarCarga_ = false;
    std::string error_;

    void extraerValores();
    bool cargarActual(std::string& error);

protected:
    void serializeComponent(std::ofstream* fileNamePathContentObject) override;
    void deserializeComponent(std::ifstream* fileNamePathContentObject) override;

public:
    // API publica historica (GUI existente de drag & drop)
    void setDllPath(std::string dllPath);
    std::string getPath() { return dllPath; }
    std::string getNameClass() { return nameClass; }
    const std::string& getNombreComponente() const noexcept {
        return nombreComponente_;
    }
    void setNombreComponente(std::string nombre) {
        nombreComponente_ = std::move(nombre);
    }
    std::string nombreParaMostrar() const {
        if (!nombreComponente_.empty()) return nombreComponente_;
        return nameClass.empty() ? "Script" : nameClass;
    }

    // Carga/ejecucion
    void actualizar(GameObject* owner, float deltaTime);
    void detener(GameObject* owner);
    void notificarContacto(GameObject* owner, Collider* propio,
                           Collider* otro, TipoContacto tipo);
    void recargar(GameObject* owner); // forza recompilar + recargar
    void cargarSiNecesario();         // carga (compila) sin arrancar onStart

    // Apagado del motor: libera backend y referencias globales JNI SIN
    // disparar onStop (si estaba en play, detener() ya hizo el onStop).
    void liberarComportamiento();

    // Cola de compilacion de GameScene (play mode): true si el fuente actual
    // requiere compilarse; aplicarCarga ejecuta esa carga/recompilacion.
    bool necesitaCompilar() const;
    bool aplicarCarga(GameObject* owner);
    void setAplazarCarga(bool aplazar) { aplazarCarga_ = aplazar; }
    bool getAplazarCarga() const { return aplazarCarga_; }
    const std::string& rutaFuente() const { return dllPath; }

    // SerializeField: campos reflejados del comportamiento (si esta cargado)
    const std::vector<ReflejoScripts::DefCampo>& obtenerCampos() const {
        return camposInspector_;
    }
    std::vector<ReflejoScripts::ValorCampo>& obtenerValores() { return valores_; }
    const std::vector<ReflejoScripts::ValorCampo>& obtenerValores() const {
        return valores_;
    }
    void escribirCampo(int indice, const ReflejoScripts::ValorCampo& valor);
    bool estaCargado() const { return comportamiento_.valido(); }
    bool estaEnPlay() const { return arrancado_; }
    std::string ultimoError() const { return error_; }

    void saveComponent(std::ofstream* fileNamePathContentObject) override;
    void loadComponent(std::ifstream* fileNamePathContentObject) override;
};
#endif