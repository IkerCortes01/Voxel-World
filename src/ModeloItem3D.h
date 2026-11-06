#pragma once
// ============================================================================
// EL ITEM CON FORMA DE VERDAD: LA SILUETA "INFLADA"
// ============================================================================
// PEDIDO: "crea un modelo 3D bien hecho de las herramientas, que no sean la
// misma imagen pero con volumen, quiero que sea realista y con la forma 3D
// pero adaptes la textura para que se vea bien, y lo mismo con todos los
// items".
//
// La silueta con canto (SiluetaItem.h) es la imagen con grosor: dos laminas
// planas y un borde recto, al estilo Minecraft. Aqui la silueta se INFLA: el
// grosor de cada punto depende de lo lejos que esta del borde del dibujo, con
// un perfil circular,
//
//     alto(d) = sqrt(d * (2R - d))   si d < R,   R si no
//
// asi que una parte estrecha sale REDONDA (un mango de 2 pixeles es una vara
// cilindrica, no un liston), una parte ancha sale abombada con el centro
// plano (la cabeza de un pico, una piedra) y una figura redonda sale como un
// cuerpo redondo (una tuna). R limita lo gordo que se pone lo ancho.
//
// LA TEXTURA se proyecta de frente: cada punto del modelo toma el color del
// pixel que tiene delante, asi que el mango sigue siendo de madera y la
// cabeza de piedra, por las dos caras. Los costados, que son la ladera del
// abombado, estiran el color del borde del dibujo, que es lo que pasa con un
// objeto real visto de lado.
//
// Para que una linea de 1 pixel no salga plana (sus cuatro esquinas tocan el
// vacio), la rejilla va a medio pixel: el centro de la linea queda a medio
// pixel del borde y ya tiene altura.
//
// Puro: recibe la mascara de opacidad y devuelve la malla. Se cachea por
// textura en main.cpp.
// ============================================================================

#include <cmath>
#include <cstdint>
#include <vector>

