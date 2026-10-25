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
#include "InspeccionEscena.h"

#include "PropiedadesComponentes.h"

#include "../Comandos/AgregarComponenteComando.h"
#include "../Comandos/AjustarVariableComando.h"
#include "../Comandos/CrearObjetoComando.h"
#include "../Comandos/EstablecerTagComando.h"
#include "../Comandos/QuitarComponenteComando.h"
#include "../Entity/Entity.h"
#include "../Herramientas/TypeUtils.h"
#include "../Objetos/Componentes/CatalogoComponentes.h"
#include "../Objetos/Componentes/Component.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/GameObjectFactory.h"
#include "../Scenes/EditorController.h"
#include "../Scenes/SceneRegistry.h"
#include "../Scenes/SceneSerializer.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>

namespace MotorInspeccion {

namespace {

std::string minusculas(const std::string& texto) {
    std::string salida = texto;
    std::transform(salida.begin(), salida.end(), salida.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return salida;
}

// El nombre se compara sin distinguir mayusculas: el filtro por nombre
// exacto y el parcial usan el mismo criterio.
bool coincideNombre(const std::string& texto, const std::string& patron,
                     bool parcial) {
    if (parcial)
        return minusculas(texto).find(minusculas(patron)) != std::string::npos;
    return minusculas(texto) == minusculas(patron);
}

}  // namespace

InspeccionEscena::InspeccionEscena(SceneRegistry* escena, EditorController* editor,
                                   SceneSerializer* serializador)
    : escena_(escena), editor_(editor), serializador_(serializador) {}

GameObject* InspeccionEscena::objetoPorIdInterno(int id) const {
    if (!escena_ || id <= 0) return nullptr;
    return escena_->getObjectByID(id);
}

std::string InspeccionEscena::canonicalizarTipo(const std::string& tipo) const {
    const DescriptorComponente* descriptor =
        CatalogoComponentes::instancia().buscar(tipo);
    if (!descriptor) return std::string();
    return descriptor->nombre;
}

VistaObjeto InspeccionEscena::vistaDe(GameObject* objeto) const {
    VistaObjeto vista;
    vista.id = objeto->getId();
    vista.nombre = objeto->inputName;
    vista.etiqueta = objeto->getTag();
    vista.tipoObjeto = nombreSimpleTipo(typeid(*objeto));
    if (Entity* padre = objeto->getParentEntity()) {
        if (GameObject* padreObjeto = dynamic_cast<GameObject*>(padre))
            vista.idPadre = padreObjeto->getId();
    }
    if (Transform* local = objeto->getComponent<Transform>()) {
        const float* posicionLocal = local->getTranslatef();
        vista.posicionLocal = {posicionLocal[0], posicionLocal[1],
                               posicionLocal[2]};
    }
    // La transformacion global se calcula en un buffer reutilizable del
    // objeto: hay que copiarla antes de seguir.
    if (Transform* global = objeto->getGlobalTransform()) {
        const float* posicionGlobal = global->getTranslatef();
        vista.posicionGlobal = {posicionGlobal[0], posicionGlobal[1],
                                posicionGlobal[2]};
    }
    if (auto* componentes = objeto->getComponents()) {
        vista.cantidadComponentes = static_cast<std::size_t>(componentes->tam());
    }
    return vista;
}

std::vector<VistaObjeto> InspeccionEscena::listarObjetos(
    const FiltroObjetos& filtro) {
    std::vector<VistaObjeto> resultado;
    if (!escena_) return resultado;

    // Un filtro por componente que no existe no puede cumplirse: se resuelve
    // antes de recorrer para no repetir la busqueda por cada objeto.
    std::string tipoComponente;
    if (filtro.componente) {
        tipoComponente = canonicalizarTipo(*filtro.componente);
        if (tipoComponente.empty()) return resultado;
    }
    std::string tipoObjeto;
    if (filtro.tipoObjeto) tipoObjeto = nombreSimpleTipo(*filtro.tipoObjeto);

    escena_->refreshGameObjectView();
    auto* lista = escena_->getGameObjects();
    if (!lista || lista->isEmpty()) return resultado;

    auto* posicion = lista->first();
    while (posicion) {
        GameObject* objeto = posicion->getElement();
        posicion = (posicion != lista->last()) ? lista->next(posicion) : nullptr;
        if (!objeto) continue;

        if (filtro.id && objeto->getId() != *filtro.id) continue;
        if (filtro.etiqueta && objeto->getTag() != *filtro.etiqueta) continue;
        if (filtro.nombre &&
            !coincideNombre(objeto->inputName, *filtro.nombre, filtro.nombreParcial))
            continue;
        if (!tipoObjeto.empty() && nombreSimpleTipo(typeid(*objeto)) != tipoObjeto)
            continue;
        if (!tipoComponente.empty() &&
            !objeto->getComponentByName(tipoComponente))
            continue;

        VistaObjeto vista = vistaDe(objeto);

        resultado.push_back(vista);
        if (filtro.limite > 0 && resultado.size() >= filtro.limite) break;
    }
    return resultado;
}

std::optional<VistaObjeto> InspeccionEscena::objetoPorId(int id) {
    GameObject* objeto = objetoPorIdInterno(id);
    if (!objeto) return std::nullopt;
    return vistaDe(objeto);
}

std::vector<VistaComponente> InspeccionEscena::componentesDe(int idObjeto) {
    std::vector<VistaComponente> resultado;
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) return resultado;
    auto* componentes = objeto->getComponents();
    if (!componentes || componentes->isEmpty()) return resultado;

