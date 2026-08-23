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
    BLOCK_BEDROCK,          // 140 (ya no se genera)

    // ========================================================================
    // HORNO PREHISPÁNICO
    // ========================================================================
    // Dos bloques para un mismo horno: apagado y encendido. Es la forma
    // clásica de resolverlo en un motor donde la celda solo guarda un ID y no
    // hay metadatos: el estado ES el bloque.
    //
    // Van al FINAL del enum a propósito. Los IDs son el formato de guardado:
    // insertarlos en medio correría los números de todo lo que viene después
    // y los mundos ya guardados leerían bloques equivocados.
    BLOCK_HORNO,            // 141 Horno apagado
    BLOCK_HORNO_ENCENDIDO,  // 142 Horno encendido (fuego animado, 4 frames)

    // ========================================================================
    // HERRAMIENTAS Y SUS PRODUCTOS
    // ========================================================================
    // Igual que el horno: al final del enum, porque los IDs son el formato de
    // guardado y meterlos en medio corrompería los mundos ya jugados.
    BLOCK_MARTILLO_PIEDRA,  // 143 Martillo de piedra (150 usos, para craftear)
    BLOCK_PICO_PIEDRA,      // 144 Pico de piedra (340 usos, para la roca)
    BLOCK_PEDERNAL_AFILADO, // 145 Pedernal afilado (pedernal + martillo)

    // ========================================================================
    // EL HACHA DE PEDERNAL Y EL MAGUEY MADURO
    // ========================================================================
    // Al final del enum, como siempre: los IDs son el formato de guardado.
    BLOCK_HACHA_PEDERNAL,   // 146 Hacha de pedernal afilado

    // El nopal que se seca: una penca mojada sin espinas, si se deja 20
    // segundos, pierde el agua y queda seca.
    BLOCK_NOPAL_SECO,       // 147 Penca seca sin espinas
    BLOCK_ESPINAS_NOPAL,    // 148 Espinas (salen al limpiar la penca)

    // Tazones de madera: uno por especie, para recoger el aguamiel.
    BLOCK_TAZON_PINO,       // 149 Tazon de madera de pino
    BLOCK_TAZON_ENCINO,     // 150 Tazon de madera de encino
    BLOCK_TAZON_OYAMEL,     // 151 Tazon de madera de oyamel

    // ------------------------------------------------------------------
    // EL MAGUEY DE 5 ANOS
    // ------------------------------------------------------------------
    // Un maguey maduro es mas grande y remata en una punta gruesa. Esa
    // punta es un bloque PROPIO, no la espina normal: se ve distinta y no
    // se puede arrancar a mano, hace falta el hacha de pedernal.
    //
    // Al cortarla queda HUECA, y en ese hueco se junta el aguamiel, que es
    // exactamente como se hace el pulque: se capa el maguey, se raspa el
    // cajete y la planta suelta el jugo ahi dentro.
    BLOCK_MAGUEY_PUNTA,     // 152 Punta gruesa del maguey maduro
    BLOCK_MAGUEY_HUECO,     // 153 La punta ya cortada, hueca
    BLOCK_AGUAMIEL,         // 154 Aguamiel juntandose en el hueco

    // El PICO de pedernal: lo mismo que el de piedra pero con las puntas de
    // pedernal afilado, que aguantan mucho mas antes de mellarse.
    BLOCK_PICO_PEDERNAL,    // 155 Pico de pedernal afilado
    BLOCK_MARTILLO_PEDERNAL, // 156 Martillo de pedernal (250 usos de taller)

    // Los tazones LLENOS de agua. Uno por especie, igual que los vacios: lo
    // que cambia es la textura, no la madera de la que estan hechos.
    BLOCK_TAZON_PINO_AGUA,   // 157 Tazon de pino con agua
    BLOCK_TAZON_ENCINO_AGUA, // 158 Tazon de encino con agua
    BLOCK_TAZON_OYAMEL_AGUA, // 159 Tazon de oyamel con agua

    // ========================================================================
    // LOS MINERALES DE HIERRO, SUELTOS POR EL SUELO
    // ========================================================================
    // Goethita, hematite y limonita son las tres formas en que el hierro
    // aflora de verdad: no en vetas profundas, sino como cantos sueltos en la
    // superficie, oxidados por el agua y el aire. Por eso son GUIJARROS y no
    // minerales de veta -- se recogen agachandose, no picando.
    BLOCK_PEDAZO_GOETHITA,   // 160 Canto de goethita
    BLOCK_PEDAZO_HEMATITE,   // 161 Canto de hematite
    BLOCK_PEDAZO_LIMONITA,   // 162 Canto de limonita

    // Nieve suelta: solo en las montanas nevadas.
    BLOCK_PEDAZO_NIEVE,      // 163 Punado de nieve

    // La PIRITA: el "oro de los tontos". Va en veta, como el carbon.
    BLOCK_PYRITE_ORE,        // 164 Mineral de pirita

    // ========================================================================
    // EL BARRO
    // ========================================================================
    // Tierra amasada con agua. Sale de mezclar polvo de tierra en un tazon
    // lleno, que es exactamente como se hace: el agua liga el polvo hasta
    // dejarlo en una pasta que se puede modelar.
    //
    // Es un ITEM, no un bloque colocable: va despues de
    // BLOCK_LAST_PLACEABLE y isPlaceableItem() lo excluye. De momento es el
    // material en bruto; lo que se haga con el (secarlo, cocerlo) viene
    // despues.
    //
    // Al final del enum, como todo lo nuevo: meterlo en medio correria los
    // IDs de lo que va detras y los mundos ya guardados leerian otra cosa.
    BLOCK_PEDAZO_BARRO       // 165 Pedazo de barro
};

