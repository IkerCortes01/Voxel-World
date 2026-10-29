#pragma once

#include "../BlockType.h"
#include "../BloqueCompuesto.h"

// ============================================================================
// EL NOMBRE DE CADA ITEM, EN UN SOLO SITIO
// ============================================================================
// PEDIDO: "todos los items deben tener -- bloques, herramientas, todo -- que
// cuando pasas el cursor encima de la casilla te muestre su nombre".
//
// Antes los nombres vivian en un `switch` dentro del HUD de depuracion (F3),
// con unas 45 entradas de mas de 160: todo lo demas salia "DESCONOCIDO". Ahora
// hay una tabla y la usan el tooltip del inventario y el HUD de depuracion. Un
// item nuevo se nombra aqui una vez y aparece en los dos.
//
// LAS REGLAS DE ESCRITURA SALEN DE LA FUENTE DEL JUEGO. `renderChar` solo
// dibuja A-Z, 0-9, espacio, '-' y '+': ni tildes, ni Ñ, ni signos. Por eso
// "PINONERO" y "PINA". Un test recorre todos los nombres y falla si alguno
// usa un caracter que la fuente no sabe dibujar (saldria como un hueco).
//
// LO QUE NO TIENE NOMBRE. Devuelve nullptr solo para el aire y para los IDs
// RESERVADOS (minerales retirados, que ya no existen en el juego). Todo lo
// demas -- incluidos los niveles parciales, las celdas mixtas y los bloques
// compuestos -- se resuelve a un nombre.

