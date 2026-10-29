# Template de commit — perf

## Formato

```
perf(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
perf(Rendering): optimizar bucle de renderizado de regiones
```

## Cuerpo del mensaje

Describe la mejora de rendimiento y las métricas afectadas (FPS, memory, CPU, etc.).

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Documentar métricas antes y después si es relevante
- Ver `doc/FLUJO_DE_RAMAS.md`
