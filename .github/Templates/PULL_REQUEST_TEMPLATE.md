## Descripción

Describe claramente qué modifica esta pull request y cuál es el motivo del cambio.

## Tipo de cambio

- [ ] Corrección de un error (`fix`)
- [ ] Nueva funcionalidad (`feat`)
- [ ] Refactorización (`refactor`)
- [ ] Documentación (`docs`)
- [ ] Tests (`test`)
- [ ] Build / CMake (`build`)
- [ ] CI/CD (`ci`)
- [ ] Otro

## Cambios realizados

Describe brevemente los principales cambios realizados.

## Proceso de trabajo seguido

- [ ] `feat`: arquitectura analizada, plan realizado, implementación validada, documentación actualizada (ver `feat.md`).
- [ ] `fix`: síntoma → evidencia → causa → fix → test (ver `fix.md`).
- [ ] `refactor`: análisis previo, checklist, validación post-refactor (ver `refactor.md`).
- [ ] `docs`: `.md` del área afectada revisado, `.github/` como documentación del proyecto (ver `doc.md`).

## Pruebas

Describe cómo se probó el cambio.

- [ ] Las pruebas existentes pasan.
- [ ] Se añadieron nuevas pruebas.
- [ ] Se realizaron pruebas manuales.
- [ ] La compilación funciona correctamente.
- [ ] La CI pasa correctamente.

## Arquitectura

¿Este cambio modifica alguna abstracción o parte importante de la arquitectura?

- [ ] No.
- [ ] Sí.

Si la respuesta es sí, explica los cambios:

## Compatibilidad

¿El cambio puede afectar código o comportamiento existente?

- [ ] No.
- [ ] Sí.

Describe los posibles cambios incompatibles:

## Documentación

- [ ] No necesita cambios de documentación.
- [ ] Se actualizó la documentación.
- [ ] La documentación debe actualizarse posteriormente.

## Información adicional

Añade cualquier información relevante para la revisión.

## Flujo de ramas

- Rama de tema: `feature/<tema>`, `fix/<tema>` o `docs/<tema>` desde `develop`.
- Rama personal: `personal/<usuario>/<tema>` desde la rama de tema.
- PR de rama personal → rama de tema.
- PR de rama de tema → `develop`.
- Ver `doc/FLUJO_DE_RAMAS.md`.
