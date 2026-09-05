@echo off
REM ============================================================================
REM ESTE SCRIPT ESTA RETIRADO. USA tools\empaquetar.bat
REM ============================================================================
REM
REM Generaba un "portable" que NO funcionaba: copiaba solo VoxelWorld.exe y se
REM dejaba resourcepacks/, asi que el juego arrancaba SIN TEXTURAS. El motor
REM las lee de disco al iniciar (ver getGameRootPath en main.cpp).
REM
REM Ademas su README describia otro proyecto: hablaba de "8 tipos de bloques"
REM (hay 187), de mundos en projects/ (estan en saves/) y de un motor
REM "extraido de AniWorld".
REM
REM El sustituto hace las dos cosas y esta probado de punta a punta:
REM
REM     tools\empaquetar.bat            el PROYECTO fuente  (~15 MB)
REM     tools\empaquetar.bat juego      solo el JUEGO       (~12 MB)
REM     tools\empaquetar.bat todo       los dos
REM
REM ============================================================================

echo.
echo   Este script esta retirado: generaba un portable sin texturas.
echo.
echo   Usa en su lugar:
echo       tools\empaquetar.bat juego
echo.
pause
