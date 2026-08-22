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

    // ------------------------------------------------------------------
    // DOS BLOQUES EN EL MISMO ESPACIO
    // ------------------------------------------------------------------
    // El chunk guarda UN BlockType por celda y no tiene sitio para
    // metadatos, asi que "dos bloques a la vez" no se puede representar
    // con dos valores... pero SI con uno que signifique la pareja.
    //
    // Cada uno de estos IDs es una COMBINACION: hoja de ixtle conviviendo
    // con otra planta en el mismo voxel. El mesher dibuja las dos piezas y
    // el raycast decide cual seleccionas segun la caja que atraviese tu
    // mirada, asi que se rompen por separado aunque compartan celda.
    //
    // Se eligen estas parejas porque son las que tienen sentido: la roseta
    // del ixtle es abierta, con huecos entre hoja y hoja, de modo que una
    // mata de hierba o un brote pequeno caben ahi sin solaparse.
    //
    // Van al final del enum para no desplazar ningun ID anterior: las
    // partidas ya guardadas se siguen leyendo igual.
    BLOCK_IXTLE_CON_HIERBA,      // 59 Hoja de ixtle + hierba corta
    BLOCK_IXTLE_CON_FLOR,        // 60 Hoja de ixtle + flor
    BLOCK_IXTLE_DOBLE,           // 61 Dos hojas de ixtle cruzadas

    // ------------------------------------------------------------------
    // TAMAÑOS DE LA MATA
    // ------------------------------------------------------------------
    // Una lechuguilla no nace adulta: brota pequeña y va creciendo. Cada
    // tamaño es un bloque distinto porque la forma cambia -- no solo la
    // escala, tambien cuantas hojas tiene y cuanto se abren.
    //
    // El tamaño MEDIANO es el que ya existia (BLOCK_IXTLE_HOJA), asi que
    // no se repite aqui: sirve de tamaño por defecto.
    BLOCK_IXTLE_PEQUENA,         // 62 Recien brotada
    BLOCK_IXTLE_GRANDE,          // 63 Mata hecha
    BLOCK_IXTLE_ENORME,          // 64 Ejemplar viejo

    // El TALLO sobre ARENA: la lechuguilla es planta de desierto y la
    // mayoria crece en suelo arenoso. Es la misma cara pintada sobre el
    // terreno, pero con la textura que corresponde a la arena.
    BLOCK_IXTLE_TALLO_ARENA,     // 65 Suelo de la mata, en arena

    // ------------------------------------------------------------------
    // PEDAZOS DE PIEDRA
    // ------------------------------------------------------------------
    // Guijarros sueltos en el suelo. NO llenan su voxel: son un montoncito
    // de cantos de 14x9 px (medido en la textura) esparcidos por la celda.
    //
    // Su forma NO se guarda: sale de un hash de la posicion, asi que hay
    // miles de disposiciones distintas -- cuantas piedras, de que tamaño,
    // donde, y giradas cuanto -- sin gastar un solo byte por bloque ni un
    // ID por variante.
    BLOCK_PEDAZO_PIEDRA,         // 66 Piedras pequeñas del suelo

    // Los demás guijarros. Comparten TODO con el de piedra -- la forma, las
    // miles de disposiciones, la colisión -- y solo cambia la textura, así
    // que el mesher los trata igual y no hay geometría duplicada.
    //
    // Aparecen con la misma probabilidad entre ellos: cuando el suelo saca
    // guijarros, cuál toca se decide al azar entre los que encajan con ese
    // terreno.
    BLOCK_PEDAZO_GRAVA,          // 67 Grava suelta
    BLOCK_PEDAZO_PEDERNAL,       // 68 Pedernal (junto a grava y agua)
    BLOCK_PEDAZO_CALIZA,         // 69 Pedazos de piedra caliza
    BLOCK_PEDAZO_TIERRA,         // 70 Terrones de tierra suelta
    BLOCK_PEDAZO_COBRE,          // 71 Cobre crudo del suelo

    // ------------------------------------------------------------------
    // NIVELES DE BLOQUE (capas acumulables)
    // ------------------------------------------------------------------
    // Un bloque de terreno no tiene por que llenar su voxel: puede ser una
    // capa fina que se va acumulando. Hay OCHO niveles, y cada uno mide lo
    // suyo en pixeles:
    //
    //     nivel 1 ->  3 px      nivel 5 ->  8 px
    //     nivel 2 ->  4 px      nivel 6 -> 10 px
    //     nivel 3 ->  5 px      nivel 7 -> 13 px
    //     nivel 4 ->  6 px      nivel 8 -> 16 px (el bloque de siempre)
    //
    // El nivel 8 NO gasta ID: es el bloque normal que ya existia. Solo los
    // siete parciales necesitan uno, y solo para las familias de TERRENO,
    // que son las que se ven al caminar.
    //
    // Van en bloques de siete seguidos para que el nivel se saque del ID con
    // una resta, sin tabla: nivel = (id - primero) + 1.
    BLOCK_GRASS_L1, BLOCK_GRASS_L2, BLOCK_GRASS_L3, BLOCK_GRASS_L4,
    BLOCK_GRASS_L5, BLOCK_GRASS_L6, BLOCK_GRASS_L7,          // 72-78

    BLOCK_DIRT_L1, BLOCK_DIRT_L2, BLOCK_DIRT_L3, BLOCK_DIRT_L4,
    BLOCK_DIRT_L5, BLOCK_DIRT_L6, BLOCK_DIRT_L7,             // 79-85

    BLOCK_STONE_L1, BLOCK_STONE_L2, BLOCK_STONE_L3, BLOCK_STONE_L4,
    BLOCK_STONE_L5, BLOCK_STONE_L6, BLOCK_STONE_L7,          // 86-92

    BLOCK_SAND_L1, BLOCK_SAND_L2, BLOCK_SAND_L3, BLOCK_SAND_L4,
    BLOCK_SAND_L5, BLOCK_SAND_L6, BLOCK_SAND_L7,             // 93-99

    BLOCK_GRAVEL_L1, BLOCK_GRAVEL_L2, BLOCK_GRAVEL_L3, BLOCK_GRAVEL_L4,
    BLOCK_GRAVEL_L5, BLOCK_GRAVEL_L6, BLOCK_GRAVEL_L7,       // 100-106

    BLOCK_SNOW_L1, BLOCK_SNOW_L2, BLOCK_SNOW_L3, BLOCK_SNOW_L4,
    BLOCK_SNOW_L5, BLOCK_SNOW_L6, BLOCK_SNOW_L7,             // 107-113

    BLOCK_CLAYD_L1, BLOCK_CLAYD_L2, BLOCK_CLAYD_L3, BLOCK_CLAYD_L4,
    BLOCK_CLAYD_L5, BLOCK_CLAYD_L6, BLOCK_CLAYD_L7,          // 114-120

    BLOCK_CLAYS_L1, BLOCK_CLAYS_L2, BLOCK_CLAYS_L3, BLOCK_CLAYS_L4,
    BLOCK_CLAYS_L5, BLOCK_CLAYS_L6, BLOCK_CLAYS_L7,          // 121-127

    // ------------------------------------------------------------------
    // NIVELES PARA EL RESTO DE BLOQUES MACIZOS
    // ------------------------------------------------------------------
    // Al principio solo ocho familias de terreno tenian niveles. Cualquier
    // otro bloque -- piedra labrada, tablones, arcilla, un mineral -- se
    // colocaba SIEMPRE entero: pedir "nivel 3" devolvia el bloque completo
    // y la capa fina no existia.
    //
    // Aqui se completan TODOS los macizos que quedaban, con el mismo
    // esquema de siete IDs seguidos, para que cualquier bloque se pueda
    // apilar por capas igual que la tierra.
    //
    // Van al final para no desplazar ningun ID anterior: las partidas
    // guardadas se siguen leyendo igual.
    BLOCK_COBBLE_L1, BLOCK_COBBLE_L2, BLOCK_COBBLE_L3, BLOCK_COBBLE_L4,
    BLOCK_COBBLE_L5, BLOCK_COBBLE_L6, BLOCK_COBBLE_L7,

    BLOCK_CLAY_L1, BLOCK_CLAY_L2, BLOCK_CLAY_L3, BLOCK_CLAY_L4,
    BLOCK_CLAY_L5, BLOCK_CLAY_L6, BLOCK_CLAY_L7,

    BLOCK_LIME_L1, BLOCK_LIME_L2, BLOCK_LIME_L3, BLOCK_LIME_L4,
    BLOCK_LIME_L5, BLOCK_LIME_L6, BLOCK_LIME_L7,

    BLOCK_PLANKS_L1, BLOCK_PLANKS_L2, BLOCK_PLANKS_L3, BLOCK_PLANKS_L4,
    BLOCK_PLANKS_L5, BLOCK_PLANKS_L6, BLOCK_PLANKS_L7,

    BLOCK_PLANKE_L1, BLOCK_PLANKE_L2, BLOCK_PLANKE_L3, BLOCK_PLANKE_L4,
    BLOCK_PLANKE_L5, BLOCK_PLANKE_L6, BLOCK_PLANKE_L7,

    BLOCK_PLANKO_L1, BLOCK_PLANKO_L2, BLOCK_PLANKO_L3, BLOCK_PLANKO_L4,
    BLOCK_PLANKO_L5, BLOCK_PLANKO_L6, BLOCK_PLANKO_L7,

    BLOCK_WOOD_L1, BLOCK_WOOD_L2, BLOCK_WOOD_L3, BLOCK_WOOD_L4,
    BLOCK_WOOD_L5, BLOCK_WOOD_L6, BLOCK_WOOD_L7,

    BLOCK_WOODE_L1, BLOCK_WOODE_L2, BLOCK_WOODE_L3, BLOCK_WOODE_L4,
    BLOCK_WOODE_L5, BLOCK_WOODE_L6, BLOCK_WOODE_L7,

    BLOCK_WOODO_L1, BLOCK_WOODO_L2, BLOCK_WOODO_L3, BLOCK_WOODO_L4,
    BLOCK_WOODO_L5, BLOCK_WOODO_L6, BLOCK_WOODO_L7,

    BLOCK_COAL_L1, BLOCK_COAL_L2, BLOCK_COAL_L3, BLOCK_COAL_L4,
    BLOCK_COAL_L5, BLOCK_COAL_L6, BLOCK_COAL_L7,

    BLOCK_SILVER_L1, BLOCK_SILVER_L2, BLOCK_SILVER_L3, BLOCK_SILVER_L4,
    BLOCK_SILVER_L5, BLOCK_SILVER_L6, BLOCK_SILVER_L7,

    BLOCK_GOLD_L1, BLOCK_GOLD_L2, BLOCK_GOLD_L3, BLOCK_GOLD_L4,
    BLOCK_GOLD_L5, BLOCK_GOLD_L6, BLOCK_GOLD_L7,

    BLOCK_DIAMOND_L1, BLOCK_DIAMOND_L2, BLOCK_DIAMOND_L3, BLOCK_DIAMOND_L4,
    BLOCK_DIAMOND_L5, BLOCK_DIAMOND_L6, BLOCK_DIAMOND_L7,

    BLOCK_SCRAP_L1, BLOCK_SCRAP_L2, BLOCK_SCRAP_L3, BLOCK_SCRAP_L4,
    BLOCK_SCRAP_L5, BLOCK_SCRAP_L6, BLOCK_SCRAP_L7,

    // ========================================================================
    // ITEMS
    // ========================================================================
    // No son bloques colocables del terreno: viven en el enum porque el
    // inventario los trata igual. Van DESPUÉS del último bloque para que la
    // lista de bloques (0..BLOCK_LAST_PLACEABLE) sea contigua.
    BLOCK_DIRT_POWDER,      // 128 Polvo de tierra
    BLOCK_STICK,            // 129 Palo
    BLOCK_HOE,              // 130 Hoz
    BLOCK_COAL_ITEM,        // 131 Carbón (item)
    BLOCK_RAW_ZINC,         // 132 Zinc crudo
    BLOCK_RAW_COPPER,       // 133 Cobre crudo

    // ========================================================================
    // RETIRADOS
    // ========================================================================
    // BLOCK_IRON_ORE y BLOCK_BRICKS/BLOCK_GLASS existían pero nunca llegaron a
    // implementarse (sin textura, sin dureza, sin generación). Se conservan al
    // final, fuera del rango util, para que el código que aún los menciona
    // siga compilando sin ocupar un ID de la lista buena.
    BLOCK_IRON_ORE,         // 134 (sin implementar)
    BLOCK_BRICKS,           // 135 (sin implementar)
    BLOCK_GLASS,            // 136 (sin implementar)
    BLOCK_ORANGE_FLOWER,    // 137 (retirado del juego)
    BLOCK_HILO_IXTLE,       // 138 Hilo de ixtle (de piedra + hoja)
    BLOCK_HACHA_PIEDRA,     // 139 Hacha de piedra (herramienta)
    // El bedrock ya no se genera en el terreno, pero el motor aún lo consulta
    // (p.ej. para no aplastar al jugador contra el fondo del mundo).
    BLOCK_BEDROCK           // 140 (ya no se genera)
};

