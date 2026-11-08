"""
Limpia las imagenes del sol y la luna para el cielo.

Las originales (Objetos cosmicos/*.jfif) son JPEG con el fondo "transparente"
de cuadritos IMPRESO en la imagen: no tienen canal alfa, y el tablero ni
siquiera es regular (las celdas cambian de tamano), asi que no sirve
"restar el tablero". Se separa el astro por lo que el tablero NO tiene:

  SOL   el tablero es gris (sin color) y nunca pasa de su gris claro. El
        brillo del sol es amarillo (tiene color) y el nucleo es mas blanco
        que ese gris. La opacidad sale de lo uno o lo otro.
  LUNA  el disco se rellena entero (tiene grises propios que se confundirian
        con el tablero) y el halo blanco cuenta solo por encima del gris
        claro del tablero.

Despues se recupera el color quitando el gris que habia debajo, se suaviza
la opacidad un pelo, se apaga lejos del astro (restos sueltos), se centra
en un cuadrado relleno de transparente (sin recortar, para no deformarlo) y
se guarda en PNG de 512.

Uso:  python tools/limpiar_astros.py
"""
import os
import numpy as np
from PIL import Image, ImageFilter

DIR = os.path.join(os.path.dirname(__file__), "..", "resourcepacks", "Textures", "Blocks", "Objetos cosmicos")
LADO = 512


def tablero(img):
    esquinas = np.concatenate([img[:120, :120].reshape(-1, 3), img[:120, -120:].reshape(-1, 3),
                               img[-120:, :120].reshape(-1, 3), img[-120:, -120:].reshape(-1, 3)])
    luz = esquinas.mean(axis=1)
    corte = (luz.min() + luz.max()) / 2.0
    return esquinas[luz >= corte].mean(axis=0), esquinas[luz < corte].mean(axis=0)


def limpiar(origen, destino, es_sol):
    img = np.asarray(Image.open(os.path.join(DIR, origen)).convert("RGB")).astype(np.float32)
    h, w, _ = img.shape
    claro, oscuro = tablero(img)
    tope = claro.max() + 6.0                       # lo mas claro que llega el tablero
    mx, mn = img.max(axis=2), img.min(axis=2)
    sat = mx - mn

    # El cuerpo del astro y su centro.
    cuerpo = (mn > 215) if es_sol else (img.mean(axis=2) > 175)
    ys, xs = np.nonzero(cuerpo)
    cy, cx = (ys.min() + ys.max()) / 2.0, (xs.min() + xs.max()) / 2.0
    radio = max(ys.max() - ys.min(), xs.max() - xs.min()) / 2.0
    yy, xx = np.mgrid[0:h, 0:w]
    r = np.sqrt((yy - cy) ** 2 + (xx - cx) ** 2)

    if es_sol:
        # Por anillos: en un anillo entero los cuadros claros y oscuros se
        # compensan y el fondo queda en su gris medio, constante. Asi el
        # perfil del sol (nucleo blanco, corona amarilla, resplandor) sale
        # limpio; los rayos dibujados se pierden, pero no queda tablero.
        alcance = 2.4
        medio_gris = (claro + oscuro) / 2.0
        luz_gris = medio_gris.mean()
        n = 256
        idx = np.clip((r / (radio * alcance) * n).astype(int), 0, n - 1)
        perfil = np.zeros((n, 3))
        for c in range(3):
            perfil[:, c] = np.bincount(idx.ravel(), img[..., c].ravel(), n) /                            np.maximum(np.bincount(idx.ravel(), None, n), 1)
        a_perfil = np.clip((perfil.mean(axis=1) - luz_gris) / (255.0 - luz_gris) * 1.25, 0.0, 1.0)
        a_perfil[0:int(n * 0.9 / alcance)] = np.maximum(a_perfil[0:int(n * 0.9 / alcance)], 1.0)
        c_perfil = np.clip((perfil - (1.0 - a_perfil[:, None]) * medio_gris) / np.maximum(a_perfil, 0.05)[:, None], 0, 255)
        alfa = a_perfil[idx]
        img = c_perfil[idx].astype(np.float32)
        claro = oscuro = np.zeros(3, np.float32)     # el color ya esta limpio
    else:
        # Solo el disco, con el borde suave: el halo lo pone el motor. El
        # borde, donde el brillo medio de cada anillo cae a medio camino
        # entre el disco y el fondo (el halo blanco no cuenta como luna).
        lum = img.mean(axis=2)
        n = 400
        idx = np.clip((r / (radio * 1.2) * n).astype(int), 0, n - 1)
        perfil = np.bincount(idx.ravel(), lum.ravel(), n) / np.maximum(np.bincount(idx.ravel(), None, n), 1)
        disco_l = perfil[: n // 4].mean()
        fondo_l = (claro.mean() + oscuro.mean()) / 2.0
        mitad = (disco_l + fondo_l) / 2.0
        borde = next(i for i in range(n // 4, n) if perfil[i] < mitad)
        radio = borde / n * radio * 1.2 * 0.985
        alfa = np.clip((radio - r) / (radio * 0.012), 0.0, 1.0)
        alcance = 1.05
    # Lejos del astro no hay nada: fuera restos del tablero.
    alfa *= np.clip((radio * alcance - r) / (radio * 0.25), 0.0, 1.0)

    # El color sin el gris de debajo (el del tablero mas parecido).
    d_claro = np.abs(img - claro).max(axis=2)
    d_oscuro = np.abs(img - oscuro).max(axis=2)
    fondo = np.where((d_claro <= d_oscuro)[..., None], claro, oscuro)
    a3 = np.maximum(alfa, 0.05)[..., None]
    color = np.clip((img - (1.0 - alfa[..., None]) * fondo) / a3, 0, 255)
    if es_sol:
        # El resplandor, hacia un blanco calido: el amarillo puro del original,
        # sobre el azul del cielo, se veia verdoso.
        calido = np.array([255.0, 236.0, 196.0], np.float32)
        borde = np.clip(1.0 - alfa, 0.0, 1.0)[..., None]
        color = img * (1.0 - 0.55 * borde) + calido * (0.55 * borde)

    # Un cuadrado centrado y relleno de transparente: nada se deforma.
    medio = int(radio * (alcance + 0.05))
    lienzo = np.zeros((2 * medio, 2 * medio, 4), np.float32)
    y0, x0 = int(cy) - medio, int(cx) - medio
    sy0, sx0 = max(0, y0), max(0, x0)
    sy1, sx1 = min(h, y0 + 2 * medio), min(w, x0 + 2 * medio)
    lienzo[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, :3] = color[sy0:sy1, sx0:sx1]
    lienzo[sy0 - y0:sy1 - y0, sx0 - x0:sx1 - x0, 3] = alfa[sy0:sy1, sx0:sx1] * 255.0
    salida = Image.fromarray(lienzo.astype(np.uint8), "RGBA").resize((LADO, LADO), Image.LANCZOS)
    # Un pelo de suavizado en la opacidad: borra el grano del JPEG.
    r_, g_, b_, a_ = salida.split()
    a_ = a_.filter(ImageFilter.GaussianBlur(1.2))
    Image.merge("RGBA", (r_, g_, b_, a_)).save(os.path.join(DIR, destino))
    print(destino, "centro", (int(cx), int(cy)), "radio", int(radio), "tablero", claro.round(), oscuro.round())


if __name__ == "__main__":
    limpiar("Sol total.jfif", "Sol.png", True)
    limpiar("Luna Llena.jfif", "Luna llena.png", False)