namespace NombreItem {

// El nombre del ID tal cual, sin normalizar. nullptr si no tiene.
inline const char* nombreDeBase(BlockType t) {
    switch (t) {
        // --- Terreno ---
        case BLOCK_GRASS:            return "BLOQUE DE PASTO";
        case BLOCK_GRASS_MOJADA:     return "PASTO EMPAPADO";
        case BLOCK_DIRT:             return "TIERRA";
        case BLOCK_DIRT_MOJADA:      return "TIERRA EMPAPADA";
        case BLOCK_STONE:            return "PIEDRA";
        case BLOCK_COBBLESTONE:      return "PIEDRA LABRADA";
        case BLOCK_LIMESTONE:        return "PIEDRA CALIZA";
        case BLOCK_SAND:             return "ARENA";
        case BLOCK_SAND_MOJADA:      return "ARENA EMPAPADA";
        case BLOCK_GRAVEL:           return "GRAVA";
        case BLOCK_CLAY:             return "BLOQUE DE ARCILLA";
        case BLOCK_CLAY_DIRT:        return "TIERRA ARCILLOSA";
        case BLOCK_CLAY_SAND:        return "ARENA ARCILLOSA";
        case BLOCK_SNOW:             return "NIEVE";
        case BLOCK_BEDROCK:          return "ROCA MADRE";
        case BLOCK_WATER:            return "AGUA";
        case BLOCK_LAVA:             return "LAVA";

        // --- Pino (la especie original) ---
        case BLOCK_WOOD:             return "TRONCO DE PINO";
        case BLOCK_LEAVES:           return "HOJAS DE PINO";
        case BLOCK_PLANKS:           return "TABLONES DE PINO";
        case BLOCK_RAMA_PINO:        return "RAMA DE PINO";

        // --- Encino ---
        case BLOCK_WOOD_ENCINO:      return "TRONCO DE ENCINO";
        case BLOCK_LEAVES_ENCINO:    return "HOJAS DE ENCINO";
        case BLOCK_PLANKS_ENCINO:    return "TABLONES DE ENCINO";
        case BLOCK_RAMA_ENCINO:      return "RAMA DE ENCINO";

        // --- Oyamel ---
        case BLOCK_WOOD_OYAMEL:      return "TRONCO DE OYAMEL";
        case BLOCK_LEAVES_OYAMEL:    return "HOJAS DE OYAMEL";
        case BLOCK_PLANKS_OYAMEL:    return "TABLONES DE OYAMEL";
        case BLOCK_RAMA_OYAMEL:      return "RAMA DE OYAMEL";

        // --- Ocote blanco: es el pino de Moctezuma (Pinus montezumae) ---
        case BLOCK_WOOD_OCOTE:            return "TRONCO DE PINO DE MOCTEZUMA";
        case BLOCK_WOOD_OCOTE_DENTRO:     return "PINO DE MOCTEZUMA RESINOSO";
        case BLOCK_LEAVES_OCOTE:          return "ACICULAS DE PINO DE MOCTEZUMA";
        case BLOCK_LEAVES_OCOTE_RAMA:     return "ACICULAS DE PINO DE MOCTEZUMA";
        case BLOCK_PLANKS_OCOTE:          return "TABLONES DE PINO DE MOCTEZUMA";
        case BLOCK_RAMA_OCOTE:            return "RAMA DE PINO DE MOCTEZUMA";

        // --- Ocote chino ---
        case BLOCK_WOOD_OCOTE_CHINO:        return "TRONCO DE OCOTE CHINO";
        case BLOCK_WOOD_OCOTE_CHINO_DENTRO: return "OCOTE CHINO POR DENTRO";
        case BLOCK_LEAVES_OCOTE_CHINO:      return "ACICULAS DE OCOTE CHINO";
        case BLOCK_LEAVES_OCOTE_CHINO_RAMA: return "ACICULAS DE OCOTE CHINO";

        // --- Pino patula ---
        case BLOCK_WOOD_PATULA:          return "TRONCO DE PINO PATULA";
        case BLOCK_WOOD_PATULA_DENTRO:   return "PINO PATULA POR DENTRO";
        case BLOCK_LEAVES_PATULA:        return "ACICULAS DE PINO PATULA";
        case BLOCK_PLANKS_PATULA:        return "TABLONES DE PINO PATULA";

        // --- Pino pinonero ---
        case BLOCK_WOOD_PINONERO:        return "TRONCO DE PINO PINONERO";
        case BLOCK_WOOD_PINONERO_DENTRO: return "PINO PINONERO POR DENTRO";
        case BLOCK_LEAVES_PINONERO:      return "ACICULAS DE PINO PINONERO";
        case BLOCK_PLANKS_PINONERO:      return "TABLONES DE PINO PINONERO";

        // --- Pino de las alturas ---
        case BLOCK_WOOD_ALTURAS:         return "TRONCO DE PINO DE LAS ALTURAS";
        case BLOCK_WOOD_ALTURAS_DENTRO:  return "PINO DE LAS ALTURAS POR DENTRO";
        case BLOCK_LEAVES_ALTURAS:       return "ACICULAS DE PINO DE LAS ALTURAS";
        case BLOCK_PLANKS_ALTURAS:       return "TABLONES DE PINO DE LAS ALTURAS";

        // --- Pino pseudostrobus ---
        case BLOCK_WOOD_PSEUDOSTROBUS:        return "TRONCO DE PSEUDOSTROBUS";
        case BLOCK_WOOD_PSEUDOSTROBUS_DENTRO: return "PSEUDOSTROBUS POR DENTRO";
        case BLOCK_LEAVES_PSEUDOSTROBUS:      return "ACICULAS DE PSEUDOSTROBUS";
        case BLOCK_PLANKS_PSEUDOSTROBUS:      return "TABLONES DE PSEUDOSTROBUS";

        // --- Raices ---
        case BLOCK_RAIZ_PEQUENA:     return "RAIZ PEQUENA";
        case BLOCK_RAIZ_MEDIANA:     return "RAIZ MEDIANA";
        case BLOCK_RAIZ_GRANDE:      return "RAIZ GRANDE";
        case BLOCK_RAIZ_ENORME:      return "RAIZ ENORME";

        // --- Hierba y flores ---
        case BLOCK_TALLGRASS:        return "HIERBA CORTA";
        case BLOCK_ORANGE_FLOWER:    return "FLOR NARANJA";

        // --- Nopal de Castilla ---
        case BLOCK_NOPAL_BASE_PASTO:     return "TALLO DE NOPAL EN PASTO";
        case BLOCK_NOPAL_BASE_TIERRA:    return "TALLO DE NOPAL EN TIERRA";
        case BLOCK_NOPAL_BASE_ARENA:     return "TALLO DE NOPAL EN ARENA";
        case BLOCK_NOPAL_BASE_T_ARCILLA: return "TALLO DE NOPAL EN TIERRA ARCILLOSA";
        case BLOCK_NOPAL_BASE_A_ARCILLA: return "TALLO DE NOPAL EN ARENA ARCILLOSA";
        case BLOCK_NOPAL_TALLO:          return "TALLO DE NOPAL";
        case BLOCK_NOPAL_FRUTO:          return "NOPAL DE CASTILLA";
        case BLOCK_NOPAL_CLADODIO:       return "PENCA DE NOPAL";
        case BLOCK_NOPAL_CLADODIO_X2:    return "PENCAS DE NOPAL";
        case BLOCK_NOPAL_CLADODIO_X3:    return "PENCAS DE NOPAL";
        case BLOCK_NOPAL_CLADODIO_Z2:    return "PENCAS DE NOPAL";
        case BLOCK_NOPAL_CLADODIO_Z3:    return "PENCAS DE NOPAL";
        case BLOCK_NOPAL_CLADODIO_DIAG:  return "PENCA DE NOPAL";
        case BLOCK_NOPAL_MOJADO:         return "PENCA MOJADA SIN ESPINAS";
        case BLOCK_NOPAL_SECO:           return "PENCA SECA SIN ESPINAS";
        case BLOCK_NOPAL_TIRAS:          return "PENCA EN TIRAS";
        case BLOCK_NOPAL_SIN_BABA:       return "TIRAS DE NOPAL SIN BABA";
        case BLOCK_NOPAL_BABA:           return "BABA DE NOPAL";
        case BLOCK_ESPINAS_NOPAL:        return "ESPINAS DE NOPAL";
        case BLOCK_TUNA:                 return "TUNA VERDE";
        case BLOCK_TUNA_AMARILLA:        return "TUNA AMARILLA";
        case BLOCK_TUNA_ROJA:            return "TUNA ROJA";

        // --- Ixtle ---
        case BLOCK_IXTLE_TALLO:        return "BASE DE IXTLE";
        case BLOCK_IXTLE_TALLO_ARENA:  return "BASE DE IXTLE EN ARENA";
        case BLOCK_IXTLE_HOJA:         return "HOJA DE IXTLE";
        case BLOCK_IXTLE_PUNTA:        return "PUNTA DE IXTLE";
        case BLOCK_IXTLE_CON_HIERBA:   return "IXTLE CON HIERBA";
        case BLOCK_IXTLE_CON_FLOR:     return "IXTLE CON FLOR";
        case BLOCK_IXTLE_DOBLE:        return "IXTLE DOBLE";
        case BLOCK_IXTLE_PEQUENA:      return "IXTLE PEQUENO";
        case BLOCK_IXTLE_GRANDE:       return "IXTLE GRANDE";
        case BLOCK_IXTLE_ENORME:       return "IXTLE ENORME";
        case BLOCK_HILO_IXTLE:         return "HILO DE IXTLE";

        // --- Maguey pulquero ---
        case BLOCK_MAGUEY_PUNTA:       return "PUNTA DE MAGUEY";
        case BLOCK_MAGUEY_HUECO:       return "MAGUEY CAPADO";
        case BLOCK_AGUAMIEL:           return "AGUAMIEL";

        // --- Agave tequilana azul ---
        case BLOCK_AGAVE_AZUL_PENCA:   return "HOJA DE AGAVE AZUL";
        case BLOCK_PENCA_AGAVE_AZUL:   return "PENCA DE AGAVE AZUL";
        case BLOCK_AGAVE_AZUL_PUNTA:   return "PUNTA DE AGAVE AZUL";
        case BLOCK_AGAVE_AZUL_PINA:    return "PINA DE AGAVE AZUL";
        case BLOCK_AGAVE_AZUL_QUIOTE:  return "QUIOTE DE AGAVE AZUL";
        case BLOCK_AGAVE_AZUL_FLOR:    return "FLOR DE AGAVE AZUL";

        // --- Agave abisaii ---
        case BLOCK_AGAVE_ABISAII_PENCA: return "HOJA DE AGAVE ABISAII";
        case BLOCK_AGAVE_ABISAII_PUNTA: return "PUNTA DE AGAVE ABISAII";

        // --- Techo de hojas (las dos orientaciones son el mismo item) ---
        case BLOCK_TECHO_HOJAS:        return "TECHO DE HOJAS";
        case BLOCK_TECHO_SUELO:        return "TECHO DE HOJAS";

        // --- Cosas sueltas del suelo ---
        case BLOCK_PEDAZO_PIEDRA:      return "PEDAZO DE PIEDRA";
        case BLOCK_PEDAZO_GRAVA:       return "GRAVA SUELTA";
        case BLOCK_PEDAZO_PEDERNAL:    return "PEDERNAL";
        case BLOCK_PEDAZO_CALIZA:      return "PEDAZO DE CALIZA";
        case BLOCK_PEDAZO_TIERRA:      return "TERRON DE TIERRA";
        case BLOCK_PEDAZO_COBRE:       return "COBRE CRUDO";
        case BLOCK_PEDAZO_GOETHITA:    return "GOETHITA";
        case BLOCK_PEDAZO_HEMATITE:    return "HEMATITA";
        case BLOCK_PEDAZO_LIMONITA:    return "LIMONITA";
        case BLOCK_PEDAZO_NIEVE:       return "PUNADO DE NIEVE";
        case BLOCK_PEDAZO_BARRO:       return "PEDAZO DE BARRO";

        // --- Materiales ---
        case BLOCK_DIRT_POWDER:        return "POLVO DE TIERRA";
        case BLOCK_STICK:              return "PALO";
        case BLOCK_COAL_ITEM:          return "CARBON";
        case BLOCK_RAW_ZINC:           return "ZINC CRUDO";
        case BLOCK_RAW_COPPER:         return "COBRE CRUDO";
        case BLOCK_PEDERNAL_AFILADO:   return "PEDERNAL AFILADO";
        case BLOCK_IRON_ORE:           return "MINERAL DE HIERRO";
        case BLOCK_BRICKS:             return "LADRILLOS";
        case BLOCK_GLASS:              return "VIDRIO";

        // --- Herramientas ---
        case BLOCK_HOE:                return "HOZ";
        case BLOCK_HACHA_PIEDRA:       return "HACHA DE PIEDRA";
        case BLOCK_MARTILLO_PIEDRA:    return "MARTILLO DE PIEDRA";
        case BLOCK_PICO_PIEDRA:        return "PICO DE PIEDRA";
        case BLOCK_HACHA_PEDERNAL:     return "HACHA DE PEDERNAL";
        case BLOCK_PICO_PEDERNAL:      return "PICO DE PEDERNAL";
        case BLOCK_MARTILLO_PEDERNAL:  return "MARTILLO DE PEDERNAL";

        // --- Horno ---
        case BLOCK_HORNO:              return "HORNO DE BARRO";
        case BLOCK_HORNO_ENCENDIDO:    return "HORNO DE BARRO ENCENDIDO";

        // --- Tazones ---
        case BLOCK_TAZON_PINO:            return "TAZON DE PINO";
        case BLOCK_TAZON_ENCINO:          return "TAZON DE ENCINO";
        case BLOCK_TAZON_OYAMEL:          return "TAZON DE OYAMEL";
        case BLOCK_TAZON_PINO_AGUA:       return "TAZON DE PINO CON AGUA";
        case BLOCK_TAZON_ENCINO_AGUA:     return "TAZON DE ENCINO CON AGUA";
        case BLOCK_TAZON_OYAMEL_AGUA:     return "TAZON DE OYAMEL CON AGUA";
        case BLOCK_TAZON_PINO_AGUAMIEL:   return "TAZON DE PINO CON AGUAMIEL";
        case BLOCK_TAZON_ENCINO_AGUAMIEL: return "TAZON DE ENCINO CON AGUAMIEL";
        case BLOCK_TAZON_OYAMEL_AGUAMIEL: return "TAZON DE OYAMEL CON AGUAMIEL";

        // --- Fauna ---
        case BLOCK_HUEVO_PECARI:       return "HUEVO DE PECARI";

        default:                       return nullptr;
    }
}

// Los bloques compuestos se nombran por su familia: el estado (etapa,
// aguamiel, nivel del agua) no cambia QUE es la cosa.
inline const char* nombreDeFamilia(Compuesto::Familia f) {
    switch (f) {
        case Compuesto::FAM_MAGUEY:        return "MAGUEY PULQUERO";
        case Compuesto::FAM_ARBOL_FRUTO:   return "ARBOL FRUTAL";
        case Compuesto::FAM_PIEDRA_MUSGO:  return "PIEDRA CON MUSGO";
        case Compuesto::FAM_PLANTA_FLOR:   return "PLANTA CON FLOR";
        case Compuesto::FAM_BIZNAGA:       return "BIZNAGA";
        case Compuesto::FAM_AGUA:          return "AGUA";
        case Compuesto::FAM_AGAVE_AZUL:    return "AGAVE TEQUILANA AZUL";
        case Compuesto::FAM_AGAVE_ABISAII: return "AGAVE ABISAII";
        case Compuesto::FAM_NOPAL:         return "NOPAL DE CASTILLA";
        default:                           return nullptr;
    }
}

// El nombre que ve el jugador. Normaliza antes de buscar:
//   - un nivel parcial se llama como su bloque (una capa de tierra es TIERRA);
//   - una celda mixta, como el material de abajo, que es el que la define;
//   - un compuesto, por su familia.
inline const char* nombreDeItem(BlockType t) {
    if (t == BLOCK_AIR) return nullptr;
    if (Compuesto::esCompuesto(t)) return nombreDeFamilia(Compuesto::familiaDe(t));
    if (esMixto(t))                t = mixtoBase(t);
    if (esNivelParcial(t))         t = bloqueBaseDe(t);

    // Las capas de TRONCO siguen en el enum pero ya no son niveles: los
    // troncos salieron de la tabla de niveles ("un tronco es un tronco
    // entero"), asi que esNivelParcial no las reconoce. No se pueden
    // conseguir hoy, pero un mundo viejo podria tenerlas: se llaman como su
    // tronco en vez de quedarse sin nombre.
    const int i = (int)t;
    if (i >= BLOCK_WOOD_L1  && i <= BLOCK_WOOD_L7)  t = BLOCK_WOOD;
    if (i >= BLOCK_WOODE_L1 && i <= BLOCK_WOODE_L7) t = BLOCK_WOOD_ENCINO;
    if (i >= BLOCK_WOODO_L1 && i <= BLOCK_WOODO_L7) t = BLOCK_WOOD_OYAMEL;

    return nombreDeBase(t);
}

// ¿Sabe la fuente del juego dibujar este caracter? (ver renderChar)
inline bool caracterDibujable(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
           c == ' ' || c == '-' || c == '+';
}

} // namespace NombreItem
