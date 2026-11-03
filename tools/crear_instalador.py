"""Crea dist/VoxelWorld-Instalador.exe a partir del juego ya compilado.

    cmake --build build --config Release
    python tools/crear_instalador.py

El instalador lo arma IExpress, que viene con Windows: no hay nada que instalar
para generarlo. Dentro lleva el juego en un ZIP y un script de PowerShell que lo
copia a %APPDATA%\\VoxelWorld. Los mundos se guardan en esa misma carpeta, y
reinstalar no los toca.
"""
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path

RAIZ = Path(__file__).resolve().parent.parent
DIST = RAIZ / "dist"
# IExpress no se lleva bien con espacios en la ruta, y la del proyecto los tiene.
TMP = Path(tempfile.gettempdir()) / "voxelworld_instalador"
SALIDA = DIST / "VoxelWorld-Instalador.exe"

# Originales de trabajo de las texturas: el juego no los carga.
EXCLUIR = {"_hd_original", "_borde_original", "_original", "_fuente"}

INSTALAR = r"""param(
    [string]$Destino = (Join-Path $env:APPDATA 'VoxelWorld'),
    [switch]$Prueba      # sin accesos directos, sin registro y sin abrir el juego
)
$ErrorActionPreference = 'Stop'
try {
    Write-Host 'Instalando Voxel World en' $Destino
    New-Item -ItemType Directory -Force -Path $Destino | Out-Null
    # Encima de lo que haya: saves\ no viene en el ZIP, asi que no se toca.
    Expand-Archive -LiteralPath (Join-Path $PSScriptRoot 'juego.zip') -DestinationPath $Destino -Force

    $exe = Join-Path $Destino 'VoxelWorld.exe'
    $desinstalar = Join-Path $Destino 'desinstalar.ps1'
    if (-not $Prueba) {
        $shell = New-Object -ComObject WScript.Shell
        foreach ($carpeta in @([Environment]::GetFolderPath('Desktop'),
                               [Environment]::GetFolderPath('Programs'))) {
            $acceso = $shell.CreateShortcut((Join-Path $carpeta 'Voxel World.lnk'))
            $acceso.TargetPath = $exe
            # Los mundos se buscan en saves\ relativo a la carpeta de trabajo.
            $acceso.WorkingDirectory = $Destino
            $acceso.IconLocation = $exe
            $acceso.Save()
        }
        $clave = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\VoxelWorld'
        New-Item -Path $clave -Force | Out-Null
        Set-ItemProperty -Path $clave -Name DisplayName -Value 'Voxel World'
        Set-ItemProperty -Path $clave -Name DisplayIcon -Value $exe
        Set-ItemProperty -Path $clave -Name InstallLocation -Value $Destino
        Set-ItemProperty -Path $clave -Name Publisher -Value 'Voxel World'
        Set-ItemProperty -Path $clave -Name UninstallString -Value ('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + $desinstalar + '"')
        Set-ItemProperty -Path $clave -Name NoModify -Value 1 -Type DWord
        Set-ItemProperty -Path $clave -Name NoRepair -Value 1 -Type DWord
        Start-Process -FilePath $exe -WorkingDirectory $Destino
    }
    Write-Host 'Listo.'
} catch {
    Write-Host 'No se pudo instalar:' $_
    if (-not $Prueba) { Read-Host 'Pulsa Enter para cerrar' }
    exit 1
}
"""

DESINSTALAR = r"""param([switch]$Prueba)
# Quita el juego y deja los mundos: saves\ se queda en la carpeta.
$Destino = $PSScriptRoot
Get-ChildItem -LiteralPath $Destino -Force |
    Where-Object { $_.Name -ne 'saves' } |
    Remove-Item -Recurse -Force
if (-not $Prueba) {
    foreach ($carpeta in @([Environment]::GetFolderPath('Desktop'),
                           [Environment]::GetFolderPath('Programs'))) {
        Remove-Item -LiteralPath (Join-Path $carpeta 'Voxel World.lnk') -ErrorAction SilentlyContinue
    }
    Remove-Item -Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\VoxelWorld' -Recurse -ErrorAction SilentlyContinue
}
"""

LANZADOR = (
    "@echo off\r\n"
    'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0instalar.ps1"\r\n'
)

SED = """[Version]
Class=IEXPRESS
SEDVersion=3
[Options]
PackagePurpose=InstallApp
ShowInstallProgramWindow=0
HideExtractAnimation=1
UseLongFileName=1
InsideCompressed=0
CAB_FixedSize=0
CAB_ResvCodeSigning=0
RebootMode=N
InstallPrompt=%InstallPrompt%
DisplayLicense=%DisplayLicense%
FinishMessage=%FinishMessage%
TargetName=%TargetName%
FriendlyName=%FriendlyName%
AppLaunched=%AppLaunched%
PostInstallCmd=%PostInstallCmd%
AdminQuietInstCmd=%AdminQuietInstCmd%
UserQuietInstCmd=%UserQuietInstCmd%
SourceFiles=SourceFiles
[Strings]
InstallPrompt=
DisplayLicense=
FinishMessage=
TargetName={salida}
FriendlyName=Voxel World
AppLaunched=cmd.exe /c lanzar.cmd
PostInstallCmd=<None>
AdminQuietInstCmd=
UserQuietInstCmd=
FILE0="juego.zip"
FILE1="instalar.ps1"
FILE2="lanzar.cmd"
[SourceFiles]
SourceFiles0={tmp}\\
[SourceFiles0]
%FILE0%=
%FILE1%=
%FILE2%=
"""


def crear_zip():
    exe = RAIZ / "build" / "bin" / "Release" / "VoxelWorld.exe"
    if not exe.exists():
        raise SystemExit("Falta el juego: cmake --build build --config Release")

    with zipfile.ZipFile(TMP / "juego.zip", "w", zipfile.ZIP_DEFLATED) as z:
        z.write(exe, "VoxelWorld.exe")
        z.writestr("desinstalar.ps1", DESINSTALAR)
        for carpeta in ("resourcepacks", "sounds"):
            for f in sorted((RAIZ / carpeta).rglob("*")):
                relativo = f.relative_to(RAIZ)
                if f.is_file() and not EXCLUIR.intersection(relativo.parts):
                    z.write(f, relativo.as_posix())


def main():
    if TMP.exists():
        shutil.rmtree(TMP)
    TMP.mkdir(parents=True)
    crear_zip()
    (TMP / "instalar.ps1").write_text(INSTALAR, encoding="ascii")
    (TMP / "lanzar.cmd").write_text(LANZADOR, encoding="ascii", newline="")
    sed = TMP / "instalador.sed"
    generado = TMP / "instalador.exe"
    sed.write_text(SED.format(salida=generado, tmp=TMP), encoding="ascii")

    # IExpress da un codigo de salida poco fiable: se mira si salio el archivo.
    subprocess.run(["iexpress", "/N", "/Q", str(sed)])
    if not generado.exists():
        raise SystemExit("IExpress no genero el instalador")
    DIST.mkdir(exist_ok=True)
    shutil.move(str(generado), SALIDA)
    shutil.rmtree(TMP)
    print(f"{SALIDA}  ({SALIDA.stat().st_size / 1048576:.1f} MB)")


if __name__ == "__main__":
    main()
