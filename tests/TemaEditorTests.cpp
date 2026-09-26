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
// Pruebas headless del TemaEditor (aplicacion del perfil Apariencia al estilo
// de ImGui). El estilo solo necesita el contexto de ImGui, sin backends ni pila
// grafica, asi que el test corre en cualquier plataforma como los demas.
//
// Cubre la regresion del bug "el color de acento no llega a toda la interfaz":
// Dear ImGui trae roles azulados de fabrica (FrameBg, Tab, TabDimmedSelected,
// Border, TableHeaderBg, TextLink, DragDropTarget...) que el tema dejaba
// intactos. Al elegir otro acento en Opciones se repintaba solo una parte de la
// UI (pista del slider y campos azules, barra de pestañas del dock azul,
// separadores azulados) y el resto quedaba del azul clasico para siempre.

#include <imgui.h>

#include <cmath>
#include <iostream>

#include "../FunshiEngineGL/src/Configuracion/Apariencia.h"
#include "../FunshiEngineGL/src/GUI/Tema/TemaEditor.h"

namespace {
int total = 0;
int fallos = 0;

#define CHECK(cond, msg)                                                      \
    do {                                                                      \
        ++total;                                                              \
        if (!(cond)) {                                                        \
            ++fallos;                                                         \
            std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")"    \
                      << std::endl;                                           \
        }                                                                     \
    } while (0)

// El color tiene el "clasico azul" de Dear ImGui: canal B dominante y visible.
bool azulado(const ImVec4& c) {
    return c.w > 0.0f && c.z > c.x + 0.05f && c.z > c.y + 0.05f;
}

bool cerca(float a, float b, float tol = 1e-5f) { return std::fabs(a - b) <= tol; }

// Roles que en la paleta de fabrica son azulados y, tras aplicar el perfil,
// siguen azulados. Con un acento no azul debe dar 0: cualquier rol listado es
// un resto del azul de Dear ImGui que la interfaz muestra para siempre.
int azulesSobrevivientes(const ImGuiStyle& fabrica) {
    const ImGuiStyle& s = ImGui::GetStyle();
    int n = 0;
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        if (!azulado(fabrica.Colors[i])) continue;
        if (!azulado(s.Colors[i])) continue;
        ++n;
        std::cout << "  rol con azul de fabrica sin recolorear: "
                  << ImGui::GetStyleColorName(i) << std::endl;
    }
    return n;
}

// Roles que no quedaron en escala de grises (para validar el modo B/N).
int rolesConColor() {
    const ImGuiStyle& s = ImGui::GetStyle();
    int n = 0;
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        const ImVec4& c = s.Colors[i];
        if (!cerca(c.x, c.y) || !cerca(c.y, c.z)) {
            ++n;
            std::cout << "  rol con color en modo B/N: "
                      << ImGui::GetStyleColorName(i) << std::endl;
        }
    }
    return n;
}

Apariencia perfil(bool claro, bool blancoYNegro, float r, float g, float b) {
    Apariencia ap;
    ap.temaClaro = claro;
    ap.blancoYNegro = blancoYNegro;
    ap.acento[0] = r;
    ap.acento[1] = g;
    ap.acento[2] = b;
    ap.acento[3] = 1.0f;
    return ap;
}

// Acento verde: cualquier resto azul de fabrica salta a la vista.
const Apariencia kVerdeOscuro = perfil(false, false, 0.10f, 0.80f, 0.20f);

} // namespace

