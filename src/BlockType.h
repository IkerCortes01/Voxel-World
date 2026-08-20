#pragma once

#include <cstdint>

// ============================================================================
// TIPOS DE BLOQUES
// ============================================================================
// Extraído de main.cpp para que PalettedStorage.h y los tests puedan usar el
// enum real sin arrastrar el resto del motor.
//
// Antes, PalettedStorage.h dependía de que BlockType estuviese definido en el
// punto exacto de su #include: una dependencia de orden invisible que rompía la
// compilación si alguien movía la línea. Ahora el header se basta a sí mismo.
//
// ----------------------------------------------------------------------------
// ORDEN DE LOS IDs
// ----------------------------------------------------------------------------
// Hasta ahora los bloques nuevos se añadían SIEMPRE al final, porque insertar
// en medio desplaza los IDs y corrompe los mundos guardados. Esta lista es una
// REORDENACIÓN COMPLETA y deliberada: agrupa los bloques por familia (troncos
// juntos, hojas juntas, tablones juntos...) en lugar de por orden histórico.
//
// Los mundos guardados con el orden viejo NO se pierden: BlockCompat.h traduce
// los IDs antiguos a los nuevos al cargar (ver SAVE_VERSION en SaveSystem.h).
// ============================================================================

enum BlockType {
    BLOCK_AIR = 0,

    // --- Terreno base ---
    BLOCK_GRASS,            // 1  Bloque de pasto
    BLOCK_DIRT,             // 2  Tierra
    BLOCK_STONE,            // 3  Piedra

    // --- Troncos (una especie por ID, juntos) ---
    BLOCK_WOOD,             // 4  Tronco de pino (la especie original)
    BLOCK_WOOD_ENCINO,      // 5  Tronco de encino
    BLOCK_WOOD_OYAMEL,      // 6  Tronco de oyamel

    BLOCK_WATER,            // 7  Agua

    // --- Minerales comunes ---
    BLOCK_COAL_ORE,         // 8  Mineral de carbón
    BLOCK_SCRAP_METAL,      // 9  Desecho de metales

    // --- Sedimentos ---
    BLOCK_SAND,             // 10 Arena
    BLOCK_GRAVEL,           // 11 Grava

    // --- Hojas (juntas, en el mismo orden que los troncos) ---
    BLOCK_LEAVES,           // 12 Hojas de pino
    BLOCK_LEAVES_ENCINO,    // 13 Hojas de encino
    BLOCK_LEAVES_OYAMEL,    // 14 Hojas de oyamel

    BLOCK_TALLGRASS,        // 15 Hierba corta
    BLOCK_SNOW,             // 16 Nieve

    // --- Tablones (juntos, mismo orden que troncos y hojas) ---
    BLOCK_PLANKS,           // 17 Tablones de pino
    BLOCK_PLANKS_ENCINO,    // 18 Tablones de encino
    BLOCK_PLANKS_OYAMEL,    // 19 Tablones de oyamel

    // --- Minerales raros ---
    BLOCK_SILVER_ORE,       // 20 Mineral de plata
    BLOCK_DIAMOND_ORE,      // 21 Mineral de diamante

    BLOCK_COBBLESTONE,      // 22 Piedra labrada
    BLOCK_LAVA,             // 23 Lava
    BLOCK_CLAY,             // 24 Bloque de arcilla

    // --- Bloques nuevos ---
    BLOCK_LIMESTONE,        // 25 Piedra caliza
    BLOCK_GOLD_ORE,         // 26 Mineral de oro
    BLOCK_CLAY_DIRT,        // 27 Tierra arcillosa
    BLOCK_CLAY_SAND,        // 28 Arena arcillosa

