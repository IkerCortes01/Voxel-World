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
        // Los minerales de veta se retiraron del juego: un mundo v2 que los
        // traiga los recibe ya convertidos en piedra, que es lo mismo que hace
        // mineralAPiedra() con los mundos nuevos. Asi las dos rutas de carga
        // coinciden y no hay una version del mundo que conserve minerales.
        case L_COAL_ORE:       return BLOCK_STONE;
        case L_DIAMOND_ORE:    return BLOCK_STONE;
        case L_GRAVEL:         return BLOCK_GRAVEL;
        case L_ORANGE_FLOWER:  return BLOCK_AIR;   // retirada del juego
        case L_SNOW:           return BLOCK_SNOW;
        case L_SCRAP_METAL:    return BLOCK_STONE;  // mineral retirado
        case L_LAVA:           return BLOCK_LAVA;
        case L_IRON_ORE:       return BLOCK_STONE; // nunca se implemento
        case L_GOLD_ORE:       return BLOCK_STONE; // mineral retirado
        case L_SILVER_ORE:     return BLOCK_STONE; // mineral retirado
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

// ============================================================================
// LOS MINERALES RETIRADOS SE CONVIERTEN EN PIEDRA
// ============================================================================
// Los siete bloques de mineral de veta (carbon, plata, oro, diamante, pirita,
// hierro y desecho) se eliminaron del juego. Sus IDs siguen existiendo en el
// enum como huecos reservados -- borrarlos habria corrido los numeros de todo
// lo que va detras -- pero ya no se generan, no tienen textura y no se pueden
// colocar.
//
// Un mundo guardado antes de esto trae esos IDs en el terreno: medido sobre el
// volumen que el jugador tiene cargado, unos 152.000 bloques. Hay que darles
// algo que sea.
//
// ----------------------------------------------------------------------------
// POR QUE PIEDRA Y NO AIRE
// ----------------------------------------------------------------------------
// Es la unica respuesta que no rompe nada. Los minerales de veta estaban
// EMBEBIDOS en la roca: convertirlos en aire agujerearia el subsuelo de todos
// los mundos guardados -- cavernas que nadie excavo, techos sin sostener, y el
// jugador cayendo por huecos que no estaban ahi al guardar la partida.
//
// La piedra es lo que rodeaba a cada veta, asi que el resultado es el terreno
// que habria habido si los minerales no hubieran existido nunca. Ademas es
// solida, opaca y del mismo material, asi que la luz y el mallado no cambian.
//
// ----------------------------------------------------------------------------
// TAMBIEN LAS LONCHAS
// ----------------------------------------------------------------------------
// Un mineral podia estar a media altura (BLOCK_COAL_L1..L7 y companyia). Esas
// lonchas se convierten en la loncha de piedra DEL MISMO NIVEL, no en un
// bloque entero: rellenar la celda cambiaria la forma del suelo por el que se
// anda. `mineralAPiedra` conserva el nivel; el llamante no tiene que saberlo.
//
// ⚠️ SE APLICA A TODOS LOS MUNDOS, no solo a los de version vieja. Esto no es
// un cambio de numeracion de IDs (que es lo que cubre fromLegacy y va atado a
// SAVE_VERSION), sino la retirada de un bloque: un mundo guardado ayer con el
// formato actual tiene los mismos minerales que uno de hace meses.
inline BlockType mineralAPiedra(BlockType t) {
    switch ((int)t) {
        // --- Los bloques enteros (sus huecos reservados en el enum) ---
        case BLOCK_RESERVADO_MINERAL_08:   // era carbon
        case BLOCK_RESERVADO_MINERAL_09:   // era desecho de metales
        case BLOCK_RESERVADO_MINERAL_20:   // era plata
        case BLOCK_RESERVADO_MINERAL_21:   // era diamante
        case BLOCK_RESERVADO_MINERAL_26:   // era oro
        case BLOCK_RESERVADO_MINERAL_164:  // era pirita
        case BLOCK_IRON_ORE:               // nunca llego a implementarse
            return BLOCK_STONE;
        default:
            break;
    }

    // --- Las lonchas: misma altura, pero de piedra ---
    //
    // Se resuelve por rango y no con 35 `case` para que anyadir o quitar una
    // familia de niveles no obligue a tocar esta lista.
    struct RangoNivel { BlockType primero; };
    static const RangoNivel LONCHAS[] = {
        { BLOCK_COAL_L1 }, { BLOCK_SILVER_L1 }, { BLOCK_GOLD_L1 },
        { BLOCK_DIAMOND_L1 }, { BLOCK_SCRAP_L1 },
    };
    for (const RangoNivel& r : LONCHAS) {
        const int ini = (int)r.primero;
        if ((int)t >= ini && (int)t < ini + 7) {
            // El desplazamiento dentro de la familia ES el nivel (L1..L7), y
            // las familias de nivel estan todas dispuestas igual, asi que se
            // traslada tal cual a la de piedra.
            return (BlockType)((int)BLOCK_STONE_L1 + ((int)t - ini));
        }
    }

    return t;
}

