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
#include "BehaviourReflection.h"

#include <algorithm>

#include "../../Objetos/GameObject.h"

namespace ReflejoScripts {

namespace {
// Resolver activo de GameObject por nombre (lo fija la escena en play mode).
ResolverObjeto g_resolver;
} // namespace

void fijarResolverObjetos(ResolverObjeto resolver) {
    g_resolver = std::move(resolver);
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
    }
    return valor;
}

namespace {
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
} // namespace

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
    }
}

// ---------------------------------------------------------------------------
// Serializacion binaria del arbol de valores (autodescriptivo)
// ---------------------------------------------------------------------------
namespace {

void escribirCadena(std::ofstream& out, const std::string& texto) {
    const uint32_t n = static_cast<uint32_t>(texto.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    out.write(texto.data(), static_cast<std::streamsize>(n));
}

std::string leerCadena(std::ifstream& in) {
    uint32_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    std::string texto;
    if (n > 0) {
        texto.resize(n);
        in.read(&texto[0], static_cast<std::streamsize>(n));
    }
    return texto;
}

void escribirTag(std::ofstream& out, TagTipo tag) {
    const uint8_t byte = static_cast<uint8_t>(tag);
    out.write(reinterpret_cast<const char*>(&byte), sizeof(byte));
}

TagTipo leerTag(std::ifstream& in) {
    uint8_t byte = 0;
    in.read(reinterpret_cast<char*>(&byte), sizeof(byte));
    return static_cast<TagTipo>(byte);
}

template <class T>
void escribirContenedor(std::ofstream& out,
                        std::function<void(std::ofstream&, const T&)> escribirUno,
                        const std::vector<T>& lista) {
    const uint32_t n = static_cast<uint32_t>(lista.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    for (const T& elemento : lista) escribirUno(out, elemento);
}

// Payload del valor segun su tag (sin nombre ni tag: eso lo escribe el llamador).
void escribirValor(std::ofstream& out, const ValorCampo& valor) {
    switch (valor.tag) {
        case TagTipo::Entero: {
            const int x = valor.como<int>();
            out.write(reinterpret_cast<const char*>(&x), sizeof(x));
            break;
        }
        case TagTipo::Flotante: {
            const float x = valor.como<float>();
            out.write(reinterpret_cast<const char*>(&x), sizeof(x));
            break;
        }
        case TagTipo::Doble: {
            const double x = valor.como<double>();
            out.write(reinterpret_cast<const char*>(&x), sizeof(x));
            break;
        }
        case TagTipo::Booleano: {
            const uint8_t x = valor.como<bool>() ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&x), sizeof(x));
            break;
        }
        case TagTipo::Texto: escribirCadena(out, valor.como<std::string>()); break;
        case TagTipo::Vec3: {
            const vec3& x = valor.como<vec3>();
            out.write(reinterpret_cast<const char*>(&x.x), sizeof(x.x));
            out.write(reinterpret_cast<const char*>(&x.y), sizeof(x.y));
            out.write(reinterpret_cast<const char*>(&x.z), sizeof(x.z));
            break;
        }
        case TagTipo::Enteros:
            escribirContenedor<int>(out,
                [](std::ofstream& o, const int& e) { o.write(reinterpret_cast<const char*>(&e), sizeof(e)); },
                valor.como<std::vector<int>>());
            break;
        case TagTipo::Flotantes:
            escribirContenedor<float>(out,
                [](std::ofstream& o, const float& e) { o.write(reinterpret_cast<const char*>(&e), sizeof(e)); },
                valor.como<std::vector<float>>());
            break;
        case TagTipo::Dobles:
            escribirContenedor<double>(out,
                [](std::ofstream& o, const double& e) { o.write(reinterpret_cast<const char*>(&e), sizeof(e)); },
                valor.como<std::vector<double>>());
            break;
        case TagTipo::Booleanos: {
            const auto& lista = valor.como<std::vector<bool>>();
            const uint32_t m = static_cast<uint32_t>(lista.size());
            out.write(reinterpret_cast<const char*>(&m), sizeof(m));
            for (bool e : lista) {
                const uint8_t x = e ? 1 : 0;
                out.write(reinterpret_cast<const char*>(&x), sizeof(x));
            }
            break;
        }
        case TagTipo::Textos:
            escribirContenedor<std::string>(out,
                [](std::ofstream& o, const std::string& e) { escribirCadena(o, e); },
                valor.como<std::vector<std::string>>());
            break;
        case TagTipo::Vec3s:
            escribirContenedor<vec3>(out,
                [](std::ofstream& o, const vec3& e) {
                    o.write(reinterpret_cast<const char*>(&e.x), sizeof(e.x));
                    o.write(reinterpret_cast<const char*>(&e.y), sizeof(e.y));
                    o.write(reinterpret_cast<const char*>(&e.z), sizeof(e.z));
                },
                valor.como<std::vector<vec3>>());
            break;
        case TagTipo::Grupo:
            escribirContenedor<ValorCampo>(out,
                [](std::ofstream& o, const ValorCampo& e) {
                    escribirCadena(o, e.nombre);
                    escribirTag(o, e.tag);
                    escribirValor(o, e);
                },
                valor.como<std::vector<ValorCampo>>());
            break;
        case TagTipo::Grupos: {
            const auto& lista = valor.como<std::vector<std::vector<ValorCampo>>>();
            const uint32_t m = static_cast<uint32_t>(lista.size());
            out.write(reinterpret_cast<const char*>(&m), sizeof(m));
            for (const auto& grupo : lista)
                escribirContenedor<ValorCampo>(out,
                    [](std::ofstream& o, const ValorCampo& e) {
                        escribirCadena(o, e.nombre);
                        escribirTag(o, e.tag);
                        escribirValor(o, e);
                    },
                    grupo);
            break;
        }
        case TagTipo::Objeto: escribirCadena(out, valor.como<std::string>()); break;
        case TagTipo::Objetos:
            escribirContenedor<std::string>(out,
                [](std::ofstream& o, const std::string& e) { escribirCadena(o, e); },
                valor.como<std::vector<std::string>>());
            break;
    }
}

} // namespace

