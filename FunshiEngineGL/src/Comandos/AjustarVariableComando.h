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
#ifndef AJUSTAR_VARIABLE_COMANDO_H
#define AJUSTAR_VARIABLE_COMANDO_H

#include "IComando.h"
#include <functional>
#include <string>

class EditorController;
class SceneRegistry;

// Ajusta una variable de un componente de un objeto y sabe devolver el valor
// que habia antes, para que el ajuste se pueda deshacer.
//
// El acceso al componente se inyecta: el comando no conoce el catalogo de
// propiedades, asi que el modulo de comandos no depende del de inspeccion
// (el camino va en un sentido: inspeccion -> comandos).
class AjustarVariableComando : public IComando {
public:
    // Devuelve el valor actual de la variable en forma de texto.
    using Lector = std::function<std::string(void*)>;
    // Interpreta el texto y lo escribe. Devuelve false con el motivo si el
    // motor no acepta el valor.
    using Escritor = std::function<bool(void*, const std::string&, std::string&)>;

    AjustarVariableComando(EditorController* ec, SceneRegistry* sr, int idObjeto,
                            std::string nombreComponente, std::string nombreVariable,
                            Lector lector, Escritor escritor, std::string valorNuevo);
    ~AjustarVariableComando() override = default;

    void ejecutar() override;
    void deshacer() override;
    std::string descripcion() const override;

    // Valor que tenia la variable antes del ajuste.
    const std::string& valorAnterior() const noexcept { return valorAnterior_; }

    // Error de la ultima escritura (vacio si fue correcta). Sirve para
    // saber si el motor rechazo el valor.
    const std::string& ultimoError() const noexcept { return ultimoError_; }

private:
    void escribir(const std::string& valor);

    EditorController* editorController;
    SceneRegistry* sceneRegistry;
    int objectId;
    std::string nombreComponente;
    std::string nombreVariable;
    Lector lector;
    Escritor escritor;
    std::string valorNuevo;
    std::string valorAnterior_;
    std::string ultimoError_;
};

#endif
