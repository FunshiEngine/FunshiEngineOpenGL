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
#include "RigidBodyDetalle.h"

void RigidBody::serializeComponent(std::ofstream* fileNamePathContentObject) {
    fileNamePathContentObject->write(
        reinterpret_cast<const char*>(&kRigidBodyMagic),
        sizeof(kRigidBodyMagic));
    fileNamePathContentObject->write(
        reinterpret_cast<const char*>(&kRigidBodyVersion),
        sizeof(kRigidBodyVersion));
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&activo),
                                     sizeof(activo));
    // Guarda masa
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&mass),
                                     sizeof(float));
    // Guarda posicion y rotacion
    fileNamePathContentObject->write(reinterpret_cast<const char*>(pos),
                                     sizeof(float) * 3);
    fileNamePathContentObject->write(reinterpret_cast<const char*>(rot),
                                     sizeof(float) * 4);
    // v2: respuesta a la gravedad, friccion y freeze por ejes.
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&usarGravedad),
                                     sizeof(usarGravedad));
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&escalaGrav),
                                     sizeof(float));
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&friccionCuerpo),
                                     sizeof(float));
    fileNamePathContentObject->write(reinterpret_cast<const char*>(freezePos),
                                     sizeof(bool) * 3);
    fileNamePathContentObject->write(reinterpret_cast<const char*>(freezeRot),
                                     sizeof(bool) * 3);
    // v3: sensor/trigger (detecta sin respuesta fisica).
    fileNamePathContentObject->write(reinterpret_cast<const char*>(&sensor),
                                     sizeof(sensor));
    // Tambien deberia guardar el tipo de collider para reconstruirlo al cargar
}

void RigidBody::leerEstado(std::ifstream* fileNamePathContentObject) {
    const std::streampos inicio = fileNamePathContentObject->tellg();
    std::uint32_t magic = 0;
    fileNamePathContentObject->read(reinterpret_cast<char*>(&magic),
                                    sizeof(magic));
    if (magic == kRigidBodyMagic) {
        std::uint32_t version = 0;
        fileNamePathContentObject->read(reinterpret_cast<char*>(&version),
                                        sizeof(version));
        fileNamePathContentObject->read(reinterpret_cast<char*>(&activo),
                                        sizeof(activo));
        if (version < 1 || version > kRigidBodyVersion) {
            fileNamePathContentObject->setstate(std::ios::failbit);
            return;
        }
        // Leer masa
        fileNamePathContentObject->read(reinterpret_cast<char*>(&mass),
                                        sizeof(float));
        // Leer posicion y rotacion
        fileNamePathContentObject->read(reinterpret_cast<char*>(pos),
                                        sizeof(float) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(rot),
                                        sizeof(float) * 4);
        if (version == 1) {
            // Escenas previas sin estos campos: conservan el estado activo y
            // toman los valores por defecto (cae con el mundo, friccion 0.5).
            usarGravedad = true;
            escalaGrav = 1.0f;
            friccionCuerpo = 0.5f;
            freezePos[0] = freezePos[1] = freezePos[2] = false;
            freezeRot[0] = freezeRot[1] = freezeRot[2] = false;
            sensor = false;
            return;
        }
        fileNamePathContentObject->read(reinterpret_cast<char*>(&usarGravedad),
                                        sizeof(usarGravedad));
        fileNamePathContentObject->read(reinterpret_cast<char*>(&escalaGrav),
                                        sizeof(float));
        fileNamePathContentObject->read(reinterpret_cast<char*>(&friccionCuerpo),
                                        sizeof(float));
        fileNamePathContentObject->read(reinterpret_cast<char*>(freezePos),
                                        sizeof(bool) * 3);
        fileNamePathContentObject->read(reinterpret_cast<char*>(freezeRot),
                                        sizeof(bool) * 3);
        // v3 agrega el flag de sensor; una escena v2 lo deja en falso.
        sensor = false;
        if (version >= 3)
            fileNamePathContentObject->read(reinterpret_cast<char*>(&sensor),
                                            sizeof(sensor));
        return;
    }
    fileNamePathContentObject->clear();
    fileNamePathContentObject->seekg(inicio);
    activo = true;
    // Leer masa
    fileNamePathContentObject->read(reinterpret_cast<char*>(&mass),
                                    sizeof(float));
    // Leer posicion y rotacion
    fileNamePathContentObject->read(reinterpret_cast<char*>(pos),
                                    sizeof(float) * 3);
    fileNamePathContentObject->read(reinterpret_cast<char*>(rot),
                                    sizeof(float) * 4);
    usarGravedad = true;
    escalaGrav = 1.0f;
    friccionCuerpo = 0.5f;
    freezePos[0] = freezePos[1] = freezePos[2] = false;
    freezeRot[0] = freezeRot[1] = freezeRot[2] = false;
    sensor = false;
}

void RigidBody::deserializeComponent(std::ifstream* fileNamePathContentObject) {
    leerEstado(fileNamePathContentObject);

    // Con los datos ya leidos se crea el cuerpo fisico (reentrante).
    createRigidBody();
}
