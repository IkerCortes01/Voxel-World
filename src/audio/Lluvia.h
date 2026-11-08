#pragma once
// ============================================================================
// EL SONIDO DE LA LLUVIA, DEL GRANIZO Y DE LOS TRUENOS
// ============================================================================
// Como el viento (Viento.h), no hay archivos: se SINTETIZA en el hilo de audio.
//
//   LLUVIA    un siseo de ruido filtrado (la lluvia lejana, miles de gotas a
//             la vez) mas gotas sueltas que golpean cerca: chasquidos cortos
//             y al azar. Con llovizna se oyen casi solo las gotas sueltas;
//             con aguacero, el siseo lo llena todo.
//   GRANIZO   golpes secos y agudos, muchos y desordenados: piedras de hielo
//             contra el suelo.
//   TRUENO    al llegar, un CHASQUIDO (solo si el rayo cayo cerca), y luego
//             el RETUMBO: ruido muy grave que crece, ondula y se apaga en
//             unos segundos. Los lejanos son solo retumbo sordo.
//
// BAJO TECHO se oye amortiguada: menos volumen y sin los agudos, como a
// traves de un tejado.
//
// Sin dependencias: se prueba en los tests sin abrir el audio.
// ============================================================================

#include <cmath>
#include <cstdint>

namespace Audio {

struct GeneradorLluvia {
    uint32_t azar = 0x9e3779b9u;
    float agua = 0.0f, granizo = 0.0f, abierto = 1.0f;   // suavizados
    float b1 = 0.0f, b2 = 0.0f;     // pasa-banda del siseo
    float g1 = 0.0f, g2 = 0.0f;     // pasa-banda del cuerpo
    float lp = 0.0f;                // pasa-bajos de lo amortiguado
    float gota = 0.0f, gotaTono = 0.0f, gotaFase = 0.0f;
    float piedra = 0.0f, piedraFase = 0.0f, piedraTono = 0.0f;

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

    // Suma `n` muestras. Las intensidades van de 0 a 1; `cielo` es cuanto
    // cielo abierto tiene encima el jugador (0 bajo techo).
    void llenar(float* salida, int n, int frecuencia, float objAgua, float objGranizo, float objCielo) {
        const float dt = 1.0f / (float)frecuencia;
        const float suaviza = 1.0f - std::exp(-dt / 1.5f);
        const float fSiseo = 2.0f * std::sin(3.14159265f * 3200.0f * dt);
        const float fCuerpo = 2.0f * std::sin(3.14159265f * 700.0f * dt);
        for (int i = 0; i < n; ++i) {
            agua += (objAgua - agua) * suaviza;
            granizo += (objGranizo - granizo) * suaviza;
            abierto += (objCielo - abierto) * suaviza;
            if (agua < 1e-4f && granizo < 1e-4f) continue;

            const float w = blanco();
            float v = 0.0f;
            // El siseo crece mas que proporcional: la llovizna casi no lo tiene.
            const float sis = agua * agua;
            v += banda(w, fSiseo, 0.9f, b1, b2) * 2600.0f * sis;
            v += banda(w, fCuerpo, 1.1f, g1, g2) * 1500.0f * sis;

            // Gotas sueltas: chasquidos de 2-4 ms, mas cuanto mas llueve.
            if (gota <= 0.0f && uno() < agua * 0.012f) {
                gota = 1.0f;
                gotaTono = 0.15f + 0.35f * uno();
                gotaFase = 0.0f;
            }
            if (gota > 0.0f) {
                gotaFase += gotaTono;
                v += std::sin(gotaFase) * gota * 2600.0f * (0.4f + 0.6f * agua);
                gota -= dt * 350.0f;
            }
            // Granizo: golpes secos, agudos y muy seguidos.
            if (piedra <= 0.0f && uno() < granizo * 0.02f) {
                piedra = 1.0f;
                piedraTono = 0.6f + 0.6f * uno();
                piedraFase = 0.0f;
            }
            if (piedra > 0.0f) {
                piedraFase += piedraTono;
                v += (std::sin(piedraFase) * 0.6f + blanco() * 0.4f) * piedra * 5200.0f;
                piedra -= dt * 600.0f;
            }
            // Bajo techo: sin agudos y mas bajo.
            lp += (v - lp) * 0.08f;
            const float a = abierto < 0.0f ? 0.0f : (abierto > 1.0f ? 1.0f : abierto);
            salida[i] += v * a + lp * (1.0f - a) * 0.55f;
        }
    }
};

struct GeneradorTrueno {
    static constexpr int MAX = 3;
    struct Uno {
        bool activo = false;
        float t = 0.0f, dura = 5.0f, fuerza = 0.0f, chasquido = 0.0f;
        float m1 = 0.0f, m2 = 0.0f, ondula = 0.0f;
    };
    Uno truenos[MAX];
    uint32_t azar = 0x2545f491u;

    float blanco() {
        azar ^= azar << 13; azar ^= azar >> 17; azar ^= azar << 5;
        return (float)(azar & 0xFFFFFFu) / 8388607.5f - 1.0f;
    }

    // Empieza un trueno. `fuerza` 0..1 (los cercanos, cerca de 1).
    void disparar(float fuerza) {
        int libre = 0;
        for (int k = 0; k < MAX; ++k) {
            if (!truenos[k].activo) { libre = k; break; }
            if (truenos[k].t > truenos[libre].t) libre = k;   // roba el mas viejo
        }
        Uno& u = truenos[libre];
        u = Uno();
        u.activo = true;
        u.fuerza = fuerza < 0.0f ? 0.0f : (fuerza > 1.0f ? 1.0f : fuerza);
        u.dura = 3.0f + 4.5f * (1.0f - u.fuerza) + 2.0f * (blanco() * 0.5f + 0.5f);
        u.chasquido = u.fuerza > 0.55f ? (u.fuerza - 0.55f) * 2.2f : 0.0f;
    }

    void llenar(float* salida, int n, int frecuencia, float cielo) {
        const float dt = 1.0f / (float)frecuencia;
        const float a = 0.45f + 0.55f * (cielo < 0.0f ? 0.0f : (cielo > 1.0f ? 1.0f : cielo));
        for (int k = 0; k < MAX; ++k) {
            Uno& u = truenos[k];
            if (!u.activo) continue;
            for (int i = 0; i < n; ++i) {
                u.t += dt;
                if (u.t >= u.dura) { u.activo = false; break; }
                const float w = blanco();
                // Retumbo: ruido integrado dos veces, muy grave.
                u.m1 = u.m1 * 0.995f + w * 0.05f;
                u.m2 = u.m2 * 0.992f + u.m1 * 0.03f;
                // Crece en medio segundo, ondula y se apaga.
                const float x = u.t / u.dura;
                float env = (u.t < 0.5f ? u.t / 0.5f : 1.0f) * std::pow(1.0f - x, 1.6f);
                u.ondula += (blanco() * 0.5f + 0.5f - u.ondula) * dt * 3.0f;
                env *= 0.55f + 0.9f * u.ondula;
                float v = u.m2 * 16000.0f * env * (0.25f + 0.75f * u.fuerza);
                // El chasquido: un latigazo de ruido en las primeras decimas.
                if (u.chasquido > 0.0f && u.t < 0.25f)
                    v += w * 6500.0f * u.chasquido * std::exp(-u.t * 18.0f);
                salida[i] += v * a;
            }
        }
    }
};

} // namespace Audio
