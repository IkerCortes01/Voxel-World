"""
Convierte un modelo GLB (por ejemplo, uno generado en Magnific / Tripo) en el
modelo del jugador del juego.

    python tools/convertir_jugador_glb.py modelo.glb [triangulos]

Deja en resourcepacks/Modelos/:
    jugador.vwm           la malla (VWM2): posicion, normal, uv y tangente
    jugador_color.jpg     la textura de color (2048)
    jugador_normal.png    el mapa de normales (2048): el relieve fino de la
                          ropa, la piel y el pelo sin gastar triangulos
    jugador_mascara.png   que es cada texel: R piel, G pelo, B camisa, A short
    jugador_rugosidad.jpg cuanto brilla cada texel (la piel algo, la tela casi nada)
                          (el avatar recolorea cada zona con sus genes y ropa)

Que hace:
  1. Lee la malla y las texturas del GLB (solo numpy y Pillow).
  2. La pone en el espacio del juego: +Y arriba, +Z delante, +X derecha, de
     pies en y=0 y 1.80 de alto. El GLB de Tripo mira hacia +X y mide 1.
  3. La reduce con tools/decimar_malla (colapso por cuadricas que respeta las
     costuras UV) de ~1 millon de triangulos a los pedidos (60.000 por
     defecto): la textura se sigue viendo igual que en el original.
  4. Mide la pose (lo que bajan los brazos) y el largo de los brazos y los
     hombros, para que el juego meta su esqueleto dentro de la malla.

El esqueleto y los pesos NO van en el archivo: los calcula el juego al
cargarlo (player/MallaImportada.h) con su propio esqueleto.

Formato .vwm VWM2 (little endian):
    char[4]  "VWM2"
    uint32   vertices, triangulos
    float[32] parametros (PARAMS)
    por vertice: float pos[3], normal[3], uv[2], tangente[4]
    por triangulo: uint32[3]
"""
import io
import json
import os
import struct
import subprocess
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

PARAMS = ["abdBrazo", "abdPierna", "propTorso", "propBrazo", "propAntebrazo", "propMuslo",
          "propPierna", "propHombroY", "alto",
          "pielR", "pielG", "pielB", "peloR", "peloG", "peloB",
          "camisaR", "camisaG", "camisaB", "shortR", "shortG", "shortB", "propMano"]
RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TEX = 2048
MASCARA = 1024


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

    def imagen(tex):
        if tex is None:
            return None
        fuente = js["textures"][tex["index"]]["source"]
        return Image.open(io.BytesIO(vista(js["images"][fuente]["bufferView"]))).convert("RGB")

    prim = js["meshes"][0]["primitives"][0]
    pos = acc(prim["attributes"]["POSITION"]).astype(np.float64)
    nor = acc(prim["attributes"]["NORMAL"]).astype(np.float64)
    uv = acc(prim["attributes"]["TEXCOORD_0"]).astype(np.float64)
    idx = acc(prim["indices"]).astype(np.int64).reshape(-1, 3)
    mat = js["materials"][prim.get("material", 0)] if js.get("materials") else {}
    color = imagen(mat.get("pbrMetallicRoughness", {}).get("baseColorTexture"))
    normal = imagen(mat.get("normalTexture"))
    rugosidad = imagen(mat.get("pbrMetallicRoughness", {}).get("metallicRoughnessTexture"))
    return pos, nor, uv, idx, color, normal, rugosidad


def al_juego(pos, nor):
    """Tripo: Y arriba, mira a +X, mide 1. Juego: mira a +Z, derecha +X."""
    alto = pos[:, 1].max() - pos[:, 1].min()
    k = 1.80 / alto
    p = np.empty_like(pos)
    p[:, 0] = -pos[:, 2] * k            # la derecha del personaje es -Z del GLB
    p[:, 1] = (pos[:, 1] - pos[:, 1].min()) * k
    p[:, 2] = pos[:, 0] * k
    n = np.stack([-nor[:, 2], nor[:, 1], nor[:, 0]], 1)
    pies = p[:, 1] < 0.10
    p[:, 0] -= 0.5 * (p[pies, 0].min() + p[pies, 0].max())
    p[:, 2] -= np.median(p[pies, 2]) - 0.03
    return p, n


def decimar(p, n, uv, idx, objetivo, temp):
    exe = os.path.join(RAIZ, "build", "bin", "Release", "decimar_malla.exe")
    if not os.path.exists(exe):
        raise SystemExit("falta %s: compila el objetivo decimar_malla" % exe)
    ent, sal = temp + "_entrada.raw", temp + "_salida.raw"
    with open(ent, "wb") as f:
        f.write(struct.pack("<II", len(p), len(idx)))
        f.write(np.hstack([p, n, uv]).astype("<f4").tobytes())
        f.write(idx.astype("<u4").tobytes())
    subprocess.run([exe, ent, sal, str(objetivo)], check=True)
    with open(sal, "rb") as f:
        d = f.read()
    nv, nt = struct.unpack_from("<II", d, 0)
    v = np.frombuffer(d, "<f4", nv * 8, 8).reshape(nv, 8).astype(np.float64)
    t = np.frombuffer(d, "<u4", nt * 3, 8 + nv * 32).reshape(nt, 3).astype(np.int64)
    os.remove(ent)
    os.remove(sal)
    return v[:, :3], v[:, 3:6], v[:, 6:8], t


