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
#include "EditorConfig.h"

#include "ConfigPersistence.h"
#include "ProjectManager.h"
#include "ProjectPaths.h"

// ============================================================================
// Configuracion del editor: fachada sobre la UNICA implementacion del esquema
// (ConfigPersistence, JSON con nlohmann vendoriado en External/nlohmann) y de
// las rutas (ProjectPaths), mas el estado en memoria (Datos) y el guardado
// diferido de la configuracion general. El ciclo de vida de los proyectos
// (CRUD, migraciones y estructura en disco) vive en ProjectManager; aqui solo
// se delega. Carga tolerante: si el archivo falta, esta corrupto o le faltan
// campos, se quedan los valores por defecto y se conserva lo que si se pudo
// leer. El binario solo persiste datos de escena, no configuracion.
// ============================================================================

// ============================================================================
// Rutas del motor: una sola fuente de verdad es ProjectPaths (mismo ancla, el
// directorio del ejecutable). Estas funciones quedan como fachada estable que
// ya usan main, escenas, tests y el exportador; toda la composicion de rutas
// vive en ProjectPaths.
// ============================================================================

std::string EditorConfig::directorioBaseMotorGrafico() {
    return ProjectPaths::directorioBase();
}

std::string EditorConfig::directorioProyects() {
    return ProjectPaths::directorioProyects();
}

std::string EditorConfig::directorioConfiguraciones() {
    return ProjectPaths::directorioConfiguraciones();
}

std::string EditorConfig::directorioProyecto(const std::string& nombreProyecto) {
    return ProjectPaths::directorioProyecto(nombreProyecto);
}

std::string EditorConfig::directorioMemory(const std::string& nombreProyecto) {
    return ProjectPaths::directorioMemory(nombreProyecto);
}

std::string EditorConfig::directorioSrc(const std::string& nombreProyecto) {
    return ProjectPaths::directorioSrc(nombreProyecto);
}

std::string EditorConfig::nombreRaizSrc(const std::string& nombreProyecto) {
    return ProjectPaths::nombreRaizSrc(nombreProyecto);
}

std::string EditorConfig::rutaConfiguracionGeneral() {
    return ProjectPaths::rutaConfiguracionGeneral();
}

std::string EditorConfig::rutaConfiguracionProyecto(const std::string& nombreProyecto) {
    return ProjectPaths::rutaConfiguracionProyecto(nombreProyecto);
}

std::string EditorConfig::rutaConfiguracion(const std::string& nombreProyecto) {
    // Alias de compatibilidad: apunta a la configuracion del proyecto.
    return rutaConfiguracionProyecto(nombreProyecto);
}

std::string EditorConfig::directorioBinarios(const std::string& nombreProyecto) {
    return ProjectPaths::directorioBinarios(nombreProyecto);
}

std::string EditorConfig::rutaScenePrefijo(const std::string& nombreProyecto) {
    return ProjectPaths::rutaScenePrefijo(nombreProyecto);
}

std::string EditorConfig::rutaSceneBBDD(const std::string& nombreProyecto) {
    return ProjectPaths::rutaSceneBBDD(nombreProyecto);
}

std::string EditorConfig::rutaSceneDir(const std::string& nombreProyecto) {
    return ProjectPaths::rutaSceneDir(nombreProyecto);
}

std::string EditorConfig::rutaImguiIni(const std::string& nombreProyecto) {
    return ProjectPaths::rutaImguiIni(nombreProyecto);
}

std::string EditorConfig::directorioSonidos(const std::string& nombreProyecto) {
    return ProjectPaths::directorioSonidos(nombreProyecto);
}

std::string EditorConfig::directorioInterfaces(const std::string& nombreProyecto) {
    return ProjectPaths::directorioInterfaces(nombreProyecto);
}

std::string EditorConfig::directorioExportaciones() {
    return ProjectPaths::directorioExportaciones();
}

std::string EditorConfig::directorioExportacion(const std::string& nombreExportacion) {
    return ProjectPaths::directorioExportacion(nombreExportacion);
}

void EditorConfig::asegurarEstructuraProyecto(const std::string& nombreProyecto) {
    // Toda la estructura y las migraciones legacy viven en ProjectManager:
    // aqui solo se delega (las rutas que usa ProjectManager coinciden con las
    // de ProjectPaths, que alimenta a EditorConfig).
    ProjectManager::asegurarEstructuraBase();
    const std::string nombre =
        nombreProyecto.empty() ? "Nuevo Proyecto" : nombreProyecto;
    ProjectManager::crearProyecto(nombre);
}

