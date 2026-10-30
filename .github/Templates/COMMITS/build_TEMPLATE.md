# Template de commit — build

## Formato

```
build(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
build(CMakeLists): corregir ruta de tests a ../tests/ → .local/tests/
```

## Cuerpo del mensaje

Describe cambios en CMakeLists.txt, empaquetado o dependencias.

## Comandos de build (ver build.md y repo.md)

- Configurar: `cmake -S FunshiEngineGL -B FunshiEngineGL/build [-DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF]`
- Compilar: `cmake --build FunshiEngineGL/build -j$(nproc)`
- Tests: `cd FunshiEngineGL/build && ctest --output-on-failure`
- Ejecutar editor: `cd FunshiEngineGL && ./build/FunshiEngineGL` (Linux) o `./build/FunshiEngineGL.exe` (Windows)

## Flags importantes

- `CMAKE_BUILD_TYPE` sin valor → Debug (activa ASan+UBSan en GNU/Clang)
- Trabajo diario: `Release -DENABLE_ASAN=OFF`
- MSYS2 GCC no linkea sanitizers: usar `-DENABLE_ASAN=OFF`
- Logs en `logs/` con timestamp UTC

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Ver `doc/FLUJO_DE_RAMAS.md`