// ¿Este ID es uno de los minerales retirados (entero o loncha)?
inline bool esMineralRetirado(BlockType t) {
    return mineralAPiedra(t) != t;
}

// ============================================================================
// ⭐⭐ CELDAS MIXTAS: EL RELLENO GANO UNA ALTURA
// ============================================================================
// EL CAMBIO. Antes el relleno de una celda mixta no tenia altura propia: iba
// de la capa de abajo AL TECHO, asi que una mixta era siempre 8 octavos y su
// ID se codificaba:
//
//     VIEJO:  BASE + (ib * 7 + (nivelBase - 1)) * FAMILIAS + ir
//
// Ahora el relleno lleva su nivel dentro del ID, para que una celda pueda
// estar a medio llenar con los dos materiales a la vista (era la causa de que
// apilar arena sobre pasto no se viera hasta completar el bloque):
//
//     NUEVO:  (lo de arriba) * NIVELES_RELLENO + (nivelRelleno - 1)
//
// POR QUE HACE FALTA MIGRAR. Los dos esquemas comparten el mismo rango de
// IDs, asi que un numero guardado con el viejo se decodifica MAL con el
// nuevo: el 1201 que era "tierra(3) + arena" pasaria a leerse como otro par
// de materiales. Sin esta funcion, cada celda mixta de un mundo guardado se
// convertiria en otra cosa al cargarlo.
//
// LA TRADUCCION. Se deshace la cuenta vieja para sacar (base, nivel, relleno)
// y se vuelve a montar con la nueva, dandole al relleno la altura que TENIA
// de hecho: la que llegaba hasta el techo, o sea 8 - nivelBase. Asi el bloque
// se ve exactamente igual que antes de la actualizacion.
//
// ⚠️ SOLO SE LLAMA SOBRE MUNDOS VIEJOS. Lo decide la version del save, igual
// que `fromLegacy` y `mineralAPiedra`: aplicarla a un ID ya nuevo lo
// corromperia.
inline BlockType mixtoAlturaRelleno(BlockType t) {
    const int id = (int)t;
    if (id < BLOCK_MIXTO_BASE || id >= BLOCK_MIXTO_FIN) return t;

    // --- Se deshace la codificacion VIEJA ---
    //
    // ⚠️ CON EL ANCHO VIEJO, NO CON EL DE HOY. `FAMILIAS_CON_NIVEL` subio de
    // 22 a 24 al entrar el techo de hojas en la tabla de relleno, asi que
    // usar la constante actual aqui decodificaria mal los IDs guardados: el
    // numero escrito en disco se monto dividiendo entre 22.
    //
    // Se fija el valor historico. Si vuelve a ampliarse, esta constante NO se
    // toca: describe lo que hay en los mundos viejos, no lo que hay hoy.
    constexpr int FAMILIAS_VIEJAS = 22;

    const int k  = id - BLOCK_MIXTO_BASE;
    const int ir = k % FAMILIAS_VIEJAS;             // indice del relleno
    const int kb = k / FAMILIAS_VIEJAS;
    const int nivelBase = (kb % 7) + 1;             // 1..7
    const int ib = kb / 7;                          // indice de la base

    // Si los indices no son validos, se deja el ID tal cual: mejor un bloque
    // raro que uno traducido a otra cosa.
    int nFam = 0; const FamiliaNivel* FAM = tablaNiveles(nFam);
    int nRel = 0; const BlockType*   REL = tablaRelleno(nRel);
    if (ib < 0 || ib >= nFam || ir < 0 || ir >= nRel) return t;

    const BlockType base    = FAM[ib].entero;
    const BlockType relleno = REL[ir];
    if (base == BLOCK_AIR || relleno == BLOCK_AIR) return t;

    // El relleno viejo llegaba hasta el techo: esa era su altura real.
    const BlockType nuevo = mixto(base, nivelBase, relleno, 8 - nivelBase);
    return (nuevo != BLOCK_AIR) ? nuevo : t;
}

} // namespace BlockCompat
