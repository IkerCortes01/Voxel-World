#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GENERA EL ICONO DEL JUEGO: UN BLOQUE DE PASTO ISOMETRICO
========================================================

Que hace
--------
Toma las texturas REALES del pasto del resourcepack y las proyecta sobre las
tres caras visibles de un cubo en vista isometrica -- la misma silueta del
icono de Minecraft, pero con la textura de este juego.

Produce:
  - resourcepacks/Textures/UI/icono.png   (512x512, para la ventana GLFW)
  - VoxelWorld.ico                        (multi-resolucion, para el .exe)

Por que se genera y no se dibuja a mano
---------------------------------------
Porque asi el icono ES el bloque. Si manana se cambia la textura del pasto,
se vuelve a correr esto y el icono la sigue. Es el mismo criterio que ya usa
el motor para el icono del pecari en el inventario: se genera de su modelo en
vez de guardar un PNG aparte, para que no puedan divergir.

La proyeccion
-------------
Isometrica de verdad (2:1, la de los videojuegos), no una isometrica
matematica de 30 grados. Cada cara del cubo es un paralelogramo:

        superior:  rombo, la cara que se ve desde arriba
        izquierda: se hunde hacia la derecha
        derecha:   se hunde hacia la izquierda

Se mapea cada textura con una transformacion afin (PIL.Image.transform con
Image.AFFINE por triangulos no existe, asi que se usa QUAD, que hace
exactamente esto: mapear la imagen origen a un cuadrilatero destino).

El sombreado
------------
Las tres caras llevan el mismo multiplicador que usa el mesher del motor para
que el icono se lea igual que el bloque en el mundo:

    superior  1.00   (recibe el cielo de lleno)
    izquierda 0.80   (la cara N/S del motor)
    derecha   0.60   (la cara E/W, la mas oscura)

Sin esa diferencia las tres caras se funden en una mancha y el cubo deja de
leerse como volumen.

Uso
---
    python tools/generar_icono.py