    // ------------------------------------------------------------------
    // NOPAL DE CASTILLA
    // ------------------------------------------------------------------
    // La planta se compone de tres piezas:
    //   BASE     - bloque COMPLETO donde el tallo arranca del suelo. Su
    //              textura lleva el terreno incrustado, por eso hay una
    //              variante por tipo de suelo.
    //   TALLO    - bloque COMPLETO: la columna carnosa que sube.
    //   CLADODIO - SPRITE 3D atravesable: las pencas que salen a los lados.
    //
    // BASES: bloques COMPLETOS. Cada una lleva el terreno en su textura
    // (medio bloque de suelo, medio de tallo), asi que hay una por tipo de
    // suelo y se elige al generar, no al dibujar.
    BLOCK_NOPAL_BASE_PASTO,      // 29 Tallo en pasto (el mas comun)
    BLOCK_NOPAL_BASE_TIERRA,     // 30 Tallo en tierra
    BLOCK_NOPAL_BASE_ARENA,      // 31 Tallo en arena
    BLOCK_NOPAL_BASE_T_ARCILLA,  // 32 Tallo en tierra arcillosa (poco comun)
    BLOCK_NOPAL_BASE_A_ARCILLA,  // 33 Tallo en arena arcillosa (poco comun)

    BLOCK_NOPAL_TALLO,           // 34 Tallo de nopal (bloque COMPLETO)
    BLOCK_NOPAL_CLADODIO,        // 35 Cladodio: sprite 3D atravesable

    // El FRUTO (la tuna). Cambia de textura cuando esta pegado a un
    // cladodio, conservando la misma forma: es el mismo bloque, solo se
    // dibuja distinto segun el vecino.
    BLOCK_NOPAL_FRUTO,           // 36 Nopal de Castilla (fruto)

    // ------------------------------------------------------------------
    // RAMAS DE LOS ARBOLES
    // ------------------------------------------------------------------
    // Una rama NO llena su voxel: es un palo de 4x4 pixeles (4/16 de bloque)
    // que atraviesa la celda. Su forma la decide el mesher a partir de los
    // vecinos, igual que el cladodio del nopal, asi que con UN solo bloque
    // por especie salen decenas de miles de siluetas distintas sin gastar
    // IDs ni memoria.
    //
    // Una por especie, para que cada arbol use la textura de SU tronco.
    BLOCK_RAMA_PINO,             // 37 Rama de pino
    BLOCK_RAMA_ENCINO,           // 38 Rama de encino
    BLOCK_RAMA_OYAMEL,           // 39 Rama de oyamel

    // ------------------------------------------------------------------
    // LA TUNA
    // ------------------------------------------------------------------
    // Bloque PROPIO, independiente del cladodio: se genera creciendo sobre
    // una penca, pero el jugador la selecciona y la rompe por separado, y al
    // romperla el cladodio se queda intacto. Ocupa su voxel con un bulto de
    // 6x4x4 pixeles apoyado en la penca de abajo.
    //
    // Hay TRES variedades, y cada una es un bloque distinto para que el
    // jugador pueda conseguir las tres por separado: al romper una tuna roja
    // recoge una tuna roja, no una generica. Se corresponden con las
    // variedades que se cultivan en Mexico.
    //
    // Van al final para no desplazar ningun ID anterior: las partidas ya
    // guardadas siguen leyendose igual.
    BLOCK_TUNA,                  // 40 Tuna verde (blanca / Alfajayucan)
    BLOCK_TUNA_AMARILLA,         // 41 Tuna amarilla
    BLOCK_TUNA_ROJA,             // 42 Tuna roja (cardona)

    // ------------------------------------------------------------------
    // PENCAS APILADAS
    // ------------------------------------------------------------------
    // En un mismo voxel caben hasta TRES pencas. Como el chunk solo guarda
    // un BlockType por bloque (sin metadatos), la cantidad ES el propio ID:
    // asi el apilado no cuesta ni un byte extra y se guarda solo.
    //
    // El eje (ancho o largo) tambien va en el ID, porque el jugador decide al
    // colocar si las junta de lado o en profundidad.
    BLOCK_NOPAL_CLADODIO_X2,     // 43 Dos pencas juntas, en ancho
    BLOCK_NOPAL_CLADODIO_X3,     // 44 Tres pencas juntas, en ancho
    BLOCK_NOPAL_CLADODIO_Z2,     // 45 Dos pencas juntas, en largo
    BLOCK_NOPAL_CLADODIO_Z3,     // 46 Tres pencas juntas, en largo

    // Penca INCLINADA: crece en diagonal en vez de recta. Es lo que da a la
    // planta las formas dobladas y en codo de un nopal real.
    BLOCK_NOPAL_CLADODIO_DIAG,   // 47 Penca en diagonal

