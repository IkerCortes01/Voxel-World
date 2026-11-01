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

    // --- Minerales comunes: RETIRADOS del juego ---
    // Los bloques de mineral se eliminaron (ver "MINERALES RETIRADOS" al final
    // de este archivo). Sus IDs NO se borran: cada uno era una posicion en el
    // enum, y quitarla correria en uno todos los bloques de detras -- que es
    // exactamente lo que obligo a escribir BlockCompat.h la vez anterior.
    //
    // El hueco se queda reservado y sin uso. Un mundo guardado que traiga uno
    // de estos IDs lo convierte en piedra al cargar (BlockCompat::mineralAPiedra).
    BLOCK_RESERVADO_MINERAL_08,  // 8  (era BLOCK_COAL_ORE)
    BLOCK_RESERVADO_MINERAL_09,  // 9  (era BLOCK_SCRAP_METAL)

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

    // --- Minerales raros: RETIRADOS (huecos reservados, ver arriba) ---
    BLOCK_RESERVADO_MINERAL_20,  // 20 (era BLOCK_SILVER_ORE)
    BLOCK_RESERVADO_MINERAL_21,  // 21 (era BLOCK_DIAMOND_ORE)

    BLOCK_COBBLESTONE,      // 22 Piedra labrada
    BLOCK_LAVA,             // 23 Lava
    BLOCK_CLAY,             // 24 Bloque de arcilla

    // --- Bloques nuevos ---
    BLOCK_LIMESTONE,        // 25 Piedra caliza
    BLOCK_RESERVADO_MINERAL_26,  // 26 (era BLOCK_GOLD_ORE, retirado)
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

    // ⭐ EL TALLO: RETIRADO, PERO SU SITIO SE QUEDA.
    //
    // Era un bloque COMPLETO que hacia de tronco central del nopal, con las
    // pencas brotando de sus costados. Se retiro porque dibujaba un arbolito
    // y no un nopal: una Opuntia ficus-indica no tiene fuste de madera, su
    // cuerpo entero son pencas encadenadas desde el suelo (lo que en un
    // ejemplar viejo parece tronco es la penca basal lignificada, que aqui la
    // representa el bloque de base). Hoy el eje de la planta es una columna de
    // BLOCK_NOPAL_CLADODIO -- ver `generarNopal` en main.cpp.
    //
    // ⚠️ LA FILA NO SE PUEDE BORRAR. Es el ID 34, en medio del enum: quitarlo
    // correria en -1 los ~156 que van detras, y todos los mundos guardados
    // leerian otro bloque (una penca pasaria a leerse como el fruto). Es la
    // misma regla que ya se aplico a los cinco minerales de veta.
    //
    // Se queda fuera de la generacion y fuera del inventario creativo (ver
    // `esBloqueRetirado`), asi que en la practica no existe para el jugador;
    // los mundos que ya lo tengan lo siguen dibujando y rompiendo igual.
    BLOCK_NOPAL_TALLO,           // 34 RETIRADO (era el tronco del nopal)
    BLOCK_NOPAL_CLADODIO,        // 35 Cladodio/penca: sprite 3D atravesable

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

    // La PIRITA iba en veta, como el carbon: RETIRADA con el resto de los
    // minerales de veta. El hueco se reserva (ver "MINERALES RETIRADOS").
    BLOCK_RESERVADO_MINERAL_164,  // 164 (era BLOCK_PYRITE_ORE)

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
    BLOCK_PEDAZO_BARRO,      // 165 Pedazo de barro

    // ========================================================================
    // LOS TAZONES CON AGUAMIEL
    // ========================================================================
    // El premio de capar un maguey. Uno por especie, igual que los vacios y
    // los de agua: lo que cambia es lo que llevan dentro, no la madera.
    //
    // Son bloques PROPIOS y no un estado del tazon de agua porque el aguamiel
    // no es agua: sabe distinto, se consigue de otra forma y llevara a otras
    // recetas (el pulque). Mezclarlos obligaria a llevar la cuenta aparte de
    // que hay en cada tazon, que es justo lo que el ID ya resuelve gratis.
    BLOCK_TAZON_PINO_AGUAMIEL,   // 166 Tazon de pino con aguamiel
    BLOCK_TAZON_ENCINO_AGUAMIEL, // 167 Tazon de encino con aguamiel
    BLOCK_TAZON_OYAMEL_AGUAMIEL, // 168 Tazon de oyamel con aguamiel

    // --- TIERRA MOJADA: la que se bebio el agua ---
    //
    // Cuando el agua cae sobre tierra o arena seca, el suelo la absorbe y pasa
    // a su version mojada. NO es decoracion: el agua absorbida esta GUARDADA
    // ahi dentro, y por eso un bloque mojado ya no bebe mas.
    //
    // Es lo que impide que un oceano se drene por su propia playa: la orilla
    // se satura y deja de robarle agua al mar.
    //
    // Van AL FINAL del enum a proposito. Insertarlos en medio correria los IDs
    // de todo lo que viene detras y los mundos guardados leerian otro bloque.
    BLOCK_DIRT_MOJADA,           // 169 Tierra empapada
    BLOCK_SAND_MOJADA,           // 170 Arena empapada
    BLOCK_GRASS_MOJADA,          // 171 Pasto empapado

    // ========================================================================
    // EL AGAVE TEQUILANA AZUL
    // ========================================================================
    // La planta en si es un BLOQUE COMPUESTO (ver FAM_AGAVE_AZUL en
    // BloqueCompuesto.h): su estado -- fase, etapa, quiote -- viaja dentro
    // del ID y no gasta entradas de este enum.
    //
    // Lo que SI necesita entrada propia es cada TEXTURA que el mesher pide
    // por separado, porque texSegura() trabaja con BlockType. Son etiquetas
    // de textura, no bloques que el jugador coloque:
    //
    //   PENCA  la hoja azul plateada mate (la cera del agave)
    //   PUNTA  la espina terminal, oscura y muy dura
    //   PINA   el tallo central fibroso, blanco y verde claro
    //   QUIOTE el tallo floral
    //   FLOR   los racimos amarillos del candelabro
    //
    // Al final del enum, como todo lo nuevo.
    BLOCK_AGAVE_AZUL_PENCA,      // 172 Hoja azul plateada del tequilana
    BLOCK_AGAVE_AZUL_PUNTA,      // 173 Espina terminal, dura y oscura
    BLOCK_AGAVE_AZUL_PINA,       // 174 Piña: el tallo central ya jimado
    BLOCK_AGAVE_AZUL_QUIOTE,     // 175 Tallo floral que sube hasta 5 m
    BLOCK_AGAVE_AZUL_FLOR,       // 176 Racimos amarillos del candelabro

    // ========================================================================
    // EL PINO OCOTE (Pinus montezumae)
    // ========================================================================
    // El pino de Moctezuma, el ocote blanco del centro de Mexico. Es una
    // ESPECIE MAS de pino, no un reemplazo del que ya habia: comparte la
    // familia pero se distingue de lejos por su copa redondeada y ancha y por
    // sus aciculas larguisimas (30-35 cm) agrupadas de cinco en cinco, que
    // cuelgan en vez de erizarse.
    //
    // Lleva las cuatro entradas del patron que ya siguen encino y oyamel --
    // tronco, tronco por dentro, hojas y tablones -- porque son cuatro
    // texturas distintas y texSegura() trabaja con BlockType.
    //
    // Al final del enum: insertar en medio correria los IDs y los mundos
    // guardados leerian otro bloque.
    BLOCK_WOOD_OCOTE,            // 177 Tronco: corteza gruesa en placas
    BLOCK_WOOD_OCOTE_DENTRO,     // 178 El corte del tronco, ambar y resinoso
    BLOCK_LEAVES_OCOTE,          // 179 Aciculas largas de cinco en cinco
    BLOCK_PLANKS_OCOTE,          // 180 Tablones de ocote blanco
    BLOCK_RAMA_OCOTE,            // 181 Rama horizontal y fuerte

    // ========================================================================
    // LA PENCA DEL AGAVE TEQUILANA AZUL, COMO ITEM
    // ========================================================================
    // El pulquero ya entregaba pencas al talarlo (BLOCK_IXTLE_HOJA). El
    // tequilana no entregaba NADA, asi que cortarlo no servia de nada.
    //
    // Lleva item PROPIO y no reutiliza el del pulquero porque son dos plantas
    // distintas y se ven distintas: la penca del tequilana es azul plateada
    // mate por la cera, la del pulquero verde. Compartir item borraria de un
    // plumazo la diferencia que costo modelar.
    BLOCK_PENCA_AGAVE_AZUL,      // 182 Penca azul plateada del tequilana

    // ========================================================================
    // EL OCOTE CHINO (Pinus leiophylla)
    // ========================================================================
    // La quinta especie de arbol, y la segunda del genero Pinus despues del
    // ocote blanco (Pinus montezumae). Se le llama "chino" por sus aciculas
    // finas y rizadas, mas cortas y en manojos mas apretados que las del
    // blanco: de ahi que su follaje se lea mas tupido.
    //
    // Va con bloques PROPIOS y no reutiliza los del ocote blanco porque son
    // dos maderas distintas y se ven distintas -- la del chino tira a rojiza
    // oscura y su corte es mas cerrado. Compartir bloque borraria de un
    // plumazo esa diferencia, que es justo la que se pidio.
    //
    // Al final del enum, como manda la regla: insertar en medio correria los
    // IDs y los mundos guardados leerian otro bloque.
    BLOCK_WOOD_OCOTE_CHINO,         // 183 Tronco: corteza rojiza en placas
    BLOCK_WOOD_OCOTE_CHINO_DENTRO,  // 184 El corte del tronco, mas cerrado
    BLOCK_LEAVES_OCOTE_CHINO,       // 185 Aciculas finas, en manojo apretado

    // ⭐ HOJA CON RAMA DENTRO
    //
    // La copa del ocote chino lleva el ramaje POR DENTRO del follaje: la rama
    // y las hojas comparten voxel en vez de gastar dos. Es una celda
    // compartida (ver esCompartido), asi que el raycast, el mesher y la
    // rotura la tratan como dos piezas independientes sin codigo especial.
    BLOCK_LEAVES_OCOTE_CHINO_RAMA,  // 186 Hojas con la rama dentro

    // ⭐ LO MISMO PARA EL OCOTE BLANCO (Pinus montezumae)
    //
    // El chino ya llevaba el ramaje dentro del follaje; el blanco no, asi que
    // su copa era hoja maciza sin nada que la sostuviera por dentro. Con esto
    // las dos especies comparten estructura: la rama y las aciculas ocupan el
    // MISMO voxel en vez de gastar dos.
    //
    // Va al final del enum, como todo lo nuevo: insertar en medio correria los
    // IDs y los mundos guardados leerian otro bloque.
    BLOCK_LEAVES_OCOTE_RAMA,        // 187 Aciculas del ocote con rama dentro

    // ========================================================================
    // HUEVO DE SPAWN: PECARI DE COLLAR
    // ========================================================================
    // Item EXCLUSIVO DEL CREATIVO. No es un bloque, no se coloca y no se
    // craftea: al hacer clic derecho suelta un pecari de collar donde apunta
    // el jugador.
    //
    //   clic derecho          un adulto suelto
    //   SHIFT + clic derecho  una manada completa, con su reparto de edades
    //
    // POR QUE UN ITEM Y NO UN COMANDO: el motor no tiene consola. El
    // inventario creativo es la unica superficie donde el jugador puede pedir
    // que aparezca algo, asi que es donde tiene que vivir.
    //
    // Va al final del enum, como todo lo nuevo: insertar en medio correria los
    // IDs y los mundos guardados leerian otro bloque.
    BLOCK_HUEVO_PECARI,             // 188 Huevo de spawn del pecari de collar

    // --- EL AGAVE ABISAII ---
    //
    // Como el tequilana, la planta en si es un BLOQUE COMPUESTO (ver
    // FAM_AGAVE_ABISAII en BloqueCompuesto.h): su etapa, giro y segmento
    // viajan dentro del ID y no gastan entradas de este enum.
    //
    // Estas dos SI son entradas reales porque son ETIQUETAS DE TEXTURA:
    // `texSegura()` trabaja con BlockType, asi que cada imagen que el mesher
    // pueda pedir necesita su nombre aqui.
    //
    //   PENCA  la hoja ancha y arqueada (el cuerpo de la roseta)
    //   PUNTA  la espina terminal y los dientes del borde
    //
    // ⚠️ Son DOS TEXTURAS DISTINTAS, no dos tramos de UV de la misma. Con el
    // texture array cada una es una CAPA, y la capa viaja en un byte del
    // vertice. Repartir una sola imagen por UV no vale aqui: el greedy
    // meshing tesela con GL_REPEAT y un sub-rectangulo se saldria de su
    // casilla (ver PENDIENTES.md, cuarta vuelta de rendimiento).
    BLOCK_AGAVE_ABISAII_PENCA,      // 189 Hoja ancha y arqueada
    BLOCK_AGAVE_ABISAII_PUNTA,      // 190 Espina terminal y dientes del borde

    // ========================================================================
    // TECHO DE HOJAS
    // ========================================================================
    // Un entramado de ramas con follaje encima: lo que se pone sobre una
    // enramada para dar sombra. Se craftea con hojas y palos en diagonal.
    //
    // Es una LAMINA de un solo nivel (3 px), no un cubo: un techo tiene
    // espesor de techo. Se pueden poner varios y la lamina crece, igual que
    // cualquier otro bloque con niveles.
    //
    // ⚠️ VA AL FINAL DEL ENUM. Insertarlo en medio correria los IDs de todo
    // lo que va detras y los mundos guardados leerian otro bloque.
    BLOCK_TECHO_HOJAS,              // 191 Entramado de ramas y follaje

    // Sus SIETE niveles (3, 4, 5, 6, 8, 10 y 13 px). Van seguidos y en orden:
    // `tablaNiveles` solo guarda el PRIMERO y deduce el resto por el rango
    // [primero, primero+7), asi que el orden no es decorativo.
    //
    // Colocado, el techo nace en su nivel 1 --una lamina de 3 px, que es el
    // espesor de un techo-- y crece al seguir poniendo del mismo material.
    BLOCK_TECHO_HOJAS_L1,           // 192
    BLOCK_TECHO_HOJAS_L2,           // 193
    BLOCK_TECHO_HOJAS_L3,           // 194
    BLOCK_TECHO_HOJAS_L4,           // 195
    BLOCK_TECHO_HOJAS_L5,           // 196
    BLOCK_TECHO_HOJAS_L6,           // 197
    BLOCK_TECHO_HOJAS_L7,           // 198

    // ========================================================================
    // CUATRO PINOS MEXICANOS MAS
    // ========================================================================
    // Con estos el motor pasa de dos pinos a seis, y todos son especies de
    // los bosques de pino-encino del centro de Mexico. Cada uno lleva las
    // CUATRO piezas del patron que ya siguen encino, oyamel y los ocotes:
    //
    //     TRONCO   la corteza, que es lo que se ve en pie
    //     DENTRO   el corte con los anillos, al talarlo
    //     HOJAS    la acicula
    //     TABLONES lo que sale de labrar la madera
    //
    // Son cuatro texturas distintas y `texSegura()` trabaja con BlockType, de
    // modo que cada imagen que el mesher pueda pedir necesita su nombre aqui.
    //
    // ⚠️ VAN AL FINAL DEL ENUM. Insertar en medio correria los IDs de todo lo
    // que va detras y los mundos guardados leerian otro bloque. Es la misma
    // regla que siguieron el ocote, el agave abisaii y el techo de hojas.
    //
    // NO llevan entrada de RAMA propia: las cuatro usan el ramaje dentro del
    // follaje de su pariente mas cercano (ver generarOcote), que es lo que ya
    // hacen los dos ocotes desde que la rama y las aciculas comparten voxel.

    // --- PINO PATULA (Pinus patula) ---
    // El "pino llorón": acicula muy larga, fina y COLGANTE, corteza rojiza
    // que se desprende en placas papiraceas en la parte alta del fuste.
    BLOCK_WOOD_PATULA,              // 199 Tronco: corteza rojiza escamosa
    BLOCK_WOOD_PATULA_DENTRO,       // 200 El corte, con sus anillos
    BLOCK_LEAVES_PATULA,            // 201 Aciculas largas y pendulas
    BLOCK_PLANKS_PATULA,            // 202 Tablones de pino patula

    // --- PINO PIÑONERO MEXICANO (Pinus cembroides) ---
    // El pino del piñon. Es el MAS PEQUEÑO de los seis y el unico de zona
    // seca: vive en el matorral y la ladera arida, no en el bosque humedo.
    BLOCK_WOOD_PINONERO,            // 203 Tronco: corteza gris agrietada
    BLOCK_WOOD_PINONERO_DENTRO,     // 204 El corte, con sus anillos
    BLOCK_LEAVES_PINONERO,          // 205 Aciculas cortas y rigidas
    BLOCK_PLANKS_PINONERO,          // 206 Tablones de pino piñonero

    // --- PINO DE LAS ALTURAS (Pinus hartwegii) ---
    // El arbol que crece MAS ALTO del mundo: forma el limite arboreo de los
    // volcanes mexicanos, por encima de los 4.000 m. Porte bajo y achaparrado
    // por el viento y el frio.
    BLOCK_WOOD_ALTURAS,             // 207 Tronco: corteza gruesa y oscura
    BLOCK_WOOD_ALTURAS_DENTRO,      // 208 El corte, con sus anillos
    BLOCK_LEAVES_ALTURAS,           // 209 Aciculas en penacho denso
    BLOCK_PLANKS_ALTURAS,           // 210 Tablones de pino de las alturas

    // --- PINO PSEUDOSTROBUS (Pinus pseudostrobus) ---
    // El pino liso. Fuste recto y muy limpio, de los mas apreciados como
    // madera. Acicula larga y colgante, como la del patula.
    BLOCK_WOOD_PSEUDOSTROBUS,       // 211 Tronco: corteza lisa pardo grisacea
    BLOCK_WOOD_PSEUDOSTROBUS_DENTRO,// 212 El corte, con sus anillos
    BLOCK_LEAVES_PSEUDOSTROBUS,     // 213 Aciculas largas y colgantes
    BLOCK_PLANKS_PSEUDOSTROBUS,     // 214 Tablones de pseudostrobus

    // ========================================================================
    // ⭐⭐ EL TECHO DE HOJAS, EN SU VERSION APOYADA
    // ========================================================================
    // PEDIDO: "que puedas colocar y apilar mas niveles de techo de hojas
    // tanto arriba como abajo; si el jugador esta abajo del nivel, colocara
    // bloques bajo el nivel, y si esta arriba se apilan sobre el nivel".
    //
    // EL PROBLEMA. El techo de hojas CUELGA de la cara de arriba de su celda
    // --es lo que lo distingue de una alfombra-- y esa orientacion esta
    // cableada en el ID: `BLOCK_TECHO_HOJAS` siempre se dibuja de 16-h a 16.
    // Asi no hay forma de tener los dos sentidos a la vez.
    //
    // LA SOLUCION. Una familia HERMANA que se apoya en el suelo, como
    // cualquier otro nivel. El material es el mismo --misma textura, misma
    // receta, mismos drops-- y lo unico que cambia es de donde crece:
    //
    //     TECHO_HOJAS        cuelga del techo   (el jugador esta DEBAJO)
    //     TECHO_HOJAS_SUELO  se apoya en el suelo (el jugador esta ENCIMA)
    //
    // Cual de las dos se coloca lo decide la posicion del jugador respecto a
    // la celda, que es justo lo que se pidio.
    //
    // ⚠️ AL FINAL DEL ENUM, como todo lo nuevo: insertar en medio correria
    // los IDs y los mundos guardados leerian otro bloque.
    BLOCK_TECHO_SUELO,              // 215 Techo de hojas apoyado en el suelo

    // Sus siete niveles, contiguos y en orden (misma regla que los demas:
    // `tablaNiveles` guarda solo el primero y deduce el resto por el rango).
    BLOCK_TECHO_SUELO_L1,           // 216
    BLOCK_TECHO_SUELO_L2,           // 217
    BLOCK_TECHO_SUELO_L3,           // 218
    BLOCK_TECHO_SUELO_L4,           // 219
    BLOCK_TECHO_SUELO_L5,           // 220
    BLOCK_TECHO_SUELO_L6,           // 221
    BLOCK_TECHO_SUELO_L7            // 222
};

