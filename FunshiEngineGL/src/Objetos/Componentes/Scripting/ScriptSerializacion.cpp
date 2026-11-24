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
#include "ScriptDetalle.h"

namespace {

std::vector<ReflejoScripts::DefCampo> metadatosDesdeValores(
    const std::vector<ReflejoScripts::ValorCampo>& valores) {
    std::vector<ReflejoScripts::DefCampo> campos;
    campos.reserve(valores.size());
    for (const ReflejoScripts::ValorCampo& valor : valores) {
        ReflejoScripts::DefCampo campo;
        campo.nombre = valor.nombre;
        campo.tag = valor.tag;
        if (valor.tag == ReflejoScripts::TagTipo::Grupo) {
            campo.subcampos =
                metadatosDesdeValores(
                    valor.como<std::vector<ReflejoScripts::ValorCampo>>());
        } else if (valor.tag == ReflejoScripts::TagTipo::Grupos) {
            const auto& grupos = valor.como<
                std::vector<std::vector<ReflejoScripts::ValorCampo>>>();
            if (!grupos.empty())
                campo.subcampos = metadatosDesdeValores(grupos.front());
        }
        campos.push_back(std::move(campo));
    }
    return campos;
}

void guardarMetadatos(std::ofstream& archivo,
                      const std::vector<ReflejoScripts::DefCampo>& campos) {
    const size_t cantidad = campos.size();
    archivo.write(reinterpret_cast<const char*>(&cantidad), sizeof(cantidad));
    for (const ReflejoScripts::DefCampo& campo : campos) {
        const size_t largo = campo.nombre.size();
        archivo.write(reinterpret_cast<const char*>(&largo), sizeof(largo));
        archivo.write(campo.nombre.data(), static_cast<std::streamsize>(largo));
        const auto tag = static_cast<uint8_t>(campo.tag);
        archivo.write(reinterpret_cast<const char*>(&tag), sizeof(tag));
        guardarMetadatos(archivo, campo.subcampos);
    }
}

std::vector<ReflejoScripts::DefCampo> cargarMetadatos(
    std::ifstream& archivo, unsigned profundidad = 0) {
    std::vector<ReflejoScripts::DefCampo> campos;
    // Los metadatos se anidan recursivamente: sin tope de profundidad, un
    // archivo corrupto con grupos anidados desborda la pila.
    if (profundidad > LimitesSerializacion::kProfundidad) {
        archivo.setstate(std::ios::failbit);
        return campos;
    }

    size_t cantidad = 0;
    if (!LimitesSerializacion::leerLongitud(archivo, cantidad,
                                            LimitesSerializacion::kCantidad,
                                            sizeof(size_t), 0)) {
        archivo.setstate(std::ios::failbit);
        return campos;
    }
    campos.reserve(cantidad);
    for (size_t i = 0; i < cantidad; ++i) {
        ReflejoScripts::DefCampo campo;
        size_t largo = 0;
        if (!LimitesSerializacion::leerLongitud(archivo, largo,
                                                LimitesSerializacion::kNombre,
                                                1)) {
            archivo.setstate(std::ios::failbit);
            return {};
        }
        campo.nombre.resize(largo);
        archivo.read(&campo.nombre[0], static_cast<std::streamsize>(largo));
        uint8_t tag = 0;
        archivo.read(reinterpret_cast<char*>(&tag), sizeof(tag));
        if (!archivo || tag > static_cast<uint8_t>(ReflejoScripts::TagTipo::Componente)) {
            // Etiqueta fuera de rango: el archivo esta corrupto y lo que
            // sigue no se puede interpretar.
            archivo.setstate(std::ios::failbit);
            return {};
        }
        campo.tag = static_cast<ReflejoScripts::TagTipo>(tag);
        campo.subcampos = cargarMetadatos(archivo, profundidad + 1);
        if (!archivo) return {};
        campos.push_back(std::move(campo));
    }
    return campos;
}

}  // namespace

void Script::escribirCampo(int indice,
                           const ReflejoScripts::ValorCampo& valor) {
    if (indice < 0 || indice >= static_cast<int>(valores_.size())) return;
    valores_[indice] = valor;
    // Si la instancia esta viva (play mode) reflejar el cambio inmediato.
    // Se despacha por backend: en Java los DefCampo no exponen `acceder` (las
    // variables viven en la JVM), invocar ReflejoScripts::escribirCampo directo
    // lanzaria std::bad_function_call al mover un slider.
    if (comportamiento_.valido()) ScriptRuntime::inyectar(comportamiento_, valores_);
}