    auto* posicion = componentes->first();
    while (posicion) {
        Component* componente = posicion->getElement();
        posicion = (posicion != componentes->last())
                       ? componentes->next(posicion)
                       : nullptr;
        if (!componente) continue;

        VistaComponente vista;
        vista.tipo = nombreSimpleTipo(typeid(*componente));
        const DescriptorComponente* descriptor =
            CatalogoComponentes::instancia().buscar(vista.tipo);
        vista.etiqueta = descriptor ? descriptor->etiqueta : vista.tipo;

        const std::vector<DefPropiedad>* propiedades =
            CatalogoPropiedades::instancia().de(vista.tipo);
        if (propiedades) {
            for (const DefPropiedad& propiedad : *propiedades) {
                VistaVariable variable;
                variable.nombre = propiedad.nombre;
                variable.tipo = nombreDeTipoValor(propiedad.tipo);
                variable.valor = propiedad.leer ? propiedad.leer(componente)
                                                : std::string();
                vista.variables.push_back(variable);
            }
        }
        resultado.push_back(vista);
    }
    return resultado;
}

std::vector<std::string> InspeccionEscena::tiposDeComponente() {
    std::vector<std::string> resultado;
    for (const DescriptorComponente& descriptor :
         CatalogoComponentes::instancia().descriptores()) {
        resultado.push_back(descriptor.nombre);
    }
    return resultado;
}

bool InspeccionEscena::crearObjeto(const std::string& nombre, int idPadre,
                                   int& idCreado, std::string& error) {
    idCreado = -1;
    if (!escena_ || !editor_) {
        error = "no hay escena cargada";
        return false;
    }
    GameObject* padre = nullptr;
    if (idPadre > 0) {
        padre = escena_->getObjectByID(idPadre);
        if (!padre) {
            error = "no existe el objeto padre " + std::to_string(idPadre);
            return false;
        }
    }
    std::unique_ptr<GameObject> objeto =
        GameObjectFactory::createSimpleObject(escena_->getRoot(), nombre);

    auto comando = std::make_unique<CrearObjetoComando>(editor_, escena_,
                                                        std::move(objeto), padre);
    // El comando pasa al historial del gestor, pero sigue vivo mientras el
    // historial lo tenga: se lee su id de inmediato, sin que nada mas lo
    // toque.
    CrearObjetoComando* creado = comando.get();
    editor_->getGestorComandos()->ejecutar(std::move(comando));
    idCreado = creado->idCreado();
    if (idCreado <= 0) {
        error = "no se pudo crear el objeto";
        return false;
    }
    return true;
}

bool InspeccionEscena::establecerEtiqueta(int idObjeto,
                                           const std::string& etiqueta,
                                           std::string& error) {
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) {
        error = "no existe el objeto " + std::to_string(idObjeto);
        return false;
    }
    if (!editor_) {
        error = "no hay editor";
        return false;
    }
    editor_->getGestorComandos()->ejecutar(
        std::make_unique<EstablecerTagComando>(editor_, escena_, objeto, etiqueta));
    return true;
}

bool InspeccionEscena::crearComponente(int idObjeto, const std::string& tipo,
                                        std::string& error) {
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) {
        error = "no existe el objeto " + std::to_string(idObjeto);
        return false;
    }
    const DescriptorComponente* descriptor =
        CatalogoComponentes::instancia().buscar(tipo);
    if (!descriptor) {
        error = "no existe el componente '" + tipo + "'";
        return false;
    }
    // Un objeto no puede tener dos componentes del mismo tipo: el motor
    // busca el primero (con dos Transform, el render se quedaria con el
    // primero y la escena quedaria desincronizada). Todos los objetos ya
    // nacen con un Transform.
    if (objeto->getComponentByName(descriptor->nombre)) {
        error = "el objeto " + std::to_string(idObjeto) +
                " ya tiene el componente " + descriptor->nombre;
        return false;
    }
    if (descriptor->prerequisito && !descriptor->prerequisito(*objeto)) {
        error = "el componente " + descriptor->nombre + " " +
                (descriptor->requisito.empty() ? std::string("tiene requisitos que este objeto no cumple")
                                                : descriptor->requisito);
        return false;
    }
    std::unique_ptr<Component> componente = descriptor->crear(*objeto);
    if (!componente) {
        error = "no se pudo crear el componente " + descriptor->nombre;
        return false;
    }
    editor_->getGestorComandos()->ejecutar(
        std::make_unique<AgregarComponenteComando>(editor_, objeto,
                                                   std::move(componente), escena_));
    return true;
}

