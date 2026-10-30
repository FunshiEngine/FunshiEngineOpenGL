# Template de commit — refactor

## Formato

```
refactor(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
refactor(Comandos): extraer GestorComandos como inyección de dependencias
```

## Cuerpo del mensaje

Describe qué se reestructuró y por qué (sin cambiar comportamiento externo).

## Checklist de refactorización (ver refactor.md)

- [ ] El build sigue compilando (`cmake --build build -j$(nproc)`)
- [ ] Las tests pasan (`ctest --output-on-failure`)
- [ ] El comportamiento es idéntico (misma salida esperada)
- [ ] No hay nuevos warnings del compilador
- [ ] Documentación actualizada si firma de API cambió
- [ ] Código revisado por pares (obligatorio para cambios significativos)

## Proceso de trabajo (ver refactor.md)

- [ ] Documentación: analizar arquitectura asociada al problema
- [ ] Análisis: plantear análisis de la arquitectura relacionada
- [ ] Verificación: revisar consistencia de la arquitectura revisada
- [ ] Sanitización: refactorizar para resolver problemas/inconsistencias
- [] Planificación: realizar plan de refactor asociado al problema
- [ ] Tests: validar corrección del código generado
- [ ] Documentación: documentar código nuevo y cambios realizados

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Flujo de ramas: `refactor` va en `feature/<tema>` o `fix/<tema>` desde `develop`
- Ver `doc/FLUJO_DE_RAMAS.md`