"""

import sys
from pathlib import Path

try:
    from PIL import Image, ImageEnhance
except ImportError:
    print("ERROR: falta Pillow.  pip install Pillow")
    sys.exit(1)

RAIZ = Path(__file__).resolve().parent.parent
BLOQUES = RAIZ / "resourcepacks" / "Textures" / "Blocks"
UI = RAIZ / "resourcepacks" / "Textures" / "UI"

# Las dos texturas del bloque de pasto. La de arriba es la cara verde; la de
# costado lleva la franja de hierba sobre la tierra.
TEX_ARRIBA = "Bloque de Pasto UP.png"
TEX_LADO = "Bloque de pasto.png"

# Lienzo de trabajo. Se genera grande y se reduce al final: el suavizado de
# los bordes del rombo sale mucho mejor reduciendo que dibujando al tamano
# final con antialias.
LADO = 1024
SS = 2                      # supermuestreo
W = LADO * SS

# Sombreado por cara, copiado del mesher (faceBrightness).
BRILLO_ARRIBA = 1.00
BRILLO_IZQ = 0.80
BRILLO_DER = 0.60


def cargar(nombre: str) -> Image.Image:
    ruta = BLOQUES / nombre
    if not ruta.exists():
        print(f"ERROR: no existe {ruta}")
        sys.exit(1)
    return Image.open(ruta).convert("RGBA")


def sombrear(img: Image.Image, factor: float) -> Image.Image:
    """Multiplica el brillo conservando el alfa."""
    if factor >= 0.999:
        return img
    r, g, b, a = img.split()
    rgb = Image.merge("RGB", (r, g, b))
    rgb = ImageEnhance.Brightness(rgb).enhance(factor)
    r2, g2, b2 = rgb.split()
    return Image.merge("RGBA", (r2, g2, b2, a))


def proyectar(textura: Image.Image, destino: list, tam: int) -> Image.Image:
    """Mapea `textura` al cuadrilatero `destino` sobre un lienzo tam x tam.

    `destino` son las 4 esquinas en orden: sup-izq, sup-der, inf-der, inf-izq.

    PIL.Image.transform con QUAD va AL REVES de lo que uno espera: los datos
    que se le pasan son las esquinas del cuadrilatero ORIGEN que hay que
    estirar hasta llenar el destino rectangular. Asi que para mapear una
    textura a un rombo hay que invertir el planteamiento: se crea el lienzo
    del tamano final y se pide la transformacion inversa.

    Se resuelve con la transformacion de perspectiva, que si acepta el sentido
    natural (origen -> destino) resolviendo el sistema de 8 incognitas.
    """
    import numpy as np

    ancho, alto = textura.size
    origen = [(0, 0), (ancho, 0), (ancho, alto), (0, alto)]

    # Coeficientes de la homografia que lleva `destino` a `origen`.
    # (PIL aplica la inversa: para cada pixel del destino busca su origen.)
    A = []
    B = []
    for (xd, yd), (xo, yo) in zip(destino, origen):
        A.append([xd, yd, 1, 0, 0, 0, -xo * xd, -xo * yd])
        B.append(xo)
        A.append([0, 0, 0, xd, yd, 1, -yo * xd, -yo * yd])
        B.append(yo)

    coef = np.linalg.solve(np.array(A, dtype=float), np.array(B, dtype=float))

    lienzo = textura.transform(
        (tam, tam), Image.Transform.PERSPECTIVE, coef,
        resample=Image.Resampling.BICUBIC,
    )
    return lienzo


def recortar(img: Image.Image, poligono: list, tam: int) -> Image.Image:
    """Deja solo lo que cae dentro del poligono (el resto, transparente)."""
    from PIL import ImageDraw
    mascara = Image.new("L", (tam, tam), 0)
    ImageDraw.Draw(mascara).polygon(poligono, fill=255)
    salida = img.copy()
    # El recorte se AND-ea con el alfa que ya trae la textura.
    alfa = salida.split()[3]
    nueva = Image.new("L", (tam, tam), 0)
    nueva.paste(alfa, (0, 0), mascara)
    salida.putalpha(nueva)
    return salida


def main() -> int:
    arriba = cargar(TEX_ARRIBA)
    lado = cargar(TEX_LADO)

    # ------------------------------------------------------------------
    # LA GEOMETRIA DEL CUBO ISOMETRICO
    # ------------------------------------------------------------------
    # Proyeccion 2:1. El cubo se inscribe en el lienzo con un margen para
    # que no toque el borde (los iconos de Windows se ven mejor con aire).
    #
    #            A
    #          /   \
    #        B       C        A,B,C,D = cara superior (rombo)
    #          \   /  \
    #            D      \     B,D,F,E = cara izquierda
    #            |  \    C    D,C,G,F = cara derecha
    #            E    \  |
    #              \    G
    #                F
    margen = int(W * 0.06)
    cx = W // 2
    ancho = W - margen * 2
    mitad = ancho // 2
    # Altura del rombo superior: la mitad del ancho (proyeccion 2:1).
    cuarto = mitad // 2
    # Altura de los costados. 0.5 del ancho deja el cubo con la proporcion
    # clasica: tan alto como ancho contando el rombo de arriba.
    alto_lado = int(ancho * 0.5)

    y0 = margen                       # vertice superior del rombo
    y1 = y0 + cuarto                  # izq/der del rombo
    y2 = y0 + cuarto * 2              # vertice inferior del rombo
    y3 = y1 + alto_lado               # base de los costados
    y4 = y2 + alto_lado               # punta inferior del cubo

    A = (cx, y0)
    B = (cx - mitad, y1)
    C = (cx + mitad, y1)
    D = (cx, y2)
    E = (cx - mitad, y3)
    F = (cx, y4)
    G = (cx + mitad, y3)

    capas = []

    # --- CARA SUPERIOR: el rombo A-C-D-B ---
    # Orden de esquinas: sup-izq=B, sup-der=A, inf-der=C, inf-izq=D
    # (asi la textura queda con su "arriba" hacia el fondo de la escena).
    poli_sup = [A, C, D, B]
    img = proyectar(sombrear(arriba, BRILLO_ARRIBA), [B, A, C, D], W)
    capas.append(recortar(img, poli_sup, W))

    # --- CARA IZQUIERDA: B-D-F-E ---
    poli_izq = [B, D, F, E]
    img = proyectar(sombrear(lado, BRILLO_IZQ), [B, D, F, E], W)
    capas.append(recortar(img, poli_izq, W))

    # --- CARA DERECHA: D-C-G-F ---
    poli_der = [D, C, G, F]
    img = proyectar(sombrear(lado, BRILLO_DER), [D, C, G, F], W)
    capas.append(recortar(img, poli_der, W))

    # Composicion.
    icono = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    for c in capas:
        icono = Image.alpha_composite(icono, c)

    # Al tamano final, con el suavizado que da reducir desde el doble.
    icono = icono.resize((LADO, LADO), Image.Resampling.LANCZOS)

    UI.mkdir(parents=True, exist_ok=True)
    png = UI / "icono.png"
    icono.save(png, "PNG", optimize=True)
    print(f"  {png.relative_to(RAIZ)}  ({LADO}x{LADO})")

    # El .ico para el ejecutable, con todas las medidas que pide Windows.
    ico = RAIZ / "VoxelWorld.ico"
    medidas = [(s, s) for s in (16, 24, 32, 48, 64, 128, 256)]
    icono.save(ico, "ICO", sizes=medidas)
    print(f"  {ico.relative_to(RAIZ)}  ({', '.join(str(s[0]) for s in medidas)})")

    return 0


if __name__ == "__main__":
    sys.exit(main())