namespace ModeloItem3D {

struct Vert { float x, y, z, nx, ny, nz, u, v; };

struct Malla {
    std::vector<Vert> v;
    std::vector<uint32_t> i;
    bool valido = false;
    int ancho = 0, alto = 0;
    float altoMax = 0.0f;      // el grosor maximo (semi), en unidades del cuadro [-1,1]
    // Por donde se agarra: el extremo FINO del eje largo de la figura (en una
    // herramienta, el final del mango) y la direccion hacia el otro extremo
    // (la cabeza). En el cuadro [-1,1], sin corregir la proporcion.
    float agarreX = 0.0f, agarreY = 0.0f, ejeX = 1.0f, ejeY = 0.0f;
};

// Reduce una mascara grande a otra de como mucho `maximo` de lado: la forma
// del modelo no necesita 256x256 (seria medio millon de triangulos) y las UV
// siguen cubriendo la textura entera, asi que no se pierde dibujo.
inline std::vector<unsigned char> reducir(const std::vector<unsigned char>& m, int w, int h,
                                          int maximo, int& rw, int& rh) {
    const int lado = w > h ? w : h;
    const int f = (lado + maximo - 1) / maximo;
    if (f <= 1) { rw = w; rh = h; return m; }
    rw = (w + f - 1) / f; rh = (h + f - 1) / f;
    std::vector<unsigned char> r((size_t)rw * rh, 0);
    for (int y = 0; y < rh; ++y)
        for (int x = 0; x < rw; ++x) {
            int n = 0, tot = 0;
            for (int yy = y * f; yy < (y + 1) * f && yy < h; ++yy)
                for (int xx = x * f; xx < (x + 1) * f && xx < w; ++xx) { ++tot; n += m[(size_t)yy * w + xx] ? 1 : 0; }
            r[(size_t)y * rw + x] = (tot > 0 && n * 2 >= tot) ? 1 : 0;
        }
    return r;
}

// `opaco` w*h, fila 0 arriba. `radio` en pixeles. Las posiciones salen en el
// cuadro [-1,1] (x a la derecha, y arriba) y la z en las mismas unidades que
// un pixel de ancho; las UV, en el convenio de OpenGL (v=0 abajo).
inline Malla construir(const std::vector<unsigned char>& opaco, int w, int h, float radio) {
    Malla m;
    if (w <= 0 || h <= 0 || (int)opaco.size() < w * h || radio <= 0.0f) return m;
    m.ancho = w; m.alto = h;
    constexpr int S = 2;                          // subdivision: medio pixel
    const int GW = w * S, GH = h * S;
    auto op = [&](int x, int y) { return x >= 0 && y >= 0 && x < w && y < h && opaco[y * w + x] != 0; };

    // La altura (en pixeles) en cada punto de la rejilla.
    std::vector<float> alt((size_t)(GW + 1) * (GH + 1), 0.0f);
    const int ventana = (int)std::ceil(radio) + 2;
    for (int gy = 0; gy <= GH; ++gy) {
        for (int gx = 0; gx <= GW; ++gx) {
            const float px = (float)gx / S, py = (float)gy / S;
            const int cx = (int)std::floor(px), cy = (int)std::floor(py);
            // Distancia al pixel vacio mas cercano (fuera de la imagen es
            // vacio). Un punto que toca un pixel vacio da 0: el borde.
            float d2 = 1e9f;
            for (int ty = cy - ventana; ty <= cy + ventana; ++ty)
                for (int tx = cx - ventana; tx <= cx + ventana; ++tx) {
                    if (op(tx, ty)) continue;
                    const float dx = px < tx ? tx - px : (px > tx + 1 ? px - (tx + 1) : 0.0f);
                    const float dy = py < ty ? ty - py : (py > ty + 1 ? py - (ty + 1) : 0.0f);
                    const float dd = dx * dx + dy * dy;
                    if (dd < d2) d2 = dd;
                }
            const float d = d2 >= 1e8f ? radio : std::sqrt(d2);
            alt[(size_t)gy * (GW + 1) + gx] = d >= radio ? radio : std::sqrt(d * (2.0f * radio - d));
        }
    }

    const float paso = 2.0f / (float)(w > h ? w : h);    // un pixel, en el cuadro
    auto A = [&](int gx, int gy) {
        gx = gx < 0 ? 0 : (gx > GW ? GW : gx);
        gy = gy < 0 ? 0 : (gy > GH ? GH : gy);
        return alt[(size_t)gy * (GW + 1) + gx];
    };

    // Un vertice por punto usado y por cara (arriba / abajo).
    std::vector<int32_t> idx((size_t)(GW + 1) * (GH + 1) * 2, -1);
    auto vert = [&](int gx, int gy, int cara) -> uint32_t {
        int32_t& r = idx[((size_t)gy * (GW + 1) + gx) * 2 + cara];
        if (r >= 0) return (uint32_t)r;
        const float a = A(gx, gy) * paso;
        // Pendiente de la altura, para la normal (en unidades del cuadro).
        const float hx = (A(gx + 1, gy) - A(gx - 1, gy)) * paso / (2.0f * paso / S);
        const float hy = -(A(gx, gy + 1) - A(gx, gy - 1)) * paso / (2.0f * paso / S);
        const float sz = cara == 0 ? 1.0f : -1.0f;
        float nx = -hx * sz, ny = -hy * sz, nz = sz;
        const float l = std::sqrt(nx * nx + ny * ny + nz * nz);
        nx /= l; ny /= l; nz /= l;
        Vert v;
        v.x = -1.0f + 2.0f * gx / GW;
        v.y = 1.0f - 2.0f * gy / GH;
        v.z = sz * a;
        v.nx = nx; v.ny = ny; v.nz = nz;
        v.u = (float)gx / GW;
        v.v = 1.0f - (float)gy / GH;
        if (a > m.altoMax) m.altoMax = a;
        r = (int32_t)m.v.size();
        m.v.push_back(v);
        return (uint32_t)r;
    };

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            if (!op(x, y)) continue;
            for (int b = 0; b < S; ++b)
                for (int a = 0; a < S; ++a) {
                    const int gx = x * S + a, gy = y * S + b;
                    for (int cara = 0; cara < 2; ++cara) {
                        const uint32_t p0 = vert(gx, gy, cara), p1 = vert(gx + 1, gy, cara),
                                       p2 = vert(gx + 1, gy + 1, cara), p3 = vert(gx, gy + 1, cara);
                        if (cara == 0) m.i.insert(m.i.end(), { p0, p3, p2, p0, p2, p1 });
                        else           m.i.insert(m.i.end(), { p0, p1, p2, p0, p2, p3 });
                    }
                }
        }
    m.valido = !m.i.empty();

