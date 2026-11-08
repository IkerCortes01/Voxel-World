#ifndef MOUNTAIN_GENERATOR_H
#define MOUNTAIN_GENERATOR_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

// ============================================================================
// MOUNTAIN GENERATOR - ETAPA 4
// ============================================================================
// RESPONSABILIDAD UNICA: calcular la contribucion vertical de las montanas.
//
// REQUISITO: "Nunca montanas aisladas generadas al azar."
//
// COMO SE CUMPLE:
//   La amplitud de la montana no sale de un ruido independiente, sino del
//   campo de EROSION del clima, que es continuo y de baja frecuencia. Como
//   la erosion forma regiones extensas conectadas, las montanas heredan esa
//   conectividad y aparecen como CORDILLERAS, no como picos sueltos.
//
// COMPONENTES (se suman en capas):
//   1. Ridged multifractal  -> crestas y cordilleras
//   2. Mascara de cordillera-> define donde hay cadena montanosa
//   3. Valles               -> talla los valles entre crestas
//   4. Acantilados          -> terrazas abruptas en zonas de baja erosion
//   5. Laderas suaves       -> transicion gradual al terreno circundante
// ============================================================================

namespace TerrainGen {

class MountainGenerator {
private:
    int seed;

    int seedRidge()   const { return seed + 813971; }
    int seedRange()   const { return seed + 927431; }
    int seedValley()  const { return seed + 1049371; }
    int seedCliff()   const { return seed + 1163741; }

public:
    explicit MountainGenerator(int s) : seed(s) {}

    // ========================================================================
    // CONTRIBUCION DE ALTURA POR MONTANAS
    // ========================================================================
    // Devuelve los bloques a SUMAR a la altura base.
    // Devuelve 0 donde no hay montanas (transicion continua garantizada).
    // ========================================================================
    // MONTANAS Y CORDILLERAS: DESACTIVADAS
    // ========================================================================
    // Levantaban hasta 58 bloques de golpe, y eso rompia la jugabilidad: en
    // un bioma ya cargado aparecian paredes y elevaciones que no encajaban
    // con el terreno de alrededor.
    //
    // En vez de arrancar el generador entero -- que se usa desde varios
    // sitios y arrastraria mas cambios de los necesarios -- se corta en la
    // SALIDA: la funcion devuelve 0 y el terreno queda con el relieve suave
    // del generador base, sin escalones.
    //
    // Todo el calculo de abajo se conserva intacto y comentado. Para
    // recuperar las montanas basta con quitar el `return 0.0f` de aqui.
    //
    // ⭐ REACTIVADAS (pedido: "mejora los biomas montanosos para que sean
    // mas o menos casi comunes"). Vuelven con dos cambios para que no
    // levanten paredes: tope de 120 bloques en vez de 150, y SIN las terrazas
    // de acantilado, que eran justo los escalones verticales.
    static constexpr bool MONTANAS_ACTIVAS = true;

    // Las terrazas de acantilado: apagadas, levantaban paredes.
    static constexpr bool TERRAZAS_ACTIVAS = false;

