#pragma once

#include <cmath>

// ============================================================================
// LOS COLORES DEL CIELO
// ============================================================================
// Sin OpenGL: el motor los pinta, los tests los comprueban.
//
//   cenit      lo mas alto, mas saturado
//   horizonte  mas claro: la luz cruza mas aire. Es tambien el color de la
//              niebla, y por eso el terreno lejano se funde con el cielo.
//   resplandor el tono del lado del sol cuando esta bajo
//
// `luz` es luzSolar(): 0 de noche, 1 en pleno dia.
// ============================================================================

namespace Atmosfera {

struct Cielo {
    float cenit[3];
    float horizonte[3];
    float resplandor[3];   // suma, solo en la parte baja y mirando al sol
    float contraluz[3];    // suma, en la parte baja y DE ESPALDAS al sol
    float luzSol[3];       // color de la luz que cae sobre el terreno
    float halo[3];         // suma: el resplandor alrededor del sol (dispersion de Mie)
    float brilloNocturno[3]; // suma: la franja tenue sobre el horizonte de noche
};

inline float mezcla(float a, float b, float t) { return a + (b - a) * t; }

inline Cielo cieloPara(float luz) {
    if (luz < 0.0f) luz = 0.0f;
    if (luz > 1.0f) luz = 1.0f;

    // Un azul mas hondo en lo alto: mirando arriba se atraviesa poco aire y
    // el azul de la dispersion de Rayleigh queda puro, sin el blanco de abajo.
    static const float DIA_CENIT[3]  = { 0.20f, 0.43f, 0.84f };
    static const float DIA_HOR[3]    = { 0.72f, 0.85f, 0.95f };
    static const float NOCHE_CENIT[3]= { 0.008f, 0.012f, 0.045f };
    // Algo mas azul que antes: de noche el horizonte no es negro, es el
    // azul de la luz de luna dispersada. Sigue por debajo de 0.15.
    static const float NOCHE_HOR[3]  = { 0.035f, 0.055f, 0.125f };
    static const float OCASO_HOR[3]  = { 0.93f, 0.55f, 0.32f };

    // Maximo a media transicion, nulo en pleno dia y de noche cerrada.
    const float ocaso = 1.0f - std::fabs(luz - 0.5f) * 2.0f;

    Cielo c;
    for (int i = 0; i < 3; ++i) {
        c.cenit[i] = mezcla(NOCHE_CENIT[i], DIA_CENIT[i], luz);
        c.horizonte[i] = mezcla(mezcla(NOCHE_HOR[i], DIA_HOR[i], luz),
                                OCASO_HOR[i], ocaso * 0.75f);
    }
    // En el ocaso el cenit se vuelve violeta: el azul de arriba se mezcla con
    // el rojo que llega rasante.
    c.cenit[0] += 0.07f * ocaso;
    c.cenit[2] += 0.04f * ocaso;

    // EL HALO DEL SOL. El aire desvia hacia delante parte de la luz del sol
    // (dispersion de Mie, la de las particulas grandes): alrededor del disco
    // el cielo es mas claro. Blanco calido de dia, naranja al atardecer.
    for (int i = 0; i < 3; ++i) c.halo[i] = 0.0f;
    static const float HALO_DIA[3] = { 1.00f, 0.95f, 0.82f };
    static const float HALO_OCASO[3] = { 1.00f, 0.55f, 0.22f };
    for (int i = 0; i < 3; ++i)
        c.halo[i] = HALO_DIA[i] * 0.32f * luz * (1.0f - ocaso) + HALO_OCASO[i] * 0.55f * ocaso;

    // EL BRILLO NOCTURNO DEL AIRE: de noche cerrada, una franja muy tenue,
    // entre verde y azul, justo por encima del horizonte.
    const float noche = 1.0f - luz / 0.3f > 0.0f ? 1.0f - luz / 0.3f : 0.0f;
    c.brilloNocturno[0] = 0.006f * noche;
    c.brilloNocturno[1] = 0.016f * noche;
    c.brilloNocturno[2] = 0.020f * noche;
    c.resplandor[0] = 0.55f * ocaso;
    c.resplandor[1] = 0.22f * ocaso;
    c.resplandor[2] = 0.02f * ocaso;

    // EL CINTURON DE VENUS: en el ocaso, el cielo bajo del lado CONTRARIO al
    // sol se tine de rosa y violeta. Es la sombra de la Tierra proyectada
    // sobre la atmosfera, con la franja iluminada por el sol rojo encima.
    // Sin esto, mirar de espaldas al atardecer daba un horizonte naranja
    // plano, igual en todas direcciones.
    c.contraluz[0] = 0.16f * ocaso;
    c.contraluz[1] = 0.05f * ocaso;
    c.contraluz[2] = 0.20f * ocaso;

    // EL COLOR DE LA LUZ DEL SOL. Blanca a mediodia (el terreno se ve con
    // sus colores de siempre), dorada cuando el sol esta bajo y azulada de
    // noche, que es como el ojo ve la luz de luna. Multiplica la luz del
    // cielo del terreno, no su brillo: de dia y a pleno sol da (1, 1, 1).
    //
    // Los dos tonos van compensados para que su luminancia ronde 1 (0.94 y
    // 0.92): cambian el COLOR de la luz, no cuanta hay. La noche ya la
    // oscurece LuzCielo.h; oscurecerla aqui otra vez la dejaba negra.
    static const float LUZ_LUNA[3]  = { 0.85f, 0.95f, 1.15f };
    static const float LUZ_OCASO[3] = { 1.08f, 0.88f, 0.66f };
    for (int i = 0; i < 3; ++i)
        c.luzSol[i] = mezcla(mezcla(LUZ_LUNA[i], 1.0f, luz),
                             LUZ_OCASO[i], ocaso * 0.8f);
    return c;
}

// El color en un punto del cielo. `senAlt` es la altura (0 horizonte, 1 cenit) y
// `haciaSol` el coseno del angulo con el sol.
inline void colorEn(const Cielo& c, float senAlt, float haciaSol, float out[3]) {
    float t = senAlt <= 0.0f ? 0.0f : senAlt;
    t = std::sqrt(t);                     // la franja clara se queda abajo
    if (t > 1.0f) t = 1.0f;

    float s = haciaSol > 0.0f ? haciaSol : 0.0f;
    s = s * s * s;
    float e = haciaSol < 0.0f ? -haciaSol : 0.0f;
    e = e * e;
    const float bajo = 1.0f - t;

    // Lo que solo existe POR ENCIMA del horizonte: el halo del sol y el brillo
    // nocturno. En el horizonte mismo valen 0, que es lo que mantiene igual el
    // color de la niebla (el shader del terreno repite esa cuenta).
    const float sobre = senAlt > 0.0f ? (senAlt * 12.0f < 1.0f ? senAlt * 12.0f : 1.0f) : 0.0f;
    const float m = haciaSol > 0.0f ? haciaSol : 0.0f;
    const float m2 = m * m, m4 = m2 * m2, m8 = m4 * m4, m32 = m8 * m8 * m8 * m8;
    const float halo = (0.12f * m4 + 0.45f * m32) * sobre;
    float banda = (senAlt - 0.06f) / 0.14f;
    banda = banda < -1.0f || banda > 1.0f ? 0.0f : (1.0f - banda * banda);
    banda *= sobre;

    for (int i = 0; i < 3; ++i) {
        float v = mezcla(c.horizonte[i], c.cenit[i], t) +
                  (c.resplandor[i] * s + c.contraluz[i] * e) * bajo +
                  c.halo[i] * halo + c.brilloNocturno[i] * banda;
        out[i] = v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
    }
}

// EL COLOR DE LA NIEBLA EN UNA DIRECCION: el del cielo en el horizonte de esa
// misma direccion. Es lo que hace que el terreno lejano se FUNDA con el cielo
// que tiene detras -- tambien al atardecer, mirando al sol, donde una niebla
// de un solo color dejaba una raya entre el monte gris y el cielo naranja.
//
// El shader del terreno repite esta cuenta (TerrenoGL.h); el test comprueba
// que la de aqui coincide con el cielo.
inline void colorNiebla(const Cielo& c, float haciaSol, float out[3]) {
    colorEn(c, 0.0f, haciaSol, out);
}

// Las estrellas salen al anochecer, cuando ya no queda luz que las tape.
inline float brilloEstrellas(float luz) {
    const float b = 1.0f - luz / 0.35f;
    return b < 0.0f ? 0.0f : (b > 1.0f ? 1.0f : b);
}

// La estrella numero `i`: direccion unitaria y brillo propio, siempre los mismos.
inline void estrella(int i, float out[4]) {
    auto azar = [](unsigned v) {
        v ^= v >> 16; v *= 0x7feb352du;
        v ^= v >> 15; v *= 0x846ca68bu;
        v ^= v >> 16;
        return (float)(v & 0xFFFFu) / 65535.0f;
    };
    const unsigned n = (unsigned)i * 3u + 1u;
    const float y = azar(n) * 2.0f - 1.0f;
    const float ang = azar(n + 1u) * 6.2831853f;
    const float r = std::sqrt(1.0f - y * y);
    out[0] = r * std::cos(ang);
    out[1] = y;
    out[2] = r * std::sin(ang);
    const float b = azar(n + 2u);
    out[3] = 0.35f + 0.65f * b * b;       // muchas tenues y pocas brillantes
}

// ============================================================================
// LAS FASES DE LA LUNA
// ============================================================================
// PEDIDO: "las fases lunares igual que el mundo real por noches, y que a veces
// se pueda ver la luna de dia pero mas transparente".
//
// Como en la realidad, la fase sale de cuanto se ha separado la luna del sol
// en el cielo (la ELONGACION): en luna nueva van juntos (y la luna no se ve),
// en cuarto creciente la luna va un cuarto de vuelta por detras del sol, en
// llena justo enfrente. Esa misma separacion es la que la pone en el cielo:
//
//   NUEVA     sale y se pone con el sol: invisible.
//   CRECIENTE se ve al atardecer, hacia el oeste, poco despues del sol.
//   CUARTO C. sale a mediodia: se ve DE DIA por la tarde, y de noche.
//   LLENA     sale al ponerse el sol y esta toda la noche.
//   CUARTO M. sale a medianoche: se ve de madrugada y DE DIA por la manana.
//
// Un ciclo dura lo mismo que el real, 29.53 noches. La fase que se dibuja
// cambia a MEDIODIA, nunca a media noche. Un mundo nuevo empieza con la luna
// gibosa creciente, a pocas noches de la llena.
constexpr double MES_LUNAR = 29.530588;
constexpr double EDAD_INICIAL = 10.0;     // dias de luna al crear el mundo
constexpr int NUM_FASES = 8;

// Los dias de luna (0 nueva .. 29.53), continuos. `dias` = dias de mundo.
inline double edadLunar(double dias) {
    double e = std::fmod(dias + EDAD_INICIAL, MES_LUNAR);
    return e < 0.0 ? e + MES_LUNAR : e;
}
// La separacion entre la luna y el sol, en radianes (0 nueva, PI llena).
inline double elongacion(double dias) {
    return edadLunar(dias) / MES_LUNAR * 6.283185307179586;
}
// La fase que se ve ESTA noche: 0 nueva, 1 creciente, 2 cuarto creciente,
// 3 gibosa creciente, 4 llena, 5 gibosa menguante, 6 cuarto menguante,
// 7 menguante. Cambia a mediodia.
inline int faseLunar(double dias) {
    const double noche = std::floor(dias + 0.5);
    const double e = edadLunar(noche) / MES_LUNAR * NUM_FASES;
    const int f = (int)std::floor(e + 0.5);
    return f % NUM_FASES;
}
// Cuanto disco esta iluminado (0 nueva, 1 llena).
inline float iluminada(double dias) {
    return (float)((1.0 - std::cos(elongacion(dias))) * 0.5);
}
// De dia la luna se ve, pero palida: el cielo claro la lava. `luz` es
// luzSolar() (0 noche, 1 dia).
inline float opacidadLuna(float luz) {
    luz = luz < 0.0f ? 0.0f : (luz > 1.0f ? 1.0f : luz);
    return 1.0f - 0.58f * luz;
}

} // namespace Atmosfera
