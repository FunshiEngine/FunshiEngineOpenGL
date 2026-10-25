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
#include "EmisorParticulas.h"

#include "../../Configuracion/EditorConfig.h"
#include "../../Particulas/LectorPerfil.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace {

// Marcador binario del payload del componente. Si no esta, el componente se
// guardo antes de que existiera: se rebobina y no se lee nada, asi que el
// emisor nace con los valores por defecto.
const char kEmisorMagicV1[4] = {'F', 'P', 't', '1'};

// Cota de la ruta del perfil, como los demas paths del motor.
constexpr std::size_t kMaxRutaPerfil = 4096;

void escribirPath(std::ofstream* file, const std::string& path) {
    const std::uint32_t len = static_cast<std::uint32_t>(path.size());
    file->write(reinterpret_cast<const char*>(&len), sizeof(len));
    file->write(path.data(), static_cast<std::streamsize>(len));
}

void leerPath(std::ifstream* file, std::string& path) {
    std::uint32_t len = 0;
    file->read(reinterpret_cast<char*>(&len), sizeof(len));
    if (len > kMaxRutaPerfil) {
        // No desalinear el stream: se salta el bloque y se deja vacio.
        file->seekg(static_cast<std::streamoff>(len), std::ios::cur);
        path.clear();
        return;
    }
    path.assign(len, '\0');
    if (len > 0) file->read(path.data(), static_cast<std::streamsize>(len));
}

}  // namespace

EmisorParticulas::EmisorParticulas() = default;

void EmisorParticulas::setRutaPerfil(const std::string& ruta) {
    rutaPerfil_ = ruta;
    recargarPerfil();
}

void EmisorParticulas::recargarPerfil() {
    perfil_.reset();
    errorPerfil_.clear();
    if (rutaPerfil_.empty()) return;
    auto perfil = std::make_shared<PerfilParticula>();
    std::string error;
    if (!cargarPerfilDesdeJson(EditorConfig::absolutizarRuta(rutaPerfil_), *perfil,
                               error)) {
        errorPerfil_ = error;
        return;
    }
    perfil_ = perfil;
}

std::size_t EmisorParticulas::emitir(SistemaParticulas& sistema,
                                      const float origen[3], float deltaTime) {
    if (!activa_ || deltaTime <= 0.0f) return 0;
    const int tasa = std::max(0, parametros_.cantidadPorSegundo);
    if (tasa <= 0) return 0;

    acumulador_ += deltaTime * static_cast<float>(tasa);
    // Un frame largo (o una pausa) no puede soltar una avalancha de
    // particulas: lo pendiente se acota a diez emitters.
    constexpr float kMaximoPendiente = 10.0f;
    if (acumulador_ > kMaximoPendiente) acumulador_ = kMaximoPendiente;

    // Con perfil, el efecto manda sobre la vida, la velocidad, la gravedad y
    // el rozamiento; el resto sigue viniendo del emisor.
    ParametrosEmisor efectivos = parametros_;
    if (perfil_) {
        efectivos.vidaMinima = perfil_->vidaMinima;
        efectivos.vidaMaxima = perfil_->vidaMaxima;
        efectivos.velocidadMinima = perfil_->velocidadMinima;
        efectivos.velocidadMaxima = perfil_->velocidadMaxima;
        for (int i = 0; i < 3; ++i) efectivos.gravedad[i] = perfil_->aceleracion[i];
        efectivos.rozamiento = perfil_->rozamiento;
    }

    std::size_t emitidas = 0;
    while (acumulador_ >= 1.0f) {
        acumulador_ -= 1.0f;
        sistema.emitir(efectivos, origen, perfil_, texturaParticula_);
        ++emitidas;
    }
    return emitidas;
}

void EmisorParticulas::serializeComponent(std::ofstream* file) {
    file->write(kEmisorMagicV1, sizeof(kEmisorMagicV1));
    const std::int32_t cantidad = parametros_.cantidadPorSegundo;
    file->write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
    file->write(reinterpret_cast<const char*>(&parametros_.vidaMinima),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.vidaMaxima),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.velocidadMinima),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.velocidadMaxima),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.tamanoMinimo),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.tamanoMaximo),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(parametros_.color),
                sizeof(float) * 4);
    file->write(reinterpret_cast<const char*>(parametros_.direccion),
                sizeof(float) * 3);
    file->write(reinterpret_cast<const char*>(&parametros_.dispersion),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(parametros_.gravedad),
                sizeof(float) * 3);
    file->write(reinterpret_cast<const char*>(&parametros_.rozamiento),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.rotacionInicial),
                sizeof(float));
    file->write(reinterpret_cast<const char*>(&parametros_.velocidadRotacion),
                sizeof(float));
    const std::uint8_t activa = activa_ ? 1 : 0;
    file->write(reinterpret_cast<const char*>(&activa), sizeof(activa));
    // La ruta del perfil se persiste relativa a la raiz de assets.
    escribirPath(file, EditorConfig::relativizarRuta(rutaPerfil_));
    // La textura de particula tambien se guarda relativa a la raiz de assets.
    escribirPath(file, EditorConfig::relativizarRuta(texturaParticula_));
}

void EmisorParticulas::deserializeComponent(std::ifstream* file) {
    char magic[4] = {};
    file->read(magic, sizeof(magic));
    const bool conDatos =
        file->gcount() == static_cast<std::streamsize>(sizeof(magic)) &&
        std::memcmp(magic, kEmisorMagicV1, sizeof(magic)) == 0;
    if (!conDatos) {
        // Se guardo antes de que existiera el componente: se rebobina y el
        // emisor se queda con los valores por defecto.
        file->seekg(-static_cast<std::streamoff>(sizeof(magic)), std::ios::cur);
        return;
    }
    std::int32_t cantidad = 0;
    file->read(reinterpret_cast<char*>(&cantidad), sizeof(cantidad));
    parametros_.cantidadPorSegundo = cantidad;
    file->read(reinterpret_cast<char*>(&parametros_.vidaMinima), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.vidaMaxima), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.velocidadMinima), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.velocidadMaxima), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.tamanoMinimo), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.tamanoMaximo), sizeof(float));
    file->read(reinterpret_cast<char*>(parametros_.color), sizeof(float) * 4);
    file->read(reinterpret_cast<char*>(parametros_.direccion), sizeof(float) * 3);
    file->read(reinterpret_cast<char*>(&parametros_.dispersion), sizeof(float));
    file->read(reinterpret_cast<char*>(parametros_.gravedad), sizeof(float) * 3);
    file->read(reinterpret_cast<char*>(&parametros_.rozamiento), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.rotacionInicial), sizeof(float));
    file->read(reinterpret_cast<char*>(&parametros_.velocidadRotacion), sizeof(float));
    std::uint8_t activa = 1;
    file->read(reinterpret_cast<char*>(&activa), sizeof(activa));
    activa_ = activa != 0;
    std::string ruta;
    leerPath(file, ruta);
    rutaPerfil_ = ruta;
    // La textura de particula tambien se guarda relativa.
    leerPath(file, ruta);
    texturaParticula_ = ruta;
    // El perfil se lee cuando la escena ya tiene la raiz de assets resuelta.
}

void EmisorParticulas::onLoaded(GameObject& owner) {
    (void)owner;
    recargarPerfil();
}
