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
#include "SceneRendererDetalle.h"

// Cielo degradado: fullscreen triangle con interpolacion vertical entre
// fondoSuperior (top) y fondoInferior (bottom). Se dibuja ANTES que la grilla
// y los objetos, con depth test ON y depth mask OFF, para que el cielo quede
// "detras" de toda la geometria sin escribir profundidad (asi los objetos
// delante ganan el test de profundidad y ocultan el cielo, pero el cielo no
// oculta nada).
//
// El color NO sale de la posicion del pixel sino de la direccion de vista de
// ese pixel: el shader des-proyecta el NDC al plano lejano, resta la camara y
// usa dir.y. Por eso el cielo va con la camara en vez de quedar clavado a la
// pantalla (mirar abajo lleva el color inferior a toda la vista, mirar arriba el
// superior, y el horizonte queda en la transicion).
void SceneRenderer::dibujarCielo(const FrameContext& ctx, const float view[16],
                                 const float projection[16],
                                 const float camaraMundo[3]) {
    if (!ctx.apariencia || !camaraMundo) return;

    auto& backend = backend_;

    // Buscar el primer componente Skybox visible en la escena.
    Skybox* skybox = nullptr;
    if (ctx.gameObjects && !ctx.gameObjects->isEmpty()) {
        Position<GameObject*>* pos = ctx.gameObjects->first();
        while (pos && pos->getElement()) {
            GameObject* obj = pos->getElement();
            Skybox* sb = obj->getComponent<Skybox>();
            if (sb && sb->getVisible()) {
                skybox = sb;
                break;
            }
            pos = (pos != ctx.gameObjects->last()) ? ctx.gameObjects->next(pos) : nullptr;
        }
    }

    // Creacion perezosa del programa del cielo degradado.
    if (!cieloProgram_ && !cieloShaderFallado_ && gestorShaders_) {
        try {
            cieloProgram_ = gestorShaders_->obtener("cielo");
        } catch (const std::exception& e) {
            registrarExcepcionDiagnostica("Rendering/SceneRenderer", e,
                                          NivelDiagnostico::Error);
            std::cerr << "[Cielo] shader del degradado no disponible: " << e.what()
                      << '\n';
            cieloShaderFallado_ = true;
            return;
        } catch (...) {
            registrarFalloDiagnostico(
                "Rendering/SceneRenderer", "excepcion_desconocida",
                "No se pudo inicializar el shader del cielo",
                NivelDiagnostico::Error);
            cieloShaderFallado_ = true;
            return;
        }
    }

    // Si hay un Skybox con cubemap valido, renderizarlo en lugar del degradado.
    if (skybox) {
        // Verificar que el Skybox tiene las 6 caras cargadas.
        bool tieneCubemap = !skybox->getCaraMasX().empty() &&
                            !skybox->getCaraMenosX().empty() &&
                            !skybox->getCaraMasY().empty() &&
                            !skybox->getCaraMenosY().empty() &&
                            !skybox->getCaraMasZ().empty() &&
                            !skybox->getCaraMenosZ().empty();
        if (tieneCubemap && dibujarSkyboxCubemap(skybox, view, projection)) {
            return;
        }
    }

    if (!cieloProgram_) return;

    // Colores efectivos del degradado (resuelven B/N y tema).
    float colorSup[3], colorInf[3];
    Cielo::coloresEfectivos(*ctx.apariencia, colorSup, colorInf);

    // Configurar estado: depth test habilitado, depth mask deshabilitado.
    backend.setDepthTestEnabled(true);
    backend.setDepthMask(false);

    // Los set por nombre no buscan en GL mas alla del primer frame:
    // ShaderProgram cachea la location de cada uniform contra el backend.
    cieloProgram_->use();
    cieloProgram_->setVec3("uColorTop",
                           glm::vec3(colorSup[0], colorSup[1], colorSup[2]));
    cieloProgram_->setVec3("uColorBottom",
                           glm::vec3(colorInf[0], colorInf[1], colorInf[2]));

    // El shader necesita deshacer projection*view para recuperar el rayo de
    // vista de cada pixel, y la posicion de camara para orientar ese rayo.
    const glm::mat4 vista = glm::make_mat4(view);
    const glm::mat4 proyeccion = glm::make_mat4(projection);
    const glm::mat4 invViewProj = glm::inverse(proyeccion * vista);
    cieloProgram_->setMat4("uInvViewProj", invViewProj);
    cieloProgram_->setVec3("uCamPos", glm::vec3(camaraMundo[0], camaraMundo[1],
                                               camaraMundo[2]));
    // Media anchura de la transicion en unidades de dir.y: 0.35 satura el
    // color a unos 20 grados por encima y por debajo del horizonte, que es lo
    // que hace legible la banda de degradado en el horizonte.
    cieloProgram_->setFloat("uTransicion", kCieloTransicion);

    // Fullscreen triangle: 3 vertices, sin VBO (gl_VertexID en el vertex shader).
    backend.drawFullscreenTriangle();

    // Restaurar estado base para la siguiente pasada.
    backend.setDepthMask(true);
    ShaderProgram::unbind(backend);
}

