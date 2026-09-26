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
// Pruebas headless del paquete Assets: normalizacion de rutas (AssetPath),
// geometria (Mesh) y cache Flyweight (AssetManager) con un loader artificial,
// sin pila grafica ni Assimp. Solo std C++17.

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include "../FunshiEngineGL/src/Assets/AssetException.h"
#include "../FunshiEngineGL/src/Assets/AssetManager.h"
#include "../FunshiEngineGL/src/Assets/AssetPath.h"
#include "../FunshiEngineGL/src/Assets/Mesh.h"

#include <cmath>

namespace fs = std::filesystem;

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// Malla unitaria de 8 vertices (cubo sin indices): util para validar bounds.
std::shared_ptr<Mesh> cuboUnitario() {
    auto mesh = std::make_shared<Mesh>();
    mesh->name = "cuboUnitario";
    const float c[][3] = {
        {0.f, 0.f, 0.f}, {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f},
        {1.f, 1.f, 0.f}, {1.f, 0.f, 1.f}, {0.f, 1.f, 1.f}, {1.f, 1.f, 1.f}};
    for (const auto& v : c) mesh->vertices.emplace_back(v[0], v[1], v[2]);
    mesh->uvs.emplace_back(0.f, 0.f); // solo el primero: hasUvs() debe fallar
    return mesh;
}

// Loader artificial: crea una malla cuyo nombre es el path, cuenta cargas y
// falla para el path marcado como corrupto.
class LoaderStub : public IMeshLoader {
public:
    int cargas = 0;
    std::shared_ptr<Mesh> load(const std::string& path) override {
        ++cargas;
        if (AssetPath::hasExtension(path, "corrupto"))
            throw AssetLoadException(path, "archivo marcado como corrupto");
        auto mesh = std::make_shared<Mesh>();
        mesh->name = path;
        mesh->vertices.emplace_back(0.f, 0.f, 0.f);
        mesh->vertices.emplace_back(1.f, 0.f, 0.f);
        mesh->vertices.emplace_back(0.f, 1.f, 0.f);
        mesh->indices = {0, 1, 2};
        return mesh;
    }
};

void testAssetPath() {
    CHECK(AssetPath::normalize("a\\b/c//d/") == "a/b/c/d",
          "normalize unifica separadores y colapsa //");
    CHECK(AssetPath::normalize("model.obj\\") == "model.obj",
          "normalize quita el separador final");
    CHECK(AssetPath::normalize("C:/") == "C:/",
          "normalize preserva la raiz de unidad");
    CHECK(AssetPath::extension("Model.OBJ") == "obj",
          "extension en minusculas");
    CHECK(AssetPath::extension("models/cubo") == "",
          "sin punto no hay extension");
    CHECK(AssetPath::extension("a/b.c/d.obj") == "obj",
          "extension de la ultima seccion");
    CHECK(AssetPath::hasExtension("cubo.FBX", "fbx"),
          "hasExtension ignora mayusculas");
    CHECK(!AssetPath::hasExtension("cubo.obj", "fbx"),
          "hasExtension distingue extensiones");
    CHECK(AssetPath::join("Meshes", "cubo.obj") == "Meshes/cubo.obj",
          "join base + relativa");
    CHECK(AssetPath::join("", "x.obj") == "x.obj", "join con base vacia");
    CHECK(AssetPath::join("/base/", "//rel/m.obj") == "/base/rel/m.obj",
          "join normaliza ambas rutas");
    CHECK(AssetPath::basename("models/cubo.obj") == "cubo",
          "basename quita directorio y extension");
    const fs::path tempAbs = fs::temp_directory_path();
    CHECK(AssetPath::isAbsolute(tempAbs.string()),
          "isAbsolute acepta rutas absolutas reales");
    CHECK(!AssetPath::isAbsolute("rel/m.obj"), "isAbsolute rechaza relativas");
}

void testMesh() {
    auto mesh = cuboUnitario();
    CHECK(!mesh->isEmpty(), "mesh poblada no esta vacia");
    CHECK(!mesh->hasNormals(), "sin normales hasNormals() es false");
    CHECK(!mesh->hasUvs(), "con UVs incompletas hasUvs() es false");
    vec3 mn, mx;
    CHECK(mesh->computeBounds(mn, mx), "computeBounds con vertices");
    CHECK(mn.x == 0.f && mn.y == 0.f && mn.z == 0.f,
          "minimo del cubo unitario");
    CHECK(mx.x == 1.f && mx.y == 1.f && mx.z == 1.f,
          "maximo del cubo unitario");
    Mesh vacio;
    CHECK(!vacio.computeBounds(mn, mx), "computeBounds con mesh vacia es false");
}