void guardarValoresCampos(std::ofstream& out,
                          const std::vector<ValorCampo>& valores) {
    escribirContenedor<ValorCampo>(out,
        [](std::ofstream& o, const ValorCampo& e) {
            escribirCadena(o, e.nombre);
            escribirTag(o, e.tag);
            escribirValor(o, e);
        },
        valores);
}

// Lee UN valor por tag (supone escribirTag/guardarValoresCampos del llamador).
// El arbol se reconstruye de forma autodescriptiva (los grupos llevan sus
// valores hijo con nombre + tag).
namespace {
void leerValor(std::ifstream& in, ValorCampo& valor) {
    switch (valor.tag) {
        case TagTipo::Entero: {
            int x = 0;
            in.read(reinterpret_cast<char*>(&x), sizeof(x));
            valor.contenido = x;
            break;
        }
        case TagTipo::Flotante: {
            float x = 0.0f;
            in.read(reinterpret_cast<char*>(&x), sizeof(x));
            valor.contenido = x;
            break;
        }
        case TagTipo::Doble: {
            double x = 0.0;
            in.read(reinterpret_cast<char*>(&x), sizeof(x));
            valor.contenido = x;
            break;
        }
        case TagTipo::Booleano: {
            uint8_t x = 0;
            in.read(reinterpret_cast<char*>(&x), sizeof(x));
            valor.contenido = (x != 0);
            break;
        }
        case TagTipo::Texto: valor.contenido = leerCadena(in); break;
        case TagTipo::Vec3: {
            vec3 x;
            in.read(reinterpret_cast<char*>(&x.x), sizeof(x.x));
            in.read(reinterpret_cast<char*>(&x.y), sizeof(x.y));
            in.read(reinterpret_cast<char*>(&x.z), sizeof(x.z));
            valor.contenido = x;
            break;
        }
        case TagTipo::Enteros: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<int> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Flotantes: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<float> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Dobles: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<double> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Booleanos: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<bool> lista(n);
            for (size_t i = 0; i < n; ++i) {
                uint8_t x = 0;
                in.read(reinterpret_cast<char*>(&x), sizeof(x));
                lista[i] = (x != 0);
            }
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Textos: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<std::string> lista(n);
            for (auto& e : lista) e = leerCadena(in);
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Vec3s: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<vec3> lista(n);
            for (auto& e : lista) {
                in.read(reinterpret_cast<char*>(&e.x), sizeof(e.x));
                in.read(reinterpret_cast<char*>(&e.y), sizeof(e.y));
                in.read(reinterpret_cast<char*>(&e.z), sizeof(e.z));
            }
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Grupo: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<ValorCampo> subs;
            subs.reserve(n);
            for (uint32_t i = 0; i < n; ++i) {
                ValorCampo sub;
                sub.nombre = leerCadena(in);
                sub.tag = leerTag(in);
                leerValor(in, sub);
                subs.push_back(std::move(sub));
            }
            valor.contenido = std::move(subs);
            break;
        }
        case TagTipo::Grupos: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<std::vector<ValorCampo>> grupos;
            grupos.reserve(n);
            for (uint32_t g = 0; g < n; ++g) {
                uint32_t m = 0;
                in.read(reinterpret_cast<char*>(&m), sizeof(m));
                std::vector<ValorCampo> subs;
                subs.reserve(m);
                for (uint32_t i = 0; i < m; ++i) {
                    ValorCampo sub;
                    sub.nombre = leerCadena(in);
                    sub.tag = leerTag(in);
                    leerValor(in, sub);
                    subs.push_back(std::move(sub));
                }
                grupos.push_back(std::move(subs));
            }
            valor.contenido = std::move(grupos);
            break;
        }
        case TagTipo::Objeto: valor.contenido = leerCadena(in); break;
        case TagTipo::Objetos: {
            uint32_t n = 0;
            in.read(reinterpret_cast<char*>(&n), sizeof(n));
            std::vector<std::string> lista(n);
            for (auto& e : lista) e = leerCadena(in);
            valor.contenido = std::move(lista);
            break;
        }
    }
}
} // namespace

