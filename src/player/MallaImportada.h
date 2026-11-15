#pragma once

#include "MallaJugador.h"
#include <algorithm>
#include <cstdio>
#include <tuple>
#include <cstring>
#include <string>
#include <vector>

// ============================================================================
// EL CUERPO DEL JUGADOR CON UN MODELO IMPORTADO (GLB -> .vwm)
// ============================================================================
// PEDIDOS: "remplaza el modelo del jugador por este model.glb" y "haz que se
// vea identico a lo que descargaste [...] y que pueda ser modificado en
// avatar con libertad".
//
// tools/convertir_jugador_glb.py reduce el GLB (1 millon de triangulos) a
// ~60.000 con un decimador que respeta las costuras UV, y deja la textura de
// color, el mapa de normales y una mascara de zonas (piel, pelo, camisa,
// short). Se ve con la textura original: ver render/CuerpoGL.h.
//
// Aqui se le pone el ESQUELETO DEL JUEGO (ModeloJugador): cada vertice sigue
// a uno o dos huesos (skinning). Asi hace todo lo que hacia el procedural --
// andar, golpear, los gestos de la tecla G y los limites anatomicos.
//
// LOS PESOS: el hueso de cada vertice es el mas cercano, midiendo la
// distancia en "grosores" de cada hueso. Si su padre o su hijo esta casi
// igual de cerca, el vertice se reparte entre los dos (codos, rodillas,
// hombros y caderas se doblan en curva).
//
// EL AVATAR, CON LIBERTAD:
//   - Cada hueso tiene su ESCALA: grosor (brazos, biceps, antebrazos, pecho,
//     cintura, cadera, muslos, gemelos) y tamano (cabeza, manos, pies). El
//     largo de cada hueso sigue a las proporciones del esqueleto: la malla se
//     estira con el.
//   - La cara y el cuerpo se moldean por ZONAS alrededor de puntos que se
//     buscan solos en la malla (la punta de la nariz, el menton, las orejas,
//     los ojos, el cuello, el pecho, el vientre, los gluteos): ancho y largo
//     de cara, frente, pomulos, mandibula, menton, ojos, cejas, nariz, boca,
//     labios, orejas, cuello, busto, abdomen y gluteos.
//   - Los colores (piel de los genes, pelo con canas, camisa y short) los
//     cambia el shader con la mascara de zonas.
// ============================================================================

namespace ModeloJugador {

// Un hueso: su marco, el segmento que ocupa y su grosor.
struct Hueso {
    Junta marco;
    V a, b;
    float radio;
    int padre;
};

// ⭐ LOS 206 HUESOS DEL CUERPO HUMANO (invisibles: solo mueven la malla).
//
// PEDIDO: "anadele huesos al cuerpo [...] exactamente 206 huesos para
// animacion invisibles para mejorar las animaciones y que jamas deformen el
// modelo 3D del jugador".
//
// Son los de un adulto, por regiones:
//   CRANEO 22      frontal, occipital, esfenoides, etmoides, mandibula, vomer
//                  y por pares parietal, temporal, maxilar, palatino,
//                  cigomatico, nasal, lagrimal y cornete
//   OIDO 6         martillo, yunque y estribo, por oido
//   HIOIDES 1
//   COLUMNA 26     7 cervicales, 12 toracicas, 5 lumbares, sacro y coccix
//   TORAX 25       esternon y 12 pares de costillas
//   CINTURA ESCAPULAR 4    claviculas y escapulas
//   BRAZOS 60      humero, radio, cubito, 8 carpos, 5 metacarpos y 14
//                  falanges por lado
//   CINTURA PELVICA 2      los coxales
//   PIERNAS 60     femur, rotula, tibia, perone, 7 tarsos, 5 metatarsos y
//                  14 falanges por lado
//   22 + 6 + 1 + 26 + 25 + 4 + 60 + 2 + 60 = 206
//
// LO QUE GANA LA ANIMACION:
//   - La COLUMNA reparte cada giro y cada inclinacion entre sus 24
//     vertebras: el tronco se curva en vez de doblarse por un punto.
//   - Los huesos que estan EN la articulacion giran a medias entre sus
//     vecinos, como en un cuerpo de verdad: la escapula (hombro), el cubito
//     (el olecranon envuelve el codo), los carpos de la primera fila
//     (muneca), el coxal (cadera), la rotula (rodilla) y el astragalo
//     (tobillo). Asi ninguna articulacion muy doblada se colapsa.
//   - El radio lleva medio giro de la pronacion: el antebrazo se tuerce a lo
//     largo y no de golpe en la muneca.
//   - Las costillas y el esternon siguen a su vertebra: el pecho acompana al
//     tronco sin aplastarse.
//   - Los dedos tienen sus 14 falanges por mano y por pie.
// Los del craneo y el oido van juntos con la cabeza.
//
// Cada hueso tiene su marco, el segmento que ocupa, su grosor y su padre
// (la cadena del esqueleto: el vertice se reparte entre un hueso y su padre o
// su hijo).

// Los huesos sin pareja.
enum : int {
    H_SACRO, H_COCCIX,
    H_VERTEBRA0,                       // L5 .. L1 (5), T12 .. T1 (12): 17 de abajo arriba
    H_CERVICAL0 = H_VERTEBRA0 + 17,    // C7 .. C1 (7)
    H_HIOIDES = H_CERVICAL0 + 7,
    H_ESTERNON,
    H_FRONTAL, H_OCCIPITAL, H_ESFENOIDES, H_ETMOIDES, H_MANDIBULA, H_VOMER,
    H_POR_LADO_INICIO
};
constexpr int H_CABEZA = H_OCCIPITAL;  // el craneo entero se mueve con este marco
// Los de cada lado, en este orden.
enum : int {
    HL_PARIETAL, HL_TEMPORAL, HL_MAXILAR, HL_PALATINO, HL_CIGOMATICO, HL_NASAL, HL_LAGRIMAL, HL_CORNETE,
    HL_MARTILLO, HL_YUNQUE, HL_ESTRIBO,
    HL_COSTILLA0,                      // 12
    HL_CLAVICULA = HL_COSTILLA0 + 12, HL_ESCAPULA, HL_HUMERO, HL_CUBITO, HL_RADIO,
    HL_CARPO0,                         // 8: escafoides, semilunar, piramidal, pisiforme | trapecio, trapezoide, grande, ganchoso
    HL_METACARPO0 = HL_CARPO0 + 8,     // 5: indice, medio, anular, menique, pulgar
    HL_FALANGE_MANO0 = HL_METACARPO0 + 5,   // 14: 3 por dedo largo y 2 del pulgar
    HL_COXAL = HL_FALANGE_MANO0 + 14, HL_FEMUR, HL_ROTULA, HL_TIBIA, HL_PERONE,
    HL_TARSO0,                         // 7: astragalo, calcaneo, navicular, cuboides, 3 cuneiformes
    HL_METATARSO0 = HL_TARSO0 + 7,     // 5: del gordo al pequeno
    HL_FALANGE_PIE0 = HL_METATARSO0 + 5,    // 14: 2 del gordo y 3 de cada otro
    HL_NUM = HL_FALANGE_PIE0 + 14
};
constexpr int NUM_HUESOS = H_POR_LADO_INICIO + 2 * HL_NUM;
static_assert(NUM_HUESOS == 206, "el cuerpo humano adulto tiene 206 huesos");
inline int huesoDe(int lado, int k) { return H_POR_LADO_INICIO + lado * HL_NUM + k; }
// Los huesos de la cabeza (craneo, cara y oido).
inline bool esDeLaCabeza(int b) {
    if (b >= H_FRONTAL && b <= H_VOMER) return true;
    if (b < H_POR_LADO_INICIO) return false;
    const int k = (b - H_POR_LADO_INICIO) % HL_NUM;
    return k <= HL_ESTRIBO;
}

namespace detalle {
// El giro a medio camino entre dos marcos (cuaterniones: la media normalizada
// es exactamente el slerp al 50 %).
struct Cuat { float w, x, y, z; };
inline Cuat aCuat(const Rot& r) {
    const float* m = r.m;
    Cuat q;
    const float tr = m[0] + m[4] + m[8];
    if (tr > 0.0f) {
        const float s = std::sqrt(tr + 1.0f) * 2.0f;
        q = { 0.25f * s, (m[7] - m[5]) / s, (m[2] - m[6]) / s, (m[3] - m[1]) / s };
    } else if (m[0] > m[4] && m[0] > m[8]) {
        const float s = std::sqrt(1.0f + m[0] - m[4] - m[8]) * 2.0f;
        q = { (m[7] - m[5]) / s, 0.25f * s, (m[1] + m[3]) / s, (m[2] + m[6]) / s };
    } else if (m[4] > m[8]) {
        const float s = std::sqrt(1.0f + m[4] - m[0] - m[8]) * 2.0f;
        q = { (m[2] - m[6]) / s, (m[1] + m[3]) / s, 0.25f * s, (m[5] + m[7]) / s };
    } else {
        const float s = std::sqrt(1.0f + m[8] - m[0] - m[4]) * 2.0f;
        q = { (m[3] - m[1]) / s, (m[2] + m[6]) / s, (m[5] + m[7]) / s, 0.25f * s };
    }
    return q;
}
inline Rot aRot(Cuat q) {
    const float w = q.w, x = q.x, y = q.y, z = q.z;
    return { { 1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
               2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
               2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y) } };
}
inline Rot medioGiro(const Rot& a, const Rot& b) {
    const Cuat p = aCuat(a);
    Cuat q = aCuat(b);
    if (p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z < 0.0f) q = { -q.w, -q.x, -q.y, -q.z };
    Cuat m = { p.w + q.w, p.x + q.x, p.y + q.y, p.z + q.z };
    const float l = std::sqrt(m.w * m.w + m.x * m.x + m.y * m.y + m.z * m.z);
    if (l < 1e-6f) return a;
    m = { m.w / l, m.x / l, m.y / l, m.z / l };
    return aRot(m);
}
} // namespace detalle