// Cuadrilatero en el plano XY: sus normales calculadas deben dar +Z.
void testMeshNormals() {
    auto quad = std::make_shared<Mesh>();
    quad->name = "cuadrilatero";
    const float pos[][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    for (int v = 0; v < 4; ++v)
        quad->vertices.emplace_back(pos[v][0], pos[v][1], pos[v][2]);
    quad->indices = {0, 1, 2, 0, 2, 3};

    CHECK(!quad->hasNormals(), "sin calcular no hay normales");
    CHECK(quad->computeNormals(), "computeNormals sobre cuadrilatero");
    CHECK(quad->hasNormals(), "hasNormals() true tras computeNormals");
    for (std::size_t v = 0; v < quad->normals.size(); ++v) {
        CHECK(std::fabs(quad->normals[v].x) < 1e-5f &&
                  std::fabs(quad->normals[v].y) < 1e-5f &&
                  std::fabs(quad->normals[v].z - 1.f) < 1e-5f,
              "normal del cuadrilatero apunta a +Z");
    }

    // Ponderacion por area: el vertice 0 comparte una cara grande (+Y, area 8) y
    // una chica (+Z, area 0.5). La normal promediada debe dominar la cara
    // grande: (0,16,0)+(0,0,1) normalizado.
    auto pesada = std::make_shared<Mesh>();
    const float posP[][3] = {
        {0, 0, 0}, {0, 0, 4}, {4, 0, 0},   // cara grande +Y (area 8)
        {1, 0, 0}, {0, 1, 0}};             // cara chica  +Z (area 0.5)
    for (const auto& v : posP)
        pesada->vertices.emplace_back(v[0], v[1], v[2]);
    pesada->indices = {0, 1, 2, 0, 3, 4};
    CHECK(pesada->computeNormals(), "computeNormals con dos caras por vertice");
    const vec3& n0 = pesada->normals[0];
    CHECK(std::fabs(n0.y - 0.99804866f) < 1e-4f,
          "la cara grande domina el promedio");
    CHECK(std::fabs(n0.z - 0.06237792f) < 1e-4f,
          "la cara chica aporta al promedio");
    CHECK(std::fabs(n0.magnitude() - 1.f) < 1e-5f, "la normal queda normalizada");

    // Un vertice que no pertenece a ninguna cara se queda con la normal nula,
    // no con un NaN (normaliza() divide por la magnitud).
    auto huerfano = std::make_shared<Mesh>();
    huerfano->vertices.emplace_back(0.f, 0.f, 0.f);
    huerfano->vertices.emplace_back(1.f, 0.f, 0.f);
    huerfano->vertices.emplace_back(0.f, 1.f, 0.f);
    huerfano->vertices.emplace_back(9.f, 9.f, 9.f); // sin ninguna cara
    huerfano->indices = {0, 1, 2};
    CHECK(huerfano->computeNormals(), "computeNormals con vertice huerfano");
    const vec3& nH = huerfano->normals[3];
    CHECK(nH.x == 0.f && nH.y == 0.f && nH.z == 0.f,
          "el vertice sin caras conserva la normal nula (no NaN)");

    // Cara degenerada (los tres vertices en el mismo punto): producto cruz
    // nulo, sin NaN.
    auto degenerada = std::make_shared<Mesh>();
    degenerada->vertices.emplace_back(1.f, 1.f, 1.f);
    degenerada->vertices.emplace_back(1.f, 1.f, 1.f);
    degenerada->vertices.emplace_back(1.f, 1.f, 1.f);
    degenerada->indices = {0, 1, 2};
    CHECK(degenerada->computeNormals(), "computeNormals con cara degenerada");
    CHECK(degenerada->normals[0].magnitude() == 0.f,
          "cara degenerada deja la normal en cero (no NaN)");

    // Indice fuera de rango: se salta la cara sin leer fuera del vector.
    auto colgado = std::make_shared<Mesh>();
    colgado->vertices.emplace_back(0.f, 0.f, 0.f);
    colgado->vertices.emplace_back(1.f, 0.f, 0.f);
    colgado->vertices.emplace_back(0.f, 1.f, 0.f);
    colgado->indices = {0, 1, 2, 0, 1, 99};
    CHECK(colgado->computeNormals(), "computeNormals con indice invalido");
    CHECK(colgado->hasNormals(), "el indice invalido no rompe el resultado");

    // Sin indices triangulados no hay de donde calcular: no debe mentir.
    Mesh vacio;
    CHECK(!vacio.computeNormals(), "computeNormals sin vertices es false");
    auto sinIndices = std::make_shared<Mesh>();
    sinIndices->vertices.emplace_back(0.f, 0.f, 0.f);
    CHECK(!sinIndices->computeNormals(), "computeNormals sin indices es false");
    auto impar = std::make_shared<Mesh>();
    impar->vertices = {vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0),
                       vec3(1, 1, 0)};
    impar->indices = {0, 1, 2, 3}; // no multipleto de 3
    CHECK(!impar->computeNormals(), "computeNormals con indices sin cerrar es false");

    // Modo "soloFaltantes": es el caso de un asset que mezcla sub-mallas con y
    // sin normales. Los vertices con normal ya informada deben quedar EXACTOS
    // (no promediados), y los que estan en cero se recalculan por cara. Es lo
    // que evita que un asset mixto salga con caras apagadas (normal nula) o con
    // aristas duras perdidas.
    auto mixto = std::make_shared<Mesh>();
    // Dos triangulos: v0-v1-v2 en el plano XY (quedara sin normal) y
    // v3-v4-v5 en el plano XZ (trae normal +Y, que hay que preservar).
    mixto->vertices = {vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 0, 1),
                       vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0)};
    for (int v = 0; v < 3; ++v)
        mixto->normals.emplace_back(0.f, 0.f, 0.f); // sub-malla sin normales
    for (int v = 0; v < 3; ++v)
        mixto->normals.emplace_back(0.f, 1.f, 0.f); // sub-malla con normales
    mixto->indices = {0, 1, 2, 3, 4, 5};
    CHECK(mixto->hasNormals(), "hasNormals() es por tamano: el mixto 'pasa'");
    CHECK(mixto->computeNormals(true),
          "computeNormals(soloFaltantes) sobre malla mixta");
    for (int v = 0; v < 3; ++v) {
        CHECK(mixto->normals[static_cast<std::size_t>(v)].magnitude() > 0.5f,
              "la sub-malla sin normales recibe normal calculada");
    }
    for (int v = 3; v < 6; ++v) {
        CHECK(mixto->normals[static_cast<std::size_t>(v)].x == 0.f &&
                  mixto->normals[static_cast<std::size_t>(v)].y == 1.f &&
                  mixto->normals[static_cast<std::size_t>(v)].z == 0.f,
              "soloFaltantes respeta la normal que ya traia el asset");
    }
    // El mismo modo sobre una malla sin ninguna normal previa equivale al
    // computeNormals() de siempre.
    auto soloFaltantesSinBase = std::make_shared<Mesh>();
    soloFaltantesSinBase->vertices = mixto->vertices;
    soloFaltantesSinBase->indices = mixto->indices;
    CHECK(soloFaltantesSinBase->computeNormals(true),
          "computeNormals(soloFaltantes) sin normales previas");
    for (std::size_t v = 0; v < soloFaltantesSinBase->normals.size(); ++v)
        CHECK(soloFaltantesSinBase->normals[v].magnitude() > 0.5f,
              "sin base previa, soloFaltantes calcula todas las normales");
}

