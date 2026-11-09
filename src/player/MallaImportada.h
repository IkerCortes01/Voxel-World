#pragma once

#include "MallaJugador.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR CON UN MODELO IMPORTADO (GLB -> .vwm)
// ============================================================================
// PEDIDO: "remplaza el modelo del jugador por este model.glb" (un modelo
// realista generado en Magnific).
//
// El GLB no trae esqueleto ni animaciones, y tiene un millon de triangulos.
// tools/convertir_jugador_glb.py lo reduce a ~20.000 triangulos con el color
// de la textura en cada vertice y lo deja en resourcepacks/Modelos/jugador.vwm.
//
// Aqui se le pone el ESQUELETO DEL JUEGO (ModeloJugador): cada vertice sigue
// a uno o dos huesos (skinning). Asi el modelo nuevo hace todo lo que ya hacia
// el procedural -- andar, golpear, agacharse, los gestos de la tecla G y los
// limites anatomicos -- sin animar nada aparte.
//
// LOS PESOS: el hueso de cada vertice es el mas cercano, midiendo la
// distancia a cada hueso en "grosores" de ese hueso (un punto del costado del
// pecho esta mas cerca del eje del brazo en metros, pero a menos grosores del
// tronco). Si el segundo mas cercano es su padre o su hijo y esta casi igual
// de cerca, el vertice se reparte entre los dos: asi codos, rodillas, hombros
// y caderas se doblan en curva y no se rompen.
//
// CADA FRAME: cada hueso pasa del marco de la pose de referencia (la cruz del
// modelo) al de la pose actual, y cada vertice se mueve con sus dos huesos.
// Son ~10.000 vertices por 2 matrices 3x3: unas decimas de milisegundo.
// ============================================================================

namespace ModeloJugador {

// Un hueso: su marco, el segmento que ocupa y su grosor.
struct Hueso {
    Junta marco;
    V a, b;
    float radio;
    int padre;
};

enum : int { H_PELVIS, H_PECHO, H_CABEZA, H_POR_LADO_INICIO };
// Por cada lado, en este orden (ver huesosDe).
enum : int { HL_CLAVICULA, HL_BRAZO, HL_ANTE1, HL_ANTE2, HL_MANO, HL_DEDO0 = 5,
             HL_MUSLO = HL_DEDO0 + 15, HL_PIERNA, HL_PIE, HL_DEDOS_PIE, HL_NUM };
constexpr int NUM_HUESOS = H_POR_LADO_INICIO + 2 * HL_NUM;
inline int huesoDe(int lado, int k) { return H_POR_LADO_INICIO + lado * HL_NUM + k; }

// Los huesos de un esqueleto ya colocado. Mismo orden siempre: el de la pose
// de referencia y el de la actual se emparejan por indice.
inline void huesosDe(const Esqueleto& s, Hueso* h) {
    h[H_PELVIS] = { s.pelvis, suma(s.pelvis.p, s.pelvis.r.aplicar({ 0, -0.14f, 0 })),
                    suma(s.pelvis.p, s.pecho.r.aplicar({ 0, 0.16f, 0 })), 0.14f, -1 };
    h[H_PECHO] = { s.pecho, suma(s.pelvis.p, s.pecho.r.aplicar({ 0, 0.16f, 0 })),
                   suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.04f, 0 })), 0.13f, H_PELVIS };
    h[H_CABEZA] = { s.cuello, s.cuello.p, suma(s.cuello.p, s.cuello.r.aplicar({ 0, 0.22f, 0.02f })), 0.095f, H_PECHO };
    for (int l = 0; l < 2; ++l) {
        auto H = [&](int k) -> Hueso& { return h[huesoDe(l, k)]; };
        const Mano& m = s.mano[l];
        const Pie& pie = s.pie[l];
        H(HL_CLAVICULA) = { s.pecho, suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.05f, 0 })), s.hombro[l].p, 0.055f, H_PECHO };
        H(HL_BRAZO) = { s.hombro[l], s.hombro[l].p, s.codo[l].p, 0.075f, huesoDe(l, HL_CLAVICULA) };
        const V medio = mezclar(s.codo[l].p, m.muneca.p, 0.5f);
        H(HL_ANTE1) = { s.codo[l], s.codo[l].p, medio, 0.050f, huesoDe(l, HL_BRAZO) };
        H(HL_ANTE2) = { { medio, s.codo[l].r.por(Rot::enY(s.prono[l] * 0.5f)) }, medio, m.muneca.p, 0.040f,
                        huesoDe(l, HL_ANTE1) };
        H(HL_MANO) = { m.muneca, m.muneca.p, m.dedo[1][0].p, 0.034f, huesoDe(l, HL_ANTE2) };
        for (int d = 0; d < 5; ++d)
            for (int f = 0; f < 3; ++f)
                H(HL_DEDO0 + d * 3 + f) = { m.dedo[d][f], m.dedo[d][f].p, m.dedo[d][f + 1].p, 0.0095f,
                                            f == 0 ? huesoDe(l, HL_MANO) : huesoDe(l, HL_DEDO0 + d * 3 + f - 1) };
        H(HL_MUSLO) = { s.cadera[l], s.cadera[l].p, s.rodilla[l].p, 0.085f, H_PELVIS };
        H(HL_PIERNA) = { s.rodilla[l], s.rodilla[l].p, pie.tobillo.p, 0.060f, huesoDe(l, HL_MUSLO) };
        H(HL_PIE) = { pie.tobillo, pie.talon, pie.bola, 0.040f, huesoDe(l, HL_PIERNA) };
        H(HL_DEDOS_PIE) = { { pie.bola, pie.dedo[0][0].r }, pie.bola, pie.dedo[2][2].p, 0.020f, huesoDe(l, HL_PIE) };
    }
}

