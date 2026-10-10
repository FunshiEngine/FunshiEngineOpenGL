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
#ifndef EDITOR_EVENT_BUS_H
#define EDITOR_EVENT_BUS_H

#include <functional>
#include <string>
#include <unordered_map>
#include <cstddef>

#include "../Configuracion/Apariencia.h"

class GameObject;

// Canal tipado de GUI interna (ARQUITECTURA_ESTADOS_GUI.md, §4.2B Fase 2).
// Complementa al EventBus de escena (src/Events/EventBus.h), que sigue atado
// a GameObject*; aqui NO se reimplementa ese bus, solo se aporta el canal de
// ventanas que faltaba. Cada ventana se subscribe a lo que necesita y publica
// lo que produce; ninguna se pasa un puntero a otra (patron Mediator/Observer
// que el proyecto ya usa de forma puntual, ahora declarado para todas las GUI).
//
// Headless (sin ImGui/OpenGL): igual de testeable que EditorConfig y que el
// orquestador de estados. El dueño del bus es GUIManager (su registro central
// de ventanas); menu, escena y main publican/suscriben sin conocerse.
enum class EditorEventType {
    VentanaEstadoCambio,  // visibilidad de la ventana "Estado" (StatusBarInterface)
    AparienciaCambio,     // perfil completo (tema/acento/fondo/B-N) -> escena + TemaEditor
    CamaraActivaCambio,   // camara elegida con "Usar" -> previews/inspector
    IdiomaCambio,         // idioma -> etiquetas sensibles (onLanguageChanged)
    SensibilidadCambio,   // multiplicador del mouse look de la camara -> escena
    SensibilidadMovimientoCambio, // multiplicador de la velocidad WASD -> escena
    ReiniciarConfiguracion, // "Restablecer configuracion" -> main reaplica defaults
    VentanaActivaCambio,  // reservado: ventana con foco ImGui (Settings futuro)
    ArchivosReubicados,   // archivo/carpeta movido o renombrado en el explorador -> Reescribir referencias
};

// Datos declarativos segun el tipo (punteros NO propietarios; cadenas por
// copia; perfil de apariencia por valor). El payload es el minimo necesario:
// cada tipo usa sus campos y el resto queda en default.
struct EditorEvent {
    EditorEventType type = EditorEventType::AparienciaCambio;

    // Para VentanaEstadoCambio / VentanaActivaCambio: nombre de ventana (dato
    // de GUIManager, no de la escena) y el estado abierto/cerrado nuevo.
    const char* nombreVentana = nullptr;
    bool abierta = false;

    // Para IdiomaCambio: codigo de idioma (Espanol/English). Las etiquetas
    // sensibles al idioma se refrescan via onLanguageChanged.
    std::string idioma;

    // Para SensibilidadCambio: multiplicador global del mouse look (1.0 = 1:1).
    float sensibilidad = 1.0f;

    // Para SensibilidadMovimientoCambio: multiplicador de la velocidad WASD de
    // la camara del editor (1.0 = velocidad base de la camara activa).
    float sensibilidadMovimiento = 1.0f;

    // Para CamaraActivaCambio: puntero NO propietario al GameObject camara
    // (igual que SceneEvent.object, pero semantica: «cambio la camara activa»,
    // no «se selecciono un objeto»). nullptr = camara automatica.
    GameObject* camara = nullptr;

    // Para AparienciaCambio: el perfil COMPLETO (tema, B/N, acento y fondo).
    // Se transporta por valor como un DTO puro, sin punteros a la escena.
    Apariencia apariencia;

    // Para ArchivosReubicados: rutas ABSOLUTAS anterior y nueva del elemento
    // que se movio o renombro dentro del explorador. main (unico suscriptor)
    // reescribe las referencias de la escena que cayan bajo rutaAnterior.
    std::string rutaAnterior;
    std::string rutaNueva;
};

// Bus pub/sub 1:1 con el patron de EventBus (SceneEvent) pero con EditorEvent.
// subscribe() devuelve un token para unsubscribe (mismo contrato que EventBus).
class EditorEventBus {
public:
    using Callback = std::function<void(const EditorEvent&)>;

    EditorEventBus() = default;
    ~EditorEventBus() = default;

    EditorEventBus(const EditorEventBus&) = delete;
    EditorEventBus& operator=(const EditorEventBus&) = delete;

    size_t subscribe(Callback callback);

    void unsubscribe(size_t token);

    void publish(const EditorEvent& event);

private:
    std::unordered_map<size_t, Callback> listeners;
    size_t nextToken = 1;
};

#endif // EDITOR_EVENT_BUS_H