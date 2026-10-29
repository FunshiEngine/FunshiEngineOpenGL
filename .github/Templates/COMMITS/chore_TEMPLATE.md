# Template de commit — chore

## Formato

```
chore(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
chore(configuracion): actualizar .gitignore para ignorar .local/build/
```

## Cuerpo del mensaje

Describe tareas de mantenimiento que no encajan en feat/fix/refactor/docs/test/build/ci/perf.

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Ver `doc/FLUJO_DE_RAMAS.md`
