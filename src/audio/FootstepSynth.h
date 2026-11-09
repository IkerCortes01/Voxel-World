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
    Dirt,       // tierra suelta y compacta
    Leaves,     // hojarasca y copas
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

    // ========================================================================
    // UNA CAPA DE PISADA (ver FootstepSystem: 100 por material)
    // ========================================================================
    // PEDIDO: "100 sonidos distintos pero similares [...] que se superpondran
    // y de forma aleatoria se combinaran perfectamente haciendo sonidos
    // unicos de pisar pasto, arena, piedra, madera y todos los bloques".
    //
    // Una capa es un pedacito de pisada de 15 a 110 ms: un roce, un crujido,
    // un golpe sordo, una lluvia de granos, un chasquido. Cada material tiene
    // su paleta de tipos de capa y cada semilla saca una distinta dentro de su
    // tipo. Sueltas suenan a poco; juntas, a una pisada.
    //
    // Se guardan en float (no en PCM): el compositor las mezcla, las afina y
    // las suaviza antes de convertir.
    struct Capa {
        std::vector<float> s;
        int fase = 1;            // 0 talon (golpe), 1 apoyo (roce), 2 punta (despegue)
    };

    static Capa capa(StepMaterial mat, uint32_t seed) {
        Rng rng(seed * 2246822519u + 3266489917u);
        // Los ingredientes de una capa.
        struct P {
            float durMs, ataqueMs, caida, lp, hp, golpe;
            int granos; float granoMs, granoAmp;
            float cuerpoHz, cuerpoAmp, cuerpoCaida;
            float ruido;
            int fase;
        } p{ 60, 2, 40, 0.5f, 0.05f, 0.2f, 0, 5, 0.3f, 0, 0, 1, 1.0f, 1 };
        auto R = [&](float a, float b) { return rng.range(a, b); };
        const int tipo = (int)(rng.unit() * 3.0f) % 3;   // tres tipos por material
        switch (mat) {
            case StepMaterial::Grass:
                if (tipo == 0) p = { R(40, 90), R(3, 9), R(25, 45), R(0.45f, 0.70f), R(0.08f, 0.16f), R(0.0f, 0.08f), (int)R(2, 8), R(2, 5), R(0.15f, 0.35f), 0, 0, 1, 1.0f, 1 };          // roce de hojas de pasto
                else if (tipo == 1) p = { R(15, 35), R(0.5f, 2), R(70, 120), R(0.70f, 0.95f), R(0.20f, 0.35f), R(0.10f, 0.30f), (int)R(1, 4), R(1, 3), R(0.30f, 0.60f), 0, 0, 1, 0.6f, 2 };  // tallitos que se doblan
                else p = { R(35, 70), R(1, 3), R(35, 60), R(0.08f, 0.18f), R(0.004f, 0.012f), R(0.10f, 0.30f), 0, 5, 0, R(60, 120), R(0.15f, 0.35f), R(1.5f, 2.5f), 0.7f, 0 };  // la tierra blanda debajo
                break;
            case StepMaterial::Leaves:
                if (tipo == 0) p = { R(50, 110), R(2, 6), R(20, 35), R(0.55f, 0.85f), R(0.10f, 0.22f), R(0.0f, 0.1f), (int)R(10, 30), R(1, 3), R(0.35f, 0.70f), 0, 0, 1, 0.5f, 1 };    // crujido de hoja seca
                else if (tipo == 1) p = { R(30, 70), R(4, 10), R(30, 50), R(0.40f, 0.60f), R(0.06f, 0.12f), 0.0f, (int)R(3, 9), R(2, 4), R(0.2f, 0.4f), 0, 0, 1, 1.0f, 2 };         // roce
                else p = { R(30, 60), R(1, 3), R(40, 70), R(0.10f, 0.20f), R(0.005f, 0.015f), R(0.10f, 0.25f), 0, 5, 0, R(70, 130), R(0.10f, 0.25f), R(1.5f, 2.5f), 0.6f, 0 };
                break;
            case StepMaterial::Dirt:
                if (tipo == 0) p = { R(40, 80), R(1, 4), R(35, 55), R(0.10f, 0.25f), R(0.006f, 0.015f), R(0.20f, 0.40f), 0, 5, 0, R(70, 140), R(0.20f, 0.40f), R(1.4f, 2.2f), 0.8f, 0 };   // golpe sordo
                else if (tipo == 1) p = { R(40, 90), R(3, 8), R(25, 40), R(0.25f, 0.45f), R(0.03f, 0.07f), 0.05f, (int)R(4, 12), R(2, 5), R(0.20f, 0.45f), 0, 0, 1, 0.8f, 1 };    // terron que se desmorona
                else p = { R(20, 45), R(1, 3), R(50, 80), R(0.35f, 0.55f), R(0.08f, 0.15f), R(0.05f, 0.15f), (int)R(2, 6), R(1, 3), R(0.2f, 0.4f), 0, 0, 1, 0.5f, 2 };
                break;
            case StepMaterial::Sand:
                if (tipo == 0) p = { R(70, 110), R(8, 18), R(14, 24), R(0.18f, 0.32f), R(0.01f, 0.04f), 0.0f, (int)R(15, 35), R(2, 5), R(0.25f, 0.55f), 0, 0, 1, 1.0f, 1 };      // siseo de arena
                else if (tipo == 1) p = { R(40, 80), R(4, 10), R(25, 40), R(0.30f, 0.50f), R(0.04f, 0.08f), 0.0f, (int)R(20, 45), R(1, 3), R(0.35f, 0.65f), 0, 0, 1, 0.4f, 2 };    // granos que ruedan
                else p = { R(40, 70), R(4, 9), R(30, 45), R(0.08f, 0.16f), R(0.004f, 0.01f), R(0.05f, 0.15f), 0, 5, 0, R(55, 95), R(0.10f, 0.25f), R(1.2f, 2.0f), 0.7f, 0 };
                break;
            case StepMaterial::Gravel:
                if (tipo == 0) p = { R(50, 100), R(1, 3), R(22, 35), R(0.50f, 0.75f), R(0.06f, 0.12f), R(0.2f, 0.4f), (int)R(15, 35), R(3, 7), R(0.45f, 0.85f), 0, 0, 1, 0.5f, 1 };   // crujido de piedritas
                else if (tipo == 1) p = { R(15, 35), R(0.3f, 1), R(80, 140), R(0.75f, 0.95f), R(0.15f, 0.30f), R(0.4f, 0.7f), (int)R(2, 6), R(1, 3), R(0.4f, 0.7f), R(900, 2200), R(0.05f, 0.15f), R(2, 4), 0.4f, 2 };  // piedrita que choca
                else p = { R(35, 60), R(1, 3), R(40, 60), R(0.15f, 0.30f), R(0.01f, 0.03f), R(0.25f, 0.45f), (int)R(4, 10), R(3, 6), R(0.3f, 0.5f), R(90, 160), R(0.15f, 0.3f), R(1.5f, 2.5f), 0.6f, 0 };
                break;
            case StepMaterial::Snow:
                if (tipo == 0) p = { R(60, 110), R(4, 10), R(18, 30), R(0.28f, 0.42f), R(0.03f, 0.06f), R(0.05f, 0.15f), (int)R(12, 30), R(3, 6), R(0.35f, 0.65f), 0, 0, 1, 0.6f, 1 };   // nieve que se comprime
                else if (tipo == 1) p = { R(30, 60), R(2, 5), R(35, 55), R(0.45f, 0.65f), R(0.08f, 0.14f), 0.05f, (int)R(5, 14), R(2, 4), R(0.3f, 0.5f), R(700, 1400), R(0.03f, 0.08f), R(2, 3), 0.4f, 2 };  // chirrido
                else p = { R(40, 70), R(2, 6), R(30, 45), R(0.08f, 0.16f), R(0.004f, 0.01f), R(0.1f, 0.2f), 0, 5, 0, R(60, 110), R(0.10f, 0.22f), R(1.4f, 2.2f), 0.6f, 0 };
                break;
            case StepMaterial::Stone:
                if (tipo == 0) p = { R(40, 80), R(0.3f, 1), R(45, 70), R(0.55f, 0.85f), R(0.10f, 0.20f), R(0.5f, 0.9f), 0, 5, 0, R(150, 380), R(0.20f, 0.40f), R(1.6f, 2.6f), 0.5f, 0 };  // taconazo
                else if (tipo == 1) p = { R(25, 55), R(2, 5), R(40, 70), R(0.60f, 0.85f), R(0.15f, 0.30f), R(0.05f, 0.15f), (int)R(2, 6), R(1, 3), R(0.15f, 0.3f), 0, 0, 1, 0.9f, 1 };   // roce de la suela
                else p = { R(15, 35), R(0.3f, 1), R(70, 120), R(0.70f, 0.95f), R(0.20f, 0.35f), R(0.3f, 0.6f), 0, 5, 0, R(500, 1200), R(0.05f, 0.15f), R(2, 3.5f), 0.5f, 2 };  // la punta
                break;
            case StepMaterial::Wood:
                if (tipo == 0) p = { R(60, 110), R(0.5f, 2), R(35, 55), R(0.40f, 0.60f), R(0.04f, 0.08f), R(0.4f, 0.7f), 0, 5, 0, R(170, 420), R(0.35f, 0.60f), R(1.2f, 1.9f), 0.5f, 0 };  // tabla hueca
                else if (tipo == 1) p = { R(30, 60), R(2, 5), R(35, 60), R(0.50f, 0.75f), R(0.10f, 0.20f), R(0.05f, 0.15f), (int)R(1, 4), R(1, 3), R(0.1f, 0.25f), R(400, 900), R(0.05f, 0.12f), R(1.5f, 2.5f), 0.8f, 1 };  // roce sobre la veta
                else p = { R(40, 90), R(5, 15), R(18, 30), R(0.30f, 0.45f), R(0.02f, 0.05f), 0.0f, 0, 5, 0, R(120, 240), R(0.10f, 0.25f), R(0.8f, 1.4f), 0.2f, 2 };  // crujido de la tabla
                break;
            default:
                break;
        }
        Capa c;
        c.fase = p.fase;
        const int sr = AudioMixer::SAMPLE_RATE;
        const int n = (int)(sr * p.durMs / 1000.0f);
        if (n <= 4) return c;
        c.s.assign(n, 0.0f);
        LowPass lp(p.lp);
        HighPass hp(p.hp);
        const int ataque = (int)(sr * p.ataqueMs / 1000.0f) + 1;
        const float w = p.cuerpoHz > 0.0f ? 2.0f * 3.14159265f * p.cuerpoHz / (float)sr : 0.0f;
        const float fase0 = rng.unit() * 6.2831853f;
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr;
            const float env = i < ataque ? (float)i / (float)ataque : expf(-p.caida * (t - (float)ataque / sr));
            float s = hp(lp(rng.noise())) * env * p.ruido;
            if (p.golpe > 0.0f) s += rng.noise() * expf(-300.0f * t) * p.golpe;
            if (w > 0.0f) s += sinf(w * (float)i + fase0) * expf(-p.caida * p.cuerpoCaida * t) * p.cuerpoAmp;
            c.s[i] = s;
        }
        // Los granos: impulsos cortos repartidos (arena, grava, hojas...).
        const int largoGrano = (int)(sr * p.granoMs / 1000.0f) + 1;
        for (int g = 0; g < p.granos; ++g) {
            const int ini = (int)(rng.unit() * n * 0.85f);
            const float amp = rng.range(0.3f, 1.0f) * p.granoAmp;
            HighPass ghp(rng.range(0.1f, 0.4f));
            for (int i = 0; i < largoGrano && ini + i < n; ++i)
                c.s[ini + i] += ghp(rng.noise()) * expf(-6.0f * (float)i / (float)largoGrano) * amp;
        }
        // Sin clics al principio ni al final.
        const int borde = (int)(sr * 0.003f);
        for (int i = 0; i < borde && i < n; ++i) {
            const float f = (float)i / (float)borde;
            c.s[n - 1 - i] *= f;
        }
        return c;
    }
};

} // namespace Audio

#endif // FOOTSTEP_SYNTH_H
