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
#ifndef INSPECCIONESCENA_H
#define INSPECCIONESCENA_H

#include <string>

#include "ApiEscena.h"

class EditorController;
class SceneRegistry;
class SceneSerializer;
class GameObject;

namespace MotorInspeccion {

// Implementacion de la API de inspeccion sobre el registro de objetos y el
// controlador del editor.
//
// No depende de la GUI ni de la pila grafica: se puede usar en un proceso
// sin ventana (la CLI, o una prueba headless).
//
// Las escenas, el controlador y el serializador se inyectan: la API no los
// crea ni los destruye.
class InspeccionEscena : public IApiEscena {
public:
    InspeccionEscena(SceneRegistry* escena, EditorController* editor,
                     SceneSerializer* serializador = nullptr);
    ~InspeccionEscena() override = default;

    std::vector<VistaObjeto> listarObjetos(const FiltroObjetos& filtro) override;
    std::optional<VistaObjeto> objetoPorId(int id) override;
    std::vector<VistaComponente> componentesDe(int idObjeto) override;
    std::vector<std::string> tiposDeComponente() override;
    bool crearObjeto(const std::string& nombre, int idPadre, int& idCreado,
                     std::string& error) override;
    bool establecerEtiqueta(int idObjeto, const std::string& etiqueta,
                            std::string& error) override;
    bool crearComponente(int idObjeto, const std::string& tipo,
                          std::string& error) override;
    bool quitarComponente(int idObjeto, const std::string& tipo,
                           std::string& error) override;
    bool asignarVariable(int idObjeto, const std::string& tipoComponente,
                          const std::string& variable, const std::string& valor,
                          std::string& error) override;
    bool posicionGlobal(int idObjeto, std::array<float, 3>& xyz) override;
    bool guardarEscena(const std::string& prefijo, std::string& error) override;
    bool cargarEscena(const std::string& prefijo, std::string& error) override;
    int version() const override;

private:
    SceneRegistry* escena_;
    EditorController* editor_;
    SceneSerializer* serializador_;

    GameObject* objetoPorIdInterno(int id) const;

    // Resumen de un objeto (id, nombre, etiqueta, posiciones y componentes).
    VistaObjeto vistaDe(GameObject* objeto) const;

    // Nombre canonico de un tipo de componente (resuelve los alias del
    // catalogo). Devuelve una cadena vacia si el tipo no existe.
    std::string canonicalizarTipo(const std::string& tipo) const;
};

}  // namespace MotorInspeccion

#endif