std::string EditorConfig::rutaPorDefecto() {
    // La "ruta por defecto" al arrancar sin argumento es la config general,
    // que vive en la raiz de MotorGrafico junto a las carpetas de proyecto.
    // Desde ahi se lee el ultimo proyecto abierto para saber que proyecto cargar.
    return rutaConfiguracionGeneral();
}

bool EditorConfig::renombrarProyecto(const std::string& viejo,
                                     const std::string& nuevo) {
    return ProjectManager::renombrarProyecto(viejo, nuevo);
}

bool EditorConfig::eliminarProyecto(const std::string& nombre) {
    return ProjectManager::eliminarProyecto(nombre);
}

bool EditorConfig::crearProyectoPorDefecto() {
    return ProjectManager::crearProyectoPorDefecto();
}

std::string EditorConfig::directorioProyectoPorDefecto() {
    return directorioMemory("Nuevo Proyecto");
}

// ============================================================================
// Raiz de assets activa (src<nombre> del proyecto abierto) y conversion de
// rutas para la serializacion portable (ver EditorConfig.h).
// ============================================================================

namespace {

// Raiz de assets del proyecto abierto en main; vacia cuando no hay proyecto
// (tests, menú). Con raiz vacia las rutas se guardan/cargan tal cual.
std::string& raizAssets() {
    static std::string raiz;
    return raiz;
}

// Dada una ruta y un prefijo candidato, dice si ruta cae exactamente bajo
// prefijo (igual o seguida de un separador), respetando NUNCA igualar un
// prefijo que no cierre en un separador (p.ej. "srcA" no debe cubrir "srcAb").
bool rutaBajo(const std::string& ruta, const std::string& prefijo) {
    if (ruta.size() < prefijo.size() ||
        ruta.compare(0, prefijo.size(), prefijo) != 0)
        return false;
    if (ruta.size() == prefijo.size()) return true;
    const char sep = ruta[prefijo.size()];
    return sep == '/' || sep == '\\';
}

// Heuristica de ruta absoluta (legacy): empieza con separador (unix/windows)
// o con letra de unidad ("C:"). Los almacenados relativos (nuevo formato)
// nunca empiezan asi.
bool esRutaAbsoluta(const std::string& ruta) {
    if (ruta.empty()) return false;
    if (ruta[0] == '/' || ruta[0] == '\\') return true;
    return ruta.size() >= 2 &&
           std::isalpha(static_cast<unsigned char>(ruta[0])) && ruta[1] == ':';
}

} // namespace

void EditorConfig::fijarRaizAssets(const std::string& srcRoot) noexcept {
    raizAssets() = srcRoot;
}

void EditorConfig::limpiarRaizAssets() noexcept {
    raizAssets().clear();
}

bool EditorConfig::hayRaizAssets() noexcept {
    return !raizAssets().empty();
}

std::string EditorConfig::relativizarRuta(const std::string& rutaAbsoluta) {
    const std::string& raiz = raizAssets();
    if (raiz.empty() || !rutaBajo(rutaAbsoluta, raiz)) return rutaAbsoluta;
    if (rutaAbsoluta.size() == raiz.size()) return std::string();
    return rutaAbsoluta.substr(raiz.size() + 1);
}

std::string EditorConfig::absolutizarRuta(const std::string& rutaGuardada) {
    const std::string& raiz = raizAssets();
    if (raiz.empty() || rutaGuardada.empty() || esRutaAbsoluta(rutaGuardada))
        return rutaGuardada;
    return raiz + "/" + rutaGuardada;
}

std::string EditorConfig::reemplazarPrefijoRuta(const std::string& ruta,
                                                const std::string& anterior,
                                                const std::string& reemplazo) {
    if (anterior.empty() || anterior == reemplazo || !rutaBajo(ruta, anterior))
        return std::string();
    return reemplazo + ruta.substr(anterior.size());
}

// ============================================================================
// Carga: la funcion publica `cargar` es el punto de entrada historico;
// delega en cargarGeneral + cargarProyecto para el arranque completo.
// ============================================================================

void EditorConfig::cargar(const std::string& ruta) {
    // ruta puede ser la config general (nueva) o la legacy; en ambos casos
    // cargamos primero la config general para obtener nombreProyecto, y luego
    // la config especifica de ese proyecto.
    cargarGeneral(ruta.empty() ? rutaConfiguracionGeneral() : ruta);
    cargarProyecto(datos_.nombreProyecto);
}

