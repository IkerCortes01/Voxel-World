#pragma once

#include <cmath>
#include <cstdint>
#include <vector>
#include "AudioMixer.h"
#include "FootstepSynth.h"

// ============================================================================
// ⭐⭐ LOS SONIDOS DE CADA BLOQUE: COLOCAR, GOLPEAR Y ROMPER
// ============================================================================
// PEDIDO: "mejora los sonidos y agrega 100 sonidos entrelazados de cada bloque
// al ser colocado, mientras es golpeado, y mejora los sonidos de pisadas del
// jugador a cada bloque, que sea unico y diferente cada bloque".
//
// Antes, colocar un bloque no sonaba (pedia un .wav que no existe) y golpearlo
// tampoco. Aqui se SINTETIZAN, igual que las pisadas, y con la misma idea de
// capas que se entrelazan:
//
//   - Cada MATERIAL (piedra, madera, tierra, arena, grava, pasto, hojas, nieve)
//     tiene un banco de 100 capas para COLOCAR y otro de 100 para GOLPEAR: el
//     chasquido, el cuerpo que resuena, los granos que saltan, la cola.
//   - Cada sonido se COMPONE en el momento con tres o cuatro de esas capas,
//     desfasadas unos milisegundos, y nunca las mismas que la vez anterior.
//   - Y cada BLOQUE tiene su VOZ: un tono, un brillo, una resonancia y un
//     peso que salen de su identidad y son siempre los mismos para el. Las cien
//     capas, pasadas por esa voz, son los cien sonidos de ESE bloque: el roble
//     no suena como el pino, ni la caliza como el granito, aunque los dos sean
//     "madera" o "piedra".
//
// La misma voz afina las pisadas (ver FootstepSystem): cada bloque se pisa con
// su tono y su brillo.
//
// Los bancos se generan la primera vez que hacen falta, material a material
// (unos milisegundos), no al arrancar: abrir el juego no se retrasa.
// ============================================================================

namespace Audio {

// ----------------------------------------------------------------------------
// LA VOZ DE UN BLOQUE
// ----------------------------------------------------------------------------
//   tono    cuanto mas agudo (>1) o grave (<1) suena que su material
//   brillo  cuanto se le abre o se le apaga el agudo
//   cuerpo  lo que pesa su resonancia (un bloque macizo retumba mas)
//   peso    lo fuerte del golpe frente a la cola
//   cola    lo que tarda en apagarse
struct VozDeBloque {
    float tono = 1.0f, brillo = 1.0f, cuerpo = 1.0f, peso = 1.0f, cola = 1.0f;
};

// La voz sale de la identidad del bloque: el mismo numero da siempre la misma
// voz, y dos numeros distintos, voces distintas (son 5 rasgos independientes).
inline VozDeBloque vozDeBloque(uint32_t idBloque) {
    auto dado = [&](uint32_t sal) {
        uint32_t h = idBloque * 2654435761u ^ (sal * 0x9E3779B9u + 0x7F4A7C15u);
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13; h *= 3266489917u; h ^= h >> 16;
        return (float)(h >> 8) / 16777216.0f;          // 0..1
    };
    VozDeBloque v;
    v.tono   = 0.84f + 0.36f * dado(1);     // de una tercera abajo a una tercera arriba
    v.brillo = 0.78f + 0.44f * dado(2);
    v.cuerpo = 0.70f + 0.60f * dado(3);
    v.peso   = 0.85f + 0.30f * dado(4);
    v.cola   = 0.85f + 0.35f * dado(5);
    return v;
}

enum class GestoBloque { COLOCAR = 0, GOLPEAR = 1, ROMPER = 2 };

class SonidosDeBloque {
public:
    static constexpr int CAPAS = 100;      // por material y por gesto
    static constexpr int ANILLO = 12;      // sonidos compuestos que pueden sonar a la vez

    struct Capa {
        std::vector<float> s;
        int parte = 0;                     // 0 golpe, 1 cuerpo, 2 cola
    };

private:
    struct Rng {
        uint32_t s;
        explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
        uint32_t next() { s = s * 1664525u + 1013904223u; return s; }
        float noise() { return (float)((int32_t)(next() >> 8) - 8388608) / 8388608.0f; }
        float unit() { return (float)(next() >> 8) / 16777216.0f; }
        float range(float a, float b) { return a + (b - a) * unit(); }
    };

    // [material][0 colocar, 1 golpear]
    std::vector<Capa> bancos[(int)StepMaterial::COUNT][2];
    SoundBuffer anillo[ANILLO];
    int siguiente = 0;
    int ultimas[3] = { -1, -1, -1 };       // para no repetir las capas de la vez anterior
    uint32_t dado = 987654321u;

