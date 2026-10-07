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
#ifndef FILESELECTION_H
#define FILESELECTION_H

#include <string>

class Carpeta;

// Estado de navegacion compartido entre los dos paneles del explorador de
// archivos (arbol + contenido). Es la fuente de verdad unica de la seleccion:
//  - carpetaActual: carpeta visible en el panel de contenido. El arbol la
//    escribe; el panel de contenido simplemente la lee cada frame.
//  - rutaVisible: ruta completa de carpetaActual, usada para re-resolver el
//    puntero cuando el arbol se reconstruye por un rescaneo (los punteros al
//    arbol viejo quedarian colgando).
//  - navegacionPendiente: ruta a abrir (doble clic en el contenido). El arbol
//    la consume al inicio de su frame (aplicacion diferida: se registra
//    durante el frame y se aplica contra el arbol del frame siguiente)
//    y la borra tras resolverla contra el arbol vigente.
//  - contadorCambios: sube solo cuando una operacion de Filesystem puede
//    modificar el arbol (crear/copiar/eliminar CARPETAS). El arbol lo compara
//    con su vista previa y se rescancea al detectar un cambio. Las operaciones
//    que solo alteran archivos (que no aparecen en el arbol) NO lo incrementan
//    para no colapsar el arbol de carpetas de forma gratuita.
struct FileSelection {
    Carpeta* carpetaActual = nullptr;
    std::string rutaVisible;
    std::string navegacionPendiente;
    unsigned long contadorCambios = 0;
};

#endif