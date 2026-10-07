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
#include "Script.h"

#include "../Behaviour/ComportamientoCargado.h"
#include "../Behaviour/IScriptBehaviour.h"
#include "../Behaviour/ScriptGameObject.h"
#include "../Behaviour/ScriptRuntime.h"
#include "Configuracion/EditorConfig.h"
#include "../Objetos/GameObject.h"

#include <iostream>
#include <utility>

namespace {
constexpr uint32_t MAGIC_SCRIPT = 0x31535346; // 'F','S','S','1'
constexpr uint32_t VERSION_SCRIPT = 3;

ReflejoScripts::DefCampo metadatosCampo(
    const ReflejoScripts::DefCampo& campo) {
    ReflejoScripts::DefCampo copia;
    copia.nombre = campo.nombre;
    copia.tag = campo.tag;
    copia.subcampos.reserve(campo.subcampos.size());
    for (const ReflejoScripts::DefCampo& subcampo : campo.subcampos)
        copia.subcampos.push_back(metadatosCampo(subcampo));
    return copia;
}

std::vector<ReflejoScripts::DefCampo> metadatosCampos(
    const std::vector<ReflejoScripts::DefCampo>& campos) {
    std::vector<ReflejoScripts::DefCampo> copia;
    copia.reserve(campos.size());
    for (const ReflejoScripts::DefCampo& campo : campos)
        copia.push_back(metadatosCampo(campo));
    return copia;
}

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
    std::ifstream& archivo) {
    size_t cantidad = 0;
    archivo.read(reinterpret_cast<char*>(&cantidad), sizeof(cantidad));
    std::vector<ReflejoScripts::DefCampo> campos;
    campos.reserve(cantidad);
    for (size_t i = 0; i < cantidad; ++i) {
        ReflejoScripts::DefCampo campo;
        size_t largo = 0;
        archivo.read(reinterpret_cast<char*>(&largo), sizeof(largo));
        campo.nombre.resize(largo);
        if (largo > 0)
            archivo.read(&campo.nombre[0],
                         static_cast<std::streamsize>(largo));
        uint8_t tag = 0;
        archivo.read(reinterpret_cast<char*>(&tag), sizeof(tag));
        campo.tag = static_cast<ReflejoScripts::TagTipo>(tag);
        campo.subcampos = cargarMetadatos(archivo);
        campos.push_back(std::move(campo));
    }
    return campos;
}
} // namespace

void Script::setDllPath(std::string dllPath) {
    this->dllPath = dllPath; // guarda el path completo del fuente
    camposInspector_.clear();

    size_t lastSlash = dllPath.find_last_of("/\\");
    size_t lastDot = dllPath.find_last_of('.');

    bool esFuenteValida = !dllPath.empty() &&
                          (dllPath.find(".cpp") != std::string::npos ||
                           dllPath.find(".java") != std::string::npos);

    if (lastSlash != std::string::npos && lastDot != std::string::npos &&
        lastDot > lastSlash && esFuenteValida) {
        this->nameClass =
            dllPath.substr(lastSlash + 1, lastDot - lastSlash - 1);
    } else {
        this->nameClass = "Debe ser <ClassName>.cpp o .java";
    }

    // El fuente cambio: invalidar lo cargado para que recompile en play mode.
    comportamiento_ = ComportamientoCargado{};
    cargado_ = false;
    arrancado_ = false;
    error_.clear();
}

bool Script::cargarActual(std::string& error) {
    if (dllPath.empty()) {
        error = "No hay fuente de script asignado.";
        return false;
    }
    return ScriptRuntime::compilarYCargar(dllPath, nameClass, comportamiento_,
                                          error);
}

void Script::cargarSiNecesario() {
    if (cargado_ || dllPath.empty()) return;
    cargado_ = true;
    if (!cargarActual(error_)) {
        // El detalle se informa en la ventana Estado y en el log del motor
        // (logs/ junto al ejecutable): el panel del componente ya no repite
        // el mensaje de error, solo muestra fuente y SerializeField.
        std::cout << "[scripts] No se pudo cargar '" << dllPath << "':\n"
                  << error_ << std::endl;
        return;
    }
    camposInspector_ = metadatosCampos(comportamiento_.campos);

    // Servicios de escena (audio, busqueda, teclado): la instancia C++ recibe
    // la tabla aca; Java consulta las mismas tablas desde sus metodos nativos
    // Nativo.*. GameScene cablea el contexto real al entrar en Play, antes del
    // primer onStart.
    if (comportamiento_.lenguaje == "cpp" && comportamiento_.instancia)
        static_cast<IScriptBehaviour*>(comportamiento_.instancia)->servicios =
            MotorScript::tablaServicios();

    if (!comportamiento_.campos.empty()) {
        // Emparejar por NOMBRE el arbol leido del .escena con los campos que
        // expone el script recien compilado, ANTES de inyectarlo. Sin esto el
        // cardinal de `valores_` era el que tuviera el archivo, no el de la
        // reflexion: una escena guardada antes de agregar un SerializeField
        // dejaba `valores_` mas corto que `campos` y el inspector leia fuera de
        // rango al editar (ver SettingsScript). Solo se llenaba cuando
        // `valores_` estaba COMPLETAMENTE vacio, que es un caso particular:
        // cualquier desajuste parcial pasaba.
        valores_ = ReflejoScripts::alinearValores(std::move(valores_),
                                                  comportamiento_.campos);
    }

    // Restaurar los valores de SerializeField persistidos en la escena sobre
    // la instancia recien compilada (reemplazos en caliente o editados).
    ScriptRuntime::inyectar(comportamiento_, valores_);
}

