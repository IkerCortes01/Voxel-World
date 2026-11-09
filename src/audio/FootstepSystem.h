#ifndef FOOTSTEP_SYSTEM_H
#define FOOTSTEP_SYSTEM_H

#include "AudioMixer.h"
#include "FootstepSynth.h"
#include <algorithm>
#include <cmath>
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

// ----------------------------------------------------------------------------
// 100 CAPAS POR MATERIAL, COMBINADAS AL AZAR
// ----------------------------------------------------------------------------
// PEDIDO: "crea 100 sonidos distintos pero similares [...] que se superpondran
// y de forma aleatoria se combinaran perfectamente haciendo sonidos unicos
// [...] y al combinarse se haran un solo sonido suave de pisada".
//
// Cada material tiene CAPAS capas (FootstepSynth::capa): roces, crujidos,
// golpes sordos, granos. Cada pisada se COMPONE en el momento con 5 a 7 de
// ellas, como pisa un pie de verdad:
//   TALON   el golpe al apoyar (1 o 2 capas, al principio)
//   APOYO   el peso que rueda sobre la planta (2 o 3 capas, 10-60 ms despues)
//   PUNTA   el despegue (1 o 2 capas, 60-120 ms despues, mas flojas)
// con desfases, tonos y volumenes al azar. Despues se suaviza (un paso bajo
// segun el material) y se normaliza a un volumen fijo y comodo: la suma
// nunca da un golpe brusco aunque coincidan capas fuertes. Con 100 capas y
// estas combinaciones no hay dos pisadas iguales en toda una partida.
class FootstepSystem {
public:
    static constexpr int VARIANTS = 4;
    static constexpr int CAPAS = 100;
    static constexpr int ANILLO = 16;   // pisadas compuestas que pueden sonar a la vez
private:
    AudioMixer* mixer = nullptr;

    // [material][variante]
    SoundBuffer buffers[(int)StepMaterial::COUNT][VARIANTS];
    std::vector<FootstepSynth::Capa> capas[(int)StepMaterial::COUNT];
    SoundBuffer anillo[ANILLO];
    int siguiente = 0;
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
        for (int mat = 0; mat < (int)StepMaterial::COUNT; ++mat) {
            capas[mat].clear();
            for (int c = 0; c < CAPAS; ++c)
                capas[mat].push_back(FootstepSynth::capa((StepMaterial)mat, (uint32_t)(mat * 7919 + c * 104729 + 17)));
        }
        generated = true;
    }

    // Compone una pisada unica con las capas del material. uerza 0..1:
    // agachado ~0.3, andando ~0.6, corriendo ~0.85, aterrizando hasta 1.
    const SoundBuffer* componer(StepMaterial material, float fuerza) {
        const std::vector<FootstepSynth::Capa>& banco = capas[(int)material];
        if (banco.empty()) return nullptr;
        const int sr = AudioMixer::SAMPLE_RATE;
        static thread_local std::vector<float> mezcla;
        mezcla.assign((size_t)(sr * 0.34f), 0.0f);
        // Cuantas capas de cada fase.
        const int nTalon = fuerza > 0.7f ? 2 : 1 + (rnd01() < 0.4f ? 1 : 0);
        const int nApoyo = 2 + (rnd01() < 0.5f ? 1 : 0);
        const int nPunta = fuerza < 0.35f ? 1 : 1 + (rnd01() < 0.5f ? 1 : 0);
        auto ponerFase = [&](int fase, int cuantas, float desdeMs, float hastaMs, float gan) {
            for (int k = 0; k < cuantas; ++k) {
                // Una capa al azar, prefiriendo las de su fase.
                int c = (int)(rnd01() * CAPAS) % CAPAS;
                for (int intento = 0; intento < 6 && banco[c].fase != fase; ++intento) c = (int)(rnd01() * CAPAS) % CAPAS;
                const std::vector<float>& s = banco[c].s;
                if (s.empty()) continue;
                const float paso = rndRange(0.90f, 1.10f);          // el tono de esta capa
                const float g = gan * rndRange(0.55f, 1.0f);
                const int ini = (int)(sr * rndRange(desdeMs, hastaMs) / 1000.0f);
                for (size_t i = 0;; ++i) {
                    const float pos = (float)i * paso;
                    const size_t j = (size_t)pos;
                    if (j + 1 >= s.size() || ini + i >= mezcla.size()) break;
                    const float fr = pos - (float)j;
                    mezcla[ini + i] += (s[j] + (s[j + 1] - s[j]) * fr) * g;
                }
            }
        };
        ponerFase(0, nTalon, 0.0f, 8.0f, 0.6f + 0.6f * fuerza);
        ponerFase(1, nApoyo, 10.0f, 60.0f, 0.8f);
        ponerFase(2, nPunta, 60.0f + 40.0f * (1.0f - fuerza), 120.0f + 40.0f * (1.0f - fuerza), 0.45f + 0.2f * fuerza);
        // Suavizar: un paso bajo de un polo (mas oscuro en los blandos) y
        // normalizar el pico a un volumen comodo.
        float corte = 0.55f;
        switch (material) {
            case StepMaterial::Sand: case StepMaterial::Snow: case StepMaterial::Dirt: corte = 0.35f; break;
            case StepMaterial::Stone: case StepMaterial::Gravel: corte = 0.75f; break;
            default: break;
        }
        float y = 0.0f, pico = 1e-6f;
        size_t ultimo = 0;
        for (size_t i = 0; i < mezcla.size(); ++i) {
            y += corte * (mezcla[i] - y);
            mezcla[i] = y;
            const float a = std::fabs(y);
            if (a > pico) pico = a;
            if (a > 1e-4f) ultimo = i;
        }
        const float k = (0.42f + 0.30f * fuerza) / pico;
        const size_t n = (std::min)(mezcla.size(), ultimo + 8);
        SoundBuffer& out = anillo[siguiente];
        siguiente = (siguiente + 1) % ANILLO;
        out.sampleRate = sr;
        out.samples.resize(n);
        const size_t entrada = (size_t)(sr * 0.0015f), salida = (size_t)(sr * 0.015f);
        for (size_t i = 0; i < n; ++i) {
            float v = mezcla[i] * k;
            if (i < entrada) v *= (float)i / (float)entrada;
            if (i + salida > n) v *= (float)(n - i) / (float)salida;
            out.samples[i] = (int16_t)((std::max)(-1.0f, (std::min)(1.0f, v)) * 32000.0f);
        }
        return &out;
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

        // ---- Emitir la pisada: una nueva, compuesta con las capas ----

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

        const float fuerza = isCrouching ? 0.3f : (isSprinting ? 0.85f : 0.6f);
        if (const SoundBuffer* b = componer(material, fuerza)) mixer->play(b, vol, pitch);
        ++stepCounter;
    }

    // Sonido de aterrizaje: mas fuerte y grave que una pisada normal.
    void playLanding(StepMaterial material, float fallSpeed) {
        if (!isReady()) return;
        if (fallSpeed < 3.0f) return;   // caidas leves no suenan

        float vol = 0.5f + fallSpeed * 0.045f;
        if (vol > 1.0f) vol = 1.0f;

        if (const SoundBuffer* b = componer(material, 1.0f)) mixer->play(b, vol, rndRange(0.80f, 0.88f));
        distanceAccum = 0.0f;
    }

    void reset() {
        distanceAccum = 0.0f;
        leftFoot = true;
    }
};

} // namespace Audio

#endif // FOOTSTEP_SYSTEM_H
