#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PREPARAR LAS TEXTURAS DE LAS TUNAS
==================================
PEDIDO: "actualiza las texturas de items".

Las tunas roja y verde tienen foto nueva y las versiones "crecida" de las dos
se borraron: ahora cada una es UN archivo (Tuna roja.png, Tuna verde.png). Tal
como llegaron no servian:

  Tuna roja.png   1277 x 1231 y 2,1 MB. Fondo ya transparente, pero con un
                  fleco de pixeles rojos sueltos alrededor (restos del recorte).
                  -> el original se guarda en _hd_original/Items y la del juego
                     baja a 256 px de lado, como el resto (ver
                     reducir_texturas.py), sin el fleco.

  Tuna verde.png  256 x 247 con el fondo NEGRO y opaco: en la mano y en el
                  inventario salia dentro de un cuadro negro.
                  -> se le quita el fondo inundando desde los bordes (lo negro
                     que toca el borde es fondo; lo oscuro de DENTRO del fruto
                     se queda), con el contorno suavizado.
                     La de fondo negro se guarda en Items/_original.

Se puede volver a ejecutar: lo que ya esta hecho no se toca.
    python tools/preparar_tunas.py
"""
import os
import shutil
import sys
from collections import deque

from PIL import Image, ImageFilter

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ITEMS = os.path.join(RAIZ, "resourcepacks", "Textures", "Items")
HD = os.path.join(RAIZ, "resourcepacks", "Textures", "_hd_original", "Items")
ORIGINAL = os.path.join(ITEMS, "_original")
LADO = 256


def reducir(nombre):
    ruta = os.path.join(ITEMS, nombre)
    im = Image.open(ruta).convert("RGBA")
    if max(im.size) <= LADO:
        print("  %s: ya mide %dx%d" % (nombre, im.size[0], im.size[1]))
        return
    os.makedirs(HD, exist_ok=True)
    shutil.copy2(ruta, os.path.join(HD, nombre))
    # El fleco: pixeles casi transparentes o islas sueltas lejos del fruto. Se
    # quita lo que no aguanta una erosion (las islas de 1-3 px) y lo tenue.
    a = im.getchannel("A").point(lambda v: 255 if v >= 128 else 0)
    nucleo = a.filter(ImageFilter.MinFilter(7)).filter(ImageFilter.MaxFilter(9))
    a = Image.composite(im.getchannel("A"), Image.new("L", im.size, 0), nucleo)
    im.putalpha(a)
    # Reducir con el color premultiplicado: si no, el negro de los pixeles
    # transparentes se mezcla en el borde y deja un halo oscuro.
    k = LADO / float(max(im.size))
    tam = (max(1, round(im.size[0] * k)), max(1, round(im.size[1] * k)))
    pre = Image.merge("RGBa", im.convert("RGBa").split()).resize(tam, Image.LANCZOS)
    out = pre.convert("RGBA")
    al = out.getchannel("A").point(lambda v: 0 if v < 24 else v)
    out.putalpha(al)
    # CUADRADA: el modelo 3D del item estira la imagen al cuadro (ver
    # tests/test_items_sueltos.cpp); una de 256x247 saldria achatada. Se
    # rellena con transparente, centrada.
    if out.size[0] != out.size[1]:
        lado = max(out.size)
        lienzo = Image.new("RGBA", (lado, lado), (0, 0, 0, 0))
        lienzo.paste(out, ((lado - out.size[0]) // 2, (lado - out.size[1]) // 2))
        out = lienzo
    out.save(ruta, optimize=True)
    print("  %s: %dx%d -> %dx%d (original en _hd_original/Items)" %
          (nombre, im.size[0], im.size[1], tam[0], tam[1]))


def quitar_fondo_negro(nombre, tolerancia=34):
    ruta = os.path.join(ITEMS, nombre)
    im = Image.open(ruta).convert("RGBA")
    w, h = im.size
    px = im.load()
    if any(px[x, y][3] == 0 for x, y in ((0, 0), (w - 1, 0), (0, h - 1), (w - 1, h - 1))):
        print("  %s: ya tiene el fondo transparente" % nombre)
        return
    os.makedirs(ORIGINAL, exist_ok=True)
    if not os.path.exists(os.path.join(ORIGINAL, nombre)):
        shutil.copy2(ruta, os.path.join(ORIGINAL, nombre))

    def oscuro(x, y):
        r, g, b, _ = px[x, y]
        return max(r, g, b) <= tolerancia

    fondo = [[False] * w for _ in range(h)]
    cola = deque()
    for x in range(w):
        for y in (0, h - 1):
            if oscuro(x, y) and not fondo[y][x]:
                fondo[y][x] = True
                cola.append((x, y))
    for y in range(h):
        for x in (0, w - 1):
            if oscuro(x, y) and not fondo[y][x]:
                fondo[y][x] = True
                cola.append((x, y))
    while cola:
        x, y = cola.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            nx, ny = x + dx, y + dy
            if 0 <= nx < w and 0 <= ny < h and not fondo[ny][nx] and oscuro(nx, ny):
                fondo[ny][nx] = True
                cola.append((nx, ny))
    mascara = Image.new("L", (w, h), 255)
    mp = mascara.load()
    quitados = 0
    for y in range(h):
        for x in range(w):
            if fondo[y][x]:
                mp[x, y] = 0
                quitados += 1
    # El contorno, suave: medio pixel de desenfoque sobre la mascara, sin que
    # el fondo vuelva a asomar (el minimo con la mascara dura encogida 1 px).
    suave = mascara.filter(ImageFilter.GaussianBlur(0.8))
    dentro = mascara.filter(ImageFilter.MinFilter(3))
    final = Image.composite(Image.new("L", (w, h), 255), suave, dentro)
    final = Image.composite(final, Image.new("L", (w, h), 0), mascara)
    im.putalpha(final)
    im.save(ruta, optimize=True)
    print("  %s: fondo negro fuera (%d de %d pixeles; la anterior en Items/_original)" %
          (nombre, quitados, w * h))


def main():
    print("Tunas:")
    reducir("Tuna roja.png")
    quitar_fondo_negro("Tuna verde.png")
    return 0


if __name__ == "__main__":
    sys.exit(main())
