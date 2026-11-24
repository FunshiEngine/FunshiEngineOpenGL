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

void gestionDeObjetosDesdeScripts() {
    SceneRegistry registro;
    PhysicsEngine fisica;
    EventBus bus;
    EditorController editor(&registro, &fisica, &bus, nullptr);
    MotorScript::inyectarServiciosScript(nullptr, &registro, nullptr, &fisica,
                                         &editor);
    const MotorScript::ScriptServices* servicios =
        MotorScript::tablaServicios();
    CHECK(servicios->version >= 4,
          "los servicios publican gravedad global y gestion de objetos");

    float gx = 9.0f;
    float gy = 9.0f;
    float gz = 9.0f;
    fisica.gravedad(gx, gy, gz);
    CHECK(gx == 0.0f && gy == kGravedadPorDefectoY && gz == 0.0f &&
              std::abs(gy + 9.81f) < 1e-4f,
          "la gravedad global por defecto es la terrestre (-9.81)");
    servicios->fijarGravedadGlobal(0.0f, -3.0f, 0.0f);
    CHECK(servicios->gravedadGlobalX() == 0.0f &&
              servicios->gravedadGlobalY() == -3.0f &&
              servicios->gravedadGlobalZ() == 0.0f,
          "los servicios fijan y leen la gravedad global");

    GameObject* creado = static_cast<GameObject*>(
        servicios->crearObjeto("Enemigo", nullptr));
    CHECK(creado != nullptr, "crearObjeto devuelve el objeto de inmediato");
    if (!creado) {
        MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
        return;
    }
    CHECK(!registro.contains(creado),
          "el objeto entra a la escena al final del frame, no adentro");
    CHECK(std::string(creado->inputName) == "Enemigo",
          "el objeto conserva el nombre pedido");
    CHECK(creado->getComponent<Transform>() != nullptr,
          "el objeto nuevo trae Transform para configurarlo ya");
    MotorScript::procesarPeticionesObjetos();
    CHECK(registro.contains(creado),
          "tras procesar, el objeto pertenece a la escena");

    GameObject* duplicado = static_cast<GameObject*>(
        servicios->crearObjeto("Enemigo", nullptr));
    MotorScript::procesarPeticionesObjetos();
    CHECK(duplicado != nullptr && duplicado != creado &&
              std::string(duplicado->inputName) != "Enemigo" &&
              registro.contains(duplicado),
          "el nombre repetido se uniciza al crear");

    CHECK(servicios->agregarColliderEsfera(creado, 1.0f),
          "el servicio agrega collider esfera");
    CHECK(!servicios->agregarColliderCubo(creado, 1.0f),
          "un segundo collider en el mismo frame no se encola");
    MotorScript::procesarPeticionesObjetos();
    CHECK(creado->getComponent<Collider>() != nullptr,
          "el collider entra al final del frame");
    CHECK(servicios->agregarRigidBody(creado, 2.0f),
          "el servicio agrega cuerpo con masa");
    MotorScript::procesarPeticionesObjetos();
    RigidBody* cuerpoCreado = creado->getComponent<RigidBody>();
    CHECK(cuerpoCreado != nullptr && cuerpoCreado->masa() == 2.0f &&
              cuerpoCreado->getRigidBody() &&
              cuerpoCreado->getRigidBody()->isInWorld(),
          "el cuerpo agregado queda registrado en el mundo fisico");
    CHECK(!servicios->agregarRigidBody(creado, 1.0f),
          "un segundo cuerpo no se agrega");

    GameObject* clon = static_cast<GameObject*>(
        servicios->clonarObjeto(creado, nullptr));
    CHECK(clon != nullptr && clon != creado,
          "clonar devuelve la copia de inmediato");
    MotorScript::procesarPeticionesObjetos();
    CHECK(registro.contains(clon), "el clon entra a la escena al procesar");
    CHECK(clon->getId() != creado->getId(),
          "el clon recibe id propio");
    CHECK(clon->getComponent<Collider>() != nullptr &&
              clon->getComponent<RigidBody>() != nullptr,
          "el clon conserva collider y cuerpo");
    RigidBody* cuerpoClon = clon->getComponent<RigidBody>();
    CHECK(cuerpoClon && cuerpoClon->masa() == 2.0f &&
              cuerpoClon->getRigidBody()->isInWorld(),
          "el cuerpo clonado queda registrado en el mundo fisico");

    CHECK(servicios->destruirObjeto(creado),
          "destruir marca el objeto");
    CHECK(registro.contains(creado),
          "el borrado tambien espera al final del frame");
    MotorScript::procesarPeticionesObjetos();
    CHECK(!registro.contains(creado),
          "tras procesar, el objeto ya no esta en la escena");
    CHECK(!servicios->destruirObjeto(creado),
          "destruir dos veces informa que ya no existe");

    MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
    CHECK(servicios->crearObjeto("Tarde", nullptr) == nullptr &&
              !servicios->destruirObjeto(clon) &&
              servicios->clonarObjeto(clon, nullptr) == nullptr &&
              !servicios->agregarColliderEsfera(clon, 1.0f) &&
              !servicios->agregarRigidBody(clon, 1.0f),
          "sin escena inyectada no hay gestion de objetos");
}

