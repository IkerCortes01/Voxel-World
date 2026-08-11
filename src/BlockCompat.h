#pragma once

#include "BlockType.h"
#include <cstdint>

// ============================================================================
// COMPATIBILIDAD DE IDs DE BLOQUE ENTRE VERSIONES DE GUARDADO
// ============================================================================
// Los IDs de bloque se REORDENARON para agrupar por familia (troncos juntos,
// hojas juntas, tablones juntos). Ese cambio rompe los mundos ya guardados:
// un chunk escrito con el orden viejo guarda el número 11 para "tablones de
// pino", pero en el orden nuevo el 11 es "grava". Sin traducir, un mundo
// existente se cargaría con los bloques cambiados unos por otros.
//
// Aquí vive la tabla que convierte los IDs de la versión ANTIGUA (save v2) a
// los actuales. SaveSystem la aplica al leer un chunk cuya versión sea menor
// que la actual; los chunks nuevos se guardan ya con los IDs nuevos y no pasan
// por aquí.
// ============================================================================

namespace BlockCompat {

// Orden ANTIGUO (save v2), tal cual estaba en el enum antes de reordenar.
// El índice es el ID viejo; el valor, el bloque al que corresponde ahora.
enum LegacyId : uint16_t {
    L_AIR = 0, L_GRASS, L_DIRT, L_STONE, L_WOOD, L_LEAVES, L_SAND, L_WATER,
    L_TALLGRASS, L_BEDROCK, L_COBBLESTONE, L_PLANKS, L_BRICKS, L_GLASS,
    L_COAL_ORE, L_DIAMOND_ORE, L_GRAVEL, L_ORANGE_FLOWER, L_SNOW,
    L_SCRAP_METAL, L_LAVA, L_IRON_ORE, L_GOLD_ORE, L_SILVER_ORE,
    L_DIRT_POWDER, L_STICK, L_HOE, L_COAL_ITEM, L_RAW_ZINC, L_RAW_COPPER,
    L_CLAY, L_WOOD_ENCINO, L_LEAVES_ENCINO, L_WOOD_OYAMEL, L_LEAVES_OYAMEL,
    L_PLANKS_ENCINO, L_PLANKS_OYAMEL,
    L_COUNT
};

// Traduce un ID guardado con el formato viejo al ID actual.
// Los IDs desconocidos (de una versión futura o datos corruptos) se degradan a
// aire, que es el valor seguro: mejor un hueco que un bloque aleatorio.
inline BlockType fromLegacy(uint16_t legacy) {
    switch (legacy) {
        case L_AIR:            return BLOCK_AIR;
        case L_GRASS:          return BLOCK_GRASS;
        case L_DIRT:           return BLOCK_DIRT;
        case L_STONE:          return BLOCK_STONE;
        case L_WOOD:           return BLOCK_WOOD;
        case L_LEAVES:         return BLOCK_LEAVES;
        case L_SAND:           return BLOCK_SAND;
        case L_WATER:          return BLOCK_WATER;
        case L_TALLGRASS:      return BLOCK_TALLGRASS;
        // El bedrock sale de la lista ordenada pero SIGUE existiendo (el
        // motor lo consulta), asi que se conserva tal cual.
        case L_BEDROCK:        return BLOCK_BEDROCK;
        case L_COBBLESTONE:    return BLOCK_COBBLESTONE;
        case L_PLANKS:         return BLOCK_PLANKS;
        case L_BRICKS:         return BLOCK_BRICKS;
        case L_GLASS:          return BLOCK_GLASS;
        case L_COAL_ORE:       return BLOCK_COAL_ORE;
        case L_DIAMOND_ORE:    return BLOCK_DIAMOND_ORE;
        case L_GRAVEL:         return BLOCK_GRAVEL;
        case L_ORANGE_FLOWER:  return BLOCK_AIR;   // retirada del juego
        case L_SNOW:           return BLOCK_SNOW;
        case L_SCRAP_METAL:    return BLOCK_SCRAP_METAL;
        case L_LAVA:           return BLOCK_LAVA;
        case L_IRON_ORE:       return BLOCK_STONE; // nunca se implemento
        case L_GOLD_ORE:       return BLOCK_GOLD_ORE;
        case L_SILVER_ORE:     return BLOCK_SILVER_ORE;
        case L_DIRT_POWDER:    return BLOCK_DIRT_POWDER;
        case L_STICK:          return BLOCK_STICK;
        case L_HOE:            return BLOCK_HOE;
        case L_COAL_ITEM:      return BLOCK_COAL_ITEM;
        case L_RAW_ZINC:       return BLOCK_RAW_ZINC;
        case L_RAW_COPPER:     return BLOCK_RAW_COPPER;
        case L_CLAY:           return BLOCK_CLAY;
        case L_WOOD_ENCINO:    return BLOCK_WOOD_ENCINO;
        case L_LEAVES_ENCINO:  return BLOCK_LEAVES_ENCINO;
        case L_WOOD_OYAMEL:    return BLOCK_WOOD_OYAMEL;
        case L_LEAVES_OYAMEL:  return BLOCK_LEAVES_OYAMEL;
        case L_PLANKS_ENCINO:  return BLOCK_PLANKS_ENCINO;
        case L_PLANKS_OYAMEL:  return BLOCK_PLANKS_OYAMEL;
        default:               return BLOCK_AIR;
    }
}

} // namespace BlockCompat
