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
#include "TemaEditor.h"

namespace {

ImVec4 conAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

ImVec4 escalar(const ImVec4& c, float f) {
    return ImVec4(c.x * f, c.y * f, c.z * f, c.w);
}

ImVec4 aGris(const ImVec4& c) {
    const float v = AparienciaUtil::luminancia(c.x, c.y, c.z);
    return ImVec4(v, v, v, c.w);
}

// Mezcla lineal de dos colores conservando el alpha de `base` (t = 0 devuelve
// el RGB de `base`, t = 1 el de `con`). Es la herramienta con la que el acento
// tiñe los fondos que define el tema (campos, cabeceras, solapas) sin inventar
// colores nuevos: siempre se parte del color que ya tiene la paleta clara u
// oscura. Los llamadores que necesiten otro alpha lo envuelven en conAlpha.
ImVec4 mezclar(const ImVec4& base, const ImVec4& con, float t) {
    return ImVec4(base.x + (con.x - base.x) * t,
                  base.y + (con.y - base.y) * t,
                  base.z + (con.z - base.z) * t,
                  base.w);
}

// Cuanto acento entra en cada fondo derivado. Los valores estan elegidos para
// que con el acento azul historico el resultado quede practicamente igual al de
// fabrica (sin cambio visual) y con cualquier otro acento la interfaz se tiña
// sin perder legibilidad del texto.
constexpr float kTinteCampoOscuro = 0.45f;     // FrameBg del tema oscuro
constexpr float kTinteCabeceraOscuro = 0.22f;  // TableHeaderBg del tema oscuro
constexpr float kTinteCabeceraClaro = 0.30f;   // TableHeaderBg del tema claro

// Mezclas de las solapas del dock: son las MISMAS que usa Dear ImGui
// (Tab = Header -> TitleBgActive, TabDimmed = Tab -> TitleBg, etc.), pero
// aplicadas sobre los roles ya teñidos con el acento.
constexpr float kMezclaSolapaOscura = 0.80f;       // Tab (tema oscuro)
constexpr float kMezclaSolapaClara = 0.90f;        // Tab (tema claro)
constexpr float kMezclaSolapaAtenuada = 0.80f;     // TabDimmed
constexpr float kMezclaSolapaAtenuadaSel = 0.40f;  // TabDimmedSelected

} // namespace

ImVec4 TemaEditor::acento(const Apariencia& ap) {
    // El alpha del perfil NO participa: cada rol del estilo aporta su propia
    // transparencia (conAlpha) y los roles de primer plano (checks, grabs,
    // enlaces, cursor) quedan opacos. Se normaliza aca para que un acento
    // translucido guardado en la configuracion no pueda apagar la interfaz.
    const ImVec4 c(ap.acento[0], ap.acento[1], ap.acento[2], 1.0f);
    return ap.blancoYNegro ? aGris(c) : c;
}

