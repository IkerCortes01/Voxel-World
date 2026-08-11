#ifndef BIOME_TYPES_H
#define BIOME_TYPES_H

#include <cstdint>

// ============================================================================
// BIOME TYPES + REGISTRO DE BIOMAS (ETAPA 13: Escalabilidad)
// ============================================================================
// Los biomas se definen COMO DATOS en una tabla, no como codigo disperso.
// Anadir un bioma nuevo = anadir una entrada al enum y una fila a la tabla
// BIOME_TABLE. Ningun otro modulo necesita cambiar: el selector de biomas,
// el generador de superficie y el decorador leen todos desde este registro.
//
// IDs DE BLOQUE: se toman DIRECTAMENTE del enum BlockType (BlockType.h).
// ============================================================================

#include "../BlockType.h"

namespace TerrainGen {

// ----------------------------------------------------------------------------
// IDs de bloque
// ----------------------------------------------------------------------------
// ⭐ Antes esto era una copia A MANO de los numeros del enum, con un comentario
// que avisaba "si el enum cambia de orden, hay que actualizar estas
// constantes". Al reordenar los IDs nadie lo hizo, y el generador empezo a
// escribir bloques equivocados: pedia SAND=6 y el 6 habia pasado a ser
// "tronco de oyamel", COBBLESTONE=10 y el 10 era "arena"... con lo que las
// playas salian de piedra labrada y el terreno de troncos.
//
// Ahora cada constante se DERIVA del enum: si manana se reordena otra vez,
// estos valores se actualizan solos y el fallo no puede repetirse.
namespace Blocks {
    constexpr uint8_t AIR          = (uint8_t)BLOCK_AIR;
    constexpr uint8_t GRASS        = (uint8_t)BLOCK_GRASS;
    constexpr uint8_t DIRT         = (uint8_t)BLOCK_DIRT;
    constexpr uint8_t STONE        = (uint8_t)BLOCK_STONE;
    constexpr uint8_t WOOD         = (uint8_t)BLOCK_WOOD;
    constexpr uint8_t LEAVES       = (uint8_t)BLOCK_LEAVES;
    constexpr uint8_t SAND         = (uint8_t)BLOCK_SAND;
    constexpr uint8_t WATER        = (uint8_t)BLOCK_WATER;
    constexpr uint8_t TALLGRASS    = (uint8_t)BLOCK_TALLGRASS;
    constexpr uint8_t BEDROCK      = (uint8_t)BLOCK_BEDROCK;
    constexpr uint8_t COBBLESTONE  = (uint8_t)BLOCK_COBBLESTONE;
    constexpr uint8_t GRAVEL       = (uint8_t)BLOCK_GRAVEL;
    constexpr uint8_t SNOW         = (uint8_t)BLOCK_SNOW;
    constexpr uint8_t LAVA         = (uint8_t)BLOCK_LAVA;
    constexpr uint8_t CLAY         = (uint8_t)BLOCK_CLAY;

    // Minerales
    constexpr uint8_t COAL_ORE     = (uint8_t)BLOCK_COAL_ORE;
    constexpr uint8_t DIAMOND_ORE  = (uint8_t)BLOCK_DIAMOND_ORE;
    constexpr uint8_t IRON_ORE     = (uint8_t)BLOCK_IRON_ORE;
    constexpr uint8_t GOLD_ORE     = (uint8_t)BLOCK_GOLD_ORE;
    constexpr uint8_t SILVER_ORE   = (uint8_t)BLOCK_SILVER_ORE;
    constexpr uint8_t SCRAP_METAL  = (uint8_t)BLOCK_SCRAP_METAL;

    // Bloques nuevos
    constexpr uint8_t LIMESTONE    = (uint8_t)BLOCK_LIMESTONE;
    constexpr uint8_t CLAY_DIRT    = (uint8_t)BLOCK_CLAY_DIRT;
    constexpr uint8_t CLAY_SAND    = (uint8_t)BLOCK_CLAY_SAND;
}

// ----------------------------------------------------------------------------
// ENUM DE BIOMAS
// ----------------------------------------------------------------------------
// Los 7 biomas pedidos. Las variantes (OCEAN_DEEP, MOUNTAIN_PEAKS, etc.)
// permiten transiciones internas sin bordes duros.
enum BiomeType : uint8_t {
    BIOME_OCEAN_DEEP = 0,   // Oceano profundo (abismal)
    BIOME_OCEAN,            // Oceano / plataforma continental
    BIOME_BEACH,            // Playa (solo en costas, ETAPA 6)
    BIOME_PLAINS,           // Planicies
    BIOME_FOREST,           // Bosque
    BIOME_DESERT,           // Desierto
    BIOME_MOUNTAINS,        // Montanas (laderas)
    BIOME_MOUNTAIN_PEAKS,   // Picos nevados
    BIOME_COUNT
};

// ----------------------------------------------------------------------------
// DEFINICION DE BIOMA (dato puro)
// ----------------------------------------------------------------------------
struct BiomeDefinition {
    BiomeType   type;
    const char* name;