void EditorConfig::cargarGeneral(const std::string& ruta) {
    // Una sola implementacion del esquema JSON: ConfigPersistence (y una sola
    // de rutas: ProjectPaths). Este metodo solo vuelca los campos de la config
    // general; los de proyecto no se tocan. Archivo ausente/corrupto/parcial ->
    // los defaults que trae ConfigPersistence::General.
    const ConfigPersistence::General g = ConfigPersistence::cargarGeneral(ruta);

    datos_.version = g.version;
    if (!g.ultimoProyecto.empty())
        datos_.nombreProyecto = g.ultimoProyecto;
    datos_.idioma = g.idioma;
    datos_.sensibilidadCamara = g.sensibilidadCamara;
    datos_.sensibilidadMovimientoCamara = g.sensibilidadMovimientoCamara;
    datos_.apariencia = g.apariencia;
}

void EditorConfig::cargarProyecto(const std::string& nombreProyecto,
                                  const std::string& ruta) {
    const ConfigPersistence::Proyecto p =
        ConfigPersistence::cargarProyecto(nombreProyecto, ruta);

    datos_.version = p.version;
    datos_.ventanaCamarasAbierta = p.ventanaCamarasAbierta;
    datos_.gizmoOperacion = p.gizmoOperacion;
    datos_.gizmoGlobal = p.gizmoGlobal;
    datos_.camaraActivaId = p.camaraActivaId;
    // Se asigna entero (y no se fusiona) para que entrar a un proyecto sin
    // configuracion arranque en defaults en lugar de heredar los del anterior.
    datos_.estadoVentanas = p.estadoVentanas;
}

// ============================================================================
// Guardado
// ============================================================================

void EditorConfig::guardar(const std::string& ruta) {
    // Guardado completo de compatibilidad: guarda general + proyecto.
    // En el flujo nuevo, main llama a guardarGeneral/guardarProyecto por separado.
    guardarGeneral(ruta);
    guardarProyecto(datos_.nombreProyecto);
}

void EditorConfig::guardarGeneral(const std::string& ruta) {
    ConfigPersistence::General g;
    g.version = datos_.version;
    // Ultimo proyecto abierto: al arrancar se retoma este proyecto sin pedir al
    // usuario. Con el nombre vacio (primer arranque sin proyecto elegido) la
    // clave no la escribe ConfigPersistence::guardarGeneral: de lo contrario al
    // releer quedaria "Nuevo Proyecto" y se crearian sus carpetas solas.
    g.ultimoProyecto = datos_.nombreProyecto;
    g.idioma = datos_.idioma;
    g.sensibilidadCamara = datos_.sensibilidadCamara;
    g.sensibilidadMovimientoCamara = datos_.sensibilidadMovimientoCamara;
    g.apariencia = datos_.apariencia;

    ConfigPersistence::guardarGeneral(g, ruta);

    // Fin del guardado diferido: hubo escritura (inmediata) ahora.
    generalPendiente = false;
    ultimaEscrituraGeneral = std::chrono::steady_clock::now();
}

void EditorConfig::guardarProyecto(const std::string& nombreProyecto,
                                   const std::string& ruta) {
    ConfigPersistence::Proyecto p;
    p.version = datos_.version;
    p.ventanaCamarasAbierta = datos_.ventanaCamarasAbierta;
    p.gizmoOperacion = datos_.gizmoOperacion;
    p.gizmoGlobal = datos_.gizmoGlobal;
    p.camaraActivaId = datos_.camaraActivaId;
    p.estadoVentanas = datos_.estadoVentanas;

    ConfigPersistence::guardarProyecto(p, nombreProyecto, ruta);
}

// ============================================================================
// Guardado diferido de la configuracion general (ver EditorConfig.h)
// ============================================================================

void EditorConfig::solicitarGuardadoGeneral(const std::string& ruta) {
    generalPendiente = true;
    rutaGeneralPendiente = ruta;
    volcarGuardadoGeneral();
}

void EditorConfig::volcarGuardadoGeneral() {
    if (!generalPendiente) return;
    const auto ahora = std::chrono::steady_clock::now();
    if (ahora - ultimaEscrituraGeneral < kIntervaloEscritura) return;
    guardarGeneral(rutaGeneralPendiente);
}