// Los huesos de un esqueleto ya colocado. Mismo orden siempre: el de la pose
// de referencia y el de la actual se emparejan por indice.
inline void huesosDe(const Esqueleto& s, Hueso* h) {
    using detalle::medioGiro;
    // El giro a una fraccion entre dos marcos (slerp por mitades sucesivas).
    auto fraccion = [](const Rot& a, const Rot& b, float t) {
        detalle::Cuat p = detalle::aCuat(a), q = detalle::aCuat(b);
        if (p.w * q.w + p.x * q.x + p.y * q.y + p.z * q.z < 0.0f) q = { -q.w, -q.x, -q.y, -q.z };
        detalle::Cuat m = { p.w + (q.w - p.w) * t, p.x + (q.x - p.x) * t, p.y + (q.y - p.y) * t, p.z + (q.z - p.z) * t };
        const float l = std::sqrt(m.w * m.w + m.x * m.x + m.y * m.y + m.z * m.z);
        if (l < 1e-6f) return a;
        return detalle::aRot({ m.w / l, m.x / l, m.y / l, m.z / l });
    };
    auto en = [](const Junta& j, V d) { return suma(j.p, j.r.aplicar(d)); };

    // --- LA COLUMNA ---
    const Junta& pel = s.pelvis;
    h[H_SACRO] = { pel, en(pel, { 0, -0.14f, -0.02f }), en(pel, { 0, 0.02f, -0.01f }), 0.14f, -1 };
    h[H_COCCIX] = { pel, en(pel, { 0, -0.16f, -0.08f }), en(pel, { 0, -0.11f, -0.10f }), 0.05f, H_SACRO };
    // 17 vertebras (5 lumbares y 12 toracicas) de la pelvis al cuello, y cada
    // una con su parte del giro del tronco: las lumbares poco, las toracicas
    // mas (como columnaGiro).
    const V baseCol = pel.p;
    const V topeCol = suma(s.cuello.p, s.pecho.r.aplicar({ 0, -0.03f, 0 }));
    for (int i = 0; i < 17; ++i) {
        const float t0 = (float)i / 17.0f, t1 = (float)(i + 1) / 17.0f, tm = 0.5f * (t0 + t1);
        const float u = sujetar(tm * 1.15f, 0.0f, 1.0f);
        const Rot r = fraccion(pel.r, s.pecho.r, u * u * (3.0f - 2.0f * u));
        const V a = mezclar(baseCol, topeCol, t0), b = mezclar(baseCol, topeCol, t1);
        h[H_VERTEBRA0 + i] = { { a, r }, a, b, i < 5 ? 0.14f : 0.13f, i == 0 ? H_SACRO : H_VERTEBRA0 + i - 1 };
    }
    // 7 cervicales: del cuello a la base del craneo, girando de a poco hacia
    // la cabeza.
    const V arribaCabeza = s.cuello.r.aplicar({ 0, 1, 0 });
    for (int i = 0; i < 7; ++i) {
        const float t0 = (float)i / 7.0f, t1 = (float)(i + 1) / 7.0f;
        const Rot r = fraccion(s.pecho.r, s.cuello.r, 0.5f * (t0 + t1));
        const V a = suma(s.cuello.p, escala(arribaCabeza, 0.10f * t0)), b = suma(s.cuello.p, escala(arribaCabeza, 0.10f * t1));
        h[H_CERVICAL0 + i] = { { a, r }, a, b, 0.065f, i == 0 ? H_VERTEBRA0 + 16 : H_CERVICAL0 + i - 1 };
    }
    const int C1 = H_CERVICAL0 + 6;
    h[H_HIOIDES] = { { suma(s.cuello.p, escala(arribaCabeza, 0.04f)), fraccion(s.pecho.r, s.cuello.r, 0.5f) },
                     en(s.cuello, { -0.015f, 0.04f, 0.05f }), en(s.cuello, { 0.015f, 0.04f, 0.05f }), 0.025f, H_CERVICAL0 + 3 };
    // El esternon, con la vertebra de su altura (T6).
    const Hueso& t6 = h[H_VERTEBRA0 + 5 + 6];
    const float Le = s.largo.espalda;
    h[H_ESTERNON] = { t6.marco, suma(pel.p, t6.marco.r.aplicar({ 0, Le * 0.55f, 0.11f })),
                      suma(pel.p, t6.marco.r.aplicar({ 0, Le * 0.90f, 0.10f })), 0.06f, H_VERTEBRA0 + 5 + 6 };

    // --- EL CRANEO (todos con el marco de la cabeza) ---
    const Junta& cab = s.cuello;
    auto craneo = [&](int b, V a, V z, float r) { h[b] = { cab, en(cab, a), en(cab, z), r, C1 }; };
    craneo(H_FRONTAL, { 0, 0.17f, 0.07f }, { 0, 0.12f, 0.09f }, 0.05f);
    craneo(H_OCCIPITAL, { 0, 0.12f, -0.07f }, { 0, 0.07f, -0.06f }, 0.06f);
    craneo(H_ESFENOIDES, { -0.02f, 0.11f, 0.0f }, { 0.02f, 0.11f, 0.0f }, 0.03f);
    craneo(H_ETMOIDES, { 0, 0.12f, 0.05f }, { 0, 0.115f, 0.065f }, 0.015f);
    craneo(H_MANDIBULA, { 0, 0.03f, 0.085f }, { 0, 0.07f, 0.0f }, 0.04f);
    craneo(H_VOMER, { 0, 0.085f, 0.06f }, { 0, 0.095f, 0.075f }, 0.01f);

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        auto L = [&](int k) { return huesoDe(l, k); };
        auto H = [&](int k) -> Hueso& { return h[L(k)]; };
        const Mano& m = s.mano[l];
        const Pie& pie = s.pie[l];

        // Cara y oido, con la cabeza.
        auto par = [&](int k, V a, V z, float r, int padre) { H(k) = { cab, en(cab, { sx * a.x, a.y, a.z }), en(cab, { sx * z.x, z.y, z.z }), r, padre }; };
        par(HL_PARIETAL, { 0.03f, 0.19f, 0.0f }, { 0.07f, 0.15f, -0.02f }, 0.05f, H_FRONTAL);
        par(HL_TEMPORAL, { 0.07f, 0.12f, 0.0f }, { 0.07f, 0.08f, -0.02f }, 0.035f, H_OCCIPITAL);
        par(HL_MAXILAR, { 0.02f, 0.075f, 0.08f }, { 0.03f, 0.06f, 0.07f }, 0.025f, H_ESFENOIDES);
        par(HL_PALATINO, { 0.012f, 0.065f, 0.05f }, { 0.012f, 0.065f, 0.06f }, 0.01f, L(HL_MAXILAR));
        par(HL_CIGOMATICO, { 0.05f, 0.095f, 0.06f }, { 0.06f, 0.09f, 0.04f }, 0.02f, L(HL_TEMPORAL));
        par(HL_NASAL, { 0.006f, 0.115f, 0.095f }, { 0.006f, 0.105f, 0.10f }, 0.008f, H_FRONTAL);
        par(HL_LAGRIMAL, { 0.028f, 0.115f, 0.08f }, { 0.03f, 0.11f, 0.078f }, 0.006f, H_ETMOIDES);
        par(HL_CORNETE, { 0.01f, 0.095f, 0.075f }, { 0.012f, 0.09f, 0.07f }, 0.006f, H_ETMOIDES);
        par(HL_MARTILLO, { 0.06f, 0.105f, -0.010f }, { 0.062f, 0.105f, -0.010f }, 0.003f, L(HL_TEMPORAL));
        par(HL_YUNQUE, { 0.063f, 0.105f, -0.012f }, { 0.064f, 0.104f, -0.012f }, 0.003f, L(HL_MARTILLO));
        par(HL_ESTRIBO, { 0.065f, 0.104f, -0.011f }, { 0.066f, 0.104f, -0.011f }, 0.002f, L(HL_YUNQUE));

        // Las costillas: cada una con su vertebra toracica, rodeando el
        // tronco desde la espalda hacia el costado y delante.
        for (int k = 0; k < 12; ++k) {
            const int v = H_VERTEBRA0 + 5 + (11 - k);       // T1 arriba .. T12 abajo
            const Hueso& vb = h[v];
            const V c = mezclar(vb.a, vb.b, 0.5f);
            const float ancho = 0.10f + 0.03f * std::sin((float)k / 11.0f * 3.1416f);
            H(HL_COSTILLA0 + k) = { vb.marco, suma(c, vb.marco.r.aplicar({ sx * 0.03f, 0, -0.07f })),
                                    suma(c, vb.marco.r.aplicar({ sx * ancho, -0.03f, 0.04f })), 0.05f, v };
        }

        // --- EL BRAZO ---
        H(HL_CLAVICULA) = { s.pecho, en(s.pecho, { 0, 0, 0 }), s.hombro[l].p, 0.045f, H_ESTERNON };
        H(HL_CLAVICULA).a = suma(s.cuello.p, s.pecho.r.aplicar({ sx * 0.02f, -0.05f, 0.04f }));
        // La escapula: en el hombro, con medio giro del brazo (el ritmo
        // escapulohumeral).
        H(HL_ESCAPULA) = { { s.hombro[l].p, medioGiro(s.pecho.r, s.hombro[l].r) }, s.hombro[l].p,
                           suma(s.hombro[l].p, s.pecho.r.aplicar({ -sx * 0.06f, -0.07f, -0.05f })), 0.055f, L(HL_CLAVICULA) };
        H(HL_HUMERO) = { s.hombro[l], s.hombro[l].p, s.codo[l].p, 0.090f, L(HL_ESCAPULA) };
        // El cubito envuelve el codo: medio giro del codo, en su primer tercio.
        const V antebrazo = resta(m.muneca.p, s.codo[l].p);
        H(HL_CUBITO) = { { s.codo[l].p, medioGiro(s.hombro[l].r, s.codo[l].r) }, s.codo[l].p,
                         suma(s.codo[l].p, escala(antebrazo, 0.30f)), 0.055f, L(HL_HUMERO) };
        // El radio: el antebrazo con media pronacion (se tuerce a lo largo).
        H(HL_RADIO) = { { s.codo[l].p, s.codo[l].r.por(Rot::enY(s.prono[l] * 0.5f)) },
                        suma(s.codo[l].p, escala(antebrazo, 0.15f)), m.muneca.p, 0.045f, L(HL_CUBITO) };
        // Los carpos: la primera fila con medio giro de la muneca; la segunda,
        // con la mano.
        const Rot mitadMuneca = medioGiro(H(HL_RADIO).marco.r, m.muneca.r);
        static const float CX[4] = { 0.018f, 0.006f, -0.006f, -0.018f };
        for (int k = 0; k < 4; ++k) {
            const V p0 = suma(m.muneca.p, m.muneca.r.aplicar({ m.lado * CX[k], -0.006f, 0 }));
            H(HL_CARPO0 + k) = { { m.muneca.p, mitadMuneca }, p0, p0, 0.022f, L(HL_RADIO) };
            const V p1 = suma(m.muneca.p, m.muneca.r.aplicar({ m.lado * CX[k], -0.024f, 0 }));
            H(HL_CARPO0 + 4 + k) = { m.muneca, p1, p1, 0.020f, L(HL_CARPO0 + k) };
        }
        // Los metacarpos: de la segunda fila del carpo a los nudillos; el
        // del pulgar es el primer tramo del pulgar del esqueleto.
        for (int d = 0; d < 4; ++d)
            H(HL_METACARPO0 + d) = { m.muneca, suma(m.muneca.p, m.muneca.r.aplicar({ m.lado * CX[d] * 0.8f, -0.03f, 0 })),
                                     m.dedo[d][0].p, 0.020f, L(HL_CARPO0 + 4 + (d < 2 ? 2 : 3)) };
        H(HL_METACARPO0 + 4) = { m.dedo[4][0], m.dedo[4][0].p, m.dedo[4][1].p, 0.014f, L(HL_CARPO0 + 4) };
        // Las falanges: 3 por dedo largo y 2 del pulgar.
        for (int d = 0; d < 4; ++d)
            for (int f = 0; f < 3; ++f)
                H(HL_FALANGE_MANO0 + d * 3 + f) = { m.dedo[d][f], m.dedo[d][f].p, m.dedo[d][f + 1].p, 0.0095f,
                                                    f == 0 ? L(HL_METACARPO0 + d) : L(HL_FALANGE_MANO0 + d * 3 + f - 1) };
        for (int f = 0; f < 2; ++f)
            H(HL_FALANGE_MANO0 + 12 + f) = { m.dedo[4][f + 1], m.dedo[4][f + 1].p, m.dedo[4][f + 2].p, 0.011f,
                                             f == 0 ? L(HL_METACARPO0 + 4) : L(HL_FALANGE_MANO0 + 12) };

        // --- LA PIERNA ---
        // El coxal: en la cadera, con medio giro del muslo.
        H(HL_COXAL) = { { s.cadera[l].p, medioGiro(pel.r, s.cadera[l].r) }, s.cadera[l].p,
                        suma(s.cadera[l].p, pel.r.aplicar({ -sx * 0.02f, 0.08f, -0.02f })), 0.10f, H_SACRO };
        H(HL_FEMUR) = { s.cadera[l], s.cadera[l].p, s.rodilla[l].p, 0.085f, L(HL_COXAL) };
        // La rotula: en la rodilla, con medio giro.
        H(HL_ROTULA) = { { s.rodilla[l].p, medioGiro(s.cadera[l].r, s.rodilla[l].r) }, s.rodilla[l].p, s.rodilla[l].p, 0.07f, L(HL_FEMUR) };
        H(HL_TIBIA) = { s.rodilla[l], s.rodilla[l].p, pie.tobillo.p, 0.060f, L(HL_ROTULA) };
        H(HL_PERONE) = { s.rodilla[l], en(s.rodilla[l], { sx * 0.03f, -0.05f, -0.01f }), en(pie.tobillo, { sx * 0.03f, 0.02f, 0 }), 0.035f, L(HL_TIBIA) };
        // Los tarsos: el astragalo con medio giro del tobillo; los demas, con el pie.
        H(HL_TARSO0) = { { pie.tobillo.p, medioGiro(s.rodilla[l].r, pie.tobillo.r) }, pie.tobillo.p, pie.tobillo.p, 0.05f, L(HL_TIBIA) };
        H(HL_TARSO0 + 1) = { pie.tobillo, pie.tobillo.p, pie.talon, 0.045f, L(HL_TARSO0) };
        const V medio = mezclar(pie.tobillo.p, pie.bola, 0.45f);
        const float dentro = -sx;
        H(HL_TARSO0 + 2) = { pie.tobillo, medio, suma(medio, pie.tobillo.r.aplicar({ dentro * 0.015f, 0, 0.01f })), 0.03f, L(HL_TARSO0) };
        H(HL_TARSO0 + 3) = { pie.tobillo, suma(medio, pie.tobillo.r.aplicar({ -dentro * 0.025f, 0, 0 })),
                             suma(medio, pie.tobillo.r.aplicar({ -dentro * 0.025f, 0, 0.02f })), 0.03f, L(HL_TARSO0 + 1) };
        for (int k = 0; k < 3; ++k) {
            const V c = suma(medio, pie.tobillo.r.aplicar({ dentro * (0.02f - k * 0.015f), 0, 0.025f }));
            H(HL_TARSO0 + 4 + k) = { pie.tobillo, c, suma(c, pie.tobillo.r.aplicar({ 0, 0, 0.015f })), 0.022f, L(HL_TARSO0 + 2) };
        }
        // Los metatarsos: del mediopie a la base de cada dedo.
        for (int d = 0; d < 5; ++d) {
            const int desde = d < 3 ? HL_TARSO0 + 4 + d : HL_TARSO0 + 3;
            H(HL_METATARSO0 + d) = { pie.tobillo, H(desde).b, pie.dedo[d][0].p, 0.022f, L(desde) };
        }
        // Las falanges: el gordo con 2; los otros con 3 (la del medio y la
        // ultima comparten el marco de la punta del esqueleto).
        H(HL_FALANGE_PIE0) = { pie.dedo[0][0], pie.dedo[0][0].p, pie.dedo[0][1].p, 0.016f, L(HL_METATARSO0) };
        H(HL_FALANGE_PIE0 + 1) = { pie.dedo[0][1], pie.dedo[0][1].p, pie.dedo[0][2].p, 0.016f, L(HL_FALANGE_PIE0) };
        for (int d = 1; d < 5; ++d) {
            const int b0 = HL_FALANGE_PIE0 + 2 + (d - 1) * 3;
            const V mitad = mezclar(pie.dedo[d][1].p, pie.dedo[d][2].p, 0.5f);
            H(b0) = { pie.dedo[d][0], pie.dedo[d][0].p, pie.dedo[d][1].p, 0.011f, L(HL_METATARSO0 + d) };
            H(b0 + 1) = { pie.dedo[d][1], pie.dedo[d][1].p, mitad, 0.011f, L(b0) };
            H(b0 + 2) = { pie.dedo[d][1], mitad, pie.dedo[d][2].p, 0.010f, L(b0 + 1) };
        }
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
inline float suaveRadio(float d, float r) {
    const float u = sujetar(1.0f - d / r, 0.0f, 1.0f);
    return u * u * (3.0f - 2.0f * u);
}
} // namespace detalle

// ----------------------------------------------------------------------------
// LO QUE EL AVATAR LE HACE AL MODELO
// ----------------------------------------------------------------------------
// Factores (1 = como el modelo) y empujes (0 = nada). Ver aplicarAvatarMalla
// en CabezaJugador.h, que los saca de los deslizadores.
struct AjustesMalla {
    // La escala de cada parte (grosor; tamano en cabeza, manos y pies).
    float cabeza = 1, cuello = 1, hombros = 1, brazo = 1, biceps = 1, antebrazo = 1, mano = 1;
    float pecho = 1, cintura = 1, cadera = 1, muslo = 1, gemelo = 1, pie = 1;
    // La cara y el cuerpo por zonas.
    float caraAncho = 1, caraLargo = 1, frente = 1, pomulos = 1, mandibula = 1, menton = 1;
    float ojos = 1, ojosSeparacion = 1, ojosAltura = 1, cejasAltura = 1;
    float narizLargo = 1, narizAncho = 1, narizPuente = 1, bocaAncho = 1, labios = 1;
    float orejas = 1, orejasDespegadas = 0, cuelloLargo = 1;
    float busto = 0, abdomen = 0, gluteos = 1;
    // Los de la segunda tanda (ver Avatar.h): 1 o 0 = nada.
    float craneo = 1, papada = 0, sienes = 1, ojosProfundidad = 1, ojosInclinacion = 0, cejasArco = 0, cejasGrosor = 1;
    float narizPunta = 0, narizAletas = 1, narizAltura = 1, bocaAltura = 1, sonrisa = 0, bocaSaliente = 0;
    float orejasAltura = 1, orejasPunta = 0, lobulos = 1, peloVolumen = 1, nuez = 0.3f, trapecio = 1;
    float triceps = 1, dedosLargo = 1, espalda = 1, pectoral = 0.2f, cuadriceps = 1, pieDedos = 1, cinturaAltura = 0;
    // El color que pide el avatar en cada zona y cuanto se usa (0 = el del
    // modelo tal cual; 1 = ese color con el sombreado de la textura).
    float piel[3] = { 1, 1, 1 }, pelo[3] = { 1, 1, 1 }, camisa[3] = { 1, 1, 1 }, shortC[3] = { 1, 1, 1 };
    float mezcla[4] = { 0, 0, 0, 0 };
    bool operator==(const AjustesMalla& o) const { return std::memcmp(this, &o, sizeof(*this)) == 0; }
};

struct MallaImportada {
    bool cargada = false;
    int formato = 1;                     // 1: color por vertice, 2: con uv y tangentes
    float params[32] = {};
    std::vector<VerticeJ> modelo;        // como viene del archivo (pose de referencia)
    std::vector<VerticeJ> base;          // con los ajustes de la cara y el cuerpo
    std::vector<float> uv, tangente;     // 2 y 4 por vertice (formato 2)
    std::vector<uint32_t> indices, indicesSinCabeza;
    std::vector<uint8_t> h0, h1;         // los dos huesos de cada vertice

