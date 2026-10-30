# Template de commit — feat

## Formato

```
feat(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
feat(Objetos): agregar sistema de seguimiento de suciedad a GameObject
```

## Cuerpo del mensaje

Describe el porqué (no el qué — el qué ya está en el diff).

## Proceso de trabajo (ver feat.md)

- [ ] Análisis arquitectónico previo
- [ ] Modificación de código necesaria
- [ ] Actualización de CMakeLists.txt (si aplica)
- [ ] Nuevas dependencias (bibliotecas, herramientas)
- [ ] Pruebas unitarias/headless
- [ ] Actualización de documentación
- [ ] Pruebas manuales en el editor
- [ ] Validación en CI (GitHub Actions)

## Criterios de aceptación

- [ ] La feature funciona según su descripción
- [ ] `cmake --build build -j$(nproc)` exitoso
- [ ] Las suites de tests relevantes pasan (`ctest --output-on-failure`)
- [ ] Sin regressions en funcionalidad existente
- [ ] Documentación actualizada si es necesario

## Flujo de ramas

- Rama de tema: `feature/<tema>` desde `develop`
- Rama personal: `personal/<usuario>/<tema>` desde la rama de tema
- PR de rama personal → rama de tema
- PR de rama de tema → `develop`
- Ver `doc/FLUJO_DE_RAMAS.md`

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
