"""Crea dist/VoxelWorld-Instalador.exe a partir del juego ya compilado.

    cmake --build build --config Release
    python tools/crear_instalador.py

El instalador lo arma IExpress, que viene con Windows: no hay nada que instalar
para generarlo. Dentro lleva el proyecto en un ZIP y un script de PowerShell que
lo copia a %APPDATA%\\VoxelWorld por etapas, con una ventana de progreso: la
aplicacion, las texturas, los sonidos y los demas archivos. Los mundos se guardan
en esa misma carpeta, y reinstalar no los toca.
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

# Lo que NO viaja al PC del jugador. Todo lo demas de la carpeta del proyecto, si.
EXCLUIR_RAIZ = {
    "build", "dist", "graphify-out",   # generados, 280 MB
    "saves",                           # los mundos de quien genera el instalador
    "HTML.txt", "Voxelworld.com", "nul", "dens.cpp",
}
# Originales de 1254 px: 254 MB que el juego no carga. Triplicarian la descarga.
EXCLUIR_DENTRO = {"_hd_original"}

INSTALAR = r"""param(
    [string]$Destino = (Join-Path $env:APPDATA 'VoxelWorld'),
    [switch]$Prueba      # sin accesos directos, sin registro y sin abrir el juego
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type -AssemblyName System.IO.Compression.FileSystem

function Etapa([string]$nombre) {
    if ($nombre -match '^(VoxelWorld\.exe|desinstalar\.ps1)$') { return 0 }
    if ($nombre -match '^(resourcepacks/)?sounds/') { return 2 }
    if ($nombre -match '^resourcepacks/') { return 1 }
    return 3
}
$a = [char]0xE1; $o = [char]0xF3
$ETAPAS = @("Instalando la aplicaci${o}n", 'Instalando las texturas',
            'Instalando los sonidos', "Instalando los dem${a}s archivos")

$ventana = New-Object Windows.Forms.Form
$ventana.Text = 'Instalando Voxel World'
$ventana.ClientSize = New-Object Drawing.Size(460, 120)
$ventana.FormBorderStyle = 'FixedDialog'
$ventana.ControlBox = $false
$ventana.StartPosition = 'CenterScreen'
$ventana.BackColor = [Drawing.Color]::FromArgb(10, 12, 24)
$ventana.ForeColor = [Drawing.Color]::White
$texto = New-Object Windows.Forms.Label
$texto.SetBounds(20, 18, 420, 24)
$detalle = New-Object Windows.Forms.Label
$detalle.SetBounds(20, 82, 420, 24)
$detalle.ForeColor = [Drawing.Color]::Silver
$barra = New-Object Windows.Forms.ProgressBar
$barra.SetBounds(20, 48, 420, 24)
$ventana.Controls.AddRange(@($texto, $barra, $detalle))
# Sin esto abre detras de las demas ventanas, y parece que no hace nada.
$ventana.TopMost = $true
$ventana.Show()
$ventana.Activate()
[Windows.Forms.Application]::DoEvents()

try {
    New-Item -ItemType Directory -Force -Path $Destino | Out-Null
    $zip = [IO.Compression.ZipFile]::OpenRead((Join-Path $PSScriptRoot 'juego.zip'))
    # Encima de lo que haya: saves\ no viene en el ZIP, asi que no se toca.
    $archivos = @($zip.Entries | Where-Object { $_.Name } |
                  Sort-Object { Etapa $_.FullName })
    $total = [double](($archivos | Measure-Object Length -Sum).Sum)
    $hecho = 0.0
    foreach ($e in $archivos) {
        $texto.Text = $ETAPAS[(Etapa $e.FullName)]
        $porcentaje = [int](100 * $hecho / $total)
        $barra.Value = $porcentaje
        $detalle.Text = "$porcentaje %"
        [Windows.Forms.Application]::DoEvents()
        $ruta = Join-Path $Destino ($e.FullName -replace '/', '\')
        New-Item -ItemType Directory -Force -Path (Split-Path $ruta) | Out-Null
        [IO.Compression.ZipFileExtensions]::ExtractToFile($e, $ruta, $true)
        $hecho += $e.Length
    }
    $zip.Dispose()
    $barra.Value = 100

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
    $ventana.Close()
} catch {
    $ventana.Close()
    if ($Prueba) { Write-Output "ERROR: $_" }
    else { [Windows.Forms.MessageBox]::Show("No se pudo instalar. Si el juego esta abierto, cierralo y vuelve a intentarlo.`n`n$_", 'Voxel World') | Out-Null }
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
    'powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "%~dp0instalar.ps1"\r\n'
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
        for f in sorted(RAIZ.rglob("*")):
            relativo = f.relative_to(RAIZ)
            primero = relativo.parts[0]
            # Lo que empieza por punto (.git, .claude...) tampoco viaja.
            if (primero.startswith(".") or primero in EXCLUIR_RAIZ or
                    EXCLUIR_DENTRO.intersection(relativo.parts) or
                    "__pycache__" in relativo.parts or not f.is_file()):
                continue
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