// Último bloque COLOCABLE (el último nivel añadido).
// Lo que va después son items y bloques retirados.
constexpr int BLOCK_LAST_PLACEABLE = BLOCK_SCRAP_L7;

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

// ============================================================================
// NIVELES: ALTURA DE UN BLOQUE PARCIAL
// ============================================================================
// Los ocho niveles y su altura en pixeles. El 8 es el bloque entero.
inline int alturaNivelPx(int nivel) {
    switch (nivel) {
        case 1: return 3;
        case 2: return 4;
        case 3: return 5;
        case 4: return 6;
        case 5: return 8;
        case 6: return 10;
        case 7: return 13;
        default: return 16;   // nivel 8: el bloque de siempre
    }
}

// ============================================================================
// FAMILIAS CON NIVELES
// ============================================================================
// UNA sola tabla: bloque entero -> primer ID de sus siete niveles. Todo lo
// demas (que nivel es, a que bloque pertenece, que ID toca) se deduce de
// aqui, asi que dar niveles a un bloque nuevo es anadir una linea.
//
// Antes esto era un switch y tres funciones con aritmetica sobre un unico
// rango contiguo, y por eso solo podian tener niveles ocho familias: al
// anadir mas, los IDs ya no eran contiguos y las cuentas se rompian. La
// tabla no depende del orden ni de que los rangos sean seguidos.
struct FamiliaNivel { BlockType entero; BlockType primero; };