// Cuadrilatero en el plano XY con normales +Z y UVs canonicas: su marco
// tangente debe resultar T = +X, B = +Y (arriba = +Y en UV).
std::shared_ptr<Mesh> cuadrilatero() {
    auto mesh = std::make_shared<Mesh>();
    mesh->name = "cuadrilatero";
    const float pos[][3] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
    for (int v = 0; v < 4; ++v) {
        mesh->vertices.emplace_back(pos[v][0], pos[v][1], pos[v][2]);
        mesh->normals.emplace_back(0.f, 0.f, 1.f);
    }
    mesh->uvs = {vec2(0, 0), vec2(1, 0), vec2(1, 1), vec2(0, 1)};
    mesh->indices = {0, 1, 2, 0, 2, 3};
    return mesh;
}

void testMeshTangents() {
    auto mesh = cuadrilatero();
    CHECK(!mesh->hasTangents(), "sin calcular no hay tangentes");
    CHECK(mesh->computeTangents(), "computeTangents sobre cuadrilatero");
    CHECK(mesh->hasTangents(), "hasTangents() true tras computeTangents");
    for (std::size_t v = 0; v < mesh->tangents.size(); ++v) {
        CHECK(std::fabs(mesh->tangents[v].x - 1.f) < 1e-5f &&
                  std::fabs(mesh->tangents[v].y) < 1e-5f,
              "tangente del cuadrilatero apunta a +X");
        CHECK(std::fabs(mesh->bitangents[v].y - 1.f) < 1e-5f &&
                  std::fabs(mesh->bitangents[v].x) < 1e-5f,
              "bitangente del cuadrilatero apunta a +Y");
    }

    // Sin UVs no hay de donde calcular: no debe lanzar ni mentir.
    auto sinUvs = std::make_shared<Mesh>();
    sinUvs->vertices.emplace_back(0.f, 0.f, 0.f);
    sinUvs->vertices.emplace_back(1.f, 0.f, 0.f);
    sinUvs->vertices.emplace_back(0.f, 1.f, 0.f);
    sinUvs->indices = {0, 1, 2};
    CHECK(!sinUvs->computeTangents(), "computeTangents sin UVs es false");
}

