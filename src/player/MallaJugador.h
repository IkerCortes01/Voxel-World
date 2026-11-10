#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include "ModeloJugador.h"

// ============================================================================
// LA PIEL DEL JUGADOR: MUSCULOS, TENDONES, ARTICULACIONES Y VENAS
// ============================================================================
// Sobre el esqueleto de ModeloJugador.h se tiende una piel continua. Cada
// tramo (brazo, antebrazo, palma, falange, muslo, pierna, pie...) es un tubo
// cuya SECCION cambia a lo largo: asi se modelan los musculos como bultos que
// dependen de la pose:
//
//   BICEPS         se hincha y se acorta al doblar el codo.
//   ANTEBRAZO      los flexores crecen al cerrar el puno; en la muneca se
//                  marcan los tendones de la cara palmar.
//   DORSO DE MANO  los tendones extensores van de la muneca a cada nudillo y
//                  se marcan con los dedos estirados; los nudillos, con el
//                  puno cerrado.
//   MUSLO          cuadriceps, la "gota" del vasto interno sobre la rodilla e
//                  isquiotibiales.
//   PIERNA         la cresta de la tibia delante; los gemelos detras, que se
//                  contraen al empujar con la punta; el tendon de Aquiles.
//   PIE            empeine, arco por dentro, planta plana, tendones del dorso.
//
// Las articulaciones se cubren con elipsoides: codo (con el olecranon),
// rotula que se desliza al doblar la rodilla, tobillos (maleolos), nudillos.
// Uñas en los 10 dedos de las manos y en los 10 de los pies.
//
// LAS VENAS Y LAS ARTERIAS: cada mundo tiene un cuerpo con su propio dibujo
// de venas, al azar pero donde estan en una persona (cefalica, basilica,
// mediana del codo, red del dorso de la mano, safena, arco dorsal del pie).
// Son tiras en relieve pegadas a la piel, mas finas en los extremos. Se
// hinchan con el esfuerzo y el calor, y mas en la mano que cuelga. Las
// arterias de verdad casi no se ven: aqui la radial (muneca) y la pedia (pie)
// son una linea tenue que LATE con el pulso.
//
// El resultado es una malla en el espacio local de ModeloJugador (pies en el
// origen, +Z delante): main.cpp solo la gira, la ilumina y la dibuja.
// ============================================================================

namespace ModeloJugador {

struct VerticeJ { V p, n; float r, g, b; };
struct Malla {
    std::vector<VerticeJ> v;
    std::vector<uint32_t> i;
    void limpiar() { v.clear(); i.clear(); }
};

struct Color { float r, g, b; };
inline Color mezclaC(Color a, Color b, float t) {
    return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t };
}
inline Color tinte(Color a, float kr, float kg, float kb) { return { a.r * kr, a.g * kg, a.b * kb }; }

// Un bulto compacto: 1 en el centro, 0 a `ancho` o mas. Polinomico, sin exp.
inline float bulto(float d, float ancho) {
    const float x = d / ancho;
    if (x <= -1.0f || x >= 1.0f) return 0.0f;
    const float u = 1.0f - x * x;
    return u * u;
}
// Diferencia de angulos en [-PI, PI].
inline float difAng(float a, float b) {
    float d = std::fmod(a - b + PI, 2.0f * PI);
    if (d < 0) d += 2.0f * PI;
    return d - PI;
}
// Un angulo entre a y b, por el arco corto.
inline float entre(float a, float b, float t) { return a + difAng(b, a) * t; }

// Los lados de cada tubo, vistos en su seccion: +Z del marco es delante (o la
// palma, o el empeine) y +X es la derecha.
constexpr float DELANTE = PI * 0.5f;
constexpr float DETRAS = -PI * 0.5f;
inline float fuera(float sx) { return sx > 0 ? 0.0f : PI; }     // lado de fuera
inline float dentro(float sx) { return sx > 0 ? PI : 0.0f; }    // lado de dentro

// ----------------------------------------------------------------------------
// UN TRAMO: eje (centro y marco segun t) y seccion (desplazamiento en XZ)
// ----------------------------------------------------------------------------
template <class Eje, class Sec>
struct Tubo {
    Eje eje;
    Sec sec;
    float t0, t1;
    V centro(float t) const { V c; Rot r; eje(t, c, r); return c; }
    V punto(float t, float a) const {
        V c; Rot r;
        eje(t, c, r);
        float dx, dz;
        sec(t, a, dx, dz);
        return suma(c, r.aplicar({ dx, 0, dz }));
    }
};
template <class E, class S>
inline Tubo<E, S> tubo(E e, S s, float t0 = 0.0f, float t1 = 1.0f) { return { e, s, t0, t1 }; }

// Una elipse con un extra de radio (musculo, tendon...).
inline void elipse(float a, float rx, float rz, float extra, float& dx, float& dz) {
    dx = (rx + extra) * std::cos(a);
    dz = (rz + extra) * std::sin(a);
}

// Tramo recto colgando de una junta: el eje va por -Y del marco.
struct EjeRecto {
    V o; Rot r; float L;
    void operator()(float t, V& c, Rot& rr) const { c = suma(o, r.aplicar({ 0, -L * t, 0 })); rr = r; }
};
// Recto hacia +Z (pie y dedos del pie): la seccion queda en el plano XY.
struct EjeDelante {
    V o; Rot r; float L;
    void operator()(float t, V& c, Rot& rr) const {
        c = suma(o, r.aplicar({ 0, 0, L * t }));
        rr = r.por(Rot::enX(PI * 0.5f));
    }
};

template <class T, class C>
inline void dibujarTubo(Malla& m, const T& tb, int anillos, int lados, C color,
                        bool tapa0, bool tapa1) {
    if (anillos < 1) anillos = 1;
    if (lados < 3) lados = 3;
    const uint32_t base = (uint32_t)m.v.size();
    for (int i = 0; i <= anillos; ++i) {
        const float t = tb.t0 + (tb.t1 - tb.t0) * i / anillos;
        for (int j = 0; j < lados; ++j) {
            const float a = 2.0f * PI * j / lados;
            const Color c = color(t, a);
            m.v.push_back({ tb.punto(t, a), { 0, 0, 0 }, c.r, c.g, c.b });
        }
    }
    // Normales de la propia rejilla: asi los bultos se sombrean como bultos.
    auto P = [&](int i, int j) -> const V& {
        i = i < 0 ? 0 : (i > anillos ? anillos : i);
        j = (j + lados) % lados;
        return m.v[base + i * lados + j].p;
    };
    for (int i = 0; i <= anillos; ++i) {
        V c = { 0, 0, 0 };
        for (int j = 0; j < lados; ++j) c = suma(c, P(i, j));
        c = escala(c, 1.0f / lados);
        for (int j = 0; j < lados; ++j) {
            V n = cruz(resta(P(i + 1, j), P(i - 1, j)), resta(P(i, j + 1), P(i, j - 1)));
            if (punto(n, resta(P(i, j), c)) < 0) n = escala(n, -1.0f);
            m.v[base + i * lados + j].n = normalizar(n);
        }
    }
    for (int i = 0; i < anillos; ++i)
        for (int j = 0; j < lados; ++j) {
            const uint32_t a = base + i * lados + j, b = base + i * lados + (j + 1) % lados;
            const uint32_t c = a + lados, d = b + lados;
            m.i.insert(m.i.end(), { a, c, d, a, d, b });
        }
    // Tapas: un abanico desde el centro del anillo.
    for (int k = 0; k < 2; ++k) {
        if (!(k == 0 ? tapa0 : tapa1)) continue;
        const int anillo = k == 0 ? 0 : anillos;
        const int vecino = k == 0 ? 1 : anillos - 1;
        V c = { 0, 0, 0 }, cv = { 0, 0, 0 };
        for (int j = 0; j < lados; ++j) { c = suma(c, P(anillo, j)); cv = suma(cv, P(vecino, j)); }
        c = escala(c, 1.0f / lados);
        cv = escala(cv, 1.0f / lados);
        const VerticeJ& muestra = m.v[base + anillo * lados];
        const uint32_t centro = (uint32_t)m.v.size();
        m.v.push_back({ c, normalizar(resta(c, cv)), muestra.r, muestra.g, muestra.b });
        for (int j = 0; j < lados; ++j)
            m.i.insert(m.i.end(), { centro, base + anillo * lados + j,
                                    base + anillo * lados + (j + 1) % lados });
    }
}

