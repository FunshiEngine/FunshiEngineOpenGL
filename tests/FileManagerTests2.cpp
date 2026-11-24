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
#include "FileManagerTestsDetalle.h"

namespace {

// Busca en pre-orden un nodo cuyo nombre coincida (la raiz se ignora, igual
// que en el explorador).
bool buscarPorNombre(ArbolEnlazado<File*>* arbol, Position<File*>* current,
                     const std::string& nombre) {
    if (!arbol || !current) return false;
    if (current != arbol->rootOfTree() && current->getElement() &&
        current->getElement()->getPathName() == nombre)
        return true;
    if (arbol->isInternal(current)) {
        auto* hijos = arbol->childsOf(current);
        auto* h = hijos->first();
        bool encontrado = false;
        while (h && !encontrado) {
            encontrado = buscarPorNombre(arbol, h->getElement(), nombre);
            h = (h != hijos->last()) ? hijos->next(h) : nullptr;
        }
        delete hijos;
        return encontrado;
    }
    return false;
}

}  // namespace

// --- Selector de caras del cubemap: filtro y validacion ------------------
// La logica del modal vive en funciones puras aparte del dibujo, para poder
// ejercitarla sin ventana. Cubre las dos condiciones que hacen que el motor
// descarte el cubemap sin avisar en pantalla: una cara sin asignar y dos
// caras que no midan lo mismo.
void seccion08() {
    {
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo_px.png"),
              "una .png sirve como cara del cubemap");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo_px.PNG"),
              "el filtro no distingue mayusculas");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("a/b/cielo.tga"),
              "una .tga anidada en carpetas sirve");
        CHECK(SelectorArchivoCubemap::esImagenCubemap("cielo.hdr"),
              "una .hdr sirve");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("cielo.obj"),
              "una malla no es una cara del cubemap");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("notas.txt"),
              "un texto no es una cara del cubemap");
        CHECK(!SelectorArchivoCubemap::esImagenCubemap("cielo"),
              "un archivo sin extension no es una cara");

        const std::string vacias[6] = {"", "", "", "", "", ""};
        CHECK(SelectorArchivoCubemap::carasSinAsignar(vacias) == 6,
              "sin nada asignado faltan las seis caras");

        const SelectorArchivoCubemap::Dimensiones iguales[6] = {
            {512, 512}, {512, 512}, {512, 512},
            {512, 512}, {512, 512}, {512, 512},
        };
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(iguales) == -1,
              "seis caras de las mismas dimensiones no se descartan");

        // El caso que la comparacion por peso de archivo nocia: dos archivos con
        // distinto peso pero los mismos pixeles. Con el criterio de pixeles no
        // hay nada que avisar.
        SelectorArchivoCubemap::Dimensiones mismoTamano[6];
        for (int i = 0; i < 6; ++i) mismoTamano[i] = {1024, 1024};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(mismoTamano) == -1,
              "caras con los mismos pixeles no se descartan aunque pesen distinto");

        // Una cara exportada a otra resolucion: el motor descarta el cubemap
        // entero, asi que el selector avisa cual es.
        SelectorArchivoCubemap::Dimensiones dispares[6];
        for (int i = 0; i < 6; ++i) dispares[i] = {512, 512};
        dispares[3] = {256, 256};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(dispares) == 3,
              "avisa la cara cuya resolucion no coincide");

        // Ancho y alto se comparan por separado: 512x256 no es lo mismo que
        // 256x512 aunque los dos produzcan el mismo numero de pixeles.
        SelectorArchivoCubemap::Dimensiones transpuesta[6];
        for (int i = 0; i < 6; ++i) transpuesta[i] = {512, 256};
        transpuesta[1] = {256, 512};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(transpuesta) == 1,
              "avisa una cara transpuesta aunque tenga igual cantidad de pixeles");

        // Una cara que no se pudo medir (no existe, o formato que no se pudo
        // leer) queda en 0x0 y no falsea la comparacion.
        SelectorArchivoCubemap::Dimensiones conIlegible[6];
        for (int i = 0; i < 6; ++i) conIlegible[i] = {512, 512};
        conIlegible[5] = {};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(conIlegible) == -1,
              "una cara ilegible no se reporta como dimension distinta");

        // Si la +X no se pudo medir no hay contra que comparar: el motor igual
        // va a avisar por su cuenta al no poder decodificar.
        SelectorArchivoCubemap::Dimensiones sinReferencia[6];
        for (int i = 0; i < 6; ++i) sinReferencia[i] = {512, 512};
        sinReferencia[0] = {};
        CHECK(SelectorArchivoCubemap::caraConDimensionDistinta(sinReferencia) == -1,
              "sin dimensiones de la +X no se reporta desajuste");
    }
}

