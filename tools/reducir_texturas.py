#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
REDUCIR LAS TEXTURAS HD AL TAMANO QUE EL MOTOR USA DE VERDAD
============================================================
PEDIDO: "que la pantalla de inicio tarde apenas 2 segundos en cargar".

EL PROBLEMA, medido en el arranque real:

    80 texturas de 1024 a 1536 px     205 MB en disco
    41 de ellas se cargan al arrancar (el resto, bajo demanda)

Cada PNG de 1254x1254 son 1.572.516 pixeles que hay que DESCOMPRIMIR enteros
antes de poder usarlos. Eso es lo que se come el arranque: no es el disco, es
el zlib del PNG mas la conversion a RGBA.

Y lo decisivo: el motor NO usa esa resolucion.

  - El array de texturas (que es por donde se dibuja el terreno a media y
    larga distancia) las REESCALA a 64x64 al entrar. Ver `LADO_CAPA` en
    main.cpp: "a la distancia en la que el motor funde chunks, un texel de 64
    ya cae por debajo de un pixel de pantalla".
  - El GL_TEXTURE_2D de cerca si usa la textura completa, y por eso NO se
    reduce a 64: se perderia detalle mirando un bloque a un metro.

De 1254 px a 4.096 pixeles utiles hay un factor de 384. Se descomprimen 384
pixeles para tirar 383.

POR QUE 256 Y NO 64 NI 16
-------------------------
256 px es el punto de equilibrio:

  - De cerca, un bloque ocupa como mucho ~500 px de pantalla. Con 256 de
    textura el detalle aguanta sin verse borroso.
  - Es 24 VECES menos pixeles que 1254: el coste de descompresion se desploma.
  - El array sigue reescalando a 64 igual que antes: ahi no cambia nada.

Bajar a 64 igualaria el array pero se notaria mirando un bloque de cerca, que
es justo lo que la nota de LADO_CAPA dice que hay que conservar.

SEGURIDAD
---------
  - COPIA DE SEGURIDAD de cada archivo en Textures/_hd_original/ antes de
    tocarlo, conservando la ruta relativa.
  - --simular enseña lo que haria sin escribir.
  - --restaurar devuelve los originales.
  - Las de 16 px NO se tocan: ya estan por debajo del objetivo.
  - Se conserva el canal alfa y se usa LANCZOS, que es el remuestreo que mejor
    preserva el borde en pixel art ampliado.
"""

import argparse
import os
import shutil
import sys

try:
    from PIL import Image
except ImportError:
    sys.exit("Hace falta Pillow:  pip install pillow")

RAIZ = os.path.join("resourcepacks", "Textures")
COPIA = os.path.join(RAIZ, "_hd_original")

# Lado maximo tras la reduccion. Ver la nota de arriba para el porque de 256.
OBJETIVO = 256

# Por debajo de esto no se toca: ya es pequeña.
UMBRAL = 320


def rutas():
    for base, dirs, files in os.walk(RAIZ):
        # No reprocesar las copias.
        dirs[:] = [d for d in dirs if d not in ("_hd_original", "_original")]
        for f in files:
            if f.lower().endswith((".png", ".gif")):
                yield os.path.join(base, f)


def reducir(ruta, simular):
    im = Image.open(ruta)

    # Los GIF animados (agua, horno) tienen varios cuadros: reducirlos con este
    # camino se cargaria la animacion. Se dejan como estan.
    if getattr(im, "n_frames", 1) > 1:
        return False, "animado (varios cuadros): no se toca"

    w, h = im.size
    if max(w, h) <= UMBRAL:
        return False, None

    escala = OBJETIVO / float(max(w, h))
    nw, nh = max(1, int(round(w * escala))), max(1, int(round(h * escala)))

    antes = os.path.getsize(ruta)
    if simular:
        return True, f"{w}x{h} -> {nw}x{nh}   ({antes/1048576:.1f} MB)"

    rel = os.path.relpath(ruta, RAIZ)
    dst = os.path.join(COPIA, rel)
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if not os.path.exists(dst):
        shutil.copy2(ruta, dst)

    # RGBA siempre: varias de estas ya llevan transparencia (las de item que se
    # recortaron) y convertir a RGB la perderia.
    out = im.convert("RGBA").resize((nw, nh), Image.LANCZOS)
    out.save(ruta, "PNG", optimize=True)

    despues = os.path.getsize(ruta)
    return True, (f"{w}x{h} -> {nw}x{nh}   "
                  f"{antes/1048576:.1f} MB -> {despues/1024:.0f} KB")


def restaurar():
    if not os.path.isdir(COPIA):
        sys.exit("No hay copias en " + COPIA)
    n = 0
    for base, _, files in os.walk(COPIA):
        for f in files:
            src = os.path.join(base, f)
            rel = os.path.relpath(src, COPIA)
            shutil.copy2(src, os.path.join(RAIZ, rel))
            n += 1
    print(f"Restaurados {n} archivos desde {COPIA}")


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--simular", action="store_true")
    ap.add_argument("--restaurar", action="store_true")
    ap.add_argument("--objetivo", type=int, default=OBJETIVO,
                    help=f"lado maximo en px (por defecto {OBJETIVO})")
    args = ap.parse_args()

    if args.restaurar:
        restaurar()
        return

    globals()["OBJETIVO"] = args.objetivo

    if not os.path.isdir(RAIZ):
        sys.exit("No encuentro " + RAIZ + " (ejecutalo desde la raiz)")

    tocados = 0
    ahorro_antes = ahorro_despues = 0
    for ruta in sorted(rutas()):
        antes = os.path.getsize(ruta)
        try:
            ok, msg = reducir(ruta, args.simular)
        except Exception as e:                       # noqa: BLE001
            print(f"  ERROR  {os.path.basename(ruta)}: {e}")
            continue
        if ok:
            print(f"  {os.path.relpath(ruta, RAIZ)}")
            print(f"      {msg}")
            tocados += 1
            ahorro_antes += antes
            if not args.simular:
                ahorro_despues += os.path.getsize(ruta)

    print()
    if args.simular:
        print(f"  se reducirian {tocados} texturas "
              f"({ahorro_antes/1048576:.0f} MB)")
    else:
        print(f"  reducidas {tocados} texturas: "
              f"{ahorro_antes/1048576:.0f} MB -> "
              f"{ahorro_despues/1048576:.1f} MB")
        print(f"  copias en: {COPIA}")
        print("  para deshacer:  python tools/reducir_texturas.py --restaurar")


if __name__ == "__main__":
    main()
