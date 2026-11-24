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
#include "SceneSerializationTestsDetalle.h"

// --- Causa raíz: el look-ahead pierde la posición con ≥2 hermanos -------------
// loadPreOrder lee SceneBBDDObjetos.txt en modo texto y el look-ahead hace
// seekg(tellg()) tras getline; en MinGW/Windows eso no es idempotente: la
// releitura de la linea del siguiente hermano arranca en un offset erroneo
// (medido con sondas: +2 a +6, depende del bufer) y la linea sale truncada
// ("ectN2.db"). Con el codigo viejo find("ObjectN") fallaba -> id 0 ->
// fantasma "Scene" sin aviso; con solo la validacion estricta el hermano
// sano se saltearia. La correccion de fondo es leer el indice en binario.
void hermanosConsecutivosSinPerdida() {
    TempPruebas::CarpetaPrueba carpetaDir("funshi_h17_hermanos");
    const fs::path base = carpetaDir.ruta();
    std::error_code ec;
    fs::create_directories(base / "Scene", ec);

    const std::string prefijo = (base / "Scene").string();
    const std::string pathTxt = (base / "SceneBBDDObjetos.txt").string();
    const std::string semiPath = (base / "Scene").string() + "/";

    // Escena: raiz + 4 hermanos consecutivos en el MISMO nivel (el caso que
    // el round-trip existente no cubria: ahi el look-ahead si hace seekg).
    {
        SceneRegistry registry;
        EventBus events;
        AssetManager assets;
        EditorController editor(&registry, nullptr, &events, &assets);
        SceneSerializer serializer(&registry, &editor, &assets);

        GameObject* raiz = registry.getRoot();
        int creados = 0;
        for (int i = 1; i <= 4; ++i) {
            auto objeto = GameObjectFactory::createSimpleObject(raiz);
            std::snprintf(objeto->inputName, sizeof(objeto->inputName),
                          "Hermano%d", i);
            if (editor.createGameObject(std::move(objeto), raiz)) ++creados;
        }
        CHECK(creados == 4, "se crean los cuatro hermanos consecutivos");
        serializer.save(prefijo);
    }

    // Recargar: los cuatro tienen que volver completos.
    SceneRegistry registry;
    EventBus events;
    AssetManager assets;
    EditorController editor(&registry, nullptr, &events, &assets);
    SceneSerializer serializer(&registry, &editor, &assets);

    std::ostringstream aviso;
    std::streambuf* buferAnterior = std::cerr.rdbuf(aviso.rdbuf());
    serializer.load(pathTxt, semiPath);
    std::cerr.rdbuf(buferAnterior);

    int hermanosCargados = 0;
    int idsCorrectos = 0;
    if (GameObject* raiz = registry.getRoot()) {
        for (auto* hijo : raiz->getChildEntities()) {
            auto* go = dynamic_cast<GameObject*>(hijo);
            if (!go) continue;
            const std::string nombre = go->inputName;
            if (nombre.rfind("Hermano", 0) != 0) continue;
            ++hermanosCargados;
            const int esperado = nombre.back() - '0';
            if (go->getId() == esperado) ++idsCorrectos;
        }
    }
    CHECK(hermanosCargados == 4,
          "sobreviven los cuatro hermanos consecutivos (el look-ahead no "
          "pierde la posicion de ninguna linea)");
    CHECK(idsCorrectos == 4,
          "cada hermano conserva su id (ninguno se releyo truncado)");
    CHECK(aviso.str().find("linea invalida") == std::string::npos,
          "sin lineas invalidas: la releertura del look-ahead es exacta");
}

// --- Rutas de las caras del Skybox -------------------------------------------
// Las seis caras del cubemap son assets del proyecto, asi que se guardan
// relativas a la raiz de assets y se resuelven a absolutas al cargar, igual que
// la malla, las texturas y el script. Sin eso la ruta decia una cosa (relativa
// al proyecto) y hacia otra: se buscaba contra el directorio de trabajo del
// proceso, la fecha de modificacion daba 0 y el cache de la textura no se
// invalidaba nunca.

// Lectura de una ruta tal cual quedo escrita, sin pasar por absolutizar: es la
// forma de comprobar QUE se persisto, no de reconstruirla.
std::string leerRutaPersistida(std::istream& in) {
    uint32_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    std::string s;
    if (len > 0) {
        s.resize(len);
        in.read(&s[0], len);
    }
    return s;
}

