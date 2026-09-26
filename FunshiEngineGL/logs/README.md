# Logs

La app escribe aqui un archivo `FunshiEngineGL_AAAAMMDD_HHMMSS.log` por
arranque (header de inicio + diagnostico GPU + mensajes `[diag]` del render +
errores). El nombre lleva timestamp UTC.

- En runtime los logs se generan en la carpeta `logs/` **junto al ejecutable**
  (con los builds tipicos: `build/logs/`).
- Esta carpeta del repositorio queda como referencia/placeholder; los `.log`
  son artefactos de runtime y no se versionan.