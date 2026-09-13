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
    // ⭐ 32 BITS, NO 16 (y antes fueron 8).
    //
    // La historia de este tipo es la historia de un mismo fallo repetido: un
    // ID que no cabe se trunca EN SILENCIO y el bloque resultante es otro.
    //
    //   uint8_t  -> la pirita (262) se truncaba a 6, asi que su veta NUNCA
    //               aparecia en el mundo. Sin error ni aviso. Lo caso un test
    //               que MIDE cuanto sale de cada mineral: daba 0,00% donde
    //               debia dar ~4%.
    //
    //   uint16_t -> el mismo fallo esperando a los BLOQUES COMPUESTOS. Su
    //               espacio de IDs empieza en 100.000 (ver BloqueCompuesto.h),
    //               muy por encima de los 65.535 que caben en 16 bits. Con
    //               este tipo, el generador NO PUEDE escribir agua con nivel:
    //               `Agua::nuevo(8)` = 100.008 se truncaria a 34.472, que no
    //               es agua ni es nada.
    //
    // Por eso sube a 32 bits ahora, que es cuando el generador de rios empieza
    // a necesitar escribir agua con volumen. BlockType es un enum de 4 bytes y
    // el chunk lo guarda como tal, asi que este tipo no ensancha nada en disco
    // ni en memoria: solo deja de estrechar por el camino.
    //
    // El alias existe para que este limite tenga UN SOLO sitio.
    using Id = uint32_t;

    constexpr Id AIR          = (Id)BLOCK_AIR;
    constexpr Id GRASS        = (Id)BLOCK_GRASS;
    constexpr Id DIRT         = (Id)BLOCK_DIRT;
    constexpr Id STONE        = (Id)BLOCK_STONE;
    constexpr Id WOOD         = (Id)BLOCK_WOOD;
    constexpr Id LEAVES       = (Id)BLOCK_LEAVES;
    constexpr Id SAND         = (Id)BLOCK_SAND;
    constexpr Id WATER        = (Id)BLOCK_WATER;
    constexpr Id TALLGRASS    = (Id)BLOCK_TALLGRASS;
    constexpr Id BEDROCK      = (Id)BLOCK_BEDROCK;
    constexpr Id COBBLESTONE  = (Id)BLOCK_COBBLESTONE;
    constexpr Id GRAVEL       = (Id)BLOCK_GRAVEL;
    constexpr Id SNOW         = (Id)BLOCK_SNOW;
    constexpr Id LAVA         = (Id)BLOCK_LAVA;
    constexpr Id CLAY         = (Id)BLOCK_CLAY;

    // Minerales
    constexpr Id COAL_ORE     = (Id)BLOCK_COAL_ORE;
    constexpr Id DIAMOND_ORE  = (Id)BLOCK_DIAMOND_ORE;
    constexpr Id IRON_ORE     = (Id)BLOCK_IRON_ORE;
    constexpr Id GOLD_ORE     = (Id)BLOCK_GOLD_ORE;
    constexpr Id SILVER_ORE   = (Id)BLOCK_SILVER_ORE;
    constexpr Id SCRAP_METAL  = (Id)BLOCK_SCRAP_METAL;
    constexpr Id PYRITE_ORE   = (Id)BLOCK_PYRITE_ORE;

    // Bloques nuevos
    constexpr Id LIMESTONE    = (Id)BLOCK_LIMESTONE;
    constexpr Id CLAY_DIRT    = (Id)BLOCK_CLAY_DIRT;
    constexpr Id CLAY_SAND    = (Id)BLOCK_CLAY_SAND;
}

// ----------------------------------------------------------------------------
// ENUM DE BIOMAS
// ----------------------------------------------------------------------------
// Los 7 biomas pedidos. Las variantes (OCEAN_DEEP, MOUNTAIN_PEAKS, etc.)
// permiten transiciones internas sin bordes duros.
// ⚠️ EL ORDEN ES FORMATO DE DISCO. Estos valores se guardan en los chunks,
// asi que los biomas NUEVOS van SIEMPRE AL FINAL, justo antes de BIOME_COUNT.
// Intercalar uno en medio renumera todos los siguientes y un mundo guardado
// pasaria a leerse con los biomas corridos: los bosques saldrian de arena.
enum BiomeType : uint8_t {
    BIOME_OCEAN_DEEP = 0,   // Oceano profundo (abismal)
    BIOME_OCEAN,            // Oceano / plataforma continental
    BIOME_BEACH,            // Playa (solo en costas, ETAPA 6)
    BIOME_PLAINS,           // Planicies
    BIOME_FOREST,           // Bosque
    BIOME_DESERT,           // Desierto
    BIOME_MOUNTAINS,        // Montanas (laderas)
    BIOME_MOUNTAIN_PEAKS,   // Picos nevados

    // --- LOS TRES OCOTALES ---
    // Los pinares del centro de Mexico. El ocote ya existia como ARBOL dentro
    // del bosque mixto; esto le da BIOMA propio, que es donde de verdad se ve
    // como lo que es: una masa continua de pino en vez de un pino suelto entre
    // encinos.
    //
    // Son tres y no uno porque las dos especies NO ocupan la misma cota:
    //   - Pinus montezumae (ocote blanco) sube mas y aguanta mas frio.
    //   - Pinus leiophylla (ocote chino)  es de cota media y mas seca.
    //   - Donde sus rangos se solapan crecen mezclados, y ese solape es un
    //     bosque distinto de los dos puros: el ocotal mixto.
    BIOME_OCOTAL_BLANCO,    // Pinar de ocote blanco (Pinus montezumae)
    BIOME_OCOTAL_CHINO,     // Pinar de ocote chino (Pinus leiophylla)
    BIOME_OCOTAL_MIXTO,     // Los dos ocotes mezclados