// Escritura cruda de una ruta, para armar a mano un archivo con el formato
// viejo (rutas absolutas tal cual, sin pasar por relativizar).
void escribirRutaPersistida(std::ostream& out, const std::string& s) {
    const uint32_t len = static_cast<uint32_t>(s.size());
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) out.write(s.data(), len);
}

void rutasDeCarasDeSkybox() {
    EditorConfig::limpiarRaizAssets();
    const std::string raiz = "/motor/FunshiEngine-DATA/Juego/srcJuego";
    EditorConfig::fijarRaizAssets(raiz);

    const std::string relativaPX = "Cielo/cielo_px.png";
    const std::string absolutaPX = raiz + "/" + relativaPX;
    const std::string relativaNY = "Cielo/cielo_ny.png";
    const std::string absolutaNY = raiz + "/" + relativaNY;

    // 1. Lo que queda EN DISCO son las rutas relativas, no las absolutas con las
    //    que se compuso el componente.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_rutas");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        {
            Skybox sky;
            sky.setCaraMasX(absolutaPX);
            sky.setCaraMenosY(absolutaNY);
            std::ofstream out(archivo, std::ios::binary);
            sky.saveComponent(&out);
        }
        std::ifstream in(archivo, std::ios::binary);
        CHECK(leerRutaPersistida(in) == relativaPX,
              "la cara +X se persiste relativa a la raiz de assets");
        CHECK(leerRutaPersistida(in) == "",
              "una cara sin asignar se persiste vacia");
        CHECK(leerRutaPersistida(in) == "",
              "otra cara sin asignar se persiste vacia");
        CHECK(leerRutaPersistida(in) == relativaNY,
              "la cara -Y se persiste relativa a la raiz de assets");
    }

    // 2. Un archivo con rutas RELATIVAS al cargar queda con rutas ABSOLUTAS en
    //    memoria, que es como las usa la pasada que dibuja el cubemap. Se arma
    //    el archivo a mano para comprobar la resolucion, no el viaje de ida.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_carga");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        const bool visible = true;
        {
            std::ofstream out(archivo, std::ios::binary);
            escribirRutaPersistida(out, relativaPX);
            for (int i = 0; i < 5; ++i) escribirRutaPersistida(out, "");
            out.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absolutaPX,
              "una ruta relativa se resuelve a absoluta al cargar");
        CHECK(sky.getCaraMenosY().empty(),
              "una cara sin asignar sigue vacia al cargar");
    }

    // 3. Ida y vuelta completa: componer con absolutas, guardar y recargar deja
    //    las mismas absolutas.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_ida_vuelta");
        const fs::path archivo = carpeta.ruta() / "skybox.bin";
        {
            Skybox sky;
            sky.setCaraMasX(absolutaPX);
            sky.setCaraMenosY(absolutaNY);
            std::ofstream out(archivo, std::ios::binary);
            sky.saveComponent(&out);
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absolutaPX,
              "ida y vuelta: la cara +X conserva su ruta");
        CHECK(sky.getCaraMenosY() == absolutaNY,
              "ida y vuelta: la cara -Y conserva su ruta");
    }

    // 3. Escena legacy: una cara ABSOLUTA guardada por el formato viejo se carga
    //    intacta, sin volver a anteponerle la raiz.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_skybox_legacy");
        const fs::path archivo = carpeta.ruta() / "skybox_legacy.bin";
        const std::string absoluta = "/legacy/Cielo/absoluta.png";
        const bool visible = true;
        {
            std::ofstream out(archivo, std::ios::binary);
            for (int i = 0; i < 6; ++i) escribirRutaPersistida(out, absoluta);
            out.write(reinterpret_cast<const char*>(&visible), sizeof(visible));
        }
        Skybox sky;
        std::ifstream in(archivo, std::ios::binary);
        sky.loadComponent(&in);
        CHECK(sky.getCaraMasX() == absoluta,
              "una ruta absoluta legacy se deja intacta al cargar");
    }

    EditorConfig::limpiarRaizAssets();
}

