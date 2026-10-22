# FunshiEngineGL

Motor y editor de juegos en tiempo real escrito en C++17, con interfaz Dear ImGui, renderizado OpenGL 3.3 Core y arquitectura modular (backends intercambiables).

**Licencia:** [Apache License 2.0](LICENSE).

## Compilación

```bash
cmake -S FunshiEngineGL -B FunshiEngineGL/build \
    -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF -DFUNSHI_JAVA=ON
cmake --build FunshiEngineGL/build -j$(nproc)
```

## Ejecución

```bash
cd FunshiEngineGL && ./build/FunshiEngineGL
```

## Documentación

- [doc/MANUAL_DE_USO.md](doc/MANUAL_DE_USO.md) – Guía de uso del editor y scripting
- [doc/PROJECT_STRUCTURE.md](doc/PROJECT_STRUCTURE.md) – Arquitectura y estructura del proyecto
- [doc/FLUJO_DE_RAMAS.md](doc/FLUJO_DE_RAMAS.md) – Convención de ramas
- [doc/DocuTecnicoBugs.md](doc/DocuTecnicoBugs.md) – Registro técnico de bugs