    // --- Superficie ---
    uint8_t surfaceBlock;    // Bloque de la capa superior
    uint8_t subsurfaceBlock; // Bloque bajo la superficie
    uint8_t underwaterBlock; // Bloque de superficie si esta bajo el agua
    int     subsurfaceDepth; // Grosor de la capa de subsuelo

    // --- Relieve (ETAPA 3) ---
    // Amplitud vertical del detalle propio del bioma, en bloques.
    float   heightVariation;
    // Frecuencia del detalle: alta = rugoso, baja = suave.
    float   detailFrequency;

    // --- Vegetacion (ETAPA 7) ---
    float   treeDensity;     // 0 = sin arboles, 1 = bosque denso
    float   grassDensity;    // Probabilidad de hierba alta
    float   flowerDensity;   // Probabilidad de flores

    // --- Clima (para mezcla, ETAPA 10) ---
    bool    isOcean;         // Si true, se rellena con agua hasta SEA_LEVEL
    bool    isSnowy;         // Cubre la superficie con nieve
};

// ----------------------------------------------------------------------------
// TABLA DE BIOMAS
// ----------------------------------------------------------------------------
// EDITAR AQUI PARA ANADIR/AJUSTAR BIOMAS. El orden debe coincidir con el enum.
//
//                     tipo               nombre       surf         subsurf      underwater   depth  hVar  detFreq  tree   grass  flower ocean  snow
static const BiomeDefinition BIOME_TABLE[BIOME_COUNT] = {
    { BIOME_OCEAN_DEEP,     "Oceano Profundo", Blocks::GRAVEL, Blocks::STONE, Blocks::GRAVEL, 3,   6.0f, 0.010f,  0.00f, 0.00f, 0.00f, true,  false },
    { BIOME_OCEAN,          "Oceano",          Blocks::SAND,   Blocks::DIRT,  Blocks::SAND,   3,   4.0f, 0.015f,  0.00f, 0.00f, 0.00f, true,  false },
    { BIOME_BEACH,          "Playa",           Blocks::SAND,   Blocks::SAND,  Blocks::SAND,   4,   1.5f, 0.020f,  0.00f, 0.02f, 0.00f, false, false },
    { BIOME_PLAINS,         "Planicies",       Blocks::GRASS,  Blocks::DIRT,  Blocks::DIRT,   4,   5.0f, 0.012f,  0.06f, 0.35f, 0.08f, false, false },
    { BIOME_FOREST,         "Bosque",          Blocks::GRASS,  Blocks::DIRT,  Blocks::DIRT,   4,   8.0f, 0.016f,  0.92f, 0.25f, 0.04f, false, false },
    { BIOME_DESERT,         "Desierto",        Blocks::SAND,   Blocks::SAND,  Blocks::SAND,   6,   7.0f, 0.014f,  0.00f, 0.01f, 0.00f, false, false },
    { BIOME_MOUNTAINS,      "Montanas",        Blocks::STONE,  Blocks::STONE, Blocks::GRAVEL, 3,  22.0f, 0.020f,  0.06f, 0.05f, 0.01f, false, false },
    { BIOME_MOUNTAIN_PEAKS, "Picos Nevados",   Blocks::SNOW,   Blocks::STONE, Blocks::STONE,  3,  30.0f, 0.024f,  0.00f, 0.00f, 0.00f, false, true  }
};

// Acceso seguro al registro.
inline const BiomeDefinition& GetBiome(BiomeType t) {
    const int i = (int)t;
    if (i < 0 || i >= (int)BIOME_COUNT) return BIOME_TABLE[BIOME_PLAINS];
    return BIOME_TABLE[i];
}

inline const char* GetBiomeName(BiomeType t) {
    return GetBiome(t).name;
}

inline bool IsOceanBiome(BiomeType t) {
    return GetBiome(t).isOcean;
}

} // namespace TerrainGen

#endif // BIOME_TYPES_H
