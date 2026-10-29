#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ENDURECER EL BORDE DIFUMINADO DE LAS TEXTURAS DE PLANTA
=======================================================
BUG: la penca de nopal colocada sale con el borde deshilachado y agujeros por
los que se ve el fondo.

LA CAUSA, medida sobre Blocks/Penca de Nopal de Castilla.png (171x256):

    alfa = 0        20%   el fondo limpio
    alfa 1..25      10%   4.133 px de BORDE DIFUMINADO
    alfa 26..127     4%

El dibujo no termina en un canto limpio: se DESVANECE poco a poco hacia los
bordes. El motor recorta con un alpha test, que es BINARIO --el pixel pasa o no
pasa-- asi que corta ese degradado por la mitad y deja un contorno dentado con
huecos donde el alfa bajaba antes de tiempo.

POR QUE NO SE ARREGLA BAJANDO EL UMBRAL
---------------------------------------
Se cambiarian los agujeros por un halo de pixeles casi invisibles alrededor de
la planta. El problema no es DONDE se corta, sino que el degradado existe: con
un test binario, cualquier umbral parte la rampa en algun sitio.

POR QUE NO SE QUITA EL ALPHA TEST
---------------------------------
Porque el 20% de la imagen es fondo con alfa 0. Sin recorte, el motor lo pinta
NEGRO: se cambiarian los agujeros por un recuadro negro, que es peor.

LA SOLUCION: BINARIZAR EL ALFA
------------------------------
Se decide de una vez, EN LA IMAGEN, que es planta y que es fondo:

    alfa >= CORTE  ->  255 (opaco del todo)
    alfa <  CORTE  ->    0 (transparente del todo)

Asi el alpha test ya no tiene nada que partir: el contorno es nitido y no hay
rampa que deje huecos. Es como funcionan las texturas de vegetacion de
cualquier motor voxel.

EL COLOR DEL BORDE SE LIMPIA ANTES
----------------------------------
Un pixel semitransparente guarda color MEZCLADO con lo que tenia detras. Al
subirlo a opaco, ese color se vuelve visible y aparece un halo claro.

Por eso, antes de binarizar, a los pixeles de la rampa se les despeja el color
tomando el del vecino opaco mas cercano. El contorno queda del color de la
planta, no de la mezcla.
"""

import argparse
import os
import shutil
import sys
from collections import deque

try:
    from PIL import Image
except ImportError:
    sys.exit("Hace falta Pillow:  pip install pillow")

# Por encima de este alfa, el pixel es PLANTA. Se elige bajo (64 de 255) porque
# la rampa de estas texturas es larga: con un corte alto se comeria el perfil
# fino de las espinas, que son justo lo que da caracter al nopal.
CORTE = 64

DIR_COPIA = "_borde_original"


def endurecer(ruta, simular):
    im = Image.open(ruta)
    if im.mode != "RGBA":
        return False, "no es RGBA (sin canal alfa que endurecer)"

    w, h = im.size
    px = im.load()

    rampa = sum(1 for y in range(h) for x in range(w)
                if 0 < px[x, y][3] < 255)
    if rampa == 0:
        return False, "el alfa ya es binario"

    # ------------------------------------------------------------------
    # 1) DESPEJAR EL COLOR DE LA RAMPA
    # ------------------------------------------------------------------
    # A cada pixel que va a pasar a opaco se le da el color del pixel OPACO
    # mas cercano. Se hace con una inundacion desde los opacos, asi que el
    # color viaja hacia fuera siguiendo la forma de la planta.
    nuevo = Image.new("RGBA", (w, h))
    npx = nuevo.load()

    color = [[None] * w for _ in range(h)]
    cola = deque()
    for y in range(h):
        for x in range(w):
            if px[x, y][3] >= 255:
                color[y][x] = px[x, y][:3]
                cola.append((x, y))

    while cola:
        x, y = cola.popleft()
        c = color[y][x]
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and color[ny][nx] is None:
                if px[nx, ny][3] > 0:          # solo dentro de la rampa
                    color[ny][nx] = c
                    cola.append((nx, ny))

    # ------------------------------------------------------------------
    # 2) BINARIZAR
    # ------------------------------------------------------------------
    opacos = 0
    for y in range(h):
        for x in range(w):
            r, g, b, a = px[x, y]
            if a >= CORTE:
                c = color[y][x] or (r, g, b)
                npx[x, y] = (c[0], c[1], c[2], 255)
                opacos += 1
            else:
                npx[x, y] = (0, 0, 0, 0)

    frac = opacos / float(w * h)
    if frac < 0.05:
        return False, (f"⚠️ SE DESCARTA: quedaria solo el {frac*100:.1f}% "
                       f"de planta. Original intacto.")

    msg = (f"{w}x{h}  rampa={rampa} px -> binaria  "
           f"planta={frac*100:.0f}%")
    if simular:
        return True, msg + "   (simulacion)"

    carpeta = os.path.join(os.path.dirname(ruta), DIR_COPIA)
    os.makedirs(carpeta, exist_ok=True)
    copia = os.path.join(carpeta, os.path.basename(ruta))
    if not os.path.exists(copia):
        shutil.copy2(ruta, copia)

    nuevo.save(ruta, "PNG")
    return True, msg


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rutas", nargs="+", help="PNG a endurecer")
    ap.add_argument("--simular", action="store_true")
    ap.add_argument("--restaurar", action="store_true")
    args = ap.parse_args()

    if args.restaurar:
        n = 0
        for ruta in args.rutas:
            copia = os.path.join(os.path.dirname(ruta), DIR_COPIA,
                                 os.path.basename(ruta))
            if os.path.exists(copia):
                shutil.copy2(copia, ruta)
                n += 1
        print(f"Restaurados {n}")
        return

    for ruta in args.rutas:
        if not os.path.exists(ruta):
            print(f"  NO EXISTE  {ruta}")
            continue
        try:
            ok, msg = endurecer(ruta, args.simular)
        except Exception as e:                       # noqa: BLE001
            print(f"  ERROR  {os.path.basename(ruta)}: {e}")
            continue
        estado = "ENDURECIDO" if ok else "sin cambios"
        print(f"  {estado}  {os.path.basename(ruta)}")
        print(f"      {msg}")


if __name__ == "__main__":
    main()