// Último bloque COLOCABLE de la lista contigua del terreno.
// Lo que va después son items y bloques retirados.
constexpr int BLOCK_LAST_PLACEABLE = BLOCK_SCRAP_L7;

// ============================================================================
// CELDAS MIXTAS: UNA CAPA DE UN MATERIAL, RELLENO DE OTRO ENCIMA
// ============================================================================
// Poner arena sobre una capa de tierra dejaba la arena FLOTANDO: el bloque
// nuevo se iba al voxel de arriba (un salto de 16 px) mientras la capa de
// tierra medía 5, así que quedaban 11 px de aire a la vista.
//
// La celda tiene sitio de sobra -- lo que faltaba era poder decir "aquí abajo
// hay tierra hasta el píxel 5, y de ahí a 16 hay arena".
//
//     ┌────────────┐ 16
//     │▒▒▒ ARENA ▒▒│   relleno: lo que falta hasta arriba
//     ├────────────┤  5
//     │███ TIERRA █│   base: su nivel de siempre
//     └────────────┘  0
//
// CÓMO SE GUARDA. El chunk guarda UN BlockType por celda y no tiene sitio
// para metadatos, así que el par se mete en el propio ID. Pero una ID por
// cada combinación serían 3388 valores y habría que escribirlos a mano en el
// enum, en las texturas y en cada switch.
//
// En vez de eso se CALCULAN: a partir de BLOCK_MIXTO_BASE, el ID codifica
// (base, relleno) con una multiplicación. No ocupan sitio en el enum, no hay
// tabla que mantener, y añadir una familia con niveles las habilita todas.
//
// Los IDs siguen cabiendo de sobra: la paleta del chunk guarda 16 bits por
// índice y serializa 4 bytes por entrada.

// Cuántas familias con niveles hay (ver tablaNiveles()). Se comprueba con un
// static_assert más abajo para que no se quede desfasado.
constexpr int FAMILIAS_CON_NIVEL = 22;

// Primer ID del rango calculado. Va MUY por encima del enum para que jamás
// pise un bloque real, presente o futuro.
constexpr int BLOCK_MIXTO_BASE = 1000;

