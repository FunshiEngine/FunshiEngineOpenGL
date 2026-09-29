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
#include "SceneRenderer.h"

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <system_error>

#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <imgui.h>

#include "Backend/IRenderBackend.h"
#include "CacheCubemap.h"
#include "Cielo.h"
#include "../Objetos/Componentes/Skybox.h"

#include "../Herramientas/IconosGUI/stb_image.h"
#include "LineBatch.h"
#include "LineBuilder.h"
#include "LineRenderer.h"
#include "MeshRenderer.h"
#include "RenderTarget.h"
#include "Shaders/ShaderProgram.h"
#include "Shaders/ShaderSources.h"

#include "../Configuracion/Apariencia.h"
#include "../Configuracion/EditorConfig.h"
#include "../Estructuras/ListasEnlazadas/ListasDoblementeEnlazada/ListaDE.h"
#include "../Iluminacion/LightSystem.h"
#include "../Objetos/Componentes/CameraComponent.h"
#include "../Objetos/Componentes/Colliders/Collider.h"
#include "../Objetos/Componentes/Grid.h"
#include "../Objetos/Componentes/Light.h"
#include "../Objetos/Componentes/Transform.h"
#include "../Objetos/GameObject.h"
#include "../Objetos/Modelos3D.h"
#include "../Assets/AssetManager.h"

namespace {

// Media anchura de la banda de degradado del cielo, en unidades de la
// componente vertical de la direccion de vista. El degradado se reparte
// alrededor del horizonte y satura a los dos extremos, asi que 0.35 lleva el
// color inferior a toda la vista al mirar claramente hacia abajo y el superior
// al mirar claramente hacia arriba, dejando la mezcla solo en el horizonte.
constexpr float kCieloTransicion = 0.35f;

}  // namespace

SceneRenderer::SceneRenderer() : meshRenderer_(std::make_unique<MeshRenderer>()) {}

SceneRenderer::~SceneRenderer() = default;

void SceneRenderer::setTextureManager(TextureManager* textureManager) noexcept {
    if (meshRenderer_) meshRenderer_->setTextureManager(textureManager);
}

void SceneRenderer::destruir() {
    grillaRenderer_.destruir();
    // Recursos GPU del Skybox cacheados (malla del cubo y textura del cubemap).
    // Se libera aca con el contexto vivo y se vacia la clave, para que si la
    // escena vuelve a dibujar la pasada suba el cubemap de nuevo en lugar de
    // dar por buena una textura ya borrada.
    auto& backend = Rendering::Backend::activeBackend();
    if (skyboxCubemap_ != Rendering::Backend::kInvalidHandle) {
        backend.destroyTextureCube(skyboxCubemap_);
        skyboxCubemap_ = Rendering::Backend::kInvalidHandle;
    }
    if (skyboxCuboMalla_ != Rendering::Backend::kInvalidHandle) {
        backend.destroyMesh(skyboxCuboMalla_);
        skyboxCuboMalla_ = Rendering::Backend::kInvalidHandle;
    }
    skyboxClave_.clear();
}

// ---------------------------------------------------------------------------
// Pasada principal
// ---------------------------------------------------------------------------

