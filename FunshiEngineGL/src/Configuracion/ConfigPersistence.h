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
#ifndef CONFIG_PERSISTENCE_H
#define CONFIG_PERSISTENCE_H

#include <map>
#include <string>

#include <nlohmann/json.hpp>

#include "ProjectPaths.h"
#include "Apariencia.h"

// Persistencia JSON pura: carga/guarda datos de configuración general y de proyecto.
// Sin estado runtime, sin lógica de migración, sin discovery de proyectos.
class ConfigPersistence {
public:
    struct General {
        int version = 2;
        std::string ultimoProyecto;      // vacío = primer arranque
        std::string idioma = "Espanol";
        float sensibilidadCamara = 0.15f;
        float sensibilidadMovimientoCamara = 1.0f;
        Apariencia apariencia;
    };

    struct Proyecto {
        int version = 2;
        bool ventanaCamarasAbierta = true;
        int gizmoOperacion = 7;
        bool gizmoGlobal = false;
        int camaraActivaId = -1;
        // estadoVentanas: WindowName -> abierto. Vive en el propio struct para
        // que guardarProyecto/cargarProyecto manejen el JSON completo: con la
        // serializacion en un metodo aparte, guardarProyecto reescribia el
        // archivo sin el mapa y se perdia el estado de los paneles.
        std::map<std::string, bool> estadoVentanas;
    };

    // Escribe JSON en disco, creando directorios padre. Escritura atomica
    // (temporal + rename): un corte a mitad de escritura no corrompe el archivo.
    static void escribirJson(const std::string& ruta, const nlohmann::json& j);

    // Lee JSON de disco; retorna object vacío si falla.
    static nlohmann::json leerJson(const std::string& ruta);

    // CONFIGURACIÓN GENERAL (MotorGrafico/Configuraciones/Configuracion.json)

    // Carga configuración general. Tolera archivo ausente/corrupto -> defaults.
    static General cargarGeneral(const std::string& ruta = "");

    // Guarda configuración general.
    static void guardarGeneral(const General& g, const std::string& ruta = "");

    // CONFIGURACIÓN DE PROYECTO (<Proyecto>/Memory/ConfiguracionProyecto.json)

    // Carga configuración de proyecto. Tolera archivo ausente/corrupto -> defaults.
    // Si `ruta` vacío, usa ruta canónica del proyecto.
    static Proyecto cargarProyecto(const std::string& nombreProyecto, const std::string& ruta = "");

    // Guarda configuración de proyecto (incluye el estado de ventanas: es el
    // único camino de escritura, para que no existan dos formas de persistir
    // el mismo archivo).
    static void guardarProyecto(const Proyecto& p, const std::string& nombreProyecto, const std::string& ruta = "");

private:
    // Helpers para Apariencia
    static nlohmann::json aparienciaToJson(const Apariencia& a);
    static Apariencia jsonToApariencia(const nlohmann::json& j);
};

#endif // CONFIG_PERSISTENCE_H