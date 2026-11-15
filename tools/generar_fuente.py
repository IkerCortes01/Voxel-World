#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
GENERAR LOS GLIFOS DE LA FUENTE DEL JUEGO (src/ui/FuenteBits.h)
===============================================================
PEDIDO: "agrega en todo Voxel World que puedas escribir con minusculas, poner
acentos, simbolos, signos y poner manual mayusculas y el @, y otros mas".

La fuente del juego esta dibujada a mano con segmentos y solo tiene A-Z, 0-9 y
diez signos. Para escribir de verdad faltan las minusculas, las vocales con
acento, la enie, la arroba y el resto de signos del teclado: 180 glifos. En
vez de dibujarlos uno a uno se sacan de una fuente con licencia libre
(Cascadia Mono, SIL Open Font License 1.1) y se guardan como MAPAS DE BITS en
una cabecera: el juego no lee ningun archivo de fuente ni depende de que este
instalada.

Cada glifo es una rejilla de 10 x 20:
    4 filas   por encima de las mayusculas (los acentos de las mayusculas)
   12 filas   la altura de una mayuscula
    4 filas   por debajo de la linea (el rabo de g, j, p, q, y)

Cubre ASCII (32-126) y Latin-1 (161-255): todo lo que da un teclado espanol.

    python tools/generar_fuente.py            # escribe src/ui/FuenteBits.h
    python tools/generar_fuente.py vista.png  # y ademas una vista para mirarla
"""
import os
import sys

from PIL import Image, ImageDraw, ImageFont

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SALIDA = os.path.join(RAIZ, "src", "ui", "FuenteBits.h")
ANCHO, ALTO = 10, 20
ARRIBA, MAYUSCULA = 4, 12
CANDIDATAS = [r"C:\Windows\Fonts\CascadiaMono.ttf", r"C:\Windows\Fonts\CascadiaCode.ttf"]
UMBRAL = 96            # de 255: por encima, el pixel esta encendido


def cargar():
    for ruta in CANDIDATAS:
        if not os.path.exists(ruta):
            continue
        # Se busca el tamano en el que la H mide MAYUSCULA filas de alto.
        for px in range(10, 40):
            f = ImageFont.truetype(ruta, px)
            try:
                f.set_variation_by_name("SemiBold")
            except Exception:
                pass
            caja = f.getbbox("H")
            if caja[3] - caja[1] >= MAYUSCULA:
                return f, ruta, px
    sys.exit("No esta Cascadia Mono en este equipo.")


def glifo(f, ch, base):
    """La rejilla de un caracter: lista de ALTO enteros, un bit por columna."""
    lienzo = Image.new("L", (ANCHO * 3, ALTO * 3), 0)
    d = ImageDraw.Draw(lienzo)
    avance = f.getlength(ch)
    # Centrado en su celda; la linea base, en la fila ARRIBA + MAYUSCULA.
    x = ANCHO + (ANCHO - avance) / 2.0
    d.text((x, ALTO + ARRIBA + MAYUSCULA - base), ch, font=f, fill=255)
    filas = []
    for y in range(ALTO):
        bits = 0
        for xx in range(ANCHO):
            if lienzo.getpixel((ANCHO + xx, ALTO + y)) >= UMBRAL:
                bits |= 1 << (ANCHO - 1 - xx)
        filas.append(bits)
    return filas


def main():
    f, ruta, px = cargar()
    base = f.getbbox("H")[3]           # donde cae la linea base al dibujar en y = 0
    codigos = list(range(33, 127)) + list(range(161, 256))
    tabla = {c: glifo(f, chr(c), base) for c in codigos}

    with open(SALIDA, "w", encoding="utf-8", newline="\n") as o:
        o.write("#pragma once\n\n#include <cstdint>\n\n")
        o.write("// ============================================================================\n")
        o.write("// LOS GLIFOS QUE LE FALTABAN A LA FUENTE DEL JUEGO\n")
        o.write("// ============================================================================\n")
        o.write("// GENERADO por tools/generar_fuente.py: no se edita a mano.\n")
        o.write("//\n")
        o.write("// Mapas de bits de 10 x 20 para ASCII (33-126) y Latin-1 (161-255):\n")
        o.write("// minusculas, acentos, enie, arroba y el resto de signos del teclado.\n")
        o.write("// 4 filas sobre la mayuscula, 12 de mayuscula y 4 bajo la linea.\n")
        o.write("// El bit 9 es la columna de la izquierda.\n")
        o.write("//\n")
        o.write("// Salen de Cascadia Mono (c) Microsoft Corporation, con licencia\n")
        o.write("// SIL Open Font License 1.1 (https://scripts.sil.org/OFL), que permite\n")
        o.write("// incluirla y redistribuirla asi.\n")
        o.write("// ============================================================================\n\n")
        o.write("namespace FuenteBits {\n\n")
        o.write("constexpr int ANCHO = %d, ALTO = %d, ARRIBA = %d, MAYUSCULA = %d;\n\n" %
                (ANCHO, ALTO, ARRIBA, MAYUSCULA))
        o.write("// Los glifos, por codigo (Unicode hasta 255). Los que no estan, a cero.\n")
        o.write("inline const uint16_t* glifo(unsigned codigo) {\n")
        o.write("    static const uint16_t G[256][ALTO] = {\n")
        for c in range(256):
            filas = tabla.get(c, [0] * ALTO)
            o.write("        { " + ",".join("0x%03x" % v for v in filas) + " },")
            o.write("  // %d%s\n" % (c, (" " + chr(c)) if c in tabla and c != 92 else ""))
        o.write("    };\n")
        o.write("    return G[codigo < 256 ? codigo : 0];\n")
        o.write("}\n\n")
        o.write("// ¿Tiene dibujo ese caracter?\n")
        o.write("inline bool tiene(unsigned codigo) {\n")
        o.write("    return (codigo >= 33 && codigo <= 126) || (codigo >= 161 && codigo <= 255);\n")
        o.write("}\n\n")
        o.write("}  // namespace FuenteBits\n")
    print("escrito %s (%s a %d px, %d glifos)" % (SALIDA, os.path.basename(ruta), px, len(tabla)))

    if len(sys.argv) > 1:
        muestra = ["ABCDEFGHIJKLM NOPQRSTUVWXYZ", "abcdefghijklm nopqrstuvwxyz", "0123456789 @#$%&*_=+-/\\|<>",
                   "áéíóúñü ÁÉÍÓÚÑÜ ¿Qué? ¡Sí!", "iker.cortes@voxelworld {[()]} ~^`\"';:,."]
        K = 4
        im = Image.new("RGB", (max(len(m) for m in muestra) * ANCHO * K + 20, len(muestra) * ALTO * K + 20), (16, 18, 28))
        px_ = im.load()
        for fila, linea in enumerate(muestra):
            for col, ch in enumerate(linea):
                g = tabla.get(ord(ch))
                if not g:
                    continue
                for y in range(ALTO):
                    for x in range(ANCHO):
                        if g[y] >> (ANCHO - 1 - x) & 1:
                            for dy in range(K):
                                for dx in range(K):
                                    px_[10 + (col * ANCHO + x) * K + dx, 10 + (fila * ALTO + y) * K + dy] = (235, 238, 245)
        im.save(sys.argv[1])
        print("vista en", sys.argv[1])
    return 0


if __name__ == "__main__":
    sys.exit(main())
