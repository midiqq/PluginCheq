@echo off
setlocal EnableDelayedExpansion

title PluginCheq - Limpieza e Instalador Oficial VST3

:: 1. Auto-elevacion automatica a Administrador (UAC) para poder limpiar C:\Program Files\Common Files\VST3
net session >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ELEVACION] Solicitando permisos de Administrador para limpiar versiones viejas e instalar...
    powershell -NoProfile -Command "Start-Process cmd.exe -ArgumentList '/c \"\"%~f0\"\"' -Verb RunAs"
    exit /b
)

echo ====================================================================
echo       PluginCheq - Limpieza de Versiones Viejas e Instalacion VST3
echo ====================================================================
echo.

set "DEST_DIR=%COMMONPROGRAMFILES%\VST3"
set "USER_VST3=%LOCALAPPDATA%\Programs\Common\VST3"
set "SRC_DIR=%~dp0PluginCheq.vst3"

:: 2. Verificar que Ableton / DAW no tenga bloqueado el archivo
tasklist /FI "IMAGENAME eq Ableton Live*" 2>NUL | find /I "Ableton" >NUL
if %ERRORLEVEL% EQU 0 (
    echo [IMPORTANTE] Cierra Ableton Live antes de continuar para liberar el archivo VST3 viejo.
    pause
)

echo [1/3] Buscando y eliminando TODAS las versiones viejas o duplicadas de PluginCheq...

for %%D in ("%DEST_DIR%" "%USER_VST3%" "C:\Program Files\VSTPlugins" "C:\Program Files\Steinberg\VstPlugins") do (
    if exist "%%~D\PluginCheq.vst3" (
        if not exist "%%~D\PluginCheq.vst3\" (
            echo   - Eliminando archivo monolitico antiguo: %%~D\PluginCheq.vst3
            del /f /q /a "%%~D\PluginCheq.vst3"
        ) else (
            echo   - Eliminando carpeta bundle anterior: %%~D\PluginCheq.vst3\
            rmdir /s /q "%%~D\PluginCheq.vst3"
        )
    )
    if exist "%%~D\PluginCheq.dll" (
        echo   - Eliminando DLL antiguo: %%~D\PluginCheq.dll
        del /f /q /a "%%~D\PluginCheq.dll"
    )
    if exist "%%~D\PluginCheq.exe" (
        echo   - Eliminando Standalone antiguo: %%~D\PluginCheq.exe
        del /f /q /a "%%~D\PluginCheq.exe"
    )
)

:: Doble verificacion con PowerShell por si Windows mantuvo atributos de solo lectura
if exist "%DEST_DIR%\PluginCheq.vst3" (
    powershell -NoProfile -Command "Remove-Item -LiteralPath '%DEST_DIR%\PluginCheq.vst3' -Force -Recurse -ErrorAction Stop"
)

echo.
echo [2/3] Instalando el bundle VST3 limpio y actualizado en:
echo       %DEST_DIR%\PluginCheq.vst3
if not exist "%DEST_DIR%" mkdir "%DEST_DIR%"
robocopy "%SRC_DIR%" "%DEST_DIR%\PluginCheq.vst3" /MIR /NP /NFL /NDL /NJH /NJS >nul

echo.
if exist "%DEST_DIR%\PluginCheq.vst3\Contents\x86_64-win\PluginCheq.vst3" (
    echo ====================================================================
    echo   [3/3 EXITO TOTAL] PluginCheq instalado sin rastros de versiones viejas!
    echo ====================================================================
    echo.
    echo PASO CLAVE EN ABLETON LIVE PARA BORRAR LA CACHE DE VERSIONES VIEJAS:
    echo   1. Abre Ableton Live y entra a Options -^> Settings (Ctrl + ,) -^> Plug-Ins.
    echo   2. Manten presionada la tecla [ALT] en tu teclado y haz click en [Rescan].
    echo      (ALT + Rescan obliga a Ableton a borrar toda su cache vieja del plugin).
    echo   3. Nota: Tienes "C:\Program Files\Common Files\VST3" configurado tambien
    echo      como carpeta VST2 Custom en Ableton; si no tienes DLLs VST2 ahi, puedes
    echo      desactivar VST2 Custom Folder para que escanee mucho mas rapido.
    echo.
) else (
    echo [ERROR] No se pudo escribir en %DEST_DIR%\PluginCheq.vst3.
    echo Asegurate de cerrar completamente Ableton Live / Reaper / FL Studio e intenta de nuevo.
    echo.
)

pause