// Skybox cubemap: renderiza un cubo centrado en la camara con el cubemap
// del componente Skybox. Se dibuja con depth test ON + depth mask OFF para
// quedar "detras" de toda la geometria sin escribir profundidad.
bool SceneRenderer::dibujarSkyboxCubemap(const Skybox* skybox,
                                         const float view[16],
                                         const float projection[16]) {
    if (!skybox) return false;

    auto& backend = backend_;

    // Caras del cubemap en el orden del backend (+X, -X, +Y, -Y, +Z, -Z).
    // Se resuelven contra la raiz de assets antes de usarlas: al deserializar
    // la escena ya vienen absolutas, pero una cara escrita a mano en el
    // inspector puede ser relativa, y sin resolverla terminaria buscandose
    // contra el directorio de trabajo del proceso en vez de contra el proyecto.
    std::string rutas[6] = {
        EditorConfig::absolutizarRuta(skybox->getCaraMasX()),
        EditorConfig::absolutizarRuta(skybox->getCaraMenosX()),
        EditorConfig::absolutizarRuta(skybox->getCaraMasY()),
        EditorConfig::absolutizarRuta(skybox->getCaraMenosY()),
        EditorConfig::absolutizarRuta(skybox->getCaraMasZ()),
        EditorConfig::absolutizarRuta(skybox->getCaraMenosZ())
    };

    for (int i = 0; i < 6; ++i) {
        if (rutas[i].empty()) return false; // Si falta alguna cara, caemos al degradado
    }

    // Identidad de las caras: ruta + fecha de modificacion de cada archivo. La
    // textura se sube una sola vez por identidad y se reemplaza solo si cambia
    // algo que la afecta. Sin esto la pasada decodificaba seis imagenes desde
    // disco, creaba una textura nueva con sus mipmaps y la destruia en CADA
    // frame, ademas de cada vista previa abierta.
    std::int64_t mtimes[6];
    for (int i = 0; i < 6; ++i) mtimes[i] = mtimeSegundos(rutas[i]);
    const std::string clave = CacheCubemap::claveDeCaras(rutas, mtimes);
    if (clave != skyboxClave_) {
        // El intento (exitoso o no) queda registrado: si las caras no se pueden
        // decodificar, no se vuelve a intentar hasta que cambie un archivo.
        skyboxClave_ = clave;
        if (skyboxCubemap_ != Rendering::Backend::kInvalidHandle) {
            backend.destroyTextureCube(skyboxCubemap_);
            skyboxCubemap_ = Rendering::Backend::kInvalidHandle;
        }
        if (!cargarCubemap(rutas)) return false;
    }
    if (skyboxCubemap_ == Rendering::Backend::kInvalidHandle) return false;

    // Programa del cubemap (cubo centrado en la camara), creacion perezosa.
    if (!skyboxProgram_ && !skyboxShaderFallado_ && gestorShaders_) {
        try {
            skyboxProgram_ = gestorShaders_->obtener("cubemap");
        } catch (const std::exception& e) {
            registrarExcepcionDiagnostica("Rendering/SceneRenderer", e,
                                          NivelDiagnostico::Error);
            std::cerr << "[Skybox] shader del cubemap no disponible: " << e.what()
                      << '\n';
            skyboxShaderFallado_ = true;
            return false;
        } catch (...) {
            registrarFalloDiagnostico(
                "Rendering/SceneRenderer", "excepcion_desconocida",
                "No se pudo inicializar el shader del skybox",
                NivelDiagnostico::Error);
            skyboxShaderFallado_ = true;
            return false;
        }
    }
    if (!skyboxProgram_) return false;

    // Cubo unitario centrado en el origen (8 vertices, 36 indices): malla del
    // backend, creada una sola vez y compartida por todos los skyboxes (a
    // diferencia de la textura, no depende de las caras).
    if (skyboxCuboMalla_ == Rendering::Backend::kInvalidHandle) {
        static const float vertices[] = {
            // posiciones
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f
        };
        static const unsigned int indices[] = {
            0, 1, 2, 2, 3, 0, // -Z
            4, 5, 6, 6, 7, 4, // +Z
            0, 3, 7, 7, 4, 0, // +Y
            1, 2, 6, 6, 5, 1, // -Y
            3, 2, 6, 6, 7, 3, // +X
            0, 1, 5, 5, 4, 0  // -X
        };
        Rendering::Backend::MeshData datos;
        datos.vertices = vertices;
        datos.vertexCount = sizeof(vertices) / (3 * sizeof(float));
        datos.indices = indices;
        datos.indexCount = sizeof(indices) / sizeof(indices[0]);
        skyboxCuboMalla_ = backend.createMesh(datos);
        if (skyboxCuboMalla_ == Rendering::Backend::kInvalidHandle) return false;
    }

    backend.setDepthTestEnabled(true);
    // El cubo se dibuja en el plano lejano (el vertex shader iguala z a w), o
    // sea NDC z = 1.0 exacto, que es JUSTO el valor con el que se limpia el
    // z-buffer. Con la funcion de comparacion por defecto (GL_LESS) el test
    // 1.0 < 1.0 falla y el cubemap entero se descarta: el fondo queda en el
    // color de limpieza. GL_LEQUAL acepta ese empate, y como la pasada no
    // escribe profundidad (mask off) el resto de la escena sigue decidiendo
    // por su cuenta. Se restaura la funcion base al terminar la pasada.
    backend.setDepthFunc(Rendering::Backend::kDepthFuncLessEqual);
    backend.setDepthMask(false);

    // El cubemap representa el fondo, que esta a distancia infinita: se dibuja
    // con la ROTACION de la camara y sin su traslacion. Con la matriz de vista
    // completa el cubo se desplaza junto con la camara (el fondo "se mueve" con
    // ella) y ademas sale del frustum al alejarse del origen, hasta desaparecer
    // del todo. En una matriz column-major la traslacion son los indices
    // 12, 13 y 14: se copian y se ponen en cero.
    float vistaSinTraslacion[16];
    for (int i = 0; i < 16; ++i) vistaSinTraslacion[i] = view[i];
    vistaSinTraslacion[12] = 0.0f;
    vistaSinTraslacion[13] = 0.0f;
    vistaSinTraslacion[14] = 0.0f;

    // Las locations de los uniforms quedan cacheadas en ShaderProgram.
    skyboxProgram_->use();
    skyboxProgram_->setMat4("uView", glm::make_mat4(vistaSinTraslacion));
    skyboxProgram_->setMat4("uProjection", glm::make_mat4(projection));
    skyboxProgram_->setInt("uSkybox", 0);

    backend.bindTextureCube(skyboxCubemap_, 0);
    backend.drawMesh(skyboxCuboMalla_, 0, 36);
    backend.bindTextureCube(Rendering::Backend::kInvalidHandle, 0);

    // Restaurar estado.
    backend.setDepthMask(true);
    backend.setDepthFunc(Rendering::Backend::kDepthFuncLess);
    ShaderProgram::unbind(backend);

    return true;
}

