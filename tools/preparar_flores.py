"""Deja listas las texturas de las flores y de las dos pencas nuevas.

Las imagenes llegan a mas de 1200 px. El juego las quiere a 256 (ver el commit
"Menos peso"): aqui se guardan las originales en _hd_original y se dejan las de
256 en su sitio.

  - Las FLORES (Textures/Items) se cuadran antes de reducir, con el pie del
    tallo abajo y centrado, para que la planta nazca del suelo y no flote.
  - Las PENCAS (Textures/Blocks) ya son cuadradas: solo se reducen.

Se puede volver a ejecutar: si la de _hd_original existe, parte de ella.

    python tools/preparar_flores.py
"""
import os
import shutil
from PIL import Image

RAIZ = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'resourcepacks', 'Textures')
LADO = 256
FLORES = ['Cempasuchil.png', 'Nochebuena.png', 'Cosmos.png', 'Dalia.png']
PENCAS = ['maguey sisal.png', 'Agave abisaii penca.png', 'Agave fourcroydes.png', 'Agave Americana.png']


def original(carpeta, nombre):
    """La imagen grande: la guardada, o la que hay (y entonces se guarda)."""
    aqui = os.path.join(RAIZ, carpeta, nombre)
    guardada = os.path.join(RAIZ, '_hd_original', carpeta, nombre)
    os.makedirs(os.path.dirname(guardada), exist_ok=True)
    if not os.path.exists(guardada) or Image.open(aqui).size[0] > LADO:
        if Image.open(aqui).size[0] > LADO:
            shutil.copyfile(aqui, guardada)
    return Image.open(guardada), aqui


for nombre in FLORES:
    im, destino = original('Items', nombre)
    im = im.convert('RGBA')
    # Solo la planta: fuera el margen transparente.
    caja = im.getchannel('A').point(lambda v: 255 if v > 24 else 0).getbbox()
    im = im.crop(caja)
    lado = max(im.size)
    lienzo = Image.new('RGBA', (lado, lado), (0, 0, 0, 0))
    # El pie del tallo, abajo y en el centro. El tallo es lo que toca la fila
    # de abajo: se centra por ahi, no por el ancho de la planta entera.
    fila = im.getchannel('A').crop((0, im.height - 6, im.width, im.height)).point(lambda v: 255 if v > 24 else 0)
    pie = fila.getbbox()
    centro_pie = (pie[0] + pie[2]) // 2 if pie else im.width // 2
    x = lado // 2 - centro_pie
    x = max(0, min(lado - im.width, x))
    lienzo.paste(im, (x, lado - im.height))
    chica = lienzo.resize((LADO, LADO), Image.LANCZOS)
    # Sin halo: lo casi transparente, transparente del todo.
    a = chica.getchannel('A').point(lambda v: 0 if v < 40 else v)
    chica.putalpha(a)
    chica.save(destino, optimize=True)
    print(nombre, 'planta', im.size, '-> lienzo', lado, 'pie en x =', round((x + centro_pie) / lado, 3))

for nombre in PENCAS:
    im, destino = original('Blocks', nombre)
    im.convert('RGBA').resize((LADO, LADO), Image.LANCZOS).save(destino, optimize=True)
    print(nombre, im.size, '->', LADO)
