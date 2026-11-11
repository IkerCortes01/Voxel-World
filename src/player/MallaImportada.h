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
        for (Brazo& b : p.brazo) {
            b = Brazo{};
            b.abd = params[0]; b.flex = 0.0f; b.rot = 0.0f; b.codo = 0.0f;
            b.prono = 0.0f; b.munFlex = 0.0f; b.munDesv = 0.0f;
        }
        for (Pierna& g : p.pierna) { g = Pierna{}; g.abd = params[1]; g.rodilla = 0.0f; }
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
        // ⭐ LA MANO DEL ESQUELETO A LA MEDIDA DE LA DEL MODELO: en la cruz,
        // la punta del dedo medio tiene que llegar a donde acaba la mano del
        // modelo. Si no, los nudillos del esqueleto caen en mitad de los
        // dedos de la malla y al cerrar el puno los dedos se rompen.
        {
            const Esqueleto s0 = resolver(poseReferencia(), true);
            float punta = 0.0f;
            for (const VerticeJ& v : modelo) punta = std::fmax(punta, v.p.x);
            const float muneca = s0.mano[1].muneca.p.x, dedo = s0.mano[1].dedo[1][3].p.x;
            if (dedo - muneca > 0.05f && punta > muneca + 0.05f) {
                const float base = params[21] > 0.3f ? params[21] : 1.0f;
                params[21] = sujetar(base * (punta - muneca) / (dedo - muneca), 0.6f, 1.4f);
            }
        }
        const Esqueleto s = resolver(poseReferencia(), true);
        huesosDe(s, ref);
        for (int b = 0; b < NUM_HUESOS; ++b) reposo[b] = ref[b];
        propReposo = proporciones();
        const size_t n = modelo.size();
        // El esqueleto del juego va M::ATRAS por detras del ojo; la malla
        // viene centrada sobre los pies. Se mueve para que el eje del tronco
        // caiga en la columna del esqueleto.
        {
            double zx = 0.0, zz = 0.0;
            int cuantos = 0;
            for (const VerticeJ& v : modelo)
                if (v.p.y > 1.00f && v.p.y < 1.30f && std::fabs(v.p.x) < 0.15f) { zx += v.p.x; zz += v.p.z; ++cuantos; }
            if (cuantos > 0) {
                const float dx = s.pelvis.p.x - (float)(zx / cuantos), dz = s.pelvis.p.z - (float)(zz / cuantos);
                for (VerticeJ& v : modelo) { v.p.x += dx; v.p.z += dz; }
            }
        }
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
        suavizarPesos();
        buscarPuntos(s);
        // En primera persona la camara va en la cabeza (ver ojos()): la cabeza
        // y el cuello no se dibujan. BUG: dejandolos, al mirar abajo la vista
        // atravesaba el cuello y el cuello de la camisa por dentro. Al mirar
        // abajo el cuerpo se echa atras (ver el cuerpo en main.cpp) y la
        // camara queda delante del pecho: se ve el pecho por fuera, como una
        // persona, y lo que se quito queda detras de la camara.
        indicesSinCabeza.clear();
        const V entreOjos = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        for (size_t t = 0; t + 2 < indices.size(); t += 3) {
            bool fuera = false;
            for (int k = 0; k < 3; ++k) {
                const uint32_t v = indices[t + k];
                // La cabeza del menton para arriba; el cuello y los hombros se ven.
                const bool cabeza = ((esDeLaCabeza(h0[v]) && w0[v] > 0.3f) || (esDeLaCabeza(h1[v]) && w0[v] < 0.7f)) &&
                                    modelo[v].p.y > pt.menton.y + 0.005f;
                if (cabeza || largo(resta(modelo[v].p, entreOjos)) < 0.08f) fuera = true;
            }
            if (!fuera) indicesSinCabeza.insert(indicesSinCabeza.end(), { indices[t], indices[t + 1], indices[t + 2] });
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
    void cuaternionesDuales(const Esqueleto& s, float* dq) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
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

    // ⭐ DONDE ESTAN SUS OJOS en la pose del esqueleto `s`: el punto entre los
    // dos, un poco por dentro, siguiendo a la cabeza (gira, se inclina y
    // escala con ella). Ahi va la camara en primera persona.
    V ojos(const Esqueleto& s) const {
        Hueso h[NUM_HUESOS];
        huesosDe(s, h);
        const Junta& r0 = ref[H_CABEZA].marco;
        const V entre = mezclar(pt.ojo[0], pt.ojo[1], 0.5f);
        V local = detalle::traspuesta(r0.r).aplicar(resta(entre, r0.p));
        // Algo por encima y por delante de los ojos: asi la camara no roza los
        // parpados ni las cejas y al mirar abajo ve el cuello y los hombros
        // por fuera, sin meterse en ellos.
        local.y += 0.045f;
        local.z += 0.03f;
        local = escala(local, ajustes.cabeza);
        return suma(h[H_CABEZA].marco.p, h[H_CABEZA].marco.r.aplicar(local));
    }
    // La malla en la pose del esqueleto `s`, en la CPU (sin shader, y para
    // las pruebas); sin la cabeza en primera persona.
    void deformar(const Esqueleto& s, Malla& m, bool conCabeza = true) const {
        const std::vector<uint32_t>& idx = indicesDe(conCabeza);
        if (m.i.size() != idx.size()) m.i = idx;
        float DQ[NUM_HUESOS * 8];
        cuaternionesDuales(s, DQ);
        m.v.resize(base.size());
        for (size_t i = 0; i < base.size(); ++i) {
            const VerticeJ& v = base[i];
            const float* A = DQ + h0[i] * 8;
            const float* B = DQ + h1[i] * 8;
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
    const std::vector<uint32_t>& indicesDe(bool conCabeza) const {
        if (!conCabeza && indicesSinCabeza.empty()) return indices;
        return conCabeza ? indices : indicesSinCabeza;
    }
};

} // namespace ModeloJugador
