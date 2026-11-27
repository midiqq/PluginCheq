@echo off
setlocal
echo =======================================================
echo          PluginCheq - Instalador Rapido VST3
echo =======================================================
echo.

set "DEST_DIR=%COMMONPROGRAMFILES%\VST3"

if not exist "%DEST_DIR%" (
    echo Creando directorio VST3 en %DEST_DIR%...
    mkdir "%DEST_DIR%" 2>nul
)

rem Si existe un archivo monolitico viejo llamado PluginCheq.vst3, eliminarlo para permitir copiar la carpeta bundle
if exist "%DEST_DIR%\PluginCheq.vst3" (
    if not exist "%DEST_DIR%\PluginCheq.vst3\" (
        del /f /q "%DEST_DIR%\PluginCheq.vst3" 2>nul
    )
)

echo Copiando PluginCheq.vst3 a %DEST_DIR%...
xcopy /E /I /Y "%~dp0PluginCheq.vst3" "%DEST_DIR%\PluginCheq.vst3" >nul

if %ERRORLEVEL% EQU 0 (
    echo.
    echo [EXITO] PluginCheq.vst3 se instalo correctamente en:
    echo        %DEST_DIR%\PluginCheq.vst3
    echo.
    echo Inicia tu DAW y realiza un rescaneo de plugins.
) else (
    echo.
    echo [AVISO] Se requieren permisos de administrador.
    echo Haz click derecho en este archivo y selecciona:
    echo "Ejecutar como administrador".
)

echo.
pause