    // ========================================================================
    // ⭐ LOS HUESOS DE LOS DEDOS DEL MODELO DE BLOQUES
    // ========================================================================
    // PEDIDO: "dale huesos de animacion y que las animaciones de las manos
    // tengan a los dedos de las manos".
    //
    // En el modelo de bloques la mano es una pieza con el antebrazo (ver
    // huesoRigido): atada a los huesos de la mano del esqueleto se despegaba,
    // porque los nudillos del esqueleto no caen donde estan los del modelo.
    // Asi que los dedos iban tiesos: el puno se cerraba en el esqueleto y en
    // pantalla no pasaba nada.
    //
    // Ahora cada bloque de dedo tiene SU hueso, medido en la propia malla:
    //   - un bloque de la primera fila (pegado a la palma) gira por su nudillo:
    //     una bisagra en su extremo de arriba, a lo ancho de la mano;
    //   - un bloque de la segunda fila (falanges y puntas) gira primero con el
    //     nudillo del bloque que tiene encima y luego por su propia bisagra.
    // La bisagra esta donde el bloque se une a lo anterior, asi que no puede
    // despegarse. El hueso cuelga del antebrazo: la mano sigue siendo una
    // pieza con el, y encima los dedos se abren y se cierran.
    //
    // CUANTO se doblan lo dicen las animaciones de siempre (Brazo::dedo): se
    // lee del esqueleto el angulo de cada dedo y de su segunda falange, y cada
    // bloque toma el del dedo que le cae encima a lo ancho de la mano. Se
    // dobla solo una parte (GANANCIA) y con tope: son bloques gruesos, y un
    // puno cerrado del todo los meteria unos dentro de otros.
    //
    // El pulgar y la palma no tienen hueso propio: se quedan con el antebrazo.
    // h0/h1 no cambian (todo lo que mide y moldea la malla sigue viendo el
    // antebrazo); el hueso del dedo solo cuenta al DIBUJAR (huesoDeDibujo).
    static constexpr int MAX_DEDOS = 24;
    static constexpr int NUM_HUESOS_MALLA = NUM_HUESOS + MAX_DEDOS;
    static constexpr float DEDO_GANANCIA = 0.55f;
    static constexpr float DEDO_ABRE_MAX = 0.30f, DEDO_CIERRA_MAX = 0.60f;   // radianes
    struct HuesoDedo {
        int lado = 0;
        int nivel = 1;                   // 1: gira por el nudillo; 2: ademas por su falange
        int padre = -1;                  // (nivel 2) el bloque de la primera fila del que cuelga
        int dA = 0, dB = 0;              // los dos dedos del esqueleto entre los que cae
        float peso = 1.0f;               // cuanto del primero
        V nudillo, falange, eje;         // en `modelo` (la pose de referencia)
        V nudilloB, falangeB, ejeB;      // en `base` (con el avatar aplicado)
    };
    HuesoDedo dedos[MAX_DEDOS];
    int numDedos = 0;
    std::vector<uint8_t> dedoDe;         // por vertice: 0 ninguno, o el numero de su hueso de dedo + 1
    float dedoRef[2][4][2] = {};         // los angulos de los dedos en la pose de referencia
    int huesoDeDibujo(size_t i) const {
        return i < dedoDe.size() && dedoDe[i] ? NUM_HUESOS + dedoDe[i] - 1 : h0[i];
    }
    int huesoDeDibujo1(size_t i) const {
        return i < dedoDe.size() && dedoDe[i] ? NUM_HUESOS + dedoDe[i] - 1 : h1[i];
    }
    // Cuanto esta doblado cada dedo largo de esa mano: [dedo][0] el nudillo y
    // [dedo][1] la segunda falange respecto a la primera. Positivo, hacia la palma.
    static void angulosDeDedos(const Esqueleto& s, int l, float out[4][2]) {
        const Mano& m = s.mano[l];
        const Rot rt = detalle::traspuesta(m.muneca.r);
        for (int d = 0; d < 4; ++d) {
            const V a = rt.aplicar(resta(m.dedo[d][1].p, m.dedo[d][0].p));
            const V b = rt.aplicar(resta(m.dedo[d][2].p, m.dedo[d][1].p));
            const float t0 = std::atan2(a.z, -a.y), t1 = std::atan2(b.z, -b.y);
            out[d][0] = t0;
            out[d][1] = t1 - t0;
        }
    }
    // Lo que gira cada bisagra de ese hueso de dedo con el esqueleto `s`.
    void giroDeDedo(const Esqueleto& s, int e, float& nudillo, float& falange) const {
        float ang[4][2];
        const HuesoDedo& h = dedos[e];
        angulosDeDedos(s, h.lado, ang);
        auto de = [&](const HuesoDedo& x, int k) {
            const float a = ang[x.dA][k] - dedoRef[x.lado][x.dA][k], b = ang[x.dB][k] - dedoRef[x.lado][x.dB][k];
            return sujetar(DEDO_GANANCIA * (a * x.peso + b * (1.0f - x.peso)), -DEDO_ABRE_MAX, DEDO_CIERRA_MAX);
        };
        nudillo = de(h.nivel == 2 && h.padre >= 0 ? dedos[h.padre] : h, 0);
        falange = h.nivel == 2 ? de(h, 1) : 0.0f;
    }
    // Giro de `ang` radianes alrededor del eje unitario `a`, en el sentido de
    // Rot::enX: positivo lleva lo que cuelga (-Y) hacia delante (+Z).
    static Rot giroEnEje(V a, float ang) {
        const float c = std::cos(-ang), sn = std::sin(-ang), k = 1.0f - c;
        return { { c + a.x * a.x * k,        a.x * a.y * k - a.z * sn, a.x * a.z * k + a.y * sn,
                   a.y * a.x * k + a.z * sn, c + a.y * a.y * k,        a.y * a.z * k - a.x * sn,
                   a.z * a.x * k - a.y * sn, a.z * a.y * k + a.x * sn, c + a.z * a.z * k } };
    }
    void atarHuesosDeDedos(const Esqueleto& s) {
        numDedos = 0;
        dedoDe.assign(modelo.size(), 0);
        for (int l = 0; l < 2; ++l) angulosDeDedos(s, l, dedoRef[l]);
        if (!esDeBloques()) return;
        std::vector<int> pieza;
        const int numPiezas = piezasDeLaMalla(pieza);
        struct Caja { V lo{ 1e9f, 1e9f, 1e9f }, hi{ -1e9f, -1e9f, -1e9f }; int n = 0, delRadio = 0; };
        for (int l = 0; l < 2; ++l) {
            const Junta& j = s.mano[l].muneca;
            const Rot rt = detalle::traspuesta(j.r);
            const int radio = huesoDe(l, HL_RADIO);
            // Cada pieza en el marco de la mano: los dedos van hacia -Y, la
            // palma mira a +Z y X es el ancho.
            std::vector<Caja> caja(numPiezas);
            for (size_t i = 0; i < modelo.size(); ++i) {
                Caja& c = caja[pieza[i]];
                ++c.n;
                if (h0[i] == radio && w0[i] >= 1.0f) ++c.delRadio;
                const V q = rt.aplicar(resta(modelo[i].p, j.p));
                c.lo = { std::fmin(c.lo.x, q.x), std::fmin(c.lo.y, q.y), std::fmin(c.lo.z, q.z) };
                c.hi = { std::fmax(c.hi.x, q.x), std::fmax(c.hi.y, q.y), std::fmax(c.hi.z, q.z) };
            }
            auto esDeLaMano = [&](const Caja& c) { return c.n > 0 && c.delRadio == c.n && c.hi.y < 0.05f; };
            // Hasta donde llega la mano, desde la muneca.
            float alcance = 0.0f;
            for (const Caja& c : caja) if (esDeLaMano(c)) alcance = std::fmax(alcance, -c.lo.y);
            if (alcance < 0.10f) continue;
            // Los bloques de dedo: los que EMPIEZAN pasada la mitad de la mano
            // (la palma y el dorso empiezan antes), menos el pulgar, que queda
            // entero del lado de la palma.
            const int primero = numDedos;
            std::vector<int> deLaPieza(numPiezas, -1);
            for (int vuelta = 1; vuelta <= 2; ++vuelta) {
                for (int q = 0; q < numPiezas && numDedos < MAX_DEDOS; ++q) {
                    const Caja& c = caja[q];
                    if (!esDeLaMano(c) || c.hi.y > -0.48f * alcance || c.lo.z > 0.012f) continue;
                    const int nivel = c.hi.y > -0.66f * alcance ? 1 : 2;
                    if (nivel != vuelta) continue;
                    HuesoDedo h;
                    h.lado = l;
                    h.nivel = nivel;
                    const float cx = 0.5f * (c.lo.x + c.hi.x), cz = 0.5f * (c.lo.z + c.hi.z);
                    const V bisagra = suma(j.p, j.r.aplicar({ cx, c.hi.y, cz }));
                    h.eje = normalizar(j.r.aplicar({ 1, 0, 0 }));
                    // Entre que dos dedos del esqueleto cae, a lo ancho.
                    float xs[4];
                    for (int d = 0; d < 4; ++d) xs[d] = rt.aplicar(resta(s.mano[l].dedo[d][0].p, j.p)).x;
                    h.dA = h.dB = 0;
                    h.peso = 1.0f;
                    const float sentido = xs[3] > xs[0] ? 1.0f : -1.0f;
                    if ((cx - xs[3]) * sentido >= 0.0f) h.dA = h.dB = 3;
                    else for (int d = 0; d < 3; ++d) {
                        if ((cx - xs[d]) * sentido >= 0.0f && (cx - xs[d + 1]) * sentido < 0.0f) {
                            h.dA = d; h.dB = d + 1;
                            h.peso = 1.0f - (cx - xs[d]) / (xs[d + 1] - xs[d]);
                        }
                    }
                    if (nivel == 1) {
                        h.nudillo = h.falange = bisagra;
                    } else {
                        h.falange = bisagra;
                        // Cuelga del bloque de la primera fila que tiene mas
                        // cerca a lo ancho; si no hay ninguno, de una bisagra
                        // puesta donde empiezan los dedos.
                        float mejor = 1e9f;
                        for (int e = primero; e < numDedos; ++e) {
                            if (dedos[e].nivel != 1) continue;
                            const float x = rt.aplicar(resta(dedos[e].nudillo, j.p)).x;
                            if (std::fabs(x - cx) < mejor) { mejor = std::fabs(x - cx); h.padre = e; }
                        }
                        h.nudillo = h.padre >= 0 ? dedos[h.padre].nudillo
                                                 : suma(j.p, j.r.aplicar({ cx, -0.52f * alcance, cz }));
                    }
                    h.nudilloB = h.nudillo; h.falangeB = h.falange; h.ejeB = h.eje;
                    deLaPieza[q] = numDedos;
                    dedos[numDedos++] = h;
                }
            }
            for (size_t i = 0; i < modelo.size(); ++i)
                if (deLaPieza[pieza[i]] >= 0) dedoDe[i] = (uint8_t)(deLaPieza[pieza[i]] + 1);
        }
    }
    std::vector<float> w0;               // el peso del primero (el segundo, 1 - w0)
    Hueso ref[NUM_HUESOS];               // la pose de referencia del modelo, tal cual
    Hueso reposo[NUM_HUESOS];            // la misma con las proporciones del avatar
    Proporciones propReposo;
    AjustesMalla ajustes;
    int versionForma = 0;                // sube cada vez que cambia `base`

    // Los puntos de la cara y del cuerpo, en la pose de referencia.
    struct Puntos {
        V cabeza, nariz, menton, boca, ojo[2], oreja[2], cuelloBase, pecho[2], vientre, gluteo[2];
        float cabezaRadio = 0.11f;
        float cabezaMedioZ = 0.0f;       // el medio de la cabeza, de la cara a la nuca
    } pt;

    Proporciones proporciones() const {
        Proporciones p;
        p.torso = params[2]; p.brazo = params[3]; p.antebrazo = params[4];
        p.muslo = params[5]; p.pierna = params[6]; p.hombroY = params[7];
        p.mano = params[21] > 0.3f ? params[21] : 1.0f;
        return p;
    }
    // La pose en que esta hecho el modelo: en cruz, los brazos algo caidos.
    Pose poseReferencia() const {
        Pose p;
        p.prop = proporciones();
        for (int l = 0; l < 2; ++l) {
            Brazo& b = p.brazo[l];
            b = Brazo{};
            b.abd = params[0]; b.flex = refBrazo[l][0]; b.rot = 0.0f; b.codo = refBrazo[l][1];
            b.prono = 0.0f; b.munFlex = refBrazo[l][2]; b.munDesv = refBrazo[l][3];
        }
        for (int l = 0; l < 2; ++l) {
            Pierna& g = p.pierna[l];
            g = Pierna{};
            g.abd = params[1] + refPierna[l][2]; g.flex = refPierna[l][0]; g.rodilla = refPierna[l][1];
            g.tobillo = refPierna[l][3];
        }
        return p;
    }
    // El color medio de cada zona en la textura (para recolorear).
    const float* colorZona(int k) const { return &params[9 + k * 3]; }

    bool cargar(const std::string& ruta) {
        cargada = false;
        FILE* f = std::fopen(ruta.c_str(), "rb");
        if (!f) return false;
        char magia[4];
        uint32_t nv = 0, nt = 0;
        bool ok = std::fread(magia, 1, 4, f) == 4 && std::fread(&nv, 4, 1, f) == 1 && std::fread(&nt, 4, 1, f) == 1;
        formato = ok && std::memcmp(magia, "VWM2", 4) == 0 ? 2 : (ok && std::memcmp(magia, "VWM1", 4) == 0 ? 1 : 0);
        ok = ok && formato != 0 && nv > 0 && nv < 2000000 && nt > 0 && nt < 4000000;
        const int nParams = formato == 2 ? 32 : 16;
        const int porVertice = formato == 2 ? 12 : 9;
        std::memset(params, 0, sizeof(params));
        std::vector<float> v;
        if (ok) {
            ok = std::fread(params, 4, nParams, f) == (size_t)nParams;
            v.resize((size_t)nv * porVertice);
            indices.resize((size_t)nt * 3);
            ok = ok && std::fread(v.data(), 4, v.size(), f) == v.size() &&
                 std::fread(indices.data(), 4, indices.size(), f) == indices.size();
        }
        std::fclose(f);
        if (!ok) return false;
        for (uint32_t i : indices) if (i >= nv) return false;
        modelo.resize(nv);
        uv.assign(formato == 2 ? (size_t)nv * 2 : 0, 0.0f);
        tangente.assign(formato == 2 ? (size_t)nv * 4 : 0, 0.0f);
        for (uint32_t i = 0; i < nv; ++i) {
            const float* q = &v[(size_t)i * porVertice];
            if (formato == 2) {
                modelo[i] = { { q[0], q[1], q[2] }, { q[3], q[4], q[5] }, 0.8f, 0.7f, 0.6f };
                uv[i * 2] = q[6]; uv[i * 2 + 1] = q[7];
                for (int k = 0; k < 4; ++k) tangente[i * 4 + k] = q[8 + k];
            } else {
                modelo[i] = { { q[0], q[1], q[2] }, { q[3], q[4], q[5] }, q[6], q[7], q[8] };
            }
        }
        atar();
        base = modelo;
        ajustes = AjustesMalla{};
        ++versionForma;
        cargada = true;
        return true;
    }

