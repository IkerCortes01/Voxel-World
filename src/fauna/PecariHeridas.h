#ifndef PECARI_HERIDAS_H
#define PECARI_HERIDAS_H

#include <cmath>
#include <cstdint>
#include <vector>
#include "AnimalMalla.h"

// ============================================================================
// HERIDAS: CALCOMANIAS PROYECTADAS SOBRE LA MALLA
// ============================================================================
// La tecnica de three.js "webgl_decals" (DecalGeometry): una caja orientada
// por la normal del punto de impacto recorta los triangulos de la malla que
// toca, y lo recortado se dibuja con una textura encima de la piel.
//
// LA DIFERENCIA CON LA PAGINA: alli la cabeza esta quieta. Aqui el animal se
// dobla, camina y cae como ragdoll. Por eso la calcomania no guarda posiciones
// sino, por cada vertice, EN QUE TRIANGULO de la malla en reposo cae y con que
// pesos baricentricos. Al dibujar se rehace a partir de los vertices YA
// DEFORMADOS: sigue a los huesos sin calcular nada mas.
//
// Todo en el espacio LOCAL del animal, en metros, como la malla.
// ============================================================================

namespace Fauna {

// Un vertice de la calcomania: un punto dentro de un triangulo de la malla.
struct PuntoCalca {
    uint16_t a, b, c;     // el triangulo
    float wb, wc;         // pos = A + wb*(B-A) + wc*(C-A)
    float u, v;           // textura
};

struct Herida {
    V3    centro;         // donde pego el golpe
    V3    normal;         // hacia fuera de la piel
    float tam = 0.12f;    // lado de la caja, metros
    float giro = 0.0f;    // radianes: cada mancha distinta

