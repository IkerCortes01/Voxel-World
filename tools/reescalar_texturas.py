#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
REESCALAR TEXTURAS DE BLOQUE A 16x16
====================================

Por que existe esta herramienta
-------------------------------
El motor solo mete en el GL_TEXTURE_2D_ARRAY las texturas de 16x16
(main.cpp, registrarCapa: `if (w != 16 || h != 16) return;`). Una textura
de bloque que no entre en el array rompe la fusion de rangos que consiguio
bajar de 411 batches a 78, y con ella los 195-200 FPS.

Las ilustraciones de alta resolucion se conservan como ARTE FUENTE en
`resourcepacks/Textures/_fuente/`; lo que consume el juego es la version
de 16x16 que genera este script.

Como se reescala, y por que asi
-------------------------------
De 1254x1254 a 16x16 hay un factor de ~78x. Un filtro de caja normal
(LANCZOS/BILINEAR) promedia tantisimo pixel que el resultado sale gris y
sin contraste -- toda la textura acaba del mismo color medio.

El proceso que se usa aqui es en dos pasos:

  1. REDUCCION POR AREA (BOX) hasta 4x el destino (64x64). Promediar es lo
     correcto mientras haya mucho detalle que condensar: conserva el color
     medio real de la imagen sin inventar nada.

  2. LANCZOS del paso intermedio a 16x16, y despues un realce de contraste
     y saturacion. Sin esto la textura se ve lavada al lado de las 59 que
     ya hay en el pack, que son pixel art nativo y tienen el contraste
     alto que pide el estilo.

El realce NO es un capricho estetico: es lo que hace que la textura nueva
se lea junto a las viejas. Una piedra promediada sin realce se ve como un
cuadro gris plano, y a 16x16 eso no es "piedra", es un vacio.

Uso
---
    python tools/reescalar_texturas.py              # reescala las pendientes
    python tools/reescalar_texturas.py --forzar     # rehace todas
    python tools/reescalar_texturas.py --listar     # solo informa, no toca
"""

import sys
import shutil
from pathlib import Path

try:
    from PIL import Image, ImageEnhance
except ImportError:
    print("ERROR: falta Pillow.  pip install Pillow")
    sys.exit(1)

# ----------------------------------------------------------------------------
# RUTAS
# ----------------------------------------------------------------------------
RAIZ = Path(__file__).resolve().parent.parent
BLOQUES = RAIZ / "resourcepacks" / "Textures" / "Blocks"
FUENTE = RAIZ / "resourcepacks" / "Textures" / "_fuente"

# El tamano que exige el motor. No es configurable por capricho: es el que
# comprueba registrarCapa() en main.cpp.
DESTINO = 16

# Umbral a partir del cual se considera que hay que reescalar. Las tiras de
# animacion (64x16) son legitimas y NO se tocan: el motor las corta en
# trozos de 16x16 al cargarlas.
def necesita_reescalado(w: int, h: int) -> bool:
    if w == DESTINO and h == DESTINO:
        return False
    # Tira horizontal de cuadros de 16x16 (animacion del agua, del horno).
    if h == DESTINO and w % DESTINO == 0:
        return False
    return True


# ----------------------------------------------------------------------------
# EL REESCALADO
# ----------------------------------------------------------------------------
def reescalar(origen: Image.Image) -> Image.Image:
    """Reduce a 16x16 conservando contraste. Ver la cabecera del archivo."""
    img = origen.convert("RGBA")

    # Paso 1: promediado por area hasta 4x el destino. BOX es exactamente
    # "media de los pixeles que caen en cada celda", que es lo que se quiere
    # mientras se condensa mucho detalle.
    intermedio = DESTINO * 4
    if img.width > intermedio or img.height > intermedio:
        img = img.resize((intermedio, intermedio), Image.Resampling.BOX)

    # Paso 2: al destino final.
    img = img.resize((DESTINO, DESTINO), Image.Resampling.LANCZOS)

    # Paso 3: devolver el contraste que el promediado se comio.
    #
    # Los valores salen de comparar el resultado con las 59 texturas de
    # pixel art que ya tiene el pack: sin esto las nuevas se ven lavadas y
    # desentonan al ponerlas una al lado de otra.
    img = ImageEnhance.Contrast(img).enhance(1.35)
    img = ImageEnhance.Color(img).enhance(1.20)
    img = ImageEnhance.Sharpness(img).enhance(1.40)

    return img


def procesar(ruta: Path, forzar: bool, solo_listar: bool) -> str:
    """Devuelve una linea de informe. No lanza: informa del fallo."""
    try:
        with Image.open(ruta) as im:
            w, h = im.size
            formato = im.format

            if not necesita_reescalado(w, h):
                return ""

            if solo_listar:
                return f"  {w}x{h}  ->  16x16   {ruta.name}"

            # El arte original se guarda ANTES de tocarlo. Si ya hay copia
            # de una pasada anterior no se pisa: la primera es la buena.
            FUENTE.mkdir(parents=True, exist_ok=True)
            respaldo = FUENTE / ruta.name
            if not respaldo.exists():
                shutil.copy2(ruta, respaldo)
            elif not forzar:
                # Ya procesada antes y el archivo volvio a crecer: se
                # reescala desde el respaldo, que es el arte de verdad.
                pass

            base = Image.open(respaldo) if respaldo.exists() else im
            pequena = reescalar(base)
            if base is not im:
                base.close()

            pequena.save(ruta, "PNG", optimize=True)
            return f"  {w}x{h}  ->  16x16   {ruta.name}"

    except Exception as e:
        return f"  [FALLO] {ruta.name}: {e}"


def main() -> int:
    forzar = "--forzar" in sys.argv
    solo_listar = "--listar" in sys.argv

    if not BLOQUES.is_dir():
        print(f"ERROR: no existe {BLOQUES}")
        return 1

    print(f"Carpeta: {BLOQUES}")
    print(f"Arte fuente: {FUENTE}")
    print()

    lineas = []
    for ruta in sorted(BLOQUES.glob("*.png")):
        linea = procesar(ruta, forzar, solo_listar)
        if linea:
            lineas.append(linea)

    if not lineas:
        print("Todas las texturas de bloque ya son 16x16. Nada que hacer.")
        return 0

    print("REESCALADAS:" if not solo_listar else "PENDIENTES:")
    for l in lineas:
        print(l)
    print()
    print(f"Total: {len(lineas)}")
    if not solo_listar:
        print()
        print("El arte original quedo en resourcepacks/Textures/_fuente/")
    return 0


if __name__ == "__main__":
    sys.exit(main())