// --- Siembra de escena por dimension -----------------------------------------
// La grilla de rejilla es un auxiliar del mundo 3D y la camara inicial se
// coloca segun el plano de juego: en 2D mira el plano XY de frente desde +Z;
// en 3D se eleva para ver la grilla. Es politica pura (sin disco ni UI), por
// eso se prueba sobre las funciones de SiembraEscena.
void siembraPorDimension() {
    CHECK(SiembraEscena::sembrarGrilla(DimensionProyecto::TresD),
          "la grilla de rejilla se siembra en los proyectos 3D");
    CHECK(!SiembraEscena::sembrarGrilla(DimensionProyecto::DosD),
          "la grilla de rejilla no se siembra en los proyectos 2D");

    const SiembraEscena::PosicionInicial camara2d =
        SiembraEscena::posicionCamaraInicial(DimensionProyecto::DosD);
    CHECK(camara2d.x == 0.f && camara2d.y == 0.f && camara2d.z == 40.f,
          "en 2D la camara inicial mira el plano de frente desde +Z");

    const SiembraEscena::PosicionInicial camara3d =
        SiembraEscena::posicionCamaraInicial(DimensionProyecto::TresD);
    CHECK(camara3d.x == 0.f && camara3d.y == 8.f && camara3d.z == 40.f,
          "en 3D la camara inicial se eleva para ver la grilla");
}

