#include <doctest/doctest.h>
#include "Inventory.h"

// ============================================================================
// EL HACHA DE PEDERNAL Y EL MAGUEY MADURO
// ============================================================================

TEST_CASE("Hacha de pedernal: aguanta 400 bloques, mas que la de piedra") {
    CHECK(HACHA_PEDERNAL_BLOQUES == 400);
    CHECK(HACHA_PEDERNAL_MEDIOS == 800);
    CHECK(vidaMaximaHerramienta(BLOCK_HACHA_PEDERNAL) == 800);

    // Es mejor que la de piedra, que aguanta 250.
    CHECK(vidaMaximaHerramienta(BLOCK_HACHA_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_HACHA_PIEDRA));
}

TEST_CASE("Hacha de pedernal: es un hacha y corta lo mismo que la de piedra") {
    CHECK(esHacha(BLOCK_HACHA_PEDERNAL));
    CHECK(esHacha(BLOCK_HACHA_PIEDRA));
    CHECK_FALSE(esHacha(BLOCK_PICO_PIEDRA));
    CHECK_FALSE(esHacha(BLOCK_MARTILLO_PIEDRA));

    // Lo organico le gasta vida igual.
    CHECK(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_WOOD) == 2);
    CHECK(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_IXTLE_HOJA) == 2);
    // Y la roca no, igual que la de piedra.
    CHECK(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_STONE) == 0);
}

TEST_CASE("Maguey: solo el hacha de pedernal puede con la punta") {
    // La de pedernal si.
    CHECK(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_MAGUEY_PUNTA) == 2);

    // Las demas no: ni la de piedra, ni el pico, ni el martillo.
    CHECK(desgasteHerramienta(BLOCK_HACHA_PIEDRA, BLOCK_MAGUEY_PUNTA) == 0);
    CHECK(desgasteHerramienta(BLOCK_PICO_PIEDRA, BLOCK_MAGUEY_PUNTA) == 0);
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PIEDRA, BLOCK_MAGUEY_PUNTA) == 0);
}

TEST_CASE("Hacha de pedernal: se rompe a los 400 bloques exactos") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PEDERNAL, 1);

    for (int i = 0; i < 399; ++i) {
        CHECK_FALSE(inv.gastarHerramienta(
            desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_WOOD)));
    }
    CHECK(inv.gastarHerramienta(
        desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_WOOD)));
    CHECK(inv.at(0).isEmpty());
}

TEST_CASE("Hacha de pedernal: lleva barrita con su propia escala") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PEDERNAL, 1);

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(1.0f));

    // Media vida son 200 bloques, no 125 como el hacha de piedra.
    for (int i = 0; i < 200; ++i)
        inv.gastarHerramienta(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_WOOD));

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(0.5f));
}

TEST_CASE("Items nuevos: no son herramientas que se gasten") {
    // Estos se acumulan como cualquier item: no llevan barra de vida.
    CHECK_FALSE(esHerramientaGastable(BLOCK_ESPINAS_NOPAL));
    CHECK_FALSE(esHerramientaGastable(BLOCK_NOPAL_SECO));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_PINO));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_ENCINO));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_OYAMEL));
    CHECK_FALSE(esHerramientaGastable(BLOCK_AGUAMIEL));

    // Y el hacha de pedernal si.
    CHECK(esHerramientaGastable(BLOCK_HACHA_PEDERNAL));
}

TEST_CASE("Bloques nuevos: caben en el rango valido del enum") {
    // BLOCK_TYPE_MAX valida lo que se lee de disco: si se queda corto, un
    // maguey guardado se rechazaria al cargar el mundo.
    CHECK((int)BLOCK_AGUAMIEL <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_MAGUEY_PUNTA <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_HACHA_PEDERNAL <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_TAZON_OYAMEL <= BLOCK_TYPE_MAX);
}
