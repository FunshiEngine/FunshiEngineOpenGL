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
#ifndef APIESCENA_H
#define APIESCENA_H

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace MotorInspeccion {

// Version del contrato de la API de inspeccion.
//
// Es independiente de `versionRuntimeScript` (la clave de la cache de scripts
// compilados): subir esta version no invalida la cache de los scripts de
// ningun proyecto. La CLI consulta este numero al arrancar y lo compara con
// el suyo.
constexpr int versionApiEscena = 1;

// Sufijo del archivo indice que el serializador escribe al lado de la carpeta
// de la escena. La escena se guarda con un PREFIJO de ruta
// ("proyecto/MiEscena"), no con un nombre de archivo: el indice queda en
// "proyecto/MiEscenaBBDDObjetos.txt" y los binarios de cada objeto en la
// carpeta "proyecto/MiEscena/".
constexpr const char* indiceEscena = "BBDDObjetos.txt";

// Filtro combinable sobre los objetos de la escena. Los campos vacios no
// filtran: se pueden usar varios a la vez (se acumulan).
struct FiltroObjetos {
    // Tipo de componente que debe tener el objeto ("RigidBody", "Light"...).
    // Se aceptan el nombre canonico y los alias del catalogo.
    std::optional<std::string> componente;
    // Etiqueta (tag) exacta del objeto.
    std::optional<std::string> etiqueta;
    // Nombre del objeto. Exacto o parcial, segun `nombreParcial`.
    std::optional<std::string> nombre;
    bool nombreParcial = false;
    // Id exacto del objeto.
    std::optional<int> id;
    // Tipo del objeto: "SimpleObject", "Modelos3D", "GameObject"...
    std::optional<std::string> tipoObjeto;
    // Maximo de objetos devueltos. 0 = sin limite.
    std::size_t limite = 0;
};

// Resumen de un objeto de la escena.
struct VistaObjeto {
    int id = 0;
    std::string nombre;
    std::string etiqueta;
    std::string tipoObjeto;
    int idPadre = 0;
    std::array<float, 3> posicionGlobal{0.0f, 0.0f, 0.0f};
    std::array<float, 3> posicionLocal{0.0f, 0.0f, 0.0f};
    std::size_t cantidadComponentes = 0;
};

// Una variable de un componente, con su tipo y su valor actual en texto.
struct VistaVariable {
    std::string nombre;
    std::string tipo;
    std::string valor;
};

// Un componente de un objeto y sus variables.
struct VistaComponente {
    std::string tipo;
    std::string etiqueta;
    std::vector<VistaVariable> variables;
};

// Contrato publico del motor para inspeccionar y modificar una escena sin
// GUI. La CLI (proyecto aparte) lo consume.
//
// Toda mutacion pasa por el gestor de comandos del editor, asi que se puede
// deshacer igual que en el editor. Los metodos que modifican devuelven false
// y dejan el motivo en `error` cuando la operacion no se puede aplicar; en
// ese caso no queda ninguna mutacion a medias.
class IApiEscena {
public:
    virtual ~IApiEscena() = default;

    // Objetos de la escena que cumplen el filtro.
    virtual std::vector<VistaObjeto> listarObjetos(const FiltroObjetos& filtro) = 0;

    // Un objeto por id. Devuelve std::nullopt si no existe.
    virtual std::optional<VistaObjeto> objetoPorId(int id) = 0;

    // Componentes de un objeto con sus variables y valores. La lista esta
    // vacia si el objeto no existe.
    virtual std::vector<VistaComponente> componentesDe(int idObjeto) = 0;

    // Tipos de componente que se pueden agregar, con su nombre canonico.
    virtual std::vector<std::string> tiposDeComponente() = 0;

    // Crea un objeto simple en la escena. `idPadre` = 0 lo deja en la raiz.
    // Devuelve el id del objeto creado en `idCreado`.
    virtual bool crearObjeto(const std::string& nombre, int idPadre, int& idCreado,
                             std::string& error) = 0;

    // Fija la etiqueta (tag) de un objeto.
    virtual bool establecerEtiqueta(int idObjeto, const std::string& etiqueta,
                                    std::string& error) = 0;

    // Agrega un componente del tipo indicado, respetando los prerrequisitos
    // del catalogo de componentes.
    virtual bool crearComponente(int idObjeto, const std::string& tipo,
                                 std::string& error) = 0;

    // Quita el componente del tipo indicado.
    virtual bool quitarComponente(int idObjeto, const std::string& tipo,
                                  std::string& error) = 0;

    // Escribe una variable de un componente. El valor es texto: lo interpreta
    // el tipo de la variable (numero, booleano, vector o valor de enumeracion).
    virtual bool asignarVariable(int idObjeto, const std::string& tipoComponente,
                                 const std::string& variable,
                                 const std::string& valor,
                                 std::string& error) = 0;

    // Posicion global de un objeto (la compuesta con la de sus padres).
    virtual bool posicionGlobal(int idObjeto, std::array<float, 3>& xyz) = 0;

    // Guarda la escena. `prefijo` es el prefijo de la escena (vease
    // `indiceEscena`): el indice queda en "<prefijo>BBDDObjetos.txt" y los
    // binarios de cada objeto en la carpeta "<prefijo>/", que se crea si no
    // existe.
    virtual bool guardarEscena(const std::string& prefijo, std::string& error) = 0;

    // Carga la escena del mismo prefijo con el que se guardo.
    virtual bool cargarEscena(const std::string& prefijo, std::string& error) = 0;

    // Version del contrato (vease `versionApiEscena`).
    virtual int version() const = 0;
};

}  // namespace MotorInspeccion

#endif