    float rnd01() { dado = dado * 1664525u + 1013904223u; return (float)(dado >> 8) / 16777216.0f; }
    float rnd(float a, float b) { return a + (b - a) * rnd01(); }

public:
    // ------------------------------------------------------------------------
    // UNA CAPA
    // ------------------------------------------------------------------------
    // `parte` reparte el banco en tres: el golpe (el chasquido del contacto),
    // el cuerpo (lo que resuena) y la cola (granos, astillas, polvo que cae).
    static Capa capa(StepMaterial mat, bool golpear, uint32_t seed) {
        Rng rng(seed * 2246822519u + 374761393u);
        auto R = [&](float a, float b) { return rng.range(a, b); };
        Capa c;
        c.parte = (int)(rng.unit() * 3.0f) % 3;

        // Los ingredientes: duracion, envolvente, color del ruido, chasquido,
        // resonancia y granos.
        float durMs = 50, ataqueMs = 1, caida = 50, lp = 0.5f, hp = 0.05f, ruido = 0.6f, chasq = 0.3f;
        float hz = 0, hzAmp = 0, hzCaida = 1.5f, hz2 = 0;
        int granos = 0; float granoMs = 3, granoAmp = 0.3f;

        // Lo que es cada material al chocar con algo.
        //                    cuerpo Hz      brillo   granos  dureza
        float baseHz = 200, brillo = 0.5f, dureza = 0.5f, suelto = 0.0f;
        switch (mat) {
            case StepMaterial::Stone:  baseHz = R(420, 980);  brillo = 0.85f; dureza = 1.00f; suelto = 0.10f; break;
            case StepMaterial::Wood:   baseHz = R(170, 430);  brillo = 0.55f; dureza = 0.70f; suelto = 0.05f; break;
            case StepMaterial::Gravel: baseHz = R(600, 1800); brillo = 0.75f; dureza = 0.65f; suelto = 0.95f; break;
            case StepMaterial::Sand:   baseHz = R(60, 110);   brillo = 0.30f; dureza = 0.15f; suelto = 0.85f; break;
            case StepMaterial::Dirt:   baseHz = R(70, 150);   brillo = 0.25f; dureza = 0.30f; suelto = 0.40f; break;
            case StepMaterial::Grass:  baseHz = R(80, 160);   brillo = 0.50f; dureza = 0.22f; suelto = 0.45f; break;
            case StepMaterial::Leaves: baseHz = R(90, 180);   brillo = 0.70f; dureza = 0.12f; suelto = 0.90f; break;
            case StepMaterial::Snow:   baseHz = R(70, 130);   brillo = 0.38f; dureza = 0.18f; suelto = 0.70f; break;
            default: break;
        }

        if (c.parte == 0) {
            // EL GOLPE: muy corto. Duro = chasquido seco y brillante; blando =
            // un "puf" sordo.
            durMs = R(12, 30) + 25.0f * (1.0f - dureza);
            ataqueMs = 0.3f + 4.0f * (1.0f - dureza);
            caida = R(90, 150) * (0.5f + 0.5f * dureza);
            lp = 0.25f + 0.70f * brillo * R(0.8f, 1.0f);
            hp = 0.02f + 0.25f * dureza * R(0.5f, 1.0f);
            ruido = 0.5f + 0.4f * (1.0f - dureza);
            chasq = (golpear ? 0.9f : 0.55f) * (0.3f + 0.7f * dureza);
            hz = baseHz * R(1.6f, 3.2f); hzAmp = 0.10f * dureza; hzCaida = 3.0f;
        } else if (c.parte == 1) {
            // EL CUERPO: lo que resuena. Al colocar pesa mas (el bloque asienta)
            // y es mas grave; al golpear es mas tenso.
            durMs = R(45, 95) * (golpear ? 0.85f : 1.15f);
            ataqueMs = R(0.5f, 2.5f);
            caida = R(30, 55) * (golpear ? 1.15f : 0.85f);
            lp = 0.12f + 0.45f * brillo;
            hp = 0.004f + 0.03f * dureza;
            ruido = 0.25f + 0.35f * (1.0f - dureza);
            chasq = 0.10f * dureza;
            hz = baseHz * (golpear ? R(0.95f, 1.25f) : R(0.70f, 1.0f));
            hzAmp = 0.20f + 0.45f * dureza;
            hzCaida = R(1.1f, 1.9f);
            hz2 = hz * R(1.45f, 2.75f);                 // un parcial que no es armonico: suena a cosa, no a nota
        } else {
            // LA COLA: lo que se mueve despues. Granos en lo suelto, un roce o
            // un crujido en lo demas.
            durMs = R(40, 110);
            ataqueMs = R(2, 10);
            caida = R(20, 40);
            lp = 0.20f + 0.60f * brillo;
            hp = 0.03f + 0.12f * brillo;
            ruido = 0.20f + 0.50f * suelto;
            chasq = 0.0f;
            granos = (int)(R(2, 8) + 26.0f * suelto);
            granoMs = R(1.5f, 5.0f);
            granoAmp = R(0.25f, 0.65f) * (0.4f + 0.6f * (golpear ? 1.0f : 0.7f));
            if (dureza > 0.6f) { hz = baseHz * R(2.0f, 4.5f); hzAmp = 0.06f; hzCaida = 2.5f; }
        }

        const int sr = AudioMixer::SAMPLE_RATE;
        const int n = (int)(sr * durMs / 1000.0f);
        if (n <= 4) return c;
        c.s.assign((size_t)n, 0.0f);
        if (lp > 0.98f) lp = 0.98f;
        float yLp = 0.0f, yHp = 0.0f;
        const int ataque = (int)(sr * ataqueMs / 1000.0f) + 1;
        const float w = hz > 0.0f ? 6.2831853f * hz / (float)sr : 0.0f;
        const float w2 = hz2 > 0.0f ? 6.2831853f * hz2 / (float)sr : 0.0f;
        const float fase0 = rng.unit() * 6.2831853f;
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr;
            const float env = i < ataque ? (float)i / (float)ataque : expf(-caida * (t - (float)ataque / sr));
            float x = rng.noise();
            yLp += lp * (x - yLp);                      // paso bajo
            yHp += hp * (yLp - yHp);                    // y paso alto (lo que queda al quitar lo lento)
            float v = (yLp - yHp) * env * ruido;
            if (chasq > 0.0f) v += rng.noise() * expf(-420.0f * t) * chasq;
            if (w > 0.0f) v += sinf(w * (float)i + fase0) * expf(-caida * hzCaida * t) * hzAmp;
            if (w2 > 0.0f) v += sinf(w2 * (float)i) * expf(-caida * hzCaida * 1.7f * t) * hzAmp * 0.45f;
            c.s[(size_t)i] = v;
        }
        const int largoGrano = (int)(sr * granoMs / 1000.0f) + 1;
        for (int g = 0; g < granos; ++g) {
            const int ini = (int)(rng.unit() * (float)n * 0.85f);
            const float amp = rng.range(0.3f, 1.0f) * granoAmp;
            float gl = 0.0f;
            const float corte = rng.range(0.15f, 0.45f);
            for (int i = 0; i < largoGrano && ini + i < n; ++i) {
                const float x = rng.noise();
                gl += corte * (x - gl);
                c.s[(size_t)(ini + i)] += (x - gl) * expf(-6.0f * (float)i / (float)largoGrano) * amp;
            }
        }
        // Sin clics al final.
        const int borde = (int)(sr * 0.003f);
        for (int i = 0; i < borde && i < n; ++i) c.s[(size_t)(n - 1 - i)] *= (float)i / (float)borde;
        return c;
    }

