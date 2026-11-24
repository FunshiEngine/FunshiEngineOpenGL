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
#include "PropiedadesComponentesDetalle.h"

namespace MotorInspeccion {

// Propiedades de audio, interfaces (web y nativa) y scripts.
void CatalogoPropiedades::registrarInterfaz() {
    // --- AudioSource -----------------------------------------------------
    registrar("AudioSource", {
        propiedadTexto<AudioSource>(
            "clipPath", "Archivo de sonido",
            [](AudioSource* a) { return a->getRutaClip(); },
            [](AudioSource* a, const std::string& v) { a->setRutaClip(v); }),
        propiedadFlotante<AudioSource>(
            "volume", "Volumen",
            [](AudioSource* a) { return a->getVolume(); },
            [](AudioSource* a, float v) { a->setVolume(v); }),
        propiedadBooleana<AudioSource>(
            "loop", "En bucle",
            [](AudioSource* a) { return a->isLoop(); },
            [](AudioSource* a, bool v) { a->setLoop(v); }),
        propiedadBooleana<AudioSource>(
            "autoplay", "Reproducir al iniciar",
            [](AudioSource* a) { return a->isReproduccionAutomatica(); },
            [](AudioSource* a, bool v) { a->setReproduccionAutomatica(v); }),
    });

    // --- Musica ----------------------------------------------------------
    registrar("Musica", {
        propiedadFlotante<Musica>(
            "volume", "Volumen",
            [](Musica* m) { return m->getVolume(); },
            [](Musica* m, float v) { m->setVolume(v); }),
        propiedadFlotante<Musica>(
            "fadeDuration", "Fundido (segundos)",
            [](Musica* m) { return m->getDuracionFundido(); },
            [](Musica* m, float v) { m->setDuracionFundido(v); }),
        propiedadBooleana<Musica>(
            "autoplay", "Reproducir al iniciar",
            [](Musica* m) { return m->isReproduccionAutomatica(); },
            [](Musica* m, bool v) { m->setReproduccionAutomatica(v); }),
        propiedadBooleana<Musica>(
            "repeatList", "Repetir la lista",
            [](Musica* m) { return m->isRepetirLista(); },
            [](Musica* m, bool v) { m->setRepetirLista(v); }),
    });

    // --- InterfaceComponent ----------------------------------------------
    registrar("InterfaceComponent", {
        propiedadTexto<InterfaceComponent>(
            "htmlPath", "Archivo HTML",
            [](InterfaceComponent* i) { return i->getRutaHtml(); },
            [](InterfaceComponent* i, const std::string& v) {
                i->setRutaHtml(v);
            }),
        propiedadBooleana<InterfaceComponent>(
            "active", "Activa",
            [](InterfaceComponent* i) { return i->estaActiva(); },
            [](InterfaceComponent* i, bool v) { i->setActivo(v); }),
        propiedadEnumeracion<InterfaceComponent>(
            "anchor", "Ancla",
            {"ArribaIzquierda", "ArribaCentro", "ArribaDerecha",
             "MedioIzquierda", "Centro", "MedioDerecha", "AbajoIzquierda",
             "AbajoCentro", "AbajoDerecha"},
            [](InterfaceComponent* i) { return static_cast<int>(i->getAncla()); },
            [](InterfaceComponent* i, int v) {
                i->setAncla(static_cast<AnclaInterfaz>(v));
            }),
        propiedadVec2<InterfaceComponent>(
            "offset", "Desplazamiento",
            [](InterfaceComponent* i) {
                const glm::vec2 v = i->getDesplazamiento();
                return std::array<float, 2>{v.x, v.y};
            },
            [](InterfaceComponent* i, float x, float y) {
                i->setDesplazamiento(glm::vec2(x, y));
            }),
        propiedadBooleana<InterfaceComponent>(
            "autoSize", "Tamano automatico",
            [](InterfaceComponent* i) { return i->usaTamanoAuto(); },
            [](InterfaceComponent* i, bool v) { i->setTamanoAuto(v); }),
        propiedadVec2<InterfaceComponent>(
            "size", "Tamano",
            [](InterfaceComponent* i) {
                const glm::vec2 v = i->getTamano();
                return std::array<float, 2>{v.x, v.y};
            },
            [](InterfaceComponent* i, float w, float h) {
                i->setTamano(glm::vec2(w, h));
            }),
        propiedadFlotante<InterfaceComponent>(
            "z", "Orden de apilado",
            [](InterfaceComponent* i) { return i->getZ(); },
            [](InterfaceComponent* i, float v) { i->setZ(v); }),
    });

    // --- UIComponent (interfaz de juego nativa) --------------------------
    // Como con el Tilemap (que solo expone el tileset), la lista de widgets es
    // un dato compuesto que se edita en el inspector: aqui se exponen las
    // propiedades simples del componente.
    registrar("UIComponent", {
        propiedadBooleana<UIComponent>(
            "active", "Activa",
            [](UIComponent* i) { return i->estaActivo(); },
            [](UIComponent* i, bool v) { i->setActivo(v); }),
        propiedadFlotante<UIComponent>(
            "z", "Orden de apilado",
            [](UIComponent* i) { return i->getZ(); },
            [](UIComponent* i, float v) { i->setZ(v); }),
    });

    // --- Script ----------------------------------------------------------
    // Cambiar la ruta del script descarta el comportamiento cargado: el
    // motor lo vuelve a compilar al cargar la escena.
    registrar("Script", {
        propiedadTexto<Script>(
            "sourcePath", "Codigo fuente del script",
            [](Script* s) { return s->getPath(); },
            [](Script* s, const std::string& v) { s->setDllPath(v); }),
        propiedadTexto<Script>(
            "componentName", "Nombre del componente",
            [](Script* s) { return s->getNombreComponente(); },
            [](Script* s, const std::string& v) { s->setNombreComponente(v); }),
        propiedadBooleana<Script>(
            "deferredLoad", "Aplazar la carga",
            [](Script* s) { return s->getAplazarCarga(); },
            [](Script* s, bool v) { s->setAplazarCarga(v); }),
    });
}

}  // namespace MotorInspeccion