inline const FamiliaNivel* tablaNiveles(int& n) {
    static const FamiliaNivel TABLA[] = {
        // --- Terreno (los originales) ---
        { BLOCK_GRASS,          BLOCK_GRASS_L1   },
        { BLOCK_DIRT,           BLOCK_DIRT_L1    },
        { BLOCK_STONE,          BLOCK_STONE_L1   },
        { BLOCK_SAND,           BLOCK_SAND_L1    },
        { BLOCK_GRAVEL,         BLOCK_GRAVEL_L1  },
        { BLOCK_SNOW,           BLOCK_SNOW_L1    },
        { BLOCK_CLAY_DIRT,      BLOCK_CLAYD_L1   },
        { BLOCK_CLAY_SAND,      BLOCK_CLAYS_L1   },
        // --- El resto de macizos ---
        { BLOCK_COBBLESTONE,    BLOCK_COBBLE_L1  },
        { BLOCK_CLAY,           BLOCK_CLAY_L1    },
        { BLOCK_LIMESTONE,      BLOCK_LIME_L1    },
        { BLOCK_PLANKS,         BLOCK_PLANKS_L1  },
        { BLOCK_PLANKS_ENCINO,  BLOCK_PLANKE_L1  },
        { BLOCK_PLANKS_OYAMEL,  BLOCK_PLANKO_L1  },
        { BLOCK_WOOD,           BLOCK_WOOD_L1    },
        { BLOCK_WOOD_ENCINO,    BLOCK_WOODE_L1   },
        { BLOCK_WOOD_OYAMEL,    BLOCK_WOODO_L1   },
        { BLOCK_COAL_ORE,       BLOCK_COAL_L1    },
        { BLOCK_SILVER_ORE,     BLOCK_SILVER_L1  },
        { BLOCK_GOLD_ORE,       BLOCK_GOLD_L1    },
        { BLOCK_DIAMOND_ORE,    BLOCK_DIAMOND_L1 },
        { BLOCK_SCRAP_METAL,    BLOCK_SCRAP_L1   },
    };
    n = (int)(sizeof(TABLA) / sizeof(TABLA[0]));
    return TABLA;
}

