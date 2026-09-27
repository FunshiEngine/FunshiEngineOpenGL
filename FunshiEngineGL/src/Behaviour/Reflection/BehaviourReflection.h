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
#ifndef BEHAVIOURREFLECTION_H
#define BEHAVIOURREFLECTION_H

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "../../Matematicas/StructVec3.h"

// Reflexion de campos de los comportamientos (IScriptBehaviour). Es el motor de
// SerializeField: un comportamiento declara sus campos editables via las macros
// REFLECT_*, el engine las enumera y puede LEER/ESCRIBIR cada campo sobre el
// objeto, serializarlos a .escena y mostrarlos en la GUI. Soportados:
//   - primitivas: int, float, double, bool, std::string, vec3
//   - vectores de primitivas (std::vector<T>, T no GameObject)
//   - grupos anidados: struct reflejado como miembro (Grupo)
//   - grupos en vector: std::vector<S> de structs reflejados (Grupos)
//   - referencias a GameObject: GameObject* y std::vector<GameObject*>
//     (se guardan por el nombre del objeto, no por puntero)
class GameObject;

namespace ReflejoScripts {

// Tipos de campo soportados por la reflexion.
enum class TagTipo : uint8_t {
    Entero,    // int
    Flotante,  // float
    Doble,     // double
    Booleano,  // bool
    Texto,     // std::string
    Vec3,      // vec3
    Enteros,   // std::vector<int>
    Flotantes, // std::vector<float>
    Dobles,    // std::vector<double>
    Booleanos, // std::vector<bool>
    Textos,    // std::vector<std::string>
    Vec3s,     // std::vector<vec3>
    Grupo,     // struct anidado reflejado
    Grupos,    // std::vector<struct> reflejado
    Objeto,    // GameObject* (guardado como nombre)
    Objetos    // std::vector<GameObject*> (guardado como nombres)
};

const char* nombreDeTag(TagTipo tag);

// Valor reflectado de un campo. `tag` dice que alternativa de `contenido` esta
// activa y `nombre` es el del campo (util para emparejar al persistir).
//   - primitiva simple / Objeto (string con el nombre)   -> contenido simple
//   - vector primitivo / Objetos (vector<string>)        -> contenido lista
//   - Grupo                                              -> vector<ValorCampo>
//   - Grupos                                             -> vector<vector<ValorCampo>>
struct ValorCampo {
    std::string nombre;
    TagTipo tag = TagTipo::Entero;
    std::variant<int, float, double, bool, std::string, vec3,
                 std::vector<int>, std::vector<float>, std::vector<double>,
                 std::vector<bool>, std::vector<std::string>, std::vector<vec3>,
                 std::vector<ValorCampo>,
                 std::vector<std::vector<ValorCampo>>> contenido;

    template <class T>
    const T& como() const {
        return std::get<T>(contenido);
    }
    template <class T>
    T& como() {
        return std::get<T>(contenido);
    }
};

// Define UN campo reflectado como { nombre, tag, descriptores de acceso }.
// Los descriptores son lambdas generadas por las macros/seccion que conoce el
// tipo concreto del comportamiento; todo el codigo generico de serializar o
// editar se apoya en ellos para no depender del layout de la clase.
struct DefCampo {
    std::string nombre;
    TagTipo tag = TagTipo::Entero;

    // Devuelve un puntero al dato del campo dentro de `instancia`.
    // Para Grupo apunta al struct anidado; para Grupos al std::vector<S>;
    // para Objeto a un GameObject*; para el resto al valor/vector.
    std::function<void*(void* instancia)> acceder;

    // SubCampos de un Grupo/Grupos (S::reflexion()).
    std::vector<DefCampo> subcampos;

