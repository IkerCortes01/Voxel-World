#include <doctest/doctest.h>
#include "Inventory.h"

// ============================================================================
// TESTS DEL HACHA DE PIEDRA
// ============================================================================
// La regla que fijan: el hacha aguanta 250 BLOQUES ORGÁNICOS exactos, gastando
// 1 de vida por bloque. Lo que no es orgánico no le gasta vida (el castigo por
// usarla mal es el tiempo, 13 minutos por bloque, no perder la herramienta).
//
// Estos números son de diseño, no de implementación: si alguien cambia el
// coste "para ajustar el balance" y con eso el hacha deja de durar 250, esto
// lo detecta. La cuenta interna va en medios puntos por compatibilidad con
// las partidas guardadas, y eso también queda fijado aquí.

TEST_CASE("Hacha: la vida anunciada son 250 bloques") {
    CHECK(HACHA_VIDA_BLOQUES == 250);
    // La cuenta interna va en medios puntos: 250 bloques x 2.
    CHECK(HACHA_VIDA_MEDIOS == 500);
}

TEST_CASE("Hacha: lo organico gasta 1 punto (2 medios)") {
    // El arbol entero
    CHECK(desgasteHacha(BLOCK_WOOD) == 2);
    CHECK(desgasteHacha(BLOCK_WOOD_ENCINO) == 2);
    CHECK(desgasteHacha(BLOCK_WOOD_OYAMEL) == 2);
    CHECK(desgasteHacha(BLOCK_RAMA_PINO) == 2);
    CHECK(desgasteHacha(BLOCK_RAMA_ENCINO) == 2);
    CHECK(desgasteHacha(BLOCK_RAMA_OYAMEL) == 2);
    CHECK(desgasteHacha(BLOCK_RAIZ_PEQUENA) == 2);
    CHECK(desgasteHacha(BLOCK_RAIZ_ENORME) == 2);

    // El nopal y sus partes
    CHECK(desgasteHacha(BLOCK_NOPAL_CLADODIO) == 2);
    CHECK(desgasteHacha(BLOCK_NOPAL_FRUTO) == 2);
    CHECK(desgasteHacha(BLOCK_NOPAL_TALLO) == 2);

    // El maguey / ixtle
    CHECK(desgasteHacha(BLOCK_IXTLE_HOJA) == 2);
}

TEST_CASE("Hacha: lo que no es organico no le gasta vida") {
    // Solidos y en polvo: el hacha no sirve, pero tampoco se destroza.
    CHECK(desgasteHacha(BLOCK_DIRT) == 0);
    CHECK(desgasteHacha(BLOCK_GRASS) == 0);
    CHECK(desgasteHacha(BLOCK_SAND) == 0);
    CHECK(desgasteHacha(BLOCK_GRAVEL) == 0);
    CHECK(desgasteHacha(BLOCK_STONE) == 0);
    CHECK(desgasteHacha(BLOCK_COBBLESTONE) == 0);
    CHECK(desgasteHacha(BLOCK_COAL_ORE) == 0);
}

TEST_CASE("Hacha: esOrganicoParaHacha reconoce la capa parcial de un organico") {
    // Un nivel parcial de tronco sigue siendo tronco: se normaliza al bloque
    // base antes de decidir. Si no, media capa de madera se trataria como
    // piedra y tardaria 13 minutos.
    const BlockType medioTronco = conNivel(BLOCK_WOOD, 4);
    if (medioTronco != BLOCK_WOOD) {   // solo si la madera admite niveles
        CHECK(esOrganicoParaHacha(medioTronco));
        CHECK(desgasteHacha(medioTronco) == 2);
    }
}

TEST_CASE("Hacha: aguanta exactamente 250 bloques organicos y se rompe") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PIEDRA, 1);

    // Los primeros 249 no la rompen.
    for (int i = 0; i < 249; ++i) {
        const bool rota = inv.gastarHerramienta(desgasteHacha(BLOCK_WOOD));
        CHECK_FALSE(rota);
        CHECK(inv.at(0).blockType == BLOCK_HACHA_PIEDRA);
    }

    // El bloque 250 la rompe y vacia el slot.
    CHECK(inv.gastarHerramienta(desgasteHacha(BLOCK_WOOD)));
    CHECK(inv.at(0).isEmpty());
}

TEST_CASE("Hacha: romper tierra no le quita vida") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PIEDRA, 1);

    // Estrenarla con un bloque organico para que tenga vida asignada.
    inv.gastarHerramienta(desgasteHacha(BLOCK_WOOD));
    const float tras1 = inv.vidaHerramienta();

    // Mil bloques de tierra no deberian moverle la vida ni romperla.
    for (int i = 0; i < 1000; ++i) {
        CHECK_FALSE(inv.gastarHerramienta(desgasteHacha(BLOCK_DIRT)));
    }
    CHECK(inv.vidaHerramienta() == doctest::Approx(tras1));
    CHECK(inv.at(0).blockType == BLOCK_HACHA_PIEDRA);
}

TEST_CASE("Hacha: sin hacha en la mano, gastarHerramienta no hace nada") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_STONE, 10);

    CHECK_FALSE(inv.gastarHerramienta(2));
    CHECK(inv.at(0).blockType == BLOCK_STONE);
    CHECK(inv.at(0).count == 10);
    CHECK(inv.vidaHerramienta() == -1.0f);   // -1 = no hay hacha
}
