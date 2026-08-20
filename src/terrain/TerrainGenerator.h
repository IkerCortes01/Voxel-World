#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"
#include "BiomeGenerator.h"
#include "MountainGenerator.h"
#include "OceanGenerator.h"
#include "BeachGenerator.h"
#include "RiverGenerator.h"
#include "DecorationSystem.h"
#include "BiomeTypes.h"

// ============================================================================
// TERRAIN GENERATOR - ETAPA 3 (Relieve multicapa) + ETAPA 10 (Transiciones)
// ============================================================================
// RESPONSABILIDAD UNICA: componer la ALTURA final del terreno y el bioma
// efectivo de cada columna, combinando los modulos especializados.
//
// ARQUITECTURA MULTICAPA (de mayor a menor escala):
//
//   CAPA 1  Continentes    -> ClimateGenerator (spline de continentalidad)
//   CAPA 2  Macro relieve  -> FBM de baja frecuencia
//   CAPA 3  Montanas       -> MountainGenerator (ridged + cordilleras)
//   CAPA 4  Oceanos        -> OceanGenerator (batimetria)
//   CAPA 5  Colinas        -> FBM de frecuencia media
//   CAPA 6  Micro relieve  -> FBM de frecuencia alta, por bioma
//   CAPA 7  Detalles       -> dunas, rugosidad de superficie
//
// Cada capa se SUMA, y cada una esta modulada por factores continuos
// (erosion, continentalidad, mezcla de biomas). Como todos los moduladores
// son campos continuos, la suma tambien lo es: no hay discontinuidades y
// por tanto no hay bordes rectos entre biomas (ETAPA 10).
//
// CACHE DE COLUMNA:
//   Calcular el clima cuesta ~15 evaluaciones de FBM. Hacerlo por voxel
//   (128 veces por columna) seria inviable. Aqui se calcula UNA VEZ por
//   columna y se reutiliza para los 128 voxeles verticales.
// ============================================================================

namespace TerrainGen {

// ----------------------------------------------------------------------------
// Datos precalculados de una columna del terreno
// ----------------------------------------------------------------------------
struct ColumnData {
    ClimateData climate;
    BiomeType   biome;        // Bioma efectivo (ya resuelta la playa)
    int         surfaceHeight;// Altura del bloque solido mas alto
    float       slope;        // |grad(altura)|, para playas y arboles
    bool        isBeach;
    bool        isOcean;
    float       riverStrength;// [0,1] intensidad del cauce (0 = sin rio)
    bool        isRiverBed;   // true si es lecho de rio (arcilla/arena)
};

class TerrainGenerator {
private:
    int seed;

    ClimateGenerator  climate;
    BiomeGenerator    biomes;
    MountainGenerator mountains;
    OceanGenerator    oceans;
    BeachGenerator    beaches;
    RiverGenerator    rivers;
    DecorationSystem  decor;

    int seedHills()  const { return seed + 2986471; }
    int seedMicro()  const { return seed + 3100227; }
    int seedDetail() const { return seed + 3213983; }

public:
    static constexpr int SEA_LEVEL = ClimateGenerator::SEA_LEVEL;

    explicit TerrainGenerator(int s)
        : seed(s),
          climate(s),
          biomes(s),
          mountains(s),
          oceans(s),
          beaches(s),
          rivers(s),
          decor(s) {}

    // Acceso a los submodulos (los usa ChunkGenerator).
    const ClimateGenerator&  Climate()    const { return climate; }
    const BiomeGenerator&    Biomes()     const { return biomes;  }
    const DecorationSystem&  Decoration() const { return decor;   }
    const RiverGenerator&    Rivers()     const { return rivers;  }