// --- Proyeccion de camara (perspectiva / ortografica) ------------------------
// El renderer de lineas clasifica la proyeccion leyendo la matriz (no el estado
// de la camara) y la vista ortografica es la base del modo 2D. Se cubre la
// politica pura (ProyeccionCamara) y la persistencia del tipo de proyeccion y
// del tamano ortografico, incluida la lectura del bloque historico sin cabecera.
void proyeccionDeCamara() {
    const float aspecto = 16.f / 9.f;

    // 1. Proyeccion en perspectiva: se reconoce como no ortografica y la
    //    distancia al plano cercano coincide con la usada para construirla.
    const glm::mat4 perspectivaGlm =
        glm::perspective(glm::radians(60.f), aspecto, 0.5f, 500.f);
    const float* persp = glm::value_ptr(perspectivaGlm);
    CHECK(!ProyeccionCamara::esOrtografica(persp),
          "una proyeccion en perspectiva no se clasifica como ortografica");
    CHECK(std::fabs(ProyeccionCamara::nearDesdeProyeccion(persp) - 0.5f) < 1e-3f,
          "el plano cercano se recupera de la matriz en perspectiva");

    // 2. Proyeccion ortografica: se reconoce y no reporta plano cercano (sin
    //    recorte de lineas por convergencia).
    const glm::mat4 ortograficaGlm =
        glm::ortho(-8.f, 8.f, -4.5f, 4.5f, 0.5f, 500.f);
    const float* ortho = glm::value_ptr(ortograficaGlm);
    CHECK(ProyeccionCamara::esOrtografica(ortho),
          "una proyeccion ortografica se clasifica como ortografica");
    CHECK(ProyeccionCamara::nearDesdeProyeccion(ortho) == 0.f,
          "en ortografica no hay recorte por plano cercano");

    // 3. La camara construye la matriz del tipo que declara: en ortografica la
    //    media altura es orthoSize y la media anchura orthoSize*aspect.
    {
        CameraComponent camara;
        float p[16];
        camara.getProjectionMatrix(p, aspecto);
        CHECK(!ProyeccionCamara::esOrtografica(p),
              "por defecto la camara proyecta en perspectiva");

        camara.setProjection(CameraComponent::Projection::Orthographic);
        camara.setOrthoSize(12.f);
        camara.getProjectionMatrix(p, aspecto);
        CHECK(ProyeccionCamara::esOrtografica(p),
              "una camara ortografica produce una matriz ortografica");
        CHECK(std::fabs(p[0] - 1.f / (12.f * aspecto)) < 1e-4f &&
                  std::fabs(p[5] - 1.f / 12.f) < 1e-4f,
              "la media altura/ancho de la ortografica salen de orthoSize");
    }

    // 4. Ida y vuelta del bloque binario: tipo de proyeccion y orthoSize se
    //    conservan.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_camara_proyeccion");
        const fs::path archivo = carpeta.ruta() / "camara.bin";
        {
            CameraComponent camara;
            camara.setFov(72.f);
            camara.setNearPlane(0.25f);
            camara.setFarPlane(800.f);
            camara.setSpeed(9.f);
            camara.setPintar(true);
            camara.setProjection(CameraComponent::Projection::Orthographic);
            camara.setOrthoSize(20.f);
            std::ofstream out(archivo, std::ios::binary);
            camara.saveComponent(&out);
        }
        CameraComponent camara;
        std::ifstream in(archivo, std::ios::binary);
        camara.loadComponent(&in);
        CHECK(camara.getProjection() ==
                  CameraComponent::Projection::Orthographic,
              "el tipo de proyeccion sobrevive el guardado");
        CHECK(camara.getOrthoSize() == 20.f,
              "el tamano ortografico sobrevive el guardado");
        CHECK(camara.getFov() == 72.f && camara.getNearPlane() == 0.25f &&
                  camara.getFarPlane() == 800.f && camara.getSpeed() == 9.f &&
                  camara.getPintar(),
              "los campos historicos de la camara se conservan");
    }

    // 5. Compatibilidad: un bloque historico (sin cabecera) se lee como
    //    perspectiva con el orthoSize por defecto.
    {
        TempPruebas::CarpetaPrueba carpeta("funshi_camara_legacy");
        const fs::path archivo = carpeta.ruta() / "camara_legacy.bin";
        {
            const float fov = 55.f, nearPlane = 0.1f, farPlane = 400.f,
                        speed = 6.f;
            const bool pintar = false;
            std::ofstream out(archivo, std::ios::binary);
            out.write(reinterpret_cast<const char*>(&fov), sizeof(fov));
            out.write(reinterpret_cast<const char*>(&nearPlane), sizeof(nearPlane));
            out.write(reinterpret_cast<const char*>(&farPlane), sizeof(farPlane));
            out.write(reinterpret_cast<const char*>(&speed), sizeof(speed));
            out.write(reinterpret_cast<const char*>(&pintar), sizeof(pintar));
        }
        CameraComponent camara;
        std::ifstream in(archivo, std::ios::binary);
        camara.loadComponent(&in);
        CHECK(camara.getFov() == 55.f && camara.getFarPlane() == 400.f,
              "el bloque historico de la camara se lee completo");
        CHECK(camara.getProjection() ==
                  CameraComponent::Projection::Perspective,
              "una camara historica se interpreta en perspectiva");
        CHECK(camara.getOrthoSize() == 5.f,
              "una camara historica toma el orthoSize por defecto");
    }
}

