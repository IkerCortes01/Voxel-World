#ifndef FOOTSTEP_SYSTEM_H
#define FOOTSTEP_SYSTEM_H

#include "AudioMixer.h"
#include "FootstepSynth.h"
#include <vector>

// ============================================================================
// FOOTSTEP SYSTEM - Decide CUANDO suena cada pisada
// ============================================================================
// RESPONSABILIDAD UNICA: llevar la cadencia del paso y disparar el sonido.
// No sintetiza (FootstepSynth) ni reproduce (AudioMixer).
//
// ----------------------------------------------------------------------------
// CADENCIA POR DISTANCIA, NO POR TIEMPO
// ----------------------------------------------------------------------------
// El sistema antiguo usaba un throttle fijo de 0.3 s. Eso produce dos fallos
// evidentes:
//   - Al caminar despacio (o empujando contra una pared sin avanzar) los pasos
//     siguen sonando al mismo ritmo.
//   - Al correr, la cadencia no se acelera y el sonido se desincroniza del
//     movimiento.
//
// Aqui se acumula la DISTANCIA HORIZONTAL recorrida y se emite una pisada cada
// STRIDE_LENGTH bloques. Asi la cadencia sale sola: mas rapido -> mas pasos,
// parado -> silencio, sin necesidad de conocer la velocidad.
//
// ----------------------------------------------------------------------------
// VARIACION
// ----------------------------------------------------------------------------
// Cada pisada elige una de VARIANTS muestras pregeneradas del material y le
// aplica un pitch aleatorio de +-6% y un volumen de +-15%. Con 4 variantes y
// esa modulacion no se percibe repeticion.
//
// Ademas se alternan pie izquierdo/derecho con un ligero desequilibrio de
// volumen, que es lo que hace que la marcha suene humana y no metronomica.
// ============================================================================

namespace Audio {

class FootstepSystem {
public:
    static constexpr int VARIANTS = 4;

private:
    AudioMixer* mixer = nullptr;

    // [material][variante]
    SoundBuffer buffers[(int)StepMaterial::COUNT][VARIANTS];
    bool generated = false;

    // Estado de la marcha
    float distanceAccum = 0.0f;
    bool  leftFoot = true;
    uint32_t stepCounter = 0;

    // Longitud de zancada en bloques. ~0.9 da una cadencia natural a la
    // velocidad de caminar (4.3 b/s -> ~4.8 pasos/s... se ajusta abajo).
    static constexpr float STRIDE_LENGTH = 1.35f;

    // Rng simple para la variacion (determinista por contador).
    uint32_t rngState = 22222u;
    float rnd01() {
        rngState = rngState * 1664525u + 1013904223u;
        return (float)(rngState >> 8) / 16777216.0f;
    }
    float rndRange(float a, float b) { return a + (b - a) * rnd01(); }

public:
    void init(AudioMixer* m) {
        mixer = m;
        if (generated) return;

        // Pregenerar todas las variantes al arrancar (unos pocos ms y
        // ~200 KB en total). Hacerlo en caliente causaria un tiron.
        for (int mat = 0; mat < (int)StepMaterial::COUNT; ++mat) {
            for (int v = 0; v < VARIANTS; ++v) {
                buffers[mat][v] = FootstepSynth::generate(
                    (StepMaterial)mat, (uint32_t)(mat * 1000 + v * 37 + 1));
            }
        }
        generated = true;
    }

    bool isReady() const { return generated && mixer && mixer->isReady(); }

    // ------------------------------------------------------------------------
    // ACTUALIZAR
    // ------------------------------------------------------------------------
    // dx, dz     : desplazamiento horizontal desde el frame anterior
    // onGround   : si no toca suelo, no hay pisadas
    // material   : material del bloque bajo los pies
    // isSprinting: acelera la cadencia y sube el volumen
    // isCrouching: pasos mas suaves y espaciados
    void update(float dx, float dz, bool onGround,
                StepMaterial material, bool isSprinting, bool isCrouching) {
        if (!isReady()) return;

        if (!onGround) {
            // En el aire no se acumula; al aterrizar la marcha empieza limpia.
            distanceAccum = 0.0f;
            return;
        }

        const float dist = sqrtf(dx * dx + dz * dz);

        // Umbral de ruido: evita pasos por micro-desplazamientos (jitter de
        // colision, deslizarse contra una pared sin avanzar de verdad).
        if (dist < 1e-4f) return;

        distanceAccum += dist;

        // Zancada: mas corta al correr (pasos mas frecuentes), mas larga
        // al agacharse (avance sigiloso).
        float stride = STRIDE_LENGTH;
        if (isSprinting) stride *= 0.78f;
        if (isCrouching) stride *= 1.45f;

        if (distanceAccum < stride) return;
        distanceAccum -= stride;

        // ---- Emitir la pisada ----
        const int mat = (int)material;
        const int variant = (int)(rnd01() * VARIANTS) % VARIANTS;

        // Volumen base por estado
        float vol = 0.55f;
        if (isSprinting) vol = 0.75f;
        if (isCrouching) vol = 0.28f;

        // Desequilibrio izquierda/derecha: el pie de apoyo suena algo mas
        // fuerte. Sutil, pero es lo que rompe la sensacion de metronomo.
        vol *= leftFoot ? 1.0f : 0.88f;
        leftFoot = !leftFoot;

        // Variacion final
        vol   *= rndRange(0.88f, 1.12f);
        float pitch = rndRange(0.94f, 1.06f);
        if (isCrouching) pitch *= 0.94f;  // mas grave al agacharse

        mixer->play(&buffers[mat][variant], vol, pitch);
        ++stepCounter;
    }

    // Sonido de aterrizaje: mas fuerte y grave que una pisada normal.
    void playLanding(StepMaterial material, float fallSpeed) {
        if (!isReady()) return;
        if (fallSpeed < 3.0f) return;   // caidas leves no suenan

        const int mat = (int)material;
        const int variant = (int)(rnd01() * VARIANTS) % VARIANTS;

        float vol = 0.5f + fallSpeed * 0.045f;
        if (vol > 1.0f) vol = 1.0f;

        mixer->play(&buffers[mat][variant], vol, rndRange(0.80f, 0.88f));
        distanceAccum = 0.0f;
    }

    void reset() {
        distanceAccum = 0.0f;
        leftFoot = true;
    }
};

} // namespace Audio

#endif // FOOTSTEP_SYSTEM_H
