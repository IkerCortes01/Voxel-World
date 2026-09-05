@echo off
setlocal enabledelayedexpansion
REM ============================================================================
REM VOXEL WORLD - EMPAQUETADOR PARA USB
REM ============================================================================
REM
REM EL PROBLEMA QUE RESUELVE
REM ------------------------
REM La carpeta del proyecto pesa ~250 MB, pero el 93 % de eso NO hace falta
REM para nada: se regenera solo.
REM
REM     build/          105 MB   lo rehace cmake --build
REM     .git/            60 MB   el historial, no el proyecto
REM     graphify-out/    35 MB   cache de analisis, regenerable
REM     saves/           18 MB   tus mundos, no el codigo
REM     ------------------------------------------------
REM     TODO LO DEMAS   ~28 MB   <- esto es el proyecto de verdad
REM
REM El codigo fuente entero (src/) son 3,5 MB: el 1,4 % del total. Copiar la
REM carpeta a pelo a un USB mueve 250 MB para llevarse 3,5 de codigo.
REM
REM MODOS
REM -----
REM   empaquetar            El proyecto FUENTE. Se descomprime y compila.
REM   empaquetar juego      Solo lo JUGABLE: el .exe y sus recursos.
REM   empaquetar todo       Los dos ZIP.
REM
REM Usa `tar` de Windows (viene de serie desde Windows 10 1803), asi que no
REM hace falta instalar nada. Genera .zip normal y corriente.
REM ============================================================================

cd /d "%~dp0\.."
set RAIZ=%CD%
set SALIDA=%RAIZ%\dist

set MODO=%1
if "%MODO%"=="" set MODO=fuente

echo.
echo ============================================
echo   VOXEL WORLD - EMPAQUETADOR
echo ============================================
echo   Proyecto: %RAIZ%
echo   Modo:     %MODO%
echo.

if not exist "%SALIDA%" mkdir "%SALIDA%"

if /i "%MODO%"=="juego"  goto :juego
if /i "%MODO%"=="todo"   goto :fuente
if /i "%MODO%"=="fuente" goto :fuente
echo ERROR: modo desconocido "%MODO%". Usa: fuente ^| juego ^| todo
exit /b 1

REM ============================================================================
:fuente
REM ============================================================================
REM EL PROYECTO PARA SEGUIR TRABAJANDO
REM
REM Lleva TODO lo necesario para compilar en otra maquina sin red: el codigo,
REM los recursos, y las dependencias vendorizadas (GLFW, stb, doctest van
REM dentro de external/ a proposito, por eso el build funciona sin internet).
REM ============================================================================
echo [1/2] Empaquetando el PROYECTO FUENTE...

set ZIP_FUENTE=%SALIDA%\VoxelWorld-fuente.zip
if exist "%ZIP_FUENTE%" del /q "%ZIP_FUENTE%"

REM --exclude va ANTES de los archivos, y tar los aplica por patron de ruta.
REM Se excluye lo regenerable y los restos que no aportan nada:
REM
REM   *.bak / *.backup   copias viejas de main.cpp; el historial de git ya
REM                      cubre eso, y son 865 KB de los 3,5 MB de src/
REM   dist               la propia carpeta de salida, o se empaqueta a si misma
tar -a -c -f "%ZIP_FUENTE%" ^
    --exclude=build ^
    --exclude=.git ^
    --exclude=graphify-out ^
    --exclude=saves ^
    --exclude=dist ^
    --exclude=*.bak ^
    --exclude=*.backup ^
    --exclude=*.pdb ^
    --exclude=*.ilk ^
    --exclude=*.obj ^
    src tests external resourcepacks cmake tools docs ^
    documentaci?n "AI simulator" ^
    CMakeLists.txt README.md .gitignore

if errorlevel 1 (
    echo ERROR: fallo al crear el ZIP del fuente.
    exit /b 1
)

for %%F in ("%ZIP_FUENTE%") do set /a MB=%%~zF/1048576
echo       Listo: VoxelWorld-fuente.zip  (!MB! MB^)
echo       Para usarlo: descomprimir y ejecutar
echo         cmake -B build -G "Visual Studio 17 2022"
echo         cmake --build build --config Release
echo.

if /i not "%MODO%"=="todo" goto :fin

REM ============================================================================
:juego
REM ============================================================================
REM SOLO PARA JUGAR
REM
REM El .exe y lo que necesita en tiempo de ejecucion. NO lleva codigo.
REM
REM !! Las TEXTURAS son imprescindibles: el motor las carga de disco al
REM arrancar (ver getGameRootPath en main.cpp). Sin ellas el juego arranca y
REM se ve sin texturas -- que es el fallo que tenia el script viejo.
REM ============================================================================
echo [2/2] Empaquetando el JUEGO...

if not exist "%RAIZ%\build\bin\Release\VoxelWorld.exe" (
    echo.
    echo   AVISO: no hay VoxelWorld.exe compilado.
    echo   Compila primero:  cmake --build build --config Release
    echo.
    if /i "%MODO%"=="todo" goto :fin
    exit /b 1
)

set TMPJ=%SALIDA%\_juego
if exist "%TMPJ%" rmdir /s /q "%TMPJ%"
mkdir "%TMPJ%"

copy /y "%RAIZ%\build\bin\Release\VoxelWorld.exe" "%TMPJ%\" >nul
xcopy /e /i /q /y "%RAIZ%\resourcepacks" "%TMPJ%\resourcepacks" >nul

REM El runtime de C++ va enlazado ESTATICAMENTE (ver CMakeLists.txt), asi que
REM el .exe corre en un Windows limpio sin instalar el Visual C++
REM Redistributable. Por eso aqui no hay DLLs que copiar.
(
echo Voxel World - alpha 1.0.0
echo.
echo Doble clic en VoxelWorld.exe para jugar.
echo.
echo Los mundos se guardan en:  saves\
echo El registro de la partida: %%LOCALAPPDATA%%\VoxelWorld\log.txt
echo.
echo No necesita instalacion: el runtime de C++ va dentro del ejecutable.
echo.
echo CONTROLES
echo   WASD          Moverse
echo   W W ^(doble^)   Correr
echo   Espacio       Saltar / nadar
echo   Shift         Agacharse
echo   V             Volar ^(creativo^)
echo   E             Inventario
echo   1-9           Slot de la hotbar
echo   Q             Tirar item
echo   F3            Depuracion del controlador
echo   F7            Rendimiento
echo   F11           Pantalla completa
echo   ESC           Pausa
) > "%TMPJ%\LEEME.txt"

set ZIP_JUEGO=%SALIDA%\VoxelWorld-juego.zip
if exist "%ZIP_JUEGO%" del /q "%ZIP_JUEGO%"

pushd "%TMPJ%"
tar -a -c -f "%ZIP_JUEGO%" *
popd
rmdir /s /q "%TMPJ%"

for %%F in ("%ZIP_JUEGO%") do set /a MBJ=%%~zF/1048576
echo       Listo: VoxelWorld-juego.zip  (!MBJ! MB^)
echo.

REM ============================================================================
:fin
REM ============================================================================
echo ============================================
echo   HECHO
echo ============================================
echo   Los ZIP estan en:  %SALIDA%
echo.
dir /b "%SALIDA%\*.zip" 2>nul
echo.
endlocal
