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
#include "VistasWebUI.h"

#include <chrono>
#include <filesystem>
#include <set>
#include <string>
#include <utility>

#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"
#include "EstadoInterfaces.h"
#include "WebUIRuntime.h"

namespace fs = std::filesystem;

namespace {

// Area que ocupa la vista: para tamano automatico, el contenedor entero (el
// CSS del HTML decide donde se pone cada elemento); para tamano fijo, el que
// guarda el componente.
void medidasDe(const InterfaceComponent* interfaz, int anchoContenedor,
               int altoContenedor, int& ancho, int& alto) {
    if (interfaz->usaTamanoAuto()) {
        ancho = anchoContenedor;
        alto = altoContenedor;
    } else {
        ancho = static_cast<int>(interfaz->getTamano().x);
        alto = static_cast<int>(interfaz->getTamano().y);
    }
}

// Sello de escritura del .html (nanosegundos desde epoch del file_time).
// nullopt si no existe o no se puede leer: el gestor no recarga por mtime en
// ese caso (mismo criterio que ScriptRuntime::cambioElFuente con fuentes
// ausentes: solo el cambio de ruta rehace la vista).
SelloEscrituraWebUI selloDe(const std::string& rutaHtml) {
    if (rutaHtml.empty()) return std::nullopt;
    std::error_code ec;
    const fs::file_time_type t = fs::last_write_time(fs::path(rutaHtml), ec);
    if (ec) return std::nullopt;
    return static_cast<long long>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t.time_since_epoch())
            .count());
}

void sellar(VistaWebUI& vista) { vista.mtimeHtml = selloDe(vista.ruta); }

} // namespace

void VistasWebUI::sincronizar(
    const std::vector<InterfaceComponent*>& interfaces, int anchoContenedor,
    int altoContenedor, float deltaTime, EstadoInterfaces* estado) {
    std::set<const InterfaceComponent*> vigentes(interfaces.begin(),
                                                 interfaces.end());

    for (InterfaceComponent* interfaz : interfaces) {
        int ancho = 0;
        int alto = 0;
        medidasDe(interfaz, anchoContenedor, altoContenedor, ancho, alto);

        auto it = vistas_.find(interfaz);
        if (it == vistas_.end()) {
            std::string error;
            VistaWebUI vista =
                WebUIRuntime::cargar(interfaz->getRutaHtml(), ancho, alto, error);
            sellar(vista);
            it = vistas_.emplace(interfaz, std::move(vista)).first;
        } else {
            VistaWebUI& vista = it->second;
            if (vista.ruta != interfaz->getRutaHtml()) {
                // Cambio de .html del componente: se rehace la vista.
                WebUIRuntime::descargar(vista);
                std::string error;
                vista = WebUIRuntime::cargar(interfaz->getRutaHtml(), ancho, alto,
                                             error);
                sellar(vista);
            } else if (vista.ancho != ancho || vista.alto != alto) {
                WebUIRuntime::redimensionar(vista, ancho, alto);
            } else {
                // Recarga en caliente: el .html cambio en disco (guardado
                // externo) desde que se cargo la vista. Se rehace como si
                // hubiera cambiado la ruta. Sin sello (archivo ilegible) no se
                // recarga: evita rehacer la vista en cada frame.
                const SelloEscrituraWebUI actual = selloDe(vista.ruta);
                if (actual.has_value() && vista.mtimeHtml.has_value() &&
                    *actual != *vista.mtimeHtml) {
                    WebUIRuntime::descargar(vista);
                    std::string error;
                    vista = WebUIRuntime::cargar(interfaz->getRutaHtml(), ancho,
                                                 alto, error);
                    sellar(vista);
                }
            }
        }

        WebUIRuntime::actualizar(it->second, deltaTime);

        // Intercambio de estado con la pagina: entrega el estado pendiente (el
        // que cambio desde el ultimo frame) y recoge la salida que el JS haya
        // publicado en `FunshiUI.salida`.
        if (estado) {
            const std::string* pendiente = estado->estadoPendiente(interfaz);
            if (pendiente) {
                WebUIRuntime::establecerEstado(it->second, *pendiente);
                estado->marcarEnviado(interfaz);
            }
            std::string salida = WebUIRuntime::leerSalida(it->second);
            if (!salida.empty()) estado->definirSalida(interfaz, salida);
        }

        WebUIRuntime::render(it->second);
    }

    // Descarga las vistas de interfaces que ya no estan activas o desaparecieron.
    for (auto it = vistas_.begin(); it != vistas_.end();) {
        if (vigentes.find(it->first) == vigentes.end()) {
            WebUIRuntime::descargar(it->second);
            if (estado) estado->olvidar(it->first);
            it = vistas_.erase(it);
        } else {
            ++it;
        }
    }
}

const VistaWebUI* VistasWebUI::buscar(
    const InterfaceComponent* interfaz) const {
    auto it = vistas_.find(interfaz);
    return it == vistas_.end() ? nullptr : &it->second;
}

VistaWebUI* VistasWebUI::buscar(const InterfaceComponent* interfaz) {
    auto it = vistas_.find(interfaz);
    return it == vistas_.end() ? nullptr : &it->second;
}

void VistasWebUI::vaciar() {
    for (auto& par : vistas_) WebUIRuntime::descargar(par.second);
    vistas_.clear();
}
