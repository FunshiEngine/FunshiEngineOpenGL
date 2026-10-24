## Comandos esenciales

```bash
# Configurar y compilar (CMake es el único build soportado; la solución de
# Visual Studio la genera CMake dentro del directorio de build, no se versiona)
cmake -S FunshiEngineGL -B FunshiEngineGL/build
cmake --build FunshiEngineGL/build -j$(nproc)

# Ejecutar la suite de tests (los targets headless registrados en CMakeLists;
# la CI lista los suyos en .github/workflows/ci.yml: agregar ahi cualquier
# target de test nuevo)
cd FunshiEngineGL/build && ctest --output-on-failure
```

- **`cmake` sin `-DCMAKE_BUILD_TYPE` fuerza Debug** (`CMakeLists.txt`), y en
  Debug se activan ASan+UBSan para compiladores GNU/Clang. Con el GCC de MSYS2
  eso **no linkea**: el toolchain no trae los runtimes de sanitizer. Para un
  build de trabajo normal y para igualar la CI:

  ```bash
  cmake -S FunshiEngineGL -B FunshiEngineGL/build \
      -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF -DFUNSHI_JAVA=ON
  ```

  Con el generador de Visual Studio el sanitizer no se activa
  (`CMAKE_CXX_COMPILER_ID` no es GNU/Clang), así que ahí el flag es inofensivo.

- **Build y tests como parte del cambio**: al tocar código fuente (o `CMakeLists.txt`
  y tests), revisar SIEMPRE si hay que actualizar el build y la suite: el engine
  usa `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`, pero los targets de test listan
  sus fuentes explícitamente; un `.cpp`/`.h` nuevo, un include, una dependencia,
  un `add_test` o el conteo de comprobaciones de un test existente pueden quedar
  fuera de sincronía. Ajustarlos en el mismo commit que el código que los motiva.
  
  
## Ejecutar el editor para prueba manual

El criterio de aceptación de un fix lo define el usuario probando la app real
(ver "NO confirmar fixes hasta validación del usuario"), así que hace falta
poder lanzarla:

```bash
cd FunshiEngineGL && ./build/FunshiEngineGL       # Linux
cd FunshiEngineGL && ./build/FunshiEngineGL.exe   # Windows
```

- Lanzarlo **desde `FunshiEngineGL/`**: los assets y las rutas relativas se
  resuelven contra el cwd.
- La salida (stdout y stderr) se redirige a un archivo por arranque en
  `logs/`, junto al ejecutable —`FunshiEngineGL_<AAAAMMDD_HHMMSS>.log`, con
  timestamp **UTC**—, así que el editor no abre consola. Es lo primero que hay
  que mirar cuando algo falla.
