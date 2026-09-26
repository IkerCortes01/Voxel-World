#ifndef AMBIENTE_SYNTH_H
#define AMBIENTE_SYNTH_H

#include "../../src/audio/AudioMixer.h"

#include <cstdint>
#include <cmath>
#include <vector>

// ============================================================================
// SONIDOS AMBIENTALES - Sintesis procedural
// ============================================================================
// RESPONSABILIDAD UNICA: generar los SoundBuffer del ambiente. No decide
// cuando suenan (eso es de AmbienteMundo) ni los reproduce (eso es del mixer).
//
// ----------------------------------------------------------------------------
// POR QUE SINTESIS Y NO ARCHIVOS
// ----------------------------------------------------------------------------
// Es la misma decision que ya tomo FootstepSynth.h para las pisadas, y por las
// mismas razones:
//
//   - El proyecto no trae archivos de audio (sounds/ solo tiene tres mp3 de
//     musica). Un sistema que dependa de archivos que no existen no suena.
//   - Permite N variaciones sin N archivos: cada grillo, cada rafaga y cada
//     gota se generan con una semilla distinta y no se repiten.
//   - Ocupa unos pocos KB de RAM en vez de megabytes.
//   - No anade dependencias ni descargas.
//
// Y una razon propia del ambiente: estos sonidos son LARGOS y se repiten sin
// parar. Un bucle de viento de 4 segundos en disco canta enseguida; generado,
// se puede rehacer con otra semilla cada vez que termina.
//
// ----------------------------------------------------------------------------
// MODELO ACUSTICO
// ----------------------------------------------------------------------------
// Cada ambiente es ruido conformado. Lo que los distingue es el ESPECTRO y la
// ENVOLVENTE, igual que en las pisadas:
//
//   AMBIENTE      FUENTE                  CARACTER
//   ------------  ----------------------  ----------------------------------
//   Viento        ruido paso-bajo lento   soplo grave que sube y baja
//   Hojas         ruido paso-alto         susurro seco, agudo, irregular
//   Grillos       pulsos tonales ~4 kHz   chirrido ritmico, nocturno
//   Pajaros       barridos de tono        silbo corto que sube o baja
//   Cueva: gota   resonador amortiguado   "ploc" con cola de reverberacion
//   Cueva: fondo  ruido muy grave         retumbo continuo, casi infrasonico
//   Buho          tono grave con vibrato  ulular de dos notas
//   Coyote        barrido largo + armonic aullido lejano
//
// Tecnicas: ruido blanco con LCG determinista, filtros de 1 polo (paso-alto y
// paso-bajo), envolventes exponenciales, resonadores de 2 polos para el
// cuerpo tonal, y vibrato por modulacion de fase.
//
// DETERMINISMO: el ruido es un LCG con semilla explicita. Dos llamadas con la
// misma semilla dan exactamente el mismo sonido, asi que el ambiente de un
// sitio se puede reproducir y depurar.
// ============================================================================

namespace Audio {

// Los ambientes que el mundo sabe pedir.
enum class Ambiente {
    VientoSuave,      // brisa de llanura
    VientoFuerte,     // racha de montana o de pico
    Hojas,            // follaje moviendose, bosque
    Grillos,          // noche en pastizal
    PajaroDia,        // canto suelto, bosque de dia
    PajaroLejano,     // el mismo, mas apagado y con cola
    GotaCueva,        // agua filtrandose bajo tierra
    RetumboCueva,     // el fondo grave de una galeria
    Buho,             // noche en bosque
    Coyote,           // noche en zona seca
    Chicharra,        // dia caluroso, semiarido
    COUNT
};

class AmbienteSynth {
private:
    // ---- Generador de ruido determinista (LCG) ----
    // Identico al de FootstepSynth: mismo algoritmo, mismas garantias. No se
    // comparte por no acoplar los dos modulos, que es la razon por la que
    // FootstepSynth tampoco lo exporta.
    struct Rng {
        uint32_t s;
        explicit Rng(uint32_t seed) : s(seed ? seed : 1u) {}
        uint32_t next() {
            s = s * 1664525u + 1013904223u;
            return s;
        }
        // Ruido blanco en [-1, 1].
        float blanco() {
            return (float)(next() >> 8) * (1.0f / 8388608.0f) - 1.0f;
        }
        // Uniforme en [a, b].
        float rango(float a, float b) {
            return a + (b - a) * ((float)(next() >> 8) * (1.0f / 16777216.0f));
        }
    };

    // ---- Filtro paso-bajo de 1 polo ----
    // y[n] = y[n-1] + k * (x[n] - y[n-1]).  k pequeno = mas grave.
    struct PasoBajo {
        float y = 0.0f, k;
        explicit PasoBajo(float corte) : k(corte) {}
        float operator()(float x) { y += k * (x - y); return y; }
    };