namespace detalle {
inline float distSegmento(V p, V a, V b) {
    const V ab = resta(b, a);
    const float l2 = punto(ab, ab);
    const float t = l2 > 1e-12f ? sujetar(punto(resta(p, a), ab) / l2, 0.0f, 1.0f) : 0.0f;
    return largo(resta(p, suma(a, escala(ab, t))));
}
// La transpuesta de una rotacion es su inversa.
inline Rot traspuesta(const Rot& r) {
    return { { r.m[0], r.m[3], r.m[6], r.m[1], r.m[4], r.m[7], r.m[2], r.m[5], r.m[8] } };
}
} // namespace detalle

struct MallaImportada {
    bool cargada = false;
    float params[16] = {};
    std::vector<VerticeJ> base;          // en la pose de referencia
    std::vector<uint32_t> indices, indicesSinCabeza;
    std::vector<uint8_t> h0, h1;         // los dos huesos de cada vertice
    std::vector<float> w0;               // el peso del primero (el segundo, 1 - w0)
    Hueso ref[NUM_HUESOS];

    // Las proporciones del esqueleto que encajan en este modelo (las midio el
    // convertidor). En el juego mandan sobre las del avatar.
    Proporciones proporciones() const {
        Proporciones p;
        p.torso = params[2]; p.brazo = params[3]; p.antebrazo = params[4];
        p.muslo = params[5]; p.pierna = params[6]; p.hombroY = params[7];
        return p;
    }
    // La pose en que esta hecho el modelo: en cruz, los brazos algo caidos,
    // los dedos estirados.
    Pose poseReferencia() const {
        Pose p;
        p.prop = proporciones();
        for (Brazo& b : p.brazo) {
            b = Brazo{};
            b.abd = params[0]; b.flex = 0.0f; b.rot = 0.0f; b.codo = 0.0f;
            b.prono = 0.0f; b.munFlex = 0.0f; b.munDesv = 0.0f;
        }
        for (Pierna& g : p.pierna) { g = Pierna{}; g.abd = params[1]; g.rodilla = 0.0f; }
        return p;
    }

    bool cargar(const std::string& ruta) {
        cargada = false;
        FILE* f = std::fopen(ruta.c_str(), "rb");
        if (!f) return false;
        char magia[4];
        uint32_t nv = 0, nt = 0;
        bool ok = std::fread(magia, 1, 4, f) == 4 && std::memcmp(magia, "VWM1", 4) == 0 &&
                  std::fread(&nv, 4, 1, f) == 1 && std::fread(&nt, 4, 1, f) == 1 &&
                  std::fread(params, 4, 16, f) == 16 && nv > 0 && nv < 2000000 && nt > 0 && nt < 4000000;
        std::vector<float> v;
        if (ok) {
            v.resize((size_t)nv * 9);
            indices.resize((size_t)nt * 3);
            ok = std::fread(v.data(), 4, v.size(), f) == v.size() &&
                 std::fread(indices.data(), 4, indices.size(), f) == indices.size();
        }
        std::fclose(f);
        if (!ok) return false;
        for (uint32_t i : indices) if (i >= nv) return false;
        base.resize(nv);
        for (uint32_t i = 0; i < nv; ++i) {
            const float* q = &v[(size_t)i * 9];
            base[i] = { { q[0], q[1], q[2] }, { q[3], q[4], q[5] }, q[6], q[7], q[8] };
        }
        atar();
        cargada = true;
        return true;
    }

