#pragma once
// ============================================================================
// EL VIENTO DE FONDO
// ============================================================================
// PEDIDO: "anade sonidos de viento en el fondo".
//
// No es un archivo que se repite (se oiria el bucle): se SINTETIZA, muestra a
// muestra, en el hilo de audio. Un viento de verdad es aire que pasa por
// cosas, y suena como ruido con color:
//
//   RUMOR     ruido "marron" (ruido blanco integrado): grave y continuo.
//   SOPLO     el mismo ruido por un filtro pasa-banda cuya frecuencia sube y
//             baja con las rafagas: cuando arrecia, el aire "silba" mas agudo.
//   RAFAGAS   dos osciladores lentos y desacompasados (0.07 y 0.19 Hz) mas
//             un poco de azar: nunca se repite igual.
//   SILBIDO   solo con mucho viento: una banda estrecha y aguda, muy baja.
//
// La intensidad (0..1) la pone el juego segun donde esta el jugador (alto y a
// cielo abierto, mas; en una cueva, casi nada) y aqui se suaviza, para que
// entrar en una cueva apague el viento en un par de segundos y no de golpe.
//
// Sin dependencias: se prueba en los tests sin abrir el audio.
// ============================================================================

#include <cmath>
#include <cstdint>

namespace Audio {

struct GeneradorViento {
    uint32_t azar = 0x1234567u;
    float marron = 0.0f;           // ruido integrado
    float bp1 = 0.0f, bp2 = 0.0f;  // estado del pasa-banda (soplo)
    float sb1 = 0.0f, sb2 = 0.0f;  // estado del pasa-banda (silbido)
    double fase = 0.0;             // segundos, para las rafagas
    float intensidad = 0.0f;       // la suavizada
    float rafagaAzar = 0.0f;

    float blanco() {
        azar ^= azar << 13; azar ^= azar >> 17; azar ^= azar << 5;
        return (float)(azar & 0xFFFFFFu) / 8388607.5f - 1.0f;
    }

    // Un pasa-banda de estado variable (Chamberlin): barato y estable para
    // frecuencias bajas respecto a la de muestreo.
    static float banda(float x, float f, float q, float& lp, float& bp) {
        const float hp = x - lp - q * bp;
        bp += f * hp;
        lp += f * bp;
        return bp;
    }

    // Suma `n` muestras de viento a `salida` (escala de int16). `objetivo` es
    // la intensidad que pide el juego, 0..1.
    void llenar(float* salida, int n, int frecuencia, float objetivo) {
        if (objetivo < 0.0f) objetivo = 0.0f;
        if (objetivo > 1.0f) objetivo = 1.0f;
        const float dt = 1.0f / (float)frecuencia;
        const float suaviza = 1.0f - std::exp(-dt / 1.2f);     // ~1.2 s
        for (int i = 0; i < n; ++i) {
            intensidad += (objetivo - intensidad) * suaviza;
            fase += dt;
            if (intensidad < 1e-4f) continue;
            // Las rafagas: lentas, desacompasadas, con un poco de azar.
            rafagaAzar += (blanco() * 0.5f - rafagaAzar) * (dt * 0.6f);
            const float rafaga = 0.55f + 0.25f * (float)std::sin(fase * 2.0 * 3.14159265 * 0.07) +
                                 0.15f * (float)std::sin(fase * 2.0 * 3.14159265 * 0.19 + 1.3) +
                                 0.35f * rafagaAzar;
            const float fuerza = intensidad * (rafaga < 0.05f ? 0.05f : rafaga);

            const float w = blanco();
            marron = marron * 0.997f + w * 0.03f;            // rumor grave
            // El soplo: centro entre ~250 y ~900 Hz segun la rafaga.
            const float centro = 250.0f + 650.0f * (fuerza > 1.0f ? 1.0f : fuerza);
            const float f = 2.0f * (float)std::sin(3.14159265f * centro * dt);
            const float soplo = banda(w, f, 1.2f, bp1, bp2);
            // El silbido: estrecho y agudo, solo con viento fuerte.
            const float fs = 2.0f * (float)std::sin(3.14159265f * (1400.0f + 500.0f * fuerza) * dt);
            const float silbido = banda(w, fs, 0.08f, sb1, sb2);
            const float silba = fuerza > 0.75f ? (fuerza - 0.75f) * 4.0f : 0.0f;

            const float v = marron * 4200.0f + soplo * 1400.0f + silbido * 120.0f * silba;
            salida[i] += v * fuerza;
        }
    }
};

// Cuanto viento se oye en un sitio. `altura` en bloques, `cielo` 0..1 (la luz
// del cielo que llega a la cabeza: 1 a la intemperie, 0 bajo techo).
inline float intensidadViento(float altura, float cielo) {
    float alto = (altura - 60.0f) / 80.0f;          // de 60 a 140 bloques, crece
    alto = alto < 0.0f ? 0.0f : (alto > 1.0f ? 1.0f : alto);
    float abierto = cielo < 0.0f ? 0.0f : (cielo > 1.0f ? 1.0f : cielo);
    abierto = abierto * abierto;                    // bajo techo cae deprisa
    return (0.08f + 0.35f * abierto + 0.45f * abierto * alto);
}

} // namespace Audio
