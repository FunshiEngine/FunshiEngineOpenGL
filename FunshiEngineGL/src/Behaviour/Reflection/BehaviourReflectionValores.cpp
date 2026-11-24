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

// ---------------------------------------------------------------------------
// Serializacion binaria del arbol de valores (autodescriptivo)
// ---------------------------------------------------------------------------
void escribirCadena(std::ofstream& out, const std::string& texto) {
    const uint32_t n = static_cast<uint32_t>(texto.size());
    out.write(reinterpret_cast<const char*>(&n), sizeof(n));
    out.write(texto.data(), static_cast<std::streamsize>(n));
}

std::string leerCadena(std::ifstream& in) {
    uint32_t n = 0;
    // La longitud la controla el archivo: sin cota, un valor enorme hace que
    // la reserva lance `std::length_error` y el motor aborte.
    if (!LimitesSerializacion::leerLongitud(
            in, n, LimitesSerializacion::kTexto, 1, 0)) {
        in.setstate(std::ios::failbit);
        return std::string{};
    }
    std::string texto;
    if (n > 0) {
        texto.resize(n);
        in.read(&texto[0], static_cast<std::streamsize>(n));
    }
    return texto;
}

TagTipo leerTag(std::ifstream& in) {
    uint8_t byte = 0;
    in.read(reinterpret_cast<char*>(&byte), sizeof(byte));
    if (!in) {
        in.setstate(std::ios::failbit);
        return TagTipo::Entero;
    }
    // Una etiqueta fuera de rango no corresponde a ningun `case`: el payload
    // no se consumiria y todo lo que sigue en el archivo seria basura.
    if (byte > static_cast<uint8_t>(TagTipo::Componente)) {
        in.setstate(std::ios::failbit);
        return TagTipo::Entero;
    }
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
        case TagTipo::Componente: escribirCadena(out, valor.como<std::string>()); break;
    }
}

void leerValor(std::ifstream& in, ValorCampo& valor,
               unsigned profundidad = 0) {
    if (profundidad > kProfundidadMaxima) {
        in.setstate(std::ios::failbit);
        return;
    }
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
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(int), 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<int> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Flotantes: {
            uint32_t n = 0;
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(float), 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<float> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Dobles: {
            uint32_t n = 0;
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(double), 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<double> lista(n);
            for (auto& e : lista) in.read(reinterpret_cast<char*>(&e), sizeof(e));
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Booleanos: {
            uint32_t n = 0;
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, 1, 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
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
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, 1, 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<std::string> lista(n);
            for (auto& e : lista) e = leerCadena(in);
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Vec3s: {
            uint32_t n = 0;
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(vec3), 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
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
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(uint32_t),
                    0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<ValorCampo> subs;
            subs.reserve(n);
            for (uint32_t i = 0; i < n; ++i) {
                ValorCampo sub;
                sub.nombre = leerCadena(in);
                sub.tag = leerTag(in);
                leerValor(in, sub, profundidad + 1);
                if (!in) return;
                subs.push_back(std::move(sub));
            }
            valor.contenido = std::move(subs);
            break;
        }
        case TagTipo::Grupos: {
            uint32_t n = 0;
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, sizeof(uint32_t),
                    0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<std::vector<ValorCampo>> grupos;
            grupos.reserve(n);
            for (uint32_t g = 0; g < n; ++g) {
                uint32_t m = 0;
                if (!LimitesSerializacion::leerLongitud(
                        in, m, LimitesSerializacion::kCantidad,
                        sizeof(uint32_t), 0)) {
                    in.setstate(std::ios::failbit);
                    return;
                }
                std::vector<ValorCampo> subs;
                subs.reserve(m);
                for (uint32_t i = 0; i < m; ++i) {
                    ValorCampo sub;
                    sub.nombre = leerCadena(in);
                    sub.tag = leerTag(in);
                    leerValor(in, sub, profundidad + 1);
                    if (!in) return;
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
            if (!LimitesSerializacion::leerLongitud(
                    in, n, LimitesSerializacion::kCantidad, 1, 0)) {
                in.setstate(std::ios::failbit);
                break;
            }
            std::vector<std::string> lista(n);
            for (auto& e : lista) e = leerCadena(in);
            valor.contenido = std::move(lista);
            break;
        }
        case TagTipo::Componente: valor.contenido = leerCadena(in); break;
    }
}

}  // namespace

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

std::vector<ValorCampo> leerValoresCrudos(std::ifstream& in) {
    uint32_t n = 0;
    if (!LimitesSerializacion::leerLongitud(
            in, n, LimitesSerializacion::kCantidad, sizeof(uint32_t), 0)) {
        in.setstate(std::ios::failbit);
        return {};
    }
    std::vector<ValorCampo> leidos;
    leidos.reserve(n);
    for (uint32_t i = 0; i < n; ++i) {
        ValorCampo valor;
        valor.nombre = leerCadena(in);
        valor.tag = leerTag(in);
        leerValor(in, valor);
        if (!in) return {};
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
                ValorCampo valor = std::move(leidos[i]);
                if (valor.tag != def.tag) {
                    // El tipo del campo cambio en el script: el valor guardado
                    // no se puede reinterpretar (la alternativa activa del
                    // variant no corresponde a su etiqueta) y el inspector
                    // lanzaria `std::bad_variant_access` al leerlo. Se usa el
                    // valor por defecto del campo actual.
                    valor = valorPorDefecto(def);
                }
                resultado.push_back(std::move(valor));
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

}  // namespace ReflejoScripts
