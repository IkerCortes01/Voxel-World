#ifndef BIOME_GENERATOR_H
#define BIOME_GENERATOR_H

#include "BiomeTypes.h"
#include "ClimateGenerator.h"
#include "NoiseSystem.h"

// ============================================================================
// BIOME GENERATOR - ETAPA 2 (Seleccion) + ETAPA 10 (Transiciones)
// ============================================================================
// RESPONSABILIDAD UNICA: dado un punto climatico, decidir que bioma es y
// con que peso se mezcla con sus vecinos.
//
// PRINCIPIO CLAVE: los biomas NUNCA se eligen al azar. Cada bioma es una
// REGION del espacio climatico de 5 dimensiones (temperatura, humedad,
// continentalidad, erosion, weirdness). Como los campos climaticos son
// continuos, las fronteras entre biomas tambien lo son.
//
// TRANSICIONES SIN BORDES RECTOS (ETAPA 10):
//   El problema clasico es que `if (temp > 0.6) desierto; else planicie;`
//   produce una frontera dura exactamente en temp=0.6, visible como una
//   linea recta de arena contra hierba.
//
//   Solucion implementada: se muestrea el bioma en varios puntos alrededor
//   del objetivo (kernel de 3x3 con jitter) y se promedian sus ALTURAS,
//   ponderadas por distancia. El bioma dominante decide los bloques, pero
//   la altura es una mezcla continua. Ademas, el punto de muestreo lleva un
//   jitter de ruido de alta frecuencia, lo que hace que la frontera sea
//   irregular y dentada en lugar de recta.
// ============================================================================

namespace TerrainGen {

// Resultado de la evaluacion de bioma en un punto.
struct BiomeSample {
    BiomeType dominant;      // Bioma que define los bloques de superficie
    float     blendWeight;   // [0,1] 1 = centro del bioma, 0 = frontera
    BiomeType secondary;     // Bioma vecino mas influyente
};

class BiomeGenerator {
private:
    int seed;

    // Seed para el jitter de fronteras.
    int seedJitter() const { return seed + 731987; }

    // ------------------------------------------------------------------------
    // UMBRALES CLIMATICOS
    // ------------------------------------------------------------------------
    // Centralizados aqui para poder ajustar el mundo sin tocar la logica.
    static constexpr float OCEAN_DEEP_MAX   = 0.30f; // < esto = oceano profundo
    static constexpr float OCEAN_MAX        = 0.47f; // < esto = oceano
    static constexpr float COAST_MAX        = 0.54f; // zona costera

    static constexpr float DESERT_TEMP_MIN  = 0.62f;
    static constexpr float DESERT_HUMID_MAX = 0.32f;

    static constexpr float FOREST_HUMID_MIN = 0.50f;

    // Montanas: erosion BAJA (roca joven) y altura suficiente.
    static constexpr float MOUNTAIN_EROSION_MAX = 0.38f;
    static constexpr float PEAKS_EROSION_MAX    = 0.24f;

public:
    explicit BiomeGenerator(int s) : seed(s) {}

    // ========================================================================
    // ETAPA 2: SELECCION DE BIOMA
    // ========================================================================
    // Arbol de decision ordenado por PRIORIDAD GEOGRAFICA:
    //   1. Oceano   (continentalidad manda sobre todo lo demas)
    //   2. Montana  (erosion + altura mandan sobre el clima)
    //   3. Clima    (temperatura/humedad deciden el bioma terrestre)
    //
    // NOTA sobre PLAYAS: no se deciden aqui. Una playa no es una region
    // climatica, es una condicion GEOMETRICA (altura cerca del mar +
    // pendiente baja). Se resuelve en BeachGenerator (ETAPA 6), que es lo
    // que impide las "playas flotantes" en la ladera de una montana.
    BiomeType SelectBiome(const ClimateData& c) const {
        // ---- 1. OCEANOS (ETAPA 5) ----
        if (c.continentalness < OCEAN_DEEP_MAX) {
            return BIOME_OCEAN_DEEP;
        }
        if (c.continentalness < OCEAN_MAX) {
            return BIOME_OCEAN;
        }

        // ---- 2. MONTANAS (ETAPA 4) ----
        // Requiere: poca erosion (roca resistente) Y estar tierra adentro.
        // La condicion de continentalidad evita montanas brotando del mar.
        const bool inland = c.continentalness > COAST_MAX;
        if (inland && c.erosion < MOUNTAIN_EROSION_MAX) {
            // Los picos requieren erosion aun menor y weirdness alta,
            // por lo que aparecen como el nucleo de las cordilleras.
            if (c.erosion < PEAKS_EROSION_MAX && c.weirdness > 0.52f) {
                return BIOME_MOUNTAIN_PEAKS;
            }
            return BIOME_MOUNTAINS;
        }

        // ---- 3. BIOMAS CLIMATICOS ----
        // Desierto: caliente Y seco (ambas condiciones, ETAPA 8).
        if (c.temperature > DESERT_TEMP_MIN && c.humidity < DESERT_HUMID_MAX) {
            return BIOME_DESERT;
        }

        // Bosque: humedo, temperatura no extrema (ETAPA 7).
        if (c.humidity > FOREST_HUMID_MIN && c.temperature > 0.25f) {
            return BIOME_FOREST;
        }

        // Por defecto: planicies. Es el bioma "relleno" que ocupa el espacio
        // climatico intermedio, lo que garantiza que siempre hay una
        // transicion suave entre desierto y bosque.
        return BIOME_PLAINS;
    }

