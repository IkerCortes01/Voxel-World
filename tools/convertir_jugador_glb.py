"""
Convierte un modelo GLB (por ejemplo, uno generado en Magnific / Tripo) en la
malla del jugador del juego: resourcepacks/Modelos/jugador.vwm.

    python tools/convertir_jugador_glb.py modelo.glb [salida.vwm]

Que hace:
  1. Lee la malla y la textura de color del GLB (sin librerias de 3D: solo
     numpy y Pillow).
  2. La pone en el espacio del juego: +Y arriba, +Z delante, +X derecha, de
     pies en y=0 y 1.80 de alto. El GLB de Tripo mira hacia +X y mide 1.
  3. La reduce agrupando vertices en celdas (mas finas en manos, cara y pies)
     de ~1 millon de triangulos a unos pocos miles: es lo que puede deformar y
     dibujar cada frame una Intel HD 4000.
  4. Pasa la textura a un color por vertice (el motor dibuja el cuerpo con
     color por vertice).
  5. Mide la pose (lo que bajan los brazos de la horizontal) y el largo de
     los brazos, para que el juego coloque su esqueleto dentro de la malla.

El esqueleto y los pesos de cada vertice NO van en el archivo: los calcula el
juego al cargarlo (player/MallaImportada.h) con su propio esqueleto, asi
siempre cuadran con las animaciones.

Formato .vwm (little endian):
    char[4]  "VWM1"
    uint32   numero de vertices, numero de triangulos
    float[16] parametros (ver PARAMS abajo)
    por vertice: float pos[3], normal[3], color[3]
    por triangulo: uint32[3]
"""
import io
import json
import struct
import sys

import numpy as np
from PIL import Image

PARAMS = ["abdBrazo", "abdPierna", "propTorso", "propBrazo", "propAntebrazo", "propMuslo",
          "propPierna", "propHombroY", "alto"]


def cargar(ruta):
    with open(ruta, "rb") as f:
        datos = f.read()
    if datos[:4] != b"glTF":
        raise SystemExit("no es un GLB")
    clen, _ = struct.unpack_from("<II", datos, 12)
    js = json.loads(datos[20:20 + clen])
    off = 20 + clen
    blen, _ = struct.unpack_from("<II", datos, off)
    binario = datos[off + 8: off + 8 + blen]

    def vista(i):
        bv = js["bufferViews"][i]
        o = bv.get("byteOffset", 0)
        return binario[o:o + bv["byteLength"]]

    def acc(i):
        a = js["accessors"][i]
        tipos = {5126: np.float32, 5125: np.uint32, 5123: np.uint16, 5121: np.uint8}
        comp = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4}[a["type"]]
        arr = np.frombuffer(vista(a["bufferView"]), dtype=tipos[a["componentType"]],
                            count=a["count"] * comp, offset=a.get("byteOffset", 0))
        return arr.reshape(-1, comp) if comp > 1 else arr

    prim = js["meshes"][0]["primitives"][0]
    pos = acc(prim["attributes"]["POSITION"]).astype(np.float64)
    uv = acc(prim["attributes"]["TEXCOORD_0"]).astype(np.float64)
    idx = acc(prim["indices"]).astype(np.int64).reshape(-1, 3)
    img = None
    mat = js["materials"][prim.get("material", 0)] if js.get("materials") else {}
    base = mat.get("pbrMetallicRoughness", {}).get("baseColorTexture")
    if base is not None:
        fuente = js["textures"][base["index"]]["source"]
        img = Image.open(io.BytesIO(vista(js["images"][fuente]["bufferView"]))).convert("RGB")
    return pos, uv, idx, img


def al_juego(pos):
    """Tripo: Y arriba, mira a +X, mide 1. Juego: mira a +Z, derecha +X."""
    alto = pos[:, 1].max() - pos[:, 1].min()
    k = 1.80 / alto
    p = np.empty_like(pos)
    p[:, 0] = -pos[:, 2] * k            # la derecha del personaje es -Z del GLB
    p[:, 1] = (pos[:, 1] - pos[:, 1].min()) * k
    p[:, 2] = pos[:, 0] * k
    # Centrado en X y Z sobre los pies.
    pies = p[:, 1] < 0.10
    p[:, 0] -= 0.5 * (p[pies, 0].min() + p[pies, 0].max())
    p[:, 2] -= np.median(p[pies, 2]) - 0.03
    return p


def reducir(p, col, idx):
    """Agrupa vertices en celdas: cada celda queda en un vertice (la media)."""
    x, y, z = p[:, 0], p[:, 1], p[:, 2]
    celda = np.full(len(p), 0.026)
    celda[(y > 1.48)] = 0.014                        # la cabeza y la cara
    celda[np.abs(x) > 0.62] = 0.0075                 # las manos (en cruz)
    celda[y < 0.11] = 0.012                          # los pies y los dedos
    nivel = np.round(np.log2(celda / 0.004)).astype(np.int64)
    q = np.floor(p / celda[:, None]).astype(np.int64)
    clave = (((nivel * 4096 + (q[:, 0] + 2048)) * 4096 + (q[:, 1] + 2048)) * 4096 + (q[:, 2] + 2048))
    unicas, grupo = np.unique(clave, return_inverse=True)
    n = len(unicas)
    cuenta = np.bincount(grupo, minlength=n).astype(np.float64)
    pr = np.stack([np.bincount(grupo, weights=p[:, k], minlength=n) for k in range(3)], 1) / cuenta[:, None]
    cr = np.stack([np.bincount(grupo, weights=col[:, k], minlength=n) for k in range(3)], 1) / cuenta[:, None]
    t = grupo[idx]
    vale = (t[:, 0] != t[:, 1]) & (t[:, 1] != t[:, 2]) & (t[:, 0] != t[:, 2])
    t = t[vale]
    # Sin triangulos repetidos (mismos tres vertices en cualquier orden).
    orden = np.sort(t, axis=1)
    _, primero = np.unique(orden, axis=0, return_index=True)
    t = t[np.sort(primero)]
    # Solo los vertices usados.
    usados = np.unique(t)
    nuevo = -np.ones(n, dtype=np.int64)
    nuevo[usados] = np.arange(len(usados))
    return pr[usados], cr[usados], nuevo[t]