    // ========================================================================
    // ALTURA DEL TERRENO (nucleo del sistema)
    // ========================================================================
    // Combina TODAS las capas de relieve. Funcion pura de (x,z) y la seed.
    //
    // Se separa de GetColumnData porque BeachGenerator necesita evaluar la
    // altura en los puntos vecinos para calcular la pendiente, y hacerlo a
    // traves de la columna completa seria recursivo.
    float GetTerrainHeight(float x, float z, const ClimateData& c) const {
        // ---- CAPA 1: altura base continental (ya incluye la spline) ----
        float height = c.baseHeight;

        // ---- CAPA 4: batimetria oceanica ----
        height += oceans.GetOceanDetail(x, z, c);

        // ---- CAPA 3: montanas ----
        height += mountains.GetMountainHeight(x, z, c);

        // Factor de tierra firme: atenua las capas terrestres bajo el agua,
        // donde el relieve es mas suave.
        const float landFactor = Noise::smoothstep(0.44f, 0.56f, c.continentalness);

        // ---- CAPA 5: COLINAS ----
        // Frecuencia media. Moduladas por erosion: el terreno erosionado es
        // mas plano, el joven mas ondulado.
        // ⭐ COLINAS MAS SUAVES
        //
        // Iban de 11 bloques de amplitud, que junto con las montanas daba
        // desniveles de golpe dentro de un mismo bioma. Bajadas a 4, el
        // terreno sigue ondulando -- no es una mesa de billar -- pero ya no
        // levanta paredes donde no tocan.
        const float hillAmplitude = Noise::lerp(4.0f, 1.5f, c.erosion);
        const float hills = Noise::fbmSimplex2D(seedHills(),
                                                x * 0.0042f,
                                                z * 0.0042f, 4, 2.0f, 0.5f);
        height += hills * hillAmplitude * landFactor;

        // ---- CAPA 6: MICRO RELIEVE ----
        // Ondulacion pequena que rompe las superficies planas. Sin esto, las
        // planicies se ven como una mesa de billar.
        const float micro = Noise::fbmSimplex2D(seedMicro(),
                                                x * 0.019f,
                                                z * 0.019f, 3, 2.0f, 0.5f);
        height += micro * 2.6f * landFactor;

        // ---- CAPA 7: DETALLE FINO ----
        // Rugosidad de 1 bloque. Da textura a la superficie.
        const float detail = Noise::fbmSimplex2D(seedDetail(),
                                                 x * 0.075f,
                                                 z * 0.075f, 2);
        height += detail * 1.1f * landFactor;

        return ClampToWorldCeiling(height);
    }

    // ========================================================================
    // LIMITADOR DE TECHO CON COMPRESION SUAVE
    // ========================================================================
    // El mundo tiene una altura finita (WORLD_HEIGHT). Un clamp duro
    // (`if (h > max) h = max`) decapitaria las montanas dejando mesetas
    // planas y artificiales, todas exactamente a la misma cota.
    //
    // En su lugar se aplica una COMPRESION ASINTOTICA por encima de
    // SOFT_CEILING: la altura sigue creciendo pero cada vez mas despacio,
    // acercandose a HARD_CEILING sin alcanzarlo nunca. Las cimas conservan
    // su forma y su variacion relativa, simplemente se aplastan un poco.
    //
    // Debajo de SOFT_CEILING (la inmensa mayoria del terreno) la funcion es
    // la identidad, por lo que no altera el relieve normal.
    static float ClampToWorldCeiling(float h) {
        constexpr float SOFT_CEILING = 96.0f;  // donde empieza la compresion
        constexpr float HARD_CEILING = 124.0f; // asintota (WORLD_HEIGHT - 4)
        constexpr float FLOOR        = 4.0f;   // sobre la bedrock

        // Suelo duro: la combinacion de fosas oceanicas + taludes puede
        // empujar la altura a valores negativos. Aunque ChunkGenerator la
        // acota antes de escribir, un surfaceHeight negativo se propagaria a
        // GetWaterDepth y a la clasificacion de oceano profundo, dando
        // profundidades incoherentes. Se corta aqui, en el origen.
        if (h < FLOOR) return FLOOR;

        if (h <= SOFT_CEILING) return h;

        const float excess = h - SOFT_CEILING;
        const float range  = HARD_CEILING - SOFT_CEILING;
        // Compresion hiperbolica: excess/(1+excess/range) -> range
        const float compressed = excess / (1.0f + excess / range);
        return SOFT_CEILING + compressed;
    }