inline void elipsoide(Malla& m, V c, const Rot& r, V radio, int lat, int lon, Color col) {
    if (lat < 2) lat = 2;
    if (lon < 3) lon = 3;
    const uint32_t base = (uint32_t)m.v.size();
    for (int i = 0; i <= lat; ++i) {
        const float th = PI * i / lat;
        const float sy = std::cos(th), sr = std::sin(th);
        for (int j = 0; j < lon; ++j) {
            const float ph = 2.0f * PI * j / lon;
            const V u = { sr * std::cos(ph), sy, sr * std::sin(ph) };
            const V p = { u.x * radio.x, u.y * radio.y, u.z * radio.z };
            const V n = { u.x / radio.x, u.y / radio.y, u.z / radio.z };
            m.v.push_back({ suma(c, r.aplicar(p)), normalizar(r.aplicar(n)), col.r, col.g, col.b });
        }
    }
    for (int i = 0; i < lat; ++i)
        for (int j = 0; j < lon; ++j) {
            const uint32_t a = base + i * lon + j, b = base + i * lon + (j + 1) % lon;
            m.i.insert(m.i.end(), { a, a + lon, b + lon, a, b + lon, b });
        }
}

// ----------------------------------------------------------------------------
// LAS VENAS
// ----------------------------------------------------------------------------
enum Region : int8_t {
    BRAZO_I, BRAZO_D, ANTE_I, ANTE_D, MANO_I, MANO_D,
    MUSLO_I, MUSLO_D, PIERNA_I, PIERNA_D, PIE_I, PIE_D, NUM_REGIONES
};

struct Vena {
    int8_t region;
    float t0, t1;          // a lo largo del tramo
    float a0, a1;          // angulo en la seccion al principio y al final
    float onda, frec, fase;// lo que serpentea
    float ancho, alto;     // metros
    bool arteria;
};

// ⭐ LA FORMA DEL AVATAR (ver Avatar.h). Todo a 1 (o 0) es el cuerpo de
// siempre: los factores multiplican los grosores de cada tramo, y la mano y
// el pie se escalan enteros alrededor de la muneca y del tobillo.
struct Forma {
    float hombros = 1, deltoides = 1, brazo = 1, biceps = 1, antebrazo = 1;
    float mano = 1, dedos = 1, unaMm = 1.0f;
    float pecho = 1, busto = 0, cintura = 1, abdomen = 0, cadera = 1, gluteos = 1;
    float muslo = 1, gemelo = 1, pie = 1, pieAncho = 1, arco = 1, musculo = 1;
};

struct Piel {
    Forma forma;
    Color piel{ 0.86f, 0.67f, 0.53f };
    Color camisa{ 0.24f, 0.45f, 0.66f };
    Color pantalon{ 0.20f, 0.22f, 0.30f };
    float prominencia = 0.7f;  // cuanto se le marcan las venas a esta persona
    std::vector<Vena> venas;
};

struct Azar {
    uint32_t s;
    float operator()() {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return (s & 0xFFFFFFu) / 16777215.0f;
    }
    float entre(float a, float b) { return a + (b - a) * (*this)(); }
};

// El dibujo de venas de un cuerpo. Mismo numero, mismo cuerpo; cada lado se
// sortea por separado, como en una persona, que no es simetrica.
inline Piel generarPiel(uint32_t semilla) {
    Piel p;
    Azar z{ semilla * 2654435761u + 0x6D2B79F5u };
    if (z.s == 0) z.s = 1;
    for (int k = 0; k < 4; ++k) z();
    p.prominencia = z.entre(0.45f, 1.0f);

    auto vena = [&](int8_t region, float t0, float t1, float a0, float a1, float ancho,
                    float alto, bool arteria = false) {
        Vena v;
        v.region = region;
        v.t0 = t0; v.t1 = t1;
        v.a0 = a0; v.a1 = a1;
        v.onda = z.entre(0.03f, 0.14f);
        v.frec = z.entre(0.6f, 2.2f);
        v.fase = z.entre(0.0f, 2.0f * PI);
        v.ancho = ancho * z.entre(0.8f, 1.2f);
        v.alto = alto * z.entre(0.7f, 1.3f);
        v.arteria = arteria;
        p.venas.push_back(v);
    };

    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const int8_t brazo = (int8_t)(BRAZO_I + l), ante = (int8_t)(ANTE_I + l),
                     mano = (int8_t)(MANO_I + l), muslo = (int8_t)(MUSLO_I + l),
                     pierna = (int8_t)(PIERNA_I + l), pie = (int8_t)(PIE_I + l);

        // --- Antebrazo ---
        const float cefalica = entre(fuera(sx), DELANTE, z.entre(0.15f, 0.40f));
        const float basilica = entre(dentro(sx), DELANTE, z.entre(0.20f, 0.45f));
        vena(ante, z.entre(0.0f, 0.1f), z.entre(0.92f, 0.99f), cefalica,
             cefalica + z.entre(-0.3f, 0.3f), 0.0022f, 0.0011f);
        if (z() < 0.9f)
            vena(ante, z.entre(0.1f, 0.25f), z.entre(0.85f, 0.95f), basilica,
                 basilica + z.entre(-0.3f, 0.3f), 0.0020f, 0.0010f);
        if (z() < 0.6f) {
            const float a = DELANTE + z.entre(-0.3f, 0.3f);
            vena(ante, z.entre(0.15f, 0.3f), z.entre(0.7f, 0.88f), a, a + z.entre(-0.2f, 0.2f),
                 0.0015f, 0.0007f);
        }
        if (z() < 0.8f)   // la mediana del codo, en diagonal por la flexura
            vena(ante, 0.0f, z.entre(0.14f, 0.22f), cefalica, basilica, 0.0021f, 0.0010f);
        if (z() < 0.9f) { // la del dorso, que sigue en la mano
            const float a = DETRAS + z.entre(-0.4f, 0.4f);
            vena(ante, z.entre(0.40f, 0.6f), 1.0f, a, DETRAS + z.entre(-0.2f, 0.2f), 0.0019f, 0.0010f);
        }
        // La arteria radial: en la muneca, del lado del pulgar.
        const float radial = entre(DELANTE, fuera(sx), 0.45f);
        vena(ante, 0.72f, 0.99f, radial, radial, 0.0012f, 0.0003f, true);

        // --- Brazo, bajo la manga ---
        const float cef2 = entre(fuera(sx), DELANTE, z.entre(0.25f, 0.45f));
        vena(brazo, 0.46f, 1.0f, cef2, cefalica, 0.0021f, 0.0009f);
        if (z() < 0.5f) {
            const float a = entre(dentro(sx), DELANTE, 0.3f);
            vena(brazo, 0.6f, 1.0f, a, basilica, 0.0018f, 0.0007f);
        }

        // --- Dorso de la mano: entre los nudillos hacia la muneca, y un arco ---
        static const float NX[4] = { 0.030f, 0.010f, -0.011f, -0.030f };
        const int nVenas = 2 + (int)(z() * 2.99f);
        for (int k = 0; k < nVenas; ++k) {
            const int g = (k * 3) / nVenas;          // hueco entre dedos 0..2
            const float x = sx * 0.5f * (NX[g] + NX[g + 1]);
            const float aNudillo = -std::acos(sujetar(x / 0.043f, -0.9f, 0.9f));
            vena(mano, z.entre(0.05f, 0.15f), z.entre(0.80f, 0.92f),
                 DETRAS + z.entre(-0.35f, 0.35f), aNudillo, 0.0017f, 0.0012f);
        }
        if (z() < 0.85f) {
            const float t = z.entre(0.42f, 0.6f);
            vena(mano, t, t + z.entre(0.04f, 0.12f), DETRAS - z.entre(0.5f, 0.8f),
                 DETRAS + z.entre(0.5f, 0.8f), 0.0015f, 0.0010f);
        }

        // --- Pierna: la safena por dentro, y algun ramal ---
        const float safena = entre(dentro(sx), DELANTE, z.entre(0.15f, 0.35f));
        vena(pierna, 0.0f, 0.98f, safena, entre(dentro(sx), DELANTE, 0.5f), 0.0023f, 0.0009f);
        const int ramas = (int)(z() * 2.5f);
        for (int k = 0; k < ramas; ++k) {
            const float t = z.entre(0.25f, 0.75f);
            vena(pierna, t, t + z.entre(0.12f, 0.25f), safena + z.entre(-0.9f, -0.4f) * sx,
                 safena, 0.0016f, 0.0006f);
        }
        if (z() < 0.6f) {
            const float a = entre(DETRAS, fuera(sx), 0.4f);
            vena(pierna, z.entre(0.3f, 0.5f), 0.95f, a, a + z.entre(-0.3f, 0.3f), 0.0016f, 0.0005f);
        }
        vena(muslo, 0.62f, 1.0f, entre(dentro(sx), DELANTE, 0.1f), dentro(sx), 0.0020f, 0.0006f);

        // --- Pie: arco dorsal, marginales y la arteria pedia ---
        const float arco = z.entre(0.66f, 0.74f);
        vena(pie, arco, arco + 0.05f, entre(DELANTE, dentro(sx), 0.65f),
             entre(DELANTE, fuera(sx), 0.65f), 0.0019f, 0.0010f);
        vena(pie, 0.12f, arco + 0.02f, entre(DELANTE, dentro(sx), z.entre(0.45f, 0.65f)),
             entre(DELANTE, dentro(sx), 0.65f), 0.0020f, 0.0010f);
        if (z() < 0.7f)
            vena(pie, 0.18f, arco + 0.03f, entre(DELANTE, fuera(sx), 0.6f),
                 entre(DELANTE, fuera(sx), 0.65f), 0.0016f, 0.0007f);
        vena(pie, 0.25f, 0.70f, DELANTE + 0.12f * sx, DELANTE + 0.25f * sx, 0.0011f, 0.0003f, true);
    }
    return p;
}

