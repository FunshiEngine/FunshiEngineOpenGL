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

// API publica para mods nativos del editor. La frontera es POD (sin STL, sin
// objetos con destructor) para que el .dll/.so del mod NO tenga que enlazar
// contra el motor ni depender de su ABI de C++: el motor le pasa una tabla de
// punteros a funcion y el mod devuelve la suya.
//
// La tabla del motor es APPEND-ONLY: los campos nuevos van al final y la
// version se incrementa al agregar funcionalidad.
//
// La version de la API que el motor ofrece es `versionApiMod`. Un mod declara
// con que version fue compilado (el valor que devuelve su funcion de entrada):
// si declara una mayor, el motor lo rechaza (no puede ofrecer servicios que el
// mod no conoce); si declara una menor, el motor lo acepta y le ofrece la
// version actual.
//
// A diferencia de las tablas de scripts (`ApiScriptGameObject`,
// `ScriptServices`), aqui `version` va PRIMERO y no al final. La razon: un
// mod se compila aparte y puede quedar desactualizado respecto del motor. Con
// `version` en el primer campo su offset no se mueve nunca, asi que el mod
// lee siempre su version real y decide con ella si usa los campos nuevos; con
// `version` al final (como en las tablas de scripts) cada ampliacion
// desplazaria el offset y el mod leeria basura. Los campos nuevos se anaden
// siempre al final, detras de los que ya existian.

#ifndef API_MOD_H
#define API_MOD_H

#include <cstddef>

// Marca de exportacion de la funcion de entrada del mod. En Windows (MSVC y
// MinGW) una funcion de una .dll NO se exporta sola: sin
// __declspec(dllexport) el binario compila pero su tabla de exportaciones no
// la tiene y GetProcAddress devuelve NULL. En Linux/macOS los simbolos se
// exportan por defecto.
#if defined(_WIN32)
#define FUNSHI_MOD_EXPORT __declspec(dllexport)
#else
#define FUNSHI_MOD_EXPORT
#endif

namespace FunshiMods {

// Contexto que el motor pasa al mod en cada dibujo. Todo lo que el mod
// necesita del motor entra por aca (o por la tabla de servicios): el mod no
// ve objetos del motor, solo datos.
struct ContextoMod {
    // Contexto grafico de ImGui (opaco). El mod que dibuja debe hacer
    // ImGui::SetCurrentContext(ctxImgui) antes de pintar, para que sus
    // llamadas a ImGui usen el contexto del motor y no el suyo.
    void* ctxImgui = nullptr;
    // Tamano en pixeles del area donde el mod dibuja.
    float areaAncho = 0.0f;
    float areaAlto = 0.0f;
    // Tiempo transcurrido desde el frame anterior, en segundos.
    float deltaTime = 0.0f;
};

// Version de la API de mods que ofrece esta build del motor.
constexpr int versionApiMod = 2;

// Nombre del tipo de un componente de un GameObject (por ejemplo
// "Transform", "CubeCollider"). Tamaño fijo para que el dato sea POD.
struct ApiModComponente {
    char tipo[64] = {0};
};

// Datos de un GameObject de la escena. Es una COPIA de los valores en el
// momento de la consulta: el mod nunca ve punteros a objetos del motor, asi
// que no puede modificarlos ni mantenerlos validos entre llamadas.
struct ApiModGameObject {
    // Id de escena del objeto (GameObject::getId).
    int id = 0;
    // Nombre del objeto.
    char nombre[64] = {0};
    // Etiqueta (tag) del objeto; "Untagged" si no tiene.
    char etiqueta[32] = {0};
    // Posicion global (en unidades de la escena).
    float posicion[3] = {0.0f, 0.0f, 0.0f};
    // Escala global.
    float escala[3] = {1.0f, 1.0f, 1.0f};
    // Rotacion global. El motor la guarda como angulo-eje: rotacion[0] es el
    // angulo en grados y rotacion[1..3] el eje (x, y, z).
    float rotacion[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    // Cantidad de componentes que tiene el objeto.
    int cantidadComponentes = 0;
    // 1 si el objeto tiene algun collider (esferico, cubico, de malla o de
    // tilemap), 0 si no. Es una comodidad: el detalle de cada componente se
    // consulta con obtenerComponente().
    int tieneCollider = 0;
};

// Callback de dibujo de un panel que registro un mod. El motor lo llama una
// vez por frame, dentro de la ventana del panel, y le entrega el contexto de
// dibujo (el mismo que recibe IMod::dibujar) y los datos que el mod entrego
// al registrar el panel.
using CallbackPanelMod = void (*)(const ContextoMod* contexto, void* datos);

// Tabla de servicios que el motor ofrece a los mods. APPEND-ONLY: los campos
// nuevos van al final. `version` va primero (vease la nota de la cabecera).
struct ApiModServicios {
    // Version de la API que el motor ofrece (versionApiMod). Primer campo:
    // su offset no cambia nunca, para que un mod compilado contra una version
    // anterior lea su version real.
    int version = versionApiMod;