    float GetMountainHeight(float x, float z, const ClimateData& c) const {
        if (!MONTANAS_ACTIVAS) {
            (void)x; (void)z; (void)c;
            return 0.0f;   // terreno sin cordilleras ni acantilados
        }

        // --------------------------------------------------------------------
        // 1. FACTOR DE MONTANA: cuanta montana corresponde a este punto.
        // --------------------------------------------------------------------
        // Erosion baja = montana. Se usa smoothstep para que el borde de la
        // zona montanosa sea una rampa suave (laderas) y no un escalon.
        // erosion 0.42 -> 0 montana ; erosion 0.10 -> montana plena
        const float erosionFactor = 1.0f - Noise::smoothstep(0.10f, 0.42f, c.erosion);
        if (erosionFactor <= 0.001f) return 0.0f;

        // Solo tierra adentro: evita montanas emergiendo del oceano.
        const float landFactor = Noise::smoothstep(0.50f, 0.62f, c.continentalness);
        if (landFactor <= 0.001f) return 0.0f;

        // --------------------------------------------------------------------
        // 2. MASCARA DE CORDILLERA
        // --------------------------------------------------------------------
        // Ruido ridged de MUY baja frecuencia: define las cadenas como lineas
        // largas y continuas a lo largo del mundo. Esta es la pieza que
        // convierte picos dispersos en cordilleras reales.
        const float rangeMask = Noise::ridged2D(seedRange(),
                                                x * 0.00045f,
                                                z * 0.00045f, 3, 2.0f, 0.5f);

        // Solo hay montana donde la mascara supera un umbral. Debajo, la
        // rampa suave produce las estribaciones (foothills).
        const float rangeFactor = Noise::smoothstep(0.30f, 0.62f, rangeMask);
        if (rangeFactor <= 0.001f) return 0.0f;

        // --------------------------------------------------------------------
        // 3. CRESTAS (ridged multifractal con domain warping)
        // --------------------------------------------------------------------
        // El warp evita que las crestas sean paralelas y regulares; produce
        // los quiebres y curvas de las cordilleras reales.
        float wx, wz;
        Noise::warp2D(seedRidge() + 17, x * 0.0016f, z * 0.0016f, 0.35f, wx, wz);

        // 6 octavas: silueta grande + roca fracturada en la cima.
        const float ridge = Noise::ridged2D(seedRidge(), wx, wz, 6, 2.05f, 0.48f);

        // --------------------------------------------------------------------
        // 4. VALLES
        // --------------------------------------------------------------------
        // Un segundo campo ridged, invertido, que RESTA altura. Talla los
        // valles fluviales y los pasos entre montanas, dando la alternancia
        // cresta-valle que caracteriza una cordillera.
        const float valleyNoise = Noise::ridged2D(seedValley(),
                                                  x * 0.0011f,
                                                  z * 0.0011f, 4, 2.0f, 0.5f);
        // valleyNoise alto = cresta del campo de valles = fondo de valle.
        const float valleyCarve = Noise::smoothstep(0.55f, 1.0f, valleyNoise) * 0.35f;

        // --------------------------------------------------------------------
        // 5. AMPLITUD FINAL
        // --------------------------------------------------------------------
        // Weirdness modula la altura maxima: unas cordilleras son mas altas
        // que otras, evitando que todas lleguen al mismo techo.
        const float peakBoost = 0.75f + c.weirdness * 0.85f; // [0.75, 1.60]

        // ⭐ SUBE DE 58 A 150 CON LA ALTURA DEL MUNDO.
        //
        // Los 58 no eran una decision estetica: eran el maximo que cabia. Con
        // el mundo de 128 y la base continental en ~78, una cordillera de 58
        // llegaba a 136 -- ya por encima del techo -- asi que la compresion de
        // ClampToWorldCeiling la aplastaba contra 124. El efecto visible era
        // que TODAS las cimas altas acababan a la misma cota, planas por
        // arriba, porque la asintota las igualaba.
        //
        // Con el techo en 300 hay sitio de sobra: 78 + 150 = 228, holgado
        // dentro de la compresion. Ahora la altura de una cordillera la decide
        // `peakBoost` (el weirdness del clima) y no el limite del mundo, que es
        // lo que hace que unas sean notablemente mas altas que otras.
        //
        // 150 y no 400: una montana tiene que poder subirse. A 150 bloques
        // sobre el mar, escalar una cumbre es una expedicion larga pero
        // posible; con 400 seria una pared inabarcable y el jugador solo la
        // veria de lejos.
        constexpr float MAX_MOUNTAIN_HEIGHT = 120.0f;

        float h = ridge * MAX_MOUNTAIN_HEIGHT * peakBoost;
        h *= (1.0f - valleyCarve);
        h *= erosionFactor;
        h *= rangeFactor;
        h *= landFactor;

        // --------------------------------------------------------------------
        // 6. ACANTILADOS
        // --------------------------------------------------------------------
        // Cuantiza la altura en terrazas SOLO en las zonas mas escarpadas.
        // Produce las paredes verticales y los escalones rocosos.
        // Se aplica despues de todo lo demas para que los acantilados sigan
        // el relieve y no lo contradigan.
        if (TERRAZAS_ACTIVAS && c.erosion < 0.20f && h > 12.0f) {
            const float cliffNoise = Noise::fbmSimplex2D(seedCliff(),
                                                         x * 0.006f,
                                                         z * 0.006f, 2);
            if (cliffNoise > 0.15f) {
                constexpr float TERRACE = 5.0f;
                const float quantized = floorf(h / TERRACE) * TERRACE;
                // Mezcla parcial: acantilados marcados pero no un zigurat.
                const float cliffStrength = Noise::smoothstep(0.15f, 0.55f, cliffNoise) * 0.6f;
                h = Noise::lerp(h, quantized, cliffStrength);
            }
        }

        return h > 0.0f ? h : 0.0f;
    }

