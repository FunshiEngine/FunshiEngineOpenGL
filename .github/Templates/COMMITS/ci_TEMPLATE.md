# Template de commit — ci

## Formato

```
ci(<ámbito>): <asunto en imperativo y en minúsculas>
```

Ejemplo:
```
ci(github): agregar target de test particulas-tests al workflow ci.yml
```

## Cuerpo del mensaje

Describe cambios en workflows de GitHub Actions (`.github/workflows/`).

## Workflows del repo (ver ci.md)

- `.github/workflows/ci.yml` — CI principal (Ubuntu, Windows)
- `.github/workflows/release.yml` — Release
- `.github/workflows/windows-release.yml` — Release para Windows

## Comandos de CI (ver ci.md)

```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DENABLE_ASAN=OFF
cmake --build build -j$(nproc)
ctest --output-on-failure
```

## Reglas

- Asunto en imperativo y en minúsculas, una línea, sin punto final
- Sin `Co-authored-by`, firmas ni menciones a herramientas o asistentes
- Commit atómico: solo archivos propios de esta tarea, `git add <archivos>` (nunca `git add .`)
- Agregar targets de test nuevos en `.github/workflows/ci.yml` cuando se agregue un `.cpp` de test
- Ver `doc/FLUJO_DE_RAMAS.md`