void testAssetManager() {
    AssetManager sinLoader;
    CHECK(sinLoader.meshCount() == 0, "manager nuevo vacio");

    // Sin loader, pedir una malla ausente debe lanzar.
    try {
        sinLoader.getMesh("nos/e x_i-ste.obj");
        CHECK(false, "getMesh sin loader deberia lanzar AssetNotFoundException");
    } catch (const AssetNotFoundException& e) {
        CHECK(std::string(e.what()).find("Asset") != std::string::npos,
              "mensaje de AssetNotFoundException");
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para asset ausente");
    }

    auto loader = std::make_unique<LoaderStub>();
    LoaderStub* loaderRaw = loader.get();
    AssetManager manager(std::move(loader));

    // Carga y cache: el MISMO path solo se parsea una vez.
    auto a = manager.getMesh("Meshes/Cubo.obj");
    auto b = manager.getMesh("Meshes/Cubo.obj");
    CHECK(a.get() == b.get(), "getMesh con el mismo path devuelve el mismo objeto");
    CHECK(loaderRaw->cargas == 1, "la malla se parseo una sola vez");
    CHECK(manager.meshCount() == 1, "una entrada en el cache");

    // Claves normalizadas: distintos separadores -> mismo recurso.
    auto c = manager.getMesh("Meshes\\Cubo.obj");
    CHECK(c.get() == a.get(), "separadores distintos resuelven al mismo asset");
    CHECK(loaderRaw->cargas == 1, "sin recarga por solo cambiar separadores");

    // Loader que falla: la excepcion del loader debe propagarse.
    try {
        manager.getMesh("Ro to.obj.corrupto");
        CHECK(false, "getMesh con loader fallido deberia lanzar");
    } catch (const AssetLoadException&) {
        // esperado
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para carga fallida");
    }

    // putMesh/contains/remove.
    manager.putMesh("Procedural/Tri.obj", cuboUnitario());
    CHECK(manager.containsMesh("Procedural/Tri.obj"), "putMesh registra");
    CHECK(manager.meshCount() == 2, "una entrada mas tras putMesh");
    try {
        manager.putMesh("vacio.obj", std::make_shared<Mesh>());
        CHECK(false, "putMesh con malla vacia deberia lanzar");
    } catch (const AssetLoadException&) {
        // esperado
    } catch (...) {
        CHECK(false, "tipo de excepcion incorrecto para malla vacia");
    }
    manager.removeMesh("Procedural/Tri.obj");
    CHECK(!manager.containsMesh("Procedural/Tri.obj"), "removeMesh elimina");

    // reloadMesh: cambia la identidad pero no invalida a los holders viejos.
    {
        auto antes = manager.getMesh("Meshes/Cubo.obj");
        manager.reloadMesh("Meshes/Cubo.obj");
        auto despues = manager.getMesh("Meshes/Cubo.obj");
        CHECK(antes.get() != despues.get(),
              "reloadMesh reemplaza la malla en el cache");
        CHECK(antes->vertices.size() == 3,
              "el holder anterior conserva datos validos tras el reload");
    }

    // clearUnusedMeshes: evicta solo lo que nadie usa.
    {
        auto retenido = manager.getMesh("Meshes/Cubo.obj"); // use_count 2
        manager.clearUnusedMeshes();
        CHECK(manager.containsMesh("Meshes/Cubo.obj"),
              "malla retenida sobrevive a clearUnusedMeshes");
    }
    manager.clearUnusedMeshes();
    CHECK(!manager.containsMesh("Meshes/Cubo.obj"),
          "malla sin usuarios externos se evicta");
}

} // namespace

int main() {
    testAssetPath();
    testMesh();
    testMeshNormals();
    testMeshTangents();
    testAssetManager();

    std::cout << "Resultado: " << (total - fallos) << "/" << total
              << " OK" << std::endl;
    if (fallos > 0) {
        std::cout << fallos << " prueba(s) fallaron." << std::endl;
        return 1;
    }
    return 0;
}