#ifndef RAIZENGINE_H
#define RAIZENGINE_H

// Busqueda de la raiz de los fuentes del engine (el directorio con
// CMakeLists.txt y src/main.cpp) para el add_subdirectory del proyecto de
// exportacion. Logica pura header-only: el predicado `existe` se inyecta, asi
// se prueba sin tocar el disco ni lanzar una exportacion.

#include <filesystem>
#include <functional>
#include <string>

namespace RaizEngine {

// Un directorio es raiz del engine si tiene CMakeLists.txt y src/main.cpp.
inline bool esRaiz(const std::string& dir,
                   const std::function<bool(const std::string&)>& existe) {
    if (dir.empty()) return false;
    std::string base = dir;
    if (base.back() != '/' && base.back() != '\\') base += '/';
    return existe(base + "CMakeLists.txt") && existe(base + "src/main.cpp");
}

// Sube desde dirInicial hasta encontrar la raiz; cadena vacia si no la hay
// (p. ej. la carpeta de una exportacion en la maquina de un jugador: ahi no
// hay checkout y re-exportar no es soportado).
inline std::string buscar(
    const std::string& dirInicial,
    const std::function<bool(const std::string&)>& existe) {
    if (dirInicial.empty()) return {};
    std::filesystem::path dir(dirInicial);
    // Techo defensivo: un arbol con symlinks que suben y bajan no puede
    // enredar la busqueda en un bucle infinito.
    for (int intento = 0; intento < 32; ++intento) {
        const std::string actual = dir.string();
        if (esRaiz(actual, existe)) return actual;
        const std::filesystem::path padre = dir.parent_path();
        if (padre.empty() || padre == dir) return {};
        dir = padre;
    }
    return {};
}

} // namespace RaizEngine

#endif // RAIZENGINE_H