    // El banco de un material y un gesto; se genera la primera vez que se pide.
    const std::vector<Capa>& banco(StepMaterial mat, bool golpear) {
        std::vector<Capa>& b = bancos[(int)mat][golpear ? 1 : 0];
        if (b.empty()) {
            b.reserve(CAPAS);
            for (int c = 0; c < CAPAS; ++c)
                b.push_back(capa(mat, golpear, (uint32_t)((int)mat * 7919 + c * 104729 + (golpear ? 50021 : 17))));
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // COMPONER UN SONIDO
    // ------------------------------------------------------------------------
    // Entrelaza un golpe, un cuerpo y una cola del banco (y, al romper, un
    // segundo cuerpo y otra cola), los pasa por la voz del bloque y deja un
    // solo sonido, a un volumen comodo. `fuerza` va de 0 a 1.
    const SoundBuffer* componer(GestoBloque gesto, StepMaterial mat, const VozDeBloque& voz, float fuerza) {
        const bool golpear = gesto != GestoBloque::COLOCAR;
        const std::vector<Capa>& b = banco(mat, golpear);
        if (b.empty()) return nullptr;
        const int sr = AudioMixer::SAMPLE_RATE;
        static thread_local std::vector<float> mezcla;
        mezcla.assign((size_t)(sr * (gesto == GestoBloque::ROMPER ? 0.42f : 0.30f)), 0.0f);

        auto poner = [&](int parte, int hueco, float desdeMs, float hastaMs, float gan, float tonoExtra) {
            // Una capa de esa parte, y que no sea la de la vez anterior.
            int c = (int)(rnd01() * CAPAS) % CAPAS;
            for (int intento = 0; intento < 12 && (b[(size_t)c].parte != parte || c == ultimas[hueco]); ++intento)
                c = (int)(rnd01() * CAPAS) % CAPAS;
            ultimas[hueco] = c;
            const std::vector<float>& s = b[(size_t)c].s;
            if (s.empty()) return;
            const float paso = voz.tono * tonoExtra * rnd(0.95f, 1.05f);
            const float g = gan * rnd(0.70f, 1.0f);
            const size_t ini = (size_t)(sr * rnd(desdeMs, hastaMs) / 1000.0f);
            for (size_t i = 0;; ++i) {
                const float pos = (float)i * paso;
                const size_t j = (size_t)pos;
                if (j + 1 >= s.size() || ini + i >= mezcla.size()) break;
                const float fr = pos - (float)j;
                mezcla[ini + i] += (s[j] + (s[j + 1] - s[j]) * fr) * g;
            }
        };
        const float colaMs = 18.0f * voz.cola;
        poner(0, 0, 0.0f, 2.0f, (0.55f + 0.55f * fuerza) * voz.peso, 1.0f);
        poner(1, 1, 1.0f, 7.0f, 0.75f * voz.cuerpo, 1.0f);
        poner(2, 2, 8.0f, 8.0f + colaMs, (0.35f + 0.35f * fuerza) * voz.cola, 1.0f);
        if (gesto == GestoBloque::ROMPER) {
            // Al romperse: otro cuerpo, mas grave, y mas escombro despues.
            poner(1, 1, 12.0f, 30.0f, 0.60f * voz.cuerpo, 0.78f);
            poner(2, 2, 40.0f, 90.0f, 0.55f * voz.cola, 0.9f);
            poner(2, 2, 90.0f, 170.0f, 0.35f * voz.cola, 1.1f);
        }

        // El brillo del bloque (un paso bajo) y el volumen final.
        float corte = 0.45f + 0.35f * (voz.brillo - 0.78f) / 0.44f;        // de 0,45 a 0,80
        if (mat == StepMaterial::Stone || mat == StepMaterial::Gravel) corte += 0.12f;
        if (corte > 0.95f) corte = 0.95f;
        float y = 0.0f, pico = 1e-6f;
        size_t ultimo = 0;
        for (size_t i = 0; i < mezcla.size(); ++i) {
            y += corte * (mezcla[i] - y);
            mezcla[i] = y;
            const float a = std::fabs(y);
            if (a > pico) pico = a;
            if (a > 1e-4f) ultimo = i;
        }
        const float k = (0.40f + 0.32f * fuerza) / pico;
        const size_t n = mezcla.size() < ultimo + 8 ? mezcla.size() : ultimo + 8;
        SoundBuffer& out = anillo[siguiente];
        siguiente = (siguiente + 1) % ANILLO;
        out.sampleRate = sr;
        out.samples.resize(n);
        const size_t entrada = (size_t)(sr * 0.0008f) + 1, salida = (size_t)(sr * 0.012f) + 1;
        for (size_t i = 0; i < n; ++i) {
            float v = mezcla[i] * k;
            if (i < entrada) v *= (float)i / (float)entrada;
            if (i + salida > n) v *= (float)(n - i) / (float)salida;
            if (v > 1.0f) v = 1.0f;
            if (v < -1.0f) v = -1.0f;
            out.samples[i] = (int16_t)(v * 32000.0f);
        }
        return &out;
    }

    // Componer y sonar. `volumen` es el del gesto (colocar mas flojo que romper).
    void sonar(AudioMixer& mixer, GestoBloque gesto, StepMaterial mat, uint32_t idBloque, float volumen) {
        if (!mixer.isReady()) return;
        const float fuerza = gesto == GestoBloque::ROMPER ? 1.0f : (gesto == GestoBloque::GOLPEAR ? rnd(0.55f, 0.85f) : rnd(0.45f, 0.65f));
        if (const SoundBuffer* b = componer(gesto, mat, vozDeBloque(idBloque), fuerza))
            mixer.play(b, volumen * rnd(0.90f, 1.08f), 1.0f);
    }
};

} // namespace Audio
