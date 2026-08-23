#include <doctest/doctest.h>
#include "terrain/DecorationSystem.h"
#include "BlockType.h"
#include <cstdio>
#include <map>

// ============================================================================
// ABUNDANCIA DE MINERALES
// ============================================================================
// Mide que fraccion de la roca se convierte en cada mineral. No comprueba un
// numero magico: comprueba el ORDEN y unos rangos razonables, que es lo que
// de verdad importa. Si alguien toca un umbral "para ajustar", esto avisa.

struct Censo {
    std::map<int, long long> porMineral;
    long long total = 0;

    // operator[] no existe en un map const: este accesor devuelve 0 si el
    // mineral no salio ni una vez, que es justo lo que se quiere medir.
    long long de(BlockType b) const {
        auto it = porMineral.find((int)b);
        return (it == porMineral.end()) ? 0 : it->second;
    }
};

static Censo censarMinerales(int seed) {
    TerrainGen::DecorationSystem deco(seed);
    Censo c;
    // Una loncha del subsuelo: 64x64 columnas por todo el rango util.
    for (int x = 0; x < 64; ++x)
        for (int z = 0; z < 64; ++z)
            for (int y = 5; y < 118; ++y) {
                ++c.total;
                // ⚠️ Blocks::Id, NO uint8_t: los IDs pasan de 255 y en un
                // byte se truncan. La primera version de este test usaba
                // uint8_t y daba pirita 0,00% -- el fallo estaba aqui, no
                // en el generador.
                const TerrainGen::Blocks::Id m = deco.GetOre(x * 3, y, z * 3);
                if (m != TerrainGen::Blocks::AIR) c.porMineral[(int)m]++;
            }
    return c;
}

TEST_CASE("Minerales: carbon, pirita y desecho son SUPER comunes") {
    const Censo c = censarMinerales(4242);

    const double carbon  = (double)c.de(BLOCK_COAL_ORE)    / c.total;
    const double pirita  = (double)c.de(BLOCK_PYRITE_ORE)  / c.total;
    const double desecho = (double)c.de(BLOCK_SCRAP_METAL) / c.total;

    printf("[MINERALES] carbon=%.2f%% pirita=%.2f%% desecho=%.2f%%\n",
           carbon * 100, pirita * 100, desecho * 100);

    // Cada uno tiene que ocupar una parte NOTABLE de la roca. Con los
    // umbrales viejos el carbon rondaba el 1%; ahora se busca mucho mas.
    CHECK(carbon  > 0.03);
    CHECK(pirita  > 0.03);
    CHECK(desecho > 0.03);

    // Pero sin pasarse: si un solo mineral se comiera media montana, ya no
    // seria roca con vetas, seria una montana de carbon.
    CHECK(carbon  < 0.35);
    CHECK(pirita  < 0.35);
    CHECK(desecho < 0.35);
}

TEST_CASE("Minerales: los raros siguen siendo raros") {
    const Censo c = censarMinerales(4242);

    const double diamante = (double)c.de(BLOCK_DIAMOND_ORE) / c.total;
    const double oro      = (double)c.de(BLOCK_GOLD_ORE)    / c.total;
    const double carbon   = (double)c.de(BLOCK_COAL_ORE)    / c.total;

    printf("[MINERALES] diamante=%.3f%% oro=%.3f%%\n",
           diamante * 100, oro * 100);

    // Lo importante no es el numero exacto, es la RELACION: encontrar
    // diamante tiene que seguir significando algo.
    CHECK(diamante < carbon);
    CHECK(oro < carbon);
    CHECK(diamante < 0.02);
}

TEST_CASE("Minerales: el mismo seed da siempre la misma veta") {
    TerrainGen::DecorationSystem a(777), b(777);
    for (int i = 0; i < 500; ++i) {
        const int x = i * 7 % 300, y = 6 + (i % 100), z = i * 13 % 300;
        CHECK(a.GetOre(x, y, z) == b.GetOre(x, y, z));
    }
}

TEST_CASE("Minerales: los comunes NO tapan a los raros donde se solapan") {
    // El bucle recorre la tabla de atras adelante y gana el PRIMERO que
    // acierta, que son los raros. Sin ese orden, una veta de carbon -- que
    // ahora ocupa casi el 5% de toda la roca -- se tragaria el oro y la
    // plata que hubiera dentro.
    //
    // Se comprueba con la PLATA y no con el diamante: el diamante tiene el
    // umbral tan alto (0.86, y el ruido apenas llega a 0.88) que es casi
    // inalcanzable de por si -- eso viene de antes y no es cosa del orden.
    TerrainGen::DecorationSystem deco(31337);

    bool hayPlata = false;
    for (int x = 0; x < 90 && !hayPlata; ++x)
        for (int z = 0; z < 90 && !hayPlata; ++z)
            for (int y = 5; y <= 45; ++y)
                if (deco.GetOre(x * 2, y, z * 2) == TerrainGen::Blocks::SILVER_ORE) {
                    hayPlata = true; break;
                }

    CHECK(hayPlata);
}

TEST_CASE("Guijarros: los tres hierros y la nieve son guijarros de verdad") {
    // Al entrar en esGuijarro heredan la forma de canto, la caja baja y que
    // no ahoguen al pasto. Si se olvidara, se dibujarian como cubos enteros.
    CHECK(esGuijarro(BLOCK_PEDAZO_GOETHITA));
    CHECK(esGuijarro(BLOCK_PEDAZO_HEMATITE));
    CHECK(esGuijarro(BLOCK_PEDAZO_LIMONITA));
    CHECK(esGuijarro(BLOCK_PEDAZO_NIEVE));

    // La pirita NO: esa va en veta, es un bloque macizo.
    CHECK_FALSE(esGuijarro(BLOCK_PYRITE_ORE));
}

TEST_CASE("Minerales: los IDs nuevos entran en el rango que se valida") {
    CHECK((int)BLOCK_PYRITE_ORE <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_NIEVE <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_GOETHITA <= BLOCK_TYPE_MAX);

    // ⚠️ Blocks::Id es de 16 bits justamente porque el enum paso de 255.
    // Si alguien lo devolviera a uint8_t, la pirita (262) se truncaria a 6 y
    // su veta desapareceria del mundo en silencio. Esto lo vigila.
    CHECK(sizeof(TerrainGen::Blocks::Id) >= 2);
    CHECK((int)BLOCK_TYPE_MAX < 65536);
}
