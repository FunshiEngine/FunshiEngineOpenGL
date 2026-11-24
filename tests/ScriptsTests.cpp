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
#include "ScriptsTestsDetalle.h"

namespace ScriptsTestsDetalle {

int total = 0;
int fallos = 0;

}  // namespace ScriptsTestsDetalle
using namespace ScriptsTestsDetalle;

bool casiIgual(float a, float b) {
    return std::fabs(a - b) < 1e-5f;
}

bool casiIgual(const vec3& a, const vec3& b) {
    return casiIgual(a.x, b.x) && casiIgual(a.y, b.y) && casiIgual(a.z, b.z);
}

void testValoresPorDefecto() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    CHECK(defs.size() == kCantidadCampos, "17 campos reflejados");
    const std::vector<ValorCampo> valores = valoresPorDefecto(defs);
    CHECK(valores.size() == kCantidadCampos, "hay un valor por campo");

    CHECK(valores[0].tag == TagTipo::Entero &&
              valores[0].como<int>() == 0,
          "default entero");
    CHECK(valores[1].tag == TagTipo::Flotante &&
              casiIgual(valores[1].como<float>(), 0.0f),
          "default flotante");
    CHECK(valores[3].tag == TagTipo::Booleano &&
              valores[3].como<bool>() == false,
          "default booleano");
    CHECK(valores[6].tag == TagTipo::Enteros &&
              valores[6].como<std::vector<int>>().empty(),
          "default enteros");
    CHECK(valores[12].tag == TagTipo::Grupo &&
              valores[12].como<std::vector<ValorCampo>>().size() == 4,
          "grupo default con 4 subcampos");
    CHECK(valores[13].tag == TagTipo::Grupos &&
              valores[13].como<std::vector<std::vector<ValorCampo>>>().empty(),
          "grupos default vacio");
    CHECK(valores[15].tag == TagTipo::Objeto &&
              valores[15].como<std::string>().empty(),
          "objeto default sin referencia");
    CHECK(valores[16].tag == TagTipo::Objetos,
          "vector de objetos default");
}

void testLecturaEscritura() {
    PruebaComportamiento b;
    const std::vector<DefCampo>& defs = defsDePrueba();

    // Lectura de primitivas.
    CHECK(leerCampo(defs[0], &b).como<int>() == 3, "leer vidas");
    CHECK(casiIgual(leerCampo(defs[1], &b).como<float>(), 1.5f), "leer velocidad");
    CHECK(leerCampo(defs[4], &b).como<std::string>() == "Hola", "leer nombre");
    CHECK(casiIgual(leerCampo(defs[5], &b).como<vec3>(), vec3(1, 2, 3)),
          "leer direccion");
    CHECK(leerCampo(defs[9], &b).como<std::vector<bool>>().size() == 2,
          "leer flags");

    // Escritura de primitivas.
    ValorCampo v = valorPorDefecto(defs[0]);
    v.contenido = 99;
    escribirCampo(defs[0], &b, v);
    CHECK(b.vidas == 99, "escribir vidas");

    // Escritura de un vector completo.
    ValorCampo vLista = leerCampo(defs[7], &b);
    vLista.contenido = std::vector<float>{9.0f, 8.0f};
    escribirCampo(defs[7], &b, vLista);
    CHECK(b.ratios.size() == 2 && casiIgual(b.ratios[0], 9.0f) &&
              casiIgual(b.ratios[1], 8.0f),
          "escribir vector de flotantes");

    // Grupo: leer subcampos y tocar uno por su nombre.
    ValorCampo vGrupo = leerCampo(defs[kIndiceMisil], &b);
    std::vector<ValorCampo>& subs = vGrupo.como<std::vector<ValorCampo>>();
    CHECK(subs.size() == 4, "grupo misil con 4 subcampos");
    bool encontroDano = false;
    for (ValorCampo& sub : subs) {
        if (sub.nombre == "dano") {
            sub.contenido = 77;
            encontroDano = true;
        }
    }
    CHECK(encontroDano, "el grupo expone 'dano'");
    escribirCampo(defs[kIndiceMisil], &b, vGrupo);
    CHECK(b.misil.dano == 77, "escribir subcampo del grupo");
    CHECK(casiIgual(b.misil.velocidad, 4.0f), "resto del grupo intacto");
}

