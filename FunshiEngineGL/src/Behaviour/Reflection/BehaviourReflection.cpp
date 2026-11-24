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
#include "BehaviourReflectionDetalle.h"

namespace ReflejoScripts {

namespace {

// Resolver activo de GameObject por nombre (lo fija la escena en play mode).
ResolverObjeto g_resolver;

// Resolucion de referencias a componentes (TagTipo::Componente).
ResolverComponentes g_resolverComp;

// Devuelve el ValorCampo hijo cuyo nombre coincide con el subcampo; si no
// aparece usa el de la posicion hononima (robustez ante reordenamientos).
const ValorCampo* valorDeSubcampo(const std::vector<ValorCampo>& valores,
                                  const DefCampo& sub, size_t indice) {
    for (const ValorCampo& valor : valores) {
        if (valor.nombre == sub.nombre) return &valor;
    }
    if (indice < valores.size()) return &valores[indice];
    return nullptr;
}

}  // namespace

void fijarResolverObjetos(ResolverObjeto resolver) {
    g_resolver = std::move(resolver);
}

void fijarResolverComponentes(ResolverComponentes resolvers) {
    g_resolverComp = std::move(resolvers);
}

const char* nombreDeTag(TagTipo tag) {
    switch (tag) {
        case TagTipo::Entero: return "Entero";
        case TagTipo::Flotante: return "Flotante";
        case TagTipo::Doble: return "Doble";
        case TagTipo::Booleano: return "Booleano";
        case TagTipo::Texto: return "Texto";
        case TagTipo::Vec3: return "Vec3";
        case TagTipo::Enteros: return "Enteros";
        case TagTipo::Flotantes: return "Flotantes";
        case TagTipo::Dobles: return "Dobles";
        case TagTipo::Booleanos: return "Booleanos";
        case TagTipo::Textos: return "Textos";
        case TagTipo::Vec3s: return "Vec3s";
        case TagTipo::Grupo: return "Grupo";
        case TagTipo::Grupos: return "Grupos";
        case TagTipo::Objeto: return "Objeto";
        case TagTipo::Objetos: return "Objetos";
        case TagTipo::Componente: return "Componente";
    }
    return "Desconocido";
}

ValorCampo valorPorDefecto(TagTipo tag) {
    ValorCampo valor;
    valor.tag = tag;
    switch (tag) {
        case TagTipo::Entero: valor.contenido = int(0); break;
        case TagTipo::Flotante: valor.contenido = 0.0f; break;
        case TagTipo::Doble: valor.contenido = 0.0; break;
        case TagTipo::Booleano: valor.contenido = false; break;
        case TagTipo::Texto: valor.contenido = std::string{}; break;
        case TagTipo::Vec3: valor.contenido = vec3{}; break;
        case TagTipo::Enteros: valor.contenido = std::vector<int>{}; break;
        case TagTipo::Flotantes: valor.contenido = std::vector<float>{}; break;
        case TagTipo::Dobles: valor.contenido = std::vector<double>{}; break;
        case TagTipo::Booleanos: valor.contenido = std::vector<bool>{}; break;
        case TagTipo::Textos: valor.contenido = std::vector<std::string>{}; break;
        case TagTipo::Vec3s: valor.contenido = std::vector<vec3>{}; break;
        case TagTipo::Grupo: valor.contenido = std::vector<ValorCampo>{}; break;
        case TagTipo::Grupos:
            valor.contenido = std::vector<std::vector<ValorCampo>>{};
            break;
        case TagTipo::Objeto: valor.contenido = std::string{}; break;
        case TagTipo::Objetos:
            valor.contenido = std::vector<std::string>{};
            break;
        case TagTipo::Componente: valor.contenido = std::string{}; break;
    }
    return valor;
}

ValorCampo valorPorDefecto(const DefCampo& def) {
    ValorCampo valor = valorPorDefecto(def.tag);
    valor.nombre = def.nombre;
    if (def.tag == TagTipo::Grupo) {
        std::vector<ValorCampo>& subs = valor.como<std::vector<ValorCampo>>();
        subs.reserve(def.subcampos.size());
        for (const DefCampo& sub : def.subcampos)
            subs.push_back(valorPorDefecto(sub));
    }
    return valor;
}

std::vector<ValorCampo> valoresPorDefecto(const std::vector<DefCampo>& defs) {
    std::vector<ValorCampo> valores;
    valores.reserve(defs.size());
    for (const DefCampo& def : defs) valores.push_back(valorPorDefecto(def));
    return valores;
}

ValorCampo leerCampo(const DefCampo& def, void* instancia) {
    ValorCampo valor;
    valor.nombre = def.nombre;
    valor.tag = def.tag;
    switch (def.tag) {
        case TagTipo::Entero:
            valor.contenido = *static_cast<int*>(def.acceder(instancia));
            break;
        case TagTipo::Flotante:
            valor.contenido = *static_cast<float*>(def.acceder(instancia));
            break;
        case TagTipo::Doble:
            valor.contenido = *static_cast<double*>(def.acceder(instancia));
            break;
        case TagTipo::Booleano:
            valor.contenido = *static_cast<bool*>(def.acceder(instancia));
            break;
        case TagTipo::Texto:
            valor.contenido = *static_cast<std::string*>(def.acceder(instancia));
            break;
        case TagTipo::Vec3:
            valor.contenido = *static_cast<vec3*>(def.acceder(instancia));
            break;
        case TagTipo::Enteros:
            valor.contenido = *static_cast<std::vector<int>*>(def.acceder(instancia));
            break;
        case TagTipo::Flotantes:
            valor.contenido = *static_cast<std::vector<float>*>(def.acceder(instancia));
            break;
        case TagTipo::Dobles:
            valor.contenido = *static_cast<std::vector<double>*>(def.acceder(instancia));
            break;
        case TagTipo::Booleanos:
            valor.contenido = *static_cast<std::vector<bool>*>(def.acceder(instancia));
            break;
        case TagTipo::Textos:
            valor.contenido =
                *static_cast<std::vector<std::string>*>(def.acceder(instancia));
            break;
        case TagTipo::Vec3s:
            valor.contenido = *static_cast<std::vector<vec3>*>(def.acceder(instancia));
            break;
        case TagTipo::Grupo: {
            void* base = def.acceder(instancia);
            std::vector<ValorCampo> subs;
            subs.reserve(def.subcampos.size());
            for (const DefCampo& sub : def.subcampos)
                subs.push_back(leerCampo(sub, base));
            valor.contenido = std::move(subs);
            break;
        }
        case TagTipo::Grupos: {
            std::vector<std::vector<ValorCampo>> grupos;
            if (def.contar) {
                const size_t n = def.contar(instancia);
                grupos.reserve(n);
                for (size_t i = 0; i < n; ++i) {
                    void* elemento = def.elemento(instancia, i);
                    std::vector<ValorCampo> subs;
                    subs.reserve(def.subcampos.size());
                    for (const DefCampo& sub : def.subcampos)
                        subs.push_back(leerCampo(sub, elemento));
                    grupos.push_back(std::move(subs));
                }
            }
            valor.contenido = std::move(grupos);
            break;
        }
        case TagTipo::Objeto: {
            GameObject** puntero = static_cast<GameObject**>(def.acceder(instancia));
            valor.contenido =
                std::string(*puntero ? (*puntero)->inputName : "");
            break;
        }
        case TagTipo::Objetos: {
            auto* arreglo =
                static_cast<std::vector<GameObject*>*>(def.acceder(instancia));
            std::vector<std::string> nombres;
            nombres.reserve(arreglo->size());
            for (GameObject* elemento : *arreglo)
                nombres.push_back(elemento ? elemento->inputName : "");
            valor.contenido = std::move(nombres);
            break;
        }
        case TagTipo::Componente: {
            void** puntero = static_cast<void**>(def.acceder(instancia));
            void* componente = puntero ? *puntero : nullptr;
            valor.contenido =
                (componente && g_resolverComp.dueno)
                    ? g_resolverComp.dueno(componente, def.tipoComponente)
                    : std::string{};
            break;
        }
    }
    return valor;
}

void escribirCampo(const DefCampo& def, void* instancia,
                   const ValorCampo& valor) {
    switch (def.tag) {
        case TagTipo::Entero:
            *static_cast<int*>(def.acceder(instancia)) = valor.como<int>();
            break;
        case TagTipo::Flotante:
            *static_cast<float*>(def.acceder(instancia)) = valor.como<float>();
            break;
        case TagTipo::Doble:
            *static_cast<double*>(def.acceder(instancia)) = valor.como<double>();
            break;
        case TagTipo::Booleano:
            *static_cast<bool*>(def.acceder(instancia)) = valor.como<bool>();
            break;
        case TagTipo::Texto:
            *static_cast<std::string*>(def.acceder(instancia)) =
                valor.como<std::string>();
            break;
        case TagTipo::Vec3:
            *static_cast<vec3*>(def.acceder(instancia)) = valor.como<vec3>();
            break;
        case TagTipo::Enteros: {
            auto* arreglo = static_cast<std::vector<int>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<int>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Flotantes: {
            auto* arreglo = static_cast<std::vector<float>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<float>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Dobles: {
            auto* arreglo = static_cast<std::vector<double>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<double>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Booleanos: {
            auto* arreglo = static_cast<std::vector<bool>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<bool>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Textos: {
            auto* arreglo =
                static_cast<std::vector<std::string>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<std::string>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Vec3s: {
            auto* arreglo = static_cast<std::vector<vec3>*>(def.acceder(instancia));
            const auto& entrada = valor.como<std::vector<vec3>>();
            arreglo->assign(entrada.begin(), entrada.end());
            break;
        }
        case TagTipo::Grupo: {
            void* base = def.acceder(instancia);
            const auto& subs = valor.como<std::vector<ValorCampo>>();
            for (size_t i = 0; i < def.subcampos.size(); ++i) {
                const DefCampo& sub = def.subcampos[i];
                const ValorCampo* subvalor = valorDeSubcampo(subs, sub, i);
                if (subvalor) escribirCampo(sub, base, *subvalor);
            }
            break;
        }
        case TagTipo::Grupos: {
            const auto& grupos = valor.como<std::vector<std::vector<ValorCampo>>>();
            if (def.redimensionar) def.redimensionar(instancia, grupos.size());
            if (!def.contar) break;
            const size_t n = def.contar(instancia);
            for (size_t i = 0; i < n && i < grupos.size(); ++i) {
                void* elemento = def.elemento(instancia, i);
                for (size_t j = 0; j < def.subcampos.size(); ++j) {
                    const DefCampo& sub = def.subcampos[j];
                    const ValorCampo* subvalor =
                        valorDeSubcampo(grupos[i], sub, j);
                    if (subvalor) escribirCampo(sub, elemento, *subvalor);
                }
            }
            break;
        }
        case TagTipo::Objeto: {
            GameObject** puntero = static_cast<GameObject**>(def.acceder(instancia));
            const std::string& nombre = valor.como<std::string>();
            *puntero = (nombre.empty() || !g_resolver) ? nullptr : g_resolver(nombre);
            break;
        }
        case TagTipo::Objetos: {
            auto* arreglo =
                static_cast<std::vector<GameObject*>*>(def.acceder(instancia));
            const auto& nombres = valor.como<std::vector<std::string>>();
            arreglo->clear();
            arreglo->reserve(nombres.size());
            for (const std::string& nombre : nombres)
                arreglo->push_back(nombre.empty() || !g_resolver
                                       ? nullptr
                                       : g_resolver(nombre));
            break;
        }
        case TagTipo::Componente: {
            void** puntero = static_cast<void**>(def.acceder(instancia));
            const std::string& nombre = valor.como<std::string>();
            *puntero = (nombre.empty() || !g_resolverComp.resolver)
                           ? nullptr
                           : g_resolverComp.resolver(
                                 nombre, def.tipoComponente);
            break;
        }
    }
}

}  // namespace ReflejoScripts
