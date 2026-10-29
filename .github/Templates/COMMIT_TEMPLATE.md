# Plantilla de mensajes de commit (maestro)

## Formato

```
<tipo>(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
fix(Objetos): corregir redeclaración de dirty_ en GameObject.h
```

## Tipos admitidos y templates individuales

Cada tipo de commit tiene su propio template en `COMMITS/`:

| Tipo | Template | Cuándo usarlo |
|---|---|---|
| `feat` | `COMMITS/feat_TEMPLATE.md` | Funcionalidad nueva |
| `fix` | `COMMITS/fix_TEMPLATE.md` | Corrección de un error |
| `refactor` | `COMMITS/refactor_TEMPLATE.md` | Reestructuración sin cambiar comportamiento |
| `docs` | `COMMITS/docs_TEMPLATE.md` | Solo documentación |
| `test` | `COMMITS/test_TEMPLATE.md` | Solo tests |
| `build` | `COMMITS/build_TEMPLATE.md` | CMakeLists.txt, empaquetado, dependencias |
| `ci` | `COMMITS/ci_TEMPLATE.md` | Workflows de GitHub Actions |
| `perf` | `COMMITS/perf_TEMPLATE.md` | Mejora de rendimiento |
| `chore` | `COMMITS/chore_TEMPLATE.md` | Tareas de mantenimiento |

Usa el template correspondiente al tipo de commit antes de hacer commit.

## Ámbitos frecuentes (módulos del repo)

`Objetos`, `Rendering`, `Fisicas`, `Particulas`, `UI`, `Inspeccion`, `Comandos`,
`Scripting`, `Audio`, `ci`, `github`, `configuracion`, `scripts`, `build`,
`docs`, `refactor`, `fix`, `feat`.

## Reglas generales

- **Asunto en imperativo y en minúsculas**: "agrega el fallback", no "Agregando
  el fallback" ni "Se agregó el fallback".
- **Una línea, sin punto final.** Si el cambio necesita explicación, va en el
  cuerpo del mensaje, separado por una línea en blanco.
- **Cuerpo para el porqué, no para el qué.** El qué ya está en el diff.
- **Sin atribuciones**: nada de coautoría, firmas ni menciones a herramientas o
  asistentes. El mensaje describe únicamente el cambio sobre el código.
- **Commits atómicos por tarea.** Si el commit mezcla dos temas sin relación,
  se parte en dos.
- **Sin `git add .` ni `git add -A`:** agregar solo los archivos propios del
  commit con `git add <archivos>`.
- **Flujo de ramas**: crear rama de tema desde `develop`
  (`feature/<tema>`, `fix/<tema>`, `docs/<tema>`), rama personal desde la rama
  del tema (`personal/<usuario>/<tema>`), PR de personal → tema, PR de tema →
  `develop`. Ver `doc/FLUJO_DE_RAMAS.md`.
