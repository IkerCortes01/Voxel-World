#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
QUITAR EL FONDO DE LAS TEXTURAS DE ITEM
=======================================
PEDIDO: "haz que todas las texturas de items les quites el fondo y solo sea el
item".

EL PROBLEMA, medido sobre resourcepacks/Textures/Items/:

    32 PNG estan en RGBA y ya tienen el fondo transparente   -> no se tocan
    14 PNG estan en RGB (SIN canal alfa) con fondo casi blanco

Esos 14 son los que se ven con un recuadro blanco alrededor del item en el
inventario y en la mano. No es un fallo del motor: el PNG no tiene ni un bit de
transparencia que el motor pueda respetar.

COMO SE DECIDE QUE ES FONDO
---------------------------
No vale "borra todo lo que sea casi blanco": un item con una zona clara --el
pedernal, la baba de nopal-- perderia su interior y quedaria agujereado.

Se usa RELLENO POR INUNDACION (flood fill) DESDE LOS BORDES:

  1. Se toma el color de las cuatro esquinas como referencia del fondo.
  2. Se inunda desde todo el perimetro hacia dentro, avanzando solo por
     pixeles parecidos a esa referencia.
  3. Lo que la inundacion NO alcanza es el ITEM, aunque sea del mismo color
     que el fondo -- porque esta rodeado de pixeles que no lo son.

Asi una mancha blanca DENTRO del item se conserva, y el fondo blanco de fuera
desaparece. Es la diferencia entre recortar y agujerear.

EL BORDE, QUE ES LO QUE SE NOTA
-------------------------------
Un recorte duro (alfa 0 o 255) deja el contorno dentado y con halo claro: los
pixeles del borde son una mezcla de item y fondo, y al quedarse opacos pintan
una linea blanca alrededor de la silueta.

Por eso el alfa es GRADUAL en la franja de transicion (ver TOL_SUAVE) y, ademas,
a los pixeles semitransparentes se les quita el tinte del fondo
(descontaminacion): sin eso, un borde verde sobre blanco se ve lavado.

SEGURIDAD
---------
  - Se hace COPIA DE SEGURIDAD de cada archivo en Items/_original/ antes de
    tocarlo. Si algo sale mal, se restaura con --restaurar.
  - --simular enseña lo que haria sin escribir nada.
  - Si un archivo acabara casi entero transparente (>97%), se DESCARTA el
    cambio y se avisa: eso significa que la heuristica se equivoco y es mejor
    dejar el original que borrar el item.
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

DIR_ITEMS = os.path.join("resourcepacks", "Textures", "Items")
DIR_COPIA = os.path.join(DIR_ITEMS, "_original")

# Distancia de color (0..255 por canal, suma de los tres) por debajo de la cual
# un pixel se considera fondo SEGURO. Generoso: los fondos reales son lisos.
TOL_DURO = 38

# Hasta donde llega la franja de transicion. Entre TOL_DURO y TOL_SUAVE el alfa
# baja de 0 a 255 de forma gradual, que es lo que quita el dentado del borde.
TOL_SUAVE = 110

# Si tras el recorte queda menos de esto de item, se considera que la
# heuristica fallo y no se escribe nada.
MIN_OPACO = 0.03


def dist(a, b):
    """Distancia de color entre dos RGB, suma de diferencias absolutas."""
    return abs(a[0] - b[0]) + abs(a[1] - b[1]) + abs(a[2] - b[2])


def color_de_fondo(px, w, h):
    """
    El color del fondo, tomado del PERIMETRO y no de una sola esquina.

    Una esquina suelta puede caer sobre una sombra o sobre el propio item si la
    imagen va justa. Se recorre todo el borde y se queda con el color mas
    repetido, que para un fondo liso es el fondo con diferencia.
    """
    cuenta = {}
    for x in range(w):
        for y in (0, h - 1):
            c = px[x, y][:3]
            cuenta[c] = cuenta.get(c, 0) + 1
    for y in range(h):
        for x in (0, w - 1):
            c = px[x, y][:3]
            cuenta[c] = cuenta.get(c, 0) + 1
    return max(cuenta.items(), key=lambda kv: kv[1])[0]


