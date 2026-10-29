#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
QUITAR EL FONDO BLANCO DE UNA TEXTURA
=====================================

Por que existe esta herramienta
-------------------------------
`Penca de Nopal de Castilla.png` llegaba en `Format24bppRgb`: **sin canal
alfa**. Un PNG sin alfa no tiene transparencia que valga, asi que el fondo
blanco del dibujo se pintaba tal cual, y la penca se veia como un recorte
sobre un cuadrado blanco.

Esto NO es el bug de los canales que documenta `render/TinteBioma.h` (aquel
era `loadTexture` pidiendo RGBA y leyendo `channels` del archivo). Aqui el
archivo de verdad no trae alfa: por muy bien que lo cargue el motor, no hay
transparencia que sacar.

Por que flood fill desde el borde, y no "todo lo blanco fuera"
--------------------------------------------------------------
La tentacion es `if pixel casi blanco: alfa=0`. Eso borra tambien los
blancos que estan DENTRO del dibujo -- los brillos del borde de la penca y
sobre todo **las espinas**, que son justo lo que hace que se lea como un
cactus.

Asi que se hace al contrario: se inunda desde los cuatro bordes de la imagen
y solo se vuelve transparente lo que es alcanzable desde fuera sin cruzar un
pixel de la planta. Un blanco rodeado de verde no se toca nunca.

El borde se difumina (antialias)
--------------------------------
Recortar a alfa 0/255 seco deja el contorno con dientes de sierra, y en una
penca --que es casi todo contorno-- se ve mucho. A los pixeles del limite se
les da un alfa intermedio segun cuanto del pixel era fondo, lo que suaviza
la silueta sin ensancharla.

El halo blanco, y por que hay que arreglarlo
--------------------------------------------
Un pixel semitransparente conserva su COLOR original, que en el borde es
casi blanco. Al mezclarlo, ese blanco tine el contorno de claro: el famoso
halo. Se corrige empujando el color de los pixeles del borde hacia el color
de la planta vecina, asi que lo que se mezcla ya es verde y no blanco.

Uso
---
    python tools/quitar_fondo.py "resourcepacks/Textures/Blocks/Penca de Nopal de Castilla.png"
    python tools/quitar_fondo.py ruta.png --umbral 240 --dry-run