    // Solo para Grupos: tamaño e indizado del std::vector<S>.
    std::function<size_t(void* instancia)> contar;
    std::function<void*(void* instancia, size_t i)> elemento;
    std::function<void(void* instancia, size_t n)> redimensionar;
};

// Lee el valor ACTUAL de un campo del objeto y lo replica en el arbol ValorCampo.
ValorCampo leerCampo(const DefCampo& def, void* instancia);

// Escribe el valor almacenado en `valor` sobre el objeto (inyeccion).
void escribirCampo(const DefCampo& def, void* instancia, const ValorCampo& valor);

// Valor por defecto (cero/vacio) de un campo, util para inicializar el
// almacenamiento cuando todavia no se expuso el comportamiento.
ValorCampo valorPorDefecto(const DefCampo& def);
std::vector<ValorCampo> valoresPorDefecto(const std::vector<DefCampo>& defs);

// --- Serializacion binaria del arbol de valores (SerializeField en .escena) --
// El arbol es autodescriptivo (cada valor lleva nombre + tag), por lo que no
// depende de los DefCampo para leerse; al cargar se reordena para casar con los
// oscuros actuales y se rellenan con los valores por defecto los campos nuevos.
// Casa un arbol de valores ya leido con los DefCampo actuales POR NOMBRE:
// conserva el valor de cada campo que sigue existiendo (aunque se haya
// reordenado), descarta los que ya no estan en el script y rellena con el
// valor por defecto los que se agregaron. El vector devuelto tiene exactamente
// un valor por DefCampo, en el mismo orden: es el invariante que consume el
// resto del motor (inyeccion, edicion en el inspector y guardado).
std::vector<ValorCampo> alinearValores(std::vector<ValorCampo> leidos,
                                       const std::vector<DefCampo>& defs);
void guardarValoresCampos(std::ofstream& out,
                          const std::vector<ValorCampo>& valores);
std::vector<ValorCampo> cargarValoresCampos(
    std::ifstream& in, const std::vector<DefCampo>& defs);
// Sin defs (p. ej. al cargar la escena sin haber compilado el script): devuelve
// el arbol tal cual quedo escrito. El emparejado con los defs reales no puede
// ocurrir aqui porque aun no se conocen; lo hace Script al compilar el script,
// llamando a alinearValores() con los campos que devolvio la reflexion.
std::vector<ValorCampo> cargarValoresCampos(std::ifstream& in);

// Resolve el nombre de un GameObject objetivo a su puntero vivo. Lo fija quien
// tiene acceso a la escena actual (GameScene) antes de inyectar campos/objeto.
using ResolverObjeto = std::function<GameObject*(const std::string& nombre)>;
void fijarResolverObjetos(ResolverObjeto resolver);

// --- Construccion generica de DefCampo (usada por las macros REFLECT_*) ---

template <class T>
constexpr bool EsVector = false;
template <class T, class A>
constexpr bool EsVector<std::vector<T, A>> = true;

// tag de un tipo singular (int, float, ..., std::string, vec3, GameObject*)
template <class T>
constexpr TagTipo tagDeSingular();
template <>
constexpr TagTipo tagDeSingular<int>() { return TagTipo::Entero; }
template <>
constexpr TagTipo tagDeSingular<float>() { return TagTipo::Flotante; }
template <>
constexpr TagTipo tagDeSingular<double>() { return TagTipo::Doble; }
template <>
constexpr TagTipo tagDeSingular<bool>() { return TagTipo::Booleano; }
template <>
constexpr TagTipo tagDeSingular<std::string>() { return TagTipo::Texto; }
template <>
constexpr TagTipo tagDeSingular<vec3>() { return TagTipo::Vec3; }
template <>
constexpr TagTipo tagDeSingular<GameObject*>() { return TagTipo::Objeto; }

// tag de un elemento de vector (enumera tambien vector<GameObject*>)
template <class T>
constexpr TagTipo tagDeLista();
template <>
constexpr TagTipo tagDeLista<int>() { return TagTipo::Enteros; }
template <>
constexpr TagTipo tagDeLista<float>() { return TagTipo::Flotantes; }
template <>
constexpr TagTipo tagDeLista<double>() { return TagTipo::Dobles; }
template <>
constexpr TagTipo tagDeLista<bool>() { return TagTipo::Booleanos; }
template <>
constexpr TagTipo tagDeLista<std::string>() { return TagTipo::Textos; }
template <>
constexpr TagTipo tagDeLista<vec3>() { return TagTipo::Vec3s; }
template <>
constexpr TagTipo tagDeLista<GameObject*>() { return TagTipo::Objetos; }

// Campo de primitiva / GameObject* (miembro escalar).
template <class C, class T,
          std::enable_if_t<!EsVector<T>, int> = 0>
DefCampo crearCampo(const char* nombre, T C::* miembro) {
    DefCampo d;
    d.nombre = nombre;
    d.tag = tagDeSingular<T>();
    d.acceder = [miembro](void* instancia) -> void* {
        C* c = static_cast<C*>(instancia);
        return &(c->*miembro);
    };
    return d;
}

// Campo vector de primitivas / vector<GameObject*>.
template <class C, class T>
DefCampo crearCampo(const char* nombre, std::vector<T> C::* miembro) {
    DefCampo d;
    d.nombre = nombre;
    d.tag = tagDeLista<T>();
    d.acceder = [miembro](void* instancia) -> void* {
        C* c = static_cast<C*>(instancia);
        return &(c->*miembro);
    };
    return d;
}

// Grupo: struct anidado reflejado (sus subcampos se generan con S::reflexion()).
template <class C, class S, std::enable_if_t<!EsVector<S>, int> = 0>
DefCampo crearGrupo(const char* nombre, S C::* miembro,
                    std::vector<DefCampo> subcampos) {
    DefCampo d;
    d.nombre = nombre;
    d.tag = TagTipo::Grupo;
    d.subcampos = std::move(subcampos);
    d.acceder = [miembro](void* instancia) -> void* {
        C* c = static_cast<C*>(instancia);
        return &(c->*miembro);
    };
    return d;
}

// Grupos: vector de structs reflejados. Los descriptores de indizado conocen el
// tipo de elemento (S) en la TU del script; el codigo generico solo los usa.
template <class C, class V, std::enable_if_t<EsVector<V>, int> = 0>
DefCampo crearGrupos(const char* nombre, V C::* miembro,
                     std::vector<DefCampo> subcampos) {
    DefCampo d;
    d.nombre = nombre;
    d.tag = TagTipo::Grupos;
    d.subcampos = std::move(subcampos);
    d.acceder = [miembro](void* instancia) -> void* {
        C* c = static_cast<C*>(instancia);
        return &(c->*miembro);
    };
    d.contar = [miembro](void* instancia) -> size_t {
        C* c = static_cast<C*>(instancia);
        return (c->*miembro).size();
    };
    d.elemento = [miembro](void* instancia, size_t i) -> void* {
        C* c = static_cast<C*>(instancia);
        return static_cast<void*>(&((c->*miembro)[i]));
    };
    d.redimensionar = [miembro](void* instancia, size_t n) {
        C* c = static_cast<C*>(instancia);
        (c->*miembro).resize(n);
    };
    return d;
}

// --- Macros de declaracion de campos en un comportamiento / struct ---
// Uso dentro de la clase:
//   REFLECT_INICIO(MiScript)
//       REFLECT_CAMPO(vidas)
//       REFLECT_ARRAY(puntos)
//       REFLECT_GRUPO(misil)        // struct Misil{...} con su propio REFLECT_*
//       REFLECT_GRUPOS(oleadas)     // std::vector<Oleada>
//       REFLECT_CAMPO(objetivo)     // GameObject*
//   REFLECT_FIN
//
// REFLECT_INICIO define el alias local ClaseReflejada (la clase que se esta
// reflejando); los macros de campo lo usan para formar los punteros a miembro.
#define REFLECT_INICIO(CLASE) \
    public: \
    static std::vector<::ReflejoScripts::DefCampo> reflexion() { \
        std::vector<::ReflejoScripts::DefCampo> r; \
        using ClaseReflejada = CLASE;

#define REFLECT_CAMPO(M) \
        r.push_back(::ReflejoScripts::crearCampo(#M, &ClaseReflejada::M));

#define REFLECT_ARRAY(M) REFLECT_CAMPO(M)

#define REFLECT_GRUPO(M) \
        r.push_back(::ReflejoScripts::crearGrupo( \
            #M, &ClaseReflejada::M, decltype(ClaseReflejada::M)::reflexion()));

#define REFLECT_GRUPOS(M) \
        r.push_back(::ReflejoScripts::crearGrupos( \
            #M, &ClaseReflejada::M, \
            decltype(ClaseReflejada::M)::value_type::reflexion()));

#define REFLECT_FIN \
        return r; \
    }

} // namespace ReflejoScripts

#endif // BEHAVIOURREFLECTION_H