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
// Preambulo compartido por las unidades de ContentFolderInterface.cpp (generado al dividirlo).
#ifndef CONTENTFOLDERINTERFACEDETALLE_H
#define CONTENTFOLDERINTERFACEDETALLE_H

// La E/S de plataforma (dialogos nativos, xdg-open/ShellExecute, listado de
// directorio) vive en FileManager; esta vista solo conversa con la fachada.
#include "ContentFolderInterface.h"
#include "../Tema/TemaEditor.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include "../../Herramientas/PathUtils.h"
#include "../../Events/EditorEventBus.h"
#include "../../FileManager/FileManager.h"
#include "../../FileManager/FileSelection.h"
#include "../../FileManager/Drop/IServicioDrop.h"
#include "../../FileManager/Portapapeles/IServicioPortapapeles.h"
#include "../Estado/StatusBarInterface.h"
#include "../WindowNames.h"
#include "SoltarEnCarpeta.h"
#include "CrearCarpeta.h"
#include "ImportarArchivos.h"
#include "../../Herramientas/IconosGUI/IconosGUI.h"
#include <imgui.h>

namespace fs = std::filesystem;

#endif
