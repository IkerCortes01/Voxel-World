#ifndef WORLD_GENERATOR_AAA_H
#define WORLD_GENERATOR_AAA_H

#include "NoiseSystem.h"
#include "ClimateGenerator.h"
#include "BiomeGenerator.h"
#include "TerrainGenerator.h"
#include "ChunkGenerator.h"
#include "CaveGenerator.h"
#include "DecorationSystem.h"
#include "BiomeTypes.h"

// ============================================================================
// WORLD GENERATOR AAA - FACHADA PUBLICA (ETAPA 14)
// ============================================================================
// Punto de entrada unico del sistema de generacion. El motor solo habla con
// esta clase; toda la complejidad queda encapsulada detras.
//
// PIPELINE COMPLETO:
//
//   NoiseSystem        Primitivas: Perlin, Simplex, Cellular, FBM, Ridged,
//                      Billow, Domain Warping, Splines
//        |
//   ClimateGenerator   ETAPA 1: 6 mapas climaticos independientes
//        |
//   BiomeGenerator     ETAPA 2 + 10: seleccion y mezcla de biomas
//        |
//   TerrainGenerator   ETAPA 3: relieve multicapa
//        |-- MountainGenerator  ETAPA 4: cordilleras, valles, acantilados
//        |-- OceanGenerator     ETAPA 5: batimetria
//        |-- BeachGenerator     ETAPA 6: playas por geometria
//        |-- DecorationSystem   ETAPA 7/8: arboles Poisson, dunas, minerales
//        |
//   CaveGenerator      ETAPA 9: cuevas multi-algoritmo
//        |
//   ChunkGenerator     ETAPA 11/12: voxelizacion segura y determinista
//        |
//   WorldGeneratorAAA  <- esta clase
//
// ----------------------------------------------------------------------------
// COMPATIBILIDAD CON EL MOTOR
// ----------------------------------------------------------------------------
// GenerateColumn() mantiene una firma parecida a la anterior para minimizar
// los cambios en main.cpp, pero con dos diferencias CRITICAS que corrigen el
// desbordamiento de memoria de la version previa:
//
//   1. La altura del array es un parametro de plantilla deducido del propio
//      array, no la constante 256 hardcodeada. El compilador verifica que
//      coincide con el buffer real.
//   2. El tipo de bloque es un parametro de plantilla, por lo que funciona
//      con BlockType (enum) sin necesidad de casts inseguros.
//
// Si el motor pasa un array de [16][128][16] de BlockType, TODOS los bucles
// quedan acotados a 128 automaticamente.
// ============================================================================

namespace TerrainGen {

class WorldGeneratorAAA {
private:
    int seed;
    ChunkGenerator chunkGen;

public:
    static constexpr int SEA_LEVEL = ChunkGenerator::SEA_LEVEL;

    explicit WorldGeneratorAAA(int s) : seed(s), chunkGen(s) {}

    int GetSeed() const { return seed; }

    // ========================================================================
    // GENERAR COLUMNA (API principal para el motor)
    // ========================================================================
    // TEMPLATE sobre el tipo de bloque y la altura del chunk: el compilador
    // deduce ambos del array que se le pasa, eliminando por construccion
    // cualquier discrepancia de dimensiones.
    //
    // Uso desde main.cpp:
    //     worldGen->GenerateColumn(worldX, worldZ, chunk->blocks, x, z);
    //
    // donde chunk->blocks es BlockType[16][128][16]. SY se deduce como 128.
    template <typename BlockT, int SY, int SZ>
    ColumnData GenerateColumn(int worldX, int worldZ,
                              BlockT (&chunkBlocks)[16][SY][SZ],
                              int localX, int localZ) const {
        ArrayChunkWriter<BlockT, 16, SY, SZ> writer(chunkBlocks);
        return chunkGen.GenerateColumn(writer, worldX, worldZ, localX, localZ, SY);
    }

    // Sobrecarga con writer explicito, para motores con otro layout
    // (por ejemplo subchunks paletizados).
    template <typename Writer>
    ColumnData GenerateColumnTo(Writer& writer,
                                int worldX, int worldZ,
                                int localX, int localZ,
                                int worldHeight) const {
        return chunkGen.GenerateColumn(writer, worldX, worldZ, localX, localZ, worldHeight);
    }

