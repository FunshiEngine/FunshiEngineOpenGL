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
#ifndef TEXTOVALOR_H
#define TEXTOVALOR_H

#include <string>

namespace MotorInspeccion {

// Tipos de dato que la API de inspeccion sabe leer y escribir. El nombre
// textual de cada variable expuesta por la API es el que se documenta en el
// manual: la CLI (proyecto aparte) los usa tal cual.
enum class TipoValor : unsigned char {
    Entero,
    Flotante,
    Booleano,
    Texto,
    Vec2,
    Vec3,
    Vec4,
    Enumeracion
};

// Nombre legible del tipo, para las vistas y para los mensajes de error.
const char* nombreDeTipoValor(TipoValor tipo);

// --- Valor -> texto -------------------------------------------------------

std::string textoEntero(int valor);

// El flotante se imprime con la representacion mas corta que vuelve a leerse
// como el mismo valor: "0.1" y no "0.100000001". Los valores no finitos se
// imprimen como "inf", "-inf" y "nan".
std::string textoFlotante(float valor);

std::string textoBooleano(bool valor);

// Los vectores se separan por espacios: "1 2 3".
std::string textoVec2(float x, float y);
std::string textoVec3(float x, float y, float z);
std::string textoVec4(float x, float y, float z, float w);

// --- Texto -> valor -------------------------------------------------------
//
// Devuelven false y dejan el motivo en `error` cuando el texto no se puede
// interpretar. En ese caso no escriben en las variables de salida.

bool leerEntero(const std::string& texto, int& valor, std::string& error);
bool leerFlotante(const std::string& texto, float& valor, std::string& error);
bool leerBooleano(const std::string& texto, bool& valor, std::string& error);

// Aceptan los numeros separados por espacios o por comas.
bool leerVec2(const std::string& texto, float& x, float& y, std::string& error);
bool leerVec3(const std::string& texto, float& x, float& y, float& z,
              std::string& error);
bool leerVec4(const std::string& texto, float& x, float& y, float& z, float& w,
              std::string& error);

}  // namespace MotorInspeccion

#endif
