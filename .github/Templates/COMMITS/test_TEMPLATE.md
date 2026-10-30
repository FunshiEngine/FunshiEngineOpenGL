# Template de commit — test

## Formato

```
test(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
test(Particulas): agregar test headless de émisor de partículas
```

## Cuerpo del mensaje

Describe qué tests se agregaron, modificaron o corrigieron.

## Tipos de tests (ver test.md)

- **Unitarios**: validar funciones y clases individuales, ejecutar rápido
- **Integración**: validar interacción entre módulos
- **Headless**: validar sin renderizado/GUI, ideales para CI/CD

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Los tests deben ser autónomos y no depender del orden de ejecución
- Nombres de targets: `<modulo>-tests` o `<funcion>-tests`
- Registro en CMakeLists.txt con `add_test()`
- Ver `doc/FLUJO_DE_RAMAS.md`