    // ========================================================================
    // ⭐ CERROS, MONTES Y VOLCANES INACTIVOS
    // ========================================================================
    // PEDIDO: "los comunes seran nuevas formas de terreno: montes, cerros y
    // volcanes inactivos".
    //
    // Las cordilleras de arriba son grandes y raras. Esto es el relieve de
    // todos los dias: elevaciones SUELTAS repartidas por el mundo. El suelo
    // se divide en celdas de 230 x 230 bloques y cada una tira un dado:
    //
    //   30 %  nada: llano
    //   36 %  CERRO   una loma redonda, 9 a 22 bloques, 28 a 50 de radio
    //   24 %  MONTE   mas alto y quebrado, 26 a 55 bloques, 55 a 95 de radio,
    //                 con crestas que le rompen la silueta
    //   10 %  VOLCAN  un cono de 45 a 80 bloques, 80 a 115 de radio, laderas
    //                 concavas y un CRATER hundido en la cima. Inactivo: no
    //                 hay lava, solo la forma.
    //
    // El contorno no es un circulo: el radio cambia con el angulo (ruido
    // sobre la circunferencia), asi cada elevacion tiene su planta propia.
    // Donde dos se tocan manda la mas alta (maximo, no suma): no aparecen
    // torres por acumulacion. Solo en tierra firme.
    enum Forma { NADA = 0, CERRO, MONTE, VOLCAN };
    struct Elevacion { int forma; float cx, cz, radio, alto; int semilla; };

    static constexpr float CELDA_RELIEVE = 230.0f;

    static uint32_t mezclar(uint32_t h) {
        h ^= h >> 16; h *= 0x7feb352du;
        h ^= h >> 15; h *= 0x846ca68bu;
        h ^= h >> 16;
        return h;
    }
    static float azar(uint32_t& s) {
        s = mezclar(s + 0x6D2B79F5u);
        return (float)(s >> 8) * (1.0f / 16777216.0f);
    }

    Elevacion ElevacionDeCelda(int i, int j) const {
        uint32_t s = mezclar((uint32_t)i * 0x9E3779B1u ^ mezclar((uint32_t)j * 0x85EBCA77u ^
                             (uint32_t)seed * 0xC2B2AE35u));
        Elevacion e{};
        const float r = azar(s);
        e.forma = r < 0.30f ? NADA : (r < 0.66f ? CERRO : (r < 0.90f ? MONTE : VOLCAN));
        e.cx = ((float)i + 0.2f + 0.6f * azar(s)) * CELDA_RELIEVE;
        e.cz = ((float)j + 0.2f + 0.6f * azar(s)) * CELDA_RELIEVE;
        const float a = azar(s), b = azar(s);
        switch (e.forma) {
            case CERRO:  e.radio = 28.0f + 22.0f * a;  e.alto = 9.0f + 13.0f * b;  break;
            case MONTE:  e.radio = 55.0f + 40.0f * a;  e.alto = 26.0f + 29.0f * b; break;
            case VOLCAN: e.radio = 80.0f + 35.0f * a;  e.alto = 45.0f + 35.0f * b; break;
            default:     e.radio = 0.0f; e.alto = 0.0f; break;
        }
        e.semilla = (int)(s & 0xFFFFF);
        return e;
    }