    // ========================================================================
    // ALTURA CON DETALLE ESPECIFICO DE BIOMA
    // ========================================================================
    // ETAPA 10: la clave de las transiciones suaves.
    //
    // El detalle propio del bioma (dunas de desierto, rugosidad de bosque)
    // se aplica ESCALADO POR blendWeight. En el centro del bioma se aplica
    // al 100%; en la frontera se desvanece a 0. Resultado: las dunas del
    // desierto se aplanan progresivamente al acercarse a la planicie, en
    // lugar de cortarse de golpe en una linea.
    float ApplyBiomeDetail(float x, float z, float baseHeight,
                           const BiomeSample& sample, const ClimateData& c) const {
        const BiomeDefinition& def = GetBiome(sample.dominant);

        // Bajo el agua no se aplica detalle de bioma terrestre.
        const float landFactor = Noise::smoothstep(0.46f, 0.58f, c.continentalness);
        if (landFactor <= 0.001f) return baseHeight;

        float h = baseHeight;

        // Peso de mezcla: 0 en frontera -> el detalle se desvanece.
        const float w = sample.blendWeight * landFactor;

        // --- Detalle generico del bioma ---
        const float biomeNoise = Noise::fbmSimplex2D(seed + 3327739 + (int)sample.dominant * 131,
                                                     x * def.detailFrequency,
                                                     z * def.detailFrequency, 3);
        h += biomeNoise * def.heightVariation * 0.32f * w;

        // --- ETAPA 8: dunas, exclusivas del desierto ---
        if (sample.dominant == BIOME_DESERT) {
            h += decor.GetDuneHeight(x, z) * w;
        }

        // El detalle de bioma tambien puede empujar contra el techo.
        return ClampToWorldCeiling(h);
    }

    // ========================================================================
    // ALTURA FINAL (incluye bioma) - punto de entrada para el muestreo
    // ========================================================================
    float GetFinalHeight(float x, float z) const {
        const ClimateData c = climate.GetClimateData(x, z);
        const float base = GetTerrainHeight(x, z, c);
        const BiomeSample s = biomes.SampleBiome(climate, x, z);
        float h = ApplyBiomeDetail(x, z, base, s, c);
        // El cauce debe verse tambien aqui: esta funcion alimenta el calculo
        // de pendiente, y sin el rio las orillas parecerian planas.
        h -= rivers.GetRiverCarve(x, z, c);
        return h;
    }

