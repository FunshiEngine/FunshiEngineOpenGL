# Template de commit — fix

## Formato

```
fix(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
fix(Rendering): corregir redeclaración de dirty_ en GameObject.h
```

## Cuerpo del mensaje

Cadena de análisis de bug (ver fix.md):

**Síntoma:** Qué se observa (comportamiento inesperado, ubicación, plataforma)

**Evidencia:** Archivo:línea, logs, stack trace, exit codes, reproductibilidad

**Causa:** Causa raíz aislada (una variable a la vez, experimento A/B)

**Fix:** Cambio mínimo implementado

**Test:** Test headless rojo→verde, sanitizer si toca memoria, varias circunstancias

## Proceso de trabajo (ver fix.md)

- [ ] Identificación del problema (síntoma, ubicación, contexto)
- [ ] Análisis de causa (investigación, hipótesis, validación)
- [ ] Desarrollo del fix (solución mínima, pruebas, sanitizer, compatibilidad)
- [ ] Validación final (build completo, tests en verde, usuario, documentación)

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Flujo: `fix/<tema>` desde `develop` (o `master` si es hotfix urgente)
- Ver `doc/FLUJO_DE_RAMAS.md`