    // Los pesos de cada vertice contra el esqueleto en la pose de referencia.
    void atar() {
        const Esqueleto s = resolver(poseReferencia(), true);
        huesosDe(s, ref);
        const size_t n = base.size();
        // El esqueleto del juego va M::ATRAS por detras del ojo; la malla
        // viene centrada sobre los pies. Se mueve para que el eje del tronco
        // (la media del torso, entre la cintura y el pecho) caiga en la
        // columna del esqueleto.
        {
            double zx = 0.0, zz = 0.0;
            int cuantos = 0;
            for (const VerticeJ& v : base)
                if (v.p.y > 1.00f && v.p.y < 1.30f && std::fabs(v.p.x) < 0.15f) { zx += v.p.x; zz += v.p.z; ++cuantos; }
            if (cuantos > 0) {
                const float dx = s.pelvis.p.x - (float)(zx / cuantos), dz = s.pelvis.p.z - (float)(zz / cuantos);
                for (VerticeJ& v : base) { v.p.x += dx; v.p.z += dz; }
            }
        }
        h0.assign(n, 0); h1.assign(n, 0); w0.assign(n, 1.0f);
        for (size_t i = 0; i < n; ++i) {
            const V p = base[i].p;
            float d[NUM_HUESOS];
            int mejor = 0;
            for (int b = 0; b < NUM_HUESOS; ++b) {
                d[b] = detalle::distSegmento(p, ref[b].a, ref[b].b) / ref[b].radio;
                if (d[b] < d[mejor]) mejor = b;
            }
            // El segundo: solo su padre o un hijo (los que comparten
            // articulacion).
            int segundo = -1;
            for (int b = 0; b < NUM_HUESOS; ++b) {
                if (b == mejor || (ref[b].padre != mejor && ref[mejor].padre != b)) continue;
                if (segundo < 0 || d[b] < d[segundo]) segundo = b;
            }
            h0[i] = (uint8_t)mejor;
            h1[i] = (uint8_t)(segundo < 0 ? mejor : segundo);
            if (segundo >= 0) {
                const float a = std::fmax(d[mejor], 0.05f), b = std::fmax(d[segundo], 0.05f);
                const float fa = 1.0f / (a * a * a * a), fb = 1.0f / (b * b * b * b);
                w0[i] = fa / (fa + fb);
                if (w0[i] > 0.97f) { w0[i] = 1.0f; h1[i] = h0[i]; }
            }
        }
        // En primera persona la camara esta dentro de la cabeza: sin ella.
        indicesSinCabeza.clear();
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            bool cabeza = false;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = indices[t + k];
                if ((h0[v] == H_CABEZA && w0[v] > 0.5f) || (h1[v] == H_CABEZA && w0[v] < 0.5f)) cabeza = true;
            }
            if (!cabeza) indicesSinCabeza.insert(indicesSinCabeza.end(), { indices[t], indices[t + 1], indices[t + 2] });
        }
    }

    // La malla en la pose del esqueleto `s`; sin la cabeza en primera persona.
    void deformar(const Esqueleto& s, Malla& m, bool conCabeza = true) const {
        const std::vector<uint32_t>& idx = indicesDe(conCabeza);
        if (m.i.size() != idx.size()) m.i = idx;
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        Rot R[NUM_HUESOS];
        V T[NUM_HUESOS];
        for (int b = 0; b < NUM_HUESOS; ++b) {
            R[b] = h[b].marco.r.por(detalle::traspuesta(ref[b].marco.r));
            T[b] = resta(h[b].marco.p, R[b].aplicar(ref[b].marco.p));
        }
        m.v.resize(base.size());
        for (size_t i = 0; i < base.size(); ++i) {
            const VerticeJ& v = base[i];
            const int a = h0[i], b = h1[i];
            V p = suma(R[a].aplicar(v.p), T[a]);
            V nn = R[a].aplicar(v.n);
            if (a != b) {
                const float wa = w0[i], wb = 1.0f - wa;
                p = suma(escala(p, wa), escala(suma(R[b].aplicar(v.p), T[b]), wb));
                nn = normalizar(suma(escala(nn, wa), escala(R[b].aplicar(v.n), wb)));
            }
            m.v[i] = { p, nn, v.r, v.g, v.b };
        }
    }
    const std::vector<uint32_t>& indicesDe(bool conCabeza) const {
        if (!conCabeza && indicesSinCabeza.empty()) return indices;
        return conCabeza ? indices : indicesSinCabeza;
    }
};

} // namespace ModeloJugador