void SceneRenderer::render(const FrameContext& ctx, GameObject* activeCameraObject,
                           CameraComponent* camara) {
    dibujarViewportsPrevios(ctx);

    auto& backend = Rendering::Backend::activeBackend();

    // Pass principal: vuelve al framebuffer de la ventana con su viewport.
    backend.bindDefaultFramebuffer();
    if (ctx.framebufferWidth <= 0 || ctx.framebufferHeight <= 0) {
        viewportsCamaras_.clear();
        viewportsObjetos_.clear();
        return;
    }
    backend.setViewport(0, 0, ctx.framebufferWidth, ctx.framebufferHeight);

    float view[16], projection[16];
    camara->getViewMatrix(view);
    camara->getProjectionMatrix(
        projection,
        static_cast<float>(ctx.framebufferWidth) /
            static_cast<float>(ctx.framebufferHeight));

    static bool diagMatricesPendiente = true;
    if (diagMatricesPendiente) {
        diagMatricesPendiente = false;
        bool noFinita = false;
        for (int i = 0; i < 16; ++i) {
            if (!std::isfinite(view[i]) || !std::isfinite(projection[i])) {
                noFinita = true;
                break;
            }
        }
        const float* diagPos = camara->getPosition();
        const float fwd[3] = {-view[2], -view[6], -view[10]};
        std::cout << "[diag] matrices camara finitas: "
                  << (noFinita ? "NO (NaN/Inf)" : "si") << "; pos camara = ("
                  << diagPos[0] << ", " << diagPos[1] << ", " << diagPos[2]
                  << ") fwd=(" << fwd[0] << ", " << fwd[1] << ", " << fwd[2]
                  << ") fov=" << camara->getFov()
                  << " near=" << camara->getNearPlane()
                  << " far=" << camara->getFarPlane() << std::endl;

        // Inventario de la escena: cuantas luces y cuantas mallas dibujables
        // (con normales) hay, para contrastar con lo que se ve en pantalla.
        std::cout << "[diag] luces_en_escena=";
        auto* diagObjects = ctx.gameObjects;
        int diagLuces = 0;
        int diagObjs = 0;
        int diagConMalla = 0;
        int diagConMallaYNormales = 0;
        if (diagObjects && !diagObjects->isEmpty()) {
            Position<GameObject*>* pos = diagObjects->first();
            while (pos && pos->getElement()) {
                GameObject* o = pos->getElement();
                ++diagObjs;
                if (o->getComponent<Light>()) ++diagLuces;
                auto* m = dynamic_cast<Modelos3D*>(o);
                if (m && m->getMesh() && !m->getMesh()->isEmpty()) {
                    ++diagConMalla;
                    if (m->getMesh()->hasNormals()) ++diagConMallaYNormales;
                }
                pos = (pos != diagObjects->last()) ? diagObjects->next(pos)
                                                   : nullptr;
            }
        }
        std::cout << diagLuces << " objetos=" << diagObjs
                  << " conMalla=" << diagConMalla
                  << " conMallaYNormales=" << diagConMallaYNormales
                  << std::endl;
    }

    dibujarEscena(ctx, view, projection, activeCameraObject, ctx.framebufferWidth,
                  ctx.framebufferHeight, true);

    static bool diagPostPassPendiente = true;
    if (diagPostPassPendiente) {
        diagPostPassPendiente = false;
        std::cout << "[diag] MeshRenderer moderno disponible="
                  << (meshRenderer_ && meshRenderer_->available() ? "si" : "no")
                  << std::endl;
    }
}