// Un ID mixto codifica: (indiceBase * 7 + (nivel-1)) * FAMILIAS + indiceRelleno
constexpr int MIXTO_POR_BASE = 7 * FAMILIAS_CON_NIVEL;
constexpr int BLOCK_MIXTO_FIN =
    BLOCK_MIXTO_BASE + FAMILIAS_CON_NIVEL * MIXTO_POR_BASE;

// ¿Es una celda mixta (capa + relleno de otro material)?
inline bool esMixto(BlockType t) {
    return (int)t >= BLOCK_MIXTO_BASE && (int)t < BLOCK_MIXTO_FIN;
}

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
        // ⭐ LA MADERA NO TIENE NIVELES
        //
        // Los tres troncos y los tres tablones salieron de esta tabla a
        // propósito: son piezas de construcción con forma y veta propias, y
        // partirlas en lonchas de 3 px las hacía irreconocibles. Un tronco es
        // un tronco entero, y punto.
        //
        // Sus IDs de nivel (BLOCK_WOOD_L1..L7, BLOCK_PLANKS_L1..L7, etc.)
        // siguen existiendo en el enum y NO se han tocado: quitarlos correría
        // los números de todo lo que va detrás y los mundos guardados leerían
        // bloques equivocados. Simplemente ya no se pueden crear, y si un
        // mundo antiguo trae uno, se dibuja y se rompe como el bloque entero
        // (bloqueBaseDe lo resuelve por el rango del ID, no por esta tabla).
        //
        // ⚠️ FAMILIAS_CON_NIVEL SIGUE SIENDO 22, no 16. Esa constante entra en
        // el cálculo del ID de las celdas mixtas
        // (base*7*FAMILIAS + relleno), así que bajarla cambiaría el
        // significado de todos los bloques mixtos ya guardados: una capa de
        // tierra con arena encima pasaría a leerse como otra cosa. El hueco
        // que dejan estas seis familias se queda reservado.
        //
        // ⭐ La madera SÍ puede ser RELLENO de una celda mixta aunque no
        // tenga niveles propios: ver tablaRelleno() justo debajo.
        { BLOCK_COAL_ORE,       BLOCK_COAL_L1    },
        { BLOCK_SILVER_ORE,     BLOCK_SILVER_L1  },
        { BLOCK_GOLD_ORE,       BLOCK_GOLD_L1    },
        { BLOCK_DIAMOND_ORE,    BLOCK_DIAMOND_L1 },
        { BLOCK_SCRAP_METAL,    BLOCK_SCRAP_L1   },
    };
    n = (int)(sizeof(TABLA) / sizeof(TABLA[0]));
    return TABLA;
}

