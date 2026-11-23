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
#include "EstadoInterfaces.h"

#include <nlohmann/json.hpp>

#include <map>
#include <string>
#include <vector>

#include "../Objetos/Componentes/Interfaz/InterfaceComponent.h"

namespace {

using nlohmann::json;

bool esIndice(const std::string& token) {
    if (token.empty()) return false;
    for (char c : token)
        if (c < '0' || c > '9') return false;
    return true;
}

std::vector<std::string> separarRuta(const std::string& ruta) {
    std::vector<std::string> tokens;
    std::string actual;
    for (char c : ruta) {
        if (c == '/') {
            if (!actual.empty() || !tokens.empty()) {
                // Conserva tokens vacios salvo el primero (barra inicial).
                tokens.push_back(actual);
                actual.clear();
            }
        } else {
            actual.push_back(c);
        }
    }
    if (!actual.empty() || tokens.empty()) tokens.push_back(actual);
    return tokens;
}

// Fija `valor` en `ruta` creando objetos intermedios. Devuelve false si la
// ruta atraviesa un escalar o un array fuera de rango.
bool fijarEnRuta(json& raiz, const std::string& ruta, json valor) {
    std::vector<std::string> tokens = separarRuta(ruta);
    if (tokens.size() == 1 && tokens[0].empty()) {
        raiz = std::move(valor);
        return true;
    }
    if (raiz.is_null()) raiz = json::object();
    json* actual = &raiz;
    for (std::size_t i = 0; i + 1 < tokens.size(); ++i) {
        const std::string& t = tokens[i];
        if (actual->is_array() && esIndice(t)) {
            std::size_t idx = static_cast<std::size_t>(std::stoul(t));
            if (idx >= actual->size()) return false;
            actual = &((*actual)[idx]);
        } else if (actual->is_object()) {
            if (!actual->contains(t) ||
                (!(*actual)[t].is_object() && !(*actual)[t].is_array()))
                (*actual)[t] = json::object();
            actual = &((*actual)[t]);
        } else {
            return false;
        }
    }
    const std::string& ultimo = tokens.back();
    if (actual->is_array() && esIndice(ultimo)) {
        std::size_t idx = static_cast<std::size_t>(std::stoul(ultimo));
        if (idx >= actual->size()) return false;
        (*actual)[idx] = std::move(valor);
        return true;
    }
    if (actual->is_object() || actual->is_null()) {
        if (actual->is_null()) *actual = json::object();
        (*actual)[ultimo] = std::move(valor);
        return true;
    }
    return false;
}

const json* leerEnRuta(const json& raiz, const std::string& ruta) {
    if (ruta.empty() || ruta == "/") return &raiz;
    const json* actual = &raiz;
    for (const std::string& t : separarRuta(ruta)) {
        if (t.empty()) continue;
        if (actual->is_object()) {
            auto it = actual->find(t);
            if (it == actual->end()) return nullptr;
            actual = &(*it);
        } else if (actual->is_array() && esIndice(t)) {
            std::size_t idx = static_cast<std::size_t>(std::stoul(t));
            if (idx >= actual->size()) return nullptr;
            actual = &((*actual)[idx]);
        } else {
            return nullptr;
        }
    }
    return actual;
}

} // namespace

struct EstadoInterfaces::Impl {
    struct Entrada {
        json estado = json::object();
        json salida = json::object();
        bool pendiente = false;
        std::string estadoTexto;
        std::string valorTexto;
    };
    std::map<const InterfaceComponent*, Entrada> entradas;
};

EstadoInterfaces::EstadoInterfaces() : impl_(new Impl()) {}

EstadoInterfaces::~EstadoInterfaces() = default;

std::string EstadoInterfaces::normalizarRuta(const std::string& ruta) {
    if (ruta.empty()) return std::string();
    if (ruta.front() == '/') return ruta;
    return "/" + ruta;
}

bool EstadoInterfaces::establecerEstado(InterfaceComponent* interfaz,
                                        const std::string& texto) {
    if (!interfaz) return false;
    json parseado = json::parse(texto, nullptr, false);
    if (parseado.is_discarded()) return false;
    Impl::Entrada& e = impl_->entradas[interfaz];
    e.estado = std::move(parseado);
    e.pendiente = true;
    return true;
}

bool EstadoInterfaces::establecerValor(InterfaceComponent* interfaz,
                                       const std::string& ruta,
                                       const std::string& valorJson) {
    if (!interfaz) return false;
    json valor = json::parse(valorJson, nullptr, false);
    if (valor.is_discarded()) return false;
    Impl::Entrada& e = impl_->entradas[interfaz];
    if (!fijarEnRuta(e.estado, normalizarRuta(ruta), std::move(valor)))
        return false;
    e.pendiente = true;
    return true;
}

const std::string* EstadoInterfaces::estadoPendiente(
    const InterfaceComponent* interfaz) {
    if (!interfaz) return nullptr;
    auto it = impl_->entradas.find(interfaz);
    if (it == impl_->entradas.end() || !it->second.pendiente) return nullptr;
    it->second.estadoTexto = it->second.estado.dump();
    return &it->second.estadoTexto;
}

void EstadoInterfaces::marcarEnviado(const InterfaceComponent* interfaz) {
    if (!interfaz) return;
    auto it = impl_->entradas.find(interfaz);
    if (it != impl_->entradas.end()) it->second.pendiente = false;
}

void EstadoInterfaces::definirSalida(InterfaceComponent* interfaz,
                                     const std::string& texto) {
    if (!interfaz) return;
    json parseado = json::parse(texto, nullptr, false);
    if (parseado.is_discarded()) return;
    impl_->entradas[interfaz].salida = std::move(parseado);
}

const char* EstadoInterfaces::valor(InterfaceComponent* interfaz,
                                    const std::string& ruta) {
    if (!interfaz) return nullptr;
    auto it = impl_->entradas.find(interfaz);
    if (it == impl_->entradas.end()) return nullptr;
    const json* encontrado = leerEnRuta(it->second.salida, normalizarRuta(ruta));
    if (!encontrado) return nullptr;
    it->second.valorTexto = encontrado->dump();
    return it->second.valorTexto.c_str();
}

void EstadoInterfaces::olvidar(const InterfaceComponent* interfaz) {
    impl_->entradas.erase(interfaz);
}

void EstadoInterfaces::vaciar() { impl_->entradas.clear(); }