std::vector<ValorCampo> leerValoresCrudos(std::ifstream& in) {
    uint32_t n = 0;
    in.read(reinterpret_cast<char*>(&n), sizeof(n));
    std::vector<ValorCampo> leidos;
    leidos.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        ValorCampo valor;
        valor.nombre = leerCadena(in);
        valor.tag = leerTag(in);
        leerValor(in, valor);
        leidos.push_back(std::move(valor));
    }
    return leidos;
}

std::vector<ValorCampo> cargarValoresCampos(std::ifstream& in) {
    return leerValoresCrudos(in);
}

std::vector<ValorCampo> alinearValores(std::vector<ValorCampo> leidos,
                                       const std::vector<DefCampo>& defs) {
    // Casa los valores leidos con los defs actuales POR NOMBRE (robusto a
    // reordenamientos y a campos agregados/quitados en versiones nuevas del
    // script): los campos desconocidos se ignoran y los nuevos quedan con su
    // valor por defecto. El resultado tiene SIEMPRE un valor por DefCampo y en
    // el mismo orden, que es el contrato que espera el resto del motor.
    std::vector<ValorCampo> resultado;
    resultado.reserve(defs.size());
    for (const DefCampo& def : defs) {
        bool casado = false;
        for (size_t i = 0; i < leidos.size(); ++i) {
            if (leidos[i].nombre == def.nombre) {
                resultado.push_back(std::move(leidos[i]));
                casado = true;
                break;
            }
        }
        if (!casado) resultado.push_back(valorPorDefecto(def));
    }
    return resultado;
}

std::vector<ValorCampo> cargarValoresCampos(
    std::ifstream& in, const std::vector<DefCampo>& defs) {
    return alinearValores(leerValoresCrudos(in), defs);
}

} // namespace ReflejoScripts