// Último bloque COLOCABLE de la lista contigua del terreno.
// Lo que va después son items y bloques retirados.
constexpr int BLOCK_LAST_PLACEABLE = BLOCK_SCRAP_L7;

// ============================================================================
// LA TIERRA QUE BEBE AGUA
// ============================================================================
// Que le pasa a este bloque cuando el agua lo empapa.
//
// Devuelve el MISMO bloque si no absorbe -- porque es impermeable (piedra,
// roca, madera) o porque ya esta saturado. El llamador usa esa igualdad como
// respuesta a "¿bebiste?", asi que no hay una segunda tabla que mantener.
//
// LA SATURACION ES LO QUE HACE QUE ESTO FUNCIONE. Si la tierra bebiera sin
// limite, la orilla de un mar se tragaria el mar entero: son millones de
// celdas de agua tocando arena. Al saturarse, la orilla se moja una vez y el
// mar se queda donde esta.
//
// La piedra no bebe. Un suelo de roca es lo que permite hacer un estanque que
// no se filtra.
inline BlockType versionMojada(BlockType t) {
    switch (t) {
        case BLOCK_DIRT:  return BLOCK_DIRT_MOJADA;
        case BLOCK_SAND:  return BLOCK_SAND_MOJADA;
        case BLOCK_GRASS: return BLOCK_GRASS_MOJADA;
        default:          return t;   // impermeable o ya saturado
    }
}

// ¿Este bloque esta empapado?
inline bool estaMojado(BlockType t) {
    return t == BLOCK_DIRT_MOJADA || t == BLOCK_SAND_MOJADA ||
           t == BLOCK_GRASS_MOJADA;
}

// El bloque seco del que viene este mojado. Sirve para que al romperlo suelte
// tierra normal y no un bloque "mojado" en el inventario.
inline BlockType versionSeca(BlockType t) {
    switch (t) {
        case BLOCK_DIRT_MOJADA:  return BLOCK_DIRT;
        case BLOCK_SAND_MOJADA:  return BLOCK_SAND;
        case BLOCK_GRASS_MOJADA: return BLOCK_GRASS;
        default:                 return t;
    }
}

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
// ⚠️ SUBIDO DE 22 A 24 al entrar el techo de hojas en `tablaRelleno`.
//
// Es el ANCHO del hueco que cada familia ocupa dentro del ID mixto, asi que
// cambiarlo cambia la codificacion -- igual que la altura del relleno. Los
// mundos guardados se traducen al cargar (ver BlockCompat), que es la misma
// via que ya se uso para ese cambio.
//
// Se deja margen (24 para 23 usadas) para que la proxima familia no obligue a
// otra migracion: ampliar este numero es barato en IDs (el rango llega a
// 100.000) y caro en compatibilidad.
constexpr int FAMILIAS_CON_NIVEL = 24;

// Primer ID del rango calculado. Va MUY por encima del enum para que jamás
// pise un bloque real, presente o futuro.
constexpr int BLOCK_MIXTO_BASE = 1000;

// ============================================================================
// ⭐⭐ UNA CELDA MIXTA YA NO ESTA SIEMPRE LLENA
// ============================================================================
// ANTES el relleno no tenia altura propia: iba de la capa de abajo AL TECHO,
// asi que una mixta solo podia representar 8 octavos EXACTOS.
//
// Eso causaba el bug reportado: al apilar arena sobre pasto, mientras el total
// no llegaba a 8 no se podia guardar "3 de pasto + 2 de arena" -- solo cabia
// "5 de algo"-- asi que la fusion devolvia 5 de PASTO. El jugador veia su
// pasto estirarse nivel a nivel y la arena no aparecia hasta completar el
// bloque, que es exactamente el sintoma descrito.
//
// AHORA el relleno lleva SU PROPIO NIVEL (1..7) dentro del ID. Con eso una
// celda puede guardar dos materiales a cualquier altura:
//
//     3 de pasto abajo  +  2 de arena encima  =  5 octavos, los dos visibles
//
// ⚠️ LAS DOS CAPAS VAN PEGADAS Y NO HAY AIRE DENTRO. El relleno empieza
// EXACTAMENTE donde acaba la capa de abajo (ver alturaBaseMixto) y la celda
// termina donde acaba el relleno. Lo que queda por encima no es "aire dentro
// del bloque": es el hueco de la celda, igual que sobre cualquier nivel
// parcial.
//
// COSTE EN IDs: se multiplica por 7 (las alturas del relleno). Pasa de 3.388
// IDs a 23.716, y el rango libre llega hasta 100.000
// (BLOQUE_COMPUESTO_BASE_ID), asi que cabe de sobra. Lo vigila un
// static_assert mas abajo.
constexpr int NIVELES_RELLENO = 7;

// Un ID mixto codifica:
//   ((indiceBase * 7 + (nivelBase-1)) * FAMILIAS + indiceRelleno)
//       * NIVELES_RELLENO + (nivelRelleno-1)
constexpr int MIXTO_POR_BASE = 7 * FAMILIAS_CON_NIVEL;
constexpr int BLOCK_MIXTO_FIN =
    BLOCK_MIXTO_BASE + FAMILIAS_CON_NIVEL * MIXTO_POR_BASE * NIVELES_RELLENO;

// El rango mixto NO puede alcanzar al de los bloques compuestos: si lo
// hiciera, una celda mixta se leeria como un maguey y al reves.
static_assert(BLOCK_MIXTO_FIN < 100000,
              "El rango de celdas mixtas pisa a los bloques compuestos");

// ¿Es una celda mixta (capa + relleno de otro material)?
inline bool esMixto(BlockType t) {
    return (int)t >= BLOCK_MIXTO_BASE && (int)t < BLOCK_MIXTO_FIN;
}

// ============================================================================
// DONDE EMPIEZAN LOS BLOQUES COMPUESTOS
// ============================================================================
// Los bloques con estado (maguey, biznaga...) viven en un rango de IDs muy
// por encima del enum. Su definicion completa esta en BloqueCompuesto.h, que
// incluye a ESTE archivo -- asi que aqui no se le puede preguntar.
//
// Se repite el numero, que es una constante del formato de guardado y no va a
// cambiar. Para que las dos copias no se separen nunca, BloqueCompuesto.h
// lleva un static_assert que compara la suya con esta: si alguien mueve una,
// el build falla al instante en vez de corromper mundos en silencio.
constexpr int BLOQUE_COMPUESTO_BASE_ID = 100000;

// Cuantos IDs ocupa cada familia compuesta (2^16 estados). Igual que el
// numero de arriba, esta copia la vigila un static_assert en
// BloqueCompuesto.h: si las dos se separan, el build falla.
constexpr int BLOQUE_COMPUESTO_ESTADOS = 65536;

// El indice de la familia AGUA dentro del rango compuesto.
//
// Hace falta AQUI porque el agua es la primera familia compuesta que NO es una
// planta, y este archivo tiene que poder distinguirla sin preguntarle a
// BloqueCompuesto.h (la dependencia va al reves). Tambien lo vigila un
// static_assert alli, contra el valor real del enum.
constexpr int BLOQUE_COMPUESTO_FAM_AGUA = 5;

// ¿Es una celda de agua con volumen?
//
// El agua vive en el rango compuesto (para heredar gratis el guardado del
// nivel), pero no es vegetacion ni se corta con el hacha, asi que todo lo que
// clasifica bloques tiene que apartarla antes de tratar el rango entero como
// plantas.
inline bool esAguaVolumen(BlockType t) {
    const int base = BLOQUE_COMPUESTO_BASE_ID +
                     BLOQUE_COMPUESTO_FAM_AGUA * BLOQUE_COMPUESTO_ESTADOS;
    return (int)t >= base && (int)t < base + BLOQUE_COMPUESTO_ESTADOS;
}

// ¿Es agua, de la que sea?
//
// ESTA es la pregunta que tiene que hacer el motor, y no `== BLOCK_WATER`.
//
// Hay DOS representaciones del agua conviviendo, a proposito:
//
//   BLOCK_WATER         el agua de siempre. Sigue existiendo porque los
//                       mundos guardados estan llenos de ella y el generador
//                       de terreno la sigue usando para llenar mares y rios.
//                       Cuenta como celda LLENA.
//   Compuesto::Agua     el agua con nivel, que aparece en cuanto el flujo
//                       toca una celda o el jugador coloca un cubo.
//
// Un mar guardado hace meses se lee como BLOCK_WATER y se convierte en agua
// con nivel sola, celda a celda, segun el jugador se acerca. Por eso no hay
// migracion que escribir ni version de formato que subir.
//
// El precio es que cada sitio que preguntaba `== BLOCK_WATER` tiene que
// preguntar esto. Los que no se cambien no reconoceran el agua nueva: se
// nadara raro, se colocaran bloques dentro o el mesher la dibujara mal.
inline bool esAguaCualquiera(BlockType t) {
    return t == BLOCK_WATER || esAguaVolumen(t);
}

// ¿Es una raíz, de cualquiera de los cuatro grosores?
inline bool esRaiz(BlockType t) {
    return t == BLOCK_RAIZ_PEQUENA || t == BLOCK_RAIZ_MEDIANA ||
           t == BLOCK_RAIZ_GRANDE  || t == BLOCK_RAIZ_ENORME;
}