    // La geometria depende de la malla (cada LOD es otra). Se recorta la
    // primera vez que se dibuja con esa malla y se guarda.
    struct Geo { const MallaAnimal* malla = nullptr; std::vector<PuntoCalca> pts; };
    mutable Geo cache[2];
    mutable int siguiente = 0;
};

inline V3 hRestar(const V3& a, const V3& b) { return V3(a.x - b.x, a.y - b.y, a.z - b.z); }
inline float hPunto(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 hCruz(const V3& a, const V3& b) {
    return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

// ----------------------------------------------------------------------------
// RECORTAR: la caja de la herida contra la malla
// ----------------------------------------------------------------------------
inline void ProyectarHerida(const MallaAnimal& m, const Herida& h,
                            std::vector<PuntoCalca>& salida) {
    salida.clear();
    const V3 n = h.normal.normalizado();
    V3 ref = std::fabs(n.y) < 0.9f ? V3(0, 1, 0) : V3(1, 0, 0);
    V3 t1 = hCruz(ref, n).normalizado();
    V3 t2 = hCruz(n, t1);
    const float c = std::cos(h.giro), s = std::sin(h.giro);
    const V3 e1(t1.x * c + t2.x * s, t1.y * c + t2.y * s, t1.z * c + t2.z * s);
    const V3 e2(t2.x * c - t1.x * s, t2.y * c - t1.y * s, t2.z * c - t1.z * s);
    const float inv = 1.0f / h.tam;
    const float radio2 = h.tam * h.tam * 1.2f;

    // Un vertice del poligono que se recorta: coordenadas en la caja (-0.5..0.5)
    // y sus pesos en el triangulo.
    struct P { float x, y, z, wb, wc; };
    P pol[12], tmp[12];

    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        const uint16_t ia = m.indices[t], ib = m.indices[t + 1], ic = m.indices[t + 2];
        if (ia >= m.vertices.size() || ib >= m.vertices.size() || ic >= m.vertices.size())
            continue;
        const V3& A = m.vertices[ia].pos;
        const V3& B = m.vertices[ib].pos;
        const V3& C = m.vertices[ic].pos;

        // Descarte rapido: lejos de la herida, o de espaldas a ella.
        const V3 cen((A.x + B.x + C.x) / 3, (A.y + B.y + C.y) / 3, (A.z + B.z + C.z) / 3);
        const V3 dc = hRestar(cen, h.centro);
        if (hPunto(dc, dc) > radio2) continue;
        const V3 cara = hCruz(hRestar(B, A), hRestar(C, A)).normalizado();
        if (hPunto(cara, n) < 0.2f) continue;

        const auto enCaja = [&](const V3& q, float wb, float wc) {
            const V3 d = hRestar(q, h.centro);
            return P{ hPunto(d, e1) * inv, hPunto(d, e2) * inv, hPunto(d, n) * inv, wb, wc };
        };
        int k = 3;
        pol[0] = enCaja(A, 0, 0);
        pol[1] = enCaja(B, 1, 0);
        pol[2] = enCaja(C, 0, 1);

        // Sutherland-Hodgman contra los seis planos |eje| <= 0.5.
        for (int plano = 0; plano < 6 && k > 0; ++plano) {
            const int eje = plano / 2;
            const float signo = (plano % 2) ? -1.0f : 1.0f;
            const auto dist = [&](const P& q) {
                const float v = eje == 0 ? q.x : (eje == 1 ? q.y : q.z);
                return 0.5f - signo * v;              // >= 0 dentro
            };
            int nk = 0;
            for (int i = 0; i < k && nk < 11; ++i) {
                const P& a = pol[i];
                const P& b = pol[(i + 1) % k];
                const float da = dist(a), db = dist(b);
                if (da >= 0) tmp[nk++] = a;
                if ((da >= 0) != (db >= 0) && nk < 11) {
                    const float f = da / (da - db);
                    tmp[nk++] = P{ a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f,
                                   a.z + (b.z - a.z) * f, a.wb + (b.wb - a.wb) * f,
                                   a.wc + (b.wc - a.wc) * f };
                }
            }
            for (int i = 0; i < nk; ++i) pol[i] = tmp[i];
            k = nk;
        }
        // Abanico de triangulos.
        for (int i = 1; i + 1 < k; ++i) {
            const P* tri[3] = { &pol[0], &pol[i], &pol[i + 1] };
            for (const P* q : tri)
                salida.push_back({ ia, ib, ic, q->wb, q->wc, q->x + 0.5f, q->y + 0.5f });
        }
    }
}

// La geometria de la herida para esta malla, recortada una sola vez.
inline const std::vector<PuntoCalca>& GeometriaHerida(const MallaAnimal& m, const Herida& h) {
    for (auto& g : h.cache)
        if (g.malla == &m) return g.pts;
    Herida::Geo& g = h.cache[h.siguiente];
    h.siguiente = (h.siguiente + 1) % 2;
    g.malla = &m;
    ProyectarHerida(m, h, g.pts);
    return g.pts;
}

// ----------------------------------------------------------------------------
// DONDE TOCA EL RAYO: Moller-Trumbore contra todos los triangulos
// ----------------------------------------------------------------------------
inline bool ImpactoEnMalla(const MallaAnimal& m, const V3& origen, const V3& dir,
                           V3& punto_, V3& normal_) {
    float mejor = 1e30f;
    bool hay = false;
    for (size_t t = 0; t + 2 < m.indices.size(); t += 3) {
        const uint16_t ia = m.indices[t], ib = m.indices[t + 1], ic = m.indices[t + 2];
        if (ia >= m.vertices.size() || ib >= m.vertices.size() || ic >= m.vertices.size())
            continue;
        const V3& A = m.vertices[ia].pos;
        const V3 e1 = hRestar(m.vertices[ib].pos, A);
        const V3 e2 = hRestar(m.vertices[ic].pos, A);
        const V3 pv = hCruz(dir, e2);
        const float det = hPunto(e1, pv);
        if (std::fabs(det) < 1e-9f) continue;
        const float id = 1.0f / det;
        const V3 tv = hRestar(origen, A);
        const float u = hPunto(tv, pv) * id;
        if (u < 0 || u > 1) continue;
        const V3 qv = hCruz(tv, e1);
        const float v = hPunto(dir, qv) * id;
        if (v < 0 || u + v > 1) continue;
        const float d = hPunto(e2, qv) * id;
        if (d <= 0 || d >= mejor) continue;
        mejor = d;
        hay = true;
        punto_ = V3(origen.x + dir.x * d, origen.y + dir.y * d, origen.z + dir.z * d);
        V3 nn = hCruz(e1, e2).normalizado();
        if (hPunto(nn, dir) > 0) nn = V3(-nn.x, -nn.y, -nn.z);   // hacia el que golpea
        normal_ = nn;
    }
    return hay;
}

} // namespace Fauna

#endif // PECARI_HERIDAS_H
