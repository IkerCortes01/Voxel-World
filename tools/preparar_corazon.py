"""
El corazon del HUD a partir de los cuadros de resourcepacks/HUD/corazon.mp4
(sacados con build/bin/Release/extraer_video.exe en una tira de 160 px).

    python tools/preparar_corazon.py tira_160.png

Deja resourcepacks/HUD/corazon_cuadros.png: una rejilla de 12 x 7 cuadros de
128 px con fondo transparente.

  - Solo los primeros 7 segundos (los 168 primeros cuadros a 24 fps), uno de
    cada dos: 84 cuadros, 12 por segundo.
  - El fondo (gris azulado oscuro del video) se vuelve transparente con un
    borde suave, para que el corazon flote sobre el juego.
  - Mas contraste y saturacion: a 70 px en pantalla el rojo del video se
    quedaba apagado.
  - El juego los recorre de ida y vuelta (84 + 84): un ciclo de 14 s sin
    salto. El LATIDO no sale del video: lo pone el juego, encogiendo e
    hinchando el corazon al pulso del jugador (ver render/HudVida.h).
"""
import os
import sys

import numpy as np
from PIL import Image, ImageFilter

LADO_IN, LADO, COLS, FILAS = 160, 128, 12, 7
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    tira = np.asarray(Image.open(sys.argv[1]).convert("RGB"), dtype=np.float64) / 255.0
    n_total = tira.shape[1] // LADO_IN
    cuadros = [tira[:, i * LADO_IN:(i + 1) * LADO_IN] for i in range(min(168, n_total))][::2]
    assert len(cuadros) == COLS * FILAS, len(cuadros)
    # El color del fondo: la media de los bordes de todos los cuadros.
    bordes = np.concatenate([np.concatenate([c[:4].reshape(-1, 3), c[-4:].reshape(-1, 3),
                                             c[:, :4].reshape(-1, 3), c[:, -4:].reshape(-1, 3)]) for c in cuadros])
    fondo = np.median(bordes, axis=0)
    hoja = np.zeros((LADO * FILAS, LADO * COLS, 4), dtype=np.uint8)
    for k, c in enumerate(cuadros):
        # Alfa: cuanto se aparta del fondo (con margen para el ruido y una
        # rampa suave), y que el corazon sea rojo ayuda: el fondo no lo es.
        d = np.linalg.norm(c - fondo, axis=-1)
        rojo = np.clip((c[..., 0] - np.maximum(c[..., 1], c[..., 2])) * 4.0, 0.0, 1.0)
        a = np.clip((d - 0.11) / 0.10, 0.0, 1.0)
        a = np.maximum(a, rojo * np.clip((d - 0.06) / 0.08, 0.0, 1.0))
        alfa = Image.fromarray((a * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8))
        # Contraste y saturacion, sin tocar lo transparente.
        gris = c.mean(-1, keepdims=True)
        col = np.clip((gris + (c - gris) * 1.35 - 0.5) * 1.25 + 0.55, 0.0, 1.0)
        rgb = Image.fromarray((col * 255).astype(np.uint8)).resize((LADO, LADO), Image.LANCZOS)
        alfa = alfa.resize((LADO, LADO), Image.LANCZOS)
        img = np.dstack([np.asarray(rgb), np.asarray(alfa)])
        x, y = (k % COLS) * LADO, (k // COLS) * LADO
        hoja[y:y + LADO, x:x + LADO] = img
    salida = os.path.join(RAIZ, "resourcepacks", "HUD", "corazon_cuadros.png")
    Image.fromarray(hoja, "RGBA").save(salida, optimize=True)
    print("fondo", fondo, "->", salida, hoja.shape)


if __name__ == "__main__":
    main()
