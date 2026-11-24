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
#include "ArrastreArchivos.h"

#include <filesystem>
#include <memory>

#include "EditorController.h"
#include "../Registro/SceneRegistry.h"
#include "../../Assets/Imagenes/StbImageLoader.h"
#include "../../Assets/Mallas/AssimpMeshLoader.h"
#include "../../Comandos/GestorComandos.h"
#include "../../Comandos/Componentes/AgregarComponenteComando.h"
#include "../../Comandos/Objetos/CrearObjetoComando.h"
#include "../../Objetos/GameObject.h"
#include "../../Objetos/GameObjectFactory.h"
#include "../../Objetos/Prefab.h"
#include "../../Objetos/PrefabLibrary.h"
#include "../../Objetos/Componentes/Audio/AudioSource.h"
#include "../../Objetos/Componentes/Efectos/EmisorParticulas.h"
#include "../../Objetos/Componentes/Escena/Transform.h"
#include "../../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "../../Objetos/Componentes/Juego2D/Tilemap.h"
#include "../../Objetos/Componentes/Render/Model.h"
#include "../../Objetos/Componentes/Render/SpriteRenderer.h"
#include "../../Objetos/Componentes/Scripting/Script.h"

namespace ArrastreArchivos {

namespace {

// Lado de la capa con la que nace un tilemap creado desde un tileset.
constexpr int kLadoCapaInicial = 16;

Resultado fallo(const std::string& mensaje) {
    Resultado r;
    r.mensaje = mensaje;
    return r;
}

// Validacion del contenido antes de crear nada: un archivo que el componente
// no puede leer no debe dejar un objeto vacio en la escena.
bool contenidoValido(TipoArchivo tipo, const std::string& ruta, std::string& motivo) {
    if (tipo == TipoArchivo::Modelo3D && !AssimpMeshLoader::puedeLeerFormato(ruta)) {
        motivo = "Assimp no importa este formato de modelo";
        return false;
    }
    if (tipo == TipoArchivo::Imagen) {
        int ancho = 0, alto = 0;
        if (!StbImageLoader::dimensiones(ruta, ancho, alto)) {
            motivo = "la imagen no se puede leer";
            return false;
        }
    }
    return true;
}

// Componente del tipo T del objeto; si no lo tiene, crea uno nuevo en
// 'nuevo' (que el llamador agrega) y devuelve ese.
template <typename T>
T* componenteDe(GameObject& objeto, std::unique_ptr<Component>& nuevo) {
    if (T* existente = objeto.getComponent<T>()) return existente;
    auto creado = std::make_unique<T>();
    T* puntero = creado.get();
    nuevo = std::move(creado);
    return puntero;
}

// Deja en el objeto el componente del tipo del archivo apuntando a 'ruta'.
// Si hay que crear el componente, queda en 'nuevo' sin agregar.
bool configurar(EditorController& editor, GameObject& objeto, TipoArchivo tipo,
                const std::string& ruta, std::unique_ptr<Component>& nuevo) {
    switch (tipo) {
    case TipoArchivo::Modelo3D: {
        Model* modelo = componenteDe<Model>(objeto, nuevo);
        modelo->setAssetManager(editor.getAssetManager());
        modelo->setPath(ruta);
        return true;
    }
    case TipoArchivo::Imagen:
        componenteDe<SpriteRenderer>(objeto, nuevo)->setRutaTextura(ruta);
        return true;
    case TipoArchivo::Audio:
        componenteDe<AudioSource>(objeto, nuevo)->setRutaClip(ruta);
        return true;
    case TipoArchivo::Tileset: {
        Tilemap* tilemap = componenteDe<Tilemap>(objeto, nuevo);
        tilemap->setRutaTileset(ruta);
        if (tilemap->cantidadCapas() == 0)
            tilemap->agregarCapa("Capa", kLadoCapaInicial, kLadoCapaInicial, false);
        return true;
    }
    case TipoArchivo::PerfilParticulas:
        componenteDe<EmisorParticulas>(objeto, nuevo)->setRutaPerfil(ruta);
        return true;
    case TipoArchivo::InterfazHtml:
        componenteDe<InterfaceComponent>(objeto, nuevo)->setRutaHtml(ruta);
        return true;
    case TipoArchivo::ScriptCpp:
    case TipoArchivo::ScriptJava:
        // Un objeto puede tener varios scripts: siempre se agrega uno nuevo.
        {
            auto script = std::make_unique<Script>();
            script->setDllPath(ruta);
            nuevo = std::move(script);
        }
        return true;
    default:
        return false;
    }
}

void colocar(GameObject* objeto, const Destino& destino) {
    if (!objeto || !destino.conPosicion) return;
    if (Transform* t = objeto->getComponent<Transform>())
        t->setTranslatef(destino.posicion.x, destino.posicion.y, destino.posicion.z);
}

Resultado instanciarPrefab(EditorController& editor, const std::string& ruta,
                           GameObject* padre, const Destino& destino) {
    PrefabLibrary* biblioteca = editor.getPrefabLibrary();
    Prefab* prefab = biblioteca ? biblioteca->obtenerPorRuta(ruta) : nullptr;
    if (!prefab) return fallo("el prefab no se pudo leer");
    GameObject* instancia = prefab->instanciar(&editor, padre);
    if (!instancia) return fallo("el prefab no se pudo instanciar");
    colocar(instancia, destino);
    Resultado r;
    r.ok = true;
    r.objeto = instancia;
    return r;
}

}  // namespace

bool aceptaEnEscena(TipoArchivo tipo) {
    return tipo != TipoArchivo::Desconocido && tipo != TipoArchivo::Proyecto;
}

bool aceptaEnObjeto(TipoArchivo tipo) { return aceptaEnEscena(tipo); }

Resultado crearDesdeArchivo(EditorController& editor, const std::string& ruta,
                            const Destino& destino) {
    const TipoArchivo tipo = TiposArchivo::clasificar(ruta);
    if (!aceptaEnEscena(tipo)) return fallo("tipo de archivo no soportado en la escena");
    if (tipo == TipoArchivo::Prefab) return instanciarPrefab(editor, ruta, destino.padre, destino);

    std::string motivo;
    if (!contenidoValido(tipo, ruta, motivo)) return fallo(motivo);

    SceneRegistry* escena = editor.getScene();
    const std::string nombre = std::filesystem::path(ruta).stem().string();
    std::unique_ptr<GameObject> objeto = GameObjectFactory::createSimpleObject(
        escena ? escena->getRoot() : nullptr, nombre);
    std::unique_ptr<Component> nuevo;
    if (!configurar(editor, *objeto, tipo, ruta, nuevo)) return fallo("tipo no soportado");
    if (nuevo) objeto->addComponent(std::move(nuevo));
    colocar(objeto.get(), destino);

    GameObject* creado = nullptr;
    if (GestorComandos* comandos = editor.getGestorComandos()) {
        auto comando = std::make_unique<CrearObjetoComando>(&editor, escena,
                                                            std::move(objeto), destino.padre);
        CrearObjetoComando* referencia = comando.get();
        comandos->ejecutar(std::move(comando));
        // El gestor conserva el comando en el historial: el id sigue valido.
        if (escena && referencia->idCreado() > 0)
            creado = escena->getObjectByID(referencia->idCreado());
    } else {
        creado = editor.createGameObject(std::move(objeto), destino.padre);
    }
    if (!creado) return fallo("no se pudo crear el objeto");
    Resultado r;
    r.ok = true;
    r.objeto = creado;
    return r;
}

Resultado aplicarAObjeto(EditorController& editor, GameObject& objeto,
                         const std::string& ruta) {
    const TipoArchivo tipo = TiposArchivo::clasificar(ruta);
    if (!aceptaEnObjeto(tipo)) return fallo("tipo de archivo no soportado en el objeto");
    if (tipo == TipoArchivo::Prefab) {
        Destino destino;
        return instanciarPrefab(editor, ruta, &objeto, destino);
    }

    std::string motivo;
    if (!contenidoValido(tipo, ruta, motivo)) return fallo(motivo);

    std::unique_ptr<Component> nuevo;
    if (!configurar(editor, objeto, tipo, ruta, nuevo)) return fallo("tipo no soportado");
    if (nuevo) {
        if (GestorComandos* comandos = editor.getGestorComandos())
            comandos->ejecutar(std::make_unique<AgregarComponenteComando>(
                &editor, &objeto, std::move(nuevo), editor.getScene()));
        else
            editor.addComponent(&objeto, std::move(nuevo));
    }
    Resultado r;
    r.ok = true;
    r.objeto = &objeto;
    return r;
}

}  // namespace ArrastreArchivos
