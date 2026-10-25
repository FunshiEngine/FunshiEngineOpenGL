## Pruebas

- **Autoría de un test headless**: seguir el patrón de
  `tests/ModelSerializationTests.cpp` — macro `CHECK(cond, msg)` definida en el
  propio archivo, `TempPruebas::CarpetaPrueba` para una carpeta temporal que se
  limpia sola al salir (RAII), y cierre con
  `std::cout << (fallos == 0 ? "OK" : "FALLOS") << ": " << ... << " comprobaciones"`,
  saliendo con 0 o 1.
- **Código de salida 77 = *skipped*, no fallo**. Lo esperado: para
  `scripts-runtime-tests` cuando el toolchain es MSVC (necesita `cl.exe` con el
  entorno de Visual Studio; con GCC/MinGW corre en cualquier SO) y para
  `scripts-java-tests` si no hay JDK resoluble (`JAVAC`/`JAVA_HOME`/PATH).
  `ctest` los reporta como *skipped* aparte; no son tests rotos. El skip es por
  **familia de toolchain**, nunca por sistema operativo (H-14).
- **GLSL embebido**: los shaders van dentro del binario
  (`src/Rendering/Shaders/ShaderSources.h`), así que un error de sintaxis no lo
  ve el compilador de C++: solo aparece cuando el driver compila el programa,
  con la app ya abierta. El pipeline lo comprueba con `glslangValidator`
  (`.github/scripts/validar-glsl.py`, job de Ubuntu). Si se toca
  `ShaderSources.h`, ese script tiene que dar verde: `python
  .github/scripts/validar-glsl.py` (sale con 77 si no hay validador).

## Depuración de bugs: instrumentación y validación

Cuando un bug no se resuelve leyendo el código, se depura con instrumentación y
evidencia: reproducirlo de forma **determinista**, aislar la causa cambiando
**una sola variable** por corrida y comprobar que el fix lo cierra en **más de
una circunstancia** (distintos estados, órdenes de pasos y plataformas). Esta
sección es el "cómo" de la evidencia que pide el análisis de bugs: la cadena
`síntoma → evidencia → causa → fix → test` se sostiene con estas técnicas, no
con suposiciones.

- **Bajar el fallo a un test antes que a una corrida manual**: si el síntoma se
  observa sin UI (lógica, rutas, serialización, ciclo de compilación/carga de
  scripts), se escribe un test headless (ver "Pruebas") y se exige **rojo antes
  y verde después**: un test que pasa con el bug y con el fix no prueba nada.
  El test queda además como regresión permanente de esa causa.
- **Sondas y laboratorios en directorios locales ignorados por git**: los
  artefactos desechables de una depuración —sondas C++ que compilan y corren
  sueltos (`FunshiEngineGL/sondas/*.cpp`), scripts de observación, volcados de
  log, copias locales de proyecto/escena usadas como laboratorio de
  experimentación— se viven en carpetas registradas en `.gitignore`
  (`FunshiEngineGL/sondas/`, `vgcore.*`). Producen la evidencia del análisis y
  se quedan en local: no son código del motor ni del juego, no entran en
  commits y se retiran cuando el análisis cierra.
- **Experimentos deterministas**: para que dos corridas sean comparables hay que
  fijar todo lo que no es la variable bajo prueba — mismo proyecto/escena y
  estado inicial, mismo cwd (las rutas relativas se resuelven contra él), mismo
  orden de pasos, timeouts acotados —. Las hipótesis se comparan con un
  experimento A/B: con y sin la condición sospechada, **una sola condición
  cambiada por corrida**, nunca varias a la vez.
- **Comparar hechos, no apariencias**: bytes, mtimes y tamaños con las
  herramientas del toolchain (`cmp`, `od`/`xxd`, `stat`, `grep -a`) y exit
  codes, en vez de "se veía igual". El log del motor es fuente primaria: cuando
  algo se descarta o se elige un camino, que el log diga **por qué** (prefijo
  del módulo + causa técnica), no solo qué pasó.
- **Sanitizadores en C++**: la opción `ENABLE_ASAN` de `CMakeLists.txt`
  (por defecto `ON`) agrega `-fsanitize=address` y `-fsanitize=undefined` con
  `-fno-omit-frame-pointer -O1 -g` cuando el compilador es GNU/Clang. Un fix de
  memoria —fuga, doble liberación, use-after-free, buffer overflow— debe correr
  contra un build con sanitizer antes de darse por cerrado. Ojo: con el GCC de
  MSYS2 esos runtimes no linkean (ver "Comandos esenciales"), por eso los
  builds de trabajo y la CI usan `-DENABLE_ASAN=OFF`; cuando el toolchain local
  no los soporte, el build con sanitizer se hace donde sí compile (Linux con
  GCC/Clang), idealmente sobre los targets de test que cubren el caso.
- **Otras herramientas**: `gdb`/`lldb` (breakpoint y backtrace) cuando la causa
  no se alcanza por log; en Linux, `valgrind` como respaldo del sanitizer; y la
  compilación aislada de la sonda o del test que reproduce el caso —sin compilar
  todo el engine— para iterar rápido.
- **Varias circunstancias, un solo fix**: antes de cerrar, reproducir el caso
  también en las variantes que existan —distintos estados de la máquina, con y
  sin interacción previa, rutas con y sin espacios, plataforma cuando esté
  disponible—. Un fix validado en una sola corrida sigue siendo una hipótesis.

## Depuración por orden de dependencia

Cuando un lote de correcciones tiene varios ítems (bugs, hallazgos nuevos,
mejoras), la lista de tareas se arma **por orden de dependencia**, no por el
orden en que se descubrieron los problemas ni por su severidad:

1. **Primero, las bases**: los fixes de los que cuelgan otros —los que tocan el
   mismo código, fijan un contrato que los demás heredan o habilitan un camino
   que otros dan por supuesto—, aunque su severidad sea menor que la de sus
   dependientes. Un ítem que hereda el arreglo de otro va después de ese otro,
   sin excepción.
2. **Después, los independientes**: los que no tocan a nadie, en cualquier
   orden. Si un fix puede revelar problemas nuevos (p. ej. un test que pasa a
   correr donde antes se saltaba), hacerlo antes que los que dependan de que
   el alcance esté cerrado.
3. **Al final, los de mayor riesgo**: los que reescriben lo ya validado o
   tocan caminos críticos; siempre después de que sus bases estén commiteadas
   y verificadas.

Reglas de trabajo sobre la lista:

- Es explícita y se trabaja contra ella, no de memoria (ver "Lista de tareas
  antes de escribir código"). Cada ítem se marca en cuanto queda hecho y
  verificado, no todo al final.
- Un ítem bloqueado por otro se espera, no se salta: saltar el orden deja
  fixes aplicados sobre bases que todavía pueden cambiar.
- Al aparecer un hallazgo nuevo, insertarlo en la posición que le corresponda
  según sus dependencias e indicar de qué ítem cuelga, en vez de agregarlo al
  final de la cola.
- El orden se documenta donde viva el lote (plan de trabajo, issue, tabla de
  secuencia de commits), para que quien retome el trabajo sepa por dónde
  sigue.