def tangentes(p, n, uv, t):
    """Las tangentes para el mapa de normales (Lengyel), con su signo en w."""
    a, b, c = p[t[:, 0]], p[t[:, 1]], p[t[:, 2]]
    ua, ub, uc = uv[t[:, 0]], uv[t[:, 1]], uv[t[:, 2]]
    e1, e2 = b - a, c - a
    d1, d2 = ub - ua, uc - ua
    r = d1[:, 0] * d2[:, 1] - d2[:, 0] * d1[:, 1]
    r = np.where(np.abs(r) < 1e-12, 1e-12, r)
    s = (e1 * d2[:, 1:2] - e2 * d1[:, 1:2]) / r[:, None]
    tt = (e2 * d1[:, 0:1] - e1 * d2[:, 0:1]) / r[:, None]
    T = np.zeros_like(p)
    B = np.zeros_like(p)
    for k in range(3):
        np.add.at(T, t[:, k], s)
        np.add.at(B, t[:, k], tt)
    T = T - n * np.sum(n * T, 1)[:, None]
    l = np.linalg.norm(T, axis=1)
    T = np.where(l[:, None] > 1e-12, T / np.maximum(l, 1e-12)[:, None], np.array([1.0, 0, 0]))
    w = np.where(np.sum(np.cross(n, T) * B, 1) < 0.0, -1.0, 1.0)
    return np.hstack([T, w[:, None]])


def medir(p):
    """La pose y los largos, para colocar el esqueleto del juego."""
    x, y = p[:, 0], p[:, 1]
    xs, ys = [], []
    for xx in np.linspace(0.32, 0.60, 8):
        m = (x > xx - 0.01) & (x < xx + 0.01) & (y > 1.0)
        if m.sum() > 10:
            xs.append(xx)
            ys.append(0.5 * (y[m].min() + y[m].max()))
    recta = np.polyfit(xs, ys, 1)
    abd = np.pi / 2 + np.arctan(recta[0])
    hombroY = np.polyval(recta, 0.20)
    alcance = x.max()
    entre = 0.0
    for yy in np.linspace(0.40, 1.10, 141):
        m = (y > yy - 0.004) & (y < yy + 0.004)
        xm = x[m]
        if (np.abs(xm) < 0.012).sum() == 0 and ((xm > 0.03) & (xm < 0.2)).any() and ((xm < -0.03) & (xm > -0.2)).any():
            entre = yy
    return abd, hombroY, alcance, entre


def mascara(p, uv, t, color):
    """Que es cada texel: piel, pelo, camisa o short. Primero la zona del
    cuerpo de cada triangulo (por donde cae en la pose en cruz) pintada en el
    espacio de la textura; luego el color decide dentro de la zona."""
    zona = Image.new("L", (MASCARA, MASCARA), 0)
    dib = ImageDraw.Draw(zona)
    c = p[t].mean(1)
    # 1 cabeza, 2 torso y brazos, 3 cadera y muslos, 4 piernas, pies y manos,
    # 5 la franja donde la camisa cae sobre el short (la decide el color)
    z = np.where(c[:, 1] > 1.53, 1, np.where(c[:, 1] > 1.05, 2, np.where(c[:, 1] > 0.82, 5, np.where(c[:, 1] > 0.50, 3, 4))))
    z = np.where((np.abs(c[:, 0]) > 0.62) & (c[:, 1] > 1.1), 4, z)
    # 6 los ojos: entre la nariz y las cejas, por delante de la cara. Son
    # oscuros como el pelo, pero no se tinen con el.
    cab = p[:, 1] > 1.53
    nariz = p[cab][np.argmax(p[cab, 2])]
    ojos = (c[:, 1] > nariz[1] + 0.012) & (c[:, 1] < nariz[1] + 0.050) & (c[:, 2] > nariz[2] - 0.055) & (np.abs(c[:, 0] - nariz[0]) < 0.065)
    z = np.where(ojos, 6, z)
    puv = uv[t] * MASCARA
    for k in range(len(t)):
        q = puv[k]
        dib.polygon([(q[0, 0], q[0, 1]), (q[1, 0], q[1, 1]), (q[2, 0], q[2, 1])], fill=int(z[k]))
    zona = np.asarray(zona.filter(ImageFilter.MaxFilter(5)))
    col = np.asarray(color.resize((MASCARA, MASCARA), Image.LANCZOS), dtype=np.float64) / 255.0
    r, g, b = col[..., 0], col[..., 1], col[..., 2]
    mx, mn = col.max(-1), col.min(-1)
    sat = (mx - mn) / np.maximum(mx, 1e-6)
    tono = np.degrees(np.arctan2(np.sqrt(3) * (g - b), 2 * r - g - b)) % 360
    piel = ((tono < 45) | (tono > 340)) & (sat > 0.18) & (sat < 0.80) & (mx > 0.16) & (r > b + 0.05)
    # En la ropa, los pliegues calidos de la tela se parecen a la piel: ahi la
    # piel de verdad (brazos, cuello) se distingue por estar mas saturada.
    vestido = (zona == 2) | (zona == 3) | (zona == 5)
    piel = piel & ~(vestido & (sat < 0.30))
    # Y en la ropa, piel solo si su color se parece mas a la piel de verdad
    # (la de piernas, pies y manos) que a la media de esa prenda.
    mPiel = col[(zona == 4) & piel].mean(0)
    for zz in (2, 3, 5):
        en = zona == zz
        prenda = col[en & ~piel].mean(0) if (en & ~piel).any() else mPiel
        piel = piel & ~(en & (np.linalg.norm(col - mPiel, axis=-1) > np.linalg.norm(col - prenda, axis=-1)))
    piel = piel.astype(np.float64)
    pelo = ((zona == 1) & (mx < 0.33) & (piel < 0.5)).astype(np.float64)
    piel = np.where(zona == 1, piel * (1 - pelo), piel)
    tela = piel < 0.5
    mCamisa = col[(zona == 2) & tela].mean(0)
    mShort = col[(zona == 3) & tela].mean(0)
    masCamisa = np.linalg.norm(col - mCamisa, axis=-1) < np.linalg.norm(col - mShort, axis=-1)
    camisa = (((zona == 2) | ((zona == 5) & masCamisa)) & tela).astype(np.float64)
    short = (((zona == 3) | ((zona == 5) & ~masCamisa)) & tela).astype(np.float64)
    m = np.stack([piel, pelo, camisa, short], -1)
    img = Image.fromarray((m * 255).astype(np.uint8), "RGBA").filter(ImageFilter.GaussianBlur(1.2))
    medias = []
    for k in range(4):
        w = m[..., k]
        medias.append((col * w[..., None]).sum((0, 1)) / max(w.sum(), 1.0))
    return img, medias