int main() {
    // Carpeta temporal unica por proceso (TempPruebas::CarpetaPrueba): sin
    // remove_all inicial que pudiera pisar a otras corridas simultaneas de
    // ctest; se limpia sola al salir del scope, incluso si el test falla.
    TempPruebas::CarpetaPrueba carpetaBase("funshi_filemanager_tests");
    const fs::path base = carpetaBase.ruta();

    // --- Proyecto sintetico -------------------------------------------------
    const fs::path proy = base / "proyecto";
    fs::create_directories(proy / "src" / "nucleo");
    fs::create_directories(proy / "Assets" / "Meshes");
    fs::create_directories(proy / "Assets" / "Scripts");
    seccion01(proy);

    FileManager fm(proy.string());

    // --- Arbol inicial ------------------------------------------------------
    ArbolEnlazado<File*>* arbol = fm.getArbol();
    CHECK(arbol != nullptr && !arbol->isEmpty(), "el arbol no esta vacio");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "src"),
          "el arbol contiene 'src'");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "Assets"),
          "el arbol contiene 'Assets'");
    CHECK(buscarPorNombre(arbol, arbol->rootOfTree(), "nucleo"),
          "el arbol camina recursivamente hasta 'nucleo'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "Assets/Meshes")) != nullptr,
          "buscarCarpetaPorRuta resuelve 'Assets/Meshes'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "NoExiste")) == nullptr,
          "buscarCarpetaPorRuta devuelve nullptr para una ruta inexistente");

    // --- Seleccion compartida y re-resolucion por ruta tras rescaneo --------
    FileSelection* sel = fm.getSelection();
    sel->rutaVisible = unir(proy, "src");
    fm.refrescar();
    CHECK(sel->carpetaActual != nullptr, "carpetaActual se re-resuelve");
    CHECK(sel->carpetaActual != nullptr &&
              sel->carpetaActual->getPathName() == "src",
          "carpetaActual apunta a 'src'");

    // --- Operaciones de dominio --------------------------------------------
    const std::string rutaNueva = unir(proy, "Assets/Nueva");
    CHECK(fm.crearCarpeta(rutaNueva), "crearCarpeta crea en disco");
    CHECK(fs::is_directory(rutaNueva), "la carpeta nueva existe");

    const std::string rutaArchivo = unir(proy, "Assets/Nueva/ok.txt");
    CHECK(fm.crearArchivo(rutaArchivo, "12345"), "crearArchivo crea en disco");
    CHECK(contenidoDe(rutaArchivo) == "12345", "el archivo nuevo tiene contenido");

    // --- Crear carpeta: "se creo" en vez de "existe" ------------------------
    // Un destino ya ocupado tiene que informar que esta vez no se creo nada; si
    // no, el explorador cierra el modal creyendo que la carpeta nacio.
    const std::string rutaRepetida = unir(proy, "Assets/Repetida");
    CHECK(fm.crearCarpeta(rutaRepetida), "la primera creacion si crea");
    CHECK(!fm.crearCarpeta(rutaRepetida),
          "crearCarpeta no reporta exito si la carpeta ya existe");
    CHECK(fs::is_directory(rutaRepetida),
          "la carpeta existente sigue en su sitio");

    CHECK(!fm.crearCarpeta(""), "crearCarpeta rechaza la ruta vacia");
    const std::string archivoOcupado = unir(proy, "Assets/ocupado.txt");
    CHECK(fm.crearArchivo(archivoOcupado, "x"), "archivo que ocupa el nombre");
    CHECK(!fm.crearCarpeta(archivoOcupado),
          "crearCarpeta falla si el destino es un archivo");

    // El helper compartido por arbol y grid valida el nombre y decide el cierre.
    CHECK(CrearCarpeta::nombreValido("Carpeta con espacios"),
          "un nombre normal es valido");
    CHECK(!CrearCarpeta::nombreValido(""), "el nombre vacio no es valido");
    CHECK(!CrearCarpeta::nombreValido("."), "'.' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido(".."), "'..' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido("con/separador"),
          "un nombre con '/' no es un nombre de carpeta");
    CHECK(!CrearCarpeta::nombreValido("con\\separador"),
          "un nombre con '\\' no es un nombre de carpeta");

    CHECK(!CrearCarpeta::crear(nullptr, unir(proy, "Assets"), "SinProyecto").creada,
          "sin FileManager no se crea nada");
    CHECK(!CrearCarpeta::crear(&fm, "", "SinPadre").creada,
          "sin carpeta padre no se crea nada");

    const std::string rutaPadre = unir(proy, "Assets");
    const unsigned long contadorAntes = sel->contadorCambios;
    const CrearCarpeta::Resultado creadaOk =
        CrearCarpeta::crear(&fm, rutaPadre, "Creada");
    CHECK(creadaOk.creada && creadaOk.error.empty(),
          "el helper crea con un nombre libre");
    CHECK(fs::is_directory(unir(rutaPadre, "Creada")),
          "la carpeta creada queda donde se pidio");
    CHECK(sel->contadorCambios == contadorAntes + 1,
          "crear una carpeta sube el contador exactamente una vez");

    const unsigned long contadorTrasCrear = sel->contadorCambios;
    const CrearCarpeta::Resultado repetida =
        CrearCarpeta::crear(&fm, rutaPadre, "Creada");
    CHECK(!repetida.creada, "el helper no reporta exito si el nombre se repite");
    CHECK(repetida.error.find("Ya existe") != std::string::npos,
          "el helper explica que el nombre ya esta en uso");
    CHECK(sel->contadorCambios == contadorTrasCrear,
          "un intento fallido no sube el contador");

    CHECK(fm.crearCarpeta(unir(proy, "Assets/pruebas")),
          "carpeta padre para el nombre con separador");
    const CrearCarpeta::Resultado conSeparador =
        CrearCarpeta::crear(&fm, rutaPadre, "pruebas/nueva");
    CHECK(!conSeparador.creada,
          "un nombre con separador no crea una carpeta anidada");
    CHECK(!fs::exists(unir(proy, "Assets/pruebas/nueva")),
          "no aparece la carpeta que el nombre pedia en otra ruta");


    // Renombrar carpeta.
    const std::string rutaRenombrada = unir(proy, "Assets/Renombrada");
    CHECK(fm.renombrar(rutaNueva, "Renombrada"), "renombrar mueve la carpeta");
    CHECK(!fs::exists(rutaNueva) && fs::is_directory(rutaRenombrada),
          "el nombre viejo dejo de existir y el nuevo existe");

    // Renombrar rechaza separadores de ruta.
    CHECK(!fm.renombrar(rutaRenombrada, "con/separador"),
          "renombrar rechaza '/'");
    CHECK(!fm.renombrar(rutaRenombrada, "con\\separador"),
          "renombrar rechaza '\\'");

    // Copiar archivo y carpeta (recursivamente).
    const std::string datosOrig = unir(proy, "src/nucleo/datos.txt");
    CHECK(fm.crearArchivo(datosOrig, "abc"), "archivo fuente para copiar");
    CHECK(fm.copiarArchivo(datosOrig, unir(proy, "src/copiaDatos.txt")),
          "copiarArchivo copia el archivo");
    CHECK(fs::is_regular_file(unir(proy, "src/copiaDatos.txt")),
          "la copia del archivo existe");
    const std::string copiaCarpeta = unir(proy, "Assets/srcCopia");
    CHECK(fm.copiarCarpeta(unir(proy, "src"), copiaCarpeta),
          "copiarCarpeta copia la rama");
    CHECK(fs::is_directory(unir(copiaCarpeta, "nucleo")),
          "copiarCarpeta es recursiva (nucleo existe dentro)");

    // Copiar una carpeta dentro de si misma (Ctrl+arrastrar sobre una
    // subcarpeta del propio arbol): el recorrido se copia a si mismo y se
    // reproduce hasta que la ruta deja de caber, dejando un arbol basura a
    // medias. Se usa un arbol propio para que el resto de la prueba no dependa
    // de lo que quede colgando del caso anidado.
    CHECK(fm.crearCarpeta(unir(proy, "Assets/anidado")),
          "crea la carpeta padre del caso anidado");
    CHECK(fm.crearCarpeta(unir(proy, "Assets/anidado/origen")),
          "crea el origen del caso anidado");
    CHECK(fm.crearArchivo(unir(proy, "Assets/anidado/origen/dato.txt"), "x"),
          "archivo dentro del origen del caso anidado");
    const std::string raizAnidada = unir(proy, "Assets/anidado/origen");
    // Un destino que solo empieza por el nombre del origen NO esta dentro de
    // el: el cotejo tiene que cerrar en un separador.
    CHECK(fm.copiarCarpeta(raizAnidada, unir(proy, "Assets/anidado/origenCopia")),
          "copiarCarpeta acepta un destino que solo empieza igual que el origen");
    CHECK(fs::is_regular_file(unir(proy, "Assets/anidado/origenCopia/dato.txt")),
          "la copia legitima al lado del origen esta entera");
    const std::string anidado = unir(raizAnidada, "dentro");
    CHECK(!fm.copiarCarpeta(raizAnidada, anidado),
          "copiarCarpeta rechaza un destino dentro del propio origen");
    CHECK(!fs::exists(anidado), "el destino anidado ni siquiera se crea");
    CHECK(!fs::exists(unir(anidado, "dato.txt")),
          "no se copia nada al destino anidado");
    CHECK(!fm.copiarCarpeta(raizAnidada, raizAnidada),
          "copiarCarpeta rechaza el origen sobre si mismo");
    CHECK(contenidoDe(unir(raizAnidada, "dato.txt")) == "x",
          "el origen queda intacto tras rechazar la copia anidada");

    // --- Mover (drag&drop del explorador) -----------------------------------
    // Es la operacion que usa el arrastre: por defecto mueve, con Ctrl copia.
    // Los casos que importan son los que un rename Ingenuo no cubre.

    // Mover un archivo: desaparece el origen y aparece el destino con su
    // contenido intacto.
    const std::string movible = unir(proy, "Assets/movible.txt");
    CHECK(fm.crearArchivo(movible, "contenido a conservar"), "archivo para mover");
    const std::string movido = unir(proy, "src/movible.txt");
    CHECK(fm.mover(movible, movido), "mover traslada el archivo");
    CHECK(!fs::exists(movible), "tras mover, el origen ya no existe");
    CHECK(fs::is_regular_file(movido), "tras mover, el destino existe");
    CHECK(contenidoDe(movido) == "contenido a conservar",
          "mover conserva el contenido del archivo");

    // Mover una carpeta completa, con su contenido y su estructura.
    const std::string rama = unir(proy, "Assets/rama");
    CHECK(fm.crearCarpeta(rama), "crea la rama");
    CHECK(fm.crearCarpeta(unir(proy, "Assets/rama/interior")),
          "crea la subcarpeta de la rama");
    CHECK(fm.crearArchivo(unir(proy, "Assets/rama/interior/dato.txt"), "x"),
          "archivo dentro de la rama");
    const std::string ramaDestino = unir(proy, "src/rama");
    CHECK(fm.mover(rama, ramaDestino), "mover traslada la carpeta");
    CHECK(!fs::exists(rama), "tras mover la carpeta, el origen ya no existe");
    CHECK(fs::is_regular_file(unir(ramaDestino, "interior/dato.txt")),
          "mover arrastra el contenido de la carpeta");

    // No se pisa un destino existente: es la proteccion contra un arrastre
    // accidental encima de algo que ya estaba ahi.
    const std::string ocupado = unir(proy, "src/ocupado.txt");
    const std::string hueco = unir(proy, "Assets/hueco.txt");
    CHECK(fm.crearArchivo(ocupado, "no tocar"), "destino ocupado");
    CHECK(fm.crearArchivo(hueco, "el que se quiere mover"), "origen a mover");
    CHECK(!fm.mover(hueco, ocupado), "mover se niega a pisar un destino existente");
    CHECK(contenidoDe(ocupado) == "no tocar",
          "el archivo ocupado quedo intacto tras el mover rechazado");
    CHECK(fs::is_regular_file(hueco),
          "el origen sigue en su sitio tras el mover rechazado");

    // Carpeta dentro de si misma: se rechaza antes de tocar disco, porque si
    // se dejara que el error_code lo cortara a mitad, quedaria un arbol a
    // medias.
    CHECK(!fm.mover(unir(proy, "src"), unir(proy, "src")),
          "mover se niega a mover una carpeta sobre si misma");
    CHECK(!fm.mover(unir(proy, "src"), unir(proy, "src/rama/dentro")),
          "mover se niega a meter una carpeta en un descendiente suyo");
    CHECK(fs::is_directory(unir(proy, "src")),
          "la carpeta origen sigue intacta tras los rechazos");

    // Origen inexistente: false sin tocar nada.
    CHECK(!fm.mover(unir(proy, "no/existe.txt"), unir(proy, "src/x.txt")),
          "mover devuelve false si el origen no existe");
    CHECK(!fm.mover("", unir(proy, "src/x.txt")),
          "mover devuelve false con origen vacio");
    CHECK(!fm.mover(unir(proy, "src/rama"), ""),
          "mover devuelve false con destino vacio");

    // Eliminar archivo.
    const std::string rutaBorrable = unir(proy, "Assets/borrable.txt");
    CHECK(fm.crearArchivo(rutaBorrable, "bye"), "archivo fuente para eliminar");
    CHECK(fm.eliminarArchivo(rutaBorrable), "eliminarArchivo borra en disco");
    CHECK(!fs::exists(rutaBorrable), "el archivo eliminado ya no existe");
    CHECK(!fm.eliminarArchivo(rutaBorrable),
          "eliminarArchivo devuelve false si la ruta no existe");
    CHECK(!fm.eliminarArchivo(""),
          "eliminarArchivo devuelve false con ruta vacia");
    CHECK(fm.eliminarArchivo(unir(copiaCarpeta, "nucleo/datos.txt")),
          "eliminarArchivo borra un archivo dentro de una rama");
    CHECK(!fs::exists(unir(copiaCarpeta, "nucleo/datos.txt")),
          "el archivo dentro de la rama ya no existe");

    // Eliminar carpeta.
    CHECK(fm.eliminarCarpeta(rutaRenombrada), "eliminarCarpeta borra en disco");
    CHECK(!fs::exists(rutaRenombrada), "la carpeta eliminada ya no existe");
    fm.refrescar();
    CHECK(fm.buscarCarpetaPorRuta(rutaRenombrada) == nullptr,
          "tras el rescaneo la carpeta eliminada no esta en el arbol");

    // --- Plantilla de script C++ (H-15: la fabrica debe viajar exportada) -----
    // La plantilla declara la fabrica con el macro portable de exportacion,
    // para que en Windows/MSVC la .dll la exporte y GetProcAddress la
    // encuentre. Sin el macro la .dll compila pero el simbolo no existe y el
    // script nunca carga (en MinGW/ELF no se nota: ahi se exporta todo solo).
    const std::string plantilla = FileManager::plantillaScript("MiScript", false);
    CHECK(plantilla.find("FUNSHI_COMPORTAMIENTO_EXPORT") != std::string::npos,
          "la plantilla del script declara el macro de exportacion");
    CHECK(plantilla.find("extern \"C\" " + std::string("FUNSHI_COMPORTAMIENTO_EXPORT")) !=
              std::string::npos,
          "el macro de exportacion aparece en la declaracion de la fabrica");
    CHECK(plantilla.find("FUNSHI_CREAR_COMPORTAMIENTO") != std::string::npos,
          "la plantilla conserva la fabrica con su nombre para GetProcAddress");
    // El camino Java no lleva exportacion (la carga el JVM, no LoadLibrary).
    const std::string plantillaJava = FileManager::plantillaScript("MiJava", true);
    CHECK(plantillaJava.find("FUNSHI_COMPORTAMIENTO_EXPORT") == std::string::npos,
          "la plantilla Java no lleva el macro de exportacion nativa");

    // --- Rescaneo refleja carpetas creadas FUERA del editor -----------------
    CHECK(fs::create_directory(proy / "Assets" / "DiscDirecto"),
          "carpeta sembrada externamente");
    sel->rutaVisible = unir(proy, "Assets");
    fm.refrescar();
    CHECK(sel->carpetaActual != nullptr &&
              sel->carpetaActual->getPathName() == "Assets",
          "carpetaActual se re-resuelve a 'Assets'");
    CHECK(fm.buscarCarpetaPorRuta(unir(proy, "Assets/DiscDirecto")) != nullptr,
          "el rescaneo incorpora carpetas creadas fuera del editor");

    // --- FileSystemWatcher ----------------------------------------------------
    // En Linux: inotify. En el resto (Windows): polling temporal sobre el mtime
    // de la raiz, con el intervalo inyectado para ejercer el contrato sin
    // esperar el pulso de produccion.
#if defined(__linux__)
    const fs::path proyw = base / "proyecto_watch";
    fs::create_directories(proyw / "ok");
    seccion02(proyw);
#else
    const fs::path proyw = base / "proyecto_watch";
    fs::create_directories(proyw / "ok");
    seccion03(proyw);
#endif

    seccion04(proy, fm, rutaNueva);

    seccion05(proy, fm, sel);

    seccion06(proy, fm, sel, rutaNueva);

    seccion07(proy, fm, rutaNueva);

    seccion08();

    // --- Resultado ----------------------------------------------------------
    fs::remove_all(base);
    std::cout << "Pruebas: " << total << ", fallos: " << fallos << std::endl;
    if (fallos == 0) std::cout << "FILEMANAGER TESTS OK" << std::endl;
    return fallos == 0 ? 0 : 1;
}
