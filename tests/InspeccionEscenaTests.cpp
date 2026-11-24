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
#include "InspeccionEscenaTestsDetalle.h"

namespace InspeccionEscenaTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace InspeccionEscenaTestsDetalle
using namespace InspeccionEscenaTestsDetalle;

// Crea un objeto por la API. Todos los objetos nacen con un Transform (lo
// agrega Entity al construirlos), asi que aqui no hay que agregarlo.
int crearObjeto(Contexto& ctx, const std::string& nombre, int idPadre) {
    std::string error;
    int id = -1;
    if (!ctx.api.crearObjeto(nombre, idPadre, id, error)) return -1;
    return id;
}

// Busca el valor de una variable en la vista de componentes.
std::string valorVariable(const std::vector<VistaComponente>& componentes,
                          const std::string& tipo, const std::string& variable) {
    for (const VistaComponente& componente : componentes) {
        if (componente.tipo != tipo) continue;
        for (const auto& v : componente.variables) {
            if (v.nombre == variable) return v.valor;
        }
    }
    return std::string();
}

// --- Conversiones texto <-> valor ------------------------------------------

void probarTextoValor() {
    std::string error;

    CHECK(MotorInspeccion::textoEntero(-7) == "-7",
          "el entero se imprime con signo");
    CHECK(MotorInspeccion::textoBooleano(true) == "true",
          "el booleano se imprime como texto");
    CHECK(MotorInspeccion::textoFlotante(0.1f) == "0.1",
          "el flotante usa la representacion mas corta que se relee igual");

    float valor = 0.0f;
    CHECK(MotorInspeccion::leerFlotante("1.5", valor, error) && valor == 1.5f,
          "un numero valido se lee");
    CHECK(!MotorInspeccion::leerFlotante("abc", valor, error) && !error.empty(),
          "un texto que no es numero se rechaza con motivo");
    CHECK(!MotorInspeccion::leerFlotante("", valor, error),
          "un valor vacio se rechaza");
    CHECK(!MotorInspeccion::leerFlotante("nan", valor, error),
          "un NaN se rechaza (romperia la matriz de transformacion)");
    CHECK(!MotorInspeccion::leerFlotante("inf", valor, error),
          "un infinito se rechaza");
    CHECK(MotorInspeccion::leerFlotante("0.1", valor, error) && valor == 0.1f,
          "el flotante releido es el mismo");

    bool booleano = false;
    CHECK(MotorInspeccion::leerBooleano("TRUE", booleano, error) && booleano,
          "el booleano no distingue mayusculas");
    CHECK(MotorInspeccion::leerBooleano("0", booleano, error) && !booleano,
          "el 0 es falso");
    CHECK(!MotorInspeccion::leerBooleano("quiza", booleano, error),
          "un booleano invalido se rechaza");

    int entero = 0;
    CHECK(MotorInspeccion::leerEntero("42", entero, error) && entero == 42,
          "un entero valido se lee");
    CHECK(!MotorInspeccion::leerEntero("4.2", entero, error),
          "un entero con decimales se rechaza");

    float x = 0.0f, y = 0.0f, z = 0.0f;
    CHECK(MotorInspeccion::leerVec3("1 2 3", x, y, z, error) && x == 1.0f &&
              y == 2.0f && z == 3.0f,
          "un vector separado por espacios se lee");
    CHECK(MotorInspeccion::leerVec3("1,2,3", x, y, z, error) && x == 1.0f &&
              y == 2.0f && z == 3.0f,
          "un vector separado por comas se lee");
    CHECK(!MotorInspeccion::leerVec3("1 2", x, y, z, error),
          "un vector con menos numeros se rechaza");
    CHECK(!MotorInspeccion::leerVec3("1 2 x", x, y, z, error),
          "un vector con un numero invalido se rechaza");
}

// --- Catalogo de propiedades ------------------------------------------------

void probarCatalogo() {
    const MotorInspeccion::CatalogoPropiedades& catalogo =
        MotorInspeccion::CatalogoPropiedades::instancia();

    CHECK(catalogo.de("Transform") != nullptr,
          "el catalogo conoce las variables del Transform");
    CHECK(catalogo.de("RigidBody") != nullptr,
          "el catalogo conoce las variables del RigidBody");
    CHECK(catalogo.de("Light") != nullptr,
          "el catalogo conoce las variables de la Light");
    CHECK(catalogo.de("CameraComponent") != nullptr,
          "el catalogo conoce las variables de la camara");
    CHECK(catalogo.de("TipoQueNoExiste") == nullptr,
          "un tipo desconocido no tiene variables");

    CHECK(catalogo.buscar("Transform", "position") != nullptr,
          "la variable position existe en el Transform");
    CHECK(catalogo.buscar("Transform", "Position") != nullptr,
          "la busqueda de variable no distingue mayusculas");
    CHECK(catalogo.buscar("Transform", "noExiste") == nullptr,
          "una variable inexistente no se encuentra");

    bool ordenado = true;
    const std::vector<std::string>& tipos = catalogo.tipos();
    for (std::size_t i = 1; i < tipos.size(); ++i) {
        if (tipos[i - 1] > tipos[i]) ordenado = false;
    }
    CHECK(ordenado, "los tipos de componente van en orden alfabetico");
    CHECK(!tipos.empty(), "el catalogo declara algun tipo de componente");
}

