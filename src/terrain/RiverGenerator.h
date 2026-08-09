#ifndef RIVER_GENERATOR_H
#define RIVER_GENERATOR_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"

// ============================================================================
// RIVER GENERATOR - Ríos continentales
// ============================================================================
// RESPONSABILIDAD UNICA: decidir si una columna pertenece al cauce de un río
// y cuánto hay que excavar.
//
// ----------------------------------------------------------------------------
// ALGORITMO: LA LINEA CERO DE UN CAMPO DE RUIDO
// ----------------------------------------------------------------------------
// Un río es una CURVA, no una mancha. Umbralizar ruido (`if n > k`) produce
// regiones, que darían lagos alargados, no cauces.
//
// La técnica correcta es tomar el conjunto de nivel CERO de un campo continuo:
//
//     |noise(x,z)| < w   ->  estás dentro del cauce
//
// El conjunto donde un campo suave vale ~0 es una curva de grosor 2w que
// serpentea por todo el mundo y NUNCA se corta: exactamente la topología de
// un río. Cuanto más cerca de cero, más cerca del centro del cauce, lo que
// además da gratis el perfil en V (más profundo en el medio).
//
// Se aplica domain warping para que el trazado no sea una sinusoide regular
// sino que tenga meandros y recodos.
//
// ----------------------------------------------------------------------------
// POR QUE LOS RIOS NO LLEGAN AL MAR NI A LA MONTAÑA ALTA
// ----------------------------------------------------------------------------
// El cauce se desvanece por dos factores continuos:
//   - continentalidad baja (cerca del océano): el río se funde con el mar en
//     vez de cortarlo con un canal artificial.
//   - altitud muy alta: no se excavan cañones en las cumbres.
// Ambos factores son suaves, así que el río se estrecha gradualmente en vez
// de terminar de golpe.
// ============================================================================

namespace TerrainGen {

class RiverGenerator {
private:
    int seed;

    int seedMain()   const { return seed + 3529471; }
    int seedWarp()   const { return seed + 3643229; }
    int seedWidth()  const { return seed + 3756987; }
    int seedBranch() const { return seed + 3870745; }

public:
    // ---- PARAMETROS AJUSTABLES ----

    // Frecuencia del campo. MAYOR = ríos MAS JUNTOS (más comunes).
    // Con 0.00140 los cauces se cruzan cada ~500-700 bloques: son frecuentes
    // de encontrar explorando, pero sin saturar el mapa.
    static constexpr float RIVER_SCALE = 0.00140f;

    // Semianchura del cauce en unidades del campo de ruido.
    // Subirlo ensancha los ríos Y los hace más frecuentes (más área cumple
    // la condición), así que es la palanca principal de "cuántos ríos hay".
    static constexpr float RIVER_WIDTH = 0.018f;

    // Segunda red de ríos, con otra escala y seed: se cruza con la primera
    // y multiplica la densidad de cauces sin que se vean repetitivos.
    static constexpr float BRANCH_SCALE = 0.00230f;
    static constexpr float BRANCH_WIDTH = 0.013f;

    // Profundidad máxima de excavación, en bloques.
    static constexpr float RIVER_DEPTH = 5.0f;

    explicit RiverGenerator(int s) : seed(s) {}

    // ========================================================================
    // INTENSIDAD DE RIO EN UN PUNTO -> [0, 1]
    // ========================================================================
    // 0   = sin río
    // 1   = centro del cauce (máxima profundidad)
    // Los valores intermedios forman las orillas, lo que da el perfil en V.
    float GetRiverStrength(float x, float z, const ClimateData& c) const {
        // ---- Factores de supresión ----
        // Cerca del océano el río se disuelve en el mar.
        const float landFactor = Noise::smoothstep(0.46f, 0.58f, c.continentalness);
        if (landFactor <= 0.001f) return 0.0f;

        // En cotas muy altas no hay cauces (nacen más abajo).
        const float altitudeFactor =
            1.0f - Noise::smoothstep(88.0f, 104.0f, c.baseHeight);
        if (altitudeFactor <= 0.001f) return 0.0f;

        // ---- Trazado principal ----
        // Domain warping: sin él, la curva cero sería demasiado regular y los
        // ríos parecerían trazados con regla.
        float wx, wz;
        Noise::warp2D(seedWarp(),
                      x * RIVER_SCALE,
                      z * RIVER_SCALE,
                      0.55f, wx, wz);

        const float field = Noise::fbmSimplex2D(seedMain(), wx, wz, 3, 2.0f, 0.5f);

        // Anchura variable: el río se ensancha y estrecha a lo largo del cauce.
        const float widthMod = Noise::toUnit(
            Noise::fbmSimplex2D(seedWidth(), x * 0.0009f, z * 0.0009f, 2));
        const float halfWidth = RIVER_WIDTH * (0.65f + widthMod * 0.8f);

        // |field| < halfWidth -> dentro del cauce.
        const float d = fabsf(field);
        float strength = 0.0f;
        if (d < halfWidth) {
            // 1 en el centro, 0 en la orilla. Curva suave para el perfil en V.
            strength = 1.0f - (d / halfWidth);
            strength = strength * strength * (3.0f - 2.0f * strength);
        }

        // ---- Segunda red (afluentes) ----
        // Otra escala y otro seed: se cruzan con la principal formando
        // confluencias y aumentando la densidad total de ríos.
        float bx, bz;
        Noise::warp2D(seedBranch() + 11,
                      x * BRANCH_SCALE,
                      z * BRANCH_SCALE,
                      0.45f, bx, bz);
        const float branch = Noise::fbmSimplex2D(seedBranch(), bx, bz, 3, 2.0f, 0.5f);

        const float bd = fabsf(branch);
        if (bd < BRANCH_WIDTH) {
            float bs = 1.0f - (bd / BRANCH_WIDTH);
            bs = bs * bs * (3.0f - 2.0f * bs);
            // Los afluentes son algo menos profundos que el cauce principal.
            bs *= 0.8f;
            if (bs > strength) strength = bs;
        }

        return strength * landFactor * altitudeFactor;
    }

    // ========================================================================
    // EXCAVACION: bloques a RESTAR de la altura del terreno
    // ========================================================================
    float GetRiverCarve(float x, float z, const ClimateData& c) const {
        const float s = GetRiverStrength(x, z, c);
        if (s <= 0.0f) return 0.0f;
        return s * RIVER_DEPTH;
    }

    // Un punto es lecho de río si la intensidad supera este umbral.
    // Se usa para decidir dónde poner arcilla y arena.
    static bool IsRiverBed(float strength) {
        return strength > 0.35f;
    }
};

} // namespace TerrainGen

#endif // RIVER_GENERATOR_H
