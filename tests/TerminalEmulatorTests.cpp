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
#include "../FunshiEngineGL/src/Terminal/TerminalEmulator.h"

#include <iostream>

namespace {
int total = 0;
int fallos = 0;
#define CHECK(cond, msg) do { ++total; if (!(cond)) { ++fallos; std::cout << "FALLO: " << msg << " (linea " << __LINE__ << ")\n"; } } while (0)
}

int main() {
    TerminalEmulator terminal;
    terminal.redimensionar(8, 4);
    terminal.procesar("abc\rZ\nQ");
    CHECK(terminal.celdas()[0].texto == "Z", "CR posiciona cursor al inicio de fila");
    CHECK(terminal.celdas()[9].texto == "Q", "LF avanza una fila y conserva la columna");
    terminal.procesar("\x1b[2;3Hxy");
    CHECK(terminal.celdas()[10].texto == "x", "H ubica cursor con coordenadas 1-based");
    CHECK(terminal.celdas()[11].texto == "y", "texto contiguo ocupa celdas sucesivas");
    terminal.procesar("\x1b[31;1mR");
    CHECK(terminal.celdas()[12].primerPlano == 1, "SGR aplica color frontal");
    CHECK(terminal.celdas()[12].negrita, "SGR aplica negrita");
    CHECK(terminal.celdas()[10].texto == "x", "pantalla principal mantiene celdas escritas");
    terminal.procesar("\x1b[40mB\x1b[6n");
    CHECK(terminal.celdas()[13].fondoDefinido, "SGR permite fondo negro explicito");
    CHECK(terminal.tomarRespuestas() == "\x1b[2;7R", "DSR devuelve la posicion del cursor");
    terminal.procesar("\x1b[2K");
    CHECK(terminal.celdas()[8].texto == " ", "K limpia la fila actual");
    terminal.procesar("\x1b[?1049hALT");
    CHECK(terminal.enPantallaAlternativa(), "1049h activa pantalla alternativa");
    terminal.procesar("\x1b[?1049l");
    CHECK(!terminal.enPantallaAlternativa(), "1049l restaura pantalla principal");
    CHECK(terminal.celdas()[10].texto == " ", "borrado de fila se conserva al restaurar pantalla");
    terminal.procesar("\x1b[?25l");
    CHECK(!terminal.cursorVisible(), "25l oculta cursor");
    terminal.procesar("\x1b[?25h");
    CHECK(terminal.cursorVisible(), "25h muestra cursor");
    terminal.procesar("\x1b[");
    terminal.procesar("6n");
    CHECK(terminal.cursorY() < terminal.filas(), "secuencia incompleta persiste entre bloques");
    terminal.procesar("ñ");
    CHECK(terminal.celdas()[terminal.cursorY() * terminal.columnas() + terminal.cursorX() - 1].texto == "ñ", "UTF-8 se conserva en una celda");
    terminal.redimensionar(10, 5);
    CHECK(terminal.columnas() == 10 && terminal.filas() == 5, "redimensionado cambia la cuadrilla");
    TerminalEmulator ajuste;
    ajuste.redimensionar(2, 2);
    ajuste.procesar("abñ");
    CHECK(ajuste.celdas()[2].texto == "ñ", "UTF-8 ajusta línea al llegar al borde");

    // CSI 1K (borrar desde el inicio hasta el cursor) con el cursor en wrap
    // pendiente de la ultima fila: cursorX_ vale columnas_ (la posicion se
    // paso de borde y el wrap se difiere), de modo que el extremo derecho es
    // min(cursor, columnas-1); sin esa cota el indice llega al tamaño de la
    // rejilla y sale del vector.
    TerminalEmulator wrap;
    wrap.redimensionar(4, 2);
    wrap.procesar("\x1b[2;1Habcd");
    CHECK(wrap.cursorX() == wrap.columnas(),
          "escribir la ultima columna deja el cursor en wrap pendiente");
    wrap.procesar("\x1b[1K");
    CHECK(wrap.celdas()[7].texto == " ",
          "CSI 1K con wrap pendiente limpia la ultima celda sin salir de la rejilla");
    CHECK(wrap.celdas()[4].texto == " ",
          "CSI 1K con wrap pendiente limpia la fila completa");

    // Resize con pantalla alternativa activa: al salir (1049l) se restaura el
    // cursor de la pantalla principal y debe quedar acotado a la rejilla nueva;
    // un CSI que use ese cursor no debe salir del vector.
    TerminalEmulator alt;
    alt.redimensionar(80, 24);
    alt.procesar("\x1b[21;71H");
    alt.procesar("\x1b[?1049h");
    alt.procesar("\x1b[2;1HX");
    alt.redimensionar(40, 10);
    alt.procesar("\x1b[?1049l");
    CHECK(alt.cursorX() == alt.columnas() - 1 && alt.cursorY() == alt.filas() - 1,
          "restaurar la pantalla principal tras redimensionar acota el cursor");
    alt.procesar("\x1b[1J");
    CHECK(alt.cursorX() < alt.columnas() && alt.cursorY() < alt.filas(),
          "CSI 1J tras la restauracion mantiene el cursor dentro de la rejilla");

    std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << total << " comprobaciones\n";
    return fallos == 0 ? 0 : 1;
}