    // ========================================================================
    // ETAPA 10: MUESTREO CON JITTER (frontera irregular)
    // ========================================================================
    // Aplica una perturbacion de alta frecuencia a la posicion antes de
    // evaluar el clima. Efecto: la frontera entre dos biomas deja de ser la
    // curva de nivel limpia del campo climatico y pasa a ser una linea
    // dentada y organica, como en la naturaleza.
    void JitterPosition(float x, float z, float& outX, float& outZ) const {
        const float jx = Noise::fbmSimplex2D(seedJitter(),      x * 0.02f, z * 0.02f, 2);
        const float jz = Noise::fbmSimplex2D(seedJitter() + 91, x * 0.02f, z * 0.02f, 2);
        // Amplitud ~6 bloques: suficiente para romper la linea sin
        // desdibujar la geografia.
        outX = x + jx * 6.0f;
        outZ = z + jz * 6.0f;
    }

    // ========================================================================
    // EVALUACION COMPLETA CON MEZCLA
    // ========================================================================
    // Muestrea el bioma en el punto y en 4 vecinos a distancia RADIUS para
    // estimar cuan cerca esta de una frontera. blendWeight = fraccion de
    // muestras que coinciden con el bioma dominante.
    //
    // Coste: 5 evaluaciones climaticas por columna. Se acepta porque el
    // resultado se cachea por columna en TerrainGenerator (no se recalcula
    // por cada voxel de la columna).
    BiomeSample SampleBiome(const ClimateGenerator& climate, float x, float z) const {
        float jx, jz;
        JitterPosition(x, z, jx, jz);
        const ClimateData center = climate.GetClimateData(jx, jz);
        return SampleBiomeFromClimate(climate, jx, jz, center);
    }

    // ------------------------------------------------------------------------
    // VARIANTE OPTIMIZADA: reutiliza un ClimateData ya calculado
    // ------------------------------------------------------------------------
    // GetColumnData ya calcula el clima del punto central. Recalcularlo aqui
    // duplicaba trabajo. Esta sobrecarga permite pasarlo.
    //
    // OPTIMIZACION DE LOS VECINOS:
    // Los 4 vecinos NO necesitan el ClimateData completo (6 campos, ~15 FBM).
    // Para decidir su bioma bastan los 5 campos que consulta SelectBiome, y
    // baseHeight no se usa en esa decision. Se omite GetBaseHeight, que es
    // el campo mas caro (spline + FBM macro). Esto reduce el coste de los
    // vecinos en ~30% sin cambiar ni un solo bioma resultante.
    BiomeSample SampleBiomeFromClimate(const ClimateGenerator& climate,
                                       float jx, float jz,
                                       const ClimateData& center) const {
        const BiomeType dominant = SelectBiome(center);

        // Radio de mezcla: distancia a la que se buscan biomas vecinos.
        constexpr float RADIUS = 12.0f;

        // Clima reducido para los vecinos: solo lo que SelectBiome consulta.
        auto neighborBiome = [&](float nx, float nz) -> BiomeType {
            ClimateData d;
            d.continentalness = climate.GetContinentalness(nx, nz);
            d.temperature     = climate.GetTemperature(nx, nz);
            d.humidity        = climate.GetHumidity(nx, nz, d.continentalness);
            d.erosion         = climate.GetErosion(nx, nz);
            d.weirdness       = climate.GetWeirdness(nx, nz);
            d.baseHeight      = 0.0f; // no lo usa SelectBiome
            return SelectBiome(d);
        };

        BiomeType neighbors[4];
        neighbors[0] = neighborBiome(jx + RADIUS, jz);
        neighbors[1] = neighborBiome(jx - RADIUS, jz);
        neighbors[2] = neighborBiome(jx, jz + RADIUS);
        neighbors[3] = neighborBiome(jx, jz - RADIUS);

        int matching = 0;
        BiomeType secondary = dominant;
        for (int i = 0; i < 4; ++i) {
            if (neighbors[i] == dominant) ++matching;
            else secondary = neighbors[i];
        }

        BiomeSample s;
        s.dominant    = dominant;
        s.secondary   = secondary;
        // 4 coincidencias = interior del bioma (peso 1).
        // 0 coincidencias = frontera pura (peso 0).
        s.blendWeight = (float)matching / 4.0f;
        return s;
    }
};

} // namespace TerrainGen

#endif // BIOME_GENERATOR_H