def quitar_fondo(ruta, simular=False):
    """
    Devuelve (cambiado, mensaje).
    """
    nombre = os.path.basename(ruta)
    im = Image.open(ruta)
    modo = im.mode
    im = im.convert("RGBA")
    w, h = im.size
    px = im.load()

    # Ya tiene transparencia util: no se toca.
    opacos = sum(1 for y in range(h) for x in range(w) if px[x, y][3] > 0)
    if opacos < w * h * 0.97:
        return False, f"      ya transparente ({opacos * 100 // (w * h)}% opaco)"

    fondo = color_de_fondo(px, w, h)

    # ------------------------------------------------------------------
    # INUNDACION DESDE EL PERIMETRO
    # ------------------------------------------------------------------
    # `nivel` guarda, por pixel, lo cerca que esta del color de fondo. Solo se
    # visitan pixeles alcanzables desde el borde: eso es lo que protege las
    # zonas claras del INTERIOR del item.
    es_fondo = bytearray(w * h)
    visto = bytearray(w * h)
    cola = deque()

    for x in range(w):
        for y in (0, h - 1):
            cola.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            cola.append((x, y))

    while cola:
        x, y = cola.popleft()
        i = y * w + x
        if visto[i]:
            continue
        visto[i] = 1
        if dist(px[x, y][:3], fondo) > TOL_SUAVE:
            continue              # aqui empieza el item: la inundacion para
        es_fondo[i] = 1
        if x > 0:     cola.append((x - 1, y))
        if x < w - 1: cola.append((x + 1, y))
        if y > 0:     cola.append((x, y - 1))
        if y < h - 1: cola.append((x, y + 1))

    # ------------------------------------------------------------------
    # ALFA GRADUAL + DESCONTAMINACION DEL BORDE
    # ------------------------------------------------------------------
    nuevo = Image.new("RGBA", (w, h))
    npx = nuevo.load()
    quedan = 0

    for y in range(h):
        for x in range(w):
            r, g, b, _ = px[x, y]
            if not es_fondo[y * w + x]:
                npx[x, y] = (r, g, b, 255)
                quedan += 1
                continue

            d = dist((r, g, b), fondo)
            if d <= TOL_DURO:
                npx[x, y] = (0, 0, 0, 0)          # fondo puro
                continue

            # Franja de transicion: alfa proporcional a lo lejos que esta del
            # fondo. Es lo que suaviza el contorno.
            a = int(255 * (d - TOL_DURO) / float(TOL_SUAVE - TOL_DURO))
            a = max(0, min(255, a))

            # DESCONTAMINAR: el pixel es una mezcla de item y fondo. Se despeja
            # el color del item invirtiendo esa mezcla. Sin esto queda un halo
            # claro alrededor de la silueta.
            if a > 0:
                f = a / 255.0
                rr = int(max(0, min(255, (r - fondo[0] * (1 - f)) / f)))
                gg = int(max(0, min(255, (g - fondo[1] * (1 - f)) / f)))
                bb = int(max(0, min(255, (b - fondo[2] * (1 - f)) / f)))
                npx[x, y] = (rr, gg, bb, a)
                quedan += 1
            else:
                npx[x, y] = (0, 0, 0, 0)

    frac = quedan / float(w * h)
    if frac < MIN_OPACO:
        return False, (f"      ⚠️ SE DESCARTA: quedaria solo el {frac*100:.1f}% "
                       f"del item. Original intacto.")

    msg = (f"      {modo} {w}x{h}  fondo={fondo}  "
           f"item restante={frac*100:.1f}%")
    if simular:
        return True, msg + "   (simulacion)"

    os.makedirs(DIR_COPIA, exist_ok=True)
    destino_copia = os.path.join(DIR_COPIA, nombre)
    if not os.path.exists(destino_copia):
        shutil.copy2(ruta, destino_copia)

    nuevo.save(ruta, "PNG")
    return True, msg


def restaurar():
    if not os.path.isdir(DIR_COPIA):
        sys.exit("No hay copias en " + DIR_COPIA)
    n = 0
    for nombre in os.listdir(DIR_COPIA):
        shutil.copy2(os.path.join(DIR_COPIA, nombre),
                     os.path.join(DIR_ITEMS, nombre))
        n += 1
    print(f"Restaurados {n} archivos desde {DIR_COPIA}")


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--simular", action="store_true",
                    help="enseña lo que haria sin escribir nada")
    ap.add_argument("--restaurar", action="store_true",
                    help="devuelve los originales desde _original/")
    args = ap.parse_args()

    if args.restaurar:
        restaurar()
        return

    if not os.path.isdir(DIR_ITEMS):
        sys.exit("No encuentro " + DIR_ITEMS + " (ejecutalo desde la raiz)")

    archivos = sorted(f for f in os.listdir(DIR_ITEMS)
                      if f.lower().endswith(".png"))
    cambiados = saltados = descartados = 0

    for nombre in archivos:
        ruta = os.path.join(DIR_ITEMS, nombre)
        try:
            ok, msg = quitar_fondo(ruta, args.simular)
        except Exception as e:                      # noqa: BLE001
            print(f"  ERROR  {nombre}: {e}")
            continue
        if ok:
            print(f"  RECORTADO  {nombre}")
            print(msg)
            cambiados += 1
        elif "DESCARTA" in msg:
            print(f"  DESCARTADO {nombre}")
            print(msg)
            descartados += 1
        else:
            saltados += 1

    print()
    print(f"  recortados: {cambiados}   ya estaban bien: {saltados}"
          f"   descartados: {descartados}")
    if cambiados and not args.simular:
        print(f"  copias de seguridad en: {DIR_COPIA}")
        print("  para deshacer:  python tools/quitar_fondo_items.py --restaurar")


if __name__ == "__main__":
    main()