    BIOME_COUNT
};

// ----------------------------------------------------------------------------
// DEFINICION DE BIOMA (dato puro)
// ----------------------------------------------------------------------------
struct BiomeDefinition {
    BiomeType   type;
    const char* name;

    // --- Superficie ---
    Blocks::Id surfaceBlock;    // Bloque de la capa superior
    Blocks::Id subsurfaceBlock; // Bloque bajo la superficie
    Blocks::Id underwaterBlock; // Bloque de superficie si esta bajo el agua
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
    { BIOME_MOUNTAIN_PEAKS, "Picos Nevados",   Blocks::SNOW,   Blocks::STONE, Blocks::STONE,  3,  30.0f, 0.024f,  0.00f, 0.00f, 0.00f, false, true  },

    // --- LOS TRES OCOTALES ---
    //
    // RELIEVE (hVar/detFreq). Mas movido que el bosque llano (8.0) y menos que
    // la montana (22.0): el pinar mexicano vive en lomerio, no en llanura ni en
    // pared. El blanco es el mas alto de los tres porque es el que sube de cota.
    //
    // ARBOLADO (tree). Por ENCIMA del bosque mixto (0.92): un ocotal es mas
    // cerrado que un bosque de encino. El mixto es el mas denso de los tres,
    // porque dos especies de porte distinto llenan huecos que una sola deja.
    //
    // SOTOBOSQUE (grass/flower). Por DEBAJO del bosque, y esto no es adorno: la
    // acicula de pino acidifica el suelo y forma una capa que ahoga la hierba.
    // Un pinar real tiene el suelo pelado y cubierto de hojarasca. Es lo que
    // hace que se SIENTA distinto de un bosque al caminarlo.
    //
    //                     tipo               nombre              surf         subsurf      underwater   depth  hVar  detFreq  tree   grass  flower ocean  snow
    { BIOME_OCOTAL_BLANCO, "Ocotal Blanco",   Blocks::GRASS,  Blocks::DIRT,  Blocks::DIRT,   4,  13.0f, 0.018f,  0.95f, 0.10f, 0.02f, false, false },
    { BIOME_OCOTAL_CHINO,  "Ocotal Chino",    Blocks::GRASS,  Blocks::DIRT,  Blocks::DIRT,   4,  10.0f, 0.017f,  0.93f, 0.14f, 0.03f, false, false },
    { BIOME_OCOTAL_MIXTO,  "Ocotal Mixto",    Blocks::GRASS,  Blocks::DIRT,  Blocks::DIRT,   4,  11.5f, 0.018f,  0.97f, 0.12f, 0.02f, false, false }
};

// ----------------------------------------------------------------------------
// LA TABLA Y EL ENUM NO PUEDEN DESINCRONIZARSE
// ----------------------------------------------------------------------------
// BIOME_TABLE se indexa por el valor del enum, asi que una fila fuera de sitio
// devuelve el bioma equivocado SIN ERROR: se veria como bosques de arena, que
// es exactamente el fallo que ya ocurrio una vez con los IDs de bloque (ver la
// nota de Blocks, arriba). Esto lo convierte en error de compilacion.
static_assert(BIOME_TABLE[BIOME_OCOTAL_BLANCO].type == BIOME_OCOTAL_BLANCO, "BIOME_TABLE desalineada");
static_assert(BIOME_TABLE[BIOME_OCOTAL_CHINO].type  == BIOME_OCOTAL_CHINO,  "BIOME_TABLE desalineada");
static_assert(BIOME_TABLE[BIOME_OCOTAL_MIXTO].type  == BIOME_OCOTAL_MIXTO,  "BIOME_TABLE desalineada");
static_assert(BIOME_TABLE[BIOME_FOREST].type        == BIOME_FOREST,        "BIOME_TABLE desalineada");
static_assert(BIOME_TABLE[BIOME_OCEAN_DEEP].type    == BIOME_OCEAN_DEEP,    "BIOME_TABLE desalineada");

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

// ----------------------------------------------------------------------------
// ¿ES UN PINAR DE OCOTE?
// ----------------------------------------------------------------------------
// Existe para que nadie tenga que escribir los tres casos a mano. Cada sitio
// que enumere los tres es un sitio que se olvidara del cuarto si algun dia se
// añade un ocotal mas -- y ese olvido no da error de compilacion, da un bioma
// que se comporta distinto de sus hermanos sin motivo aparente.
//
// Es tambien lo que responde "¿esto cuenta como bosque?" para la fauna y para
// cualquier regla que hoy mire BIOME_FOREST.
inline bool EsOcotal(BiomeType t) {
    return t == BIOME_OCOTAL_BLANCO ||
           t == BIOME_OCOTAL_CHINO  ||
           t == BIOME_OCOTAL_MIXTO;
}

// ¿Es bosque en sentido amplio? Bosque mixto o cualquier ocotal.
inline bool EsBoscoso(BiomeType t) {
    return t == BIOME_FOREST || EsOcotal(t);
}

} // namespace TerrainGen

#endif // BIOME_TYPES_H
