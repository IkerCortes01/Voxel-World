#pragma once
// ============================================================================
// EL AMBIENTE: HOJAS AL VIENTO, PAJAROS, GRILLOS Y LAS RACHAS DEL DIA
// ============================================================================
// PEDIDO: "los sonidos de viento y ambiente super comunes y de diferentes
// magnitudes o en climas, y si estas en bosques o zonas con muchos arboles o
// con un solo arbol, se escuche el viento con hojas".
//
// Como el viento y la lluvia, todo se SINTETIZA en el hilo de audio; no hay
// archivos que se repitan.
//
//   HOJAS     el susurro de las copas: ruido agudo con chasquidos finos, que
//             sube y baja con las rafagas. Cuantas hojas hay alrededor lo
//             decide el juego: un arbol solo susurra bajito, un bosque llena
//             el aire. Sin viento, casi nada.
//   PAJAROS   de dia, cantos cortos de 2 a 7 notas que barren el tono, cada
//             pocos segundos, de varias "especies" (agudos, graves, trinos).
//             Mas en el bosque; con lluvia se callan.
//   GRILLOS   de noche, tres grillos con su tono y su ritmo: trenes de
//             pulsos muy agudos. Con frio o lluvia, callan.
//
// LAS RACHAS DEL DIA. El viento no sopla igual todo el dia: hay ratos de
// calma, de brisa, de viento y de ventarron, de 1,5 a 4 minutos cada uno, que
// se funden entre si. Sale del reloj del mundo, sin estado (como Clima.h).
// ============================================================================

#include <cmath>
#include <cstdint>

namespace Audio {

inline uint32_t hashAmbiente(uint32_t h) {
    h ^= h >> 16; h *= 0x7feb352du;
    h ^= h >> 15; h *= 0x846ca68bu;
    h ^= h >> 16;
    return h;
}

// Cuanto sopla en este rato del dia, de 0.1 (calma) a 1 (ventarron).
inline float rachaDelDia(double t) {
    constexpr double TRAMO = 150.0;
    static const float NIVELES[5] = { 0.12f, 0.30f, 0.45f, 0.68f, 0.95f };
    static const float PESOS[5]   = { 0.20f, 0.30f, 0.25f, 0.17f, 0.08f };
    auto nivel = [&](int64_t k) {
        const float r = (float)(hashAmbiente((uint32_t)k * 0x9E3779B1u + 0x51ED27u) >> 8) / 16777216.0f;
        float acc = 0.0f;
        for (int i = 0; i < 5; ++i) { acc += PESOS[i]; if (r < acc) return NIVELES[i]; }
        return NIVELES[4];
    };
    const double x = t / TRAMO;
    const int64_t k = (int64_t)std::floor(x);
    float f = (float)(x - (double)k);
    // Se queda en su nivel casi todo el tramo y cambia en el ultimo 25 %.
    f = f < 0.75f ? 0.0f : (f - 0.75f) / 0.25f;
    f = f * f * (3.0f - 2.0f * f);
    return nivel(k) + (nivel(k + 1) - nivel(k)) * f;
}

struct GeneradorAmbiente {
    uint32_t azar = 0x6D2B79F5u;
    float hojas = 0.0f, pajaros = 0.0f, grillos = 0.0f;   // suavizados
    double fase = 0.0;

    // Hojas
    float h1 = 0.0f, h2 = 0.0f, rafaga = 0.5f, crujido = 0.0f;
    // Pajaros: el canto en curso (ver cantar)
    float tonoIni = 3000.0f, tonoFin = 3500.0f, faseTono = 0.0f, vibrato = 0.0f;
    int especie = 0;
    // Grillos
    struct Grillo { float tono, periodo, t, fase; int pulsos; };
    Grillo grillo[3] = { {4300.0f, 0.70f, 0.0f, 0.0f, 3}, {4650.0f, 0.95f, 0.3f, 0.0f, 4},
                         {4050.0f, 1.20f, 0.6f, 0.0f, 2} };

    float blanco() {
        azar ^= azar << 13; azar ^= azar >> 17; azar ^= azar << 5;
        return (float)(azar & 0xFFFFFFu) / 8388607.5f - 1.0f;
    }
    float uno() { return blanco() * 0.5f + 0.5f; }
    static float banda(float x, float f, float q, float& l, float& b) {
        const float h = x - l - q * b;
        b += f * h;
        l += f * b;
        return b;
    }

