// ============================================================================
// DECIMAR UNA MALLA CON TEXTURA SIN ROMPER SUS COSTURAS UV
// ============================================================================
// Lo usa tools/convertir_jugador_glb.py para bajar el modelo del jugador de
// ~1 millon de triangulos a los que pida, conservando sus coordenadas de
// textura: asi el modelo del juego se ve con la textura original y no con un
// color por vertice.
//
//     decimar_malla entrada.raw salida.raw triangulos_objetivo
//
// .raw (little endian): uint32 nv, nt; nv x float[8] (pos, normal, uv);
// nt x uint32[3].
//
// EL METODO: colapso de media arista con el error cuadratico de Garland y
// Heckbert (la suma de las distancias al cuadrado a los planos de los
// triangulos que tocaba cada vertice). Se opera sobre POSICIONES soldadas;
// cada posicion tiene una o mas "cunas" (los vertices del indice, con su uv y
// su normal): en una costura UV la misma posicion tiene dos cunas.
//
// Al llevar la posicion p0 a p1, cada cuna de p0 pasa a la cuna de p1 con la
// que comparte un triangulo de la arista. Si alguna cuna de p0 no comparte
// ninguno, la costura cruzaria la arista en vez de seguirla: ese colapso no
// se hace. Resultado: las costuras se simplifican a lo largo, las dos orillas
// juntas, y nunca se abren grietas ni se mezclan dos islas de la textura.
//
// Ademas se rechaza lo que voltearia un triangulo, lo que dejaria la malla no
// manifold (la condicion de enlace) y mover un borde abierto fuera de su borde.
// ============================================================================
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <queue>
#include <unordered_map>
#include <vector>