    // --- v2: registro de paneles de GUI ---
    // Registra un panel de GUI en el editor. El motor lo muestra como una
    // ventana propia, con su entrada en el menu "Ventanas" y su estado
    // persistido junto al de las demas ventanas.
    //   id      : identificador del panel dentro del mod (unico en el mod).
    //   titulo  : texto visible de la ventana.
    //   dibujar : callback al que el motor llama por frame con el contexto.
    //   datos   : puntero opaco que el motor devuelve tal cual al callback.
    // Devuelve false si falta algun dato, si el id ya esta registrado en ese
    // mod o si el mod ya no esta cargado.
    bool (*registrarPanel)(const char* id, const char* titulo,
                           CallbackPanelMod dibujar, void* datos);

    // --- v2: datos de la escena ---
    // Cantidad de GameObjects de la escena abierta. El recorrido es el mismo
    // que ve el editor (la lista de la escena, en orden).
    int (*contarObjetos)();
    // Datos del GameObject `indice` (0 .. contarObjetos()-1). Devuelve false
    // si el indice no existe o si el objeto ya no esta en la escena.
    bool (*obtenerObjeto)(int indice, ApiModGameObject* salida);
    // Cantidad de componentes del GameObject `indice`.
    int (*contarComponentes)(int indice);
    // Datos del componente `indiceComponente` del GameObject `indiceObjeto`.
    // Devuelve false si alguno de los dos indices no existe.
    bool (*obtenerComponente)(int indiceObjeto, int indiceComponente,
                              ApiModComponente* salida);
    // Nombre del proyecto abierto ("" si no hay ninguno abierto).
    const char* (*nombreProyecto)();
    // Contexto de ImGui del motor (opaco). El mod que dibuja debe hacer
    // ImGui::SetCurrentContext(contexto) con el valor devuelto para que sus
    // llamadas a ImGui usen el contexto del motor. Devuelve nullptr si el
    // contexto todavia no se creo.
    void* (*contextoImGui)();
};

// Interfaz que el mod implementa. Es una tabla de punteros a funcion (no una
// clase con vtable): todos los campos son opcionales, y un nullptr significa
// "el mod no implementa esto".
//
// Sus campos tambien son APPEND-ONLY (los nuevos al final): el motor los
// lee por offset, asi que un mod compilado contra una version anterior
// sigue siendo valido mientras no se reordenen.
struct IMod {
    // Inicializacion: el motor la llama una vez, tras cargar el binario y
    // antes de cualquier dibujo o actualizacion. Recibe la tabla de servicios.
    // Es el momento natural para registrar los paneles del mod.
    void (*init)(const ApiModServicios* servicios);
    // Actualizacion por frame (logica del mod). Opcional.
    void (*update)(float deltaTime);
    // Dibujo del mod, dentro de su propia ventana o panel. Opcional. El
    // motor la invoca una vez por frame en una ventana propia del mod; un mod
    // que registra sus paneles con registrarPanel() puede omitirla.
    void (*dibujar)(const ContextoMod* contexto);
    // Limpieza de recursos: el motor la llama al descargar el mod, antes de
    // cerrar el binario. Opcional.
    void (*shutdown)();
    // Nombre visible del mod (para el menu y los diagnosticos). Opcional.
    const char* (*nombre)();
    // Version del mod (texto corto). Opcional.
    const char* (*versionStr)();
};

// Tipo de la funcion de entrada que el motor busca en el binario del mod:
//   extern "C" FUNSHI_MOD_EXPORT int funshi_mod_api(FunshiMods::IMod** salida);
// Devuelve la version de la API con la que el mod fue compilado, o 0 si no
// puede trabajar con el motor (en ese caso el motor lo rechaza). Escribe la
// interfaz del mod en `salida`.
using FuncionEntradaMod = int (*)(IMod** salida);

} // namespace FunshiMods

#endif // API_MOD_H