    // `objHojas` 0..1 (cuantas hojas y cuanto viento), `objPajaros` y
    // `objGrillos` 0..1. Se suaviza dentro.
    void llenar(float* salida, int n, int frecuencia, float objHojas, float objPajaros, float objGrillos) {
        const float dt = 1.0f / (float)frecuencia;
        const float suaviza = 1.0f - std::exp(-dt / 2.0f);
        const float fHojas = 2.0f * std::sin(3.14159265f * 3800.0f * dt);
        for (int i = 0; i < n; ++i) {
            hojas += (objHojas - hojas) * suaviza;
            pajaros += (objPajaros - pajaros) * suaviza;
            grillos += (objGrillos - grillos) * suaviza;
            fase += dt;
            float v = 0.0f;

            // ---- HOJAS ----
            if (hojas > 1e-4f) {
                // Rafagas propias, lentas: las copas no se mueven a la vez que
                // el rumor grave del viento, van un poco a su aire.
                rafaga += ((0.55f + 0.45f * (float)std::sin(fase * 0.9) * (float)std::sin(fase * 0.23 + 1.1)) - rafaga) * dt * 2.0f;
                const float r = rafaga < 0.05f ? 0.05f : rafaga;
                const float w = blanco();
                const float sus = banda(w, fHojas, 0.7f, h1, h2);
                // Crujidos: hojitas que chocan, mas con rafaga fuerte.
                if (crujido <= 0.0f && uno() < 0.004f * r * hojas) crujido = 1.0f;
                float cru = 0.0f;
                if (crujido > 0.0f) { cru = blanco() * crujido; crujido -= dt * 220.0f; }
                v += (sus * 1700.0f + cru * 1300.0f) * hojas * r;
            }

            salida[i] += v;
        }
        // Los pajaros y los grillos se sintetizan aparte (ver abajo), por
        // claridad: cada uno con su propio reloj.
        cantar(salida, n, frecuencia);
        cricrear(salida, n, frecuencia);
    }

    // ---- PAJAROS: un canto es una serie de notas que barren el tono ----
    float notaT = -1.0f;       // tiempo dentro de la nota, <0 entre notas
    float notaLargo = 0.0f, hueco = 0.0f;
    int quedan = 0;
    float espera = 1.5f;
    void cantar(float* salida, int n, int frecuencia) {
        const float dt = 1.0f / (float)frecuencia;
        for (int i = 0; i < n; ++i) {
            if (pajaros <= 0.01f) { espera = 0.5f + 2.0f * uno(); quedan = 0; notaT = -1.0f; continue; }
            if (quedan <= 0) {
                espera -= dt;
                if (espera > 0.0f) continue;
                especie = (int)(uno() * 3.0f) % 3;
                quedan = 2 + (int)(uno() * 6.0f);
                notaT = -0.001f;
                hueco = 0.0f;
            }
            if (notaT < 0.0f) {
                hueco -= dt;
                if (hueco > 0.0f) continue;
                // Empieza una nota.
                const float base = especie == 0 ? 3600.0f : (especie == 1 ? 2300.0f : 4400.0f);
                tonoIni = base * (0.85f + 0.3f * uno());
                tonoFin = especie == 2 ? tonoIni * 1.02f : tonoIni * (0.65f + 0.75f * uno());
                notaLargo = especie == 2 ? 0.035f : 0.06f + 0.11f * uno();
                notaT = 0.0f;
                vibrato = 30.0f + 40.0f * uno();
            }
            notaT += dt;
            const float u = notaT / notaLargo;
            if (u >= 1.0f) {
                notaT = -1.0f;
                --quedan;
                hueco = especie == 2 ? 0.015f : 0.04f + 0.09f * uno();
                if (quedan <= 0) espera = (1.2f + 6.0f * uno()) / (0.35f + pajaros);
                continue;
            }
            const float f = tonoIni + (tonoFin - tonoIni) * u + 120.0f * std::sin(notaT * vibrato * 6.2831853f);
            faseTono += 6.2831853f * f * dt;
            if (faseTono > 6.2831853f) faseTono -= 6.2831853f;
            const float env = std::sin(3.14159265f * u);
            salida[i] += std::sin(faseTono) * env * env * 2600.0f * pajaros;
        }
    }

    // ---- GRILLOS: trenes de pulsos agudos ----
    void cricrear(float* salida, int n, int frecuencia) {
        if (grillos <= 0.01f) return;
        const float dt = 1.0f / (float)frecuencia;
        for (Grillo& g : grillo) {
            for (int i = 0; i < n; ++i) {
                g.t += dt;
                if (g.t >= g.periodo) g.t -= g.periodo;
                // Cada pulso dura 18 ms y van cada 33 ms, `pulsos` seguidos.
                const float enTren = g.t / 0.033f;
                const int pulso = (int)enTren;
                if (pulso >= g.pulsos) continue;
                const float dentro = (enTren - (float)pulso) * 0.033f;
                if (dentro > 0.018f) continue;
                const float env = std::sin(3.14159265f * dentro / 0.018f);
                g.fase += 6.2831853f * g.tono * dt;
                if (g.fase > 6.2831853f) g.fase -= 6.2831853f;
                salida[i] += std::sin(g.fase) * env * 900.0f * grillos;
            }
        }
    }
};

} // namespace Audio