    // ========================================================================
    // DATOS COMPLETOS DE UNA COLUMNA
    // ========================================================================
    // Calcula todo lo necesario para rellenar la columna vertical.
    // ESTA es la funcion que llama ChunkGenerator, una vez por columna.
    ColumnData GetColumnData(float x, float z) const {
        ColumnData col;

        col.climate = climate.GetClimateData(x, z);

        // El jitter de frontera desplaza el punto de evaluacion del bioma.
        // A esa distancia (~6 bloques) los campos climaticos son practicamente
        // identicos, por lo que se reutiliza el clima ya calculado en vez de
        // volver a evaluarlo: ahorra una GetClimateData completa por columna.
        float jx, jz;
        biomes.JitterPosition(x, z, jx, jz);
        const BiomeSample sample =
            biomes.SampleBiomeFromClimate(climate, jx, jz, col.climate);

        // Altura en el punto
        const float baseH = GetTerrainHeight(x, z, col.climate);
        float h = ApplyBiomeDetail(x, z, baseH, sample, col.climate);

        // ---- RIOS: excavar el cauce ----
        // Se resta DESPUES del detalle de bioma para que el rio corte por
        // encima de dunas y rugosidad; si se restara antes, el micro relieve
        // volveria a rellenar el lecho y el cauce quedaria roto.
        col.riverStrength = rivers.GetRiverStrength(x, z, col.climate);
        col.isRiverBed = false;
        if (col.riverStrength > 0.0f) {
            h -= col.riverStrength * RiverGenerator::RIVER_DEPTH;
            col.isRiverBed = RiverGenerator::IsRiverBed(col.riverStrength);
        }

        // --------------------------------------------------------------------
        // PENDIENTE (diferencias finitas centradas)
        // --------------------------------------------------------------------
        // Necesaria para dos cosas criticas:
        //   1. Impedir playas en acantilados (ETAPA 6)
        //   2. Impedir arboles flotando en paredes verticales (ETAPA 7)
        //
        // OPTIMIZACION IMPORTANTE:
        // Se muestrea GetTerrainHeight (relieve base) y NO GetFinalHeight
        // (que ademas evalua bioma y detalle). Motivo:
        //
        //   GetFinalHeight llama a SampleBiome, que a su vez hace 5
        //   evaluaciones climaticas. Usarlo 4 veces costaba 25 evaluaciones
        //   de clima por columna -> 58 us/columna -> ~15 ms por chunk solo
        //   en pendiente. Medido con perfilador.
        //
        // La pendiente solo se usa para umbrales gruesos (es acantilado?
        // es demasiado inclinado para un arbol?), y el detalle de bioma
        // aporta +-2 bloques sobre una escala de decenas. Muestrear el
        // relieve base da la misma decision a 1/5 del coste.
        //
        // Se reutiliza el clima YA calculado en los 4 puntos vecinos, que
        // es una aproximacion valida porque los campos climaticos varian en
        // escalas de cientos de bloques y aqui D=2.
        constexpr float D = 2.0f; // distancia de muestreo
        const float hxm = GetTerrainHeight(x - D, z, col.climate);
        const float hxp = GetTerrainHeight(x + D, z, col.climate);
        const float hzm = GetTerrainHeight(x, z - D, col.climate);
        const float hzp = GetTerrainHeight(x, z + D, col.climate);

        col.slope = BeachGenerator::ComputeSlope(hxm, hxp, hzm, hzp, D);

        col.surfaceHeight = (int)floorf(h);

        // --------------------------------------------------------------------
        // RESOLUCION DEL BIOMA EFECTIVO
        // --------------------------------------------------------------------
        BiomeType effective = sample.dominant;

        col.isOcean = (col.surfaceHeight < SEA_LEVEL);

        // ETAPA 6: la playa se decide GEOMETRICAMENTE, no por clima.
        // Solo se evalua si el bioma base no es ya oceano profundo ni montana:
        // esto refuerza que nunca se corte una montana con arena.
        col.isBeach = false;
        if (effective != BIOME_MOUNTAINS &&
            effective != BIOME_MOUNTAIN_PEAKS &&
            effective != BIOME_OCEAN_DEEP) {

            if (beaches.IsBeach(x, z, SEA_LEVEL, col.surfaceHeight, col.slope)) {
                effective = BIOME_BEACH;
                col.isBeach = true;
            }
        }

        // Coherencia: si el terreno quedo bajo el agua pero el bioma es
        // terrestre, se reclasifica como oceano. Evita "bosques sumergidos".
        if (col.isOcean && !col.isBeach && !IsOceanBiome(effective)) {
            const int depth = SEA_LEVEL - col.surfaceHeight;
            effective = (depth > 14) ? BIOME_OCEAN_DEEP : BIOME_OCEAN;
        }

        col.biome = effective;
        return col;
    }
};

} // namespace TerrainGen

#endif // TERRAIN_GENERATOR_H