    // ------------------------------------------------------------------
    // RAICES 3D
    // ------------------------------------------------------------------
    // Funcionan como las ramas -- un nucleo con brazos hacia los vecinos
    // conectados -- pero crecen HACIA ABAJO, agarradas al suelo, y vienen en
    // cuatro grosores. Una raiz real se engrosa al acercarse al tronco, asi
    // que la fina va en la punta y la gruesa junto a la base del arbol.
    //
    //   PEQUENA   4x4 px   las raicillas del extremo
    //   MEDIANA   8x8 px   el tramo intermedio
    //   GRANDE   12x12 px  las principales
    //   ENORME   16x16 px  el arranque, pegado al tronco
    BLOCK_RAIZ_PEQUENA,          // 48 Raiz de 4x4
    BLOCK_RAIZ_MEDIANA,          // 49 Raiz de 8x8
    BLOCK_RAIZ_GRANDE,           // 50 Raiz de 12x12
    BLOCK_RAIZ_ENORME,           // 51 Raiz de 16x16

    // ------------------------------------------------------------------
    // NOPAL MOJADO
    // ------------------------------------------------------------------
    // Una penca metida en agua pierde las espinas al instante y se
    // convierte en esto. Es el nopal ya limpio, listo para comer, que es lo
    // que se hace en la cocina mexicana: se lavan y se desespinan.
    //
    // Se rompe en 1.5 s, mucho mas rapido que la penca con espinas.
    BLOCK_NOPAL_MOJADO,          // 52 Nopal mojado, sin espinas

    // Nopal cortado en TIRAS. Se craftea a partir del nopal mojado (uno da
    // cuatro) y SI se puede colocar en el mundo: dejandolo en agua 10
    // segundos se le va la baba, que es lo que se hace al cocinarlo.
    BLOCK_NOPAL_TIRAS,           // 53 Penca en tiras

    // Lo que sale de desbabar las tiras en agua: las tiras ya limpias y la
    // baba por separado. Cada tira da una desbabada y dos babas.
    BLOCK_NOPAL_SIN_BABA,        // 54 Tiras sin baba, mojadas
    BLOCK_NOPAL_BABA,            // 55 Baba de nopal

    // ------------------------------------------------------------------
    // IXTLE (lechuguilla)
    // ------------------------------------------------------------------
    // Agave de roseta que crece en el desierto mexicano, del que se saca la
    // fibra de ixtle. Se compone de tres piezas:
    //
    //   TALLO   la base carnosa de la que salen las hojas
    //   HOJA    el cuerpo de la hoja, rigido y erecto
    //   PUNTA   el remate en espina de cada hoja
    BLOCK_IXTLE_TALLO,           // 56 Base de la roseta
    BLOCK_IXTLE_HOJA,            // 57 Cuerpo de la hoja
    BLOCK_IXTLE_PUNTA,           // 58 Punta en espina

    // ========================================================================
    // ITEMS
    // ========================================================================
    // No son bloques colocables del terreno: viven en el enum porque el
    // inventario los trata igual. Van DESPUÉS del último bloque para que la
    // lista de bloques (0..BLOCK_LAST_PLACEABLE) sea contigua.
    BLOCK_DIRT_POWDER,      // 59 Polvo de tierra
    BLOCK_STICK,            // 60 Palo
    BLOCK_HOE,              // 61 Hoz
    BLOCK_COAL_ITEM,        // 62 Carbón (item)
    BLOCK_RAW_ZINC,         // 63 Zinc crudo
    BLOCK_RAW_COPPER,       // 64 Cobre crudo

    // ========================================================================
    // RETIRADOS
    // ========================================================================
    // BLOCK_IRON_ORE y BLOCK_BRICKS/BLOCK_GLASS existían pero nunca llegaron a
    // implementarse (sin textura, sin dureza, sin generación). Se conservan al
    // final, fuera del rango util, para que el código que aún los menciona
    // siga compilando sin ocupar un ID de la lista buena.
    BLOCK_IRON_ORE,         // 65 (sin implementar)
    BLOCK_BRICKS,           // 66 (sin implementar)
    BLOCK_GLASS,            // 67 (sin implementar)
    BLOCK_ORANGE_FLOWER,    // 68 (retirado del juego)
    // El bedrock ya no se genera en el terreno, pero el motor aún lo consulta
    // (p.ej. para no aplastar al jugador contra el fondo del mundo).
    BLOCK_BEDROCK           // 69 (ya no se genera)
};