void TemaEditor::aplicarEstilo(const Apariencia& ap) {
    // Base clara u oscura de fabrica; luego se pisan los roles de acento.
    if (ap.temaClaro) {
        ImGui::StyleColorsLight();
    } else {
        ImGui::StyleColorsDark();
    }

    ImGuiStyle& s = ImGui::GetStyle();

    // Geometria coherente para toda la interfaz (bordes suaves y aire).
    s.WindowRounding = 6.0f;
    s.ChildRounding = 4.0f;
    s.FrameRounding = 4.0f;
    s.PopupRounding = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.GrabRounding = 4.0f;
    s.TabRounding = 4.0f;
    s.WindowBorderSize = 1.0f;
    s.ChildBorderSize = 1.0f;
    s.FrameBorderSize = 0.0f;
    s.WindowPadding = ImVec2(10.0f, 10.0f);
    s.FramePadding = ImVec2(8.0f, 5.0f);
    s.ItemSpacing = ImVec2(8.0f, 6.0f);
    s.ScrollbarSize = 12.0f;
    s.GrabMinSize = 10.0f;

    const ImVec4 ac = acento(ap);
    const ImVec4 acOscuro = escalar(ac, 0.75f);
    const ImVec4 acClaro = escalar(ac, 1.20f);

    // Alphas de fabrica de los roles que mas abajo se derivan del acento: se
    // leen ANTES de pisarlos para conservar la transparencia historica de cada
    // rol sin hardcodear valores distintos por tema.
    const float alfaCampo = s.Colors[ImGuiCol_FrameBg].w;
    const float alfaCabeceraTabla = s.Colors[ImGuiCol_TableHeaderBg].w;
    const float alfaSolapa = s.Colors[ImGuiCol_Tab].w;
    const float alfaSolapaAtenuada = s.Colors[ImGuiCol_TabDimmed].w;
    const float alfaSolapaAtenuadaSel = s.Colors[ImGuiCol_TabDimmedSelected].w;

    // Roles de acento. Se usan con transparencia donde ImGui espera fondos.
    s.Colors[ImGuiCol_Button] = conAlpha(ac, 0.55f);
    s.Colors[ImGuiCol_ButtonHovered] = ac;
    s.Colors[ImGuiCol_ButtonActive] = acOscuro;
    s.Colors[ImGuiCol_Header] = conAlpha(ac, 0.45f);
    s.Colors[ImGuiCol_HeaderHovered] = conAlpha(ac, 0.80f);
    s.Colors[ImGuiCol_HeaderActive] = ac;
    s.Colors[ImGuiCol_CheckMark] = ac;
    s.Colors[ImGuiCol_SliderGrab] = acOscuro;
    s.Colors[ImGuiCol_SliderGrabActive] = acClaro;
    s.Colors[ImGuiCol_TitleBgActive] = acOscuro;
    s.Colors[ImGuiCol_TabHovered] = conAlpha(ac, 0.80f);
    s.Colors[ImGuiCol_TabSelected] = conAlpha(ac, 0.65f);
    s.Colors[ImGuiCol_TabSelectedOverline] = ac;
    s.Colors[ImGuiCol_SeparatorHovered] = conAlpha(ac, 0.78f);
    s.Colors[ImGuiCol_SeparatorActive] = ac;
    s.Colors[ImGuiCol_ResizeGrip] = conAlpha(ac, 0.25f);
    s.Colors[ImGuiCol_ResizeGripHovered] = conAlpha(ac, 0.67f);
    s.Colors[ImGuiCol_ResizeGripActive] = conAlpha(ac, 0.95f);
    s.Colors[ImGuiCol_TextSelectedBg] = conAlpha(ac, 0.35f);
    s.Colors[ImGuiCol_FrameBgHovered] = conAlpha(ac, 0.40f);
    s.Colors[ImGuiCol_FrameBgActive] = conAlpha(ac, 0.67f);
    s.Colors[ImGuiCol_DockingPreview] = conAlpha(ac, 0.70f);
    s.Colors[ImGuiCol_NavCursor] = ac;

    // --- Resto de roles que en la paleta de fabrica llevan el azul de ImGui --
    // Dear ImGui tiñe de azul (0.26/0.59/0.98 y derivados) los fondos de campo,
    // las solapas del dock, los bordes, las cabeceras de tabla, los enlaces de
    // texto y el destino de arrastre. Si no se derivan del acento, la interfaz
    // queda medio azul con cualquier otro color elegido en Opciones: se derivan
    // aca para que NINGUN rol conserve el azul clasico.
    const ImVec4 fondo = s.Colors[ImGuiCol_WindowBg];

    // Campos de entrada, combos y pista del slider (FrameBg). En el tema oscuro
    // el valor de fabrica es azul (0.16, 0.29, 0.48): se mezcla el fondo del
    // tema con el acento, asi con el azul por defecto el resultado es casi
    // identico al historico y con otro acento el campo lo acompaña. En el tema
    // claro el campo ya es neutro (blanco) y se conserva tal cual.
    if (!ap.temaClaro) {
        s.Colors[ImGuiCol_FrameBg] =
            conAlpha(mezclar(fondo, ac, kTinteCampoOscuro), alfaCampo);
    }

    // Cabeceras y bordes de tabla: la cabecera se tiñe con el acento (es el
    // fondo con el que Dear ImGui marca la tabla) y los bordes pasan a gris
    // neutro para perder el sesgo azulado del gris de fabrica.
    s.Colors[ImGuiCol_TableHeaderBg] = conAlpha(
        mezclar(fondo, ac,
                ap.temaClaro ? kTinteCabeceraClaro : kTinteCabeceraOscuro),
        alfaCabeceraTabla);
    s.Colors[ImGuiCol_TableBorderStrong] =
        aGris(s.Colors[ImGuiCol_TableBorderStrong]);
    s.Colors[ImGuiCol_TableBorderLight] =
        aGris(s.Colors[ImGuiCol_TableBorderLight]);

    // Bordes, separadores y lineas del arbol: el gris de fabrica (0.43, 0.43,
    // 0.50) tira a azul; se pasa a su equivalente neutro manteniendo el brillo
    // y la transparencia, que es lo que usan ventanas, paneles y la propia
    // vista Opciones.
    s.Colors[ImGuiCol_Border] = aGris(s.Colors[ImGuiCol_Border]);
    s.Colors[ImGuiCol_Separator] = s.Colors[ImGuiCol_Border];
    s.Colors[ImGuiCol_TreeLines] = s.Colors[ImGuiCol_Border];

    // Enlaces de texto y destino del arrastre: roles de acento puros.
    s.Colors[ImGuiCol_TextLink] = ac;
    s.Colors[ImGuiCol_DragDropTarget] = conAlpha(ac, 0.95f);

    // Solapas del dock: se recalculan con las mismas formulas de mezcla que usa
    // Dear ImGui (ver kMezclaSolapa*), pero sobre los roles ya teñidos, asi la
    // barra de pestañas acompaña al acento tanto en foco como sin foco
    // (TabDimmed es el que ImGui usa en los nodos de dock sin foco).
    const ImVec4 tituloActivo = s.Colors[ImGuiCol_TitleBgActive];
    s.Colors[ImGuiCol_Tab] =
        conAlpha(mezclar(s.Colors[ImGuiCol_Header], tituloActivo,
                         ap.temaClaro ? kMezclaSolapaClara
                                      : kMezclaSolapaOscura),
                 alfaSolapa);
    s.Colors[ImGuiCol_TabDimmed] =
        conAlpha(mezclar(s.Colors[ImGuiCol_Tab], s.Colors[ImGuiCol_TitleBg],
                         kMezclaSolapaAtenuada),
                 alfaSolapaAtenuada);
    s.Colors[ImGuiCol_TabDimmedSelected] =
        conAlpha(mezclar(s.Colors[ImGuiCol_TabSelected],
                         s.Colors[ImGuiCol_TitleBg],
                         kMezclaSolapaAtenuadaSel),
                 alfaSolapaAtenuadaSel);
    // La linea superior de la solapa atenuada nunca se dibuja en ImGui: se deja
    // con el acento y alpha 0 para que tampoco guarde el azul de fabrica.
    s.Colors[ImGuiCol_TabDimmedSelectedOverline] = conAlpha(ac, 0.0f);

    // Modo blanco y negro: se desatura TODA la paleta (incluido el acento ya
    // aplicado) preservando las transparencias. El resultado es una interfaz
    // monocroma coherente con el fondo y la grilla del viewport.
    if (ap.blancoYNegro) {
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            s.Colors[i] = aGris(s.Colors[i]);
        }
    }
}