// Una vena: una tira con lomo, pegada a la superficie del tramo. Se afina en
// los extremos para que nazca y muera en la piel.
template <class T>
inline void dibujarVena(Malla& m, const T& tb, const Vena& v, Color piel, float visible,
                        float latido, int muestras) {
    if (visible <= 0.01f || muestras < 2) return;
    const Color colorVena = v.arteria ? tinte(piel, 1.0f, 0.68f, 0.68f) : tinte(piel, 0.55f, 0.68f, 0.95f);
    // PEDIDO: "venas no tan visibles, pero realistas igual". A traves de la
    // piel una vena es un tono apenas azulado y un relieve suave, no una
    // linea pintada: poco color y poco alto.
    const float fuerza = v.arteria ? 0.12f : 0.28f;
    const float alto = (v.arteria ? v.alto + 0.0004f * latido : v.alto) * 0.6f;

    std::vector<V> P(muestras + 1), N(muestras + 1);
    for (int k = 0; k <= muestras; ++k) {
        const float u = (float)k / muestras;
        const float t = v.t0 + (v.t1 - v.t0) * u;
        const float a = v.a0 + difAng(v.a1, v.a0) * u + v.onda * std::sin(2.0f * PI * v.frec * u + v.fase);
        P[k] = tb.punto(t, a);
        const V pa = tb.punto(t, a + 0.03f);
        const V pt = tb.punto(t + 0.01f, a);
        V n = cruz(resta(pt, P[k]), resta(pa, P[k]));
        if (punto(n, resta(P[k], tb.centro(t))) < 0) n = escala(n, -1.0f);
        N[k] = normalizar(n);
    }
    const uint32_t base = (uint32_t)m.v.size();
    for (int k = 0; k <= muestras; ++k) {
        const float u = (float)k / muestras;
        // 0 en los extremos. sin(PI) en float sale -8e-8: sin el tope, NaN.
        const float fin = std::sqrt(std::fmax(0.0f, std::sin(PI * u)));
        const V tg = resta(P[k < muestras ? k + 1 : k], P[k > 0 ? k - 1 : k]);
        const V B = normalizar(cruz(N[k], tg));
        const float w = v.ancho * (0.35f + 0.65f * fin);
        const float h = alto * visible * fin;
        const float mz = sujetar(fuerza * visible * fin, 0.0f, 0.75f);
        const Color cc = mezclaC(piel, colorVena, mz), cb = mezclaC(piel, colorVena, mz * 0.25f);
        const V nI = normalizar(suma(escala(N[k], 0.6f), escala(B, 0.8f)));
        const V nD = normalizar(suma(escala(N[k], 0.6f), escala(B, -0.8f)));
        m.v.push_back({ suma(suma(P[k], escala(B, w)), escala(N[k], 0.0002f)), nI, cb.r, cb.g, cb.b });
        m.v.push_back({ suma(P[k], escala(N[k], 0.0003f + h)), N[k], cc.r, cc.g, cc.b });
        m.v.push_back({ suma(suma(P[k], escala(B, -w)), escala(N[k], 0.0002f)), nD, cb.r, cb.g, cb.b });
    }
    for (int k = 0; k < muestras; ++k)
        for (int s = 0; s < 2; ++s) {
            const uint32_t a = base + k * 3 + s, b = a + 1, c = a + 3, d = a + 4;
            m.i.insert(m.i.end(), { a, c, d, a, d, b });
        }
}

// ----------------------------------------------------------------------------
// LAS SECCIONES DE LOS MUSCULOS (libres para poder probarlas)
// ----------------------------------------------------------------------------
// Brazo: t 0 hombro .. 1 codo. `flexN` = codo / su maximo.
inline void seccionBrazo(float t, float a, float sx, float flexN, float& dx, float& dz) {
    const float base = 0.046f - 0.010f * t;
    const float biceps = (0.006f + 0.010f * flexN) * bulto(t - (0.55f - 0.08f * flexN), 0.30f) *
                         bulto(difAng(a, DELANTE), 1.1f);
    const float triceps = 0.007f * bulto(t - 0.35f, 0.32f) * bulto(difAng(a, DETRAS), 1.2f);
    const float deltoides = 0.009f * bulto(t - 0.05f, 0.25f) * bulto(difAng(a, fuera(sx)), 1.4f);
    const float epicondilos = 0.004f * bulto(t - 0.97f, 0.08f) *
                              (bulto(difAng(a, fuera(sx)), 0.6f) + bulto(difAng(a, dentro(sx)), 0.6f));
    elipse(a, base, base * 0.95f, biceps + triceps + deltoides + epicondilos, dx, dz);
}