    // ========================================================================
    // CONSULTAS (para spawn, HUD, depuracion)
    // ========================================================================

    // Altura del terreno en un punto. Util para encontrar un spawn seguro.
    int GetTerrainHeight(int worldX, int worldZ) const {
        const ColumnData col = chunkGen.Terrain().GetColumnData((float)worldX, (float)worldZ);
        return col.surfaceHeight;
    }

    // Bioma en un punto.
    BiomeType GetBiomeAt(int worldX, int worldZ) const {
        const ColumnData col = chunkGen.Terrain().GetColumnData((float)worldX, (float)worldZ);
        return col.biome;
    }

    const char* GetBiomeNameAt(int worldX, int worldZ) const {
        return GetBiomeName(GetBiomeAt(worldX, worldZ));
    }

    // Datos completos de la columna.
    ColumnData GetColumnData(int worldX, int worldZ) const {
        return chunkGen.Terrain().GetColumnData((float)worldX, (float)worldZ);
    }

    // ========================================================================
    // DECORACION (ETAPA 7) - la consulta el motor tras generar el terreno
    // ========================================================================
    // El motor ya tiene rutinas de dibujado de arboles (generarRoble, etc.),
    // asi que aqui solo se decide QUE y DONDE.

    bool ShouldPlaceTree(int worldX, int worldZ, const ColumnData& col) const {
        // Nunca en agua ni en playa.
        if (col.isOcean || col.isBeach) return false;
        if (col.surfaceHeight <= SEA_LEVEL) return false;
        return chunkGen.Terrain().Decoration().HasTree(worldX, worldZ, col.biome, col.slope);
    }

    TreeType GetTreeTypeAt(int worldX, int worldZ, const ColumnData& col) const {
        return chunkGen.Terrain().Decoration().GetTreeType(
            worldX, worldZ, col.biome, col.climate.temperature);
    }

    bool ShouldPlaceTallGrass(int worldX, int worldZ, const ColumnData& col) const {
        if (col.isOcean || col.surfaceHeight <= SEA_LEVEL) return false;
        return chunkGen.Terrain().Decoration().HasTallGrass(worldX, worldZ, col.biome);
    }

    bool ShouldPlaceFlower(int worldX, int worldZ, const ColumnData& col) const {
        if (col.isOcean || col.surfaceHeight <= SEA_LEVEL) return false;
        return chunkGen.Terrain().Decoration().HasFlower(worldX, worldZ, col.biome);
    }

    // ========================================================================
    // ETAPA 12: VERIFICACION DE DETERMINISMO
    // ========================================================================
    // Comprueba que dos evaluaciones independientes del mismo punto dan el
    // mismo resultado. Devuelve true si el generador es determinista.
    // Pensado para tests, no para el bucle de juego.
    bool VerifyDeterminism(int worldX, int worldZ) const {
        const ColumnData a = GetColumnData(worldX, worldZ);
        const ColumnData b = GetColumnData(worldX, worldZ);
        return a.surfaceHeight == b.surfaceHeight
            && a.biome == b.biome
            && a.isBeach == b.isBeach;
    }

    // ========================================================================
    // VERIFICACION DE CONTINUIDAD (sin costuras entre chunks)
    // ========================================================================
    // Comprueba que la altura no salta bruscamente al cruzar la frontera de
    // un chunk. Como el ruido se evalua en coordenadas de mundo, la
    // diferencia entre dos columnas adyacentes debe ser pequena.
    // Devuelve la diferencia maxima de altura encontrada.
    int VerifyChunkSeam(int chunkX, int chunkZ, int chunkSize = 16) const {
        int maxDelta = 0;
        const int borderX = chunkX * chunkSize + chunkSize - 1;

        for (int z = 0; z < chunkSize; ++z) {
            const int wz = chunkZ * chunkSize + z;
            const int hA = GetTerrainHeight(borderX, wz);      // ultima col. del chunk
            const int hB = GetTerrainHeight(borderX + 1, wz);  // primera del vecino
            const int d = hA > hB ? hA - hB : hB - hA;
            if (d > maxDelta) maxDelta = d;
        }
        return maxDelta;
    }
};

} // namespace TerrainGen

#endif // WORLD_GENERATOR_AAA_H