int main() {
    ImGui::CreateContext();

    ImGuiStyle fabricaOscura;
    ImGui::StyleColorsDark(&fabricaOscura);
    ImGuiStyle fabricaClara;
    ImGui::StyleColorsLight(&fabricaClara);

    // 1. Acento no azul en tema oscuro: el acento llega a los roles que antes
    // quedaban azules (fondo de campo / pista del slider, solapas, enlaces) y
    // los grises azulados pasan a neutro.
    {
        TemaEditor::aplicarEstilo(kVerdeOscuro);
        const ImGuiStyle& s = ImGui::GetStyle();

        CHECK(azulesSobrevivientes(fabricaOscura) == 0,
              "tema oscuro con acento verde: ningun rol conserva el azul de fabrica");

        CHECK(s.Colors[ImGuiCol_FrameBg].y > s.Colors[ImGuiCol_FrameBg].z,
              "FrameBg (campos y pista del slider) sigue al acento");
        CHECK(s.Colors[ImGuiCol_Tab].y > s.Colors[ImGuiCol_Tab].z,
              "Tab (barra de pestañas del dock) sigue al acento");
        CHECK(s.Colors[ImGuiCol_TabDimmed].y > s.Colors[ImGuiCol_TabDimmed].z,
              "TabDimmed (dock sin foco) sigue al acento");
        CHECK(s.Colors[ImGuiCol_TextLink].y > s.Colors[ImGuiCol_TextLink].z,
              "TextLink sigue al acento");
        CHECK(cerca(s.Colors[ImGuiCol_Separator].x, s.Colors[ImGuiCol_Separator].z),
              "Separator queda gris neutro, sin el sesgo azulado de fabrica");
        CHECK(cerca(s.Colors[ImGuiCol_Border].x, s.Colors[ImGuiCol_Border].z),
              "Border queda gris neutro");
    }

    // 2. Acento no azul en tema claro: mismo contrato (la paleta clara tambien
    // trae azules de fabrica en la tabla y los enlaces).
    {
        TemaEditor::aplicarEstilo(perfil(true, false, 0.85f, 0.25f, 0.20f));
        const ImGuiStyle& s = ImGui::GetStyle();

        CHECK(azulesSobrevivientes(fabricaClara) == 0,
              "tema claro con acento rojo: ningun rol conserva el azul de fabrica");
        CHECK(s.Colors[ImGuiCol_TableHeaderBg].x > s.Colors[ImGuiCol_TableHeaderBg].z,
              "TableHeaderBg sigue al acento en tema claro");
        CHECK(s.Colors[ImGuiCol_DragDropTarget].x > s.Colors[ImGuiCol_DragDropTarget].z,
              "DragDropTarget sigue al acento en tema claro");
    }

    // 3. Con el acento por defecto (azul de Dear ImGui) el tema no cambia de
    // identidad ni la transparencia historica de los roles derivados.
    {
        TemaEditor::aplicarEstilo(Apariencia{});
        const ImGuiStyle& s = ImGui::GetStyle();
        const ImVec4 f = fabricaOscura.Colors[ImGuiCol_FrameBg];

        CHECK(cerca(s.Colors[ImGuiCol_FrameBg].x, f.x, 0.10f) &&
                  cerca(s.Colors[ImGuiCol_FrameBg].y, f.y, 0.10f) &&
                  cerca(s.Colors[ImGuiCol_FrameBg].z, f.z, 0.10f),
              "con el acento por defecto el fondo del campo queda casi identico al historico");
        CHECK(cerca(s.Colors[ImGuiCol_FrameBg].w, f.w),
              "el fondo del campo conserva la transparencia de fabrica");
        CHECK(cerca(s.Colors[ImGuiCol_Tab].w, fabricaOscura.Colors[ImGuiCol_Tab].w),
              "la solapa conserva la transparencia de fabrica");
        CHECK(cerca(s.Colors[ImGuiCol_TabDimmed].w,
                    fabricaOscura.Colors[ImGuiCol_TabDimmed].w),
              "la solapa atenuada conserva la transparencia de fabrica");
        CHECK(azulado(s.Colors[ImGuiCol_FrameBg]) && azulado(s.Colors[ImGuiCol_Tab]),
              "el perfil por defecto mantiene la identidad azul del editor");
    }

    // 4. Idempotencia (la promete el header): aplicar dos veces el mismo perfil
    // deja el mismo estilo, porque la base se rearma en cada llamada.
    {
        TemaEditor::aplicarEstilo(kVerdeOscuro);
        const ImGuiStyle unaVez = ImGui::GetStyle();
        TemaEditor::aplicarEstilo(kVerdeOscuro);
        const ImGuiStyle& s = ImGui::GetStyle();

        int distintos = 0;
        for (int i = 0; i < ImGuiCol_COUNT; ++i) {
            if (!cerca(s.Colors[i].x, unaVez.Colors[i].x) ||
                !cerca(s.Colors[i].y, unaVez.Colors[i].y) ||
                !cerca(s.Colors[i].z, unaVez.Colors[i].z) ||
                !cerca(s.Colors[i].w, unaVez.Colors[i].w))
                ++distintos;
        }
        CHECK(distintos == 0, "aplicar el mismo perfil dos veces no acumula cambios");
    }

    // 5. Modo blanco y negro: la paleta completa (incluido el acento ya
    // aplicado) queda monocroma, asi que tampoco sobrevive azul alguno.
    {
        const Apariencia bn = perfil(false, true, 0.85f, 0.25f, 0.20f);
        TemaEditor::aplicarEstilo(bn);
        CHECK(rolesConColor() == 0,
              "modo B/N: todos los roles quedan en escala de grises con el acento incluido");

        const ImVec4 a = TemaEditor::acento(bn);
        CHECK(cerca(a.x, a.y) && cerca(a.y, a.z),
              "acento() devuelve el acento en gris cuando el modo B/N esta activo");
    }

    // 6. "Restablecer apariencia": el perfil vuelve al de fabrica y con el el
    // azul por defecto del editor.
    {
        Apariencia ap = perfil(true, true, 0.10f, 0.90f, 0.30f);
        ap.restablecer();
        TemaEditor::aplicarEstilo(ap);
        const ImGuiStyle& s = ImGui::GetStyle();
        CHECK(s.Colors[ImGuiCol_FrameBg].z > s.Colors[ImGuiCol_FrameBg].x,
              "tras restablecer, el acento vuelve al azul por defecto");
        CHECK(!ap.temaClaro && !ap.blancoYNegro,
              "restablecer apaga el tema claro y el modo B/N");
    }

    // 7. El alpha del acento no participa del tema: un acento translucido
    // (config guardada antes, o elegido con la barra de alpha que ya no se
    // expone en Opciones) no puede dejar apagados los roles de primer plano.
    {
        Apariencia ap = perfil(false, false, 0.10f, 0.80f, 0.20f);
        ap.acento[3] = 0.0f; // caso extremo: totalmente transparente
        TemaEditor::aplicarEstilo(ap);
        const ImGuiStyle& s = ImGui::GetStyle();

        CHECK(s.Colors[ImGuiCol_CheckMark].w == 1.0f,
              "con acento transparente el check sigue opaco");
        CHECK(s.Colors[ImGuiCol_SliderGrab].w == 1.0f,
              "el grab del slider sigue opaco");
        CHECK(s.Colors[ImGuiCol_NavCursor].w == 1.0f,
              "el cursor de navegacion sigue opaco");
        CHECK(s.Colors[ImGuiCol_TextLink].w == 1.0f,
              "los enlaces de texto siguen opacos");
        CHECK(TemaEditor::acento(ap).w == 1.0f,
              "acento() devuelve el acento opaco");
        CHECK(s.Colors[ImGuiCol_Button].w == 0.55f &&
                  s.Colors[ImGuiCol_Header].w == 0.45f,
              "los fondos conservan su alpha propio (no el del perfil)");
        CHECK(s.Colors[ImGuiCol_FrameBg].w ==
                  fabricaOscura.Colors[ImGuiCol_FrameBg].w,
              "el fondo del campo conserva la transparencia de fabrica");
        CHECK(azulesSobrevivientes(fabricaOscura) == 0,
              "el acento transparente tampoco deja restos azules de fabrica");
    }

    ImGui::DestroyContext();

    std::cout << (fallos == 0 ? "OK: " : "FALLOS: ") << (total - fallos) << "/"
              << total << " comprobaciones" << std::endl;
    return fallos == 0 ? 0 : 1;
}