// --- Reescritura de referencias al mover/renombrar ----------------------------
// El explorador publica la ruta con el separador nativo (std::filesystem) y la
// escena resuelve sus rutas con '/': el cotejo de prefijos tiene que tratar
// ambos como el mismo separador (en Windows) para que mover o renombrar una
// carpeta reescriba mallas, texturas y scripts en vez de devolver 0 cambios
// en silencio y dejar los .db apuntando al lugar viejo.
void reescrituraDeReferencias() {
    EditorConfig::limpiarRaizAssets();
    const std::string raiz = "C:\\base\\FunshiEngine-DATA/Proyects/JuegoX/srcJuegoX";
    EditorConfig::fijarRaizAssets(raiz);

    // Rutas en memoria tal como quedan al cargar la escena (absolutizar
    // concatena la raiz con '/').
    const std::string scriptEnEscena =
        EditorConfig::absolutizarRuta("Scripts/cpp.cpp");
    const std::string mallaEnEscena =
        EditorConfig::absolutizarRuta("Mallas/Auto.fbx");
    const std::string texturaEnEscena =
        EditorConfig::absolutizarRuta("Texturas/difuso.png");
    const std::string audioEnEscena =
        EditorConfig::absolutizarRuta("Audio/disparo.wav");

    // 1. Mover `Scripts` dentro de `Assets`: solo la fuente esta bajo el
    //    prefijo movido; malla y textura quedan intactas.
    const std::string prefijoScripts = raiz + PATH_SEP + "Scripts";
    const std::string destinoScripts =
        raiz + PATH_SEP + "Assets" + PATH_SEP + "Scripts";

    // Componentes con las tres familias de asset que reescribe el modulo.
    GameObject objeto;
    auto script = std::make_unique<Script>();
    script->setDllPath(scriptEnEscena);
    Script* refScript = script.get();
    objeto.addComponent(std::move(script));

    auto modelo = std::make_unique<Model>();
    modelo->setPath(mallaEnEscena);
    Model* refModelo = modelo.get();
    objeto.addComponent(std::move(modelo));

    auto material = std::make_unique<Material>();
    material->setDiffuseMapPath(texturaEnEscena);
    Material* refMaterial = material.get();
    objeto.addComponent(std::move(material));

    // El archivo de audio es un asset mas: su ruta se reescribe igual que la
    // de una malla o una textura.
    auto audio = std::make_unique<AudioSource>();
    audio->setRutaClip(audioEnEscena);
    AudioSource* refAudio = audio.get();
    objeto.addComponent(std::move(audio));

    ListaDE<GameObject*> escena;
    escena.addLast(&objeto);

    const int mover = RutasReescritura::reescribirEnEscena(
        &escena, prefijoScripts, destinoScripts);
    CHECK(mover == 1, "mover la carpeta reescribe la fuente del script");
    CHECK(refScript->getPath() == destinoScripts + "/cpp.cpp",
          "la fuente del script queda bajo la carpeta destino");
    CHECK(refModelo->getPath() == mallaEnEscena,
          "la malla (fuera del prefijo movido) no se toca");
    CHECK(refMaterial->getDiffuseMapPath() == texturaEnEscena,
          "la textura (fuera del prefijo movido) no se toca");
    CHECK(refAudio->getRutaClip() == audioEnEscena,
          "el audio (fuera del prefijo movido) no se toca");

    // 2. Renombrar la carpeta de la malla: reescribe la malla y nada mas.
    const int renombrarMalla = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "Mallas", raiz + PATH_SEP + "MallasNuevas");
    CHECK(renombrarMalla == 1,
          "renombrar una carpeta reescribe la referencia de la malla");
    CHECK(refModelo->getPath() ==
              raiz + PATH_SEP + "MallasNuevas/Auto.fbx",
          "la malla queda bajo el nombre nuevo");

    // 3. Renombrar la carpeta de texturas: reescribe la textura.
    const int renombrarTextura = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "Texturas",
        raiz + PATH_SEP + "TexturasNuevas");
    CHECK(renombrarTextura == 1,
          "renombrar una carpeta reescribe la textura del material");
    CHECK(refMaterial->getDiffuseMapPath() ==
              raiz + PATH_SEP + "TexturasNuevas/difuso.png",
          "la textura queda bajo el nombre nuevo");

    // 4. Un prefijo que no corresponde a ninguna referencia no cambia nada.
    const int sinCambios = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "OtraCarpeta",
        raiz + PATH_SEP + "NadaCarpeta");
    CHECK(sinCambios == 0, "prefijo que no matchea no cambia nada");

    // 5. Renombrar la carpeta del audio: reescribe la ruta del componente.
    const int renombrarAudio = RutasReescritura::reescribirEnEscena(
        &escena, raiz + PATH_SEP + "Audio", raiz + PATH_SEP + "Sonidos");
    CHECK(renombrarAudio == 1,
          "renombrar una carpeta reescribe la ruta del archivo de audio");
    CHECK(refAudio->getRutaClip() ==
              raiz + PATH_SEP + "Sonidos/disparo.wav",
          "el archivo de audio queda bajo el nombre nuevo");

    EditorConfig::limpiarRaizAssets();
}