// Antebrazo: t 0 codo .. 1 muneca, en el marco que se tuerce con el radio.
inline void seccionAntebrazo(float t, float a, float sx, float puno, float& dx, float& dz) {
    const float rx = 0.040f - 0.011f * std::pow(t, 1.3f);
    const float rz = 0.039f - 0.020f * std::pow(t, 1.2f);
    const float braquio = 0.008f * bulto(t - 0.22f, 0.28f) * bulto(difAng(a, entre(fuera(sx), DELANTE, 0.5f)), 1.0f);
    const float flexores = (0.007f + 0.005f * puno) * bulto(t - 0.28f, 0.32f) *
                           bulto(difAng(a, entre(dentro(sx), DELANTE, 0.5f)), 1.1f);
    const float extensores = (0.006f + 0.002f * puno) * bulto(t - 0.30f, 0.32f) *
                             bulto(difAng(a, entre(fuera(sx), DETRAS, 0.5f)), 1.1f);
    // Los tendones de la cara palmar, en la muneca: con el puno cerrado saltan.
    float tendones = 0.0f;
    for (float o : { -0.35f, 0.0f, 0.30f })
        tendones += bulto(difAng(a, DELANTE + o), 0.13f);
    tendones *= (0.0012f + 0.0016f * puno) * bulto(t - 0.85f, 0.17f);
    // Cabeza del cubito (el hueso del lado del menique) y apofisis del radio.
    const float cubito = 0.004f * bulto(t - 0.94f, 0.07f) * bulto(difAng(a, entre(dentro(sx), DETRAS, 0.5f)), 0.6f);
    const float radio = 0.0025f * bulto(t - 0.97f, 0.06f) * bulto(difAng(a, fuera(sx)), 0.6f);
    elipse(a, rx, rz, braquio + flexores + extensores + tendones + cubito + radio, dx, dz);
}

// Muslo: t 0 cadera .. 1 rodilla.
inline void seccionMuslo(float t, float a, float sx, float& dx, float& dz) {
    const float base = 0.080f - 0.030f * t;
    const float cuadriceps = 0.010f * bulto(t - 0.45f, 0.45f) * bulto(difAng(a, DELANTE), 1.1f);
    const float vastoInterno = 0.010f * bulto(t - 0.86f, 0.11f) *
                               bulto(difAng(a, entre(dentro(sx), DELANTE, 0.5f)), 0.7f);
    const float vastoExterno = 0.007f * bulto(t - 0.5f, 0.4f) * bulto(difAng(a, fuera(sx)), 0.9f);
    const float isquios = 0.008f * bulto(t - 0.45f, 0.4f) * bulto(difAng(a, DETRAS), 1.1f);
    elipse(a, base, base * 0.95f, cuadriceps + vastoInterno + vastoExterno + isquios, dx, dz);
}

// Pierna: t 0 rodilla .. 1 tobillo. `punta` = cuanto empuja con la punta (0..1).
inline void seccionPierna(float t, float a, float sx, float punta, float& dx, float& dz) {
    const float base = 0.043f - 0.015f * t;
    const float tibia = 0.003f * bulto(difAng(a, DELANTE), 0.35f) * bulto(t - 0.45f, 0.45f);
    const float k = 1.0f + 0.35f * punta;
    const float gemeloIn = 0.020f * k * bulto(t - (0.30f - 0.04f * punta), 0.19f) *
                           bulto(difAng(a, entre(DETRAS, dentro(sx), 0.5f)), 1.2f);
    const float gemeloEx = 0.015f * k * bulto(t - (0.27f - 0.04f * punta), 0.17f) *
                           bulto(difAng(a, entre(DETRAS, fuera(sx), 0.5f)), 1.1f);
    const float soleo = 0.008f * bulto(t - 0.55f, 0.24f) * bulto(difAng(a, DETRAS), 1.3f);
    // El Aquiles: a los lados se hunde y en medio queda el tendon.
    const float aquiles = bulto(t - 0.88f, 0.15f) *
                          (0.003f * bulto(difAng(a, DETRAS), 0.35f) -
                           0.004f * (bulto(difAng(a, DETRAS + 0.7f), 0.4f) + bulto(difAng(a, DETRAS - 0.7f), 0.4f)));
    elipse(a, base, base * 0.97f, tibia + gemeloIn + gemeloEx + soleo + aquiles, dx, dz);
}

// Escala los vertices [desde, fin) en el marco (o, r): kx, ky, kz en los ejes
// de ese marco. Es como crece una mano o un pie entero sin tocar el hueso.
inline void escalarEnMarco(Malla& m, uint32_t desde, V o, const Rot& r, float kx, float ky, float kz) {
    for (uint32_t i = desde; i < (uint32_t)m.v.size(); ++i) {
        const V d = resta(m.v[i].p, o);
        // Al marco (la traspuesta es la inversa), escalar, y de vuelta.
        V l = { r.m[0] * d.x + r.m[3] * d.y + r.m[6] * d.z,
                r.m[1] * d.x + r.m[4] * d.y + r.m[7] * d.z,
                r.m[2] * d.x + r.m[5] * d.y + r.m[8] * d.z };
        l = { l.x * kx, l.y * ky, l.z * kz };
        m.v[i].p = suma(o, r.aplicar(l));
        V n = m.v[i].n;
        V ln = { r.m[0] * n.x + r.m[3] * n.y + r.m[6] * n.z,
                 r.m[1] * n.x + r.m[4] * n.y + r.m[7] * n.z,
                 r.m[2] * n.x + r.m[5] * n.y + r.m[8] * n.z };
        ln = { ln.x / kx, ln.y / ky, ln.z / kz };
        m.v[i].n = normalizar(r.aplicar(ln));
    }
}

// ----------------------------------------------------------------------------
// LA MALLA ENTERA
// ----------------------------------------------------------------------------
struct Calidad {
    int detalle = 2;      // 0 BAJO, 1 MEDIO, 2 ALTO (Opciones::DETALLE)
    bool venas = true;
};