def normales(p, t):
    a, b, c = p[t[:, 0]], p[t[:, 1]], p[t[:, 2]]
    fn = np.cross(b - a, c - a)
    n = np.zeros_like(p)
    for k in range(3):
        np.add.at(n, t[:, k], fn)
    l = np.linalg.norm(n, axis=1)
    l[l < 1e-12] = 1.0
    return n / l[:, None]


def medir(p):
    """La pose y los largos, para colocar el esqueleto del juego."""
    x, y = p[:, 0], p[:, 1]
    # Los brazos en cruz: la linea media del brazo derecho entre 0.35 y 0.60
    # de X. Lo que baja respecto a la horizontal da la abduccion.
    xs, ys = [], []
    for xx in np.linspace(0.32, 0.60, 8):
        m = (x > xx - 0.01) & (x < xx + 0.01) & (y > 1.0)
        if m.sum() > 10:
            xs.append(xx)
            ys.append(0.5 * (y[m].min() + y[m].max()))
    pend = np.polyfit(xs, ys, 1)[0]
    abd = np.pi / 2 + np.arctan(pend)          # pendiente negativa: el brazo baja
    hombroY = np.polyval(np.polyfit(xs, ys, 1), 0.20)
    alcance = x.max()                           # punta de los dedos
    # La entrepierna: lo mas alto donde aun hay hueco entre las piernas.
    # Hay hueco si hay pierna a los dos lados y nada en el centro.
    entre = 0.0
    for yy in np.linspace(0.40, 1.10, 141):
        m = (y > yy - 0.004) & (y < yy + 0.004)
        xm = x[m]
        if (np.abs(xm) < 0.012).sum() == 0 and ((xm > 0.03) & (xm < 0.2)).any() and ((xm < -0.03) & (xm > -0.2)).any():
            entre = yy
    return abd, hombroY, alcance, entre


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    salida = sys.argv[2] if len(sys.argv) > 2 else "resourcepacks/Modelos/jugador.vwm"
    pos, uv, idx, img = cargar(sys.argv[1])
    print("GLB:", len(pos), "vertices,", len(idx), "triangulos")
    p = al_juego(pos)
    if img is not None:
        w, h = img.size
        tex = np.asarray(img, dtype=np.float64) / 255.0
        u = np.clip((uv[:, 0] % 1.0) * (w - 1), 0, w - 1).astype(np.int64)
        v = np.clip((uv[:, 1] % 1.0) * (h - 1), 0, h - 1).astype(np.int64)
        col = tex[v, u]
    else:
        col = np.full((len(p), 3), 0.8)
    pr, cr, t = reducir(p, col, idx)
    nr = normales(pr, t)
    abd, hombroY, alcance, entre = medir(p)      # sobre la malla completa: mas fiel
    # Largos del esqueleto del juego (ModeloJugador::M) para encajar en la malla:
    # brazo + antebrazo + mano llegan a la punta de los dedos.
    X_HOMBRO, BRAZO, ANTEBRAZO, MANO = 0.20, 0.30, 0.26, 0.185
    largoBrazo = (alcance - X_HOMBRO) / np.sin(abd) - MANO
    kBrazo = largoBrazo / (BRAZO + ANTEBRAZO)
    # La cadera del juego esta a 0.94; la articulacion queda ~0.06 bajo la
    # entrepierna medida... por encima: la cabeza del femur esta por encima.
    # Con short la tela une las piernas y la entrepierna medida sale baja: si
    # no es creible (una persona de 1.80 la tiene entre 0.78 y 0.92) se usa la
    # de las medidas medias, que es la del esqueleto.
    caderaY = entre + 0.07 if 0.78 <= entre <= 0.92 else 0.94
    kPierna = (caderaY - 0.07) / (0.94 - 0.07)
    PELVIS, HOMBRO = 0.96, 0.48
    kHombroY = (hombroY - (caderaY + 0.02)) / HOMBRO
    print("  medido: hombroY=%.3f alcance=%.3f entrepierna=%.3f" % (hombroY, alcance, entre))
    params = [abd, 0.03, 1.0, kBrazo, kBrazo, kPierna, kPierna, kHombroY, 1.80]
    for n, v in zip(PARAMS, params):
        print("  %-14s %.4f" % (n, v))
    print("Salida:", len(pr), "vertices,", len(t), "triangulos ->", salida)
    import os
    os.makedirs(os.path.dirname(salida) or ".", exist_ok=True)
    with open(salida, "wb") as f:
        f.write(b"VWM1")
        f.write(struct.pack("<II", len(pr), len(t)))
        f.write(struct.pack("<16f", *(params + [0.0] * (16 - len(params)))))
        f.write(np.hstack([pr, nr, cr]).astype("<f4").tobytes())
        f.write(t.astype("<u4").tobytes())


if __name__ == "__main__":
    main()