Deja una copia `.orig.png` al lado la primera vez, para poder volver atras.
"""

import argparse
import os
import sys
from collections import deque

try:
    from PIL import Image
except ImportError:
    sys.exit("Hace falta Pillow:  pip install Pillow")


def es_fondo(px, umbral):
    """Casi blanco y sin saturacion: el fondo del dibujo."""
    r, g, b = px[0], px[1], px[2]
    if r < umbral or g < umbral or b < umbral:
        return False
    # Un blanco de fondo es NEUTRO. Si los canales se separan mucho es un
    # brillo con tinte (verde palido de la penca), y eso no es fondo.
    return (max(r, g, b) - min(r, g, b)) <= 12


def quitar_fondo(ruta, umbral=238, dry_run=False):
    img = Image.open(ruta)
    ancho, alto = img.size
    tenia_alfa = img.mode in ("RGBA", "LA") or "transparency" in img.info
    img = img.convert("RGBA")
    px = img.load()

    # ------------------------------------------------------------------
    # 1. INUNDACION DESDE LOS BORDES
    # ------------------------------------------------------------------
    # Solo es fondo lo que se alcanza desde fuera. Un blanco encerrado por
    # la planta (brillos, espinas) queda intacto.
    fondo = bytearray(ancho * alto)
    cola = deque()

    def sembrar(x, y):
        i = y * ancho + x
        if not fondo[i] and es_fondo(px[x, y], umbral):
            fondo[i] = 1
            cola.append((x, y))

    for x in range(ancho):
        sembrar(x, 0)
        sembrar(x, alto - 1)
    for y in range(alto):
        sembrar(0, y)
        sembrar(ancho - 1, y)

    while cola:
        x, y = cola.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < ancho and 0 <= ny < alto:
                sembrar(nx, ny)

    n_fondo = sum(fondo)
    if n_fondo == 0:
        print(f"  {os.path.basename(ruta)}: no se encontro fondo alcanzable "
              f"desde el borde (umbral {umbral}). Sin cambios.")
        return False

    # ------------------------------------------------------------------
    # 2. ALFA CON BORDE SUAVE + COLOR SIN HALO
    # ------------------------------------------------------------------
    # Un pixel de planta que toca el fondo se queda opaco, pero los del
    # limite del fondo reciben alfa parcial segun cuantos vecinos suyos son
    # planta: eso es lo que redondea el contorno.
    nuevo = Image.new("RGBA", (ancho, alto))
    npx = nuevo.load()

    for y in range(alto):
        for x in range(ancho):
            i = y * ancho + x
            r, g, b, a = px[x, y]
            if not fondo[i]:
                # PLANTA: opaca, color tal cual.
                npx[x, y] = (r, g, b, 255)
                continue

            # FONDO: transparente. Pero si toca planta, se le da un alfa
            # intermedio y se le roba el color al vecino de planta, para que
            # al mezclarse no aporte blanco (el halo).
            vecinos_planta = []
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1),
                           (1, 1), (1, -1), (-1, 1), (-1, -1)):
                nx, ny = x + dx, y + dy
                if 0 <= nx < ancho and 0 <= ny < alto:
                    if not fondo[ny * ancho + nx]:
                        vecinos_planta.append(px[nx, ny])

            if not vecinos_planta:
                npx[x, y] = (0, 0, 0, 0)          # fondo limpio
            else:
                # Cuanto de este pixel "pertenece" al contorno.
                cobertura = len(vecinos_planta) / 8.0
                alfa = int(round(255 * min(cobertura, 0.55)))
                mr = sum(v[0] for v in vecinos_planta) // len(vecinos_planta)
                mg = sum(v[1] for v in vecinos_planta) // len(vecinos_planta)
                mb = sum(v[2] for v in vecinos_planta) // len(vecinos_planta)
                npx[x, y] = (mr, mg, mb, alfa)

    pct = 100.0 * n_fondo / (ancho * alto)
    print(f"  {os.path.basename(ruta)}: {ancho}x{alto} "
          f"{'(ya tenia alfa)' if tenia_alfa else '(SIN alfa)'} -> "
          f"fondo retirado {n_fondo} px ({pct:.1f}%)")

    if dry_run:
        print("    --dry-run: no se escribe nada")
        return False

    # La copia va a `_fuente/`, que es donde este proyecto guarda el ARTE
    # FUENTE (ver tools/reescalar_texturas.py). Dejarla al lado del PNG bueno,
    # dentro de Blocks/, la metia en el resourcepack que se distribuye y encima
    # la exponia a los tests que barren esa carpeta.
    base = os.path.basename(ruta)
    fuente = os.path.join(os.path.dirname(ruta), "..", "_fuente")
    fuente = os.path.normpath(fuente)
    if os.path.isdir(fuente):
        orig = os.path.join(
            fuente, os.path.splitext(base)[0] + " (fondo blanco original).png")
    else:
        orig = os.path.splitext(ruta)[0] + ".orig.png"
    if not os.path.exists(orig):
        Image.open(ruta).save(orig)
        print(f"    copia de seguridad -> {os.path.relpath(orig, os.getcwd())}")

    nuevo.save(ruta)
    return True


def main():
    ap = argparse.ArgumentParser(description="Quita el fondo blanco de una textura PNG")
    ap.add_argument("rutas", nargs="+", help="PNG(s) a procesar")
    ap.add_argument("--umbral", type=int, default=238,
                    help="a partir de que valor un gris cuenta como blanco (0-255)")
    ap.add_argument("--dry-run", action="store_true", help="solo informar")
    args = ap.parse_args()

    print("QUITAR FONDO BLANCO")
    cambiados = 0
    for r in args.rutas:
        if not os.path.exists(r):
            print(f"  FALTA: {r}")
            continue
        if quitar_fondo(r, args.umbral, args.dry_run):
            cambiados += 1
    print(f"Listo: {cambiados} textura(s) modificada(s)")


if __name__ == "__main__":
    main()
