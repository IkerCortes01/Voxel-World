#ifndef FOOTSTEP_SYNTH_H
#define FOOTSTEP_SYNTH_H

#include "AudioMixer.h"
#include <cstdint>
#include <cmath>
#include <vector>

// ============================================================================
// FOOTSTEP SYNTH - Sintesis procedural de sonidos de pisada
// ============================================================================
// RESPONSABILIDAD UNICA: generar los SoundBuffer de pasos. No decide cuando
// suenan (eso es del juego) ni los reproduce (eso es del mixer).
//
// ----------------------------------------------------------------------------
// POR QUE SINTESIS Y NO ARCHIVOS
// ----------------------------------------------------------------------------
// El proyecto no tiene archivos de audio (la carpeta sounds/ esta vacia) y el
// codigo antiguo caia en un Beep() del sistema. Generar el audio en codigo:
//   - No anade dependencias ni descargas.
//   - Permite N variaciones por material sin N archivos.
//   - Ocupa unos pocos KB de RAM.
//
// ----------------------------------------------------------------------------
// MODELO ACUSTICO
// ----------------------------------------------------------------------------
// Una pisada real es RUIDO conformado por tres cosas: el contenido espectral
// del material, una envolvente muy corta, y un transitorio de impacto.
//
//   MATERIAL   ESPECTRO              ENVOLVENTE        CARACTER
//   ---------  --------------------  ----------------  ---------------------
//   Pasto      agudo, filtrado alto  muy corta (80ms)  susurro seco, suave
//   Piedra     banda media + click   corta con golpe   impacto duro, nitido
//   Arena      grave, muy difuso     larga (150ms)     siseo mate, sin ataque
//   Madera     resonancia tonal      media             hueco, con cuerpo
//   Grava      ruido + granos        media irregular   crujido, particulas
//
// Tecnicas usadas:
//   1. Ruido blanco como fuente (todas las frecuencias por igual).
//   2. Filtro paso-bajo/paso-alto de 1 polo para dar el color del material.
//   3. Envolvente exponencial: ataque instantaneo, caida configurable.
//   4. "Granos" superpuestos en grava/arena para el detalle de particulas.
//   5. Resonancia amortiguada en madera y piedra para el cuerpo del golpe.
//
// DETERMINISMO: el generador de ruido es un LCG con semilla explicita, asi
// que las variaciones son reproducibles y no dependen de rand() global.
// ============================================================================

namespace Audio {

// Materiales de pisada reconocidos por el juego.
enum class StepMaterial {
    Grass,
    Stone,
    Sand,
    Wood,
    Gravel,
    Snow,
    COUNT
};

class FootstepSynth {
private:
    // ---- Generador de ruido determinista (LCG) ----
    struct Rng {
        uint32_t s;
        explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
        uint32_t next() {
            s = s * 1664525u + 1013904223u;
            return s;
        }
        // Ruido blanco en [-1, 1]
        float noise() {
            return (float)((int32_t)(next() >> 8) - 8388608) / 8388608.0f;
        }
        float unit() { return (float)(next() >> 8) / 16777216.0f; }
        float range(float a, float b) { return a + (b - a) * unit(); }
    };

    // Filtro paso-bajo de 1 polo. cutoff en [0,1] (fraccion de Nyquist).
    struct LowPass {
        float y = 0.0f, a;
        explicit LowPass(float cutoff) { a = cutoff < 0.001f ? 0.001f : (cutoff > 1.0f ? 1.0f : cutoff); }
        float operator()(float x) { y += a * (x - y); return y; }
    };

    // Paso-alto derivado del paso-bajo.
    struct HighPass {
        LowPass lp;
        explicit HighPass(float cutoff) : lp(cutoff) {}
        float operator()(float x) { return x - lp(x); }
    };