    // El eje largo (componente principal de los pixeles opacos) y cual de sus
    // dos extremos es el fino.
    double sx = 0, sy = 0, n = 0;
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            if (op(x, y)) { sx += -1.0 + 2.0 * (x + 0.5) / w; sy += 1.0 - 2.0 * (y + 0.5) / h; ++n; }
    if (n > 2) {
        const double cx = sx / n, cy = sy / n;
        double xx = 0, xy = 0, yy = 0;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (op(x, y)) {
                    const double dx = -1.0 + 2.0 * (x + 0.5) / w - cx, dy = 1.0 - 2.0 * (y + 0.5) / h - cy;
                    xx += dx * dx; xy += dx * dy; yy += dy * dy;
                }
        const double ang = 0.5 * std::atan2(2.0 * xy, xx - yy);
        const double ex = std::cos(ang), ey = std::sin(ang);
        double lo = 1e9, hi = -1e9;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (op(x, y)) {
                    const double t = (-1.0 + 2.0 * (x + 0.5) / w - cx) * ex + (1.0 - 2.0 * (y + 0.5) / h - cy) * ey;
                    if (t < lo) lo = t;
                    if (t > hi) hi = t;
                }
        // Los pixeles del 15 % de cada punta: la punta con menos es la fina.
        const double corte = (hi - lo) * 0.15;
        double nLo = 0, nHi = 0, xLo = 0, yLo = 0, xHi = 0, yHi = 0;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (op(x, y)) {
                    const double px = -1.0 + 2.0 * (x + 0.5) / w, py = 1.0 - 2.0 * (y + 0.5) / h;
                    const double t = (px - cx) * ex + (py - cy) * ey;
                    if (t <= lo + corte) { ++nLo; xLo += px; yLo += py; }
                    if (t >= hi - corte) { ++nHi; xHi += px; yHi += py; }
                }
        const bool finoAbajo = nLo <= nHi;
        const double gx = finoAbajo ? xLo / (nLo > 0 ? nLo : 1) : xHi / (nHi > 0 ? nHi : 1);
        const double gy = finoAbajo ? yLo / (nLo > 0 ? nLo : 1) : yHi / (nHi > 0 ? nHi : 1);
        m.agarreX = (float)gx;
        m.agarreY = (float)gy;
        m.ejeX = (float)(finoAbajo ? ex : -ex);
        m.ejeY = (float)(finoAbajo ? ey : -ey);
    }
    return m;
}

// El radio de inflado de cada clase de objeto, en fraccion del ancho de la
// textura: las herramientas y los palos, finos (el mango sale redondo y la
// cabeza no engorda de mas); las pencas, como placas; el resto (frutos,
// piedras, pedacitos), bien redondos.
enum class Forma : uint8_t { HERRAMIENTA, PLACA, REDONDO };
inline float radioPara(Forma f, int ancho) {
    const float k = f == Forma::HERRAMIENTA ? 0.08f : (f == Forma::PLACA ? 0.07f : 0.22f);
    float r = k * (float)ancho;
    return r < 0.8f ? 0.8f : r;
}

} // namespace ModeloItem3D