void testGruposVector() {
    PruebaComportamiento b;
    const std::vector<DefCampo>& defs = defsDePrueba();
    b.munis.push_back(Misil{});
    b.munis.push_back(Misil{});
    b.munis[0].dano = 5;
    b.munis[0].etiqueta = "pesado";

    ValorCampo vG = leerCampo(defs[kIndiceMunis], &b);
    std::vector<std::vector<ValorCampo>>& grupos =
        vG.como<std::vector<std::vector<ValorCampo>>>();
    CHECK(grupos.size() == 2, "leer 2 munis");
    CHECK(grupos[0].size() == 4, "cada mun se lee completo");
    CHECK(grupos[0][1].nombre == "dano" &&
              grupos[0][1].como<int>() == 5,
          "valores iniciales por nombre");

    // Agregar un tercer elemento y escribir: el vector del objeto se redimensiona.
    grupos.push_back(valoresPorDefecto(defs[kIndiceMunis].subcampos));
    escribirCampo(defs[kIndiceMunis], &b, vG);
    CHECK(b.munis.size() == 3, "grupos vector redimensionado a 3");
    CHECK(b.munis[0].dano == 5 && b.munis[0].etiqueta == "pesado",
          "elementos previos preservados");
}

void testObjetoResolucion() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    PruebaComportamiento b;

    // Sin resolver activo, escribir un nombre (objetivo inexistente) => nullptr.
    fijarResolverObjetos(nullptr);
    ValorCampo v = valorPorDefecto(defs[15]);
    v.contenido = std::string("Enemigo");
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == nullptr, "sin resolver el objetivo queda nulo");

    // Con resolver que no lo encuentra => nullptr.
    fijarResolverObjetos([](const std::string&) { return nullptr; });
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == nullptr, "resolver sin el objeto devuelve nulo");

    GameObject* camara =
        reinterpret_cast<GameObject*>(static_cast<std::uintptr_t>(0x1234));
    fijarResolverObjetos([camara](const std::string& nombre) {
        return nombre == "Camara" ? camara : nullptr;
    });
    v.contenido = std::string("Camara");
    escribirCampo(defs[15], &b, v);
    CHECK(b.objetivo == camara,
          "referencia a GameObject se resuelve por nombre");
    fijarResolverObjetos(nullptr);

    // Vector de objetos: nombres -> punteros nulos.
    std::vector<std::string> nombres{"A", "B"};
    ValorCampo vL = valorPorDefecto(defs[16]);
    vL.contenido = nombres;
    escribirCampo(defs[16], &b, vL);
    CHECK(b.enemigos.size() == 2 && b.enemigos[0] == nullptr &&
              b.enemigos[1] == nullptr,
          "vector de objetos resuelto a nulos");
}

void testComponenteResolucion() {
    const std::vector<DefCampo>& defs = ComportamientoConHUD::reflexion();
    CHECK(defs.size() == 1 && defs[0].tag == TagTipo::Componente,
          "el campo de componente se refleja con su tag");
    CHECK(defs[0].tipoComponente == "InterfaceComponent",
          "el campo conserva el nombre del tipo de componente");

    ComportamientoConHUD b;

    // Sin resolvers: el nombre no tiene a quien apuntar => nullptr.
    fijarResolverComponentes(ResolverComponentes{});
    ValorCampo v = valorPorDefecto(defs[0]);
    v.contenido = std::string("Panel");
    escribirCampo(defs[0], &b, v);
    CHECK(b.panel == nullptr, "sin resolver el componente queda nulo");

    RecursoHUD componente;
    fijarResolverComponentes(ResolverComponentes{
        [&componente](const std::string& nombre,
                      const std::string& tipo) -> void* {
            return (nombre == "Panel" && tipo == "InterfaceComponent")
                       ? static_cast<void*>(&componente)
                       : nullptr;
        },
        [&componente](const void* puntero, const std::string&) -> std::string {
            return puntero == static_cast<const void*>(&componente)
                       ? std::string("Panel")
                       : std::string{};
        }});

    escribirCampo(defs[0], &b, v);
    CHECK(b.panel == &componente,
          "el componente se resuelve por nombre de objeto + tipo");

    ValorCampo leido = leerCampo(defs[0], &b);
    CHECK(leido.tag == TagTipo::Componente &&
              leido.como<std::string>() == "Panel",
          "leer el componente devuelve el nombre del objeto dueno");

    // Round-trip binario: se persiste el nombre del dueno, no el puntero.
    TempPruebas::CarpetaPrueba carpeta("funshi_reflexion_componente");
    const std::filesystem::path ruta = carpeta.ruta() / "componente.bin";
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        CHECK(out.good(), "abrir archivo temporal de componente");
        std::vector<ValorCampo> valores{leido};
        guardarValoresCampos(out, valores);
        out.close();
    }
    {
        std::ifstream in(ruta, std::ios::binary);
        std::vector<ValorCampo> cargados = cargarValoresCampos(in, defs);
        CHECK(cargados.size() == 1 &&
                  cargados[0].como<std::string>() == "Panel",
              "round-trip del componente conserva el nombre del dueno");
    }

    // Un objeto inexistente deja la referencia nula.
    v.contenido = std::string("NoExiste");
    escribirCampo(defs[0], &b, v);
    CHECK(b.panel == nullptr, "objeto inexistente deja el componente nulo");

    // Un resolver que no encuentra nada tampoco resuelve.
    fijarResolverComponentes(ResolverComponentes{
        [](const std::string&, const std::string&) -> void* { return nullptr; },
        [](const void*, const std::string&) -> std::string {
            return std::string{};
        }});
    v.contenido = std::string("Panel");
    escribirCampo(defs[0], &b, v);
    CHECK(b.panel == nullptr, "resolver que no encuentra deja nulo");

    fijarResolverComponentes(ResolverComponentes{});
}

