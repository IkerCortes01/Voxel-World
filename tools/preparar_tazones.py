"""Las imagenes de los tazones que faltan, hechas a partir de las que hay.

PEDIDO: "arregla que cada tablones de madera tenga su propio tazon".

Cada madera tiene tres tazones: vacio, con agua y con aguamiel. De casi todas
hay imagen del vacio y del de agua; este script hace las demas:

  - "... con aguamiel.png"  el de agua, con el agua azul vuelta aguamiel
                            (un jugo palido, entre blanco y ambar).
  - el de ENCINO con agua   no hay imagen: se le pone al tazon vacio de encino
                            el agua de otro tazon, ajustada a su boca.
  - el de PINONERO          copia con nombre sin enie (el motor no abre
                            archivos con enie).

No toca ninguna imagen existente: solo escribe las que no estan. Para rehacer
una, se borra y se vuelve a correr.
"""
import os
import shutil
import sys

from PIL import Image, ImageFilter

ITEMS = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'resourcepacks', 'Textures', 'Items')

# madera: (vacio, con agua)
TAZONES = {
    'pino':          ('tazon de madera de pino.png', 'Tazon de madera de pino con agua.png'),
    'encino':        ('tazon de madera de encino.png', 'Tazon de madera de encino con agua.png'),
    'oyamel':        ('Tazon Madera de Oyame.png', 'Tazon de madera de Oyame con agua.png'),
    'ocote':         ('Tazon de madera Pinus montezumae.png', 'Tazon de madera Pinus montezumae con agua.png'),
    'patula':        ('Tazon de Madera de Pino patula.png', 'Tazon de Madera de Pino patula con agua.png'),
    'pinonero':      ('Tazon Madera de Pino pinonero mexicano.png', 'Tazon Madera de Pino pinonero mexicano con agua.png'),
    'alturas':       ('Tazon Madera de Pino de las alturas.png', 'Tazon Madera de Pino de las alturas con agua.png'),
    'pseudostrobus': ('Tazon madera de Pino de pseudostrobus.png', 'Tazon madera de Pino de pseudostrobus con agua.png'),
}


def ruta(nombre):
    return os.path.join(ITEMS, nombre)


def de_aguamiel(nombre_agua):
    return nombre_agua.replace(' con agua.png', ' con aguamiel.png')


def mascara_de_agua(im):
    """Cuanto de azul (de agua) tiene cada pixel, de 0 a 255."""
    r, g, b, a = im.split()
    pr, pb, pa = r.load(), b.load(), a.load()
    m = Image.new('L', im.size, 0)
    pm = m.load()
    for y in range(im.height):
        for x in range(im.width):
            if pa[x, y] < 8:
                continue
            d = pb[x, y] - pr[x, y]
            if d > 20:
                pm[x, y] = min(255, (d - 20) * 255 // 50)
    return m


def a_aguamiel(agua):
    """El agua azul, vuelta aguamiel: mismo brillo y dibujo, otro color."""
    m = mascara_de_agua(agua)
    gris = agua.convert('L')
    lo, hi = gris.getextrema()
    pg, pm = gris.load(), m.load()
    sal = agua.copy()
    ps = sal.load()
    OSCURO, CLARO = (176, 146, 84), (250, 240, 204)
    # El brillo, estirado sobre lo que hay dentro del agua.
    vals = [pg[x, y] for y in range(0, agua.height, 4) for x in range(0, agua.width, 4) if pm[x, y] > 128]
    if vals:
        vals.sort()
        lo, hi = vals[len(vals) // 50], vals[-1 - len(vals) // 50]
    for y in range(agua.height):
        for x in range(agua.width):
            k = pm[x, y]
            if not k:
                continue
            t = min(1.0, max(0.0, (pg[x, y] - lo) / float(max(1, hi - lo))))
            r0, g0, b0, a0 = ps[x, y]
            n = [OSCURO[i] + (CLARO[i] - OSCURO[i]) * t for i in range(3)]
            f = k / 255.0
            ps[x, y] = (int(r0 + (n[0] - r0) * f), int(g0 + (n[1] - g0) * f), int(b0 + (n[2] - b0) * f), a0)
    return sal


def con_agua_de(vacio, donante):
    """El tazon vacio, con el agua del donante encajada en su figura."""
    cv = vacio.getchannel('A').point(lambda v: 255 if v > 24 else 0).getbbox()
    cd = donante.getchannel('A').point(lambda v: 255 if v > 24 else 0).getbbox()
    # El donante, estirado para que su tazon caiga encima del vacio.
    recorte = donante.crop(cd).resize((cv[2] - cv[0], cv[3] - cv[1]), Image.LANCZOS)
    lienzo = Image.new('RGBA', vacio.size, (0, 0, 0, 0))
    lienzo.paste(recorte, (cv[0], cv[1]))
    m = mascara_de_agua(lienzo).filter(ImageFilter.GaussianBlur(1.2))
    sal = vacio.copy()
    sal.paste(lienzo, (0, 0), m)
    sal.putalpha(vacio.getchannel('A'))
    return sal


def main():
    # El pinonero: copias sin enie.
    for llano in TAZONES['pinonero']:
        con_enie = llano.replace('pinonero', 'piñonero')
        if not os.path.exists(ruta(llano)) and os.path.exists(ruta(con_enie)):
            shutil.copyfile(ruta(con_enie), ruta(llano))
            print('copiado', llano)
    # El encino con agua: no hay imagen.
    vacio, agua = TAZONES['encino']
    if not os.path.exists(ruta(agua)):
        donante = Image.open(ruta(TAZONES['pino'][1])).convert('RGBA')
        con_agua_de(Image.open(ruta(vacio)).convert('RGBA'), donante).save(ruta(agua), optimize=True)
        print('hecho', agua)
    # Los de aguamiel.
    for madera, (vacio, agua) in TAZONES.items():
        for n in (vacio, agua):
            if not os.path.exists(ruta(n)):
                print('FALTA', n)
                return 1
        miel = de_aguamiel(agua)
        if not os.path.exists(ruta(miel)):
            a_aguamiel(Image.open(ruta(agua)).convert('RGBA')).save(ruta(miel), optimize=True)
            print('hecho', miel)
    return 0


if __name__ == '__main__':
    sys.exit(main())