// Primer ID de cada familia con niveles. Devuelve BLOCK_AIR si el bloque no
// admite niveles (plantas, agua, lava, guijarros...).
inline BlockType primerNivelDe(BlockType completo) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    for (int i = 0; i < n; ++i)
        if (T[i].entero == completo) return T[i].primero;
    return BLOCK_AIR;
}

// ¿Este bloque es un nivel parcial (1..7)?
inline bool esNivelParcial(BlockType t) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    for (int i = 0; i < n; ++i) {
        const int p = (int)T[i].primero;
        if ((int)t >= p && (int)t < p + 7) return true;
    }
    return false;
}

// Que nivel es (1..8). Un bloque entero es 8; lo que no tiene niveles, 8.
inline int nivelDe(BlockType t) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    for (int i = 0; i < n; ++i) {
        const int p = (int)T[i].primero;
        if ((int)t >= p && (int)t < p + 7) return ((int)t - p) + 1;
    }
    return 8;
}

// El bloque COMPLETO al que pertenece este nivel.
inline BlockType bloqueBaseDe(BlockType t) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    for (int i = 0; i < n; ++i) {
        const int p = (int)T[i].primero;
        if ((int)t >= p && (int)t < p + 7) return T[i].entero;
    }
    return t;
}

// El ID que corresponde a un bloque y un nivel. Nivel 8 -> el bloque entero.
//
// Acepta que le pasen un nivel parcial como "completo": se normaliza a su
// bloque entero primero. Asi conNivel(tierra_L3, 5) da tierra_L5 en vez de
// devolver una barbaridad.
inline BlockType conNivel(BlockType completo, int nivel) {
    completo = bloqueBaseDe(completo);
    if (nivel >= 8) return completo;
    if (nivel < 1) nivel = 1;
    const BlockType primero = primerNivelDe(completo);
    if (primero == BLOCK_AIR) return completo;   // no admite niveles
    return (BlockType)((int)primero + (nivel - 1));
}