    static int16_t toPCM(float v) {
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        return (int16_t)(v * 32000.0f);
    }

public:
    // ========================================================================
    // GENERAR UNA PISADA
    // ========================================================================
    // seed: distintas semillas -> variaciones del mismo material.
    static SoundBuffer generate(StepMaterial mat, uint32_t seed) {
        SoundBuffer out;
        out.sampleRate = AudioMixer::SAMPLE_RATE;

        Rng rng(seed * 2654435761u + 12345u);

        // --- Parametros por material ---
        float durationMs;   // duracion total
        float lpCutoff;     // color: bajo = grave/apagado, alto = brillante
        float hpCutoff;     // recorte de graves
        float decay;        // velocidad de caida de la envolvente
        float attackMs;     // duracion del ataque
        float impactAmp;    // fuerza del transitorio inicial
        float grainAmp;     // cantidad de "granos" (particulas)
        float bodyFreq;     // resonancia del cuerpo (Hz), 0 = sin cuerpo
        float bodyAmp;

        switch (mat) {
            case StepMaterial::Grass:
                // Susurro corto y agudo. Sin golpe: la hierba amortigua.
                durationMs = 95.0f;  lpCutoff = 0.55f; hpCutoff = 0.06f;
                decay      = 42.0f;  attackMs = 2.0f;  impactAmp = 0.20f;
                grainAmp   = 0.18f;  bodyFreq = 0.0f;  bodyAmp   = 0.0f;
                break;

            case StepMaterial::Stone:
                // Golpe duro y nitido, con resonancia corta y seca.
                durationMs = 110.0f; lpCutoff = 0.75f; hpCutoff = 0.10f;
                decay      = 55.0f;  attackMs = 0.5f;  impactAmp = 0.85f;
                grainAmp   = 0.05f;  bodyFreq = 190.0f; bodyAmp  = 0.30f;
                break;

            case StepMaterial::Sand:
                // Siseo mate y difuso, sin ataque marcado y con cola larga.
                durationMs = 165.0f; lpCutoff = 0.28f; hpCutoff = 0.02f;
                decay      = 16.0f;  attackMs = 14.0f; impactAmp = 0.10f;
                grainAmp   = 0.30f;  bodyFreq = 0.0f;  bodyAmp   = 0.0f;
                break;

            case StepMaterial::Wood:
                // Hueco, con cuerpo tonal claro.
                durationMs = 120.0f; lpCutoff = 0.50f; hpCutoff = 0.05f;
                decay      = 38.0f;  attackMs = 1.0f;  impactAmp = 0.55f;
                grainAmp   = 0.06f;  bodyFreq = 320.0f; bodyAmp  = 0.45f;
                break;

            case StepMaterial::Gravel:
                // Crujido: muchos granos discretos.
                durationMs = 140.0f; lpCutoff = 0.68f; hpCutoff = 0.08f;
                decay      = 26.0f;  attackMs = 1.5f;  impactAmp = 0.45f;
                grainAmp   = 0.55f;  bodyFreq = 0.0f;  bodyAmp   = 0.0f;
                break;

            case StepMaterial::Snow:
                // Crujido apagado y grave, muy amortiguado.
                durationMs = 130.0f; lpCutoff = 0.32f; hpCutoff = 0.03f;
                decay      = 24.0f;  attackMs = 6.0f;  impactAmp = 0.25f;
                grainAmp   = 0.35f;  bodyFreq = 0.0f;  bodyAmp   = 0.0f;
                break;

            default:
                durationMs = 100.0f; lpCutoff = 0.5f; hpCutoff = 0.05f;
                decay      = 40.0f;  attackMs = 2.0f; impactAmp = 0.3f;
                grainAmp   = 0.1f;   bodyFreq = 0.0f; bodyAmp   = 0.0f;
                break;
        }

        // Variacion por semilla (+-8%): dos pisadas nunca son idénticas.
        durationMs *= rng.range(0.92f, 1.08f);
        decay      *= rng.range(0.92f, 1.08f);
        lpCutoff   *= rng.range(0.90f, 1.10f);
        if (lpCutoff > 0.98f) lpCutoff = 0.98f;

        const int sr = out.sampleRate;
        const int n  = (int)(sr * durationMs / 1000.0f);
        if (n <= 0) return out;

        out.samples.resize(n);

        LowPass  lp(lpCutoff);
        HighPass hp(hpCutoff);

        const int attackSamples = (int)(sr * attackMs / 1000.0f) + 1;

        // --- Granos: pequeños impulsos dispersos (arena, grava, nieve) ---
        const int grainCount = (int)(grainAmp * 40.0f);
        std::vector<int> grainPos(grainCount);
        std::vector<float> grainGain(grainCount);
        for (int g = 0; g < grainCount; ++g) {
            grainPos[g]  = (int)(rng.unit() * n * 0.7f);
            grainGain[g] = rng.range(0.3f, 1.0f);
        }

        // --- Resonancia del cuerpo ---
        const float bodyOmega = bodyFreq > 0.0f
                              ? 2.0f * 3.14159265f * bodyFreq / (float)sr
                              : 0.0f;

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr;

            // Envolvente: ataque lineal corto + caida exponencial.
            float env;
            if (i < attackSamples) {
                env = (float)i / (float)attackSamples;
            } else {
                env = expf(-decay * (t - (float)attackSamples / sr));
            }

            // Fuente: ruido conformado por los filtros.
            float s = rng.noise();
            s = lp(s);
            s = hp(s);
            s *= env;

            // Transitorio de impacto: pico muy corto al inicio.
            if (impactAmp > 0.0f) {
                const float imp = expf(-260.0f * t);
                s += rng.noise() * imp * impactAmp;
            }

            // Cuerpo resonante (madera/piedra).
            if (bodyOmega > 0.0f) {
                s += sinf(bodyOmega * (float)i) * expf(-decay * 1.6f * t) * bodyAmp;
            }

            out.samples[i] = toPCM(s * 0.85f);
        }

        // --- Superponer los granos ---
        for (int g = 0; g < grainCount; ++g) {
            const int start = grainPos[g];
            const int len = (int)(sr * 0.006f); // 6 ms por grano
            Rng grng(seed * 7919u + (uint32_t)g * 104729u);
            for (int i = 0; i < len && start + i < n; ++i) {
                const float ge = expf(-380.0f * (float)i / (float)sr);
                const float gs = grng.noise() * ge * grainGain[g] * grainAmp * 0.5f;
                const int idx = start + i;
                float mixed = (float)out.samples[idx] / 32000.0f + gs;
                out.samples[idx] = toPCM(mixed);
            }
        }

        // --- Fade-out final: evita el "click" al cortar el buffer ---
        const int fade = (int)(sr * 0.008f);
        for (int i = 0; i < fade && i < n; ++i) {
            const float f = (float)i / (float)fade;
            out.samples[n - 1 - i] = (int16_t)(out.samples[n - 1 - i] * f);
        }

        return out;
    }
};

} // namespace Audio

#endif // FOOTSTEP_SYNTH_H