// ¿Es follaje de ocote (las dos especies, suelto o con la rama dentro)?
//
// ⚠️ ESTO ES UNA COPIA DELIBERADA de Acicula::esAciculaOcote, y la razón es la
// misma que ya obliga a BlockType.h a duplicar las constantes de los bloques
// compuestos: la dependencia va en el otro sentido. AciculaOcote.h INCLUYE
// este archivo, así que este no puede incluir aquél sin crear un ciclo.
//
// El riesgo de una copia es que las dos se separen. Aquí no puede pasar sin
// que salte el build: AciculaOcote.h tiene un static_assert que comprueba que
// las dos funciones coinciden en las cuatro variantes.
constexpr bool Acicula_esFollajeOcote(BlockType t) {
    return t == BLOCK_LEAVES_OCOTE       || t == BLOCK_LEAVES_OCOTE_RAMA ||
           t == BLOCK_LEAVES_OCOTE_CHINO || t == BLOCK_LEAVES_OCOTE_CHINO_RAMA;
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
        // ⭐ LOS CINCO MINERALES: RETIRADOS, PERO SU SITIO SE QUEDA.
        //
        // Aqui habia carbon, plata, oro, diamante y desecho. Los bloques ya no
        // existen, pero estas cinco FILAS no se pueden borrar: el indice de
        // esta tabla viaja DENTRO del ID de cada celda mixta guardada
        // (base*7*FAMILIAS + relleno). Quitar una fila corre en uno todas las
        // de detras, y una capa de tierra con arena encima pasaria a leerse
        // como otra cosa en todos los mundos ya guardados.
        //
        // ⚠️ EL CENTINELA CONSERVA SU RANGO DE NIVELES, Y NO ES OPCIONAL.
        //
        // La tentacion es poner {BLOCK_AIR, BLOCK_AIR} y quedarse tranquilo.
        // Eso ROMPE EL MOTOR ENTERO, y conviene dejar escrito por que:
        // `bloqueBaseDe()` y `nivelDe()` no buscan por `entero`, buscan por el
        // RANGO [primero, primero+7). Con `primero` = BLOCK_AIR = 0, ese rango
        // es [0,7) -- o sea pasto, tierra, piedra, los troncos y el agua. La
        // primera fila centinela se tragaba la consulta y toda capa parcial
        // del juego respondia "soy aire de nivel 1". Medido: 62 casos rotos en
        // trece ficheros, incluida la fusion de materiales y las celdas
        // mixtas.
        //
        // Se conserva entonces `primero` (su rango de IDs sigue existiendo en
        // el enum y no se puede reutilizar) y solo se anula `entero`, que es
        // por donde preguntan indiceFamilia() y primerNivelDe(). Asi la fila
        // ocupa su posicion, sus lonchas viejas se siguen resolviendo, y no se
        // puede fabricar una celda mixta nueva con ella.
        { BLOCK_AIR,            BLOCK_COAL_L1    },  // 11 (era carbon)
        { BLOCK_AIR,            BLOCK_SILVER_L1  },  // 12 (era plata)
        { BLOCK_AIR,            BLOCK_GOLD_L1    },  // 13 (era oro)
        { BLOCK_AIR,            BLOCK_DIAMOND_L1 },  // 14 (era diamante)
        { BLOCK_AIR,            BLOCK_SCRAP_L1   },  // 15 (era desecho)

        // ⭐⭐ TECHO DE HOJAS -- VA AL FINAL, EN EL INDICE 16.
        //
        // ⚠️ NO puede ir en medio, y el intento de ponerlo arriba fallo el
        // test "Mixta: las 16 familias con niveles conservan su indice".
        //
        // El INDICE de esta tabla viaja DENTRO del ID de cada celda mixta ya
        // guardada (base*7*FAMILIAS + relleno), y las posiciones 0..15 tienen
        // que coincidir una a una con `tablaRelleno`. Insertar aqui en medio
        // corria los cinco centinelas de los minerales y, con ellos, la madera
        // de las posiciones 16-21: una capa de tierra con un tronco encima
        // pasaria a leerse como otra cosa en TODOS los mundos existentes.
        //
        // Al final es seguro: ocupa el primer hueco libre por detras de lo que
        // ya hay, sin mover nada. Quedan 5 huecos de los 22 reservados.
        { BLOCK_TECHO_HOJAS,    BLOCK_TECHO_HOJAS_L1 },

        // ⭐⭐ LOS TABLONES VUELVEN A TENER NIVELES (indices 17, 18 y 19).
        //
        // BUG REPORTADO: entre un tablon y un nivel queda una separacion de
        // aire, visible y fisica.
        //
        // LA CAUSA: los tablones salieron de esta tabla junto con los troncos,
        // y el argumento de entonces --"un tronco es un tronco entero, no una
        // loncha de 3 px"-- vale para el TRONCO pero no para el TABLON. Un
        // tablon ES una tabla: material de construccion plano, que es
        // justamente lo que uno quiere poder poner en capas.
        //
        // Sin fila aqui, `esNivelParcial()` decia NO sobre ellos y TODO el
        // sistema de apilado los trataba como cubos: no se fundian con la capa
        // de debajo, se quedaban encima, y entre los 3 px de la capa y los 16
        // del tablon quedaban 13 px de aire. Esa es la separacion reportada --
        // y era fisica ademas de visible, porque la colision sale de la misma
        // altura.
        //
        // ⚠️ LOS TRONCOS SIGUEN FUERA, a proposito: su veta y su forma se
        // pierden al partirlos, y ese razonamiento no ha cambiado.
        //
        // Van AL FINAL, detras del techo. Sus 21 IDs de nivel ya existian en
        // el enum (reservados al sacarlos de la tabla), asi que esto no mueve
        // ni un ID: solo los vuelve a dar de alta.
        { BLOCK_PLANKS,         BLOCK_PLANKS_L1  },
        { BLOCK_PLANKS_ENCINO,  BLOCK_PLANKE_L1  },
        { BLOCK_PLANKS_OYAMEL,  BLOCK_PLANKO_L1  },

        // ⭐ EL TECHO APOYADO (indice 20). La version hermana del techo que
        // cuelga: mismo material, pero crece desde el SUELO de su celda.
        //
        // Necesita sus propios niveles porque la altura es lo que se apila, y
        // el ID de nivel es lo que la guarda. Al final de la tabla, como todo
        // lo nuevo: el indice viaja dentro de cada celda mixta ya guardada.
        { BLOCK_TECHO_SUELO,    BLOCK_TECHO_SUELO_L1 },
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
        // ⭐ POSICIONES 11-15: LOS MINERALES RETIRADOS.
        //
        // Eran carbon, plata, oro, diamante y desecho. NO se borran, se
        // rellenan con BLOCK_AIR -- y el motivo esta justo debajo: la madera
        // ocupa las posiciones 16-21. Borrar estas cinco la correria a la
        // 11-16, y cada celda mixta con relleno de tronco ya guardada pasaria
        // a leerse como relleno de otra cosa.
        //
        // indiceRelleno() no puede devolver estos indices (rechaza BLOCK_AIR),
        // asi que son inalcanzables para las celdas nuevas.
        BLOCK_AIR,        BLOCK_AIR,        BLOCK_AIR,
        BLOCK_AIR,        BLOCK_AIR,
        // --- Los 6 huecos reservados: la madera, que no tiene niveles ---
        BLOCK_WOOD,       BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_PLANKS,     BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,
        // ⭐ EL TECHO DE HOJAS, EN LA POSICION 22.
        //
        // Se pidio que "los techos de hojas puedan tener eso encima y se
        // puedan colocar en el suelo o combinado con niveles". Para ser el
        // material de ARRIBA de una celda mixta hay que estar en esta tabla:
        // sin entrada aqui, `indiceRelleno` devuelve -1 y `mixto()` rechaza
        // la pareja.
        //
        // ⚠️ AL FINAL, NUNCA EN MEDIO. El indice viaja dentro del ID de cada
        // celda mixta ya guardada: insertarlo antes correria la madera y los
        // mundos existentes leerian otro relleno.
        BLOCK_TECHO_HOJAS,
    };
    n = (int)(sizeof(TABLA) / sizeof(TABLA[0]));
    return TABLA;
}

