#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================================
// ⭐ EL CLIC DE LOS BOTONES
// ============================================================================
// PEDIDO: "quiero que ahora los botones tengan un mini sonido al ser
// cliqueados".
//
// No hay archivo: son 55 ms sintetizados una vez al arrancar. Dos partes, como
// una tecla de madera:
//   - el GOLPE: un chasquido de ruido que se apaga en 4 ms,
//   - el CUERPO: un tono que cae de 1250 a 760 Hz y se apaga en ~18 ms.
// Empieza y acaba en cero para no meter un "pop" en la mezcla.
//
// Sin motor ni Windows: lo prueba tests/test_clic_boton.cpp.
// ============================================================================

namespace Audio {
namespace ClicBoton {

constexpr float DURACION = 0.055f;      // segundos
constexpr float PICO = 11000.0f;        // de 32767: es un sonido pequeno
constexpr float VOLUMEN = 0.55f;        // con el que se manda al mezclador
// Dos clics mas juntos que esto suenan como uno (doble clic, arrastres).
constexpr double SEPARACION = 0.045;
// Cuanto vale "el raton estaba sobre un boton": lo marca el dibujo de cada
// frame, y el clic llega entre dos frames.
constexpr double VIGENCIA = 0.20;

inline std::vector<int16_t> generar(int sampleRate) {
    const int n = (int)(DURACION * (float)sampleRate);
    std::vector<int16_t> out((size_t)(n > 0 ? n : 0));
    unsigned semilla = 0x9E3779B9u;
    double fase = 0.0;
    float mayor = 0.0f;
    std::vector<float> s(out.size());
    for (int i = 0; i < n; ++i) {
        const float t = (float)i / (float)sampleRate;
        const float u = (float)i / (float)(n - 1);
        semilla = semilla * 1664525u + 1013904223u;
        const float ruido = (float)((semilla >> 9) & 0xFFFFu) / 32767.5f - 1.0f;
        const float hz = 760.0f + 490.0f * std::exp(-t / 0.010f);
        fase += 6.283185307179586 * (double)hz / (double)sampleRate;
        const float cuerpo = (float)std::sin(fase) * std::exp(-t / 0.018f);
        const float golpe = ruido * std::exp(-t / 0.004f) * 0.45f;
        // Entra en 1 ms y sale suave: sin escalon al principio ni al final.
        const float entra = t < 0.001f ? t / 0.001f : 1.0f;
        const float sale = u > 0.8f ? (1.0f - u) / 0.2f : 1.0f;
        s[(size_t)i] = (cuerpo + golpe) * entra * sale;
        const float a = std::fabs(s[(size_t)i]);
        if (a > mayor) mayor = a;
    }
    if (mayor <= 0.0f) return out;
    for (int i = 0; i < n; ++i) out[(size_t)i] = (int16_t)(s[(size_t)i] / mayor * PICO);
    return out;
}

// ¿Suena este clic? `ahora` y `sobreBoton` (la ultima vez que el raton estuvo
// encima de un boton) en segundos; `ultimo` es el clic anterior que sono.
inline bool debeSonar(double ahora, double sobreBoton, double ultimo) {
    if (ahora - sobreBoton > VIGENCIA || ahora < sobreBoton) return false;
    return ahora - ultimo >= SEPARACION;
}

}  // namespace ClicBoton
}  // namespace Audio