// ---------------------------------------------------------------------------
// Skybox: carga de las 6 caras y utilidades de su cache
// ---------------------------------------------------------------------------

// Decodifica las 6 caras con stb_image y sube el cubemap a GPU (con mipmaps),
// dejando el handle en skyboxCubemap_. Devuelve false sin dejar nada cacheado
// si alguna cara no se puede decodificar (o si no todas miden lo mismo) o si el
// backend no puede crear la textura: la pasada cae al degradado. El aviso es
// una sola vez por clave de caras, no por frame.
bool SceneRenderer::cargarCubemap(const std::string rutas[6]) {
    auto& backend = backend_;

    int width = 0, height = 0;
    unsigned char* facePixels[6] = {nullptr, nullptr, nullptr,
                                    nullptr, nullptr, nullptr};
    bool ok = true;

    for (int i = 0; i < 6; ++i) {
        int w = 0, h = 0, ch = 0;
        // Forzar RGBA para que las 6 caras queden con el mismo layout sin
        // depender de cuantos canales trae cada archivo.
        unsigned char* data = stbi_load(rutas[i].c_str(), &w, &h, &ch, 4);

        if (!data) { ok = false; break; }
        if (i == 0) { width = w; height = h; }
        else if (w != width || h != height) { ok = false; stbi_image_free(data); break; }
        else if (w != h) { ok = false; stbi_image_free(data); break; }  // caras deben ser cuadradas
        facePixels[i] = data;
    }
    if (!ok) {
        for (int i = 0; i < 6; ++i) if (facePixels[i]) stbi_image_free(facePixels[i]);
        std::cerr << "[Skybox] no se pudieron decodificar las 6 caras del cubemap "
                     "(faltan, no son legibles, no tienen el mismo tamano o no son cuadradas): "
                  << rutas[0] << " ...; se usa el cielo degradado\n";
        return false;
    }

    Rendering::Backend::IRenderBackend::ImageCube imgCube;
    imgCube.width = width;
    imgCube.height = height;
    for (int i = 0; i < 6; ++i) imgCube.faces[i] = facePixels[i];
    imgCube.generateMipmaps = true;


    skyboxCubemap_ = backend.createTextureCube(imgCube);

    // Liberar memoria CPU ya subida a GPU.
    for (int i = 0; i < 6; ++i) stbi_image_free(facePixels[i]);

    if (skyboxCubemap_ == Rendering::Backend::kInvalidHandle) {
        std::cerr << "[Skybox] el backend no pudo crear la textura cubemap "
                     "(caras: " << rutas[0] << " ...); se usa el cielo degradado\n";
        return false;
    }
    return true;
}
