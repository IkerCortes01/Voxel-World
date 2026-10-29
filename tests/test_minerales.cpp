#include <doctest/doctest.h>
#include "terrain/DecorationSystem.h"
#include "BlockType.h"
#include "BlockCompat.h"

// ============================================================================
// LOS MINERALES DE VETA SE RETIRARON DEL JUEGO
// ============================================================================
// Este fichero medía la abundancia de las siete vetas (carbón, pirita,
// desecho, hierro, oro, plata y diamante): qué fracción de la roca se
// convertía en cada una, que el orden común→raro se respetara, y que los
// comunes no taparan a los raros donde se solapaban.
//
// Los siete bloques ya no existen y `GetOre()` se fue con ellos, así que esos
// cinco casos se han retirado: no queda nada que censar.
//
// Lo que SÍ sigue vivo y se comprueba aquí:
//
//   - Los GUIJARROS del suelo (goethita, hematite, limonita, nieve). Son
//     items que se recogen agachándose, nunca fueron bloques de veta y no
//     pasaban por GetOre().
//   - Que el subsuelo se genere como roca limpia.
//   - Que los IDs retirados se conviertan en piedra al cargar un mundo viejo.
//   - El ancho de Blocks::Id, que es un invariante del formato y no de los
//     minerales.
// ============================================================================

TEST_CASE("Guijarros: los tres hierros y la nieve son guijarros de verdad") {
    // Al entrar en esGuijarro heredan la forma de canto, la caja baja y que
    // no ahoguen al pasto. Si se olvidara, se dibujarian como cubos enteros.
    //
    // Estos cuatro son la razon de que retirar los minerales de VETA no deje
    // al jugador sin hierro: siguen sembrandose por el suelo con su propio
    // reparto por bioma.
    CHECK(esGuijarro(BLOCK_PEDAZO_GOETHITA));
    CHECK(esGuijarro(BLOCK_PEDAZO_HEMATITE));
    CHECK(esGuijarro(BLOCK_PEDAZO_LIMONITA));
    CHECK(esGuijarro(BLOCK_PEDAZO_NIEVE));
}

TEST_CASE("Minerales retirados: el subsuelo se genera como roca limpia") {
    // ⭐ EL CASO QUE FIJA LA RETIRADA.
    //
    // Antes, DecorationSystem::GetOre() decidia por ruido 3D que voxeles de
    // roca profunda se convertian en mineral, y ChunkGenerator la llamaba para
    // cada celda por debajo del subsuelo. La funcion ya no existe.
    //
    // Este caso no puede llamar a lo que se borro, asi que comprueba lo otro:
    // que ninguno de los siete IDs retirados sobreviva a la carga. Si alguien
    // volviera a sembrarlos, la migracion los barreria igualmente y el terreno
    // seguiria siendo roca -- que es la garantia que de verdad importa.
    const BlockType RETIRADOS[] = {
        BLOCK_RESERVADO_MINERAL_08,   // carbon
        BLOCK_RESERVADO_MINERAL_09,   // desecho de metales
        BLOCK_RESERVADO_MINERAL_20,   // plata
        BLOCK_RESERVADO_MINERAL_21,   // diamante
        BLOCK_RESERVADO_MINERAL_26,   // oro
        BLOCK_RESERVADO_MINERAL_164,  // pirita
        BLOCK_IRON_ORE,               // hierro (nunca se implemento)
    };
    for (BlockType t : RETIRADOS) {
        INFO("ID retirado ", (int)t);
        CHECK(BlockCompat::mineralAPiedra(t) == BLOCK_STONE);
        CHECK(BlockCompat::esMineralRetirado(t));
    }
}

TEST_CASE("Minerales retirados: las lonchas conservan su altura") {
    // Un mineral podia estar a media altura. Convertir esas lonchas en un
    // bloque ENTERO de piedra levantaria el suelo de los mundos guardados: se
    // anda por encima, asi que la altura es colision, no decoracion.
    //
    // Se comprueban los siete niveles de una familia y el primero de las demas.
    for (int n = 0; n < 7; ++n) {
        INFO("nivel L", n + 1);
        CHECK(BlockCompat::mineralAPiedra((BlockType)((int)BLOCK_COAL_L1 + n))
              == (BlockType)((int)BLOCK_STONE_L1 + n));
    }
    CHECK(BlockCompat::mineralAPiedra(BLOCK_SILVER_L1)  == BLOCK_STONE_L1);
    CHECK(BlockCompat::mineralAPiedra(BLOCK_GOLD_L1)    == BLOCK_STONE_L1);
    CHECK(BlockCompat::mineralAPiedra(BLOCK_DIAMOND_L1) == BLOCK_STONE_L1);
    CHECK(BlockCompat::mineralAPiedra(BLOCK_SCRAP_L1)   == BLOCK_STONE_L1);
}

TEST_CASE("Minerales retirados: no se lleva por delante a los demas bloques") {
    // La otra mitad del contrato: la migracion tiene que tocar SOLO lo
    // retirado. Si `mineralAPiedra` se pasara de rango, convertiria en piedra
    // bloques sanos de todos los mundos guardados -- un destrozo silencioso y
    // sin vuelta atras.
    const BlockType INTOCABLES[] = {
        BLOCK_AIR, BLOCK_GRASS, BLOCK_DIRT, BLOCK_STONE, BLOCK_SAND,
        BLOCK_WOOD, BLOCK_LEAVES, BLOCK_WATER, BLOCK_LAVA, BLOCK_COBBLESTONE,
        BLOCK_LIMESTONE, BLOCK_CLAY, BLOCK_GRAVEL, BLOCK_SNOW,
        // Los guijarros y los items de metal, que es lo que se conserva.
        BLOCK_PEDAZO_GOETHITA, BLOCK_PEDAZO_HEMATITE, BLOCK_PEDAZO_LIMONITA,
        BLOCK_PEDAZO_NIEVE, BLOCK_COAL_ITEM, BLOCK_RAW_ZINC, BLOCK_RAW_COPPER,
        // Y las lonchas de piedra, que comparten forma con las de mineral.
        BLOCK_STONE_L1, BLOCK_STONE_L7, BLOCK_DIRT_L1,
    };
    for (BlockType t : INTOCABLES) {
        INFO("bloque ", (int)t);
        CHECK(BlockCompat::mineralAPiedra(t) == t);
        CHECK_FALSE(BlockCompat::esMineralRetirado(t));
    }
}

TEST_CASE("Minerales: los IDs nuevos entran en el rango que se valida") {
    CHECK((int)BLOCK_PEDAZO_NIEVE <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_GOETHITA <= BLOCK_TYPE_MAX);

    // ⚠️ Blocks::Id es de 16 bits justamente porque el enum paso de 255.
    // Si alguien lo devolviera a uint8_t, todo ID por encima de 255 se
    // truncaria y el bloque cambiaria por otro en silencio. Esto lo vigila.
    CHECK(sizeof(TerrainGen::Blocks::Id) >= 2);
    CHECK((int)BLOCK_TYPE_MAX < 65536);
}