// ============================================================================
// MATERIALES QUE PUEDEN SER RELLENO DE UNA CELDA MIXTA
// ============================================================================
// TENER NIVELES PROPIOS y PODER RELLENAR UNA CELDA son dos cosas distintas, y
// confundirlas era justo el bug de los troncos flotando.
//
// Un relleno no se parte en lonchas: ocupa DE GOLPE desde donde acaba la capa
// de abajo hasta el techo de la celda. Para eso no hace falta que el material
// tenga siete niveles — solo que se le pueda asignar un índice para meterlo en
// el ID mixto. Por eso la madera entra aquí sin volver a tablaNiveles(): un
// tronco sigue siendo un tronco entero, nunca una loncha de 3 px.
//
//     ┌────────────┐ 16
//     │▓▓ TRONCO ▓▓│  relleno: de la capa al techo, sin partirse
//     ├────────────┤  5
//     │███ TIERRA █│  base: su nivel de siempre
//     └────────────┘  0
//
// ORDEN INTOCABLE. El índice de esta tabla se guarda dentro del ID mixto, así
// que reordenarla o insertar en medio cambiaría el significado de las celdas
// mixtas YA GUARDADAS. Las 16 primeras posiciones son exactamente las de
// tablaNiveles() y en el mismo orden, para que todo lo guardado hasta hoy siga
// leyéndose igual; lo nuevo se añade DETRÁS, ocupando los huecos reservados.
inline const BlockType* tablaRelleno(int& n) {
    static const BlockType TABLA[] = {
        // --- Las 16 con niveles propios: MISMO ORDEN que tablaNiveles() ---
        BLOCK_GRASS,      BLOCK_DIRT,       BLOCK_STONE,     BLOCK_SAND,
        BLOCK_GRAVEL,     BLOCK_SNOW,       BLOCK_CLAY_DIRT, BLOCK_CLAY_SAND,
        BLOCK_COBBLESTONE, BLOCK_CLAY,      BLOCK_LIMESTONE,
        BLOCK_COAL_ORE,   BLOCK_SILVER_ORE, BLOCK_GOLD_ORE,
        BLOCK_DIAMOND_ORE, BLOCK_SCRAP_METAL,
        // --- Los 6 huecos reservados: la madera, que no tiene niveles ---
        BLOCK_WOOD,       BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_PLANKS,     BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,
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

// ============================================================================
// CELDAS MIXTAS: codificar y descodificar
// ============================================================================
// El ID no está en el enum: se calcula. Ver el comentario de BLOCK_MIXTO_BASE.

// Posición de una familia dentro de tablaNiveles(), o -1 si no tiene niveles.
// Es quien puede ser la CAPA DE ABAJO de una celda mixta: esa sí necesita
// niveles, porque su altura es la del nivel.
inline int indiceFamilia(BlockType entero) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    entero = bloqueBaseDe(entero);
    for (int i = 0; i < n; ++i) if (T[i].entero == entero) return i;
    return -1;
}

// Posición dentro de tablaRelleno(), o -1 si el material no puede rellenar.
// Es quien puede ser el MATERIAL DE ARRIBA: no necesita niveles, porque
// siempre ocupa de la capa al techo.
inline int indiceRelleno(BlockType entero) {
    int n = 0; const BlockType* T = tablaRelleno(n);
    entero = bloqueBaseDe(entero);
    for (int i = 0; i < n; ++i) if (T[i] == entero) return i;
    return -1;
}

// ¿Caben las tablas en el espacio de IDs reservado para las celdas mixtas?
//
// FAMILIAS_CON_NIVEL es el ANCHO del hueco reservado (22). Lo que hay que
// vigilar es que ninguna de las dos tablas CREZCA por encima de él: si eso
// pasara, los índices se solaparían y dos parejas distintas de materiales
// darían el mismo ID mixto.
//
// Añadir una entrada nueva mientras quepa es seguro y no toca nada guardado:
// ocupa uno de los huecos reservados, siempre POR DETRÁS de lo que ya hay.
inline bool familiasCaben() {
    int nNiv = 0; tablaNiveles(nNiv);
    int nRel = 0; tablaRelleno(nRel);
    return nNiv <= FAMILIAS_CON_NIVEL && nRel <= FAMILIAS_CON_NIVEL;
}

// El ID de "capa de `base` a nivel `nivel`, y de ahí a 16 px, `relleno`".
// Devuelve BLOCK_AIR si la pareja no se puede representar.
inline BlockType mixto(BlockType base, int nivel, BlockType relleno) {
    // La capa de ABAJO necesita niveles (su altura es la del nivel); el
    // material de ARRIBA no, porque siempre llega hasta el techo. Por eso
    // cada uno usa su propia tabla: es lo que permite que un tronco rellene
    // una celda sin tener que partirse en lonchas.
    const int ib = indiceFamilia(base);
    const int ir = indiceRelleno(relleno);
    if (ib < 0 || ir < 0) return BLOCK_AIR;      // no se puede representar
    if (nivel < 1 || nivel > 7) return BLOCK_AIR; // 8 es la celda llena
    return (BlockType)(BLOCK_MIXTO_BASE +
                       (ib * 7 + (nivel - 1)) * FAMILIAS_CON_NIVEL + ir);
}

// Las tres partes de una celda mixta.
inline BlockType mixtoBase(BlockType t) {
    if (!esMixto(t)) return t;
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    const int k = ((int)t - BLOCK_MIXTO_BASE) / FAMILIAS_CON_NIVEL;
    const int ib = k / 7;
    return (ib >= 0 && ib < n) ? T[ib].entero : BLOCK_AIR;
}
inline int mixtoNivel(BlockType t) {
    if (!esMixto(t)) return 8;
    const int k = ((int)t - BLOCK_MIXTO_BASE) / FAMILIAS_CON_NIVEL;
    return (k % 7) + 1;
}
inline BlockType mixtoRelleno(BlockType t) {
    if (!esMixto(t)) return BLOCK_AIR;
    int n = 0; const BlockType* T = tablaRelleno(n);
    const int ir = ((int)t - BLOCK_MIXTO_BASE) % FAMILIAS_CON_NIVEL;
    return (ir >= 0 && ir < n) ? T[ir] : BLOCK_AIR;
}

// Altura en bloques (0..1) que ocupa este bloque. Es su colision EXACTA.
//
// Una celda mixta está LLENA: la capa de abajo más el relleno llegan hasta
// arriba, así que se anda por encima como por un bloque entero.
inline float alturaDe(BlockType t) {
    if (esMixto(t)) return 1.0f;
    return (float)alturaNivelPx(nivelDe(t)) / 16.0f;
}

// Altura (0..1) a la que acaba la capa de ABAJO de una celda mixta. Es donde
// empieza el relleno, y lo que necesita el mesher para dibujar las dos.
inline float alturaBaseMixto(BlockType t) {
    if (!esMixto(t)) return 0.0f;
    return (float)alturaNivelPx(mixtoNivel(t)) / 16.0f;
}

// ¿Es un guijarro suelto del suelo, de cualquier material?
// Todos comparten forma, colisión y generación; solo cambia la textura.
inline bool esGuijarro(BlockType t) {
    return t == BLOCK_PEDAZO_PIEDRA || t == BLOCK_PEDAZO_GRAVA ||
           t == BLOCK_PEDAZO_PEDERNAL || t == BLOCK_PEDAZO_CALIZA ||
           t == BLOCK_PEDAZO_TIERRA || t == BLOCK_PEDAZO_COBRE ||
           // Los tres hierros y la nieve suelta: mismos cantos, otro
           // material. Al entrar aqui heredan TODO el comportamiento de
           // guijarro -- la forma de montoncito, la caja de colision baja,
           // que no ahoguen al pasto, que se recojan sin picar -- sin tener
           // que tocar ni una linea del mesher ni del raycast.
           t == BLOCK_PEDAZO_GOETHITA || t == BLOCK_PEDAZO_HEMATITE ||
           t == BLOCK_PEDAZO_LIMONITA || t == BLOCK_PEDAZO_NIEVE;
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
// LAS HERRAMIENTAS Y LO QUE AGUANTAN
// ============================================================================
// Cada herramienta sirve para UNA cosa, y fuera de ahí no sirve. Ese es el
// criterio de todo lo que hay debajo:
//
//   HACHA   250 bloques  lo ORGÁNICO: madera, ramas, raíces, nopal, maguey.
//                        1 segundo cada uno; 13 minutos todo lo demás.
//   PICO    340 bloques  la ROCA: piedra, caliza, minerales, grava, arena...
//                        1,5 segundos cada uno; 13 minutos lo orgánico.
//   MARTILLO 150 usos    no pica: es una herramienta de CRAFTEO. Junto a un
//                        pedernal en la rejilla da un pedernal afilado, y
//                        gasta un uso por cada uno.
//
// Usar una herramienta para lo que no es NO le gasta vida: el castigo es que
// tarda 13 minutos, que es un "esto no es lo tuyo" mucho más claro que ver
// desaparecer la herramienta.
//
// La cuenta se lleva en MEDIOS PUNTOS porque `vidaMedios` es el campo que ya
// está en las partidas guardadas. Un bloque = 2 medios = 1 punto.
constexpr int HACHA_VIDA_BLOQUES    = 250;
constexpr int PICO_VIDA_BLOQUES     = 340;
constexpr int MARTILLO_VIDA_USOS    = 150;

// ----------------------------------------------------------------------------
// LA RAMA DE PEDERNAL
// ----------------------------------------------------------------------------
// Las mismas tres herramientas, con el filo de pedernal afilado en vez de
// piedra comun. Hacen exactamente lo mismo y al mismo ritmo: lo unico que
// cambia es cuanto aguantan antes de mellarse.
//
// El hacha de pedernal, ademas, es la unica que puede con la punta del
// maguey maduro.
constexpr int HACHA_PEDERNAL_BLOQUES    = 390;
constexpr int PICO_PEDERNAL_BLOQUES     = 550;
constexpr int MARTILLO_PEDERNAL_USOS    = 250;

constexpr int HACHA_VIDA_MEDIOS     = HACHA_VIDA_BLOQUES  * 2;   // 500
constexpr int PICO_VIDA_MEDIOS      = PICO_VIDA_BLOQUES   * 2;   // 680
constexpr int MARTILLO_VIDA_MEDIOS  = MARTILLO_VIDA_USOS  * 2;   // 300
constexpr int HACHA_PEDERNAL_MEDIOS = HACHA_PEDERNAL_BLOQUES * 2;    // 780
constexpr int PICO_PEDERNAL_MEDIOS  = PICO_PEDERNAL_BLOQUES  * 2;    // 1100
constexpr int MARTILLO_PEDERNAL_MEDIOS = MARTILLO_PEDERNAL_USOS * 2; // 500

// ¿Este objeto es una herramienta que se gasta?
inline bool esHerramientaGastable(BlockType t) {
    return t == BLOCK_HACHA_PIEDRA ||
           t == BLOCK_PICO_PIEDRA  ||
           t == BLOCK_MARTILLO_PIEDRA ||
           t == BLOCK_HACHA_PEDERNAL ||
           t == BLOCK_PICO_PEDERNAL ||
           t == BLOCK_MARTILLO_PEDERNAL;
}

// ¿Es un hacha, de la clase que sea? Las dos cortan lo mismo; la de pedernal
// aguanta más y además puede con la punta del maguey.
inline bool esHacha(BlockType t) {
    return t == BLOCK_HACHA_PIEDRA || t == BLOCK_HACHA_PEDERNAL;
}

// ¿Es un pico? Los dos rompen la misma roca al mismo ritmo; el de pedernal
// solo aguanta más.
inline bool esPico(BlockType t) {
    return t == BLOCK_PICO_PIEDRA || t == BLOCK_PICO_PEDERNAL;
}

// ¿Es un martillo? Ninguno de los dos pica: son herramientas de TALLER, y se
// gastan al craftear (afilar un pedernal, p. ej.), no al romper bloques.
inline bool esMartillo(BlockType t) {
    return t == BLOCK_MARTILLO_PIEDRA || t == BLOCK_MARTILLO_PEDERNAL;
}

// ============================================================================
// LOS TAZONES: VACIO <-> LLENO
// ============================================================================
// La correspondencia entre cada tazon y su version con agua vive AQUI, en un
// solo sitio. Cualquier otro punto del motor (llenar en el rio, vaciar,
// dibujar el icono) pregunta a estas funciones en vez de llevar su propia
// lista, que es como se acaba teniendo dos que no coinciden.

inline bool esTazonVacio(BlockType t) {
    return t == BLOCK_TAZON_PINO || t == BLOCK_TAZON_ENCINO ||
           t == BLOCK_TAZON_OYAMEL;
}

inline bool esTazonConAgua(BlockType t) {
    return t == BLOCK_TAZON_PINO_AGUA || t == BLOCK_TAZON_ENCINO_AGUA ||
           t == BLOCK_TAZON_OYAMEL_AGUA;
}

inline bool esTazon(BlockType t) {
    return esTazonVacio(t) || esTazonConAgua(t);
}

// El mismo tazon, lleno de agua. Devuelve BLOCK_AIR si no era un tazon vacio.
inline BlockType tazonLleno(BlockType vacio) {
    switch (vacio) {
        case BLOCK_TAZON_PINO:   return BLOCK_TAZON_PINO_AGUA;
        case BLOCK_TAZON_ENCINO: return BLOCK_TAZON_ENCINO_AGUA;
        case BLOCK_TAZON_OYAMEL: return BLOCK_TAZON_OYAMEL_AGUA;
        default:                 return BLOCK_AIR;
    }
}

// El mismo tazon, vacio. Devuelve BLOCK_AIR si no era un tazon con agua.
inline BlockType tazonVaciado(BlockType lleno) {
    switch (lleno) {
        case BLOCK_TAZON_PINO_AGUA:   return BLOCK_TAZON_PINO;
        case BLOCK_TAZON_ENCINO_AGUA: return BLOCK_TAZON_ENCINO;
        case BLOCK_TAZON_OYAMEL_AGUA: return BLOCK_TAZON_OYAMEL;
        default:                      return BLOCK_AIR;
    }
}

// Vida COMPLETA de cada herramienta, en medios puntos.
// Devuelve 0 si no es una herramienta que se gaste.
inline int vidaMaximaHerramienta(BlockType t) {
    switch (t) {
        case BLOCK_HACHA_PIEDRA:    return HACHA_VIDA_MEDIOS;
        case BLOCK_PICO_PIEDRA:     return PICO_VIDA_MEDIOS;
        case BLOCK_MARTILLO_PIEDRA: return MARTILLO_VIDA_MEDIOS;
        case BLOCK_HACHA_PEDERNAL:  return HACHA_PEDERNAL_MEDIOS;
        case BLOCK_PICO_PEDERNAL:   return PICO_PEDERNAL_MEDIOS;
        case BLOCK_MARTILLO_PEDERNAL: return MARTILLO_PEDERNAL_MEDIOS;
        default:                    return 0;
    }
}

// ¿Es un bloque ORGÁNICO, de los que el hacha corta en 1 segundo?
//
// Son las cosas vivas del mundo: el árbol entero (tronco, rama, raíz), y las
// plantas carnosas (nopal con todas sus partes, y el maguey/ixtle).
//
// Esta es la lista que manda en las DOS cosas del hacha: cuánta vida gasta el
// bloque y cuánto tarda en romperse. Tenerlas juntas evita que se separen —
// si mañana se añade una planta nueva, se añade aquí y las dos reglas la
// reconocen a la vez.
inline bool esOrganicoParaHacha(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);

    // --- El árbol: tronco, rama y raíz ---
    if (t == BLOCK_WOOD || t == BLOCK_WOOD_ENCINO || t == BLOCK_WOOD_OYAMEL)
        return true;
    if (t == BLOCK_RAMA_PINO || t == BLOCK_RAMA_ENCINO ||
        t == BLOCK_RAMA_OYAMEL)
        return true;
    if (esRaiz(t)) return true;

    // --- Nopal: cladodio, penca, tallo, tunas y sus bases ---
    if (esCladodio(t) || t == BLOCK_NOPAL_FRUTO || esTuna(t) ||
        t == BLOCK_NOPAL_TALLO || t == BLOCK_NOPAL_MOJADO ||
        t == BLOCK_NOPAL_TIRAS || t == BLOCK_NOPAL_SIN_BABA ||
        t == BLOCK_NOPAL_BABA ||
        t == BLOCK_NOPAL_BASE_PASTO || t == BLOCK_NOPAL_BASE_TIERRA ||
        t == BLOCK_NOPAL_BASE_ARENA || t == BLOCK_NOPAL_BASE_T_ARCILLA ||
        t == BLOCK_NOPAL_BASE_A_ARCILLA)
        return true;

    // --- Maguey / ixtle: hoja y punta ---
    if (esIxtle(t)) return true;

    return false;
}

// Cuántos medios puntos gasta romper este bloque con el hacha.
// Orgánico: 1 punto (2 medios). Todo lo demás: nada, porque el hacha no
// sirve para eso y el castigo ya es el tiempo que tarda.
inline int desgasteHacha(BlockType t) {
    return esOrganicoParaHacha(t) ? 2 : 0;
}

// ¿Es un bloque de ROCA, de los que el pico rompe en 1,5 segundos?
//
// Es el complemento del hacha: todo lo mineral del mundo. Se define por
// EXCLUSIÓN de lo orgánico y de las plantas, para que un bloque mineral nuevo
// entre solo sin tener que acordarse de apuntarlo aquí.
inline bool esRocaParaPico(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    if (esMixto(t))        t = mixtoRelleno(t);

    if (t == BLOCK_AIR || t == BLOCK_WATER || t == BLOCK_LAVA) return false;

    // Lo orgánico es del hacha, no del pico.
    if (esOrganicoParaHacha(t)) return false;

    // Las plantas y la hojarasca tampoco: no son roca.
    if (t == BLOCK_TALLGRASS || t == BLOCK_LEAVES ||
        t == BLOCK_LEAVES_ENCINO || t == BLOCK_LEAVES_OYAMEL)
        return false;

    // ⭐ EL MAGUEY MADURO Y LO QUE SALE DE EL
    //
    // La punta gruesa es planta, no piedra: el pico no debe poder con ella
    // (solo el hacha de pedernal), y desde luego no debe gastarse al
    // intentarlo. El tocon capado y el aguamiel, por lo mismo.
    //
    // Este agujero salio de un test, y es el riesgo de definir "roca" por
    // EXCLUSION: cada planta nueva que se anade entra sola en la lista del
    // pico si nadie se acuerda de excluirla.
    if (t == BLOCK_MAGUEY_PUNTA || t == BLOCK_MAGUEY_HUECO ||
        t == BLOCK_AGUAMIEL || t == BLOCK_NOPAL_SECO ||
        t == BLOCK_ESPINAS_NOPAL)
        return false;

    // Las herramientas, los tazones y los items sueltos no son bloques del
    // mundo: nada que picar ahi.
    if (esHerramientaGastable(t)) return false;
    if (esTazon(t)) return false;   // vacios y llenos, ninguno es roca

    return true;
}

// Cuántos medios puntos gasta romper este bloque con el pico.
inline int desgastePico(BlockType t) {
    return esRocaParaPico(t) ? 2 : 0;
}

// Lo que gasta la herramienta que se lleve en la mano al romper ESE bloque.
// Punto único: así el desgaste y el tiempo de rotura no pueden discrepar.
inline int desgasteHerramienta(BlockType herramienta, BlockType bloque) {
    switch (herramienta) {
        case BLOCK_HACHA_PIEDRA: return desgasteHacha(bloque);
        // La de pedernal corta lo mismo, y ademas la punta del maguey, que
        // es el unico bloque que solo ella puede arrancar.
        case BLOCK_HACHA_PEDERNAL:
            return (desgasteHacha(bloque) > 0 ||
                    bloque == BLOCK_MAGUEY_PUNTA) ? 2 : 0;
        // Los dos picos gastan igual: lo que cambia es cuanto aguantan.
        case BLOCK_PICO_PIEDRA:
        case BLOCK_PICO_PEDERNAL: return desgastePico(bloque);
        // El martillo no pica: se gasta al craftear, no al romper.
        default:                 return 0;
    }
}

// Último valor válido del enum: se usa para validar los datos leídos de
// archivos, donde un blockType fuera de rango llega desde disco y no del juego.
// ⚠️ Actualizar si se añaden bloques al final del enum.
constexpr int BLOCK_TYPE_MAX = BLOCK_PYRITE_ORE;