void Script::serializeComponent(std::ofstream* f) {
    // 1. Magic + version para distinguir el formato nuevo (con SerializeField)
    uint32_t magic = MAGIC_SCRIPT;
    uint32_t version = VERSION_SCRIPT;
    f->write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    f->write(reinterpret_cast<const char*>(&version), sizeof(version));

    // 2. Campos historicos: path + nombre de clase. La ruta del fuente se
    // persiste relativa a la raiz de assets del proyecto (si esta fijada): los
    // fuentes viven bajo src<nombre>/Scripts, y asi la escena sigue valida al
    // mover/renombrar el proyecto entero.
    const std::string pathGuardado = EditorConfig::relativizarRuta(dllPath);
    size_t pathLength = pathGuardado.size();
    f->write(reinterpret_cast<const char*>(&pathLength), sizeof(size_t));
    f->write(pathGuardado.c_str(), pathLength);

    size_t nameLength = nameClass.size();
    f->write(reinterpret_cast<const char*>(&nameLength), sizeof(size_t));
    f->write(nameClass.c_str(), nameLength);

    // 3. Valores de SerializeField (arbol autodescriptivo)
    extraerValores();
    ReflejoScripts::guardarValoresCampos(*f, valores_);

    const size_t nombreLength = nombreComponente_.size();
    f->write(reinterpret_cast<const char*>(&nombreLength), sizeof(size_t));
    f->write(nombreComponente_.data(),
             static_cast<std::streamsize>(nombreLength));

    if (comportamiento_.valido())
        camposInspector_ = metadatosCampos(comportamiento_.campos);
    if (camposInspector_.empty())
        camposInspector_ = metadatosDesdeValores(valores_);
    guardarMetadatos(*f, camposInspector_);
}

void Script::deserializeComponent(std::ifstream* f) {
    if (!f || !f->is_open()) return;
    std::streampos inicio = f->tellg();

    uint32_t magic = 0;
    f->read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic == MAGIC_SCRIPT) {
        uint32_t version = 0;
        f->read(reinterpret_cast<char*>(&version), sizeof(version));

        size_t pathLength = 0;
        if (!LimitesSerializacion::leerLongitud(*f, pathLength,
                                                LimitesSerializacion::kRuta, 1,
                                                0)) {
            std::cerr << "Ruta de script invalida en el binario (len "
                      << pathLength
                      << "); se corta la lectura del componente\n";
            f->setstate(std::ios::failbit);
            return;
        }
        dllPath.resize(pathLength);
        if (pathLength > 0)
            f->read(&dllPath[0], static_cast<std::streamsize>(pathLength));
        // Escena nueva: relativa a la raiz de assets; legacy: absoluta intacta.
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        if (!LimitesSerializacion::leerLongitud(*f, nameLength,
                                                LimitesSerializacion::kNombre, 1,
                                                0)) {
            std::cerr << "Nombre de clase de script invalido en el binario "
                         "(len "
                      << nameLength
                      << "); se corta la lectura del componente\n";
            f->setstate(std::ios::failbit);
            return;
        }
        nameClass.resize(nameLength);
        if (nameLength > 0)
            f->read(&nameClass[0], static_cast<std::streamsize>(nameLength));

        if (version >= 1)
            valores_ = ReflejoScripts::cargarValoresCampos(*f);
        std::string nombreComponente;
        if (version >= 2) {
            size_t nombreLength = 0;
            if (!LimitesSerializacion::leerLongitud(
                    *f, nombreLength, LimitesSerializacion::kNombre, 1, 0)) {
                std::cerr << "Nombre de componente de script invalido en el "
                             "binario (len "
                          << nombreLength
                          << "); se corta la lectura del componente\n";
                f->setstate(std::ios::failbit);
                return;
            }
            nombreComponente.resize(nombreLength);
            if (nombreLength > 0)
                f->read(&nombreComponente[0],
                        static_cast<std::streamsize>(nombreLength));
        }
        std::vector<ReflejoScripts::DefCampo> camposGuardados;
        if (version >= 3) camposGuardados = cargarMetadatos(*f);

        // NO llamar a setDllPath aqui: ya tenemos dllPath, nameClass, valores_ y
        // camposGuardados leidos del archivo. setDllPath limpiaria camposInspector_
        // y resetearia estado de carga innecesariamente.
        this->dllPath = dllPath;
        this->nameClass = nameClass;
        // comportamiento_ y cargado_ se resetean abajo
        nombreComponente_ = std::move(nombreComponente);
        camposInspector_ = camposGuardados.empty()
                           ? metadatosDesdeValores(valores_)
                           : std::move(camposGuardados);
    } else {
        // Formato legacy: solo path + nombre de clase (sin magic).
        f->seekg(inicio); // rebobinar para releer por el camino viejo

        size_t pathLength = 0;
        if (!LimitesSerializacion::leerLongitud(*f, pathLength,
                                                LimitesSerializacion::kRuta, 1,
                                                0)) {
            std::cerr << "Ruta de script invalida en el binario (len "
                      << pathLength
                      << "); se corta la lectura del componente\n";
            f->setstate(std::ios::failbit);
            return;
        }
        dllPath.resize(pathLength);
        if (pathLength > 0)
            f->read(&dllPath[0], static_cast<std::streamsize>(pathLength));
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        if (!LimitesSerializacion::leerLongitud(*f, nameLength,
                                                LimitesSerializacion::kNombre, 1,
                                                0)) {
            std::cerr << "Nombre de clase de script invalido en el binario "
                         "(len "
                      << nameLength
                      << "); se corta la lectura del componente\n";
            f->setstate(std::ios::failbit);
            return;
        }
        nameClass.resize(nameLength);
        if (nameLength > 0)
            f->read(&nameClass[0], static_cast<std::streamsize>(nameLength));
        nombreComponente_.clear();
        setDllPath(dllPath);
        return; // setDllPath ya resetea cargado_ y comportamiento_
    }

    cargado_ = false;
    comportamiento_ = ComportamientoCargado{};
}
