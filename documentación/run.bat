@echo off
echo ============================================
echo   Voxel World - Sandbox Infinito
echo ============================================
echo.

if not exist "build\bin\Release\VoxelWorld.exe" (
    echo ERROR: VoxelWorld.exe no encontrado
    echo.
    echo Compila primero con:
    echo   cmake -B build -G "Visual Studio 17 2022"
    echo   cmake --build build --config Release
    pause
    exit /b 1
)

echo Iniciando Voxel World...
echo.
echo Controles:
echo   WASD         - Movimiento
echo   Mouse        - Rotar camara
echo   Espacio      - Saltar
echo   ESC          - Bloquear/desbloquear cursor
echo.

cd build\bin\Release
start VoxelWorld.exe