// Primer ID de cada familia con niveles. Devuelve BLOCK_AIR si el bloque no
// admite niveles (plantas, agua, lava, guijarros...).
inline BlockType primerNivelDe(BlockType completo) {
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    // El aire no tiene niveles, y ademas es el centinela de los minerales
    // retirados: sin esta guarda devolveria el BLOCK_AIR de la fila 11, que
    // por casualidad es el valor correcto, pero por el motivo equivocado.
    if (completo == BLOCK_AIR) return BLOCK_AIR;
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

// ============================================================================
// ⭐ EL TECHO DE HOJAS, EN SUS DOS ORIENTACIONES
// ============================================================================
// El mismo material con dos formas de crecer:
//
//     BLOCK_TECHO_HOJAS   cuelga de la cara de ARRIBA de su celda
//     BLOCK_TECHO_SUELO   se apoya en el SUELO, como cualquier otro nivel
//
// Cual se coloca lo decide donde este el jugador respecto a la celda: desde
// abajo cuelga (es un techo sobre su cabeza), desde arriba se apoya (es una
// capa a sus pies).
//
// Estos predicados existen para que el resto del motor no tenga que repetir
// la comparacion de los dos IDs. Es la leccion que ya dejaron el ocote y las
// hojas: una lista de especies repetida por diez sitios acaba desincronizada.

// ¿Es techo de hojas, de cualquier orientacion y nivel?
inline bool esTechoDeHojas(BlockType t) {
    const BlockType b = bloqueBaseDe(t);
    return b == BLOCK_TECHO_HOJAS || b == BLOCK_TECHO_SUELO;
}

// ¿Es de los que CUELGAN? Solo esta variante se dibuja de 16-h a 16.
inline bool esTechoColgante(BlockType t) {
    return bloqueBaseDe(t) == BLOCK_TECHO_HOJAS;
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
// ⭐ ¿SE COLOCA ENTERO, AUNQUE ADMITA NIVELES?
// ============================================================================
// La regla general del juego es que un bloque con niveles se coloca como CAPA
// (nivel 1) y crece clic a clic. Hay dos excepciones que van ENTERAS:
//
//   - la piedra labrada: el material de construccion, con el que se levantan
//     muros. Ocho clics por bloque harian inviable construir.
//   - el techo de hojas: se pidio que fuera "un bloque completo".
//
// Antes cada rama de placeBlock repetia la excepcion a mano, y en una se
// quedo fuera el techo: apuntando a la tapa de un bloque nacia como lamina.
// Ahora todas preguntan aqui.
//
// ⭐ LOS TABLONES TAMBIEN. PEDIDO: "que los tablones de todas las maderas
// sean bloques completos". Solo pino, encino y oyamel tienen niveles (los
// otros cinco nunca los tuvieron y ya iban enteros); a esos tres se les
// conservan los IDs de nivel para los mundos guardados, pero al colocarlos
// nacen enteros.

// ¿Es un tablon, de cualquiera de las ocho maderas?
inline bool esTablon(BlockType t) {
    switch (bloqueBaseDe(t)) {
        case BLOCK_PLANKS:        case BLOCK_PLANKS_ENCINO:
        case BLOCK_PLANKS_OYAMEL: case BLOCK_PLANKS_OCOTE:
        case BLOCK_PLANKS_PATULA: case BLOCK_PLANKS_PINONERO:
        case BLOCK_PLANKS_ALTURAS: case BLOCK_PLANKS_PSEUDOSTROBUS:
            return true;
        default:
            return false;
    }
}

inline bool seColocaEntero(BlockType t) {
    if (!admiteNiveles(t)) return false;     // sin niveles: no hay eleccion
    if (esNivelParcial(t)) return false;     // ya es una capa concreta
    const BlockType b = bloqueBaseDe(t);
    return b == BLOCK_COBBLESTONE || esTechoDeHojas(b) || esTablon(b);
}

// ============================================================================
// ⭐ IDs DE NIVEL HUERFANOS: LA MADERA QUE SALIO DE LA TABLA
// ============================================================================
// EL BUG QUE ESTO CORRIGE: el inventario creativo mostraba 42 casillas con
// textura de PIEDRA que no eran piedra.
//
// Son los niveles de la madera --BLOCK_PLANKS_L1..L7, PLANKE, PLANKO, WOOD,
// WOODE, WOODO-- y quedaron en un limbo:
//
//   1. La madera SALIO de tablaNiveles() a proposito (ver la nota alli): un
//      tronco es un tronco entero, no una loncha de 3 px.
//   2. Pero sus 42 IDs SIGUEN en el enum, y con razon -- quitarlos correria
//      los numeros de todo lo que va detras y los mundos guardados leerian
//      bloques equivocados.
//   3. Al salir de la tabla, `esNivelParcial()` empezo a decir NO sobre ellos.
//   4. Y el filtro del inventario creativo es exactamente
//      `if (esNivelParcial(bt)) continue;`
//
// O sea: dejaron de contar como nivel, asi que el filtro dejo de saltarlos y
// entraron al creativo. Y como getBlockTexture tampoco tiene entrada para
// ellos, caen en su `default:`, que devuelve Piedra.png. De ahi las 42
// casillas de "piedra" que no son piedra.
//
// Esto los reconoce por su RANGO en el enum, que es la misma tecnica que ya
// usa bloqueBaseDe para resolverlos: no dependen de la tabla, asi que la
// respuesta es correcta aunque la madera nunca vuelva a ella.
//
// NO se borran ni se les da textura: son bloques MUERTOS. Lo que hace falta es
// que nadie pueda colocarlos, y eso se consigue sacandolos del creativo.
inline bool esNivelDeMaderaHuerfano(BlockType t) {
    const int id = (int)t;
    // Los seis rangos, en el orden en que estan declarados en el enum. Se
    // comprueban contra los IDs de sus extremos, no contra numeros escritos a
    // mano: si el enum se reordena, esto sigue apuntando a lo correcto.
    // ⚠️ LOS TABLONES YA NO SON HUERFANOS.
    //
    // Volvieron a `tablaNiveles` para que se junten con las capas (ver la nota
    // alli). Sus 21 IDs ya no son bloques muertos: son niveles de verdad, con
    // textura y comportamiento. Dejarlos aqui los sacaria del creativo y haria
    // que `esNivelParcial` y esta funcion se contradijeran.
    //
    // Los TRONCOS siguen huerfanos: esos no volvieron a la tabla.
    return (id >= (int)BLOCK_WOOD_L1   && id <= (int)BLOCK_WOOD_L7)   ||
           (id >= (int)BLOCK_WOODE_L1  && id <= (int)BLOCK_WOODE_L7)  ||
           (id >= (int)BLOCK_WOODO_L1  && id <= (int)BLOCK_WOODO_L7);
}

// ============================================================================
// LOS HUECOS QUE DEJARON LOS MINERALES DE VETA
// ============================================================================
// Al retirarse los siete bloques de mineral, sus posiciones en el enum se
// conservaron como reservadas: borrarlas habria corrido en uno los IDs de todo
// lo que va detras y los mundos guardados leerian otro bloque.
//
// Pero un hueco reservado NO es un bloque. No se genera, no tiene textura y no
// se puede picar. Si entra en el bucle del inventario creativo sale como un
// cubo de piedra -- el mismo sintoma que las lonchas de madera de aqui arriba,
// y por la misma razon: getBlockTexture no tiene entrada para ellos y su
// `default:` devuelve Piedra.png.
//
// BLOCK_IRON_ORE no entra en esta lista porque vive DESPUES de
// BLOCK_LAST_PLACEABLE, asi que el bucle del creativo no llega hasta el.
inline bool esHuecoDeMineralRetirado(BlockType t) {
    switch ((int)t) {
        case BLOCK_RESERVADO_MINERAL_08:   // era carbon
        case BLOCK_RESERVADO_MINERAL_09:   // era desecho de metales
        case BLOCK_RESERVADO_MINERAL_20:   // era plata
        case BLOCK_RESERVADO_MINERAL_21:   // era diamante
        case BLOCK_RESERVADO_MINERAL_26:   // era oro
        case BLOCK_RESERVADO_MINERAL_164:  // era pirita
            return true;
        default:
            return false;
    }
}

// ============================================================================
// BLOQUES RETIRADOS QUE SIGUEN TENIENDO TEXTURA Y NOMBRE
// ============================================================================
// No es lo mismo que `esHuecoDeMineralRetirado`, y por eso es otra funcion.
//
// Un hueco de mineral no es un bloque: no se genera, no tiene textura propia y
// cae en el `default:` de getBlockTexture. Los de aqui SI son bloques hechos y
// derechos -- tienen textura, forma, tiempo de rotura y drop -- y siguen
// dibujandose en los mundos que ya los tengan. Lo unico que se les quita es
// poder aparecer: no se generan y no se pueden coger en creativo.
//
// La distincion importa porque el filtro del inventario ya salta los huecos por
// "no son bloques"; estos hay que saltarlos por "ya no se usan", que es una
// razon distinta y puede cambiar por separado.
inline bool esBloqueRetirado(BlockType t) {
    switch ((int)t) {
        // El tronco del nopal. Ver la nota en su entrada del enum: la planta
        // pasa a ser pencas encadenadas, sin fuste.
        case BLOCK_NOPAL_TALLO:
            return true;
        default:
            return false;
    }
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
    // ⚠️ EL AIRE NO ES UNA FAMILIA, y desde que los minerales se retiraron hay
    // cinco centinelas BLOCK_AIR ocupando su sitio en la tabla. Sin esta
    // guarda, preguntar por el aire devolveria el indice del primer centinela
    // y se podria fabricar una celda mixta "de aire con relleno".
    if (entero == BLOCK_AIR) return -1;
    for (int i = 0; i < n; ++i) if (T[i].entero == entero) return i;
    return -1;
}

// Posición dentro de tablaRelleno(), o -1 si el material no puede rellenar.
// Es quien puede ser el MATERIAL DE ARRIBA: no necesita niveles, porque
// siempre ocupa de la capa al techo.
inline int indiceRelleno(BlockType entero) {
    int n = 0; const BlockType* T = tablaRelleno(n);
    entero = bloqueBaseDe(entero);
    // Misma guarda que indiceFamilia: los centinelas de los minerales
    // retirados son BLOCK_AIR y no deben poder casarse con nada.
    if (entero == BLOCK_AIR) return -1;
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
// `nivelRelleno` es la ALTURA del material de arriba, en octavos (1..7).
//
// Por defecto 0 = "hasta el techo", que es como se comportaba antes: con eso
// las llamadas que solo quieren llenar la celda no cambian, y el valor se
// deriva de lo que queda libre (8 - nivel).
inline BlockType mixto(BlockType base, int nivel, BlockType relleno,
                       int nivelRelleno = 0) {
    // La capa de ABAJO necesita niveles (su altura es la del nivel). El
    // material de ARRIBA ahora tambien lleva la suya, para que una celda
    // pueda estar a medio llenar con dos materiales a la vista.
    const int ib = indiceFamilia(base);
    const int ir = indiceRelleno(relleno);
    if (ib < 0 || ir < 0) return BLOCK_AIR;      // no se puede representar
    if (nivel < 1 || nivel > 7) return BLOCK_AIR; // 8 es la celda llena

    // 0 significa "lo que quepa hasta el techo": el comportamiento de antes.
    int nr = (nivelRelleno <= 0) ? (8 - nivel) : nivelRelleno;

    // Las dos capas tienen que caber en la celda. Si no, se recorta: es mejor
    // perder un octavo que producir un ID fuera de rango.
    if (nr < 1) nr = 1;
    if (nivel + nr > 8) nr = 8 - nivel;

    return (BlockType)(BLOCK_MIXTO_BASE +
                       ((ib * 7 + (nivel - 1)) * FAMILIAS_CON_NIVEL + ir)
                           * NIVELES_RELLENO + (nr - 1));
}

// Las CUATRO partes de una celda mixta: material de abajo, su nivel, material
// de arriba y SU nivel.
//
// Se decodifica deshaciendo la aritmetica de `mixto()`, de dentro hacia
// fuera: primero el nivel del relleno (el factor mas interno), luego el
// relleno, luego el nivel de la base y por ultimo la base.
inline BlockType mixtoBase(BlockType t) {
    if (!esMixto(t)) return t;
    int n = 0; const FamiliaNivel* T = tablaNiveles(n);
    const int sinNR = ((int)t - BLOCK_MIXTO_BASE) / NIVELES_RELLENO;
    const int k = sinNR / FAMILIAS_CON_NIVEL;
    const int ib = k / 7;
    return (ib >= 0 && ib < n) ? T[ib].entero : BLOCK_AIR;
}
inline int mixtoNivel(BlockType t) {
    if (!esMixto(t)) return 8;
    const int sinNR = ((int)t - BLOCK_MIXTO_BASE) / NIVELES_RELLENO;
    const int k = sinNR / FAMILIAS_CON_NIVEL;
    return (k % 7) + 1;
}
inline BlockType mixtoRelleno(BlockType t) {
    if (!esMixto(t)) return BLOCK_AIR;
    int n = 0; const BlockType* T = tablaRelleno(n);
    const int sinNR = ((int)t - BLOCK_MIXTO_BASE) / NIVELES_RELLENO;
    const int ir = sinNR % FAMILIAS_CON_NIVEL;
    return (ir >= 0 && ir < n) ? T[ir] : BLOCK_AIR;
}

// ⭐ LA ALTURA DEL MATERIAL DE ARRIBA, en octavos (1..7).
//
// Es lo que permite que una celda mixta este a MEDIO LLENAR con los dos
// materiales a la vista, en vez de que el de arriba se estire hasta el techo.
inline int mixtoNivelRelleno(BlockType t) {
    if (!esMixto(t)) return 0;
    return (((int)t - BLOCK_MIXTO_BASE) % NIVELES_RELLENO) + 1;
}

// Cuantos octavos ocupa en total una celda mixta: las dos capas juntas.
// Puede ser menos de 8, que es justo la novedad.
inline int mixtoOctavosTotales(BlockType t) {
    if (!esMixto(t)) return 8;
    return mixtoNivel(t) + mixtoNivelRelleno(t);
}

// ============================================================================
// ⭐⭐⭐ CELDAS DE VARIAS CAPAS ("PILAS")
// ============================================================================
// PEDIDO: "elimina el bug de que cuando combinas 2 niveles y quieres poner
// otros, no te deje y solo estire el nivel 2 diferente; haz que sean por
// separado pero esten juntos los niveles".
//
// EL LIMITE QUE LO CAUSABA. Una celda mixta guarda DOS materiales y nada mas.
// Con "3 de tierra + 2 de arena" ya puesta, un tercer material no tenia donde
// ir, asi que fusionarNiveles lo sumaba a la capa de arriba: la arena se
// estiraba y la piedra que el jugador ponia no aparecia nunca.
//
// LA SOLUCION: un rango nuevo de IDs, la PILA, que guarda hasta TRES capas,
// cada una con su material y su altura, pegadas de abajo arriba:
//
//     ┌────────────┐ 16
//     │            │  hueco de la celda (si no esta llena)
//     ├────────────┤  6
//     │▒▒ PIEDRA ▒▒│  capa 3: 1 octavo
//     ├────────────┤  5
//     │░░ ARENA ░░░│  capa 2: 2 octavos
//     ├────────────┤  3
//     │██ TIERRA ██│  capa 1: 3 octavos
//     └────────────┘  0
//
// ⚠️ NO HACE FALTA MIGRAR NINGUN MUNDO. Las celdas de 1 y 2 capas siguen
// usando sus IDs de siempre (nivel parcial y mixta): `celdaDeCapas` elige la
// forma vieja SIEMPRE que puede. La pila solo aparece para lo que antes no se
// podia guardar -- tres capas, o un tronco/tablon que no cabe en la tabla de
// la mixta -- asi que lo ya guardado se lee exactamente igual.
//
// ⚠️ EL RANGO VA MUY LEJOS DE TODO: 2^28 en adelante. Los IDs viajan en 4
// bytes por todo el guardado (arrays crudos y paletas), igual que ya hacen
// las mixtas (1.000+) y los compuestos (100.000+).

constexpr int MAX_CAPAS = 3;

struct Capa {
    BlockType material = BLOCK_AIR;   // el bloque ENTERO del material
    int       octavos  = 0;           // su altura dentro de la celda, 1..8
};

struct Capas {
    int  n = 0;                       // cuantas hay, de abajo arriba
    Capa c[MAX_CAPAS];

    int total() const {
        int s = 0;
        for (int i = 0; i < n; ++i) s += c[i].octavos;
        return s;
    }
};

// Materiales que pueden formar capa en una pila.
//
// ⚠️ ORDEN INTOCABLE, como tablaRelleno: el INDICE viaja dentro del ID de cada
// pila guardada. Lo nuevo se anade AL FINAL. El 0 es "capa vacia".
//
// Aqui SI entran las ocho maderas, troncos y tablones: la tabla de la mixta
// se quedo sin huecos (24, todos usados o reservados) y por eso los cinco
// pinos nuevos no podian pegarse encima de una capa.
inline const BlockType* tablaPila(int& n) {
    static const BlockType TABLA[] = {
        BLOCK_AIR,                                                     // 0
        BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_SAND, BLOCK_GRAVEL,// 1-5
        BLOCK_SNOW, BLOCK_CLAY_DIRT, BLOCK_CLAY_SAND, BLOCK_COBBLESTONE,// 6-9
        BLOCK_CLAY, BLOCK_LIMESTONE, BLOCK_TECHO_HOJAS,                // 10-12
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,        // 13-15
        BLOCK_PLANKS_OCOTE, BLOCK_PLANKS_PATULA, BLOCK_PLANKS_PINONERO,// 16-18
        BLOCK_PLANKS_ALTURAS, BLOCK_PLANKS_PSEUDOSTROBUS,              // 19-20
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,              // 21-23
        BLOCK_WOOD_OCOTE, BLOCK_WOOD_OCOTE_CHINO, BLOCK_WOOD_PATULA,   // 24-26
        BLOCK_WOOD_PINONERO, BLOCK_WOOD_ALTURAS,                       // 27-28
        BLOCK_WOOD_PSEUDOSTROBUS,                                      // 29
    };
    n = (int)(sizeof(TABLA) / sizeof(TABLA[0]));
    return TABLA;
}

constexpr int PILA_BITS_MATERIAL = 6;     // hasta 64 materiales
constexpr int PILA_BITS_ALTURA   = 3;     // 1..8 guardado como 0..7
constexpr int PILA_BITS_CAPA     = PILA_BITS_MATERIAL + PILA_BITS_ALTURA;
constexpr int BLOCK_PILA_BASE    = 1 << 28;
constexpr int BLOCK_PILA_FIN     = BLOCK_PILA_BASE + (1 << (PILA_BITS_CAPA * MAX_CAPAS));

static_assert(BLOCK_PILA_FIN > BLOCK_PILA_BASE, "el rango de pilas desborda int");

// ¿Es una pila (celda de capas en el rango nuevo)?
inline bool esPila(BlockType t) {
    return (int)t >= BLOCK_PILA_BASE && (int)t < BLOCK_PILA_FIN;
}

// El material normalizado para la pila: el bloque entero, y las dos
// orientaciones del techo como una sola (dentro de una celda de capas el
// techo crece desde el suelo, como cualquier otra capa).
inline BlockType materialDeCapa(BlockType t) {
    t = bloqueBaseDe(t);
    if (t == BLOCK_TECHO_SUELO) t = BLOCK_TECHO_HOJAS;
    return t;
}

// Posicion del material en tablaPila, o -1 si no puede formar capa.
inline int indicePila(BlockType material) {
    material = materialDeCapa(material);
    if (material == BLOCK_AIR) return -1;
    int n = 0; const BlockType* T = tablaPila(n);
    for (int i = 1; i < n; ++i) if (T[i] == material) return i;
    return -1;
}

// Las capas de una celda, de abajo arriba. Vale para CUALQUIER celda:
//   aire               -> 0 capas
//   nivel parcial      -> 1 capa
//   bloque entero que puede ser capa (tierra, tronco, tablon...) -> 1 de 8
//   mixta              -> 2 capas
//   pila               -> 1 a 3 capas
//   lo demas (plantas, agua, items...) -> 0 capas: no es una celda de capas
inline Capas capasDe(BlockType t) {
    Capas r;
    if (t == BLOCK_AIR) return r;

    if (esPila(t)) {
        int nT = 0; const BlockType* T = tablaPila(nT);
        const int k = (int)t - BLOCK_PILA_BASE;
        for (int i = 0; i < MAX_CAPAS; ++i) {
            const int v   = (k >> (PILA_BITS_CAPA * i)) & ((1 << PILA_BITS_CAPA) - 1);
            const int mat = v >> PILA_BITS_ALTURA;
            if (mat <= 0 || mat >= nT) break;
            r.c[r.n].material = T[mat];
            r.c[r.n].octavos  = (v & ((1 << PILA_BITS_ALTURA) - 1)) + 1;
            ++r.n;
        }
        return r;
    }

    if (esMixto(t)) {
        r.n = 2;
        r.c[0] = { mixtoBase(t),    mixtoNivel(t) };
        r.c[1] = { materialDeCapa(mixtoRelleno(t)), mixtoNivelRelleno(t) };
        return r;
    }

    if (esNivelParcial(t)) {
        r.n = 1;
        r.c[0] = { bloqueBaseDe(t), nivelDe(t) };
        return r;
    }

    if (admiteNiveles(t) || indicePila(t) >= 0) {
        r.n = 1;
        r.c[0] = { bloqueBaseDe(t), 8 };
    }
    return r;
}

// El ID que representa estas capas, o BLOCK_AIR si no se puede.
//
// Elige SIEMPRE la forma mas antigua que sirva (nivel, mixta) y solo usa la
// pila para lo que no cabe en ellas. Es lo que hace que ningun mundo guardado
// necesite migracion, y que las celdas de siempre no cambien de ID.
//
// Antes de codificar: se descartan capas vacias y se UNEN las contiguas del
// mismo material (dos capas de tierra seguidas son una capa de tierra mas
// alta, no dos).
inline BlockType celdaDeCapas(Capas in) {
    Capas k;
    for (int i = 0; i < in.n; ++i) {
        if (in.c[i].octavos <= 0 || in.c[i].material == BLOCK_AIR) continue;
        const BlockType m = materialDeCapa(in.c[i].material);
        if (k.n > 0 && materialDeCapa(k.c[k.n - 1].material) == m) {
            k.c[k.n - 1].octavos += in.c[i].octavos;
            continue;
        }
        if (k.n >= MAX_CAPAS) return BLOCK_AIR;
        k.c[k.n++] = { in.c[i].material, in.c[i].octavos };
    }

    if (k.n == 0) return BLOCK_AIR;
    if (k.total() > 8) return BLOCK_AIR;

    // --- Una capa: el nivel o el bloque de siempre ---
    if (k.n == 1) {
        const BlockType m = k.c[0].material;
        const int h = k.c[0].octavos;
        if (h >= 8) return bloqueBaseDe(m);
        if (admiteNiveles(m)) return conNivel(m, h);
        // Un tronco o tablon sin niveles a media altura: solo la pila lo sabe
        // guardar (pasa al picar la capa de abajo de una celda de varias).
    }

    // --- Dos capas: la mixta, si la pareja cabe en sus tablas ---
    if (k.n == 2 && k.c[0].octavos <= 7 && k.c[1].octavos <= 7) {
        const BlockType m = mixto(k.c[0].material, k.c[0].octavos,
                                  k.c[1].material, k.c[1].octavos);
        if (m != BLOCK_AIR && mixtoNivel(m) == k.c[0].octavos &&
            mixtoNivelRelleno(m) == k.c[1].octavos)
            return m;
    }

    // --- La pila: lo que no cabe en las formas viejas ---
    int id = 0;
    for (int i = 0; i < k.n; ++i) {
        const int mat = indicePila(k.c[i].material);
        const int h   = k.c[i].octavos;
        if (mat < 0 || h < 1 || h > 8) return BLOCK_AIR;
        // Con varias capas ninguna llega a 8 (sumarian mas de una celda);
        // una sola de 8 ya se devolvio arriba como bloque entero.
        id |= ((mat << PILA_BITS_ALTURA) | (h - 1)) << (PILA_BITS_CAPA * i);
    }
    return (BlockType)(BLOCK_PILA_BASE + id);
}

// ¿Es una celda de varias capas, en cualquiera de sus dos formatos?
inline bool esCeldaDeCapas(BlockType t) {
    return esMixto(t) || esPila(t);
}

// El material de la capa de ARRIBA: lo que se ve, se pisa y se pica primero.
// Para lo que no es celda de capas devuelve el propio bloque.
inline BlockType materialArriba(BlockType t) {
    if (!esCeldaDeCapas(t)) return t;
    const Capas k = capasDe(t);
    return (k.n > 0) ? k.c[k.n - 1].material : BLOCK_AIR;
}

// El material de la capa de ABAJO: la que define la celda (nombre, color).
inline BlockType materialAbajo(BlockType t) {
    if (!esCeldaDeCapas(t)) return t;
    const Capas k = capasDe(t);
    return (k.n > 0) ? k.c[0].material : BLOCK_AIR;
}

// ============================================================================
// ⭐ ¿CUANTO MATERIAL HAY EN ESTA CELDA, EN OCTAVOS?
// ============================================================================
// La pregunta que hay que hacer para decidir si cabe algo mas. Antes el
// codigo preguntaba por el TIPO de bloque ("¿es un nivel parcial?", "¿es una
// mixta?") y eso dejo de valer cuando una mixta pudo estar a medio llenar:
// una celda de "3 de tierra + 2 de arena" no es un nivel parcial, pero tiene
// tres octavos libres.
//
// Tener la cuenta en un solo sitio evita que cada guarda la reinvente -- que
// es justo como aparecieron los bugs de "se cree que esta lleno".
inline int octavosOcupados(BlockType t) {
    if (t == BLOCK_AIR) return 0;
    if (esMixto(t)) return mixtoOctavosTotales(t);
    if (esPila(t))  return capasDe(t).total();
    if (esNivelParcial(t)) return nivelDe(t);
    return 8;                     // bloque entero
}

// ¿Esta celda esta LLENA de verdad? Es la unica forma correcta de preguntar
// "¿ya no cabe nada mas aqui?".
inline bool celdaLlena(BlockType t) {
    return octavosOcupados(t) >= 8;
}

// ¿Admite esta celda que se le anada material? Lo contrario de la anterior,
// pero exigiendo ademas que sea un material apilable (no aire, no planta).
inline bool celdaAdmiteMas(BlockType t) {
    if (t == BLOCK_AIR) return true;
    if (!esCeldaDeCapas(t) && !esNivelParcial(t) && !admiteNiveles(t)) return false;
    return octavosOcupados(t) < 8;
}

// Altura en bloques (0..1) que ocupa este bloque. Es su colision EXACTA.
//
// Una celda mixta está LLENA: la capa de abajo más el relleno llegan hasta
// arriba, así que se anda por encima como por un bloque entero.
inline float alturaDe(BlockType t) {
    // ⭐ UNA MIXTA YA NO ESTA SIEMPRE LLENA.
    //
    // Su altura es la de las DOS capas juntas, que puede ser menos de 8
    // octavos. Antes se devolvia 1.0 fijo porque el relleno llegaba siempre
    // al techo; ahora lleva su propia altura.
    //
    // Esto es la COLISION exacta del bloque, asi que de aqui depende que el
    // jugador se suba a la altura correcta y no flote ni se hunda.
    if (esCeldaDeCapas(t)) {
        const int oct = octavosOcupados(t);
        if (oct >= 8) return 1.0f;
        return (float)alturaNivelPx(oct) / 16.0f;
    }
    return (float)alturaNivelPx(nivelDe(t)) / 16.0f;
}

// ============================================================================
// ⭐ UN NIVEL QUE CAE SOBRE OTRO: SE JUNTAN, NO SE APILAN
// ============================================================================
// EL BUG QUE ESTO CORRIGE: un nivel de tierra que caía sobre un nivel de
// piedra no se fundía con él. El motor lo trataba como "celda ocupada",
// buscaba hueco en la celda de ARRIBA y lo dejaba ahí -- flotando sobre media
// celda vacía -- o, si no encontraba sitio en tres intentos, LO BORRABA.
//
// Lo correcto, y lo que se pidió, es que se SUMEN las alturas:
//
//     nivel 1 de tierra  +  nivel 1 de piedra   ->  nivel 2
//     nivel 3            +  nivel 4             ->  nivel 7
//     nivel 5            +  nivel 6             ->  celda llena + sobra 3
//
// Las texturas no se mezclan ni se inventa un material nuevo: la celda
// resultante conserva el material de ABAJO, que es el que estaba allí primero.
// Lo que cambia es su TAMAÑO.
//
// ----------------------------------------------------------------------------
// EL AGUA ES EL CASO APARTE
// ----------------------------------------------------------------------------
// Un sólido que cae en agua no se suma con ella: la DESPLAZA. Un nivel 1 de
// tierra cayendo sobre agua deja un nivel 1 de tierra, y el agua se va. Es lo
// que se pidió, y además es lo que ya hace el resto del motor -- colocar un
// bloque dentro del agua la reparte a los lados en vez de borrarla (ver
// repartirAguaDesplazada en main.cpp).
//
// Por eso esta función NO decide sobre el agua: devuelve SOLO_EL_QUE_CAE y
// deja que el llamante haga el desplazamiento con las rutinas que ya existen.
// ============================================================================

// Qué hay que hacer cuando `queCae` aterriza sobre `enDestino`.
enum class ResultadoFusion {
    NO_FUSIONA,        // no son fusionables: el que cae se queda encima
    SOLO_EL_QUE_CAE,   // el destino desaparece (agua desplazada)
    FUSIONADOS,        // se juntan: ver `nivelResultante` y `sobrante`

    // ⭐ EL DESTINO ESTA LLENO: TODO SUBE, PERO NO SE PIERDE NADA.
    //
    // Una celda ya llena --un cubo entero o una mixta de dos materiales-- no
    // admite un octavo mas. Antes eso devolvia NO_FUSIONA, y el llamante
    // dejaba el bloque "encima" sin mas: ahi se cortaba la cadena de apilado
    // y aparecian las barreras.
    //
    // Ahora se dice explicitamente "aqui no cabe, subelo entero": `sobrante`
    // lleva lo que cae CON SU NIVEL Y SU MATERIAL INTACTOS, para que vuelva a
    // fusionarse en la celda de encima. Asi la columna sigue creciendo pase
    // lo que pase, que es lo que hace el apilado de verdad ilimitado.
    SUBE_ENTERO
};

struct Fusion {
    ResultadoFusion tipo = ResultadoFusion::NO_FUSIONA;
    BlockType resultado  = BLOCK_AIR;   // lo que queda en la celda de destino
    BlockType sobrante   = BLOCK_AIR;   // lo que no cupo, va en la celda de arriba
};

// ¿Es agua, en cualquiera de sus formas? Se declara aquí porque la fusión la
// necesita y `esAguaVolumen` ya vive más arriba en este mismo archivo.
inline bool esAguaParaFusion(BlockType t) {
    return t == BLOCK_WATER || esAguaVolumen(t);
}

// Calcula qué pasa al caer `queCae` sobre `enDestino`.
//
// Funciona con CUALQUIER par de materiales con niveles y con cualquier
// combinación de niveles, que es lo que se pidió: no hay tabla de casos ni
// lista de parejas permitidas, solo aritmética sobre las alturas.
inline Fusion fusionarNiveles(BlockType queCae, BlockType enDestino) {
    Fusion f;

    // ========================================================================
    // ⭐⭐ UN BLOQUE ENTERO TAMBIEN SE JUNTA CON UN DESNIVEL
    // ========================================================================
    // PEDIDO: "que los bloques enteros puedan juntarse con cualquier nivel o
    // desnivel de bloques sin problemas".
    //
    // ANTES: esta funcion solo admitia NIVELES PARCIALES como lo que cae. Un
    // bloque ENTERO devolvia NO_FUSIONA y se quedaba encima, de modo que entre
    // la capa de 3 px y el cubo de 16 quedaban 13 px de aire a la vista. El
    // jugador veia su bloque FLOTANDO sobre la capa.
    //
    // Y no habia forma de arreglarlo desde fuera: una celda no puede estar a
    // media altura, asi que "bajar el cubo" no es una opcion.
    //
    // AHORA un bloque entero cuenta como OCHO OCTAVOS y entra en la misma
    // aritmetica que todo lo demas. Como el destino ya tiene algo, el total
    // pasa de 8 y la cuenta se resuelve sola por el camino que ya existia:
    //
    //     capa de tierra 3/8  +  piedra entera 8/8  =  11/8
    //       -> la celda de abajo queda LLENA (mixta tierra+piedra)
    //       -> y los 3/8 que sobran SUBEN como piedra a la celda de encima
    //
    // Ni un octavo se pierde ni se inventa, que es el invariante que ya
    // vigilan los tests del apilado.
    //
    // ⚠️ DOS ENTEROS SIGUEN SIN FUSIONARSE. 8 + 8 = 16 son exactamente dos
    // celdas llenas: no hay nada que repartir y el resultado seria el mismo
    // que apilarlos. Se devuelve NO_FUSIONA para que el llamante siga el
    // camino barato de siempre y no pague una cuenta inutil.
    const bool caeEntero = !esNivelParcial(queCae) && admiteNiveles(queCae);

    // Lo que no admite niveles (plantas, guijarros, items) no entra en esta
    // aritmetica: no tiene octavos que repartir.
    if (!esNivelParcial(queCae) && !caeEntero) return f;

    // --- EL AGUA SE DESPLAZA, NO SE SUMA ---
    // El sólido ocupa la celda y el agua se va. El llamante es quien la
    // reparte a los lados: aquí solo se dice qué queda.
    if (esAguaParaFusion(enDestino)) {
        f.tipo = ResultadoFusion::SOLO_EL_QUE_CAE;
        f.resultado = queCae;
        return f;
    }

    // ========================================================================
    // ⭐⭐ EL DESTINO ESTA LLENO: NO SE CORTA, SE SUBE
    // ========================================================================
    // PEDIDO: "que todos los niveles y tipos de niveles de 1-8 puedan
    // combinarse sin limite y no creen barreras".
    //
    // AQUI ESTABAN LAS BARRERAS. Esta guarda devolvia NO_FUSIONA para todo
    // destino que no fuera un nivel parcial, o sea:
    //
    //   - una celda MIXTA (ya tiene sus dos materiales y esta llena)
    //   - un bloque ENTERO (ocho octavos, sin hueco)
    //
    // Y NO_FUSIONA significa "el que cae se queda encima sin tocar nada": la
    // cadena de apilado se cortaba ahi. Apilando sobre una celda que acababa
    // de quedar mixta, el siguiente bloque ya no se fundia con nada y la
    // columna dejaba de comportarse como el resto -- esa es la barrera que se
    // ve al construir.
    //
    // La aritmetica de abajo nunca fue el limite: funciona con cualquier par
    // de materiales y cualquier combinacion de 1 a 8. El limite era esta
    // puerta de entrada.
    //
    // AHORA se responde lo correcto: "aqui no cabe, subelo ENTERO". El
    // material sube con su nivel intacto a la celda de encima y alli vuelve a
    // pasar por esta misma funcion, que lo fundira con lo que haya. La
    // columna crece sin tope y sin perder ni un octavo -- el unico limite es
    // la altura del mundo, como debe ser.
    //
    // ⚠️ Esto incluye el caso ENTERO SOBRE ENTERO, que antes tenia su propia
    // guarda: 8 + 8 son dos celdas llenas, y "subir entero" es exactamente el
    // resultado correcto.
    // ⚠️⚠️ "LLENO" SE MIDE EN OCTAVOS, NO POR EL TIPO DE BLOQUE.
    //
    // BUG QUE ESTO CORRIGE: EL HUECO DE AIRE DENTRO DE LA COLUMNA.
    //
    // Antes la condicion era solo `!esNivelParcial(enDestino)`, y una CELDA
    // MIXTA no es un nivel parcial -- asi que entraba aqui siempre. Pero
    // desde que el relleno tiene altura propia, una mixta puede estar a MEDIO
    // LLENAR: "3 de madera + 2 de tierra" son 5 octavos con 3 libres arriba.
    //
    // Al subir el material a la celda de encima, este nacia en el suelo de
    // ESA celda -- 3 octavos mas arriba del borde real-- y entre las dos
    // capas quedaba una franja de aire a la vista. Es justo lo que se ve en
    // la captura: madera abajo, hueco, y madera flotando encima.
    //
    // La pregunta correcta no es "¿es un nivel parcial?" sino "¿QUEDA SITIO?".
    // Si quedan octavos libres, se rellenan primero; solo cuando la celda
    // esta de verdad a 8 se sube lo que sobra.
    const int octavosDestino =
        esCeldaDeCapas(enDestino) ? octavosOcupados(enDestino)
      : esNivelParcial(enDestino) ? nivelDe(enDestino)
                                  : 8;    // bloque entero

    if (octavosDestino >= 8) {
        f.tipo      = ResultadoFusion::SUBE_ENTERO;
        f.resultado = enDestino;   // lo de abajo no se toca
        f.sobrante  = queCae;      // y lo que cae sube tal cual
        return f;
    }

    // ========================================================================
    // ⭐ UNA MIXTA A MEDIO LLENAR: SE LE ANADE AL RELLENO
    // ========================================================================
    // Queda sitio en la celda, asi que lo que cae entra AQUI en vez de subir.
    // No puede quedar aire dentro de la columna.
    //
    // La celda ya tiene sus dos materiales (base y relleno), y el formato no
    // admite un tercero. Lo que cae se suma al RELLENO, que es la capa de
    // arriba y la que toca: es el mismo criterio que usa la rama de materiales
    // distintos --conservar el material de abajo-- aplicado un piso mas
    // arriba.
    //
    // Lo que no quepa sube, y alli vuelve a fusionarse con lo que haya.
    //
    // ⭐⭐⭐ Y SI EL MATERIAL ES OTRO, VA EN SU PROPIA CAPA.
    //
    // BUG QUE ESTO CORRIGE: "cuando combinas 2 niveles y quieres poner otros,
    // no te deje y solo estire el nivel 2 diferente".
    //
    // Aqui se sumaba SIEMPRE a la capa de arriba, fuera cual fuera el
    // material que llegaba, porque la mixta no sabia guardar un tercero:
    // tierra(3) + arena(2), y al poner piedra, la ARENA crecia y la piedra no
    // aparecia nunca.
    //
    // Ahora:
    //   - mismo material que la capa de arriba -> esa capa crece (lo de
    //     siempre: dos capas de arena seguidas son una capa de arena);
    //   - material distinto -> CAPA NUEVA encima, con su altura, pegada a la
    //     de debajo. La celda pasa a ser una pila (hasta 3 capas).
    //
    // Si la celda ya tiene 3 capas o el material no puede formar capa, se
    // vuelve a lo de antes (crece la de arriba): nunca se pierde ni se
    // inventa un octavo, y nada flota.
    if (esCeldaDeCapas(enDestino)) {
        const int libre = 8 - octavosDestino;
        const int nCaeM = esNivelParcial(queCae) ? nivelDe(queCae) : 8;
        const int entra = (nCaeM < libre) ? nCaeM : libre;
        const int sobra = nCaeM - entra;

        const Capas antes = capasDe(enDestino);

        // Intento 1: el material que llega, en su propia capa encima. Si es el
        // mismo que el de arriba, celdaDeCapas las une en una sola mas alta.
        Capas conNueva = antes;
        BlockType nueva = BLOCK_AIR;
        if (conNueva.n < MAX_CAPAS ||
            materialDeCapa(queCae) == materialDeCapa(antes.c[antes.n - 1].material)) {
            if (conNueva.n < MAX_CAPAS) {
                conNueva.c[conNueva.n++] = { bloqueBaseDe(queCae), entra };
            } else {
                conNueva.c[conNueva.n - 1].octavos += entra;
            }
            nueva = celdaDeCapas(conNueva);
        }

        // Intento 2 (sin sitio para otra capa, o material que no puede ser
        // capa): crece la de arriba, como antes.
        if (nueva == BLOCK_AIR) {
            Capas crece = antes;
            crece.c[crece.n - 1].octavos += entra;
            nueva = celdaDeCapas(crece);
        }

        f.tipo      = ResultadoFusion::FUSIONADOS;
        f.resultado = (nueva != BLOCK_AIR) ? nueva : enDestino;
        f.sobrante  = (sobra <= 0) ? BLOCK_AIR
                    : (sobra >= 8) ? bloqueBaseDe(queCae)
                                   : conNivel(bloqueBaseDe(queCae), sobra);
        return f;
    }

    const int nCae     = nivelDe(queCae);
    const int nDestino = nivelDe(enDestino);
    const int total    = nCae + nDestino;

    const BlockType matAbajo  = bloqueBaseDe(enDestino);
    const BlockType matArriba = bloqueBaseDe(queCae);

    f.tipo = ResultadoFusion::FUSIONADOS;

    // ========================================================================
    // ⭐ MISMO MATERIAL: SE SUMAN LAS ALTURAS
    // ========================================================================
    // Es el caso simple y el unico que existia antes. Dos capas de tierra son
    // una capa de tierra mas alta: no hay dos materiales que conservar.
    // ⭐ LAS DOS ORIENTACIONES DEL TECHO SON EL MISMO MATERIAL.
    //
    // `BLOCK_TECHO_HOJAS` (cuelga) y `BLOCK_TECHO_SUELO` (se apoya) son la
    // misma hoja: lo unico que cambia es de donde crece dentro de la celda.
    // Sin esta equivalencia, apilar uno sobre otro formaria una celda MIXTA
    // de "techo + techo" -- dos capas del mismo material con una frontera
    // visible en medio, que es justo el bug que se quiere evitar.
    //
    // Se conserva la orientacion de la capa de ABAJO, que es la que estaba
    // primero: si empezaste colgando, sigue colgando al engordar.
    const bool mismoMaterial =
        (matAbajo == matArriba) ||
        (esTechoDeHojas(matAbajo) && esTechoDeHojas(matArriba));

    if (mismoMaterial) {
        if (total <= 7) {
            f.resultado = conNivel(matAbajo, total);
            f.sobrante  = BLOCK_AIR;
        } else if (total == 8) {
            f.resultado = bloqueBaseDe(matAbajo);   // celda llena
            f.sobrante  = BLOCK_AIR;
        } else {
            f.resultado = bloqueBaseDe(matAbajo);
            f.sobrante  = conNivel(matAbajo, total - 8);
        }
        return f;
    }

    // ========================================================================
    // ⭐ MATERIALES DISTINTOS: CELDA MIXTA, Y LO QUE NO CABE SUBE
    // ========================================================================
    // ESTO ES LO QUE SE PIDIO: que al juntar niveles de tipos distintos cada
    // uno conserve SU material y SU altura, en vez de que el de arriba se
    // convierta en el de abajo.
    //
    // Antes esta funcion hacia `material = bloqueBaseDe(enDestino)` para los
    // dos casos, asi que arena cayendo sobre tierra daba... mas tierra. El
    // material que caia se perdia, y con el la razon de haberlo tirado ahi.
    //
    // ------------------------------------------------------------------------
    // POR QUE EL QUE CAE VA ARRIBA
    // ------------------------------------------------------------------------
    // Porque es lo que hace la gravedad: lo que llega despues se posa ENCIMA
    // de lo que ya estaba. La celda mixta guarda exactamente eso -- una capa
    // de abajo con su nivel, y un relleno encima.
    //
    // ------------------------------------------------------------------------
    // EL DESBORDE ES LO QUE HACE EL APILADO INFINITO
    // ------------------------------------------------------------------------
    // Una celda solo guarda DOS materiales. Con tres o mas, la solucion no es
    // agrandar la celda --eso cambiaria el formato de guardado y romperia los
    // mundos existentes-- sino dejar que la columna crezca: lo que no cabe
    // sube a la celda de encima CON SU MATERIAL INTACTO, y alli vuelve a
    // fusionarse con lo que haya.
    //
    // Asi, apilando arena sobre tierra sobre grava sobre nieve, cada celda
    // acaba con dos materiales y la pila sigue subiendo. No hay limite de
    // tipos ni de combinaciones: el limite es la altura del mundo.
    // ------------------------------------------------------------------------
    // ⚠️ UNA CELDA MIXTA ESTA SIEMPRE LLENA. ESO MANDA SOBRE TODO LO DEMAS.
    // ------------------------------------------------------------------------
    // El relleno de una celda mixta va de la capa de abajo AL TECHO: no tiene
    // nivel propio (ver `mixto`, que ni siquiera lo recibe). Asi que una mixta
    // solo puede representar exactamente 8 octavos.
    //
    // Consecuencia, y es la trampa de todo esto: si el total es MENOR que 8 y
    // se forma una mixta igualmente, el relleno se estira hasta arriba y
    // aparece material de la nada. Tierra(4) + arena(3) = 7 se convertiria en
    // una celda de 8. Lo caza el test "ni un octavo se pierde ni se inventa",
    // y fue exactamente el fallo de la primera version de esta funcion.
    //
    // Por eso hay tres casos y no dos:
    //
    //   total <  8   la mixta NO vale. Sube el nivel de la capa de abajo.
    //   total == 8   encaje perfecto: mixta con los dos materiales.
    //   total >  8   mixta llena aqui, y lo que sobra sube con SU material.
    if (total < 8) {
        // ====================================================================
        // ⭐⭐ NO LLENAN LA CELDA: SE SUMAN AQUI, PEGADOS AL SUELO
        // ====================================================================
        // BUG QUE ESTO CORRIGE: EL NIVEL QUE FLOTABA.
        //
        // La version anterior dejaba los dos materiales SEPARADOS --la capa de
        // abajo intacta y la nueva subiendo a la celda de encima-- para que
        // cada uno conservara su identidad:
        //
        //     3 de tierra + 2 de arena  ->  abajo 3 tierra
        //                                   arriba 2 arena
        //
        // El volumen se conservaba, pero el resultado EN PANTALLA estaba mal:
        // los niveles de una celda se dibujan desde SU PROPIO SUELO, asi que
        // esos 2 de arena no se apoyaban sobre los 3 de tierra -- nacian en el
        // suelo de la celda de arriba, 16 px mas alto. Entre los dos quedaban
        // 13 px de aire y la arena se veia FLOTANDO.
        //
        // LA REGLA CORRECTA, y la que se pidio: se suman las alturas en la
        // MISMA celda y el resultado conserva el material de ABAJO, que es el
        // que estaba primero.
        //
        //     3 de tierra + 2 de arena  ->  5 de TIERRA, pegado al suelo
        //
        // POR QUE NO SE PUEDE CONSERVAR LA ARENA. Una celda MIXTA es la unica
        // forma de guardar dos materiales en un voxel, y esta SIEMPRE LLENA:
        // su relleno va de la capa de abajo al techo, sin nivel propio (ver
        // `mixto`, que ni siquiera lo recibe). Formar una mixta con total 5
        // estiraria la arena hasta arriba e inventaria 3 octavos de la nada --
        // justo lo que caza el test "ni un octavo se pierde ni se inventa".
        //
        // Asi que de las tres salidas posibles --flotar, inventar material, o
        // perder la identidad del de arriba-- esta es la unica que no rompe
        // nada visible ni el invariante. El coste es que la arena se vea como
        // tierra; a cambio NADA FLOTA NUNCA, que es lo que se pidio.
        //
        // ⭐⭐ ACTUALIZADO: AHORA SI SE CONSERVAN LOS DOS MATERIALES.
        //
        // Lo de arriba describe por que antes habia que elegir entre flotar,
        // inventar material o perder la identidad del de encima: una celda
        // mixta solo sabia representar 8 octavos exactos.
        //
        // Ya no. El relleno lleva SU PROPIA ALTURA dentro del ID (ver
        // NIVELES_RELLENO), asi que una celda puede guardar "3 de pasto + 2
        // de arena = 5" con los dos materiales a la vista y pegados entre si.
        //
        // Eso cierra el bug que quedaba: apilar arena sobre pasto ya NO se ve
        // como pasto estirandose hasta el octavo nivel. La arena aparece
        // desde el primer clic, encima del pasto, a su altura real.
        //
        // Si los dos materiales no se pueden combinar en una mixta (alguno no
        // esta en las tablas), se cae al comportamiento anterior: se suman
        // como el material de abajo. Nunca se deja flotar nada.
        //
        // ⚠️ EL MISMO MATERIAL NO ENTRA AQUI: la rama de arriba ya lo sumo.
        // Esto es solo para materiales DISTINTOS.
        // celdaDeCapas elige la mixta de siempre si la pareja cabe, y la
        // pila si no (los materiales que no estan en la tabla de la mixta).
        Capas dos;
        dos.n = 2;
        dos.c[0] = { matAbajo,  nDestino };
        dos.c[1] = { matArriba, nCae };
        const BlockType mezclaParcial = celdaDeCapas(dos);
        f.resultado = (mezclaParcial != BLOCK_AIR)
                        ? mezclaParcial
                        : conNivel(matAbajo, total);
        f.sobrante  = BLOCK_AIR;      // nada sube: todo cabe en esta celda
        return f;
    }

    if (total == 8) {
        // Encaje perfecto: los dos materiales caben tal cual y la celda queda
        // llena. Este es el caso bonito -- abajo su nivel, arriba el resto.
        Capas dos;
        dos.n = 2;
        dos.c[0] = { matAbajo,  nDestino };
        dos.c[1] = { matArriba, 8 - nDestino };
        const BlockType mezcla = celdaDeCapas(dos);
        f.resultado = (mezcla != BLOCK_AIR) ? mezcla : bloqueBaseDe(matAbajo);
        f.sobrante  = BLOCK_AIR;
        return f;
    }

    // total > 8: la celda se llena con lo que quepa del que cae, y el resto
    // sube manteniendo su material.
    //
    // `nDestino` es la altura de la capa de abajo, asi que el hueco que queda
    // es 8 - nDestino. Eso se queda aqui; lo demas sube y vuelve a fusionarse
    // alli con lo que haya -- que es lo que hace el apilado ilimitado.
    const int cabe  = 8 - nDestino;          // 1..7, porque nDestino <= 7
    const int sobra = nCae - cabe;           // > 0 por la condicion de arriba

    Capas dos;
    dos.n = 2;
    dos.c[0] = { matAbajo,  nDestino };
    dos.c[1] = { matArriba, cabe };
    const BlockType mezcla = celdaDeCapas(dos);
    f.resultado = (mezcla != BLOCK_AIR) ? mezcla : bloqueBaseDe(matAbajo);
    f.sobrante  = (sobra >= 8) ? bloqueBaseDe(matArriba)
                               : conNivel(matArriba, sobra);
    return f;
}

// ============================================================================
// ⭐⭐ DEPOSITAR MATERIAL EN UNA COLUMNA, SIN PERDER NADA
// ============================================================================
// PEDIDO: "que en cualquier direccion de mirada y posicion del jugador puedas
// juntar niveles sin limites".
//
// Empieza en la celda `y`: lo que quepa se funde ahi y lo que sobra sube, celda
// a celda, fundiendose con lo que encuentre.
//
// SOLO PLANIFICA. Devuelve las escrituras sin tocar el mundo, y false si el
// material no tiene donde acabar (un bloque que no es capa en medio, el techo
// del mundo). Es lo que permite no gastar el item ni perder un octavo: antes,
// lo que sobraba se tiraba si la celda de arriba no estaba vacia.
//
//     get(y)        -> BlockType   que hay en esa altura de la columna
//     esLibre(tipo) -> bool        se puede sustituir (aire, hierba)
struct EscrituraColumna { int y; BlockType tipo; };

constexpr int MAX_ESCRITURAS_COLUMNA = 4;

template <class Get, class EsLibre>
bool planearDeposito(Get get, EsLibre esLibre, int y, int yMax, BlockType material,
                     EscrituraColumna* out, int& n) {
    n = 0;
    for (int paso = 0; paso < 8; ++paso, ++y) {
        if (y < 0 || y >= yMax) return false;

        const BlockType celda = get(y);
        if (esLibre(celda)) {
            if (n >= MAX_ESCRITURAS_COLUMNA) return false;
            out[n++] = { y, material };
            return true;
        }

        const Fusion f = fusionarNiveles(material, celda);
        // Solo se salta la celda de PARTIDA, si esta llena. Mas arriba, lo que
        // sobra no atraviesa un bloque: acabaria encima de un techo.
        if (f.tipo == ResultadoFusion::SUBE_ENTERO) {
            if (paso == 0) continue;
            return false;
        }
        if (f.tipo != ResultadoFusion::FUSIONADOS) return false;

        if (n >= MAX_ESCRITURAS_COLUMNA) return false;
        out[n++] = { y, f.resultado };
        if (f.sobrante == BLOCK_AIR) return true;
        material = f.sobrante;
    }
    return false;
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

// ============================================================================
// CELDAS COMPARTIDAS: VARIOS BLOQUES EN EL MISMO VOXEL
// ============================================================================
// Un voxel guarda UN solo BlockType, pero eso no obliga a que dentro haya una
// sola cosa: el ID puede describir un CONJUNTO de piezas que conviven en el
// mismo espacio, cada una con su caja, su textura y su comportamiento.
//
// Asi el maguey capado son dos bloques -- el cuenco y el jugo de dentro -- y
// el ixtle puede llevar hierba o una flor creciendo entre sus hojas. Se
// seleccionan y se rompen POR SEPARADO: el rayo prueba la caja de cada pieza
// y gana la que atraviesa antes.
//
// ⭐ EL SISTEMA ADMITE N PIEZAS, no solo dos.
//
// La version anterior estaba cableada a exactamente 2 (un bool "segunda"), y
// eso ponia un techo artificial: para meter una tercera pieza habia que tocar
// el raycast, el mesher y la rotura. Ahora se pregunta por INDICE, asi que
// añadir una celda de tres o cuatro piezas es rellenar una fila de la tabla y
// nada mas.
//
// El contrato es simple:
//   piezasDe(t)      -> cuantas piezas hay (1 si no es compartida)
//   piezaN(t, i)     -> que bloque es la pieza i
//   cajaDePiezaN()   -> que espacio ocupa (en main.cpp, necesita geometria)

// Tope de piezas por celda. Subirlo no rompe nada: es solo el tamaño de los
// bucles que recorren las piezas.
constexpr int MAX_PIEZAS_CELDA = 4;

inline bool esCompartido(BlockType t) {
    return t == BLOCK_IXTLE_CON_HIERBA || t == BLOCK_IXTLE_CON_FLOR ||
           t == BLOCK_IXTLE_DOBLE ||
           t == BLOCK_AGUAMIEL ||
           // La copa del ocote chino: la rama va DENTRO del follaje, no en
           // una celda aparte. Asi el ramaje se ve entre las hojas sin gastar
           // el doble de bloques.
           t == BLOCK_LEAVES_OCOTE_CHINO_RAMA ||
           // Lo mismo en el ocote blanco: misma estructura, otra especie.
           t == BLOCK_LEAVES_OCOTE_RAMA;
}

// Cuantas piezas conviven en esta celda. Un bloque normal es "una pieza",
// de modo que el codigo que recorre piezas vale para todos sin casos aparte.
inline int piezasDe(BlockType t) {
    if (!esCompartido(t)) return 1;
    // De momento todas las compartidas son de dos. Cuando exista una de tres,
    // se devuelve 3 aqui y el resto del motor se adapta solo.
    return 2;
}

// La pieza numero `i` (0 = la principal). Devuelve BLOCK_AIR si no existe.
inline BlockType piezaN(BlockType t, int i) {
    if (i < 0 || i >= piezasDe(t)) return BLOCK_AIR;

    if (!esCompartido(t)) return t;

    if (i == 0) {
        // La PRINCIPAL: la que sostiene la celda.
        // El cajete de maguey es la pieza solida de su celda: es lo que queda
        // cuando el jugador se lleva el jugo.
        if (t == BLOCK_AGUAMIEL) return BLOCK_MAGUEY_HUECO;
        // En la copa del ocote chino manda la RAMA: es lo que sujeta el
        // follaje, igual que el cajete sujeta el jugo. Quitando las hojas
        // queda la rama; quitando la rama no queda nada de que colgar.
        if (t == BLOCK_LEAVES_OCOTE_CHINO_RAMA) return BLOCK_RAMA_OCOTE;
        if (t == BLOCK_LEAVES_OCOTE_RAMA)       return BLOCK_RAMA_OCOTE;
        return BLOCK_IXTLE_HOJA;
    }

    // Las ACOMPAÑANTES.
    switch (t) {
        case BLOCK_IXTLE_CON_HIERBA: return BLOCK_TALLGRASS;
        case BLOCK_IXTLE_CON_FLOR:   return BLOCK_ORANGE_FLOWER;
        case BLOCK_IXTLE_DOBLE:      return BLOCK_IXTLE_HOJA;
        // El JUGO: la pieza de dentro del cajete. Se "quita" recogiendolo con
        // un tazon, nunca rompiendolo.
        case BLOCK_AGUAMIEL:         return BLOCK_AGUAMIEL;
        // Las hojas que envuelven la rama del ocote chino.
        case BLOCK_LEAVES_OCOTE_CHINO_RAMA: return BLOCK_LEAVES_OCOTE_CHINO;
        case BLOCK_LEAVES_OCOTE_RAMA:       return BLOCK_LEAVES_OCOTE;
        default:                     return BLOCK_AIR;
    }
}

// --- Atajos de los dos primeros, que es lo que usa casi todo el motor ---
inline BlockType piezaPrimera(BlockType t) {
    return esCompartido(t) ? piezaN(t, 0) : t;
}
inline BlockType piezaSegunda(BlockType t) {
    return piezaN(t, 1);
}

// La combinación que resulta de juntar `encima` con lo que ya hay (`base`).
// Devuelve AIR si esa pareja no puede compartir espacio.
inline BlockType combinar(BlockType base, BlockType encima) {
    // ========================================================================
    // ⭐ LA ACICULA DEL OCOTE SOLO CONVIVE CON LA RAIZ
    // ========================================================================
    // El follaje del ocote ya comparte celda con su RAMA (las variantes
    // _RAMA de esCompartido), y esa es la unica pareja que le corresponde
    // dentro del arbol. Lo que se declara aqui es la regla para todo lo demas:
    //
    //   con una RAIZ  -> si. Es lo unico que puede atravesar el mismo espacio
    //                    sin contradecir a la planta: la raiz va por el suelo
    //                    y el follaje por encima, y ninguna de las dos llena
    //                    su voxel.
    //   con NADA MAS  -> no. Sin este corte, cualquier sprite que cayera en la
    //                    celda de una acicula podria fundirse con ella y el
    //                    mesher acabaria dibujando dos plantas en el mismo
    //                    hueco.
    //
    // Se resuelve ANTES que el ixtle porque es la regla mas especifica: la
    // acicula no es una hoja de lechuguilla y no debe heredar sus parejas.
    if (Acicula_esFollajeOcote(base) || Acicula_esFollajeOcote(encima)) {
        // La raiz es la unica compañia admitida, en cualquiera de los dos
        // ordenes (da igual quien llegue primero a la celda).
        const bool hayRaiz = esRaiz(base) || esRaiz(encima);
        const bool hayHoja = Acicula_esFollajeOcote(base) ||
                             Acicula_esFollajeOcote(encima);
        if (hayRaiz && hayHoja) {
            // Se conserva la HOJA como bloque de la celda: es lo que se ve, y
            // la raiz pasa por dentro sin cambiar su ID. Esto mantiene el
            // formato de guardado intacto -- no hace falta un ID nuevo para la
            // pareja, que es lo que obligaria a migrar mundos.
            return Acicula_esFollajeOcote(base) ? base : encima;
        }
        return BLOCK_AIR;   // con cualquier otra cosa, no comparten
    }

    // Solo se comparte con una hoja de ixtle: su roseta es abierta y deja
    // huecos entre hoja y hoja donde cabe otra planta.
    const bool baseIxtle = esIxtleHoja(base);
    if (!baseIxtle) return BLOCK_AIR;

    if (encima == BLOCK_TALLGRASS)     return BLOCK_IXTLE_CON_HIERBA;
    if (encima == BLOCK_ORANGE_FLOWER) return BLOCK_IXTLE_CON_FLOR;
    if (encima == BLOCK_IXTLE_HOJA)    return BLOCK_IXTLE_DOBLE;
    return BLOCK_AIR;
}

// Al quitar la pieza `i`, ¿que queda en la celda?
//
// La regla general: quitar una ACOMPAÑANTE deja la principal en pie; quitar
// la PRINCIPAL se lleva la celda entera, porque las demas se apoyaban en ella
// (la hierba crece entre las hojas del ixtle, el jugo vive dentro del cuenco).
inline BlockType quitarPiezaN(BlockType t, int i) {
    if (!esCompartido(t)) return BLOCK_AIR;
    if (i <= 0) return BLOCK_AIR;          // se va la principal: cae todo

    // El maguey capado: quitarle el JUGO deja el cuenco vacio, que es lo que
    // pasa al recogerlo con el tazon.
    if (t == BLOCK_AGUAMIEL) return BLOCK_MAGUEY_HUECO;

    // En una celda de dos, quitar la acompañante deja la principal sola.
    // (Con tres o mas habria que devolver la combinacion restante; hoy no
    // existe ninguna, y `combinar` es donde se declararia.)
    return piezaPrimera(t);
}

// Compatibilidad con el codigo que piensa en dos piezas.
inline BlockType quitarPieza(BlockType t, bool quitarSegunda) {
    return quitarPiezaN(t, quitarSegunda ? 1 : 0);
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

// ============================================================================
// ¿ES UNA PENCA DE MAGUEY SUELTA?
// ============================================================================
// Las hojas cortadas de las dos especies de agave: la del pulquero
// (BLOCK_IXTLE_HOJA) y la del tequilana (BLOCK_PENCA_AGAVE_AZUL).
//
// Se agrupan porque comparten una propiedad que el resto de items no tiene:
// son hojas CARNOSAS, no laminas. Por eso el render las dibuja mas gruesas
// (4 px de los 16, frente a los ~3 del resto), y por eso conviene una sola
// funcion que las nombre en vez de repetir la lista en cada sitio -- que es
// como se acaba con una especie que se ve distinta de la otra sin motivo.
//
// Las variantes de tamaño del ixtle (pequeña, grande, enorme) son bloques del
// mundo, no items sueltos, asi que no entran aqui.
inline bool esPencaDeMaguey(BlockType t) {
    return t == BLOCK_IXTLE_HOJA || t == BLOCK_PENCA_AGAVE_AZUL;
}

// ============================================================================
// ¿ES UN ITEM DE CACTUS TIRADO EN EL SUELO?
// ============================================================================
// Lo que el jugador recoge del nopal y de la biznaga: pencas, tunas, espinas y
// los productos de trabajar la penca (tiras, baba, seca).
//
// Se agrupan por una razon de RENDER, no de gameplay: son las piezas mas
// GRUESAS del inventario. Una penca de nopal es suculenta -- guarda agua -- y
// una tuna es un fruto redondo; las dos tienen bastante mas cuerpo que la hoja
// fibrosa de un agave, y muchisimo mas que una lamina de pedernal. Dibujarlas
// con el grosor de una hoja las aplanaria.
//
// ⚠️ SOLO LOS QUE SE SUELTAN COMO ITEM. Los bloques con los que el generador
// construye la planta (las bases del tallo, los cladodios apilados X2/X3, la
// biznaga viva) NO entran: esos se ven en el mundo, no en la mano, y el
// mesher ya tiene su propia geometria para ellos.
inline bool esCactusSuelto(BlockType t) {
    switch (t) {
        // --- La penca del nopal y lo que sale de trabajarla ---
        case BLOCK_NOPAL_CLADODIO:   // lo que cae al romper cualquier parte
        case BLOCK_NOPAL_MOJADO:     // limpia de espinas
        case BLOCK_NOPAL_SECO:       // curada al sol
        case BLOCK_NOPAL_TIRAS:      // cortada en tiras
        case BLOCK_NOPAL_SIN_BABA:   // desbabada
        case BLOCK_NOPAL_BABA:       // el mucilago
        case BLOCK_ESPINAS_NOPAL:    // lo que se le quita
        // --- Los frutos ---
        case BLOCK_NOPAL_FRUTO:
        case BLOCK_TUNA:
        case BLOCK_TUNA_AMARILLA:
        case BLOCK_TUNA_ROJA:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// ⭐ ¿ES UNA PENCA DE NOPAL, EN CUALQUIERA DE SUS ESTADOS?
// ============================================================================
// Las CINCO son la MISMA pieza en distintos momentos de su vida: el cladodio
// que se corta de la planta, y lo que queda tras lavarlo, secarlo, desbabarlo
// o cortarlo en tiras. Ninguna de esas operaciones lo parte en trozos: sigue
// siendo una penca entera.
//
// Por eso se agrupan, y por eso tienen que ir JUNTAS en todo lo que decide su
// TAMANO y su GROSOR al dibujarse. Si una se quedara fuera, lavar una penca la
// encogeria a la mitad delante del jugador -- un cambio de estado no puede
// cambiar lo que mide el objeto.
//
// ⚠️ NO ES LO MISMO QUE `esCactusSuelto`. Aquel incluye ademas la baba, las
// espinas y las tunas, que son piezas PEQUENAS: comparten el grosor carnoso
// pero no el tamano de una hoja.
inline bool esPencaDeNopal(BlockType t) {
    return t == BLOCK_NOPAL_CLADODIO || t == BLOCK_NOPAL_MOJADO ||
           t == BLOCK_NOPAL_SECO     || t == BLOCK_NOPAL_SIN_BABA ||
           t == BLOCK_NOPAL_TIRAS;
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
// EL CAJETE DEL MAGUEY: MEDIDAS DE LA FORMA 3D
// ============================================================================
// Un maguey capado no es un cubo macizo: es un CUENCO. Tiene suelo y cuatro
// paredes de maguey, y en medio un agujero cuadrado donde se junta el
// aguamiel -- el "cajete" que se raspa para sacar el jugo.
//
// Las medidas viven AQUI, en un solo sitio, porque las usan tres sistemas que
// tienen que coincidir al pixel:
//   - el MESHER, para dibujar el cuenco y el liquido
//   - la SELECCION (formaDeBloque), para que el contorno siga el borde real
//   - la COLISION, para que el jugador se apoye en el borde y no en el aire
//
// Si cada uno llevara sus numeros, bastaria tocar uno para que el contorno
// dejara de cuadrar con lo que se ve.
//
//     ┌──────────────┐ 1.0
//     │ ▓▓        ▓▓ │      <- paredes (3/16 de grosor)
//     │ ▓▓~~~~~~~~▓▓ │ 0.62 <- AGUAMIEL_ALTO: el liquido no rebosa
//     │ ▓▓▓▓▓▓▓▓▓▓▓▓ │ 0.25 <- CAJETE_SUELO: el fondo del cuenco
//     └──────────────┘ 0.0
//
constexpr float CAJETE_PARED   = 3.0f / 16.0f;   // grosor de las paredes
constexpr float CAJETE_SUELO   = 4.0f / 16.0f;   // altura del fondo
constexpr float AGUAMIEL_ALTO  = 10.0f / 16.0f;  // hasta donde sube el jugo

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

// ¿Es un tazon lleno de AGUAMIEL? Es otra cosa que el agua: se consigue de
// un maguey capado, no de un rio, y lleva a otras recetas.
inline bool esTazonConAguamiel(BlockType t) {
    return t == BLOCK_TAZON_PINO_AGUAMIEL ||
           t == BLOCK_TAZON_ENCINO_AGUAMIEL ||
           t == BLOCK_TAZON_OYAMEL_AGUAMIEL;
}

inline bool esTazon(BlockType t) {
    return esTazonVacio(t) || esTazonConAgua(t) || esTazonConAguamiel(t);
}

// ============================================================================
// LOS HUEVOS DE SPAWN
// ============================================================================
// Items de CREATIVO que sueltan un animal vivo en vez de colocar un bloque.
//
// Es una funcion y no una comparacion suelta porque en cuanto haya una
// segunda especie --y la habra: el proyecto documenta el colibri-- todos los
// sitios que hoy preguntan "es el huevo del pecari" tendrian que pasar a
// preguntar por dos, y siempre se olvida uno.
inline bool esHuevoDeSpawn(BlockType t) {
    return t == BLOCK_HUEVO_PECARI;
}

// El mismo tazon, lleno de AGUAMIEL. Devuelve BLOCK_AIR si no era un tazon
// vacio.
//
// ⭐ ACEPTA CUALQUIER TAZON, no solo el vacio: se pidio que "con cualquier
// tipo de tazon" se pueda recoger. Uno con agua se vacia y se llena de
// aguamiel -- el agua se tira, que es lo que haria cualquiera al ver que hay
// algo mejor que echar dentro.
inline BlockType tazonConAguamiel(BlockType cualquiera) {
    switch (cualquiera) {
        case BLOCK_TAZON_PINO:
        case BLOCK_TAZON_PINO_AGUA:
        case BLOCK_TAZON_PINO_AGUAMIEL:   return BLOCK_TAZON_PINO_AGUAMIEL;
        case BLOCK_TAZON_ENCINO:
        case BLOCK_TAZON_ENCINO_AGUA:
        case BLOCK_TAZON_ENCINO_AGUAMIEL: return BLOCK_TAZON_ENCINO_AGUAMIEL;
        case BLOCK_TAZON_OYAMEL:
        case BLOCK_TAZON_OYAMEL_AGUA:
        case BLOCK_TAZON_OYAMEL_AGUAMIEL: return BLOCK_TAZON_OYAMEL_AGUAMIEL;
        default:                          return BLOCK_AIR;
    }
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

// El mismo tazon, vacio. Devuelve BLOCK_AIR si no era un tazon con AGUA.
//
// ⚠️ SOLO AGUA, a proposito. Lo usa la receta del barro, que amasa tierra con
// agua: si aceptara tambien el aguamiel, se podria gastar el jugo del maguey
// -- mucho mas costoso de conseguir -- en hacer barro sin querer.
inline BlockType tazonVaciado(BlockType lleno) {
    switch (lleno) {
        case BLOCK_TAZON_PINO_AGUA:   return BLOCK_TAZON_PINO;
        case BLOCK_TAZON_ENCINO_AGUA: return BLOCK_TAZON_ENCINO;
        case BLOCK_TAZON_OYAMEL_AGUA: return BLOCK_TAZON_OYAMEL;
        default:                      return BLOCK_AIR;
    }
}

// El mismo tazon, vacio, VENGA DE DONDE VENGA (agua o aguamiel). Se usa
// cuando lo que importa es dejar el recipiente limpio, no que llevaba dentro.
inline BlockType tazonVaciadoTodo(BlockType lleno) {
    switch (lleno) {
        case BLOCK_TAZON_PINO_AGUA:
        case BLOCK_TAZON_PINO_AGUAMIEL:   return BLOCK_TAZON_PINO;
        case BLOCK_TAZON_ENCINO_AGUA:
        case BLOCK_TAZON_ENCINO_AGUAMIEL: return BLOCK_TAZON_ENCINO;
        case BLOCK_TAZON_OYAMEL_AGUA:
        case BLOCK_TAZON_OYAMEL_AGUAMIEL: return BLOCK_TAZON_OYAMEL;
        default:                          return BLOCK_AIR;
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
// ============================================================================
// LAS PIEZAS DEL ARBOL, POR NOMBRE
// ============================================================================
// La lista de las cuatro especies estaba repetida a mano por medio motor
// (soporte, densidad, hacha, pico, mesher...). Cada vez que se anadio una
// especie hubo que acordarse de TODAS esas copias, y alguna se quedo atras:
// el ocote falto en la lista de sprites de los tests durante toda su vida.
//
// Con estos dos predicados, anadir una especie es tocar un sitio.
inline bool esHojaDeArbol(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    // La celda de hoja+rama del ocote chino cuenta como hoja: por dentro
    // lleva ramaje, pero de cara al mundo (soporte, hacha, mesher) es follaje.
    return t == BLOCK_LEAVES || t == BLOCK_LEAVES_ENCINO ||
           t == BLOCK_LEAVES_OYAMEL || t == BLOCK_LEAVES_OCOTE ||
           t == BLOCK_LEAVES_OCOTE_CHINO ||
           t == BLOCK_LEAVES_OCOTE_CHINO_RAMA ||
           t == BLOCK_LEAVES_OCOTE_RAMA ||
           // Los cuatro pinos nuevos. Entran aqui y no en una lista aparte
           // porque este predicado es justamente el sitio unico del que habla
           // el comentario de arriba: soporte, hacha, densidad y mesher
           // preguntan todos por aqui.
           t == BLOCK_LEAVES_PATULA || t == BLOCK_LEAVES_PINONERO ||
           t == BLOCK_LEAVES_ALTURAS || t == BLOCK_LEAVES_PSEUDOSTROBUS;
}

// El tronco, incluido el corazon del ocote (que es el mismo tronco visto por
// dentro). Es lo que SOSTIENE una copa: las ramas no cuentan, porque una rama
// suelta sin tronco tampoco deberia sostener nada.
inline bool esTroncoDeArbol(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);
    return t == BLOCK_WOOD || t == BLOCK_WOOD_ENCINO ||
           t == BLOCK_WOOD_OYAMEL || t == BLOCK_WOOD_OCOTE ||
           t == BLOCK_WOOD_OCOTE_DENTRO ||
           t == BLOCK_WOOD_OCOTE_CHINO || t == BLOCK_WOOD_OCOTE_CHINO_DENTRO ||
           // Los cuatro pinos nuevos, cada uno con su corteza y su corte.
           // El corte (…_DENTRO) cuenta como tronco porque ES el mismo
           // tronco visto por dentro: sostiene la copa igual.
           t == BLOCK_WOOD_PATULA   || t == BLOCK_WOOD_PATULA_DENTRO ||
           t == BLOCK_WOOD_PINONERO || t == BLOCK_WOOD_PINONERO_DENTRO ||
           t == BLOCK_WOOD_ALTURAS  || t == BLOCK_WOOD_ALTURAS_DENTRO ||
           t == BLOCK_WOOD_PSEUDOSTROBUS ||
           t == BLOCK_WOOD_PSEUDOSTROBUS_DENTRO;
}

// Esta es la lista que manda en las DOS cosas del hacha: cuánta vida gasta el
// bloque y cuánto tarda en romperse. Tenerlas juntas evita que se separen —
// si mañana se añade una planta nueva, se añade aquí y las dos reglas la
// reconocen a la vez.
inline bool esOrganicoParaHacha(BlockType t) {
    if (esNivelParcial(t)) t = bloqueBaseDe(t);

    // --- El árbol: tronco, rama y raíz ---
    //
    // El tronco va por esTroncoDeArbol() en vez de repetir aqui la lista de
    // especies: es justo la duplicacion que dejo al ocote fuera de media
    // docena de sitios. Anadir una especie es tocar ese predicado y ya.
    if (esTroncoDeArbol(t)) return true;
    if (t == BLOCK_RAMA_PINO || t == BLOCK_RAMA_ENCINO ||
        t == BLOCK_RAMA_OYAMEL || t == BLOCK_RAMA_OCOTE)
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

    // --- Los BLOQUES COMPUESTOS son plantas ---
    //
    // BUG QUE ESTO CORRIGE: cortar un maguey del sistema nuevo NO gastaba el
    // hacha. Sin esta linea el compuesto no contaba como organico, asi que
    // desgasteHacha() devolvia 0 y la herramienta duraba eternamente talando
    // magueyes -- justo lo contrario de lo que cuesta cortar un agave hecho.
    //
    // Se comprueba por RANGO DE ID y no llamando a Compuesto::esCompuesto()
    // porque la dependencia va al reves: BloqueCompuesto.h incluye a este
    // archivo, no al contrario. El rango es una constante del formato de
    // guardado, asi que repetirla aqui es seguro -- pero tiene que coincidir
    // con COMPUESTO_BASE / COMPUESTO_FIN, y por eso hay un static_assert
    // alli que lo comprueba.
    //
    // Ese dia llego: el AGUA es compuesta y no es una planta. Se aparta
    // antes, porque cortar agua con el hacha no gasta el hacha ni tiene
    // sentido. El resto de familias (maguey, biznaga) si son vegetacion.
    if (esAguaVolumen(t)) return false;
    if ((int)t >= BLOQUE_COMPUESTO_BASE_ID) return true;

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
    if (esCeldaDeCapas(t)) t = materialArriba(t);

    if (t == BLOCK_AIR || t == BLOCK_WATER || t == BLOCK_LAVA) return false;

    // El agua con nivel tampoco, por lo mismo: es liquida. Sin esta linea
    // caeria hasta el `return true` del final -- es justo el agujero del que
    // avisa el comentario de mas abajo -- y el pico se gastaria picando agua.
    if (esAguaVolumen(t)) return false;

    // La tierra empapada se pica igual que la seca: sigue siendo tierra, y
    // la tierra no es del pico.
    if (estaMojado(t)) t = versionSeca(t);

    // Lo orgánico es del hacha, no del pico.
    if (esOrganicoParaHacha(t)) return false;

    // Las plantas y la hojarasca tampoco: no son roca.
    if (t == BLOCK_TALLGRASS || t == BLOCK_LEAVES ||
        t == BLOCK_LEAVES_ENCINO || t == BLOCK_LEAVES_OYAMEL ||
        t == BLOCK_LEAVES_OCOTE)
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
        t == BLOCK_ESPINAS_NOPAL ||
        // El abisaii, por lo mismo: sus dos piezas son hoja carnosa y espina,
        // no roca. Se excluyen AQUI y no en otro sitio para que la lista de
        // "esto es planta" siga siendo una sola.
        t == BLOCK_AGAVE_ABISAII_PENCA || t == BLOCK_AGAVE_ABISAII_PUNTA)
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
//
// ⭐ SE QUEDO DESFASADO UNA VEZ, Y COSTO DOS BUGS.
//
// Al añadir el agave tequilana azul (5 bloques al final) esto siguio apuntando
// al tazon, asi que el tope se quedo 10 IDs corto. Consecuencias reales:
//
//   1. prewarmItemTextures() recorre `0..BLOCK_TYPE_MAX`, asi que las piezas
//      del agave NO se registraban como icono de item: en la mano y en el
//      inventario salian sin textura.
//   2. Es el tope con el que se validan los IDs que llegan de disco. Un bloque
//      por encima del tope es "fuera de rango" -- justo lo que este valor
//      existe para detectar.
//
// Ninguno de los dos avisa: el primero se ve como un hueco y el segundo no se
// ve hasta que alguien recarga un mundo. Por eso hay un test que ata este
// valor al ultimo del enum (test_agave_azul.cpp), para que la proxima vez que
// se olvide falle el build de tests en vez de salir a produccion.
constexpr int BLOCK_TYPE_MAX = BLOCK_TECHO_SUELO_L7;

// ============================================================================
//  CARAS OCULTAS Y VISION DEL INTERIOR
// ============================================================================
//
// QUE RESUELVE
// Un bloque puede declarar que una o varias de sus caras NO se dibujan. Por el
// hueco que dejan se ve el INTERIOR del bloque: las otras cinco caras, vistas
// desde dentro.
//
// Sin esto, mirar por el hueco muestra el vacio. El mundo se dibuja con
// GL_CULL_FACE(GL_BACK), asi que OpenGL descarta toda cara cuyo recorrido se
// vea horario desde el ojo -- y desde dentro de un bloque, las cinco caras
// restantes se ven exactamente asi. El bloque queda hueco y transparente.
//
// COMO SE RESUELVE
// Las caras de un bloque hueco se emiten DOS veces: la normal, que se ve desde
// fuera, y su gemela con los vertices en orden inverso, que se ve desde
// dentro. No se toca el estado de OpenGL: el resto del mundo sigue con su
// culling intacto y no hay que separar el lote de dibujo.
//
// El coste es de hasta 5 quads extra por bloque hueco, y solo por los bloques
// que declaran alguna cara oculta -- el resto del mundo no paga nada.
//
// ORIENTACION DE LAS CARAS (indice `dir` del mesher, ver DIR_VEC)
//   0 = +Y (arriba)    1 = -Y (abajo)
//   2 = +Z (norte)     3 = -Z (sur)
//   4 = +X (este)      5 = -X (oeste)

/// Mascara de caras ocultas: bit `dir` a 1 = esa cara no se dibuja.
using CaraMask = unsigned;

constexpr CaraMask CARA_NINGUNA = 0u;
constexpr CaraMask CARA_ARRIBA  = 1u << 0;   // +Y
constexpr CaraMask CARA_ABAJO   = 1u << 1;   // -Y
constexpr CaraMask CARA_NORTE   = 1u << 2;   // +Z
constexpr CaraMask CARA_SUR     = 1u << 3;   // -Z
constexpr CaraMask CARA_ESTE    = 1u << 4;   // +X
constexpr CaraMask CARA_OESTE   = 1u << 5;   // -X

/// Que caras oculta este tipo de bloque.
///
/// Es la UNICA fuente de verdad: el mesher no vuelve a codificar casos
/// especiales por tipo de bloque. Anadir un bloque hueco es anadir una linea
/// aqui, sin tocar el mesher.
inline CaraMask carasOcultas(BlockType type) {
    switch (type) {
    // Copas de ocote: no tienen suelo. Mirando hacia arriba desde el pie del
    // arbol se ve el interior de la copa -- el ramaje entre el follaje -- en
    // vez de un techo liso.
    case BLOCK_LEAVES_OCOTE_CHINO:
    case BLOCK_LEAVES_OCOTE_CHINO_RAMA:
    case BLOCK_LEAVES_OCOTE:
    case BLOCK_LEAVES_OCOTE_RAMA:
        return CARA_ABAJO;

    default:
        return CARA_NINGUNA;
    }
}

/// true si `dir` esta oculta en este bloque.
inline bool caraEstaOculta(BlockType type, int dir) {
    if (dir < 0 || dir > 5) return false;
    return (carasOcultas(type) & (1u << dir)) != 0u;
}

/// true si el bloque deja ver su interior por algun hueco, y por tanto sus
/// caras necesitan gemela interior.
inline bool muestraInterior(BlockType type) {
    return carasOcultas(type) != CARA_NINGUNA;
}
