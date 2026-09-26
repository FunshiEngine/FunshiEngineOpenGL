# Flujo de ramas del repositorio

Convención de ramas de FunshiEngineGL. Define los nombres genéricos que viven en
el remoto, para qué sirve cada uno y el camino que sigue cada cambio desde que se
escribe hasta que llega al código estable.

> Nota: la descripción de ramas en GitHub es una función solo de la interfaz web
> (y por eso no se puede configurar por API). Este documento es la fuente de
> verdad del propósito de cada rama; la misma información se debe mantener aquí
> al agregar o cambiar ramas.

---

## 1. Ramas genéricas del remoto

| Rama | Propósito |
|---|---|
| `master` | Código estable y publicado. Solo se toca con merges de `release` (y hotfixes urgentes). Siempre debe compilar y pasar los tests. |
| `develop` | Integración continua. Las ramas `feature/*` se mergean acá y se prueban como conjunto antes de avanzar al resto del flujo. |
| `test` | Ambiente de pruebas / QA. Recibe candidatos estables de `develop` para verificar regresiones de forma aislada. |
| `staging` | Pre-producción. Candidatos a release listos para la validación final (empaquetado, datos de ejemplo, docs). |
| `release` | Preparación de versiones: dependencias, changelog y tags antes del merge a `master`. |

Regla general: ninguna rama borra el historial de otra. Solo `release` y
`master` aceptan merges hacia adelante; el resto del flujo avanza en una sola
dirección.

---

## 2. Flujo de trabajo

```
feature/*  ->  develop  ->  test  ->  staging  ->  release  ->  master
```

Pasos por cambio:

1. **Crear** la rama desde `develop` (o desde `master` solo para hotfixes).
   Nombre semántico corto: `feature/lo-que-sea`, `fix/bug-cosito`.
2. **Desarrollar** en la rama con commits pequeños y mensajes en español.
3. **Mergear** a `develop` (pull request) cuando el cambio está integrado y
   compila.
4. Promover la cadena: `develop` → `test` → `staging` → `release` a medida que
   la versión madura.
5. Al cerrar versión: tag + merge de `release` → `master`.

Las ramas `test`, `staging` y `release` son de puerta de paso: sirven para
probar/consolidar y se pueden descartar (recreadas desde el punto de la cadena
que corresponda) cuando ya cumplieron su función.

---

## 3. Comandos útiles

Crear una rama de trabajo desde `develop`:

```bash
git fetch origin
git checkout -b feature/ejemplo origin/develop
```

Promover un candidato (ej. `develop` → `test`):

```bash
git fetch origin
git push origin develop:test       # test refleja develop sin tocar nada local
```

Ver el flujo completo:

```bash
git branch -a                      # ramas locales y remotas
git log --oneline --graph origin/develop origin/test origin/staging
```