"""
Prepara las fases de la luna para el cielo.

Las imagenes de las fases (Objetos cosmicos/*.webp, *.jfif) traen el fondo
"transparente" de cuadritos IMPRESO, y la cara oscura de la luna en negro. En
el cielo, las dos cosas tienen que dejar ver el cielo:

  - lo iluminado es lo que se ve: opaco;
  - el tablero y la cara en sombra son "lo negro": transparentes.

Asi que la opacidad sale del BRILLO: por encima de lo mas claro del tablero,
opaco; por debajo, transparente, con una rampa suave (el terminador, la linea
entre la luz y la sombra, queda difuminado como en la luna de verdad).

Todas las fases se recortan con el MISMO encuadre que "Luna llena.png" (el
disco ocupa 1/1.10 del medio lado): al cambiar de fase la luna no salta de
tamano ni de sitio en el cielo.

Uso:  python tools/fases_lunares.py
"""
import os
import numpy as np
from PIL import Image, ImageFilter

DIR = os.path.join(os.path.dirname(__file__), "..", "resourcepacks", "Textures", "Blocks", "Objetos cosmicos")
LADO = 512
ENCUADRE = 1.10          # medio lado del cuadro / radio del disco (como Luna llena.png)

FASES = [
    ("Luna nueva.webp", "Luna nueva.png"),
    ("Creciente.jfif", "Creciente.png"),
    ("Cuarto creciente.webp", "Cuarto creciente.png"),
    ("Luna gibosa creciente.webp", "Luna gibosa creciente.png"),
    ("Luna gibosa menguante.webp", "Luna gibosa menguante.png"),
    ("Cuarto menguante.webp", "Cuarto menguante.png"),
    ("Luna menguante.webp", "Luna menguante.png"),
]


def preparar(origen, destino):
    img = np.asarray(Image.open(os.path.join(DIR, origen)).convert("RGB")).astype(np.float32)
    h, w, _ = img.shape
    lum = img.mean(axis=2)
    # El tablero: lo que hay en las cuatro esquinas.
    m = max(8, min(h, w) // 12)
    esquinas = np.concatenate([lum[:m, :m].ravel(), lum[:m, -m:].ravel(),
                               lum[-m:, :m].ravel(), lum[-m:, -m:].ravel()])
    claro = np.percentile(esquinas, 98)
    oscuro = np.percentile(esquinas, 2)

    # El disco: lo iluminado (mas claro que el tablero) y la cara en sombra
    # (mas negra que el tablero). Juntos dan la luna entera, sea cual sea la fase.
    disco = lum > claro + 15
    # Fuera los puntos sueltos: se queda lo que tiene vecinos.
    d = Image.fromarray((disco * 255).astype(np.uint8)).filter(ImageFilter.MedianFilter(7))
    disco = np.asarray(d) > 127
    ys, xs = np.nonzero(disco)
    if len(ys) == 0:
        raise RuntimeError("no se encontro la luna en " + origen)
    # El alto del disco siempre es el diametro (toda fase llega de arriba a
    # abajo). El centro en x sale del LIMBO, el borde exterior iluminado: si la
    # luz esta a la derecha, el centro queda un radio a la izquierda del borde
    # derecho, y al reves. Asi una creciente no se centra en el cuadro: queda
    # donde esta en el disco, y al cambiar de fase la luna no se mueve.
    radio = (ys.max() - ys.min()) / 2.0
    cy = (ys.min() + ys.max()) / 2.0
    medio_x = (xs.min() + xs.max()) / 2.0
    if xs.max() - xs.min() > 1.9 * radio:
        cx = medio_x
    elif xs.mean() > medio_x - 0.0 and xs.max() - xs.mean() < xs.mean() - xs.min():
        cx = xs.max() - radio
    else:
        cx = xs.min() + radio if xs.mean() < medio_x else xs.max() - radio

    # La opacidad, del brillo: lo iluminado opaco, lo negro transparente.
    alfa = np.clip((lum - (claro + 15.0)) / 70.0, 0.0, 1.0)
    # Y nada fuera del disco (restos del tablero).
    yy, xx = np.mgrid[0:h, 0:w]
    r = np.sqrt((yy - cy) ** 2 + (xx - cx) ** 2)
    alfa *= np.clip((radio * 1.01 - r) / (radio * 0.02), 0.0, 1.0)

    medio = int(radio * ENCUADRE)
    lienzo = np.zeros((2 * medio, 2 * medio, 4), np.float32)
    y0, x0 = int(cy) - medio, int(cx) - medio
    sy0, sx0 = max(0, y0), max(0, x0)
    sy1, sx1 = min(h, y0 + 2 * medio), min(w, x0 + 2 * medio)
    lienzo[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, :3] = img[sy0:sy1, sx0:sx1]
    lienzo[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, 3] = alfa[sy0:sy1, sx0:sx1] * 255.0
    salida = Image.fromarray(lienzo.astype(np.uint8), "RGBA").resize((LADO, LADO), Image.LANCZOS)
    salida.save(os.path.join(DIR, destino))
    print(destino, "centro", (int(cx), int(cy)), "radio", int(radio),
          "tablero", int(oscuro), "-", int(claro), "iluminado", round(float((alfa > 0.5).mean() * 100), 1), "%")


if __name__ == "__main__":
    for origen, destino in FASES:
        if os.path.exists(os.path.join(DIR, origen)):
            preparar(origen, destino)
        else:
            print("falta", origen)
