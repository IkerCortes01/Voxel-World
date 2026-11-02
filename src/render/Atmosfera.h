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
};

inline float mezcla(float a, float b, float t) { return a + (b - a) * t; }

inline Cielo cieloPara(float luz) {
    if (luz < 0.0f) luz = 0.0f;
    if (luz > 1.0f) luz = 1.0f;

    static const float DIA_CENIT[3]  = { 0.27f, 0.51f, 0.86f };
    static const float DIA_HOR[3]    = { 0.72f, 0.85f, 0.95f };
    static const float NOCHE_CENIT[3]= { 0.008f, 0.012f, 0.045f };
    static const float NOCHE_HOR[3]  = { 0.035f, 0.05f, 0.11f };
    static const float OCASO_HOR[3]  = { 0.93f, 0.55f, 0.32f };

    // Maximo a media transicion, nulo en pleno dia y de noche cerrada.
    const float ocaso = 1.0f - std::fabs(luz - 0.5f) * 2.0f;

    Cielo c;
    for (int i = 0; i < 3; ++i) {
        c.cenit[i] = mezcla(NOCHE_CENIT[i], DIA_CENIT[i], luz);
        c.horizonte[i] = mezcla(mezcla(NOCHE_HOR[i], DIA_HOR[i], luz),
                                OCASO_HOR[i], ocaso * 0.75f);
    }
    c.resplandor[0] = 0.55f * ocaso;
    c.resplandor[1] = 0.22f * ocaso;
    c.resplandor[2] = 0.02f * ocaso;
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
    const float bajo = 1.0f - t;

    for (int i = 0; i < 3; ++i) {
        const float v = mezcla(c.horizonte[i], c.cenit[i], t) +
                        c.resplandor[i] * s * bajo;
        out[i] = v > 1.0f ? 1.0f : v;
    }
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

} // namespace Atmosfera