// --- Filtros y vistas --------------------------------------------------------

void probarFiltros() {
    Contexto ctx;
    std::string error;

    int enemigo = crearObjeto(ctx, "Enemigo");
    int aliado = crearObjeto(ctx, "Aliado");
    int jefe = crearObjeto(ctx, "EnemigoJefe");
    CHECK(enemigo > 0 && aliado > 0 && jefe > 0,
          "se crean tres objetos por la API");

    CHECK(ctx.api.establecerEtiqueta(enemigo, "enemigo", error),
          "se etiqueta el primer objeto");
    CHECK(ctx.api.establecerEtiqueta(jefe, "enemigo", error),
          "se etiqueta el tercero");

    CHECK(ctx.api.crearComponente(enemigo, "Light", error),
          "se agrega una luz al primer objeto");
    CHECK(ctx.api.crearComponente(jefe, "Light", error),
          "se agrega una luz al tercero");
    CHECK(ctx.api.crearComponente(jefe, "EsfereCollider", error),
          "se agrega un colisionador al tercero");

    CHECK(ctx.api.listarObjetos(FiltroObjetos{}).size() == 3,
          "sin filtro salen los tres objetos");

    FiltroObjetos porEtiqueta;
    porEtiqueta.etiqueta = "enemigo";
    CHECK(ctx.api.listarObjetos(porEtiqueta).size() == 2,
          "el filtro por etiqueta devuelve los dos etiquetados");

    FiltroObjetos porComponente;
    porComponente.componente = "Light";
    CHECK(ctx.api.listarObjetos(porComponente).size() == 2,
          "el filtro por componente devuelve los que lo tienen");

    FiltroObjetos porAlias;
    porAlias.componente = "Camera";
    CHECK(ctx.api.listarObjetos(porAlias).empty(),
          "un alias de componente sin objetos devuelve una lista vacia");

    FiltroObjetos porTipoDesconocido;
    porTipoDesconocido.componente = "TipoQueNoExiste";
    CHECK(ctx.api.listarObjetos(porTipoDesconocido).empty(),
          "un filtro por componente inexistente no devuelve nada");

    FiltroObjetos porId;
    porId.id = aliado;
    std::vector<VistaObjeto> porIdLista = ctx.api.listarObjetos(porId);
    CHECK(porIdLista.size() == 1 && porIdLista[0].nombre == "Aliado",
          "el filtro por id devuelve el objeto pedido");

    FiltroObjetos porNombre;
    porNombre.nombre = "Enemigo";
    CHECK(ctx.api.listarObjetos(porNombre).size() == 1,
          "el filtro por nombre exacto no confunde prefijos");

    FiltroObjetos porNombreParcial;
    porNombreParcial.nombre = "Enemigo";
    porNombreParcial.nombreParcial = true;
    CHECK(ctx.api.listarObjetos(porNombreParcial).size() == 2,
          "el filtro por nombre parcial encuentra los dos");

    FiltroObjetos porTipoObjeto;
    porTipoObjeto.tipoObjeto = "SimpleObject";
    CHECK(ctx.api.listarObjetos(porTipoObjeto).size() == 3,
          "el filtro por tipo de objeto encuentra los tres");

    FiltroObjetos combinados;
    combinados.etiqueta = "enemigo";
    combinados.componente = "Light";
    CHECK(ctx.api.listarObjetos(combinados).size() == 2,
          "los filtros se acumulan (etiqueta + componente)");

    FiltroObjetos combinadosQueNoCuadran;
    combinadosQueNoCuadran.etiqueta = "enemigo";
    combinadosQueNoCuadran.nombre = "Aliado";
    CHECK(ctx.api.listarObjetos(combinadosQueNoCuadran).empty(),
          "los filtros que se contradicen no devuelven nada");

    FiltroObjetos conLimite;
    conLimite.limite = 1;
    CHECK(ctx.api.listarObjetos(conLimite).size() == 1,
          "el limite recorta el resultado");

    // Vistas.
    std::optional<VistaObjeto> vista = ctx.api.objetoPorId(enemigo);
    CHECK(vista.has_value(), "el objeto existe");
    if (vista) {
        CHECK(vista->nombre == "Enemigo", "la vista trae el nombre");
        CHECK(vista->etiqueta == "enemigo", "la vista trae la etiqueta");
        CHECK(vista->tipoObjeto == "SimpleObject", "la vista trae el tipo");
        CHECK(vista->idPadre == 0, "un objeto de la raiz no tiene padre");
        CHECK(vista->cantidadComponentes == 2,
              "la vista cuenta los componentes (transform y luz)");
    }
    CHECK(!ctx.api.objetoPorId(9999).has_value(),
          "un id inexistente no devuelve vista");
    CHECK(ctx.api.componentesDe(9999).empty(),
          "los componentes de un id inexistente son una lista vacia");

    std::vector<VistaComponente> componentes = ctx.api.componentesDe(enemigo);
    CHECK(componentes.size() == 2, "el objeto tiene dos componentes");
    CHECK(valorVariable(componentes, "Transform", "position") == "0 0 0",
          "la variable position se lee con su valor");
    CHECK(!valorVariable(componentes, "Light", "type").empty(),
          "la luz expone su tipo");
}

