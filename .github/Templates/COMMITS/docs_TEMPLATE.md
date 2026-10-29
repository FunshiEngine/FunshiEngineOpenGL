# Template de commit — docs

## Formato

```
docs(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
docs(README): agregar sección de compilación y ejecución del editor
```

## Cuerpo del mensaje

Describe qué documentación se agregó, corrigió o actualizó.

## Áreas de documentación (ver doc.md)

- `README.md` — Presentación del proyecto
- `doc/MANUAL_DE_USO.md` — Manual de uso (especialmente sección 13, scripting)
- `doc/PROJECT_STRUCTURE.md` — Estructura del proyecto
- `doc/DocuTecnicoBugs.md` — Bugs técnicos por limitaciones de herramientas
- `doc/FLUJO_DE_RAMAS.md` — Flujo de ramas del repositorio
- `.github/` — Documentación del proyecto (COMMIT_TEMPLATE, PULL_REQUEST_TEMPLATE, ISSUE_TEMPLATE, workflows)

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Flujo de ramas: `docs/<tema>` desde `develop`
- Ver `doc/FLUJO_DE_RAMAS.md`