// SpriteRenderer: round-trip binario aislado del componente. El path de textura
// se persiste relativizado y se relee absolutizado (como Material), asi que se
// compara el nombre de archivo; el resto de campos (tinte, tamano, region UV) se
// comparan exactos.
void spriteRendererSerializaRoundTrip() {
    TempPruebas::CarpetaPrueba carpeta("funshi_escena_sprite");
    const fs::path archivo = carpeta.ruta() / "sprite.bin";

    SpriteRenderer original;
    original.setRutaTextura("assets/heroe.png");
    original.setTint(0.25f, 0.5f, 0.75f, 0.5f);
    original.setTamano(2.0f, 3.5f);
    original.setUvRect(0.1f, 0.2f, 0.6f, 0.9f);
    {
        std::ofstream out(archivo, std::ios::binary | std::ios::trunc);
        CHECK(out.is_open(), "abre el archivo para guardar el sprite");
        original.saveComponent(&out);
    }

    SpriteRenderer cargado;
    {
        std::ifstream in(archivo, std::ios::binary);
        CHECK(in.is_open(), "abre el archivo para cargar el sprite");
        cargado.loadComponent(&in);
    }

    CHECK(fs::path(cargado.getRutaTextura()).filename() == "heroe.png",
          "la ruta de textura sobrevive el round-trip");
    CHECK(cargado.getTint()[0] == 0.25f && cargado.getTint()[1] == 0.5f &&
              cargado.getTint()[2] == 0.75f && cargado.getTint()[3] == 0.5f,
          "el tinte sobrevive el round-trip");
    CHECK(cargado.getTamano()[0] == 2.0f && cargado.getTamano()[1] == 3.5f,
          "el tamano sobrevive el round-trip");
    CHECK(cargado.getUvRect()[0] == 0.1f && cargado.getUvRect()[1] == 0.2f &&
              cargado.getUvRect()[2] == 0.6f && cargado.getUvRect()[3] == 0.9f,
          "la region UV sobrevive el round-trip");

    // Un componente nuevo arranca con los defaults: quad 1x1, textura completa y
    // sin textura asignada.
    SpriteRenderer nuevo;
    CHECK(nuevo.getRutaTextura().empty(), "un sprite nuevo no tiene textura");
    CHECK(nuevo.getTamano()[0] == 1.0f && nuevo.getTamano()[1] == 1.0f,
          "un sprite nuevo mide 1x1");
    CHECK(nuevo.getUvRect()[0] == 0.0f && nuevo.getUvRect()[2] == 1.0f &&
              nuevo.getUvRect()[3] == 1.0f,
          "un sprite nuevo cubre la textura completa");
}

// La API de scripts expone la musica por ruta (v8 de la tabla de servicios).
void musicaDesdeScripts() {
    TempPruebas::CarpetaPrueba carpeta("funshi_musica_scripts");
    const std::string pista = (carpeta.ruta() / "tema.ogg").string();
    std::ofstream(pista) << "contenido";

    AudioEngine audio;
    audio.esperarPilaVacia();

    MotorScript::inyectarServiciosScript(&audio, nullptr, nullptr);
    const MotorScript::ScriptServices* servicios = MotorScript::tablaServicios();
    CHECK(servicios->version >= 8,
          "los servicios publican la musica por ruta");
    CHECK(servicios->reproducirMusica != nullptr &&
              servicios->detenerMusica != nullptr &&
              servicios->volumenMusica != nullptr,
          "la tabla expone las tres funciones de musica");

    CHECK(servicios->reproducirMusica(pista.c_str(), 0.8f, 0.0f),
          "una pista existente se reproduce desde un script");
    CHECK(!servicios->reproducirMusica("", 1.0f, 0.0f),
          "una pista sin ruta no se reproduce");
    const std::string inexistente = (carpeta.ruta() / "no_existe.ogg").string();
    CHECK(!servicios->reproducirMusica(inexistente.c_str(), 1.0f, 0.0f),
          "una pista cuyo archivo no existe no se reproduce");

    // El volumen del bus y la deteccion no dejan nada sin cerrar.
    servicios->volumenMusica(0.5f);
    servicios->detenerMusica(0.0f);

    // Sin motor de audio la API es tolerante: no reproduce y no falla.
    MotorScript::inyectarServiciosScript(nullptr, nullptr, nullptr);
    CHECK(!servicios->reproducirMusica(pista.c_str(), 1.0f, 0.0f),
          "sin motor de audio la musica no se reproduce");
    servicios->volumenMusica(1.0f);
    servicios->detenerMusica(0.0f);
}