// --- Posiciones ---------------------------------------------------------------

void probarPosiciones() {
    Contexto ctx;
    std::string error;

    int padre = crearObjeto(ctx, "Padre");
    int hijo = crearObjeto(ctx, "Hijo", padre);
    CHECK(padre > 0 && hijo > 0, "se crean un padre y un hijo");

    CHECK(ctx.api.asignarVariable(padre, "Transform", "position", "10 0 0",
                                  error),
          "se mueve el padre");
    CHECK(ctx.api.asignarVariable(hijo, "Transform", "position", "5 0 0", error),
          "se mueve el hijo");

    std::array<float, 3> global{};
    CHECK(ctx.api.posicionGlobal(hijo, global), "se consulta la posicion global");
    CHECK(global[0] == 15.0f && global[1] == 0.0f && global[2] == 0.0f,
          "la posicion global del hijo es la compuesta con el padre");

    std::optional<VistaObjeto> vista = ctx.api.objetoPorId(hijo);
    CHECK(vista.has_value() && vista->posicionLocal[0] == 5.0f,
          "la posicion local del hijo es la suya propia");
    CHECK(vista.has_value() && vista->posicionGlobal[0] == 15.0f,
          "la vista distingue la posicion local de la global");
    CHECK(vista.has_value() && vista->idPadre == padre,
          "la vista trae el id del padre");

    std::array<float, 3> fuera{};
    CHECK(!ctx.api.posicionGlobal(9999, fuera),
          "la posicion global de un id inexistente falla");
}

// --- Mutaciones y deshacer -----------------------------------------------------