void Script::extraerValores() {
    if (comportamiento_.valido())
        valores_ = ScriptRuntime::extraer(comportamiento_);
}

void Script::recargar(GameObject* owner) {
    extraerValores();
    ScriptRuntime::descargar(comportamiento_, owner); // llama onStop si invalido
    cargado_ = false;
    arrancado_ = false;
    cargarSiNecesario();
}

bool Script::necesitaCompilar() const {
    return !dllPath.empty() &&
           (!cargado_ || ScriptRuntime::cambioElFuente(comportamiento_));
}

bool Script::aplicarCarga(GameObject* owner) {
    if (!dllPath.empty() && !cargado_) {
        cargarSiNecesario();
        return comportamiento_.valido();
    }
    if (ScriptRuntime::cambioElFuente(comportamiento_)) recargar(owner);
    return comportamiento_.valido();
}

void Script::actualizar(GameObject* owner, float deltaTime) {
    if (dllPath.empty()) return;

    // La compilacion esta en la cola de GameScene: no compilar inline aca, la
    // cola aplica la carga en su turno (progreso visible en la barra).
    if (aplazarCarga_ && necesitaCompilar()) return;

    if (!cargado_) cargarSiNecesario();
    if (!comportamiento_.valido()) return;

    // Hot reload: si el fuente cambio en disco se recompila y se recarga.
    if (ScriptRuntime::cambioElFuente(comportamiento_)) recargar(owner);
    if (!comportamiento_.valido()) return;

    if (!arrancado_) {
        ScriptRuntime::llamarInicio(comportamiento_, owner);
        arrancado_ = true;
    }
    ScriptRuntime::llamarActualizar(comportamiento_, owner, deltaTime);
}

void Script::detener(GameObject* owner) {
    if (!arrancado_) return;
    ScriptRuntime::llamarDetener(comportamiento_, owner);
    arrancado_ = false;
    extraerValores(); // que la GUI conserve los ultimos valores editados
}

void Script::liberarComportamiento() {
    if (!comportamiento_.cargado) return;
    camposInspector_ = metadatosCampos(comportamiento_.campos);
    ScriptRuntime::descargar(comportamiento_); // sin owner: no dispara onStop
    cargado_ = false;
    arrancado_ = false;
    aplazarCarga_ = false;
}

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
    std::streampos inicio = f->tellg();

    uint32_t magic = 0;
    f->read(reinterpret_cast<char*>(&magic), sizeof(magic));

    if (magic == MAGIC_SCRIPT) {
        uint32_t version = 0;
        f->read(reinterpret_cast<char*>(&version), sizeof(version));

        size_t pathLength = 0;
        f->read(reinterpret_cast<char*>(&pathLength), sizeof(size_t));
        dllPath.resize(pathLength);
        f->read(&dllPath[0], pathLength);
        // Escena nueva: relativa a la raiz de assets; legacy: absoluta intacta.
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        f->read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
        nameClass.resize(nameLength);
        f->read(&nameClass[0], nameLength);

        if (version >= 1)
            valores_ = ReflejoScripts::cargarValoresCampos(*f);
        std::string nombreComponente;
        if (version >= 2) {
            size_t nombreLength = 0;
            f->read(reinterpret_cast<char*>(&nombreLength), sizeof(size_t));
            nombreComponente.resize(nombreLength);
            if (nombreLength > 0)
                f->read(&nombreComponente[0],
                        static_cast<std::streamsize>(nombreLength));
        }
        std::vector<ReflejoScripts::DefCampo> camposGuardados;
        if (version >= 3) camposGuardados = cargarMetadatos(*f);

        setDllPath(dllPath); // valida nombre clase + invalida lo cargado
        nombreComponente_ = std::move(nombreComponente);
        camposInspector_ = camposGuardados.empty()
                               ? metadatosDesdeValores(valores_)
                               : std::move(camposGuardados);
    } else {
        // Formato legacy: solo path + nombre de clase (sin magic).
        f->seekg(inicio); // rebobinar para releer por el camino viejo

        size_t pathLength = 0;
        f->read(reinterpret_cast<char*>(&pathLength), sizeof(size_t));
        dllPath.resize(pathLength);
        f->read(&dllPath[0], pathLength);
        dllPath = EditorConfig::absolutizarRuta(dllPath);

        size_t nameLength = 0;
        f->read(reinterpret_cast<char*>(&nameLength), sizeof(size_t));
        nameClass.resize(nameLength);
        f->read(&nameClass[0], nameLength);
        nombreComponente_.clear();
        setDllPath(dllPath);
    }

    cargado_ = false;
    comportamiento_ = ComportamientoCargado{};
}

void Script::saveComponent(std::ofstream* fileNamePathContentObject) {
    serializeComponent(fileNamePathContentObject);
}

void Script::loadComponent(std::ifstream* fileNamePathContentObject) {
    deserializeComponent(fileNamePathContentObject);
}