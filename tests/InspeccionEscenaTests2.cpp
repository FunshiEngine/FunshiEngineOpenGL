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

// --- Tipos y version ------------------------------------------------------------

void probarVersion() {
    Contexto ctx;
    const std::vector<std::string> tipos = ctx.api.tiposDeComponente();
    bool tieneTransform = false;
    bool tieneRigidBody = false;
    for (const std::string& tipo : tipos) {
        if (tipo == "Transform") tieneTransform = true;
        if (tipo == "RigidBody") tieneRigidBody = true;
    }
    CHECK(tieneTransform && tieneRigidBody,
          "la API enumera los tipos de componente del catalogo");
    CHECK(ctx.api.version() == MotorInspeccion::versionApiEscena,
          "la version del contrato es la del modulo de inspeccion");
    CHECK(MotorInspeccion::versionApiEscena != MotorScript::versionRuntimeScript,
          "el contrato de inspeccion y el de scripts se versionan por separado");
}

// --- Guardar y cargar ------------------------------------------------------------

void probarGuardarCargar() {
    TempPruebas::CarpetaPrueba carpeta("funshi_inspeccion_tests");
    const fs::path base = carpeta.ruta();

    EventBus events;
    AssetManager assets;
    SceneRegistry escena;
    EditorController editor(&escena, nullptr, &events, &assets);
    SceneSerializer serializer(&escena, &editor, &assets);
    MotorInspeccion::InspeccionEscena api(&escena, &editor, &serializer);

    std::string error;
    int id = -1;
    CHECK(api.crearObjeto("Enemigo", 0, id, error), "se crea el objeto");
    CHECK(api.asignarVariable(id, "Transform", "position", "1 2 3", error),
          "se le mueve");

    const fs::path archivo = base / "escena.db";
    CHECK(api.guardarEscena(archivo.string(), error), "se guarda la escena");
    CHECK(fs::exists(std::string(archivo.string()) + MotorInspeccion::indiceEscena),
          "el indice de la escena existe");
    CHECK(fs::is_directory(archivo),
          "la escena se guarda en una carpeta con un binario por objeto");

    // Recargar en una escena nueva debe recuperar el cambio: si la escritura
    // del transform se desincronizara de la copia que se serializa, el
    // valor guardado seria el viejo.
    SceneRegistry recargada;
    EditorController editorRecarga(&recargada, nullptr, &events, &assets);
    SceneSerializer serializerRecarga(&recargada, &editorRecarga, &assets);
    MotorInspeccion::InspeccionEscena apiRecarga(&recargada, &editorRecarga,
                                                  &serializerRecarga);
    CHECK(apiRecarga.cargarEscena(archivo.string(), error),
          "la escena guardada se puede cargar");
    std::optional<VistaObjeto> vista = apiRecarga.objetoPorId(id);
    CHECK(vista.has_value() && vista->posicionLocal[0] == 1.0f,
          "el objeto recargado conserva la posicion");

    // Una ruta imposible: dentro de un archivo, no de una carpeta.
    const fs::path bloqueo = base / "bloqueo";
    {
        std::ofstream archivo(bloqueo);
        archivo << "bloquea la carpeta";
    }
    CHECK(!api.guardarEscena((bloqueo / "escena").string(), error),
          "guardar en una ruta imposible falla con motivo");
    CHECK(!error.empty(), "el motivo de la ruta imposible se explica");
    CHECK(!api.cargarEscena((base / "no-existe").string(), error),
          "cargar una escena inexistente falla con motivo");
}

int main() {
    probarTextoValor();
    probarCatalogo();
    probarFiltros();
    probarPosiciones();
    probarMutaciones();
    probarObjetos();
    probarVersion();
    probarGuardarCargar();

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total
              << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