inline void generar(const Esqueleto& s, const Pose& p, const Piel& piel, const Fisiologia& fis,
                    const Calidad& q, Malla& m) {
    m.limpiar();
    const float kRes = q.detalle <= 0 ? 0.5f : (q.detalle == 1 ? 0.75f : 1.0f);
    auto N = [&](int n) { const int r = (int)(n * kRes + 0.5f); return r < 3 ? 3 : r; };
    const int muestrasVena = q.detalle >= 2 ? 16 : 10;

    const Color P = piel.piel;
    const Color camisa = piel.camisa, pantalon = piel.pantalon;
    const Forma& F = piel.forma;
    // Las costuras y los dobladillos, un poco mas oscuros que la tela.
    const Color camisaCostura = tinte(camisa, 0.80f, 0.80f, 0.82f);
    const Color pantalonCostura = tinte(pantalon, 0.72f, 0.72f, 0.74f);
    const Color boton = mezclaC(camisa, Color{ 0.92f, 0.90f, 0.85f }, 0.75f);
    const Color articulacion = tinte(P, 1.04f, 0.93f, 0.92f);   // nudillos, codos, rodillas
    const Color palma = tinte(P, 1.06f, 1.00f, 0.97f);
    // Unas claras: es lo que mas separa un dedo de otro vistos desde arriba.
    const Color una = mezclaC(tinte(P, 1.12f, 1.04f, 1.04f), Color{ 0.98f, 0.88f, 0.86f }, 0.65f);
    // Los pliegues entre dedos, algo mas oscuros: dibujan el contorno de cada
    // uno contra el de al lado.
    auto contornoDedo = [P](float, float a) {
        const float c = std::cos(a);
        return tinte(P, 1.0f - 0.22f * c * c, 1.0f - 0.25f * c * c, 1.0f - 0.25f * c * c);
    };
    const float visibleBase = piel.prominencia * (0.55f + 0.35f * fis.calor + 0.15f * fis.esfuerzo);
    // La piel no es de un solo color: manchas suaves y grandes, mas rojiza
    // por la cara interna (donde es fina y la sangre esta cerca), y nada que
    // se repita a simple vista.
    auto pielCon = [P](float semilla, float rojizo) {
        return [P, semilla, rojizo](float t, float a) {
            const float n = 0.030f * std::sin(a * 3.0f + t * 9.0f + semilla) +
                            0.020f * std::sin(a * 7.0f - t * 17.0f + semilla * 1.7f) +
                            0.012f * std::sin(t * 31.0f + a * 2.0f + semilla * 0.3f);
            const float rojo = rojizo * (0.5f + 0.5f * std::cos(a - PI * 0.5f));   // cara de delante
            return Color{ P.r * (1.0f + n + 0.03f * rojo), P.g * (1.0f + n * 0.9f - 0.01f * rojo),
                          P.b * (1.0f + n * 0.8f - 0.015f * rojo) };
        };
    };
    const float latido = fis.latido();

    auto plano = [](Color c) { return [c](float, float) { return c; }; };

    // ================= TORSO =================
    {
        const float L = s.largo.espalda + 0.10f;
        auto eje = [&](float t, V& c, Rot& r) {
            r = s.pelvis.r.por(Rot::enZ((-0.8f * p.pelvisLado - p.torsoLado) * columnaLado(t)))
                       .por(Rot::enY(p.giroTorso * columnaGiro(t))).por(Rot::enX(-p.inclinacion * suave(t * 2.0f)));
            c = suma(s.pelvis.p, r.aplicar({ 0, -0.10f + L * t, 0 }));
        };
        static const float KX[][2] = { { 0.0f, 0.150f }, { 0.18f, 0.165f }, { 0.38f, 0.145f },
                                       { 0.70f, 0.175f }, { 0.86f, 0.180f }, { 0.95f, 0.130f },
                                       { 1.0f, 0.065f } };
        static const float KZ[][2] = { { 0.0f, 0.095f }, { 0.18f, 0.105f }, { 0.38f, 0.095f },
                                       { 0.70f, 0.115f }, { 0.86f, 0.100f }, { 0.95f, 0.080f },
                                       { 1.0f, 0.055f } };
        auto curva = [](const float (*k)[2], int n, float t) {
            for (int i = 1; i < n; ++i)
                if (t <= k[i][0]) {
                    const float u = suave((t - k[i - 1][0]) / (k[i][0] - k[i - 1][0]));
                    return k[i - 1][1] + (k[i][1] - k[i - 1][1]) * u;
                }
            return k[n - 1][1];
        };
        const float resp = p.respira;
        auto sec = [&](float t, float a, float& dx, float& dz) {
            float rx = curva(KX, 7, t), rz = curva(KZ, 7, t);
            const float pecho = bulto(t - 0.7f, 0.3f);
            rz *= 1.0f + 0.03f * resp * pecho;
            rx *= 1.0f + 0.012f * resp * pecho;
            const float pectorales = 0.012f * bulto(t - 0.72f, 0.13f) *
                                     (bulto(difAng(a, DELANTE + 0.55f), 0.6f) + bulto(difAng(a, DELANTE - 0.55f), 0.6f));
            const float escapulas = 0.008f * bulto(t - 0.78f, 0.12f) *
                                    (bulto(difAng(a, DETRAS + 0.6f), 0.6f) + bulto(difAng(a, DETRAS - 0.6f), 0.6f));
            const float gluteos = 0.014f * F.gluteos * bulto(t - 0.12f, 0.12f) *
                                  (bulto(difAng(a, DETRAS + 0.5f), 0.7f) + bulto(difAng(a, DETRAS - 0.5f), 0.7f));
            // Las arrugas de la camisa: en la cintura, donde la tela sobra, y
            // unas diagonales bajo las axilas.
            const float arrugas = 0.0015f * std::sin(a * 7.0f + t * 23.0f) * bulto(t - 0.32f, 0.15f)
                                + 0.0010f * std::sin(a * 5.0f - t * 31.0f) * bulto(t - 0.80f, 0.08f)
                                  * (bulto(difAng(a, 0.0f), 0.5f) + bulto(difAng(a, PI), 0.5f));
            // ⭐ LAS PROPORCIONES DEL AVATAR: cadera abajo, cintura en medio,
            // pecho arriba; cada una pesa en su altura del tronco.
            const float kCadera = (F.cadera - 1.0f) * bulto(t - 0.12f, 0.26f);
            const float kCintura = (F.cintura - 1.0f) * bulto(t - 0.40f, 0.20f);
            const float kPecho = (F.pecho - 1.0f) * bulto(t - 0.78f, 0.28f);
            const float k = 1.0f + kCadera + kCintura + kPecho;
            rx *= k;
            rz *= 1.0f + (k - 1.0f) * 0.6f;
            const float busto = 0.045f * F.busto * bulto(t - 0.71f, 0.13f) *
                                (bulto(difAng(a, DELANTE + 0.42f), 0.55f) + bulto(difAng(a, DELANTE - 0.42f), 0.55f));
            const float abdomen = 0.040f * F.abdomen * bulto(t - 0.37f, 0.22f) * bulto(difAng(a, DELANTE), 1.3f);
            // LA ROPA: el dobladillo de la camisa cae por fuera del short, la
            // pretina del short marca la cintura y el cuello lleva su tira.
            const float dobladillo = 0.0045f * bulto(t - 0.215f, 0.025f);
            const float pretina = 0.0025f * bulto(t - 0.185f, 0.022f);
            const float cuelloTira = 0.0040f * bulto(t - 0.960f, 0.020f);
            elipse(a, rx, rz, pectorales * F.musculo + escapulas * F.musculo + gluteos + arrugas +
                              busto + abdomen + dobladillo + pretina + cuelloTira, dx, dz);
        };
        auto color = [&](float t, float a) {
            if (t < 0.20f) {
                // La pretina y la costura del costado del short.
                if (t > 0.165f) return pantalonCostura;
                if (bulto(difAng(a, 0.0f), 0.05f) + bulto(difAng(a, PI), 0.05f) > 0.5f) return pantalonCostura;
                return pantalon;
            }
            if (t > 0.975f) return P;          // el cuello asoma por la camisa
            if (t > 0.945f) return camisaCostura;   // la tira del cuello
            if (t < 0.232f) return camisaCostura;   // el dobladillo
            // La tapeta de los botones, una franja por delante.
            if (t > 0.40f && bulto(difAng(a, DELANTE), 0.07f) > 0.3f) return camisaCostura;
            // Pliegues: la tela se sombrea un poco en los surcos.
            const float pliegue = 1.0f - 0.05f * (0.5f + 0.5f * std::sin(a * 7.0f + t * 23.0f)) * bulto(t - 0.32f, 0.15f);
            return tinte(camisa, pliegue, pliegue, pliegue);
        };
        const auto torso = tubo(eje, sec);
        dibujarTubo(m, torso, N(18), N(28), color, true, true);
        // Tres botones en la tapeta, y el nudo del cordon del short.
        for (float tb : { 0.55f, 0.68f, 0.81f }) {
            V c; Rot r;
            eje(tb, c, r);
            const V pb = torso.punto(tb, DELANTE);
            elipsoide(m, suma(pb, r.aplicar({ 0, 0, 0.0016f })), r, { 0.0042f, 0.0042f, 0.0016f }, N(3), N(6), boton);
        }
        for (float lado : { -1.0f, 1.0f }) {
            V c; Rot r;
            eje(0.185f, c, r);
            const V pc = torso.punto(0.185f, DELANTE + lado * 0.06f);
            elipsoide(m, suma(pc, r.aplicar({ 0, -0.012f, 0.002f })), r, { 0.0022f, 0.012f, 0.0022f }, N(3), N(6),
                      Color{ 0.88f, 0.86f, 0.80f });
        }
    }

    // ================= BRAZOS =================
    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Brazo& b = p.brazo[l];
        const float flexN = b.codo / Lim::CODO_MAX;
        const float puno = l == 1 ? p.apretar : 0.0f;
        const float dedosFlex = (b.dedo[0][0] + b.dedo[1][0] + b.dedo[2][0] + b.dedo[3][0]) * 0.25f;
        // La mano que cuelga se llena de sangre: las venas se marcan mas.
        const float cuelga = sujetar((s.hombro[l].p.y - s.mano[l].muneca.p.y) / 0.55f, 0.0f, 1.0f);
        const float visible = visibleBase * (0.75f + 0.35f * cuelga);

        // El hombro, bajo la camisa.
        const float kDelt = F.deltoides * (0.85f + 0.15f * F.musculo) * F.hombros;
        elipsoide(m, suma(s.hombro[l].p, s.hombro[l].r.aplicar({ 0, -0.010f, 0 })), s.hombro[l].r,
                  { 0.050f * kDelt, 0.054f * (0.9f + 0.1f * kDelt), 0.050f * kDelt }, N(7), N(12), camisa);

        // Brazo: piel desde el borde de la manga, y la manga encima.
        const EjeRecto ejeBrazo{ s.hombro[l].p, s.hombro[l].r, s.largo.brazo };
        const float kBiceps = F.biceps * F.musculo;
        auto kBrazo = [=](float t) { return F.brazo * (1.0f + (kBiceps - 1.0f) * 0.30f * bulto(t - 0.60f, 0.38f)); };
        auto secBrazo = [=](float t, float a, float& dx, float& dz) {
            seccionBrazo(t, a, sx, flexN, dx, dz);
            const float k = kBrazo(t);
            dx *= k; dz *= k;
        };
        const auto brazo = tubo(ejeBrazo, secBrazo, 0.38f, 1.0f);
        dibujarTubo(m, brazo, N(10), N(22), pielCon(1.3f + l, 0.6f), false, false);
        auto secManga = [=](float t, float a, float& dx, float& dz) {
            seccionBrazo(t, a, sx, flexN, dx, dz);
            // La manga, holgada, con su dobladillo en el borde.
            const float k = kBrazo(t) * (1.0f + 0.008f / 0.045f + 0.003f * std::sin(a * 5.0f + t * 9.0f)
                                         + 0.06f * bulto(t - 0.43f, 0.025f));
            dx *= k; dz *= k;
        };
        auto colorManga = [=](float t, float) { return t > 0.41f ? camisaCostura : camisa; };
        dibujarTubo(m, tubo(ejeBrazo, secManga, 0.0f, 0.45f), N(6), N(22), colorManga, false, false);

        // Codo y olecranon (la punta del codo).
        elipsoide(m, s.codo[l].p, s.codo[l].r, { 0.037f, 0.036f, 0.035f }, N(6), N(12), articulacion);
        elipsoide(m, suma(s.codo[l].p, s.codo[l].r.aplicar({ 0, 0.004f, -0.031f })), s.codo[l].r,
                  { 0.015f, 0.017f, 0.012f }, N(4), N(8), articulacion);

        // Antebrazo, torcido poco a poco: la pronacion es el radio girando
        // sobre el cubito a lo largo de todo el antebrazo.
        const V oAnte = s.codo[l].p;
        const Rot rAnte = s.codo[l].r;
        const float giro = s.prono[l];
        auto ejeAnte = [=](float t, V& c, Rot& r) {
            r = rAnte.por(Rot::enY(giro * t));
            c = suma(oAnte, rAnte.aplicar({ 0, -s.largo.antebrazo * t, 0 }));
        };
        const float kAnte = F.antebrazo * F.brazo;
        auto secAnte = [=](float t, float a, float& dx, float& dz) {
            seccionAntebrazo(t, a, sx, puno, dx, dz);
            const float k = kAnte * (1.0f + (F.musculo - 1.0f) * 0.25f * bulto(t - 0.28f, 0.35f));
            dx *= k; dz *= k;
        };
        const auto ante = tubo(ejeAnte, secAnte);
        dibujarTubo(m, ante, N(18), N(28), pielCon(2.9f + l, 0.8f), false, false);

        // Muneca. Desde aqui, todo es la mano: se escala entera al final.
        const uint32_t inicioMano = (uint32_t)m.v.size();
        const Mano& mano = s.mano[l];
        elipsoide(m, mano.muneca.p, mano.muneca.r, { 0.028f, 0.020f, 0.019f }, N(5), N(12), P);

        // Palma y dorso.
        const float lado = mano.lado;
        static const float NX[4] = { 0.030f, 0.010f, -0.011f, -0.030f };
        const float tension = sujetar(1.0f - 0.6f * dedosFlex, 0.2f, 1.0f);
        const float nudillos = 0.003f + 0.004f * sujetar(dedosFlex / 1.4f, 0.0f, 1.0f);
        const EjeRecto ejePalma{ mano.muneca.p, mano.muneca.r, 0.092f };
        auto secPalma = [=](float t, float a, float& dx, float& dz) {
            const float rx = 0.027f + 0.016f * suave(t * 1.4f);
            const float rz = 0.016f - 0.003f * t;
            const float tenar = 0.009f * bulto(t - 0.35f, 0.32f) * bulto(difAng(a, entre(DELANTE, fuera(lado), 0.5f)), 0.9f);
            const float hipotenar = 0.005f * bulto(t - 0.5f, 0.38f) * bulto(difAng(a, entre(DELANTE, dentro(lado), 0.5f)), 0.8f);
            const float hueco = -0.002f * bulto(t - 0.55f, 0.3f) * bulto(difAng(a, DELANTE), 0.7f);
            float dorso = 0.0f;
            for (int d = 0; d < 4; ++d) {
                const float x = lado * NX[d] * (0.35f + 0.65f * t);
                const float ad = -std::acos(sujetar(x / rx, -0.9f, 0.9f));
                const float da = difAng(a, ad);
                dorso += 0.0016f * tension * bulto(t - 0.55f, 0.42f) * bulto(da, 0.13f);
                dorso += nudillos * bulto(t - 0.98f, 0.09f) * bulto(da, 0.32f);
            }
            elipse(a, rx, rz, tenar + hipotenar + hueco + dorso, dx, dz);
        };
        const auto palmaT = tubo(ejePalma, secPalma);
        // Las lineas de la palma: tres surcos algo mas oscuros (la de la
        // vida rodea la base del pulgar, la de la cabeza la cruza, la del
        // corazon va bajo los dedos).
        const Color surco = tinte(palma, 0.86f, 0.80f, 0.80f);
        auto colorPalma = [=](float t, float a) {
            const Color base = mezclaC(P, palma, suave((std::sin(a) - 0.1f) / 0.5f));
            if (std::sin(a) < 0.35f) return base;
            const float x = std::cos(a) * lado;
            const float vida = bulto(t - (0.30f + 0.35f * x), 0.035f) * (x > -0.2f ? 1.0f : 0.0f);
            const float cabeza = bulto(t - (0.55f - 0.10f * x), 0.028f);
            const float corazon = bulto(t - (0.80f + 0.05f * x), 0.028f) * (x < 0.4f ? 1.0f : 0.0f);
            return mezclaC(base, surco, sujetar(vida + cabeza + corazon, 0.0f, 1.0f));
        };
        dibujarTubo(m, palmaT, N(10), N(24), colorPalma, false, true);

        // Los cinco dedos: tres falanges, cada una con su articulacion, la
        // yema y la una. Un poco mas gruesos que los de una mano real (x1.25):
        // a la distancia a la que se miran en primera persona, con el grosor
        // real se veian como una sola paleta.
        static const float RAD[5] = { 0.0122f, 0.0127f, 0.0119f, 0.0103f, 0.0175f };
        static const float AFINA[5][3] = { { 1.0f, 0.90f, 0.82f }, { 1.0f, 0.90f, 0.82f },
                                           { 1.0f, 0.90f, 0.82f }, { 1.0f, 0.90f, 0.82f },
                                           { 1.0f, 0.81f, 0.74f } };
        for (int d = 0; d < 5; ++d) {
            for (int f = 0; f < 3; ++f) {
                const Junta& j = mano.dedo[d][f];
                const float L = M::DEDO[d][f];
                const float r = RAD[d] * AFINA[d][f] * F.dedos;
                const bool yema = f == 2;
                auto secDedo = [=](float t, float a, float& dx, float& dz) {
                    const float rx = r * (1.0f - 0.08f * t);
                    float extra = 0.06f * r * (bulto(t, 0.18f) + bulto(t - 1.0f, 0.18f));
                    if (yema) extra += 0.15f * r * bulto(t - 0.7f, 0.35f) * bulto(difAng(a, DELANTE), 1.2f);
                    elipse(a, rx, rx * 0.88f, extra, dx, dz);
                };
                dibujarTubo(m, tubo(EjeRecto{ j.p, j.r, L }, secDedo), N(4), N(10), contornoDedo, false, false);
                if (!(d == 4 && f == 0))
                    elipsoide(m, j.p, j.r, { r * 1.03f, r * 1.03f, r * 0.95f }, N(4), N(8),
                              tinte(articulacion, 0.95f, 0.92f, 0.92f));
                if (yema) {
                    const float rf = r * 0.92f;
                    elipsoide(m, mano.dedo[d][3].p, j.r, { rf, rf * 0.9f, rf * 0.83f }, N(4), N(8), P);
                    // La una, en el dorso (-Z).
                    elipsoide(m, suma(j.p, j.r.aplicar({ 0, -L * 0.62f, -rf * 0.80f })), j.r,
                              { rf * 0.82f, L * 0.36f, rf * 0.28f }, N(3), N(8), una);
                    // ⭐ EL BORDE LIBRE: lo que la una crece (Avatar: ~3,5 mm al
                    // mes). Pasado el milimetro asoma por delante de la yema,
                    // blanco, curvado sobre ella.
                    const float libre = (F.unaMm - 1.0f) * 0.001f;
                    if (libre > 0.0002f) {
                        const Color blanco{ 0.95f, 0.92f, 0.86f };
                        elipsoide(m, suma(mano.dedo[d][3].p, j.r.aplicar({ 0, -libre * 0.5f, -rf * 0.55f })), j.r,
                                  { rf * 0.80f, libre * 0.55f + 0.0008f, rf * 0.20f }, N(3), N(8), blanco);
                    }
                }
            }
        }

        // La mano entera, a la medida del avatar.
        if (std::fabs(F.mano - 1.0f) > 1e-3f)
            escalarEnMarco(m, inicioMano, mano.muneca.p, mano.muneca.r, F.mano, F.mano, F.mano);

        // Las venas de este brazo.
        if (q.venas) {
            for (const Vena& v : piel.venas) {
                if (v.region == BRAZO_I + l) dibujarVena(m, brazo, v, P, visible, latido, muestrasVena);
                else if (v.region == ANTE_I + l) dibujarVena(m, ante, v, P, visible, latido, muestrasVena);
                else if (v.region == MANO_I + l && std::fabs(F.mano - 1.0f) <= 1e-3f) dibujarVena(m, palmaT, v, P, visible * 1.15f, latido, muestrasVena);
            }
        }
    }

    // ================= PIERNAS =================
    for (int l = 0; l < 2; ++l) {
        const float sx = l == 0 ? -1.0f : 1.0f;
        const Pierna& g = p.pierna[l];
        const float visible = visibleBase * 0.85f;

        // Muslo: el pantalon corto cubre hasta un palmo sobre la rodilla.
        const EjeRecto ejeMuslo{ s.cadera[l].p, s.cadera[l].r, s.largo.muslo };
        const float kMuslo = F.muslo * (1.0f + (F.musculo - 1.0f) * 0.30f);
        auto secMuslo = [=](float t, float a, float& dx, float& dz) {
            seccionMuslo(t, a, sx, dx, dz);
            dx *= kMuslo; dz *= kMuslo;
        };
        const auto muslo = tubo(ejeMuslo, secMuslo, 0.55f, 1.0f);
        dibujarTubo(m, muslo, N(8), N(24), pielCon(4.1f + l, 0.3f), false, false);
        auto secShort = [=](float t, float a, float& dx, float& dz) {
            seccionMuslo(t, a, sx, dx, dz);
            // Holgado, con el dobladillo vuelto en el borde.
            const float k = kMuslo * (1.0f + 0.009f / 0.07f + 0.004f * std::sin(a * 4.0f + t * 7.0f)
                                      + 0.05f * bulto(t - 0.60f, 0.03f));
            dx *= k; dz *= k;
        };
        auto colorShort = [=](float t, float a) {
            if (t > 0.575f) return pantalonCostura;                          // dobladillo
            if (bulto(difAng(a, fuera(sx)), 0.05f) > 0.4f) return pantalonCostura;   // costura
            // Un bolsillo: el borde de su abertura en diagonal, por delante.
            const float bolsillo = bulto(t - 0.10f - 0.25f * sujetar(difAng(a, fuera(sx)) * sx, 0.0f, 1.0f), 0.012f)
                                   * bulto(difAng(a, entre(fuera(sx), DELANTE, 0.5f)), 0.6f);
            return bolsillo > 0.5f ? pantalonCostura : pantalon;
        };
        dibujarTubo(m, tubo(ejeMuslo, secShort, 0.0f, 0.62f), N(8), N(24), colorShort, false, false);

        // Rodilla y rotula, que se desliza por el femur al doblarla.
        elipsoide(m, s.rodilla[l].p, s.rodilla[l].r, { 0.050f, 0.050f, 0.048f }, N(6), N(14), articulacion);
        const Rot rRotula = s.cadera[l].r.por(Rot::enX(-g.rodilla * 0.5f));
        elipsoide(m, suma(s.rodilla[l].p, rRotula.aplicar({ 0, 0.012f, 0.046f })), rRotula,
                  { 0.024f, 0.028f, 0.011f }, N(4), N(10), articulacion);
        elipsoide(m, suma(s.rodilla[l].p, s.rodilla[l].r.aplicar({ 0, -0.055f, 0.040f })), s.rodilla[l].r,
                  { 0.012f, 0.016f, 0.008f }, N(3), N(8), P);

        // Pierna: tibia, gemelos y Aquiles.
        const float punta = sujetar(-g.tobillo / 0.87f, 0.0f, 1.0f);
        const EjeRecto ejePierna{ s.rodilla[l].p, s.rodilla[l].r, s.largo.pierna };
        const float kGemelo = F.gemelo * F.musculo;
        auto secPierna = [=](float t, float a, float& dx, float& dz) {
            seccionPierna(t, a, sx, punta, dx, dz);
            const float k = 1.0f + (kGemelo - 1.0f) * 0.32f * bulto(t - 0.30f, 0.30f) * bulto(difAng(a, DETRAS), 1.4f);
            dx *= k; dz *= k;
        };
        const auto pierna = tubo(ejePierna, secPierna);
        dibujarTubo(m, pierna, N(18), N(26), pielCon(5.7f + l, 0.2f), false, false);

        // Tobillo y maleolos: el de dentro mas alto que el de fuera.
        const Pie& pie = s.pie[l];
        const Rot& rP = s.rodilla[l].r;
        elipsoide(m, pie.tobillo.p, rP, { 0.030f, 0.032f, 0.030f }, N(5), N(12), P);
        elipsoide(m, suma(pie.tobillo.p, rP.aplicar({ -sx * 0.026f, 0.006f, 0.0f })), rP,
                  { 0.010f, 0.013f, 0.012f }, N(4), N(8), articulacion);
        elipsoide(m, suma(pie.tobillo.p, rP.aplicar({ sx * 0.026f, -0.008f, -0.006f })), rP,
                  { 0.010f, 0.012f, 0.011f }, N(4), N(8), articulacion);

        // Pie descalzo: talon, empeine, arco por dentro y planta plana.
        const uint32_t inicioPie = (uint32_t)m.v.size();
        const Rot& rPie = pie.tobillo.r;
        const float ad = -sx;          // hacia dentro, en X
        const V talonC = suma(pie.tobillo.p, rPie.aplicar({ 0, -0.036f, -0.055f }));
        const V bolaC = suma(pie.tobillo.p, rPie.aplicar({ ad * 0.004f, -0.054f, 0.145f }));
        auto ejePie = [=](float t, V& c, Rot& r) {
            c = mezclar(talonC, bolaC, t);
            r = rPie.por(Rot::enX(PI * 0.5f));
        };
        const float extDedos = sujetar(g.dedos, 0.0f, 1.0f);
        auto secPie = [=](float t, float a, float& dx, float& dz) {
            const float rx = 0.030f + 0.015f * suave(t * 1.2f);
            const float arriba = 0.030f + 0.016f * bulto(t - 0.3f, 0.35f) - 0.014f * suave((t - 0.3f) / 0.7f);
            const float sn = std::sin(a), cs = std::cos(a);
            float abajo = 0.034f - 0.018f * t;
            const float porDentro = sujetar(cs * ad, 0.0f, 1.0f);
            abajo -= 0.012f * F.arco * bulto(t - 0.45f, 0.32f) * porDentro;  // el arco
            // La almohadilla de la bola del pie, bajo los metatarsianos.
            abajo += 0.004f * bulto(t - 0.80f, 0.16f);
            float tendones = 0.0f;
            for (float o : { -0.45f, -0.15f, 0.15f, 0.45f })
                tendones += bulto(difAng(a, DELANTE + o), 0.12f);
            tendones *= 0.0012f * (1.0f + extDedos) * bulto(t - 0.62f, 0.32f);
            dx = (rx + tendones * 0.3f) * cs;
            dz = sn >= 0 ? (arriba + tendones) * sn : -abajo * std::pow(-sn, 0.5f);  // planta plana
        };
        const auto pieT = tubo(ejePie, secPie);
        auto colorPie = [=](float, float a) { return mezclaC(P, palma, suave(-std::sin(a) * 2.0f - 0.6f)); };
        dibujarTubo(m, pieT, N(14), N(22), colorPie, false, true);
        elipsoide(m, suma(talonC, rPie.aplicar({ 0, 0.0f, -0.008f })), rPie,
                  { 0.031f, 0.034f, 0.034f }, N(6), N(12), P);

        // Los cinco dedos del pie: dos tramos, articulacion, punta y una.
        for (int d = 0; d < 5; ++d) {
            // La yema de cada dedo, abajo: es lo que apoya.
            {
                const float rp0 = radioDedoPie(d);
                const Junta& pt = pie.dedo[d][2];
                elipsoide(m, suma(pt.p, pt.r.aplicar({ 0, -rp0 * 0.45f, -rp0 * 0.25f })), pt.r,
                          { rp0 * 0.80f, rp0 * 0.45f, rp0 * 0.85f }, N(3), N(6), palma);
            }
            const float r = radioDedoPie(d);
            const float L = M::DEDO_PIE[d];
            for (int f = 0; f < 2; ++f) {
                const Junta& j = pie.dedo[d][f];
                const float Lf = L * (f == 0 ? 0.55f : 0.45f);
                const float rf = r * (f == 0 ? 1.0f : 0.93f);
                auto secDedo = [=](float t, float a, float& dx, float& dz) {
                    elipse(a, rf * (1.0f - 0.05f * t), rf * 0.85f, 0.05f * rf * bulto(t, 0.2f), dx, dz);
                };
                dibujarTubo(m, tubo(EjeDelante{ j.p, j.r, Lf }, secDedo), N(3), N(8), contornoDedo, false, false);
                elipsoide(m, j.p, j.r, { rf, rf * 0.86f, rf }, N(3), N(8), f == 0 ? P : articulacion);
            }
            const Junta& punta2 = pie.dedo[d][2];
            const float rp = r * 0.9f;
            elipsoide(m, punta2.p, punta2.r, { rp, rp * 0.82f, rp * 0.9f }, N(3), N(8), P);
            elipsoide(m, suma(pie.dedo[d][1].p, punta2.r.aplicar({ 0, rp * 0.72f, L * 0.45f * 0.55f })),
                      punta2.r, { rp * 0.75f, rp * 0.24f, L * 0.45f * 0.38f }, N(3), N(8), una);
        }

        // El pie entero, a la medida del avatar: largo (Z del pie) y ancho
        // (X), sin tocar el alto, que es lo que lo apoya en el suelo.
        if (std::fabs(F.pie - 1.0f) > 1e-3f || std::fabs(F.pieAncho - 1.0f) > 1e-3f)
            escalarEnMarco(m, inicioPie, pie.tobillo.p, rPie, F.pie * F.pieAncho, 1.0f, F.pie);

        if (q.venas) {
            for (const Vena& v : piel.venas) {
                if (v.region == MUSLO_I + l) dibujarVena(m, muslo, v, P, visible, latido, muestrasVena);
                else if (v.region == PIERNA_I + l) dibujarVena(m, pierna, v, P, visible, latido, muestrasVena);
                else if (v.region == PIE_I + l && std::fabs(F.pie - 1.0f) <= 1e-3f && std::fabs(F.pieAncho - 1.0f) <= 1e-3f) dibujarVena(m, pieT, v, P, visible, latido, muestrasVena);
            }
        }
    }
}

} // namespace ModeloJugador
