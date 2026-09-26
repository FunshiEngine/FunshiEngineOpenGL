# Avisos de terceros (Third-Party Notices)

Este repositorio integra bibliotecas de terceros. **No están cubiertas por la
Apache License 2.0 del proyecto** (ver `LICENSE` y `NOTICE`): conservan sus
licencias originales, que se resumen a continuación junto a su ubicación.

| Componente | Ubicación | Licencia | Titular |
|---|---|---|---|
| Dear ImGui v1.92.2 | `FunshiEngineGL/src/ImGui/` | MIT | Copyright (c) 2014-2025 Omar Cornut |
| ImGuizmo | `FunshiEngineGL/ImGuizmo/` | MIT (incluye `LICENSE` propio) | Copyright (c) 2016 Cedric Guillemet |
| nlohmann/json v3.11.3 | `FunshiEngineGL/External/nlohmann/` | MIT (SPDX en `json.hpp`) | Copyright (c) 2013-2023 Niels Lohmann |
| stb_image v2.16 | `FunshiEngineGL/src/Herramientas/IconosGUI/stb_image.h` | Dominio público (dual MIT) | Sean Barrett |
| OpenGL Mathematics (GLM) | Dependencia del sistema, con copia de respaldo si se detecta en el sistema | MIT / The Happy Bunny License | OpenGL Mathematics |
| miniaudio | `FunshiEngineGL/src/Audio/miniaudio.h` | Dominio público (dual MIT-0 / MIT / CC0) | David Reid |

## Textos de licencia

### MIT (Dear ImGui, ImGuizmo, nlohmann/json, GLM)

> Permission is hereby granted, free of charge, to any person obtaining a copy
> of this software and associated documentation files (the "Software"), to deal
> in the Software without restriction, including without limitation the rights
> to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
> copies of the Software, and to permit persons to whom the Software is
> furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all
> copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
> IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
> FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
> AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
> LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
> OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
> SOFTWARE.

- **Dear ImGui**: aviso completo en la cabecera de `imgui.h` / `imgui.cpp`.
  Proyecto: https://github.com/ocornut/imgui
- **ImGuizmo**: aviso completo en `FunshiEngineGL/ImGuizmo/LICENSE`.
  Proyecto: https://github.com/CedricGuillemet/ImGuizmo
- **nlohmann/json**: cabecera *single-header* con `SPDX-License-Identifier: MIT` y
  `SPDX-FileCopyrightText: 2013-2023 Niels Lohmann`. Proyecto:
  https://github.com/nlohmann/json
- **GLM**: https://github.com/g-truc/glm

### stb_image

> stb_image - v2.16 - public domain image loader - http://nothings.org/stb_image.h
> no warranty implied; use at your own risk

Dominio público. El autor ofrece además la opción de usarlo bajo licencia MIT
si el dominio público no aplica en tu jurisdicción (ver cabecera del archivo).

### miniaudio

> This software is dual-licensed. You are free to choose which license you want
> to use. If you are unsure which license you want to use, it is recommended you
> use the MIT-0 license.

David Reid, 2016-2024. Ver cabecera de `miniaudio.h`: dominio público (CC0), o
MIT-0 / MIT a elección. En el instalador Linux, el aviso completo vive en
`data/licencia/third-party/miniaudio-copyright`.

## Nota

Las aportaciones al código propio de FunshiEngineGL se rigen por la Apache
License 2.0 (sección 5, "Submission of Contributions"). Si añades una nueva
biblioteca de terceros, documenta aquí su licencia, titular y ubicación.

Todos los archivos fuente propios (`.h`/`.cpp` bajo `FunshiEngineGL/src/` y
`tests/`) incluyen al comienzo el encabezado estándar Apache 2.0 con su
etiqueta `SPDX-License-Identifier: Apache-2.0`. Al crear archivos nuevos,
copia ese encabezado desde cualquier archivo existente. Los archivos de
terceros listados arriba conservan sus cabeceras originales sin modificar.

