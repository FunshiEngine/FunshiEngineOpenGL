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
#ifndef EDITORMATERIALGUI_H
#define EDITORMATERIALGUI_H

#include <string>

class Material;
class IPreviewMaterial;

// Editor de un material: preset, colores del modelo de iluminacion, brillo y
// los cuatro slots de textura.
//
// Vive aparte del panel del componente Material porque el panel de materiales
// por region reutiliza los mismos controles: asi el material de una region se
// edita exactamente igual que el material unico del objeto.
//
// Devuelve true si el material cambio (para que quien lo dibuje invalidate
// su vista previa). La vista previa (esfera de muestra) es opcional: sin ella
// el editor funciona igual, solo que sin la imagen.
bool dibujarEditorMaterial(Material& material, int& presetAplicado,
                            IPreviewMaterial* preview = nullptr,
                            const std::string& clave = std::string());

#endif // EDITORMATERIALGUI_H