// ---------------------------------------------------------------------------
// Dibujo de la escena (grilla + luces + objetos) desde una vista/proyeccion.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarEscena(const FrameContext& ctx,
                                  const float view[16],
                                  const float projection[16],
                                  GameObject* camaraOjo, int viewportAncho,
                                  int viewportAlto, bool esPasadaPrincipal) {
    auto& backend = Rendering::Backend::activeBackend();

    // Estado de la pasada de lineas: el shader de lineas grosses necesita las
    // matrices de la camara y el tamano del viewport para pasar el ancho de
    // pixeles a NDC. Se fija aca porque TODA pasada (principal y vistas previas)
    // entra por esta funcion.
    lineRenderer().setVista(view, projection);
    lineRenderer().setViewport(viewportAncho, viewportAlto);

    // Posicion de la camara en el mundo a partir de su matriz de vista:
    // view = [R | t] (column-major), ojo = -(R^T * t). La usa la grilla para
    // extender el plano hasta el horizonte y difuminarlo por distancia.
    float camaraMundo[3];
    camaraMundo[0] =
        -(view[0] * view[12] + view[1] * view[13] + view[2] * view[14]);
    camaraMundo[1] =
        -(view[4] * view[12] + view[5] * view[13] + view[6] * view[14]);
    camaraMundo[2] =
        -(view[8] * view[12] + view[9] * view[13] + view[10] * view[14]);

    // Cielo degradado: primera pasada, con depth test on + depth mask off para
    // que quede "detras" de todo sin escribir en el z-buffer. Recibe la
    // posicion de camara en el mundo (ya calculada arriba) porque el color del
    // cielo sale de la direccion de vista, no de la posicion del pixel.
    dibujarCielo(ctx, view, projection, camaraMundo);

    // La grilla se dibuja como una pasada independiente del renderer de
    // modelos: no depende de Modelos3D ni del recorrido normal de las
    // entidades.
    dibujarGrillaEditor(ctx, camaraMundo);

    // Luces de la pasada: van como uniforms del shader (MeshRenderer), que es
    // la unica via de iluminacion que queda.
    prepararLucesFrame(ctx);

    dibujarGameObjectsConOjo(ctx, camaraOjo, view, projection);

    // Guia de eje: despues de los objetos para que la recta se vea por encima
    // de la malla, y solo en la pasada principal.
    if (esPasadaPrincipal) dibujarGuiaEje(ctx, camaraMundo);

    ShaderProgram::unbind();
}

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

    auto& backend = Rendering::Backend::activeBackend();

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
    if (!cieloProgram_ && !cieloShaderFallado_) {
        try {
            cieloProgram_ = std::make_unique<ShaderProgram>(
                ShaderProgram::fromSource(kSkyVertexShader, kSkyFragmentShader));
        } catch (const std::exception& e) {
            std::cerr << "[Cielo] shader del degradado no disponible: " << e.what()
                      << '\n';
            cieloShaderFallado_ = true;
            return;
        } catch (...) {
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
    ShaderProgram::unbind();
}

// Ultima modificacion del archivo en unidades del reloj de la filesystem
// (segundos desde el epoch en la mayoria de los SO); 0 si el archivo no existe
// o no se puede consultar. La variante con error_code no lanza: una cara que
// falte es un dato para la clave, no una excepcion.
static std::int64_t mtimeSegundos(const std::string& ruta) {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(std::filesystem::path(ruta), ec);
    if (ec) return 0;
    return static_cast<std::int64_t>(t.time_since_epoch().count());
}

// Skybox cubemap: renderiza un cubo centrado en la camara con el cubemap
// del componente Skybox. Se dibuja con depth test ON + depth mask OFF para
// quedar "detras" de toda la geometria sin escribir profundidad.
bool SceneRenderer::dibujarSkyboxCubemap(const Skybox* skybox,
                                         const float view[16],
                                         const float projection[16]) {
    if (!skybox) return false;

    auto& backend = Rendering::Backend::activeBackend();

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
    if (!skyboxProgram_ && !skyboxShaderFallado_) {
        static const char* skyboxVert = R"(#version 330 core
layout(location = 0) in vec3 aPos;
out vec3 vTexCoord;
uniform mat4 uView;
uniform mat4 uProjection;
void main() {
    vTexCoord = aPos;
    vec4 pos = uProjection * uView * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // z = w para estar en el plano lejano
}
)";
        static const char* skyboxFrag = R"(#version 330 core
in vec3 vTexCoord;
out vec4 FragColor;
uniform samplerCube uSkybox;
void main() {
    FragColor = texture(uSkybox, vTexCoord);
}
)";
        try {
            skyboxProgram_ = std::make_unique<ShaderProgram>(
                ShaderProgram::fromSource(skyboxVert, skyboxFrag));
        } catch (const std::exception& e) {
            std::cerr << "[Skybox] shader del cubemap no disponible: " << e.what()
                      << '\n';
            skyboxShaderFallado_ = true;
            return false;
        } catch (...) {
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
    backend.drawMesh(skyboxCuboMalla_, 36);

    // Restaurar estado.
    backend.setDepthMask(true);
    backend.setDepthFunc(Rendering::Backend::kDepthFuncLess);
    ShaderProgram::unbind();

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
    auto& backend = Rendering::Backend::activeBackend();

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

// Recta guia del objeto seleccionado (teclas X/Y/Z): la recta sobre la que
void SceneRenderer::dibujarGuiaEje(const FrameContext& ctx,
                                   const float camaraMundo[3]) {
    if (ctx.guiaEje < GuiaEje::kEjeX || ctx.guiaEje > GuiaEje::kEjeZ) return;
    GameObject* object = ctx.selectedObject;
    if (!object) return;

    Transform* transform = object->getGlobalTransform();
    if (!transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    GuiaEje::Eje eje;
    if (!GuiaEje::calcularEje(modelArr, ctx.guiaEje,
                              ctx.guiaCoordenadasGlobales, &eje))
        return;

    // El color del eje se mide contra el color de la grilla (misma regla que
    // sus ejes): si no, la guia se pierde sobre una grilla de su mismo color.
    float color[4];
    float referencia[3];
    if (colorReferenciaGuia(ctx, referencia)) {
        GuiaEje::colorEfectivo(ctx.guiaEje, referencia, color);
    } else {
        GuiaEje::colorEje(ctx.guiaEje, color);
    }
    // Mismo radio que la grilla (radioDifuminado lee el perfil de apariencia),
    // asi que guia y piso se desvanecen siempre en el mismo circulo.
    // Mas subdivisiones que la grilla: la guia es mucho mas larga que una linea
    // de la grilla, asi que con los 6 trozos de aquella el degradado se veria
    // escalonado a lo largo de toda la recta.
    const Difuminado dif = Difuminado::desdeRadio(
        radioDifuminado(ctx), 24);

    LineBuilder builder;
    GuiaEje::emitir(builder, eje, camaraMundo, color, dif);
    // Geometria ya en mundo (la recta sale de la matriz global): model = null.
    lineRenderer().dibujar(builder, marcadoresBatch_, nullptr, 3.0f);
}

void SceneRenderer::prepararLucesFrame(const FrameContext& ctx) {
    if (!meshRenderer_) return;
    meshRenderer_->setLuces(ctx.lights, ctx.lightCount, ctx.globalAmbient);
}

void SceneRenderer::dibujarGameObjectsConOjo(const FrameContext& ctx,
                                             GameObject* camaraOjo,
                                             const float view[16],
                                             const float projection[16]) {
    auto* gameObjects = ctx.gameObjects;
    if (!gameObjects || gameObjects->isEmpty()) return;
    Position<GameObject*>* pos = gameObjects->first();
    while (pos && pos->getElement()) {
        dibujarObjectConOjo(ctx, pos->getElement(), camaraOjo, view, projection);
        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}

void SceneRenderer::dibujarObjectConOjo(const FrameContext& ctx,
                                        GameObject* object,
                                        GameObject* camaraOjo,
                                        const float view[16],
                                        const float projection[16]) {
    object->setTam(10);
    object->setColor(object->auxColor);

    if (object->getComponent<Transform>()) {
        // El dibujado va siempre por el pipeline moderno (MeshRenderer: VBO/VAO
        // + shader). Si el objeto no tiene malla con normales, simplemente no
        // se dibuja (MeshRenderer lo avisa una vez por malla).
        auto* modelo = dynamic_cast<Modelos3D*>(object);
        if (modelo && meshRenderer_) {
            meshRenderer_->intentarRender(modelo, view, projection,
                                          ctx.deltaTime);
        } else {
            // Tambien renderizar GameObjects con componente Model (no Modelos3D)
            if (auto* model = object->getComponent<Model>(); model && meshRenderer_ && ctx.assetManager) {
                const std::string& path = model->getPath();
                if (!path.empty()) {
                    // Cargar malla via AssetManager (cache compartida)
                    auto mesh = ctx.assetManager->getMesh(path);
                    if (mesh && !mesh->isEmpty() && mesh->hasNormals()) {
                        // Render temporal: usamos MeshRenderer internamente
                        // Creando un objeto temporal con la malla
                        static Modelos3D tempModel(nullptr);
                        tempModel.setAssetManager(ctx.assetManager);
                        tempModel.setPath(path);
                        meshRenderer_->intentarRender(&tempModel, view, projection, ctx.deltaTime);
                    }
                }
            }
        }
    }

    if (object->getComponent<Light>()) dibujarMarcadorLuz(object);

    if (object->getComponent<CameraComponent>() && object != camaraOjo)
        dibujarMarcadorCamara(object);

    // Wireframe del collider en la escena 3D: SOLO mientras el gizmo del
    // offset del collider esta habilitado para este objeto (checkbox "Gizmo
    // activo" del transform del collider).
    if (ctx.editorActivo && object != camaraOjo && ctx.selectedObject) {
        Collider* collider = object->getComponent<Collider>();
        Transform* colliderTransform =
            collider ? collider->getTransform() : nullptr;

        if (collider && colliderTransform &&
            colliderTransform->gizmoHabilitado &&
            collider->getOwner() == ctx.selectedObject) {
            collider->dibujarCollider();
        }
    }
}

// Gizmo visual de una luz: un octaedro alambre amarillo en la posicion del
// objeto, para poder ubicar y seleccionar luces que no tienen cuerpo.
void SceneRenderer::dibujarMarcadorLuz(GameObject* object) {
    Transform* transform = object->getGlobalTransform();
    if (!transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    const float size = 0.5f;
    const float v[6][3] = {
        { 1.f, 0.f, 0.f}, {-1.f, 0.f, 0.f},
        { 0.f, 1.f, 0.f}, { 0.f,-1.f, 0.f},
        { 0.f, 0.f, 1.f}, { 0.f, 0.f,-1.f}};
    const int edges[12][2] = {
        {0,2},{0,3},{0,4},{0,5},
        {1,2},{1,3},{1,4},{1,5},
        {2,4},{2,5},{3,4},{3,5}};

    // Los vertices se escalan (octaedro chico) y el dibujo lo hace la capa de
    // Rendering con el batch de lineas.
    float vsize[6][3];
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 3; ++j) vsize[i][j] = v[i][j] * size;

    const float color[4] = {1.f, 0.85f, 0.1f, 1.f};
    LineBuilder builder;
    builder.agregarAristas(&vsize[0][0], 6, &edges[0][0], 12, color);
    lineRenderer().dibujar(builder, marcadoresBatch_, modelArr, 2.0f);
}

// Gizmo visual de una camara secundaria: frustum de vision alambre cian. La
// camara activa no dibuja el suyo (seria visera en la propia vista).
void SceneRenderer::dibujarMarcadorCamara(GameObject* object) {
    CameraComponent* camara = object->getComponent<CameraComponent>();
    Transform* transform = object->getGlobalTransform();
    if (!camara || !transform) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    ImGuiIO& io = ImGui::GetIO();
    const float aspect = (io.DisplaySize.x > 0.f && io.DisplaySize.y > 0.f)
                             ? io.DisplaySize.x / io.DisplaySize.y
                             : 1.77f;

    const float tanHalf =
        std::tan(camara->getFov() * 0.5f * 3.14159265358979f / 180.f);
    const float nearDist = camara->getNearPlane();
    const float farDist = camara->getFarPlane();
    const float halfHNear = tanHalf * nearDist;
    const float halfWNear = halfHNear * aspect;
    const float halfHFar = tanHalf * farDist;
    const float halfWFar = halfHFar * aspect;

    // Frustum: 4 esquinas del plano near (0..3) + 4 del far (4..7). Los 12
    // bordes de "edges" indexan los 8 puntitos, por eso todo vive en un solo
    // array (antes far y near estaban separados y se leia fuera de rango).
    const float vFrustum[8][3] = {
        {-halfWNear, -halfHNear, -nearDist},
        { halfWNear, -halfHNear, -nearDist},
        {-halfWNear,  halfHNear, -nearDist},
        { halfWNear,  halfHNear, -nearDist},
        {-halfWFar,  -halfHFar,  -farDist},
        { halfWFar,  -halfHFar,  -farDist},
        {-halfWFar,   halfHFar,  -farDist},
        { halfWFar,   halfHFar,  -farDist}};
    const int edges[12][2] = {
        {0,1},{0,2},{3,1},{3,2},
        {4,5},{4,6},{7,5},{7,6},
        {0,4},{1,5},{2,6},{3,7}};

    const float color[4] = {0.3f, 0.8f, 0.9f, 1.f};
    LineBuilder builder;
    builder.agregarAristas(&vFrustum[0][0], 8, &edges[0][0], 12, color);
    lineRenderer().dibujar(builder, marcadoresBatch_, modelArr, 2.0f);
}

void SceneRenderer::dibujarGrillaEditor(const FrameContext& ctx,
                                        const float camaraMundo[3]) {
    auto* gameObjects = ctx.gameObjects;

    if (!gameObjects || gameObjects->isEmpty()) return;

    Position<GameObject*>* pos = gameObjects->first();

    while (pos && pos->getElement()) {
        GameObject* object = pos->getElement();

        if (object->getComponent<Grid>() != nullptr) {
            dibujarGrilla(ctx, object, camaraMundo);
            return;
        }

        pos = (pos != gameObjects->last()) ? gameObjects->next(pos) : nullptr;
    }
}

void SceneRenderer::dibujarGrilla(const FrameContext& ctx, GameObject* object,
                                  const float camaraMundo[3]) {
    Grid* grid = object->getComponent<Grid>();
    Transform* transform = object->getGlobalTransform();
    if (!grid || !grid->getVisible() || !transform || !ctx.apariencia) return;

    float modelArr[16];
    buildMatrixFromTransform(transform, modelArr);

    // Color efectivo de la grilla segun el perfil de apariencia: en modo
    // blanco y negro se ignora el color del componente y se usa el contraste
    // puro (la geometria del frame se genera con el color efectivo).
    float colorGrilla[3];
    AparienciaUtil::grillaEfectiva(*ctx.apariencia, grid->getColor(),
                                   colorGrilla);

    // El dibujado (extent infinito del plano + difuminado del horizonte con
    // densidad fija + anchos) vive en la capa de Rendering; aqui se le pasa la
    // matriz del objeto "Grilla" y la posicion del ojo en el mundo.
    grillaRenderer_.dibujar(modelArr, colorGrilla, camaraMundo,
                            Difuminado::desdeRadio(radioDifuminado(ctx),
                                                   GrillaRenderer::kSubdivisiones));
}

float SceneRenderer::radioDifuminado(const FrameContext& ctx) {
    // Sin perfil de apariencia en la pasada (una vista previa, por ejemplo) se
    // usa el valor por defecto, que es el mismo que pone una configuracion
    // recien creada. El acotado al rango admitido lo hace Difuminado::desdeRadio.
    if (!ctx.apariencia) return AparienciaUtil::kRadioDifuminadoPorDefecto;
    return ctx.apariencia->radioDifuminado;
}

bool SceneRenderer::colorReferenciaGuia(const FrameContext& ctx,
                                        float out[3]) const {
    if (!ctx.apariencia || !out) return false;

    // Mismo criterio que elegir la grilla que se dibuja: gana el PRIMER objeto
    // con componente Grid, y si ese no se ve no se dibuja ninguna (el fallback
    // al fondo es lo que le corresponde a la guia en ese caso).
    auto* objetos = ctx.gameObjects;
    if (objetos && !objetos->isEmpty()) {
        Position<GameObject*>* pos = objetos->first();
        while (pos && pos->getElement()) {
            GameObject* object = pos->getElement();
            Grid* grid = object->getComponent<Grid>();
            if (grid) {
                if (grid->getVisible() && object->getGlobalTransform()) {
                    AparienciaUtil::grillaEfectiva(*ctx.apariencia,
                                                   grid->getColor(), out);
                    return true;
                }
                break;
            }
            pos = objetos->last() == pos ? nullptr : objetos->next(pos);
        }
    }

    AparienciaUtil::fondoEfectivo(*ctx.apariencia, out);
    return true;
}

// ---------------------------------------------------------------------------
// Pasada de vista previa (Fase 2): por cada camara con "Vista previa" activo
// se pinta la escena a una textura FBO que luego muestra una ventana ImGui.
// ---------------------------------------------------------------------------

void SceneRenderer::dibujarViewportsPrevios(const FrameContext& ctx) {
    auto* gameObjects = ctx.gameObjects;
    std::vector<std::unique_ptr<RenderTarget>> nuevos;
    std::vector<GameObject*> nuevosObjetos;
    if (gameObjects && !gameObjects->isEmpty()) {
        Position<GameObject*>* pos = gameObjects->first();
        while (pos && pos->getElement()) {
            GameObject* objeto = pos->getElement();
            CameraComponent* camara = objeto->getComponent<CameraComponent>();
            if (camara && camara->getPintar()) {
                camara->setUp(objeto);

                // Reutilizar el FBO del frame anterior del mismo objeto en
                // lugar de recrearlo (evita churn de texturas en el GPU).
                std::unique_ptr<RenderTarget> target;
                for (size_t i = 0; i < viewportsCamaras_.size(); ++i) {
                    if (viewportsObjetos_[i] == objeto) {
                        target.reset(viewportsCamaras_[i].release());
                        break;
                    }
                }
                if (!target) target = std::make_unique<RenderTarget>();

                target->resize(kPreviewW, kPreviewH);
                target->bind();

                auto& backend = Rendering::Backend::activeBackend();
                backend.setViewport(0, 0, kPreviewW, kPreviewH);
                // El FBO hereda el estado GL; se fija el fondo del perfil para
                // que la vista previa use el mismo color que la pasada principal.
                float fondoPreview[3] = {0.f, 0.f, 0.f};
                if (ctx.apariencia)
                    AparienciaUtil::fondoEfectivo(*ctx.apariencia, fondoPreview);
                backend.clearScreen(fondoPreview);

                float view[16], projection[16];
                camara->getViewMatrix(view);
                camara->getProjectionMatrix(
                    projection,
                    static_cast<float>(kPreviewW) /
                        static_cast<float>(kPreviewH));
                // La vista previa de camara NO muestra la guia de eje: es una
                // ayuda del editor sobre la pasada principal (en el preview
                // ocuparia la imagen sin que el usuario la haya pedido).
                dibujarEscena(ctx, view, projection, objeto, kPreviewW,
                              kPreviewH, false);

                backend.bindDefaultFramebuffer();

                nuevos.push_back(std::move(target));
                nuevosObjetos.push_back(objeto);
            }
            pos = (pos != gameObjects->last()) ? gameObjects->next(pos)
                                               : nullptr;
        }
    }
    viewportsCamaras_ = std::move(nuevos);
    viewportsObjetos_ = std::move(nuevosObjetos);
}