    // La altura que pone una elevacion en (x, z): 0 fuera de ella.
    float PerfilDe(const Elevacion& e, float x, float z) const {
        if (e.forma == NADA) return 0.0f;
        const float dx = x - e.cx, dz = z - e.cz;
        const float d2 = dx * dx + dz * dz;
        const float rMax = e.radio * 1.25f;
        if (d2 >= rMax * rMax) return 0.0f;
        const float d = std::sqrt(d2);
        // El contorno: el radio varia con el angulo, +-22 %.
        const float ca = d > 1e-3f ? dx / d : 1.0f, sa = d > 1e-3f ? dz / d : 0.0f;
        const float borde = 1.0f + 0.22f * Noise::simplex2D(seed + 4441 + e.semilla,
                                                            ca * 1.6f + (float)(e.semilla % 97),
                                                            sa * 1.6f);
        const float u = d / (e.radio * borde);
        if (u >= 1.0f) return 0.0f;
        constexpr float PI = 3.14159265f;

        if (e.forma == CERRO) {
            // Una loma: coseno, plana en la cima y en el pie.
            const float c = 0.5f + 0.5f * std::cos(PI * u);
            return e.alto * std::pow(c, 1.2f);
        }
        if (e.forma == MONTE) {
            // Base ancha y cima quebrada por crestas.
            const float base = (1.0f - u * u) * (1.0f - u * u);
            const float cresta = Noise::ridged2D(seed + 5557 + e.semilla, x * 0.018f, z * 0.018f, 3, 2.0f, 0.5f);
            return e.alto * base * (0.62f + 0.48f * cresta);
        }
        // VOLCAN: cono de laderas concavas, con barrancos, y un crater.
        const float cono = std::pow(1.0f - u, 1.7f);
        const float barranco = Noise::ridged2D(seed + 6661 + e.semilla, ca * 3.0f + u * 0.5f,
                                               sa * 3.0f + (float)(e.semilla % 53), 2, 2.0f, 0.5f);
        float h = e.alto * cono * (0.90f + 0.10f * barranco);
        // El crater: un cuenco en el 17 % central, con el borde redondeado.
        constexpr float R_CRATER = 0.17f;
        const float bordeCrater = e.alto * std::pow(1.0f - R_CRATER, 1.7f);
        if (u < R_CRATER * 1.35f) {
            const float t = u / R_CRATER;                       // 0 centro, 1 borde
            const float fondo = bordeCrater - e.alto * 0.30f;    // hondo: 30 % del alto
            const float cuenco = fondo + (bordeCrater - fondo) * t * t;
            // Del cuenco al cono, suave alrededor del borde.
            const float m = Noise::smoothstep(0.85f, 1.35f, t);
            h = (t < 1.35f) ? (cuenco * (1.0f - m) + (std::min)(h, bordeCrater) * m) : h;
        }
        return h > 0.0f ? h : 0.0f;
    }

    float GetLandformHeight(float x, float z, const ClimateData& c) const {
        const float landFactor = Noise::smoothstep(0.53f, 0.63f, c.continentalness);
        if (landFactor <= 0.001f) return 0.0f;
        const int ci = (int)std::floor(x / CELDA_RELIEVE);
        const int cj = (int)std::floor(z / CELDA_RELIEVE);
        float h = 0.0f;
        for (int di = -1; di <= 1; ++di)
            for (int dj = -1; dj <= 1; ++dj)
                h = (std::max)(h, PerfilDe(ElevacionDeCelda(ci + di, cj + dj), x, z));
        return h * landFactor;
    }
};

} // namespace TerrainGen

#endif // MOUNTAIN_GENERATOR_H