    // Los pesos de cada vertice contra el esqueleto en la pose de referencia,
    // y los puntos de la cara y el cuerpo.
    void atar() {
        // El esqueleto del juego va M::ATRAS por detras del ojo; la malla
        // viene centrada sobre los pies. Se mueve para que el eje del tronco
        // caiga en la columna del esqueleto.
        for (auto& r : refBrazo) for (float& x : r) x = 0.0f;
        for (auto& r : refPierna) for (float& x : r) x = 0.0f;
        {
            const Esqueleto s = resolver(poseReferencia(), true);
            double zx = 0.0, zz = 0.0;
            int cuantos = 0;
            for (const VerticeJ& v : modelo)
                if (v.p.y > 1.00f && v.p.y < 1.30f && std::fabs(v.p.x) < 0.15f) { zx += v.p.x; zz += v.p.z; ++cuantos; }
            if (cuantos > 0) {
                const float dx = s.pelvis.p.x - (float)(zx / cuantos), dz = s.pelvis.p.z - (float)(zz / cuantos);
                for (VerticeJ& v : modelo) { v.p.x += dx; v.p.z += dz; }
            }
        }
        ajustarBrazos();
        if (ajustarLargoAntebrazo()) ajustarBrazos();      // con el largo nuevo, otra vez
        ajustarPiernas();
        // ⭐ LA MANO DEL ESQUELETO A LA MEDIDA DE LA DEL MODELO: en la cruz,
        // la punta del dedo medio tiene que llegar a donde acaba la mano del
        // modelo. Si no, los nudillos del esqueleto caen en mitad de los
        // dedos de la malla y al cerrar el puno los dedos se rompen.
        // Se mide a lo largo de la mano del esqueleto, que ya sigue a la de
        // la malla (dos vueltas: el largo y hacia donde va se ajustan juntos).
        for (int vuelta = 0; vuelta < 2; ++vuelta) {
            const Esqueleto s0 = resolver(poseReferencia(), true);
            const V muneca = s0.mano[1].muneca.p;
            const V eje = resta(s0.mano[1].dedo[1][3].p, muneca);
            const float dedo = largo(eje);
            const V dir = escala(eje, 1.0f / std::fmax(dedo, 1e-4f));
            float punta = 0.0f;
            for (const VerticeJ& v : modelo) {
                const V d = resta(v.p, muneca);
                const float a = d.x * dir.x + d.y * dir.y + d.z * dir.z;
                if (a > 0.0f && largo(resta(d, escala(dir, a))) < 0.06f) punta = std::fmax(punta, a);
            }
            if (dedo > 0.05f && punta > 0.05f) {
                const float base = params[21] > 0.3f ? params[21] : 1.0f;
                params[21] = sujetar(base * punta / dedo, 0.6f, 1.4f);
            }
            ajustarManos();
        }
        const Esqueleto s = resolver(poseReferencia(), true);
        medirAgarre(s);
        huesosDe(s, ref);
        for (int b = 0; b < NUM_HUESOS; ++b) reposo[b] = ref[b];
        propReposo = proporciones();
        const size_t n = modelo.size();
        h0.assign(n, 0); h1.assign(n, 0); w0.assign(n, 1.0f);
        for (size_t i = 0; i < n; ++i) {
            const V p = modelo[i].p;
            float d[NUM_HUESOS];
            int mejor = 0;
            for (int b = 0; b < NUM_HUESOS; ++b) {
                d[b] = detalle::distSegmento(p, ref[b].a, ref[b].b) / ref[b].radio;
                if (d[b] < d[mejor]) mejor = b;
            }
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
        atarPies(s);
        suavizarPesos();
        atarDedos(s);
        atarLoQueCuelgaDeLaMano(s);
        atarPiezasRigidas();
        atarHuesosDeDedos(s);
        buscarPuntos(s);
        // En primera persona la camara va en la cabeza (ver ojos()): la cabeza
        // y el cuello no se dibujan. BUG: dejandolos, al mirar abajo la vista
        // atravesaba el cuello y el cuello de la camisa por dentro. Al mirar
        // abajo el cuerpo se echa atras (ver el cuerpo en main.cpp) y la
        // camara queda delante del pecho: se ve el pecho por fuera, como una
        // persona, y lo que se quito queda detras de la camara.
        //
        // BUG: "que la cabeza sea invisible en primera persona pero que para
        // alguien si se conectara viera la cabeza normal". Con el modelo de
        // bloques la cabeza se seguia viendo casi entera: es un cubo de 30 cm
        // de fondo y sus esquinas quedan lejos de los huesos del craneo, asi
        // que se ataban al cuello o a los hombros y la regla de los huesos no
        // las quitaba (solo caian 3.020 triangulos). Ahora ademas se quita
        // TODO lo que este por encima de donde empieza la cabeza y dentro de
        // su columna: en la pose de referencia ahi arriba no hay otra cosa
        // (los brazos van en cruz o en A, por debajo). La malla ENTERA sigue
        // en `indices`: quien mire al jugador desde fuera (indicesDe(true))
        // lo ve con su cabeza.
        indicesSinCabeza.clear();
        const V entreOjos = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        // PEDIDO: "cuando te agachas la cabeza cualquier parte para la 1ra
        // persona aun sea invisible". De pie bastaba con quitar del menton
        // para arriba; agachado y tumbado el tronco se inclina, la camara
        // queda encima del cuello y se veia por dentro lo que quedaba: el
        // cuello y el cuello de la armadura. Se quita tambien eso -- los 6 cm
        // de debajo de donde empieza la cabeza --, siempre dentro de su
        // columna: los hombros, que estan mas afuera, se siguen viendo.
        const float yCabeza = yDeLaCabeza();
        const float radioColumna = std::fmax(0.22f, pt.cabezaRadio * 2.0f);
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            bool fuera = false;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = indices[t + k];
                // La cabeza del menton para arriba; el cuello y los hombros se ven.
                const bool cabeza = ((esDeLaCabeza(h0[v]) && w0[v] > 0.3f) || (esDeLaCabeza(h1[v]) && w0[v] < 0.7f)) &&
                                    modelo[v].p.y > pt.menton.y + 0.005f;
                const float dxc = modelo[v].p.x - pt.cabeza.x, dzc = modelo[v].p.z - pt.cabeza.z;
                const bool arriba = modelo[v].p.y > yCabeza && dxc * dxc + dzc * dzc < radioColumna * radioColumna;
                if (cabeza || arriba || largo(resta(modelo[v].p, entreOjos)) < 0.08f) fuera = true;
            }
            if (!fuera) indicesSinCabeza.insert(indicesSinCabeza.end(), { indices[t], indices[t + 1], indices[t + 2] });
        }
    }

    // Las piezas de la malla: los vertices unidos entre si por triangulos
    // (union-find). Deja en `pieza` el numero de la de cada vertice y devuelve
    // cuantas hay. Un modelo de piel continua es una sola (o unas pocas); uno
    // de bloques, cientos.
    int piezasDeLaMalla(std::vector<int>& pieza) const {
        std::vector<uint32_t> jefe(modelo.size());
        for (size_t i = 0; i < jefe.size(); ++i) jefe[i] = (uint32_t)i;
        auto raiz = [&](uint32_t x) {
            while (jefe[x] != x) { jefe[x] = jefe[jefe[x]]; x = jefe[x]; }
            return x;
        };
        for (size_t k = 0; k + 2 < indices.size(); k += 3) {
            const uint32_t a = raiz(indices[k]);
            for (int e = 1; e < 3; ++e) {
                const uint32_t b = raiz(indices[k + e]);
                if (a != b) jefe[b] = a;
            }
        }
        pieza.assign(modelo.size(), 0);
        int numPiezas = 0;
        std::vector<int> nombre(modelo.size(), -1);
        for (size_t i = 0; i < modelo.size(); ++i) {
            const uint32_t r = raiz((uint32_t)i);
            if (nombre[r] < 0) nombre[r] = numPiezas++;
            pieza[i] = nombre[r];
        }
        return numPiezas;
    }

    // ⭐ UN BLOQUE, UN HUESO.
    //
    // PEDIDO: "haz que las manos, brazos, piernas y pies etc esten [...] mejor
    // animados". El modelo de bloques son 324 piezas sueltas de unos 150
    // vertices. El atado esta pensado para PIEL: cada vertice se reparte entre
    // dos huesos vecinos, para que un codo se doble sin arrugarse. A un bloque
    // rigido eso lo deforma: la mitad se va con el brazo y la otra con el
    // antebrazo, y el bloque se estira en abanico (medido: andando, 2.900
    // aristas a mas del triple de su largo, alguna a mas de cien veces).
    //
    // Una pieza PEQUENA (menos de 25 cm de punta a punta: no puede cruzar una
    // articulacion entera) se ata ENTERA al hueso que mas vertices suyos tenia.
    // Se mueve como lo que es, una pieza rigida, y no puede estirarse. Las
    // piezas grandes -- y cualquier modelo de piel continua, que es una sola
    // pieza -- se quedan con su atado suave.
    //
    // Los huesos que giran "a medias" en una articulacion (cubito, rotula,
    // perone, carpos) se cuentan con su vecino entero: un bloque del antebrazo
    // va con el radio y uno de la espinilla con la tibia, no medio girados.
    static int huesoRigido(int b) {
        if (b < H_POR_LADO_INICIO) return b;
        const int lado = (b - H_POR_LADO_INICIO) / HL_NUM, k = (b - H_POR_LADO_INICIO) % HL_NUM;
        if (k == HL_CUBITO) return huesoDe(lado, HL_RADIO);
        // ⭐ LA MANO VA CON EL ANTEBRAZO.
        //
        // BUG: "las manos y munecas del personaje no estan sujetas al
        // antebrazo y cuando corro, salto, me agacho y tiro, se separan mucho
        // y se nota". Las piezas de la mano iban con los huesos de la mano,
        // que giran por la muneca. Pero la muneca del ESQUELETO no cae en la
        // union de la mano del MODELO (queda 5 cm a un lado), y ademas el
        // antebrazo solo lleva la mitad del giro de la mano (la pronacion se
        // reparte): la pinza giraba alrededor de un punto que no era su
        // union y se despegaba del antebrazo, mas cuanto mas se movia el brazo.
        //
        // En un modelo de bloques la mano no tiene una muneca blanda que lo
        // disimule: carpos, metacarpos y dedos se atan al mismo hueso que el
        // antebrazo. Mano y antebrazo son UNA pieza y no pueden separarse.
        if (k >= HL_CARPO0 && k < HL_COXAL) return huesoDe(lado, HL_RADIO);
        if (k == HL_ROTULA || k == HL_PERONE) return huesoDe(lado, HL_TIBIA);
        return b;
    }
    static constexpr float PIEZA_RIGIDA_MAX = 0.45f;     // de punta a punta, en metros
    int piezasRigidas = 0;                               // cuantas se ataron asi (diagnostico y tests)
    void atarPiezasRigidas() {
        piezasRigidas = 0;
        std::vector<int> pieza;
        const int numPiezas = piezasDeLaMalla(pieza);
        if (numPiezas < 2) return;
        std::vector<V> lo(numPiezas, V{ 1e9f, 1e9f, 1e9f }), hi(numPiezas, V{ -1e9f, -1e9f, -1e9f });
        for (size_t i = 0; i < modelo.size(); ++i) {
            const V p = modelo[i].p;
            V& a = lo[pieza[i]];
            V& b = hi[pieza[i]];
            a = { std::fmin(a.x, p.x), std::fmin(a.y, p.y), std::fmin(a.z, p.z) };
            b = { std::fmax(b.x, p.x), std::fmax(b.y, p.y), std::fmax(b.z, p.z) };
        }
        // Los votos de cada pieza: por hueso, el peso de sus vertices.
        std::vector<std::vector<uint32_t>> de(numPiezas);
        for (size_t i = 0; i < modelo.size(); ++i) de[pieza[i]].push_back((uint32_t)i);
        std::vector<float> voto(NUM_HUESOS);
        for (int q = 0; q < numPiezas; ++q) {
            if (largo(resta(hi[q], lo[q])) > PIEZA_RIGIDA_MAX) continue;
            std::fill(voto.begin(), voto.end(), 0.0f);
            for (uint32_t i : de[q]) {
                voto[huesoRigido(h0[i])] += w0[i];
                voto[huesoRigido(h1[i])] += 1.0f - w0[i];
            }
            int mejor = 0;
            for (int b = 1; b < NUM_HUESOS; ++b) if (voto[b] > voto[mejor]) mejor = b;
            for (uint32_t i : de[q]) {
                h0[i] = h1[i] = (uint8_t)mejor;
                w0[i] = 1.0f;
            }
            ++piezasRigidas;
        }
    }

    // ⭐ LOS MIEMBROS, SEPARADOS.
    //
    // PEDIDO: "haz que las manos, brazos, piernas y pies etc esten mas
    // separados [...], estan muy juntos". Las posturas del juego estan hechas
    // para un cuerpo de piel: en reposo el brazo cuelga a 6 grados del tronco
    // y las piernas a 2, casi rozandose, porque la carne cede. Un modelo de
    // BLOQUES no cede: sus brazos son prismas de 14 cm de ancho y a esa
    // distancia se meten en la cadera, y los muslos uno en otro.
    //
    // Si el modelo es de bloques (se ato por piezas rigidas) cada postura se
    // abre un poco: los brazos 9 grados mas y las piernas 4. El brazo solo se
    // abre cuando cuelga: al levantarlo para golpear o al juntar las manos en
    // el hacha la apertura se va, o las dos manos dejarian de encontrarse.
    //
    // ⭐ CORRIENDO, AGACHADO Y TUMBADO, MUCHO MAS.
    //
    // PEDIDO: "que la animacion de correr, agachar y tirarse esten las manos y
    // brazos muy separados". En esas tres posturas los brazos se abren ademas
    // 23-26 grados, aunque no cuelguen (corriendo van doblados y tumbado van
    // por delante de la cabeza: `cuelga` vale 0 y antes no se abrian nada).
    // `corre` es 0..1 (andando -> corriendo). El brazo derecho solo se abre si
    // tiene la mano libre: mientras golpea o coloca, su gesto manda.
    static constexpr float ABRIR_BRAZOS = 0.16f, ABRIR_PIERNAS = 0.07f;
    static constexpr float ABRIR_CORRIENDO = 0.40f, ABRIR_AGACHADO = 0.40f, ABRIR_TUMBADO = 0.45f;
    bool esDeBloques() const { return piezasRigidas >= 20; }
    void separarMiembros(Pose& p, float corre = 0.0f, bool derechaLibre = true) const {
        if (!esDeBloques()) return;
        const float tumb = sujetar(p.tumbado, 0.0f, 1.0f);
        const float agach = sujetar(p.agachado, 0.0f, 1.0f) * (1.0f - tumb);
        const float mas = (std::max)(ABRIR_CORRIENDO * sujetar(corre, 0.0f, 1.0f) * (1.0f - tumb) * (1.0f - agach),
                                     ABRIR_AGACHADO * agach) + ABRIR_TUMBADO * tumb;
        for (int l = 0; l < 2; ++l) {
            const float cuelga = sujetar(1.0f - std::fabs(p.brazo[l].flex) / 0.9f, 0.0f, 1.0f);
            p.brazo[l].abd += ABRIR_BRAZOS * cuelga;
            if (l == 0 || derechaLibre) p.brazo[l].abd += mas;
            p.pierna[l].abd += ABRIR_PIERNAS;
        }
    }

    // ⭐ RECTO Y ROBOTICO.
    //
    // PEDIDO: "haz mas rectos y roboticos las animaciones del personaje". Las
    // posturas del juego imitan a una persona: la cadera cae y gira con cada
    // paso, los hombros giran al reves, el tronco se ladea, el pecho respira,
    // las piernas rotan un poco y las munecas van detras del brazo. En un
    // modelo de bloques todo eso sobra: aqui se quita y cada miembro se mueve
    // en su plano, recto, de delante atras. Queda lo que hace falta: el paso,
    // el braceo, agacharse, tumbarse y los golpes.
    //
    // Los hombros solo acompanan a la cabeza (`giroCuello`, radianes): un
    // tercio de su giro, como siempre. Con una accion en marcha se respeta el
    // giro que ella pida. (Los muelles sin rebote: SuavizadoPose::rigido.)
    void hacerRobotico(Pose& p, float giroCuello, bool enAccion) const {
        if (!esDeBloques()) return;
        p.pelvisLado = 0.0f;
        p.giroPelvis = 0.0f;
        p.torsoLado = 0.0f;
        p.pelvisInclina = 0.0f;
        p.cuelloLado = 0.0f;
        p.respira = 0.0f;
        if (!enAccion) p.giroTorso = giroCuello * 0.35f;
        for (int l = 0; l < 2; ++l) {
            p.pierna[l].rot = 0.0f;
            p.pierna[l].tobilloLado = 0.0f;
            if (!enAccion || l == 0) {
                p.brazo[l].rot = 0.0f;
                p.brazo[l].munFlex = 0.0f;
                p.brazo[l].munDesv = 0.0f;
                p.brazo[l].encoger = 0.0f;
            }
        }
    }

    // ⭐ LA PUNTA DE LA MANO VA CON LA MANO.
    //
    // BUG (lo cazo el test "con los brazos abajo las manos van con el
    // esqueleto" al cambiar al modelo de bloques): la pinza del modelo es mas
    // larga que la mano del esqueleto y se curva hacia la palma, asi que sus
    // puntas quedaban lejos de todos los huesos de la mano y se ataban al mas
    // "cercano" que hubiera -- uno de la pierna o de la cadera --. Al mover el
    // brazo se quedaban donde estaban, a medio metro del cuerpo, estirando la
    // malla.
    //
    // Lo que esta pasada la muneca y mas cerca del eje de la mano que de su
    // propio hueso es mano, aunque ningun hueso de los dedos le llegue: se ata
    // rigido a la palma. Solo se toca lo que NO tenia ya un hueso de ese brazo.
    void atarLoQueCuelgaDeLaMano(const Esqueleto& s) {
        std::vector<int> pieza;
        const int numPiezas = piezasDeLaMalla(pieza);
        for (int l = 0; l < 2; ++l) {
            const Junta& j = s.mano[l].muneca;
            const V eje = resta(s.mano[l].dedo[1][3].p, j.p);
            const float L = largo(eje);
            if (L < 0.05f) continue;
            const V d = escala(eje, 1.0f / L);
            auto delBrazo = [&](int b) {
                if (b < H_POR_LADO_INICIO) return false;
                const int lado = (b - H_POR_LADO_INICIO) / HL_NUM, k = (b - H_POR_LADO_INICIO) % HL_NUM;
                return lado == l && k >= HL_CLAVICULA && k < HL_COXAL;
            };
            const uint8_t palma = (uint8_t)huesoDe(l, HL_METACARPO0 + 1);
            // El eje de la mano, alargado: la pinza llega mas lejos que los dedos.
            const V fin = suma(j.p, escala(d, 1.6f * L));
            auto pasadaLaMuneca = [&](const V& p) {
                const V q = resta(p, j.p);
                return (p.x > 0.0f) == (j.p.x > 0.0f) && q.x * d.x + q.y * d.y + q.z * d.z > 0.02f;
            };
            // ⭐ POR PIEZAS ENTERAS. El modelo esta hecho de bloques sueltos:
            // si de un bloque de la pinza se ata media docena de vertices a la
            // mano y el resto se queda en la pierna, al mover el brazo el
            // bloque se estira en abanico. Una pieza (los vertices unidos por
            // triangulos) cuya mayoria ya es de este brazo se lleva ENTERA.
            std::vector<int> deBrazo(numPiezas, 0), total(numPiezas, 0);
            for (size_t i = 0; i < modelo.size(); ++i) {
                ++total[pieza[i]];
                if (delBrazo(h0[i])) ++deBrazo[pieza[i]];
            }
            for (size_t i = 0; i < modelo.size(); ++i) {
                if (delBrazo(h0[i])) continue;
                const V p = modelo[i].p;
                if (!pasadaLaMuneca(p)) continue;
                const bool piezaDelBrazo = 2 * deBrazo[pieza[i]] >= total[pieza[i]];
                if (!piezaDelBrazo) {
                    // Suelta de la mano: solo si le queda MAS CERCA la mano que
                    // el hueso al que se ato (en metros, no en radios: el femur
                    // es gordo y por eso "ganaba"). La cadera sigue en la cadera.
                    const float aLaMano = detalle::distSegmento(p, j.p, fin);
                    if (aLaMano > 0.30f) continue;
                    if (aLaMano >= detalle::distSegmento(p, ref[h0[i]].a, ref[h0[i]].b)) continue;
                }
                h0[i] = h1[i] = palma;
                w0[i] = 1.0f;
            }
        }
    }

    // ⭐ DONDE SE LLEVA EL OBJETO: EN LA MANO DE LA MALLA, NO EN LA DEL ESQUELETO.
    //
    // BUG: "quiero que los items, bloques y herramientas y animaciones de la
    // mano esten en la mano del modelo, no en una parte anterior". El agarre
    // de ModeloJugador::agarre va a 6,8 cm de la muneca, que es el centro de
    // la palma de una mano humana de 19 cm. El modelo de bloques no tiene esa
    // mano: su antebrazo es un bloque largo y la pinza empieza a 10 cm de la
    // muneca del esqueleto y se curva hacia la palma. El objeto salia en el
    // antebrazo.
    //
    // Ahora el punto se MIDE en la malla al cargarla: el centro de lo que hay
    // en el tramo medio de la mano (del 30% al 70% de la muneca a la punta),
    // en el marco de la muneca -- asi sigue a la mano en cualquier postura --,
    // y un poco hacia la palma, que es donde cierra el puno. Vale para
    // cualquier modelo, sin numeros a mano.
    V agarreLocal = { 0.0f, -0.068f, 0.020f };       // en el marco de la muneca derecha
    Junta agarreRef;                                 // ese mismo marco, en la pose de referencia
    void medirAgarre(const Esqueleto& s) {
        agarreLocal = { 0.0f, -0.068f, 0.020f };
        agarreRef = marcoDeAgarre(s);
        medirAgarreEnLaMalla(s);
        agarreRef = marcoDeAgarre(s);
    }
    Junta marcoDeAgarre(const Esqueleto& s) const {
        const Mano& m = s.mano[1];
        const Junta& j = m.muneca;
        const Rot r = m.lado > 0 ? j.r : j.r.por(Rot::enY(PI));
        return { suma(j.p, j.r.aplicar(agarreLocal)), r };
    }
    void medirAgarreEnLaMalla(const Esqueleto& s) {
        const Junta& j = s.mano[1].muneca;
        const float L = largo(resta(s.mano[1].dedo[1][3].p, j.p));
        if (L < 0.05f) return;
        const V ex = j.r.aplicar({ 1, 0, 0 }), ey = j.r.aplicar({ 0, 1, 0 }), ez = j.r.aplicar({ 0, 0, 1 });
        double sx = 0.0, sy = 0.0, sz = 0.0;
        int n = 0;
        for (const VerticeJ& v : modelo) {
            const V q = resta(v.p, j.p);
            const float lx = q.x * ex.x + q.y * ex.y + q.z * ex.z;
            const float ly = q.x * ey.x + q.y * ey.y + q.z * ey.z;      // los dedos van hacia -Y
            const float lz = q.x * ez.x + q.y * ez.y + q.z * ez.z;
            if (-ly < 0.30f * L || -ly > 0.70f * L) continue;
            if (lx * lx + lz * lz > 0.10f * 0.10f) continue;            // lo que no es brazo
            sx += lx; sy += ly; sz += lz;
            ++n;
        }
        if (n < 8) return;
        agarreLocal = { (float)(sx / n), (float)(sy / n), (float)(sz / n) + 0.015f };
    }
    // El marco donde se dibuja lo que lleva en la mano derecha: como
    // ModeloJugador::agarre (+X pulgar, +Y codo, +Z palma), en la mano de ESTA malla.
    //
    // ⭐ EN UN MODELO DE BLOQUES, PEGADO A LA PINZA. PEDIDO: "que todos los
    // items y herramientas y bloques esten entre los dedos del modelo y en su
    // palma de mano". La mano del modelo es una pieza con el antebrazo (ver
    // huesoRigido), asi que el objeto va con ESE hueso, no con la muneca del
    // esqueleto: se lleva el punto medido en la palma -- entre los dedos de la
    // pinza -- con la misma cuenta que mueve a los vertices de la mano. Haga lo
    // que haga el brazo, el objeto esta siempre en el mismo sitio de la mano.
    Junta agarre(const Esqueleto& s) const {
        if (!esDeBloques()) return marcoDeAgarre(s);
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        const int b = huesoDe(1, HL_RADIO);
        const Rot giro = h[b].marco.r.por(detalle::traspuesta(ref[b].marco.r));     // de la referencia a ahora
        return { suma(h[b].marco.p, giro.aplicar(resta(agarreRef.p, ref[b].marco.p))), giro.por(agarreRef.r) };
    }

    // ⭐ EL ESQUELETO DE REFERENCIA, SOBRE LOS BRAZOS DE LA MALLA.
    //
    // BUG: "arregla las manos [...] para que esten perfectas" (las manos
    // salian rotas en tiras aunque los dedos no se doblaran). Los brazos del
    // modelo no estan donde los ponia el esqueleto en cruz: van unos 6 cm mas
    // adelante y la mano cae y se tuerce hacia el pulgar. Los dedos de la
    // malla quedaban a 3 cm de los del esqueleto y se ataban al antebrazo;
    // al girar la muneca la mano se rompia.
    //
    // Ahora, antes de atar, el esqueleto de referencia se ajusta a la malla:
    // el hombro y el codo para que el codo y la muneca caigan en el centro
    // del brazo de la malla, y la muneca para que la mano siga a la de la
    // malla. El centro se mide en un corte de la malla a la altura de cada
    // articulacion (los brazos van en cruz, a lo largo de X).
    float refBrazo[2][4] = {};           // flex, codo, munFlex, munDesv
    // Hasta donde baja un brazo en la pose de referencia. Era 0.9 (brazos en
    // cruz); con los brazos en A la mano cuelga hasta la cadera, y el ajuste
    // de la muneca no veia la mano: se quedaba apuntando hacia fuera mientras
    // la de la malla cuelga recta. Por fuera de x = 0.30 no hay piernas, asi
    // que bajar el limite no mete nada que no sea brazo.
    static constexpr float Y_BRAZO_MIN = 0.55f;

    // ⭐ EL LARGO DEL ANTEBRAZO, MEDIDO EN LA MALLA.
    //
    // BUG: "quiero que los items, bloques y herramientas y animaciones de la
    // mano esten en la mano del modelo, no en una parte anterior" y "haz que
    // las manos, brazos [...] esten mejor animados". El largo del brazo lo
    // calcula el conversor con lo que ALCANZA la malla en X, que solo vale
    // con los brazos en cruz. El modelo de bloques los trae en A: el codo le
    // salia en su sitio, pero al antebrazo le daba 19 cm cuando el de la malla
    // mide 34. La muneca del esqueleto caia en mitad del antebrazo: ese bloque
    // se ataba a los dedos y se deshacia al cerrar el puno, y las pinzas --
    // fuera del alcance de la mano -- se ataban a las piernas.
    //
    // Se mide desde el CODO, que ya esta ajustado: de el a lo mas lejano del
    // brazo hay antebrazo y mano, y el antebrazo es el 58% (0.26 de 0.45, las
    // medidas del esqueleto). El brazo de arriba no se toca: tocarlo mueve el
    // codo, que estaba bien (asi se rompio el primer intento de este arreglo).
    // Devuelve si cambio algo.
    bool ajustarLargoAntebrazo() {
        const Esqueleto s = resolver(poseReferencia(), true);
        float k = 0.0f;
        int lados = 0;
        for (int l = 0; l < 2; ++l) {
            const V codo = s.codo[l].p;
            const float antebrazo = largo(resta(s.mano[l].muneca.p, codo));
            if (antebrazo < 0.05f) continue;
            float alcance = 0.0f;
            for (const VerticeJ& v : modelo) {
                if ((v.p.x > 0.0f) != (codo.x > 0.0f)) continue;        // el otro lado
                if (std::fabs(v.p.x) < 0.30f) continue;                 // cuerpo y piernas
                if (v.p.y > codo.y || v.p.y < Y_BRAZO_MIN) continue;    // del codo para abajo
                alcance = std::fmax(alcance, largo(resta(v.p, codo)));
            }
            if (alcance < 0.20f) continue;
            k += 0.52f * alcance / antebrazo;
            ++lados;
        }
        if (lados == 0) return false;
        k = sujetar(k / lados, 0.70f, 2.00f);
        if (std::fabs(k - 1.0f) < 0.08f) return false;      // ya cuadra (brazos en cruz)
        params[4] *= k;
        return true;
    }
    bool centroCorte(V p, const std::vector<V>& brazo, V& c) const {
        double x = 0, y = 0, z = 0;
        int n = 0;
        for (const V& v : brazo) {
            if (std::fabs(v.x - p.x) > 0.012f || std::fabs(v.y - p.y) > 0.15f || std::fabs(v.z - p.z) > 0.15f) continue;
            x += v.x; y += v.y; z += v.z; ++n;
        }
        if (n < 8) return false;
        c = { (float)(x / n), (float)(y / n), (float)(z / n) };
        return true;
    }
    // ⭐ EL BRAZO SE CORTA DE TRAVES, VENGA COMO VENGA.
    //
    // El centro del brazo se media siempre en una rebanada VERTICAL (a X
    // constante), que es cortar de traves un brazo en cruz. A un brazo que
    // cuelga esa rebanada lo corta casi a lo largo: su "centro" no dice nada,
    // y el ajuste mandaba la muneca 26 cm hacia atras persiguiendolo. Con los
    // brazos colgando (mas cerca de la vertical que de la horizontal) se
    // corta en HORIZONTAL y se compara en X y Z.
    bool brazosColgando() const { return params[0] < 0.9f; }
    bool esDelBrazo(const V& p, float signo) const {
        if (p.y <= Y_BRAZO_MIN) return false;
        if (p.x * signo > 0.30f) return true;
        // Colgando, el brazo de arriba queda pegado al torso, por dentro de 0.30.
        return brazosColgando() && p.x * signo > 0.22f && p.y > 0.95f;
    }
    // Cuanto se aparta `p` del centro del brazo de la malla (al cuadrado).
    float errorCentro(V p, const std::vector<V>& brazo) const {
        if (!brazosColgando()) {
            V c;
            if (!centroCorte(p, brazo, c)) return 0.01f;
            const V d = resta(p, c);
            return d.y * d.y + d.z * d.z;
        }
        double x = 0, z = 0;
        int n = 0;
        for (const V& v : brazo) {
            if (std::fabs(v.y - p.y) > 0.012f || std::fabs(v.x - p.x) > 0.20f || std::fabs(v.z - p.z) > 0.20f) continue;
            x += v.x; z += v.z; ++n;
        }
        // Sin malla a esa altura el punto esta FUERA del brazo: tiene que
        // costar mas que cualquier desajuste, o el ajuste se va a buscarlo.
        if (n < 8) return 0.04f;
        const float dx = p.x - (float)(x / n), dz = p.z - (float)(z / n);
        return dx * dx + dz * dz;
    }
    template <class F> void buscar(float& x, float lo, float hi, F error) {
        float mejor = x, e = error();
        for (float paso : { 0.04f, 0.01f }) {
            const float c = mejor;
            for (float v = std::fmax(lo, c - 10 * paso); v <= std::fmin(hi, c + 10 * paso) + 1e-4f; v += paso) {
                x = v;
                const float ev = error();
                if (ev < e) { e = ev; mejor = v; }
            }
        }
        x = mejor;
    }
    void ajustarBrazos() {
        for (int l = 0; l < 2; ++l) {
            const float signo = resolver(poseReferencia(), true).mano[l].muneca.p.x < 0.0f ? -1.0f : 1.0f;
            std::vector<V> brazo;
            for (const VerticeJ& v : modelo) if (esDelBrazo(v.p, signo)) brazo.push_back(v.p);
            auto error = [&]() {
                const Esqueleto s = resolver(poseReferencia(), true);
                float e = 0.0f;
                for (V p : { s.codo[l].p, s.mano[l].muneca.p }) e += errorCentro(p, brazo);
                return e;
            };
            for (int vuelta = 0; vuelta < 3; ++vuelta) {
                buscar(refBrazo[l][0], -0.8f, 0.8f, error);
                buscar(refBrazo[l][1], 0.0f, 0.8f, error);
            }
        }
    }
    // Lo mismo con las piernas. BUG: "las rodillas se estiran y rompen
    // deformadas al agacharse y tirarse": la rodilla del esqueleto caia en el
    // borde de delante de la rodilla de la malla (5 cm fuera de su centro) y
    // al doblarla la piel giraba alrededor de un punto que no era el suyo.
    // Las piernas van de pie, a lo largo de Y: los cortes son horizontales.
    float refPierna[2][4] = {};          // flex, rodilla, abd (sobre params[1]), tobillo
    bool centroCorteY(V p, const std::vector<V>& pierna, V& c) const {
        double x = 0, z = 0;
        int n = 0;
        for (const V& v : pierna) {
            if (std::fabs(v.y - p.y) > 0.01f || std::fabs(v.x - p.x) > 0.12f || std::fabs(v.z - p.z) > 0.15f) continue;
            x += v.x; z += v.z; ++n;
        }
        if (n < 8) return false;
        c = { (float)(x / n), p.y, (float)(z / n) };
        return true;
    }
    void ajustarPiernas() {
        for (int l = 0; l < 2; ++l) {
            const float signo = resolver(poseReferencia(), true).cadera[l].p.x < 0.0f ? -1.0f : 1.0f;
            std::vector<V> pierna;
            for (const VerticeJ& v : modelo) if (v.p.x * signo > 0.015f && std::fabs(v.p.x) < 0.30f && v.p.y < 0.85f) pierna.push_back(v.p);
            auto error = [&]() {
                const Esqueleto s = resolver(poseReferencia(), true);
                const V r = s.rodilla[l].p, t = s.pie[l].tobillo.p;
                float e = 0.0f;
                // La rodilla, el muslo por encima y el tobillo (6 cm por encima:
                // mas abajo el corte se lleva el pie; la espinilla no, la tuerce
                // el gemelo).
                for (V p : { r, mezclar(r, s.cadera[l].p, 0.35f), suma(t, V{ 0.0f, 0.06f, 0.0f }) }) {
                    V c;
                    if (centroCorteY(p, pierna, c)) { const V d = resta(p, c); e += d.x * d.x + d.z * d.z; }
                    else e += 0.01f;
                }
                return e;
            };
            for (int vuelta = 0; vuelta < 3; ++vuelta) {
                buscar(refPierna[l][0], -0.5f, 0.5f, error);
                buscar(refPierna[l][1], -0.2f, 0.6f, error);
                buscar(refPierna[l][2], -0.15f, 0.15f, error);
            }
            // El pie, plano en el suelo como en la malla.
            buscar(refPierna[l][3], -0.6f, 0.6f, [&]() {
                const Esqueleto s = resolver(poseReferencia(), true);
                const float d = s.pie[l].bola.y - s.pie[l].talon.y;
                return d * d;
            });
        }
    }
    void ajustarManos() {
        for (int l = 0; l < 2; ++l) {
            const float signo = resolver(poseReferencia(), true).mano[l].muneca.p.x < 0.0f ? -1.0f : 1.0f;
            std::vector<V> brazo;
            for (const VerticeJ& v : modelo) if (esDelBrazo(v.p, signo)) brazo.push_back(v.p);
            auto error = [&]() {
                const Esqueleto s = resolver(poseReferencia(), true);
                float e = 0.0f;
                // Los nudillos y la mitad de los dedos: la palma y hacia donde van.
                for (V p : { mezclar(s.mano[l].dedo[1][0].p, s.mano[l].dedo[2][0].p, 0.5f),
                             mezclar(s.mano[l].dedo[1][2].p, s.mano[l].dedo[2][2].p, 0.5f) })
                    e += errorCentro(p, brazo);
                return e;
            };
            for (int vuelta = 0; vuelta < 3; ++vuelta) {
                buscar(refBrazo[l][2], -0.9f, 0.9f, error);
                buscar(refBrazo[l][3], -0.6f, 0.6f, error);
            }
        }
    }

    // ⭐ LOS PIES, CON LOS HUESOS DEL PIE.
    //
    // BUG: tirado, de los pies salia un pico. La distancia a cada hueso se
    // mide en radios del hueso, y la tibia es gruesa: la planta y los dedos
    // del pie le quedaban "mas cerca" que los huesos del pie, finos, y al
    // estirar el tobillo el pie se quedaba con la pierna. Lo que esta por
    // debajo del tobillo se ata solo a los huesos del pie (con la distancia
    // tal cual); el suavizado lo une luego con la pierna.
    void atarPies(const Esqueleto& s) {
        for (int l = 0; l < 2; ++l) {
            const V tob = s.pie[l].tobillo.p;
            const int b0 = huesoDe(l, HL_TARSO0), b1 = huesoDe(l, HL_FALANGE_PIE0 + 13);
            for (size_t i = 0; i < modelo.size(); ++i) {
                const V p = modelo[i].p;
                if (p.x * tob.x <= 0.0f || p.y > tob.y - 0.015f || largo(resta(p, tob)) > 0.35f) continue;
                float d[NUM_HUESOS];
                int mejor = b0;
                for (int b = b0; b <= b1; ++b) {
                    d[b] = detalle::distSegmento(p, ref[b].a, ref[b].b);
                    if (d[b] < d[mejor]) mejor = b;
                }
                int segundo = -1;
                for (int b = b0; b <= b1; ++b) {
                    if (b == mejor || (ref[b].padre != mejor && ref[mejor].padre != b)) continue;
                    if (segundo < 0 || d[b] < d[segundo]) segundo = b;
                }
                h0[i] = (uint8_t)mejor;
                h1[i] = (uint8_t)(segundo < 0 ? mejor : segundo);
                w0[i] = 1.0f;
                if (segundo >= 0) {
                    const float a = std::fmax(d[mejor], 0.002f), b = std::fmax(d[segundo], 0.002f);
                    const float fa = 1.0f / (a * a * a * a), fb = 1.0f / (b * b * b * b);
                    w0[i] = fa / (fa + fb);
                    if (w0[i] > 0.97f) { w0[i] = 1.0f; h1[i] = h0[i]; }
                }
            }
        }
    }

    // ⭐ LOS DEDOS, MEDIDOS EN LA MALLA.
    //
    // BUG: "arregla las manos [...] para que esten perfectas y no asi" (los
    // dedos salian como garras rotas al cerrar la mano o al apoyarla). Cada
    // vertice del dedo seguia a la falange del esqueleto MAS CERCANA, pero los
    // dedos del modelo no caen justo donde los del esqueleto: un vertice del
    // indice podia seguir al medio, o a la falange equivocada, y al cerrar la
    // mano el dedo se rompia en tiras.
    //
    // Ahora los dedos se reconocen en la propia malla: en la pose de
    // referencia (en cruz, palmas abajo) lo que esta pasados los nudillos se
    // reparte en los 4 dedos por su posicion a lo ancho de la palma (k-medias
    // empezando por donde estan los del esqueleto), y cada dedo, del nudillo a
    // su punta de verdad, en sus 3 falanges con las proporciones de un dedo
    // real (cerca de cada articulacion el vertice se reparte entre las dos).
    // El pulgar, que sale del lado, sigue con lo de antes.
    void atarDedos(const Esqueleto& s) {
        for (int l = 0; l < 2; ++l) {
            const Mano& m = s.mano[l];
            const Rot Rt = detalle::traspuesta(m.muneca.r);
            auto local = [&](V p) { return Rt.aplicar(resta(p, m.muneca.p)); };
            // Lo de cada dedo del esqueleto, en el marco de la mano: a lo
            // largo (hacia la punta) y a lo ancho (hacia el pulgar).
            float largoNudillo[4], anchoNudillo[4], nudilloMedio = 0.0f;
            for (int d = 0; d < 4; ++d) {
                const V k = local(m.dedo[d][0].p);
                largoNudillo[d] = -k.y;
                anchoNudillo[d] = k.x * m.lado;
                nudilloMedio += 0.25f * largoNudillo[d];
            }
            // Del antebrazo tambien: con el radio grueso, la palma y hasta
            // los dedos le quedaban "mas cerca" que los huesos finos de la mano.
            auto esDeLaMano = [&](int b) {
                if (b < H_POR_LADO_INICIO) return false;
                const int lado = (b - H_POR_LADO_INICIO) / HL_NUM, k = (b - H_POR_LADO_INICIO) % HL_NUM;
                return lado == l && (k == HL_RADIO || k == HL_CUBITO ||
                                     (k >= HL_CARPO0 && k < HL_FALANGE_MANO0 + 12 && !(k == HL_METACARPO0 + 4)));
            };
            auto delAntebrazo = [&](int b) {
                const int k = (b - H_POR_LADO_INICIO) % HL_NUM;
                return k == HL_RADIO || k == HL_CUBITO;
            };
            std::vector<uint32_t> palma;
            std::vector<float> largoP, anchoP;
            // Los vertices de los 4 dedos: de la mano, pasados los nudillos.
            std::vector<uint32_t> vs;
            std::vector<float> largoV, anchoV;
            for (size_t i = 0; i < modelo.size(); ++i) {
                if (!esDeLaMano(h0[i])) continue;
                const V q = local(modelo[i].p);
                if (-q.y < 0.0f || std::fabs(q.z) > 0.05f || std::fabs(q.x) > 0.09f) continue;
                if (-q.y < nudilloMedio - 0.004f) {
                    palma.push_back((uint32_t)i); largoP.push_back(-q.y); anchoP.push_back(q.x * m.lado);
                    continue;
                }
                vs.push_back((uint32_t)i);
                largoV.push_back(-q.y);
                anchoV.push_back(q.x * m.lado);
            }
            if (vs.size() < 40) continue;
            // k-medias a lo ancho, empezando por los dedos del esqueleto; el
            // quinto grupo es el pulgar (lo que cae de su lado no es del indice).
            float c[5];
            for (int d = 0; d < 4; ++d) c[d] = anchoNudillo[d];
            c[4] = local(m.dedo[4][2].p).x * m.lado;
            std::vector<int> grupo(vs.size(), 0);
            for (int it = 0; it < 12; ++it) {
                float suma[5] = {}, n[5] = {};
                for (size_t k = 0; k < vs.size(); ++k) {
                    int mejor = 0;
                    for (int d = 1; d < 5; ++d) if (std::fabs(anchoV[k] - c[d]) < std::fabs(anchoV[k] - c[mejor])) mejor = d;
                    grupo[k] = mejor;
                    suma[mejor] += anchoV[k];
                    n[mejor] += 1.0f;
                }
                for (int d = 0; d < 5; ++d) if (n[d] > 0) c[d] = suma[d] / n[d];
            }
            // La palma, de la muneca a los nudillos: con el metacarpiano de su
            // dedo (el del pulgar si cae de su lado), y junto a la muneca
            // mezclada con el antebrazo (2.5 cm).
            for (size_t k = 0; k < palma.size(); ++k) {
                int d = 0;
                for (int e = 1; e < 5; ++e) if (std::fabs(anchoP[k] - c[e]) < std::fabs(anchoP[k] - c[d])) d = e;
                const uint32_t i = palma[k];
                if (d == 4 && !delAntebrazo(h0[i])) continue;     // el pulgar, como estaba
                const int meta = huesoDe(l, HL_METACARPO0 + d), radio = huesoDe(l, HL_RADIO);
                h0[i] = (uint8_t)meta; h1[i] = (uint8_t)meta; w0[i] = 1.0f;
                if (largoP[k] < 0.025f) { h1[i] = (uint8_t)radio; w0[i] = largoP[k] / 0.025f; }
            }
            // Donde acaba cada dedo de verdad.
            float punta[5] = { 0, 0, 0, 0, 0 };
            for (size_t k = 0; k < vs.size(); ++k) punta[grupo[k]] = std::fmax(punta[grupo[k]], largoV[k]);
            for (size_t k = 0; k < vs.size(); ++k) {
                const int d = grupo[k];
                if (d == 4) continue;        // el pulgar: con su atado de siempre
                const float a0 = largoNudillo[d], a1 = punta[d];
                if (a1 <= a0 + 0.02f) continue;
                const float L0 = M::DEDO[d][0], L1 = M::DEDO[d][1], L2 = M::DEDO[d][2], Lt = L0 + L1 + L2;
                // Los bordes: el nudillo, las dos articulaciones de los dedos.
                const float borde[3] = { a0, a0 + (a1 - a0) * L0 / Lt, a0 + (a1 - a0) * (L0 + L1) / Lt };
                const int hueso[4] = { huesoDe(l, HL_METACARPO0 + d), huesoDe(l, HL_FALANGE_MANO0 + d * 3),
                                       huesoDe(l, HL_FALANGE_MANO0 + d * 3 + 1), huesoDe(l, HL_FALANGE_MANO0 + d * 3 + 2) };
                const float x = largoV[k];
                int tramo = 0;
                while (tramo < 3 && x >= borde[tramo]) ++tramo;     // 0 palma .. 3 la ultima falange
                const uint32_t i = vs[k];
                h0[i] = (uint8_t)hueso[tramo];
                h1[i] = (uint8_t)hueso[tramo];
                w0[i] = 1.0f;
                // Cerca de una articulacion, entre las dos falanges (6 mm).
                const float ancho = 0.006f;
                for (int b = 0; b < 3; ++b) {
                    const float dd = x - borde[b];
                    if (std::fabs(dd) < ancho) {
                        const float t = 0.5f + 0.5f * dd / ancho;     // 0 lado de antes, 1 lado de despues
                        h0[i] = (uint8_t)hueso[b + 1];
                        h1[i] = (uint8_t)hueso[b];
                        w0[i] = t;
                    }
                }
            }
        }
    }

    // ⭐ LOS PESOS, SUAVIZADOS SOBRE LA MALLA.
    //
    // BUG: agachado (rodilla a 130 grados) la rodilla salia rota: un vertice
    // seguia al femur y su vecino a la tibia, y el triangulo entre los dos se
    // estiraba como una tira. Ahora cada vertice mezcla sus pesos con los de
    // sus vecinos unas cuantas veces (y los que comparten posicion en una
    // costura de la textura, igual): no hay saltos de un vertice al de al
    // lado. Al final cada uno se queda con sus dos huesos mas fuertes.
    void suavizarPesos() {
        const size_t n = modelo.size();
        constexpr int K = 4;
        struct Pesos { uint8_t b[K]; float w[K]; int c = 0; };
        std::vector<Pesos> P(n), Q(n);
        auto meter = [](Pesos& p, uint8_t b, float w) {
            for (int k = 0; k < p.c; ++k) if (p.b[k] == b) { p.w[k] += w; return; }
            if (p.c < K) { p.b[p.c] = b; p.w[p.c] = w; ++p.c; return; }
            int menor = 0;
            for (int k = 1; k < K; ++k) if (p.w[k] < p.w[menor]) menor = k;
            if (w > p.w[menor]) { p.b[menor] = b; p.w[menor] = w; }
        };
        for (size_t i = 0; i < n; ++i) {
            meter(P[i], h0[i], w0[i]);
            if (h1[i] != h0[i]) meter(P[i], h1[i], 1.0f - w0[i]);
        }
        // Los vecinos (por los triangulos) y los gemelos de costura (misma posicion).
        std::vector<std::vector<uint32_t>> vec(n);
        for (size_t t = 0; t + 2 < indices.size(); t += 3)
            for (int a = 0; a < 3; ++a)
                for (int b = 0; b < 3; ++b)
                    if (a != b) vec[indices[t + a]].push_back(indices[t + b]);
        {
            std::vector<uint32_t> orden(n);
            for (size_t i = 0; i < n; ++i) orden[i] = (uint32_t)i;
            auto clave = [&](uint32_t i) { const V p = modelo[i].p; return std::make_tuple(p.x, p.y, p.z); };
            std::sort(orden.begin(), orden.end(), [&](uint32_t a, uint32_t b) { return clave(a) < clave(b); });
            for (size_t i = 0; i + 1 < n; ++i)
                if (clave(orden[i]) == clave(orden[i + 1])) {
                    vec[orden[i]].push_back(orden[i + 1]);
                    vec[orden[i + 1]].push_back(orden[i]);
                }
        }
        for (auto& v : vec) { std::sort(v.begin(), v.end()); v.erase(std::unique(v.begin(), v.end()), v.end()); }
        for (int it = 0; it < 4; ++it) {
            for (size_t i = 0; i < n; ++i) {
                Pesos q;
                for (int k = 0; k < P[i].c; ++k) meter(q, P[i].b[k], 0.5f * P[i].w[k]);
                const float r = vec[i].empty() ? 0.0f : 0.5f / (float)vec[i].size();
                for (uint32_t j : vec[i])
                    for (int k = 0; k < P[j].c; ++k) meter(q, P[j].b[k], r * P[j].w[k]);
                if (vec[i].empty()) for (int k = 0; k < q.c; ++k) q.w[k] *= 2.0f;
                Q[i] = q;
            }
            std::swap(P, Q);
        }
        for (size_t i = 0; i < n; ++i) {
            const Pesos& p = P[i];
            int a = 0, b = -1;
            for (int k = 1; k < p.c; ++k) if (p.w[k] > p.w[a]) a = k;
            for (int k = 0; k < p.c; ++k) if (k != a && (b < 0 || p.w[k] > p.w[b])) b = k;
            const float wa = p.w[a], wb = b >= 0 ? p.w[b] : 0.0f;
            h0[i] = p.b[a];
            h1[i] = b >= 0 ? p.b[b] : p.b[a];
            w0[i] = wa + wb > 1e-6f ? wa / (wa + wb) : 1.0f;
            if (w0[i] > 0.985f) { w0[i] = 1.0f; h1[i] = h0[i]; }
        }
    }

    // ⭐ LOS PUNTOS DE REFERENCIA, buscados en la malla: la punta de la nariz
    // es lo mas adelantado de la cara; el menton, lo mas bajo de delante de
    // la cabeza; las orejas, lo mas ancho de la cabeza a cada lado; los ojos,
    // sobre la nariz y a los lados (con las medidas de una cara media).
    void buscarPuntos(const Esqueleto& s) {
        Puntos p;
        const float yCuello = s.cuello.p.y;
        double cx = 0, cy = 0, cz = 0;
        int n = 0;
        float yMax = 0.0f;
        for (const VerticeJ& v : modelo)
            if (v.p.y > yCuello + 0.06f) { cx += v.p.x; cy += v.p.y; cz += v.p.z; ++n; yMax = std::fmax(yMax, v.p.y); }
        p.cabeza = n ? V{ (float)(cx / n), (float)(cy / n), (float)(cz / n) } : s.cuello.p;
        // El medio de la cabeza de delante atras: entre lo mas adelantado y lo
        // mas atrasado. (El promedio de vertices se va hacia la cara, que tiene
        // mas detalle.)
        float zMin = 1e9f, zMax = -1e9f;
        for (const VerticeJ& v : modelo)
            if (v.p.y > yCuello + 0.06f) { zMin = std::fmin(zMin, v.p.z); zMax = std::fmax(zMax, v.p.z); }
        p.cabezaMedioZ = n ? 0.5f * (zMin + zMax) : p.cabeza.z;
        p.cabezaRadio = std::fmax(0.08f, (yMax - yCuello) * 0.5f);
        p.nariz = p.cabeza;
        for (const VerticeJ& v : modelo)
            if (v.p.y > yCuello + 0.06f && std::fabs(v.p.x - p.cabeza.x) < 0.025f && v.p.y < p.cabeza.y + 0.04f &&
                v.p.z > p.nariz.z) p.nariz = v.p;
        p.menton = p.nariz;
        for (const VerticeJ& v : modelo)
            if (std::fabs(v.p.x - p.cabeza.x) < 0.02f && v.p.z > p.cabeza.z + 0.04f && v.p.y > yCuello &&
                v.p.y < p.menton.y && v.p.y < p.nariz.y) p.menton = v.p;
        p.boca = mezclar(p.nariz, p.menton, 0.42f);
        p.boca.z = p.nariz.z - 0.012f;
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            p.ojo[l] = { p.nariz.x + sx * 0.032f, p.nariz.y + 0.034f, p.nariz.z - 0.028f };
            V oreja = p.cabeza;
            for (const VerticeJ& v : modelo)
                if (std::fabs(v.p.y - p.cabeza.y) < 0.035f && v.p.y > yCuello + 0.06f && (v.p.x - p.cabeza.x) * sx > (oreja.x - p.cabeza.x) * sx)
                    oreja = v.p;
            p.oreja[l] = oreja;
            p.pecho[l] = { s.pelvis.p.x + sx * 0.09f, s.pelvis.p.y + 0.36f, s.pelvis.p.z + 0.11f };
            p.gluteo[l] = { s.pelvis.p.x + sx * 0.08f, s.pelvis.p.y - 0.06f, s.pelvis.p.z - 0.10f };
        }
        p.cuelloBase = s.cuello.p;
        p.vientre = { s.pelvis.p.x, s.pelvis.p.y + 0.12f, s.pelvis.p.z + 0.10f };
        pt = p;
    }

    // ⭐ LA CARA Y EL CUERPO POR ZONAS. Cada ajuste mueve los vertices de
    // alrededor de su punto con un peso que se apaga suave hasta su radio.
    void aplicarAjustes(const AjustesMalla& a) { aplicarAjustes(a, proporciones()); }
    void aplicarAjustes(const AjustesMalla& a, const Proporciones& prop) {
        const bool mismaProp = std::memcmp(&prop, &propReposo, sizeof(prop)) == 0;
        if (a == ajustes && mismaProp && !base.empty() && versionForma > 1) return;
        ajustes = a;
        propReposo = prop;
        base = modelo;
        // Escalar alrededor de `c` en cada eje, dentro del radio `r`.
        auto zona = [&](V c, float r, float kx, float ky, float kz) {
            if (kx == 1.0f && ky == 1.0f && kz == 1.0f) return;
            for (VerticeJ& v : base) {
                const float w = detalle::suaveRadio(largo(resta(v.p, c)), r);
                if (w <= 0.0f) continue;
                v.p.x = c.x + (v.p.x - c.x) * (1.0f + (kx - 1.0f) * w);
                v.p.y = c.y + (v.p.y - c.y) * (1.0f + (ky - 1.0f) * w);
                v.p.z = c.z + (v.p.z - c.z) * (1.0f + (kz - 1.0f) * w);
            }
        };
        auto empujar = [&](V c, float r, V d) {
            if (d.x == 0.0f && d.y == 0.0f && d.z == 0.0f) return;
            for (VerticeJ& v : base) {
                const float w = detalle::suaveRadio(largo(resta(v.p, c)), r);
                if (w > 0.0f) v.p = suma(v.p, escala(d, w));
            }
        };
        const Puntos& p = pt;
        const float R = p.cabezaRadio;
        // La cabeza entera: ancho y largo.
        zona(p.cabeza, R * 1.6f, a.caraAncho, a.caraLargo, 1.0f);
        // La frente, los pomulos, la mandibula y el menton.
        empujar({ p.cabeza.x, p.nariz.y + 0.07f, p.nariz.z - 0.03f }, 0.06f, { 0, 0.012f * (a.frente - 1) * 6, 0.02f * (a.frente - 1) * 6 });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.nariz.x + sx * 0.050f, p.nariz.y, p.nariz.z - 0.040f }, 0.035f, { sx * 0.012f * (a.pomulos - 1) * 5, 0, 0.006f * (a.pomulos - 1) * 5 });
        }
        zona({ p.menton.x, p.menton.y + 0.025f, p.menton.z - 0.05f }, 0.075f, a.mandibula, 1.0f, 1.0f);
        empujar(p.menton, 0.035f, { 0, -0.010f * (a.menton - 1) * 5, 0.018f * (a.menton - 1) * 5 });
        // Los ojos y las cejas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.ojo[l], 0.020f, a.ojos, a.ojos, a.ojos);
            empujar(p.ojo[l], 0.024f, { sx * 0.045f * (a.ojosSeparacion - 1), 0.060f * (a.ojosAltura - 1), 0.012f * (1 - a.ojosProfundidad) });
            // La comisura de fuera sube o baja: los ojos se inclinan.
            empujar({ p.ojo[l].x + sx * 0.016f, p.ojo[l].y, p.ojo[l].z - 0.004f }, 0.014f, { 0, 0.005f * a.ojosInclinacion, 0 });
            const V ceja = { p.ojo[l].x, p.ojo[l].y + 0.018f, p.ojo[l].z + 0.004f };
            empujar(ceja, 0.022f, { 0, 0.08f * (a.cejasAltura - 1), 0.004f * (a.cejasGrosor - 1) });
            empujar({ ceja.x + sx * 0.008f, ceja.y + 0.003f, ceja.z }, 0.012f, { 0, 0.005f * a.cejasArco, 0 });
        }
        // La nariz y la boca.
        // El largo: la punta sale (~2 cm de nariz: un 25 % son 5 mm); el ancho,
        // alrededor de la base.
        empujar(p.nariz, 0.030f, { 0, -0.006f * (a.narizLargo - 1), 0.035f * (a.narizLargo - 1) });
        empujar(p.nariz, 0.016f, { 0, 0.006f * a.narizPunta, 0 });
        zona({ p.nariz.x, p.nariz.y - 0.010f, p.nariz.z - 0.012f }, 0.020f, a.narizAletas, 1.0f, 1.0f);
        empujar({ p.nariz.x, p.nariz.y + 0.01f, p.nariz.z - 0.015f }, 0.035f, { 0, 0.06f * (a.narizAltura - 1), 0 });
        zona({ p.nariz.x, p.nariz.y, p.nariz.z - 0.018f }, 0.035f, a.narizAncho, 1.0f, 1.0f);
        empujar({ p.nariz.x, p.nariz.y + 0.025f, p.nariz.z - 0.012f }, 0.022f, { 0, 0, 0.012f * (a.narizPuente - 1) * 5 });
        zona(p.boca, 0.030f, a.bocaAncho, a.labios, 1.0f);
        empujar(p.boca, 0.032f, { 0, 0.07f * (a.bocaAltura - 1), 0.008f * a.bocaSaliente });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.boca.x + sx * 0.022f, p.boca.y, p.boca.z - 0.006f }, 0.013f, { sx * 0.002f * a.sonrisa, 0.004f * a.sonrisa, -0.001f * a.sonrisa });
        }
        // El craneo, las sienes y la papada.
        zona({ p.cabeza.x, p.cabeza.y, p.cabeza.z - 0.05f }, R * 1.2f, 1.0f, 1.0f, a.craneo);
        zona({ p.cabeza.x, p.ojo[0].y + 0.02f, p.ojo[0].z - 0.05f }, 0.06f, a.sienes, 1.0f, 1.0f);
        empujar({ p.menton.x, p.menton.y - 0.02f, p.menton.z - 0.04f }, 0.045f, { 0, -0.012f * a.papada, 0.010f * a.papada });
        // Las orejas: tamano y lo despegadas.
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            zona(p.oreja[l], 0.035f, a.orejas, a.orejas, a.orejas);
            empujar(p.oreja[l], 0.035f, { sx * 0.025f * a.orejasDespegadas, 0.05f * (a.orejasAltura - 1), 0 });
            empujar({ p.oreja[l].x, p.oreja[l].y + 0.025f, p.oreja[l].z - 0.005f }, 0.018f, { sx * 0.004f * a.orejasPunta, 0.012f * a.orejasPunta, -0.004f * a.orejasPunta });
            zona({ p.oreja[l].x, p.oreja[l].y - 0.025f, p.oreja[l].z }, 0.014f, a.lobulos, a.lobulos, a.lobulos);
        }
        // El cuello: lo de encima sube (la cabeza con el) y la franja se
        // estira; y su grosor.
        if (a.cuelloLargo != 1.0f) {
            const float dy = 0.07f * (a.cuelloLargo - 1.0f), y0 = p.cuelloBase.y - 0.03f;
            for (VerticeJ& v : base) {
                const float u = sujetar((v.p.y - y0) / 0.08f, 0.0f, 1.0f);
                if (std::fabs(v.p.x - p.cuelloBase.x) < 0.35f) v.p.y += dy * u;
            }
        }
        // El pelo: lo de arriba de la cabeza, mas o menos voluminoso.
        if (a.peloVolumen != 1.0f)
            for (VerticeJ& v : base) {
                const float u = sujetar((v.p.y - (p.ojo[0].y + 0.035f)) / 0.05f, 0.0f, 1.0f);
                if (u <= 0.0f || largo(resta(v.p, p.cabeza)) > R * 2.0f) continue;
                const float k = 1.0f + (a.peloVolumen - 1.0f) * u;
                v.p = suma(p.cabeza, escala(resta(v.p, p.cabeza), k));
            }
        // La nuez, los trapecios y los pectorales.
        empujar({ p.cuelloBase.x, p.cuelloBase.y + 0.03f, p.cuelloBase.z + 0.05f }, 0.025f, { 0, 0, 0.014f * (a.nuez - 0.3f) });
        for (int l = 0; l < 2; ++l) {
            const float sx = l == 0 ? -1.0f : 1.0f;
            empujar({ p.cuelloBase.x + sx * 0.09f, p.cuelloBase.y - 0.03f, p.cuelloBase.z - 0.02f }, 0.06f, { 0, 0.03f * (a.trapecio - 1), 0 });
            empujar({ p.pecho[l].x, p.pecho[l].y + 0.04f, p.pecho[l].z }, 0.08f, { 0, 0, 0.025f * (a.pectoral - 0.2f) });
        }
        // El busto, el vientre y los gluteos: abultan hacia fuera.
        for (int l = 0; l < 2; ++l) {
            empujar(p.pecho[l], 0.085f, { 0, -0.010f * a.busto, 0.055f * a.busto });
            zona({ p.gluteo[l].x, p.gluteo[l].y + 0.05f * a.cinturaAltura, p.gluteo[l].z }, 0.10f, 1.0f, 1.0f, a.gluteos);
        }
        empujar(p.vientre, 0.14f, { 0, 0, 0.07f * a.abdomen });

        // ⭐ EL REPOSO CON LAS PROPORCIONES Y GROSORES DEL AVATAR. El esqueleto
        // de referencia con los largos pedidos, y cada vertice llevado alli
        // con la escala de sus huesos (grosor y largo). Se hace una vez por
        // cambio; cada frame solo se GIRA (ver cuaternionesDuales), que es lo
        // que permite el skinning con cuaterniones duales sin estirar nada.
        Pose pr = poseReferencia();
        pr.prop = prop;
        huesosDe(resolver(pr, true), reposo);
        V S[NUM_HUESOS];
        escalas(reposo, S);
        Rot M[NUM_HUESOS];
        V T[NUM_HUESOS];
        for (int b = 0; b < NUM_HUESOS; ++b) {
            const Rot escalada = reposo[b].marco.r.por(Rot{ { S[b].x, 0, 0, 0, S[b].y, 0, 0, 0, S[b].z } });
            M[b] = escalada.por(detalle::traspuesta(ref[b].marco.r));
            T[b] = resta(reposo[b].marco.p, M[b].aplicar(ref[b].marco.p));
        }
        // Las bisagras de los dedos van a donde va su mano.
        for (int e = 0; e < numDedos; ++e) {
            HuesoDedo& d = dedos[e];
            const int r = huesoDe(d.lado, HL_RADIO);
            d.nudilloB = suma(M[r].aplicar(d.nudillo), T[r]);
            d.falangeB = suma(M[r].aplicar(d.falange), T[r]);
            d.ejeB = normalizar(M[r].aplicar(d.eje));
        }
        for (size_t i = 0; i < base.size(); ++i) {
            const int x = h0[i], y = h1[i];
            const V p0 = base[i].p;
            V q = suma(M[x].aplicar(p0), T[x]);
            if (x != y) q = suma(escala(q, w0[i]), escala(suma(M[y].aplicar(p0), T[y]), 1.0f - w0[i]));
            base[i].p = q;
        }
        ++versionForma;
    }

    // La escala de cada hueso en su propio marco: x y z el grosor, y el
    // largo. El largo sale solo: lo que mide el hueso ahora entre lo que media
    // en la pose de referencia (las proporciones del avatar lo cambian).
    void escalas(const Hueso* h, V* S) const {
        const AjustesMalla& a = ajustes;
        for (int b = 0; b < NUM_HUESOS; ++b) S[b] = { 1, 1, 1 };
        auto largoRel = [&](int b) {
            const float l0 = largo(resta(ref[b].b, ref[b].a));
            return l0 > 1e-4f ? largo(resta(h[b].b, h[b].a)) / l0 : 1.0f;
        };
        auto grosor = [&](int b, float x, float z) { S[b] = { x, largoRel(b), z }; };
        auto todo = [&](int b, float k) { S[b] = { k, k, k }; };
        // Los de las articulaciones: el grosor medio de sus vecinos, sin estirar.
        auto media = [&](int b, int x, int y) { S[b] = { 0.5f * (S[x].x + S[y].x), 1.0f, 0.5f * (S[x].z + S[y].z) }; };
        // La columna: de la cadera y la cintura abajo al pecho arriba.
        S[H_SACRO] = { a.cadera, 1.0f, 0.5f * (a.cadera + a.cintura) };
        S[H_COCCIX] = S[H_SACRO];
        for (int i = 0; i < 17; ++i) {
            const float t = (float)i / 16.0f;
            const float x = i < 5 ? a.cintura + (a.cadera - a.cintura) * (1.0f - i / 4.0f) * 0.5f
                                  : a.cintura + (a.pecho * a.espalda - a.cintura) * sujetar((t - 0.3f) / 0.5f, 0.0f, 1.0f);
            const float z = i < 5 ? a.cintura : a.cintura + (a.pecho - a.cintura) * sujetar((t - 0.3f) / 0.5f, 0.0f, 1.0f);
            grosor(H_VERTEBRA0 + i, x, z);
        }
        for (int i = 0; i < 7; ++i) S[H_CERVICAL0 + i] = { a.cuello, 1.0f, a.cuello };
        S[H_HIOIDES] = { a.cuello, 1.0f, a.cuello };
        todo(H_ESTERNON, a.pecho);
        for (int b = 0; b < NUM_HUESOS; ++b) if (esDeLaCabeza(b)) todo(b, a.cabeza);
        for (int l = 0; l < 2; ++l) {
            auto H = [&](int k) { return huesoDe(l, k); };
            for (int k = 0; k < 12; ++k) S[H(HL_COSTILLA0 + k)] = { a.pecho, 1.0f, a.pecho };
            todo(H(HL_CLAVICULA), a.hombros);
            const float brazo = a.brazo * (1.0f + (a.biceps - 1.0f) * 0.6f) * (1.0f + (a.triceps - 1.0f) * 0.4f);
            grosor(H(HL_HUMERO), brazo, brazo);
            media(H(HL_ESCAPULA), H(HL_CLAVICULA), H(HL_HUMERO));
            grosor(H(HL_RADIO), a.antebrazo, a.antebrazo);
            media(H(HL_CUBITO), H(HL_HUMERO), H(HL_RADIO));
            for (int k = 0; k < 8; ++k) todo(H(HL_CARPO0 + k), a.mano);
            for (int k = 0; k < 5; ++k) todo(H(HL_METACARPO0 + k), a.mano);
            for (int k = 0; k < 14; ++k) S[H(HL_FALANGE_MANO0 + k)] = { a.mano, a.mano * a.dedosLargo, a.mano };
            grosor(H(HL_FEMUR), a.muslo, a.muslo * (1.0f + (a.cuadriceps - 1.0f) * 0.7f));
            media(H(HL_COXAL), H_SACRO, H(HL_FEMUR));
            grosor(H(HL_TIBIA), a.gemelo, a.gemelo);
            grosor(H(HL_PERONE), a.gemelo, a.gemelo);
            media(H(HL_ROTULA), H(HL_FEMUR), H(HL_TIBIA));
            for (int k = 1; k < 7; ++k) todo(H(HL_TARSO0 + k), a.pie);
            media(H(HL_TARSO0), H(HL_TIBIA), H(HL_TARSO0 + 1));
            for (int k = 0; k < 5; ++k) todo(H(HL_METATARSO0 + k), a.pie);
            for (int k = 0; k < 14; ++k) S[H(HL_FALANGE_PIE0 + k)] = { a.pie, a.pie, a.pie * a.pieDedos };
        }
    }

    // ⭐ CADA HUESO COMO CUATERNION DUAL (giro + traslacion), del reposo a la
    // pose actual: 8 floats por hueso (giro x,y,z,w y la parte dual x,y,z,w).
    //
    // Con matrices (skinning lineal) un vertice entre dos huesos muy girados
    // promedia dos posiciones y se mete hacia dentro: codos y rodillas se
    // adelgazan, la muneca girada se retuerce como un caramelo y los dedos se
    // aplastan. Mezclando cuaterniones duales el vertice GIRA alrededor de la
    // articulacion y conserva el volumen.
    // `dq` tiene sitio para NUM_HUESOS_MALLA huesos: los 206 del esqueleto y
    // detras los de los dedos del modelo de bloques (ver HuesoDedo).
    void cuaternionesDuales(const Esqueleto& s, float* dq) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        auto escribir = [&](int b, const Rot& R, const V& t) {
            const detalle::Cuat q = detalle::aCuat(R);
            float* o = dq + b * 8;
            o[0] = q.x; o[1] = q.y; o[2] = q.z; o[3] = q.w;
            o[4] = 0.5f * (t.x * q.w + t.y * q.z - t.z * q.y);
            o[5] = 0.5f * (-t.x * q.z + t.y * q.w + t.z * q.x);
            o[6] = 0.5f * (t.x * q.y - t.y * q.x + t.z * q.w);
            o[7] = -0.5f * (t.x * q.x + t.y * q.y + t.z * q.z);
        };
        for (int e = 0; e < MAX_DEDOS; ++e) {
            const int r = huesoDe(e < numDedos ? dedos[e].lado : 0, HL_RADIO);
            const Rot R = h[r].marco.r.por(detalle::traspuesta(reposo[r].marco.r));
            const V t = resta(h[r].marco.p, R.aplicar(reposo[r].marco.p));
            if (e >= numDedos) { escribir(NUM_HUESOS + e, R, t); continue; }
            // El bloque gira por su falange, luego por el nudillo, y lo que
            // queda lo lleva el antebrazo como a cualquier otro bloque suyo.
            const HuesoDedo& d = dedos[e];
            float a1 = 0.0f, a2 = 0.0f;
            giroDeDedo(s, e, a1, a2);
            const Rot R1 = giroEnEje(d.ejeB, a1);
            Rot G = R1;
            V g = resta(d.nudilloB, R1.aplicar(d.nudilloB));
            if (d.nivel == 2) {
                const Rot R2 = giroEnEje(d.ejeB, a2);
                G = R1.por(R2);
                g = suma(d.nudilloB, R1.aplicar(resta(resta(d.falangeB, R2.aplicar(d.falangeB)), d.nudilloB)));
            }
            escribir(NUM_HUESOS + e, R.por(G), suma(R.aplicar(g), t));
        }
        for (int b = 0; b < NUM_HUESOS; ++b) {
            const Rot R = h[b].marco.r.por(detalle::traspuesta(reposo[b].marco.r));
            const V t = resta(h[b].marco.p, R.aplicar(reposo[b].marco.p));
            const detalle::Cuat q = detalle::aCuat(R);
            float* o = dq + b * 8;
            o[0] = q.x; o[1] = q.y; o[2] = q.z; o[3] = q.w;
            // La parte dual: (0, t) * q / 2.
            o[4] = 0.5f * (t.x * q.w + t.y * q.z - t.z * q.y);
            o[5] = 0.5f * (-t.x * q.z + t.y * q.w + t.z * q.x);
            o[6] = 0.5f * (t.x * q.y - t.y * q.x + t.z * q.w);
            o[7] = -0.5f * (t.x * q.x + t.y * q.y + t.z * q.z);
        }
    }

    // ⭐ DONDE VA LA CAMARA en la pose del esqueleto `s`: EN EL MEDIO DE LA
    // CABEZA, a la altura de los ojos, siguiendola (gira, se inclina y escala
    // con ella).
    //
    // PEDIDO: "que la camara este mas atras, justo en donde esta la cabeza".
    // Antes iba entre los ojos y 3 cm por delante -- en la punta de la nariz --:
    // la vista salia de delante de la cara y los brazos y lo que se lleva en
    // la mano quedaban demasiado cerca. Ahora sale del centro de la cabeza
    // (entre la cara y la nuca), que en primera persona no se dibuja.
    V ojos(const Esqueleto& s) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        const Junta& r0 = ref[H_CABEZA].marco;
        V entre = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        entre.y += 0.045f;               // algo por encima: no roza cejas ni parpados
        entre.z = zDeLaCamara();
        V local = detalle::traspuesta(r0.r).aplicar(resta(entre, r0.p));
        local = escala(local, ajustes.cabeza);
        return suma(h[H_CABEZA].marco.p, h[H_CABEZA].marco.r.aplicar(local));
    }
    // ⭐ UN POCO MAS ATRAS: ARRIBA DEL CUELLO.
    //
    // PEDIDO: "haz un poco mas atras la camara que este arriba del cuello". El
    // medio de la cabeza caia justo sobre el eje del cuello; ahora va 4 cm por
    // detras de el (y nunca por delante del medio de la cabeza): sobre la
    // parte de atras del cuello, todavia dentro de la cabeza, que no se dibuja.
    static constexpr float CAMARA_TRAS_CUELLO = 0.04f;
    float zDeLaCamara() const {
        return (std::min)(pt.cabezaMedioZ, pt.cuelloBase.z) - CAMARA_TRAS_CUELLO;
    }
    // Lo que la camara se ha echado atras respecto a donde iba (la punta de la
    // nariz), en metros. Al mirar abajo el cuerpo se aparta eso mismo de mas.
    float camaraAtras() const {
        return (std::max)(0.0f, (0.5f * (pt.ojo[0].z + pt.ojo[1].z) + 0.03f - zDeLaCamara()) * ajustes.cabeza);
    }
    // La malla en la pose del esqueleto `s`, en la CPU (sin shader, y para
    // las pruebas); sin la cabeza en primera persona.
    void deformar(const Esqueleto& s, Malla& m, bool conCabeza = true) const {
        const std::vector<uint32_t>& idx = indicesDe(conCabeza);
        if (m.i.size() != idx.size()) m.i = idx;
        float DQ[NUM_HUESOS_MALLA * 8];
        cuaternionesDuales(s, DQ);
        m.v.resize(base.size());
        for (size_t i = 0; i < base.size(); ++i) {
            const VerticeJ& v = base[i];
            const float* A = DQ + huesoDeDibujo(i) * 8;
            const float* B = DQ + huesoDeDibujo1(i) * 8;
            const float wa = w0[i];
            const float signo = (A[0] * B[0] + A[1] * B[1] + A[2] * B[2] + A[3] * B[3]) < 0.0f ? -1.0f : 1.0f;
            const float wb = (1.0f - wa) * signo;
            float r[4], d[4];
            for (int k = 0; k < 4; ++k) { r[k] = A[k] * wa + B[k] * wb; d[k] = A[4 + k] * wa + B[4 + k] * wb; }
            const float l = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);
            for (int k = 0; k < 4; ++k) { r[k] /= l; d[k] /= l; }
            const V qv = { r[0], r[1], r[2] };
            auto girar = [&](V p) {
                const V c = suma(cruz(qv, p), escala(p, r[3]));
                return suma(p, escala(cruz(qv, c), 2.0f));
            };
            const V dv = { d[0], d[1], d[2] };
            const V t = escala(suma(resta(escala(dv, r[3]), escala(qv, d[3])), cruz(qv, dv)), 2.0f);
            m.v[i] = { suma(girar(v.p), t), normalizar(girar(v.n)), v.r, v.g, v.b };
        }
    }
    // Desde que altura (en la pose de referencia) no se dibuja nada en primera
    // persona: la cabeza y el cuello.
    float yDeLaCabeza() const { return std::fmin(pt.menton.y, pt.cuelloBase.y) - 0.06f; }
    const std::vector<uint32_t>& indicesDe(bool conCabeza) const {
        if (!conCabeza && indicesSinCabeza.empty()) return indices;
        return conCabeza ? indices : indicesSinCabeza;
    }
};

} // namespace ModeloJugador
