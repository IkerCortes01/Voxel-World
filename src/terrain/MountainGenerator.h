#ifndef MOUNTAIN_GENERATOR_H
#define MOUNTAIN_GENERATOR_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"

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
    static constexpr bool MONTANAS_ACTIVAS = false;

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

        constexpr float MAX_MOUNTAIN_HEIGHT = 58.0f;

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
        if (c.erosion < 0.20f && h > 12.0f) {
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
};

} // namespace TerrainGen

#endif // MOUNTAIN_GENERATOR_H