bool InspeccionEscena::quitarComponente(int idObjeto, const std::string& tipo,
                                         std::string& error) {
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) {
        error = "no existe el objeto " + std::to_string(idObjeto);
        return false;
    }
    const std::string canonico = canonicalizarTipo(tipo);
    if (canonico.empty()) {
        error = "no existe el componente '" + tipo + "'";
        return false;
    }
    if (!objeto->getComponentByName(canonico)) {
        error = "el objeto " + std::to_string(idObjeto) +
                " no tiene el componente " + canonico;
        return false;
    }
    editor_->getGestorComandos()->ejecutar(
        std::make_unique<QuitarComponenteComando>(editor_, objeto, canonico, escena_));
    return true;
}

bool InspeccionEscena::asignarVariable(int idObjeto,
                                       const std::string& tipoComponente,
                                       const std::string& variable,
                                       const std::string& valor,
                                       std::string& error) {
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) {
        error = "no existe el objeto " + std::to_string(idObjeto);
        return false;
    }
    const std::string canonico = canonicalizarTipo(tipoComponente);
    if (canonico.empty()) {
        error = "no existe el componente '" + tipoComponente + "'";
        return false;
    }
    const DefPropiedad* propiedad =
        CatalogoPropiedades::instancia().buscar(canonico, variable);
    if (!propiedad) {
        error = "el componente " + canonico + " no tiene la variable '" +
                variable + "'";
        return false;
    }
    Component* componente = objeto->getComponentByName(canonico);
    if (!componente) {
        error = "el objeto " + std::to_string(idObjeto) +
                " no tiene el componente " + canonico;
        return false;
    }
    // El valor se interpreta antes de encolar el comando: si el texto no es
    // valido, la escena no se toca.
    if (!propiedad->validar(valor, error)) return false;

    auto comando = std::make_unique<AjustarVariableComando>(
        editor_, escena_, idObjeto, canonico, propiedad->nombre, propiedad->leer,
        propiedad->escribir, valor);
    AjustarVariableComando* ajuste = comando.get();
    editor_->getGestorComandos()->ejecutar(std::move(comando));
    // El motor puede rechazar el valor aunque el texto sea valido (por
    // ejemplo un radio que no puede ser cero): en ese caso el comando lo
    // avisa y la escena queda como estaba.
    if (!ajuste->ultimoError().empty()) {
        error = ajuste->ultimoError();
        return false;
    }
    return true;
}

bool InspeccionEscena::posicionGlobal(int idObjeto, std::array<float, 3>& xyz) {
    GameObject* objeto = objetoPorIdInterno(idObjeto);
    if (!objeto) return false;
    Transform* global = objeto->getGlobalTransform();
    if (!global) return false;
    const float* posicion = global->getTranslatef();
    xyz = {posicion[0], posicion[1], posicion[2]};
    return true;
}

bool InspeccionEscena::guardarEscena(const std::string& prefijo,
                                     std::string& error) {
    if (!serializador_) {
        error = "no hay serializador configurado";
        return false;
    }
    // El serializador trabaja con un PREFIJO de ruta: escribe el indice
    // "<prefijo>BBDDObjetos.txt" y un binario por objeto dentro de la carpeta
    // "<prefijo>/". Esa carpeta tiene que existir: se crea aqui.
    std::error_code codigo;
    std::filesystem::create_directories(prefijo, codigo);
    if (codigo) {
        error = "no se pudo crear la carpeta '" + prefijo + "': " +
                codigo.message();
        return false;
    }
    serializador_->save(prefijo);
    if (!std::filesystem::exists(prefijo + indiceEscena, codigo)) {
        error = "no se pudo guardar la escena en '" + prefijo + "'";
        return false;
    }
    return true;
}

bool InspeccionEscena::cargarEscena(const std::string& prefijo,
                                     std::string& error) {
    if (!serializador_) {
        error = "no hay serializador configurado";
        return false;
    }
    const std::string indice = prefijo + indiceEscena;
    if (!std::filesystem::exists(indice)) {
        error = "no se encontro la escena '" + indice + "'";
        return false;
    }
    // Los binarios de cada objeto estan en la carpeta "<prefijo>/".
    if (!serializador_->load(indice, prefijo)) {
        error = "no se pudo cargar la escena '" + indice + "'";
        return false;
    }
    return true;
}

int InspeccionEscena::version() const {
    return versionApiEscena;
}

}  // namespace MotorInspeccion