// ¿Este bloque admite niveles?
inline bool admiteNiveles(BlockType t) {
    return primerNivelDe(bloqueBaseDe(t)) != BLOCK_AIR;
}

// Altura en bloques (0..1) que ocupa este bloque. Es su colision EXACTA.
inline float alturaDe(BlockType t) {
    return (float)alturaNivelPx(nivelDe(t)) / 16.0f;
}

// ¿Es un guijarro suelto del suelo, de cualquier material?
// Todos comparten forma, colisión y generación; solo cambia la textura.
inline bool esGuijarro(BlockType t) {
    return t == BLOCK_PEDAZO_PIEDRA || t == BLOCK_PEDAZO_GRAVA ||
           t == BLOCK_PEDAZO_PEDERNAL || t == BLOCK_PEDAZO_CALIZA ||
           t == BLOCK_PEDAZO_TIERRA || t == BLOCK_PEDAZO_COBRE;
}

// ¿Es el cuerpo de una mata de ixtle, de cualquier tamaño?
inline bool esIxtleHoja(BlockType t) {
    return t == BLOCK_IXTLE_HOJA || t == BLOCK_IXTLE_PEQUENA ||
           t == BLOCK_IXTLE_GRANDE || t == BLOCK_IXTLE_ENORME;
}

// Escala de la mata: cuanto mide respecto al tamaño mediano.
// Sale del propio ID, así que no cuesta memoria ni hace falta guardarla.
inline float escalaIxtle(BlockType t) {
    switch (t) {
        case BLOCK_IXTLE_PEQUENA: return 0.45f;   // recién brotada
        case BLOCK_IXTLE_GRANDE:  return 1.40f;   // mata hecha
        case BLOCK_IXTLE_ENORME:  return 1.90f;   // ejemplar viejo
        default:                  return 1.00f;   // el mediano de siempre
    }
}

// ¿Es el suelo pintado bajo una mata (en pasto/tierra o en arena)?
inline bool esTalloIxtle(BlockType t) {
    return t == BLOCK_IXTLE_TALLO || t == BLOCK_IXTLE_TALLO_ARENA;
}

// ¿Es una celda COMPARTIDA: dos bloques ocupando el mismo espacio?
inline bool esCompartido(BlockType t) {
    return t == BLOCK_IXTLE_CON_HIERBA || t == BLOCK_IXTLE_CON_FLOR ||
           t == BLOCK_IXTLE_DOBLE;
}

// Las dos piezas que conviven en una celda compartida.
// PRIMERA: la que ocupa el centro (el ixtle). SEGUNDA: la acompañante.
inline BlockType piezaPrimera(BlockType t) {
    return esCompartido(t) ? BLOCK_IXTLE_HOJA : t;
}
inline BlockType piezaSegunda(BlockType t) {
    switch (t) {
        case BLOCK_IXTLE_CON_HIERBA: return BLOCK_TALLGRASS;
        case BLOCK_IXTLE_CON_FLOR:   return BLOCK_ORANGE_FLOWER;
        case BLOCK_IXTLE_DOBLE:      return BLOCK_IXTLE_HOJA;
        default:                     return BLOCK_AIR;
    }
}

