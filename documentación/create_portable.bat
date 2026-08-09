@echo off
REM ============================================================================
REM Voxel World - Creador de Version Portable
REM ============================================================================

echo ============================================
echo   Voxel World - Version Portable
echo ============================================
echo.

set PORTABLE_DIR=VoxelWorld_Portable
set BUILD_DIR=build\bin\Release

REM Verificar compilacion
if not exist "%BUILD_DIR%\VoxelWorld.exe" (
    echo ERROR: VoxelWorld.exe no encontrado
    echo Compila primero con: cmake --build build --config Release
    pause
    exit /b 1
)

REM Limpiar directorio portable anterior
if exist "%PORTABLE_DIR%" (
    echo Eliminando version portable anterior...
    rmdir /s /q "%PORTABLE_DIR%"
)

REM Crear estructura
echo Creando estructura de carpetas...
mkdir "%PORTABLE_DIR%"
mkdir "%PORTABLE_DIR%\projects"

REM Copiar ejecutable
echo Copiando ejecutable...
copy "%BUILD_DIR%\VoxelWorld.exe" "%PORTABLE_DIR%\"

REM Crear archivo de informacion
echo Creando README.txt...
(
echo ============================================
echo   Voxel World - Sandbox Infinito v1.0.0
echo ============================================
echo.
echo Para ejecutar Voxel World:
echo   1. Haz doble clic en VoxelWorld.exe
echo   2. Tus mundos se guardaran en: projects/
echo.
echo Controles:
echo   WASD         - Movimiento
echo   Mouse        - Rotar camara
echo   Espacio      - Saltar
echo   ESC          - Bloquear/desbloquear cursor
echo.
echo Caracteristicas:
echo   - Mundo infinito con chunks
echo   - Generacion procedural de terreno
echo   - Fisica de gravedad realista
echo   - 8 tipos de bloques
echo   - Sistema de guardado
echo.
echo Motor 3D extraido de AniWorld
echo Sin keyframes ni timeline - Solo bloques 3D
echo.
echo Generado: %date% %time%
) > "%PORTABLE_DIR%\README.txt"

REM Crear acceso directo
echo Creando acceso directo...
powershell -Command "$ws = New-Object -ComObject WScript.Shell; $s = $ws.CreateShortcut('%CD%\%PORTABLE_DIR%\VoxelWorld.lnk'); $s.TargetPath = '%CD%\%PORTABLE_DIR%\VoxelWorld.exe'; $s.WorkingDirectory = '%CD%\%PORTABLE_DIR%'; $s.Description = 'Voxel World - Sandbox Infinito'; $s.Save()"

echo.
echo ============================================
echo   VERSION PORTABLE CREADA!
echo ============================================
echo.
echo Ubicacion: %CD%\%PORTABLE_DIR%\
echo.
echo Puedes:
echo   - Copiar esta carpeta a cualquier PC
echo   - Ejecutar desde USB
echo   - No requiere instalacion
echo.
echo Presiona cualquier tecla para abrir la carpeta...
pause >nul
explorer "%PORTABLE_DIR%"
