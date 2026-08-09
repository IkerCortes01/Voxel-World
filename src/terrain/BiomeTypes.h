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
// IDs DE BLOQUE: coinciden con el enum BlockType de main.cpp. Se replican
// aqui como constantes con nombre para que el modulo de terreno no dependa
// de main.cpp (evita una dependencia circular) y para que el mapeo sea
// explicito y auditable.
// ============================================================================

namespace TerrainGen {

// ----------------------------------------------------------------------------
// IDs de bloque (espejo de BlockType en main.cpp)
// ----------------------------------------------------------------------------
// VERIFICADO contra main.cpp:142-176. Si el enum de main.cpp cambia de orden,
// hay que actualizar estas constantes.
namespace Blocks {
    constexpr uint8_t AIR          = 0;
    constexpr uint8_t GRASS        = 1;
    constexpr uint8_t DIRT         = 2;
    constexpr uint8_t STONE        = 3;
    constexpr uint8_t WOOD         = 4;
    constexpr uint8_t LEAVES       = 5;
    constexpr uint8_t SAND         = 6;
    constexpr uint8_t WATER        = 7;
    constexpr uint8_t TALLGRASS    = 8;
    constexpr uint8_t BEDROCK      = 9;
    constexpr uint8_t COBBLESTONE  = 10;
    constexpr uint8_t GRAVEL       = 16;
    constexpr uint8_t ORANGE_FLOWER= 17;
    constexpr uint8_t SNOW         = 18;
    constexpr uint8_t LAVA         = 20;
    // Arcilla: añadida al final del enum de main.cpp.
    // ID 30 (verificado contando el enum: AIR=0 ... RAW_COPPER=29, CLAY=30).
    constexpr uint8_t CLAY         = 30;

    // Minerales
    constexpr uint8_t COAL_ORE     = 14;
    constexpr uint8_t DIAMOND_ORE  = 15;
    constexpr uint8_t IRON_ORE     = 21;
    constexpr uint8_t GOLD_ORE     = 22;
    constexpr uint8_t SILVER_ORE   = 23;
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