// La combinación que resulta de juntar `encima` con lo que ya hay (`base`).
// Devuelve AIR si esa pareja no puede compartir espacio.
inline BlockType combinar(BlockType base, BlockType encima) {
    // Solo se comparte con una hoja de ixtle: su roseta es abierta y deja
    // huecos entre hoja y hoja donde cabe otra planta.
    const bool baseIxtle = esIxtleHoja(base);
    if (!baseIxtle) return BLOCK_AIR;

    if (encima == BLOCK_TALLGRASS)     return BLOCK_IXTLE_CON_HIERBA;
    if (encima == BLOCK_ORANGE_FLOWER) return BLOCK_IXTLE_CON_FLOR;
    if (encima == BLOCK_IXTLE_HOJA)    return BLOCK_IXTLE_DOBLE;
    return BLOCK_AIR;
}

// Al romper UNA de las dos piezas, ¿qué queda en la celda?
inline BlockType quitarPieza(BlockType t, bool quitarSegunda) {
    if (!esCompartido(t)) return BLOCK_AIR;
    return quitarSegunda ? piezaPrimera(t) : piezaSegunda(t);
}

// ¿Es una pieza de ixtle (lechuguilla)?
// Las celdas COMPARTIDAS también cuentan: llevan una hoja dentro.
inline bool esIxtle(BlockType t) {
    return esTalloIxtle(t) || esIxtleHoja(t) ||
           t == BLOCK_IXTLE_PUNTA || esCompartido(t);
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

// ============================================================================
// DESGASTE DEL HACHA DE PIEDRA
// ============================================================================
// El hacha aguanta 125 BLOQUES DE MADERA. Como cada bloque de tronco gasta
// 1 punto, eso son 125 puntos de vida.
//
// Todo se cuenta en MEDIOS PUNTOS (enteros) porque hay costes de 0.5, y con
// enteros no se acumula error de redondeo tras cientos de golpes.
//
//   raíz de árbol ......... 0.5   la madera fina apenas la mella
//   rama .................. 0.5
//   tronco ................ 1     la medida de referencia
//   nopal e ixtle ......... 1     carne de planta: cuesta como un tronco
//   todo lo demás ......... 3     piedra, arena, tierra... castiga usarla
//                                 para lo que no es un hacha
constexpr int HACHA_VIDA_MEDIOS = 250;   // 125 puntos x 2

// Cuántos medios puntos gasta romper este bloque con el hacha.
inline int desgasteHacha(BlockType t) {
    // --- Madera del árbol: es para lo que sirve el hacha ---
    if (t == BLOCK_WOOD || t == BLOCK_WOOD_ENCINO || t == BLOCK_WOOD_OYAMEL)
        return 2;                                  // 1 punto

    if (t == BLOCK_RAMA_PINO || t == BLOCK_RAMA_ENCINO ||
        t == BLOCK_RAMA_OYAMEL)
        return 1;                                  // 0.5

    if (esRaiz(t)) return 1;                       // 0.5

    // --- Plantas carnosas: nopal e ixtle, todas sus partes ---
    if (esCladodio(t) || t == BLOCK_NOPAL_FRUTO || esTuna(t) ||
        t == BLOCK_NOPAL_TALLO || t == BLOCK_NOPAL_MOJADO ||
        t == BLOCK_NOPAL_TIRAS || t == BLOCK_NOPAL_SIN_BABA ||
        t == BLOCK_NOPAL_BABA ||
        t == BLOCK_NOPAL_BASE_PASTO || t == BLOCK_NOPAL_BASE_TIERRA ||
        t == BLOCK_NOPAL_BASE_ARENA || t == BLOCK_NOPAL_BASE_T_ARCILLA ||
        t == BLOCK_NOPAL_BASE_A_ARCILLA ||
        esIxtle(t))
        return 2;                                  // 1 punto

    // --- Todo lo demás: piedra, arena, tierra, minerales... ---
    // El hacha no es un pico: usarla para esto la destroza.
    return 6;                                      // 3 puntos
}

// Último valor válido del enum: se usa para validar los datos leídos de
// archivos, donde un blockType fuera de rango llega desde disco y no del juego.
// ⚠️ Actualizar si se añaden bloques al final del enum.
constexpr int BLOCK_TYPE_MAX = BLOCK_BEDROCK;