def main():
    if len(sys.argv) < 2:
        raise SystemExit(__doc__)
    objetivo = int(sys.argv[2]) if len(sys.argv) > 2 else 80000
    dir_salida = os.path.join(RAIZ, "resourcepacks", "Modelos")
    os.makedirs(dir_salida, exist_ok=True)
    pos, nor, uv, idx, color, normal, rugosidad = cargar(sys.argv[1])
    print("GLB:", len(pos), "vertices,", len(idx), "triangulos")
    p, n = al_juego(pos, nor)
    abd, hombroY, alcance, entre = medir(p)
    pr, nr, ur, tr = decimar(p, n, uv, idx, objetivo, os.path.join(dir_salida, "_tmp"))
    nr /= np.maximum(np.linalg.norm(nr, axis=1), 1e-12)[:, None]
    tg = tangentes(pr, nr, ur, tr)

    X_HOMBRO, BRAZO, ANTEBRAZO, MANO = 0.20, 0.30, 0.26, 0.185
    kBrazo = ((alcance - X_HOMBRO) / np.sin(abd) - MANO) / (BRAZO + ANTEBRAZO)
    # Con short la tela une las piernas y la entrepierna sale baja: si no es
    # creible (entre 0.78 y 0.92 a 1.80 de alto) se usa la del esqueleto.
    caderaY = entre + 0.07 if 0.78 <= entre <= 0.92 else 0.94
    kPierna = (caderaY - 0.07) / (0.94 - 0.07)
    kHombroY = (hombroY - (caderaY + 0.02)) / 0.48

    mascara_img, medias = mascara(pr, ur, tr, color)
    params = [abd, 0.03, 1.0, kBrazo, kBrazo, kPierna, kPierna, kHombroY, 1.80]
    for m in medias:
        params += list(m)
    params.append(kBrazo)            # la mano, en proporcion al brazo
    for nombre, v in zip(PARAMS, params):
        print("  %-14s %.4f" % (nombre, v))

    color.resize((TEX, TEX), Image.LANCZOS).save(os.path.join(dir_salida, "jugador_color.jpg"), quality=92)
    if normal is not None:
        normal.resize((TEX, TEX), Image.LANCZOS).save(os.path.join(dir_salida, "jugador_normal.png"), optimize=True)
    mascara_img.save(os.path.join(dir_salida, "jugador_mascara.png"), optimize=True)
    # La rugosidad (canal G del metallicRoughness de glTF): la piel brilla
    # algo, el pelo menos, la tela casi nada.
    if rugosidad is not None:
        rugosidad.split()[1].resize((1024, 1024), Image.LANCZOS).save(os.path.join(dir_salida, "jugador_rugosidad.jpg"), quality=90)
    with open(os.path.join(dir_salida, "jugador.vwm"), "wb") as f:
        f.write(b"VWM2")
        f.write(struct.pack("<II", len(pr), len(tr)))
        f.write(struct.pack("<32f", *(params + [0.0] * (32 - len(params)))))
        f.write(np.hstack([pr, nr, ur, tg]).astype("<f4").tobytes())
        f.write(tr.astype("<u4").tobytes())
    print("Salida:", len(pr), "vertices,", len(tr), "triangulos ->", dir_salida)


if __name__ == "__main__":
    main()