    // ---- Filtro paso-alto de 1 polo ----
    // Lo que queda al restarle a la senal su propia version grave.
    struct PasoAlto {
        PasoBajo bajo;
        explicit PasoAlto(float corte) : bajo(corte) {}
        float operator()(float x) { return x - bajo(x); }
    };

    // ---- Resonador de 2 polos ----
    // Da CUERPO TONAL: una campana, una gota, el eco de una galeria. Es un
    // oscilador amortiguado excitado por la entrada.
    struct Resonador {
        float y1 = 0.0f, y2 = 0.0f;
        float a1, a2;
        Resonador(float freq, float q, int sr) {
            const float w = 6.28318531f * freq / (float)sr;
            const float r = std::exp(-w / (2.0f * q));
            a1 = 2.0f * r * std::cos(w);
            a2 = -r * r;
        }
        float operator()(float x) {
            const float y = x + a1 * y1 + a2 * y2;
            y2 = y1; y1 = y;
            return y;
        }
    };

    static int sr() { return AudioMixer::SAMPLE_RATE; }

    // Convierte float [-1,1] a int16 con recorte duro. El recorte es
    // deliberado: es preferible a que una suma se desborde y suene un chasquido.
    static int16_t aPCM(float v) {
        if (v >  1.0f) v =  1.0f;
        if (v < -1.0f) v = -1.0f;
        return (int16_t)(v * 32000.0f);
    }

public:
    // ------------------------------------------------------------------------
    // VIENTO
    // ------------------------------------------------------------------------
    // Ruido paso-bajo con la amplitud modulada muy despacio. Las rachas son lo
    // que lo hace viento y no un siseo: sin ellas suena a television sin senal.
    //
    // `fuerza` 0..1 abre el filtro y sube la modulacion: una brisa es grave y
    // regular, una racha de pico es mas ancha y mas viva.
    static SoundBuffer viento(float segundos, float fuerza, uint32_t semilla) {
        SoundBuffer b;
        const int n = (int)(segundos * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        Rng rng(semilla);
        PasoBajo grave(0.004f + 0.020f * fuerza);
        PasoBajo suavizado(0.0006f);     // la envolvente de las rachas

        // Dos moduladores lentos e inconmensurables: asi el patron no se
        // repite dentro del bucle y no se oye la costura.
        const float f1 = 0.07f + 0.05f * fuerza;
        const float f2 = 0.031f;

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();
            const float racha =
                0.55f + 0.45f * (0.6f * std::sin(6.28318531f * f1 * t) +
                                 0.4f * std::sin(6.28318531f * f2 * t + 1.7f));

            const float env = suavizado(racha);
            float v = grave(rng.blanco()) * env * (0.25f + 0.45f * fuerza);

            // Desvanecido en los extremos para que el bucle no chasquee.
            v *= bordes(i, n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // HOJAS
    // ------------------------------------------------------------------------
    // Paso-ALTO en vez de paso-bajo: el follaje es seco y agudo. Y la
    // modulacion va mucho mas rapida que en el viento, porque lo que se oye
    // son hojas sueltas golpeandose, no una masa de aire.
    static SoundBuffer hojas(float segundos, uint32_t semilla) {
        SoundBuffer b;
        const int n = (int)(segundos * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        Rng rng(semilla);
        PasoAlto agudo(0.35f);
        PasoBajo tope(0.30f);            // corta el siseo mas chillon
        PasoBajo env(0.004f);

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();
            const float mov = 0.4f + 0.6f *
                std::fabs(std::sin(6.28318531f * 0.23f * t) *
                          std::sin(6.28318531f * 0.09f * t + 0.9f));

            float v = tope(agudo(rng.blanco())) * env(mov) * 0.30f;
            v *= bordes(i, n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // GRILLOS
    // ------------------------------------------------------------------------
    // Un grillo real no es un tono puro: es un PULSO repetido muy deprisa
    // (~4 kHz de portadora, troceada a ~30 Hz). De ahi que se oiga como un
    // chirrido y no como un pitido.
    //
    // Se superponen varios con frecuencias y fases distintas, que es lo que
    // convierte un grillo en un campo de grillos.
    static SoundBuffer grillos(float segundos, uint32_t semilla) {
        SoundBuffer b;
        const int n = (int)(segundos * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        Rng rng(semilla);
        constexpr int CUANTOS = 5;
        float freq[CUANTOS], ritmo[CUANTOS], fase[CUANTOS], vol[CUANTOS];
        for (int g = 0; g < CUANTOS; ++g) {
            freq[g]  = rng.rango(3600.0f, 4800.0f);
            ritmo[g] = rng.rango(24.0f, 34.0f);
            fase[g]  = rng.rango(0.0f, 6.28f);
            vol[g]   = rng.rango(0.06f, 0.16f);
        }

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();
            float v = 0.0f;
            for (int g = 0; g < CUANTOS; ++g) {
                // La puerta del pulso: solo suena en la primera mitad de cada
                // ciclo del ritmo. Es lo que da el "cri-cri".
                const float ciclo = std::fmod(t * ritmo[g] + fase[g], 1.0f);
                if (ciclo > 0.45f) continue;
                const float env = std::sin(3.14159265f * ciclo / 0.45f);
                v += std::sin(6.28318531f * freq[g] * t) * env * vol[g];
            }
            v *= bordes(i, n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // CHICHARRA
    // ------------------------------------------------------------------------
    // La cigarra del semiarido: mas grave y MUCHO mas continua que el grillo,
    // con un zumbido que sube y se sostiene. Es el sonido del mediodia seco.
    static SoundBuffer chicharra(float segundos, uint32_t semilla) {
        SoundBuffer b;
        const int n = (int)(segundos * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        Rng rng(semilla);
        const float base = rng.rango(2100.0f, 2600.0f);
        PasoBajo suav(0.02f);

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();

            // Arranca, se sostiene y cae: el ciclo de una chicharra.
            float env;
            const float T = segundos;
            if (t < T * 0.15f)      env = t / (T * 0.15f);
            else if (t > T * 0.75f) env = (T - t) / (T * 0.25f);
            else                    env = 1.0f;

            // Troceado rapido: es lo que la hace aspera en vez de un pitido.
            const float grano = 0.7f + 0.3f * suav(rng.blanco());
            const float mod = 1.0f + 0.02f * std::sin(6.28318531f * 7.0f * t);

            float v = std::sin(6.28318531f * base * mod * t) * env * grano * 0.16f;
            v += std::sin(6.28318531f * base * 2.0f * mod * t) * env * 0.05f;
            v *= bordes(i, n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // PAJARO
    // ------------------------------------------------------------------------
    // Un silbo es un BARRIDO de frecuencia, no un tono fijo. `subiendo` decide
    // si el canto sube o baja, que es lo que distingue una especie de otra al
    // oido sin tener que modelar ninguna.
    //
    // `lejania` 0..1 lo apaga y le anade cola: un pajaro lejos se oye filtrado
    // por el aire y con algo de reverberacion del terreno.
    static SoundBuffer pajaro(bool subiendo, float lejania, uint32_t semilla) {
        SoundBuffer b;
        Rng rng(semilla);

        const float dur = rng.rango(0.18f, 0.40f);
        const int n = (int)(dur * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        const float f0 = rng.rango(1800.0f, 2600.0f);
        const float f1 = subiendo ? f0 * rng.rango(1.5f, 2.2f)
                                  : f0 * rng.rango(0.45f, 0.7f);

        PasoBajo aire(0.5f - 0.42f * lejania);   // el aire se come los agudos
        Resonador cola(f0, 3.0f + 8.0f * lejania, sr());

        float fase = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)n;

            // Barrido de frecuencia, y vibrato encima: un silbo real tiembla.
            const float f = f0 + (f1 - f0) * t;
            const float vib = 1.0f + 0.03f *
                std::sin(6.28318531f * 18.0f * (float)i / (float)sr());
            fase += 6.28318531f * f * vib / (float)sr();

            // Ataque rapido, caida suave.
            const float env = (t < 0.12f) ? (t / 0.12f)
                                          : std::exp(-3.2f * (t - 0.12f));

            float v = std::sin(fase) * env * (0.22f - 0.13f * lejania);
            v = aire(v);
            if (lejania > 0.3f) v += cola(v * 0.10f) * 0.25f;

            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // BUHO
    // ------------------------------------------------------------------------
    // Grave, con vibrato lento y en DOS notas: el patron "uh-uuuh" que es lo
    // que hace reconocible al animal. Sin la segunda nota suena a soplo.
    static SoundBuffer buho(uint32_t semilla) {
        SoundBuffer b;
        Rng rng(semilla);

        const float dur = 1.15f;
        const int n = (int)(dur * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        const float f = rng.rango(330.0f, 420.0f);
        PasoBajo suave(0.25f);

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();

            // Dos notas: una corta y otra larga, separadas por silencio.
            float env = 0.0f;
            if (t < 0.22f)                  env = std::sin(3.14159265f * t / 0.22f);
            else if (t > 0.40f && t < 0.95f) env = std::sin(3.14159265f * (t - 0.40f) / 0.55f);
            if (env <= 0.0f) { b.samples[i] = 0; continue; }

            const float vib = 1.0f + 0.018f * std::sin(6.28318531f * 5.5f * t);
            float v = std::sin(6.28318531f * f * vib * t) * env * 0.26f;
            // Un armonico debil da cuerpo sin quitarle lo aterciopelado.
            v += std::sin(6.28318531f * f * 2.0f * vib * t) * env * 0.05f;
            b.samples[i] = aPCM(suave(v));
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // COYOTE
    // ------------------------------------------------------------------------
    // Aullido: barrido largo que sube, se sostiene y cae, con armonicos. Se
    // deja apagado a proposito -- se oye de lejos, que es como se oye de
    // verdad en el semiarido.
    static SoundBuffer coyote(uint32_t semilla) {
        SoundBuffer b;
        Rng rng(semilla);

        const float dur = rng.rango(1.3f, 2.0f);
        const int n = (int)(dur * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        const float f0 = rng.rango(420.0f, 560.0f);
        PasoBajo aire(0.18f);

        float fase = 0.0f;
        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)n;

            // Sube deprisa, se mantiene, baja despacio.
            float f;
            if (t < 0.18f)      f = f0 * (0.6f + 0.4f * (t / 0.18f));
            else if (t < 0.65f) f = f0 * (1.0f + 0.05f * std::sin(6.28318531f * 3.0f * t));
            else                f = f0 * (1.0f - 0.35f * ((t - 0.65f) / 0.35f));

            fase += 6.28318531f * f / (float)sr();

            const float env = (t < 0.10f) ? (t / 0.10f)
                            : (t > 0.70f) ? ((1.0f - t) / 0.30f) : 1.0f;

            float v = std::sin(fase) * env * 0.20f;
            v += std::sin(fase * 2.0f) * env * 0.07f;
            v += std::sin(fase * 3.0f) * env * 0.03f;
            b.samples[i] = aPCM(aire(v));
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // GOTA DE CUEVA
    // ------------------------------------------------------------------------
    // El "ploc" es un impulso muy corto metido en un resonador de Q alto: el
    // resonador es lo que hace la COLA, y la cola es lo que dice "esto es una
    // cueva" y no un grifo.
    static SoundBuffer gotaCueva(uint32_t semilla) {
        SoundBuffer b;
        Rng rng(semilla);

        const float dur = 0.85f;
        const int n = (int)(dur * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        const float f = rng.rango(900.0f, 1600.0f);
        Resonador cuerpo(f, 22.0f, sr());
        Resonador eco(f * 0.5f, 40.0f, sr());   // la galeria

        for (int i = 0; i < n; ++i) {
            // Impulso: solo los primeros milisegundos tienen energia.
            const float imp = (i < 12) ? rng.blanco() * (1.0f - i / 12.0f) : 0.0f;

            float v = cuerpo(imp) * 0.30f;
            v += eco(imp * 0.5f) * 0.18f;

            // Caida exponencial: la cola se apaga en el aire humedo.
            v *= std::exp(-4.5f * (float)i / (float)n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

    // ------------------------------------------------------------------------
    // RETUMBO DE CUEVA
    // ------------------------------------------------------------------------
    // Ruido MUY grave y continuo. Casi no se oye como sonido: se siente como
    // presion. Es lo que hace que una galeria se note distinta del exterior
    // aunque no pase nada.
    static SoundBuffer retumboCueva(float segundos, uint32_t semilla) {
        SoundBuffer b;
        const int n = (int)(segundos * sr());
        b.samples.resize(n);
        b.sampleRate = sr();

        Rng rng(semilla);
        PasoBajo muyGrave(0.0018f);
        PasoBajo grave2(0.004f);
        PasoBajo env(0.0008f);

        for (int i = 0; i < n; ++i) {
            const float t = (float)i / (float)sr();
            const float lento = 0.6f + 0.4f * std::sin(6.28318531f * 0.043f * t);

            float v = grave2(muyGrave(rng.blanco())) * env(lento) * 2.2f;
            v *= bordes(i, n);
            b.samples[i] = aPCM(v);
        }
        return b;
    }

private:
    // Desvanecido en los extremos del bucle.
    //
    // ⭐ ES LO QUE EVITA EL CHASQUIDO. Un bucle que empieza y acaba en valores
    // distintos produce un salto brusco en la senal, y eso se oye como un
    // "clic" en cada vuelta -- el defecto mas reconocible de un ambiente mal
    // montado. Con 60 ms de rampa a cada lado el empalme es inaudible.
    static float bordes(int i, int n) {
        const int rampa = (int)(0.060f * AudioMixer::SAMPLE_RATE);
        if (rampa <= 0 || n < rampa * 2) return 1.0f;
        if (i < rampa)         return (float)i / (float)rampa;
        if (i >= n - rampa)    return (float)(n - 1 - i) / (float)rampa;
        return 1.0f;
    }
};

} // namespace Audio

#endif // AMBIENTE_SYNTH_H