struct V3 { double x, y, z; };
static V3 operator-(V3 a, V3 b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
static V3 cruz(V3 a, V3 b) { return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x }; }
static double punto(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static double largo(V3 a) { return std::sqrt(punto(a, a)); }

struct Cuadrica {
    double m[10] = {};
    void sumarPlano(double a, double b, double c, double d, double w) {
        m[0] += w * a * a; m[1] += w * a * b; m[2] += w * a * c; m[3] += w * a * d;
        m[4] += w * b * b; m[5] += w * b * c; m[6] += w * b * d;
        m[7] += w * c * c; m[8] += w * c * d; m[9] += w * d * d;
    }
    void sumar(const Cuadrica& o) { for (int i = 0; i < 10; ++i) m[i] += o.m[i]; }
    double error(V3 p) const {
        const double x = p.x, y = p.y, z = p.z;
        return m[0] * x * x + 2 * m[1] * x * y + 2 * m[2] * x * z + 2 * m[3] * x + m[4] * y * y +
               2 * m[5] * y * z + 2 * m[6] * y + m[7] * z * z + 2 * m[8] * z + m[9];
    }
};

struct Cuna { int pos; float n[3], uv[2]; };

int main(int argc, char** argv) {
    if (argc < 4) { std::fprintf(stderr, "uso: decimar_malla entrada.raw salida.raw triangulos\n"); return 1; }
    const long objetivo = std::atol(argv[3]);
    FILE* f = std::fopen(argv[1], "rb");
    if (!f) { std::fprintf(stderr, "no abre %s\n", argv[1]); return 1; }
    uint32_t nv = 0, nt = 0;
    if (std::fread(&nv, 4, 1, f) != 1 || std::fread(&nt, 4, 1, f) != 1) return 1;
    std::vector<float> vb((size_t)nv * 8);
    std::vector<uint32_t> ib((size_t)nt * 3);
    if (std::fread(vb.data(), 4, vb.size(), f) != vb.size() || std::fread(ib.data(), 4, ib.size(), f) != ib.size()) return 1;
    std::fclose(f);

    // Soldar posiciones exactas: las cunas de una costura comparten posicion.
    std::vector<V3> P;
    std::vector<Cuna> W(nv);
    {
        std::unordered_map<uint64_t, std::vector<int>> mapa;
        for (uint32_t i = 0; i < nv; ++i) {
            const float* q = &vb[(size_t)i * 8];
            const int64_t kx = (int64_t)std::llround(q[0] * 1e6), ky = (int64_t)std::llround(q[1] * 1e6),
                          kz = (int64_t)std::llround(q[2] * 1e6);
            const uint64_t h = (uint64_t)kx * 73856093ull ^ (uint64_t)ky * 19349663ull ^ (uint64_t)kz * 83492791ull;
            int encontrada = -1;
            for (int p : mapa[h])
                if (std::llround(P[p].x * 1e6) == kx && std::llround(P[p].y * 1e6) == ky && std::llround(P[p].z * 1e6) == kz) encontrada = p;
            if (encontrada < 0) { encontrada = (int)P.size(); P.push_back({ q[0], q[1], q[2] }); mapa[h].push_back(encontrada); }
            W[i] = { encontrada, { q[3], q[4], q[5] }, { q[6], q[7] } };
        }
    }
    const int np = (int)P.size();
    std::vector<std::array<int, 3>> T(nt);
    std::vector<char> viva(nt, 1);
    for (uint32_t t = 0; t < nt; ++t) T[t] = { (int)ib[t * 3], (int)ib[t * 3 + 1], (int)ib[t * 3 + 2] };
    auto posDe = [&](int t, int k) { return W[T[t][k]].pos; };

    std::vector<std::vector<int>> tris(np);
    for (uint32_t t = 0; t < nt; ++t) {
        if (posDe(t, 0) == posDe(t, 1) || posDe(t, 1) == posDe(t, 2) || posDe(t, 0) == posDe(t, 2)) { viva[t] = 0; continue; }
        for (int k = 0; k < 3; ++k) tris[posDe(t, k)].push_back((int)t);
    }
    // Las cuadricas, con el area como peso.
    std::vector<Cuadrica> Q(np);
    for (uint32_t t = 0; t < nt; ++t) {
        if (!viva[t]) continue;
        const V3 a = P[posDe(t, 0)], b = P[posDe(t, 1)], c = P[posDe(t, 2)];
        V3 n = cruz(b - a, c - a);
        const double l = largo(n);
        if (l < 1e-20) continue;
        n = { n.x / l, n.y / l, n.z / l };
        const double d = -punto(n, a);
        for (int k = 0; k < 3; ++k) Q[posDe(t, k)].sumarPlano(n.x, n.y, n.z, d, l * 0.5);
    }
    // Los bordes abiertos: un plano perpendicular que los sujeta.
    std::vector<char> borde(np, 0);
    {
        std::map<std::pair<int, int>, int> aristas;
        for (uint32_t t = 0; t < nt; ++t) {
            if (!viva[t]) continue;
            for (int k = 0; k < 3; ++k) {
                int a = posDe(t, k), b = posDe(t, (k + 1) % 3);
                if (a > b) std::swap(a, b);
                aristas[{ a, b }]++;
            }
        }
        for (const auto& e : aristas)
            if (e.second == 1) { borde[e.first.first] = 1; borde[e.first.second] = 1; }
    }

    std::vector<int> version(np, 0);
    std::vector<char> vivo(np, 1);
    struct Cand { double coste; int p0, p1, v0, v1; bool operator<(const Cand& o) const { return coste > o.coste; } };
    std::priority_queue<Cand> cola;
    auto vecinos = [&](int p, std::vector<int>& out) {
        out.clear();
        for (int t : tris[p]) if (viva[t]) for (int k = 0; k < 3; ++k) { const int q = posDe(t, k); if (q != p) out.push_back(q); }
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());
    };
    auto proponer = [&](int p0) {
        std::vector<int> vs;
        vecinos(p0, vs);
        for (int p1 : vs) {
            if (borde[p0] && !borde[p1]) continue;
            Cuadrica q = Q[p0];
            q.sumar(Q[p1]);
            cola.push({ q.error(P[p1]), p0, p1, version[p0], version[p1] });
        }
    };
    for (int p = 0; p < np; ++p) if (!tris[p].empty()) proponer(p);

    long vivos = 0;
    for (uint32_t t = 0; t < nt; ++t) vivos += viva[t];
    std::vector<int> n0, n1;
    while (vivos > objetivo && !cola.empty()) {
        const Cand c = cola.top();
        cola.pop();
        if (!vivo[c.p0] || !vivo[c.p1] || version[c.p0] != c.v0 || version[c.p1] != c.v1) continue;
        // Condicion de enlace: los vecinos comunes son los de los triangulos
        // de la arista (2 en el interior, 1 en el borde).
        vecinos(c.p0, n0);
        vecinos(c.p1, n1);
        int comunes = 0;
        for (int a : n0) if (std::binary_search(n1.begin(), n1.end(), a)) ++comunes;
        int deArista = 0;
        for (int t : tris[c.p0]) if (viva[t]) for (int k = 0; k < 3; ++k) if (posDe(t, k) == c.p1) ++deArista;
        if (deArista == 0 || comunes != deArista) continue;
        // Las cunas: cada cuna de p0 a la de p1 de un triangulo de la arista.
        std::vector<std::pair<int, int>> mapa;
        bool ok = true;
        for (int t : tris[c.p0]) {
            if (!viva[t]) continue;
            int w0 = -1, w1 = -1;
            for (int k = 0; k < 3; ++k) { if (posDe(t, k) == c.p0) w0 = T[t][k]; if (posDe(t, k) == c.p1) w1 = T[t][k]; }
            if (w1 < 0) continue;
            bool ya = false;
            for (auto& m : mapa) if (m.first == w0) { ya = true; if (m.second != w1) ok = false; }
            if (!ya) mapa.push_back({ w0, w1 });
        }
        for (int t : tris[c.p0]) {
            if (!viva[t] || !ok) continue;
            for (int k = 0; k < 3; ++k)
                if (posDe(t, k) == c.p0) {
                    bool hay = false;
                    for (auto& m : mapa) if (m.first == T[t][k]) hay = true;
                    if (!hay) ok = false;
                }
        }
        if (!ok) continue;
        // Que nada se voltee ni se aplaste.
        for (int t : tris[c.p0]) {
            if (!viva[t]) continue;
            bool conP1 = false;
            for (int k = 0; k < 3; ++k) if (posDe(t, k) == c.p1) conP1 = true;
            if (conP1) continue;
            V3 a = P[posDe(t, 0)], b = P[posDe(t, 1)], cc = P[posDe(t, 2)];
            const V3 antes = cruz(b - a, cc - a);
            for (int k = 0; k < 3; ++k) if (posDe(t, k) == c.p0) { if (k == 0) a = P[c.p1]; else if (k == 1) b = P[c.p1]; else cc = P[c.p1]; }
            const V3 despues = cruz(b - a, cc - a);
            const double la = largo(antes), ld = largo(despues);
            if (ld < 1e-14 || punto(antes, despues) < 0.3 * la * ld) { ok = false; break; }
        }
        if (!ok) continue;
        // Colapsar.
        for (int t : tris[c.p0]) {
            if (!viva[t]) continue;
            bool conP1 = false;
            for (int k = 0; k < 3; ++k) if (posDe(t, k) == c.p1) conP1 = true;
            if (conP1) { viva[t] = 0; --vivos; continue; }
            for (int k = 0; k < 3; ++k)
                if (posDe(t, k) == c.p0)
                    for (auto& m : mapa) if (m.first == T[t][k]) { T[t][k] = m.second; break; }
            tris[c.p1].push_back(t);
        }
        tris[c.p0].clear();
        vivo[c.p0] = 0;
        Q[c.p1].sumar(Q[c.p0]);
        // Compactar la lista de p1 de vez en cuando.
        if (tris[c.p1].size() > 64) {
            auto& L = tris[c.p1];
            L.erase(std::remove_if(L.begin(), L.end(), [&](int t) { return !viva[t]; }), L.end());
            std::sort(L.begin(), L.end());
            L.erase(std::unique(L.begin(), L.end()), L.end());
        }
        ++version[c.p1];
        vecinos(c.p1, n1);
        for (int q : n1) ++version[q];
        proponer(c.p1);
        for (int q : n1) proponer(q);
    }

    // Escribir solo lo vivo.
    std::vector<int> nuevo(nv, -1);
    std::vector<uint32_t> io;
    std::vector<float> vo;
    for (uint32_t t = 0; t < nt; ++t) {
        if (!viva[t]) continue;
        for (int k = 0; k < 3; ++k) {
            const int w = T[t][k];
            if (nuevo[w] < 0) {
                nuevo[w] = (int)(vo.size() / 8);
                const V3 p = P[W[w].pos];
                vo.insert(vo.end(), { (float)p.x, (float)p.y, (float)p.z, W[w].n[0], W[w].n[1], W[w].n[2], W[w].uv[0], W[w].uv[1] });
            }
            io.push_back((uint32_t)nuevo[w]);
        }
    }
    FILE* o = std::fopen(argv[2], "wb");
    if (!o) return 1;
    const uint32_t onv = (uint32_t)(vo.size() / 8), ont = (uint32_t)(io.size() / 3);
    std::fwrite(&onv, 4, 1, o); std::fwrite(&ont, 4, 1, o);
    std::fwrite(vo.data(), 4, vo.size(), o); std::fwrite(io.data(), 4, io.size(), o);
    std::fclose(o);
    std::printf("decimar_malla: %u -> %u triangulos, %u vertices\n", nt, ont, onv);
    return 0;
}