// Último bloque COLOCABLE de la lista ordenada (la punta de ixtle).
// Lo que va después son items y bloques retirados.
constexpr int BLOCK_LAST_PLACEABLE = BLOCK_IXTLE_PUNTA;

// ¿Es una raíz, de cualquiera de los cuatro grosores?
inline bool esRaiz(BlockType t) {
    return t == BLOCK_RAIZ_PEQUENA || t == BLOCK_RAIZ_MEDIANA ||
           t == BLOCK_RAIZ_GRANDE  || t == BLOCK_RAIZ_ENORME;
}

// Grosor de la raíz en píxeles (4, 8, 12 o 16).
inline int grosorRaiz(BlockType t) {
    switch (t) {
        case BLOCK_RAIZ_PEQUENA: return 4;
        case BLOCK_RAIZ_MEDIANA: return 8;
        case BLOCK_RAIZ_GRANDE:  return 12;
        case BLOCK_RAIZ_ENORME:  return 16;
        default:                 return 0;
    }
}

// ¿Es una pieza de ixtle (lechuguilla), de cualquiera de las tres?
inline bool esIxtle(BlockType t) {
    return t == BLOCK_IXTLE_TALLO || t == BLOCK_IXTLE_HOJA ||
           t == BLOCK_IXTLE_PUNTA;
}

// ¿Es una tuna, de cualquiera de las tres variedades? Se usa en todos los
// sitios donde el comportamiento es el mismo (forma, colisión, luz) y solo
// cambia la textura.
inline bool esTuna(BlockType t) {
    return t == BLOCK_TUNA || t == BLOCK_TUNA_AMARILLA || t == BLOCK_TUNA_ROJA;
}

// ¿Es una penca (cladodio), en cualquiera de sus formas? Todas comparten
// textura, dureza y comportamiento; solo cambia cuántas caben y cómo se
// inclinan.
inline bool esCladodio(BlockType t) {
    return t == BLOCK_NOPAL_CLADODIO ||
           t == BLOCK_NOPAL_CLADODIO_X2 || t == BLOCK_NOPAL_CLADODIO_X3 ||
           t == BLOCK_NOPAL_CLADODIO_Z2 || t == BLOCK_NOPAL_CLADODIO_Z3 ||
           t == BLOCK_NOPAL_CLADODIO_DIAG;
}

// Cuántas pencas hay apiladas en este bloque (1..3).
inline int pencasApiladas(BlockType t) {
    if (t == BLOCK_NOPAL_CLADODIO_X3 || t == BLOCK_NOPAL_CLADODIO_Z3) return 3;
    if (t == BLOCK_NOPAL_CLADODIO_X2 || t == BLOCK_NOPAL_CLADODIO_Z2) return 2;
    return 1;
}

// ¿Se apilan a lo ANCHO (eje X) o a lo LARGO (eje Z)?
inline bool apiladoEnX(BlockType t) {
    return t == BLOCK_NOPAL_CLADODIO_X2 || t == BLOCK_NOPAL_CLADODIO_X3;
}

// El siguiente escalón al añadir una penca más. Devuelve AIR si ya está lleno.
inline BlockType apilarPenca(BlockType actual, bool enX) {
    if (actual == BLOCK_NOPAL_CLADODIO)
        return enX ? BLOCK_NOPAL_CLADODIO_X2 : BLOCK_NOPAL_CLADODIO_Z2;
    if (actual == BLOCK_NOPAL_CLADODIO_X2) return BLOCK_NOPAL_CLADODIO_X3;
    if (actual == BLOCK_NOPAL_CLADODIO_Z2) return BLOCK_NOPAL_CLADODIO_Z3;
    return BLOCK_AIR;   // X3, Z3 y la diagonal ya no admiten mas
}

// Último valor válido del enum: se usa para validar los datos leídos de
// archivos, donde un blockType fuera de rango llega desde disco y no del juego.
// ⚠️ Actualizar si se añaden bloques al final del enum.
constexpr int BLOCK_TYPE_MAX = BLOCK_BEDROCK;
