# Plantilla de mensajes de commit

GitHub no ofrece plantillas de mensaje de commit (esa función se retiró), así
que este archivo es la referencia de la convención. La regla completa vive en
`AGENTS.md`; este resumen es para tenerla a mano al commitear.

## Formato

```
<tipo>(<ámbito>): <asunto en imperativo y en minúsculas>
```

El ámbito es opcional. Si se omite, el asunto cierra el encabezado con dos
puntos.

```
fix(packaging): hacer que la seccion [Code] del .iss compile
docs: corregir la estructura de carpetas del manual
```

## Tipos admitidos

| Tipo | Cuándo usarlo |
|---|---|
| `feat` | Funcionalidad nueva. |
| `fix` | Corrección de un error. |
| `refactor` | Reestructuración sin cambiar el comportamiento. |
| `docs` | Solo documentación. |
| `test` | Solo tests. |
| `build` | `CMakeLists.txt`, empaquetado, dependencias. |
| `ci` | Workflows de GitHub Actions. |
| `perf` | Mejora de rendimiento. |
| `chore` | Tareas de mantenimiento que no encajan en los anteriores. |

## Ámbitos frecuentes

`packaging`, `iniciocodigo`, `ci`, `github`, `configuracion`, `scripts`,
`explorador`, `fisicas`, `render`, `audio`, `interfaz`.

## Reglas

- **Asunto en imperativo y en minúsculas**: "agrega el fallback", no "Agregando
  el fallback" ni "Se agregó el fallback".
- **Una línea, sin punto final.** Si el cambio necesita explicación, va en el
  cuerpo del mensaje, separado por una línea en blanco.
- **Cuerpo para el porqué, no para el qué.** El qué ya está en el diff.
- **Sin atribuciones**: nada de coautoría, firmas ni menciones a herramientas o
  asistentes. El mensaje describe únicamente el cambio sobre el código.
- **Commits atómicos por tarea.** Si el commit mezcla dos temas sin relación,
  se parte en dos.

## Ejemplo completo

```
fix(scripts): alinear el arbol de SerializeField con los campos del script

El inspector iteraba por los campos reflejados e indexaba el arbol de
valores sin acotar, asi que una escena guardada antes de agregar un
SerializeField leia fuera de rango y terminaba en bad_variant_access.

El emparejado por nombre que el header de la reflexion ya describia nunca
se llamo. Se implementa alinearValores y se invoca al compilar el
script, que es cuando se conocen los campos reales.
```