void testSerializacionBinaria() {
    const std::vector<DefCampo>& defs = defsDePrueba();
    PruebaComportamiento b;
    b.vidas = 42;
    b.oleadas.push_back(Oleada{});
    b.oleadas[0].conteo = 8;
    b.oleadas[0].espaciado = 2.25f;
    std::vector<std::string> enemigosNombres{"E1", "E2"};

    std::vector<ValorCampo> valores;
    for (const DefCampo& d : defs) valores.push_back(leerCampo(d, &b));
    // El vector de objetos se guarda por nombre: reemplazamos el valor leido
    // (b.enemigos esta vacio en la prueba) para ejercitar la serializacion.
    valores[16].contenido = enemigosNombres;

    // Carpeta temporal unica por proceso (RAII): ademas el .bin ahora se
    // limpia al salir (antes quedaba en el sistema tras cada corrida).
    TempPruebas::CarpetaPrueba carpetaPrueba("funshi_reflexion_binaria");
    const std::filesystem::path ruta =
        carpetaPrueba.ruta() / "reflexion_binaria.bin";
    {
        std::ofstream out(ruta, std::ios::binary | std::ios::trunc);
        CHECK(out.good(), "abrir archivo temporal de escritura");
        guardarValoresCampos(out, valores);
        out.close();
    }

    // Carga con los mismos defs: round-trip exacto.
    {
        std::ifstream in(ruta, std::ios::binary);
        std::vector<ValorCampo> cargados = cargarValoresCampos(in, defs);
        CHECK(cargados.size() == kCantidadCampos, "round-trip mantiene cantidad");
        CHECK(cargados[0].como<int>() == 42, "round-trip entero");
        CHECK(cargados[4].como<std::string>() == "Hola", "round-trip texto");
        CHECK(cargados[6].como<std::vector<int>>() ==
                  std::vector<int>({1, 2, 3}),
              "round-trip enteros");
        CHECK(cargados[14].como<std::vector<std::vector<ValorCampo>>>().size() ==
                  1,
              "round-trip grupos vector");
        CHECK(cargados[14].como<std::vector<std::vector<ValorCampo>>>()[0][0]
                          .como<int>() == 8,
              "round-trip subcampo de grupos");
        CHECK(cargados[15].como<std::string>().empty(), "round-trip objeto vacio");
        CHECK(cargados[16].como<std::vector<std::string>>() == enemigosNombres,
              "round-trip vector de objetos");
    }

    // Carga con defs REORDENADOS y con un campo nuevo: casa por nombre.
    {
        std::vector<DefCampo> defsMutados = {
            defs[4],  // nombre
            defs[9],  // flags
            defs[0],  // vidas
            defs[1],  // velocidad
            // campo nuevo inexistente en el archivo original:
            DefCampo{}};
        defsMutados[4].nombre = "campoNuevo";
        defsMutados[4].tag = TagTipo::Texto;

        std::ifstream in(ruta, std::ios::binary);
        std::vector<ValorCampo> cargados = cargarValoresCampos(in, defsMutados);
        CHECK(cargados.size() == 5, "carga con defs mutados mantiene cantidad");
        CHECK(cargados[0].nombre == "nombre" &&
                  cargados[0].como<std::string>() == "Hola",
              "casa por nombre aunque cambie el orden");
        CHECK(cargados[2].nombre == "vidas" && cargados[2].como<int>() == 42,
              "casa por nombre (vidas)");
        CHECK(cargados[4].nombre == "campoNuevo" &&
                  cargados[4].como<std::string>().empty(),
              "campo nuevo queda con default");
    }

    std::filesystem::remove(ruta);
}