void probarMutaciones() {
    Contexto ctx;
    std::string error;

    int id = crearObjeto(ctx, "Enemigo");
    CHECK(id > 0, "se crea el objeto");

    // Un componente con prerrequisito incumplido: el RigidBody necesita un
    // Collider y el objeto todavia no tiene ninguno.
    CHECK(!ctx.api.crearComponente(id, "RigidBody", error),
          "un RigidBody sin collider no se puede agregar");
    CHECK(!error.empty(), "el motivo del prerrequisito se explica");
    CHECK(error.find("Collider") != std::string::npos,
          "el motivo nombra el componente que falta");
    CHECK(ctx.api.componentesDe(id).size() == 1,
          "un prerrequisito incumplido no deja componentes a medias");

    // Agregar un componente y deshacerlo.
    CHECK(ctx.api.crearComponente(id, "EsfereCollider", error),
          "se agrega un colisionador");
    CHECK(ctx.api.componentesDe(id).size() == 2,
          "el objeto tiene transform y colisionador");
    CHECK(ctx.editor.deshacer().empty() == false,
          "el deshacer del colisionador devuelve descripcion");
    CHECK(ctx.api.componentesDe(id).size() == 1,
          "tras deshacer queda solo el transform");
    CHECK(!ctx.api.quitarComponente(id, "EsfereCollider", error),
          "quitar un componente que no tiene falla");

    // Un tipo de componente que no existe.
    CHECK(!ctx.api.crearComponente(id, "TipoQueNoExiste", error),
          "un tipo inexistente no se puede agregar");
    CHECK(!error.empty(), "el tipo inexistente se explica");

    // Un componente que el objeto ya tiene.
    CHECK(!ctx.api.crearComponente(id, "Transform", error),
          "no se puede agregar un componente que el objeto ya tiene");
    CHECK(!error.empty(), "el componente duplicado se explica");

    // Asignar una variable.
    CHECK(ctx.api.asignarVariable(id, "Transform", "position", "1 2 3", error),
          "se asigna la posicion");
    std::vector<VistaComponente> componentes = ctx.api.componentesDe(id);
    CHECK(valorVariable(componentes, "Transform", "position") == "1 2 3",
          "la posicion se relee con el valor nuevo");

    GameObject* objeto = ctx.escena.getObjectByID(id);
    CHECK(objeto != nullptr, "el objeto sigue en la escena");
    if (objeto) {
        Transform* transform = objeto->getComponent<Transform>();
        CHECK(transform != nullptr, "el objeto tiene transform");
        if (transform) {
            const float* posicion = transform->getTranslatef();
            CHECK(posicion[0] == 1.0f && posicion[1] == 2.0f &&
                      posicion[2] == 3.0f,
                  "el transform tiene la posicion nueva");
        }
    }

    // Deshacer el ajuste devuelve el valor anterior.
    CHECK(!ctx.editor.deshacer().empty(), "el ajuste se puede deshacer");
    componentes = ctx.api.componentesDe(id);
    CHECK(valorVariable(componentes, "Transform", "position") == "0 0 0",
          "tras deshacer vuelve la posicion anterior");

    // Errores al asignar.
    CHECK(!ctx.api.asignarVariable(id, "Transform", "noExiste", "1", error),
          "una variable inexistente no se puede asignar");
    CHECK(!error.empty(), "la variable inexistente se explica");
    CHECK(!ctx.api.asignarVariable(id, "Transform", "position", "abc", error),
          "un valor que no es numero no se puede asignar");
    CHECK(!ctx.api.asignarVariable(id, "TipoQueNoExiste", "position", "1",
                                   error),
          "un componente inexistente no se puede asignar");
    CHECK(!ctx.api.asignarVariable(id, "Light", "type", "Puntual", error),
          "no se puede asignar a un componente que el objeto no tiene");
    componentes = ctx.api.componentesDe(id);
    CHECK(valorVariable(componentes, "Transform", "position") == "0 0 0",
          "un rechazo no deja el valor a medias");

    // Enumeraciones.
    CHECK(ctx.api.crearComponente(id, "Light", error), "se agrega una luz");
    CHECK(ctx.api.asignarVariable(id, "Light", "type", "Puntual", error),
          "se escribe el tipo de luz por su nombre");
    componentes = ctx.api.componentesDe(id);
    CHECK(valorVariable(componentes, "Light", "type") == "Puntual",
          "el tipo de luz se relee por su nombre");
    CHECK(!ctx.api.asignarVariable(id, "Light", "type", "Laser", error),
          "un valor de enumeracion invalido se rechaza");
    componentes = ctx.api.componentesDe(id);
    CHECK(valorVariable(componentes, "Light", "type") == "Puntual",
          "un valor invalido no cambia el tipo de luz");
}

// --- Objetos y etiquetas --------------------------------------------------------

void probarObjetos() {
    Contexto ctx;
    std::string error;

    int id = -1;
    CHECK(ctx.api.crearObjeto("Enemigo", 0, id, error) && id > 0,
          "se crea un objeto con nombre");
    std::optional<VistaObjeto> vista = ctx.api.objetoPorId(id);
    CHECK(vista.has_value() && vista->nombre == "Enemigo",
          "el objeto creado tiene el nombre pedido");

    // Un nombre repetido recibe sufijo, igual que el nombre por defecto.
    int repetido = -1;
    CHECK(ctx.api.crearObjeto("Enemigo", 0, repetido, error) && repetido > 0,
          "se puede crear un objeto con un nombre ya usado");
    std::optional<VistaObjeto> vistaRepetido = ctx.api.objetoPorId(repetido);
    CHECK(vistaRepetido.has_value() && vistaRepetido->nombre != "Enemigo",
          "el nombre repetido recibe un sufijo");

    // Etiquetas.
    CHECK(ctx.api.establecerEtiqueta(id, "enemigo", error), "se etiqueta");
    CHECK(ctx.api.objetoPorId(id)->etiqueta == "enemigo", "la etiqueta queda");
    CHECK(!ctx.editor.deshacer().empty(), "el cambio de etiqueta se deshace");
    CHECK(ctx.api.objetoPorId(id)->etiqueta == "Untagged",
          "tras deshacer vuelve la etiqueta anterior");

    CHECK(!ctx.api.establecerEtiqueta(9999, "enemigo", error),
          "no se etiqueta un objeto inexistente");

    // Padre inexistente.
    int huerfano = -1;
    CHECK(!ctx.api.crearObjeto("Huerfano", 9999, huerfano, error),
          "no se crea un objeto con un padre inexistente");
    CHECK(!error.empty(), "el padre inexistente se explica");

    // Deshacer la creacion quita el objeto.
    CHECK(!ctx.editor.deshacer().empty(), "la creacion se puede deshacer");
    CHECK(!ctx.api.objetoPorId(huerfano).has_value(),
          "tras deshacer el objeto ya no esta");
}
