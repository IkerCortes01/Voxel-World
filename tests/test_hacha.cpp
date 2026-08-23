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

// ============================================================================
// EL PICO Y EL MARTILLO
// ============================================================================

TEST_CASE("Herramientas: cada una aguanta lo suyo") {
    CHECK(HACHA_VIDA_BLOQUES == 250);
    CHECK(PICO_VIDA_BLOQUES == 340);
    CHECK(MARTILLO_VIDA_USOS == 150);

    CHECK(vidaMaximaHerramienta(BLOCK_HACHA_PIEDRA) == 500);
    CHECK(vidaMaximaHerramienta(BLOCK_PICO_PIEDRA) == 680);
    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PIEDRA) == 300);

    // Lo que no es herramienta no tiene vida.
    CHECK(vidaMaximaHerramienta(BLOCK_STONE) == 0);
    CHECK(vidaMaximaHerramienta(BLOCK_STICK) == 0);
}

TEST_CASE("Pico: la roca le gasta vida, lo organico no") {
    CHECK(desgastePico(BLOCK_STONE) == 2);
    CHECK(desgastePico(BLOCK_COBBLESTONE) == 2);
    CHECK(desgastePico(BLOCK_LIMESTONE) == 2);
    CHECK(desgastePico(BLOCK_GRAVEL) == 2);
    CHECK(desgastePico(BLOCK_COAL_ORE) == 2);
    CHECK(desgastePico(BLOCK_DIRT) == 2);

    // Lo organico es del hacha: el pico no lo reconoce como suyo.
    CHECK(desgastePico(BLOCK_WOOD) == 0);
    CHECK(desgastePico(BLOCK_NOPAL_CLADODIO) == 0);
    CHECK(desgastePico(BLOCK_IXTLE_HOJA) == 0);
}

TEST_CASE("Herramientas: cada una solo gasta con lo suyo") {
    // El hacha con madera si, con piedra no.
    CHECK(desgasteHerramienta(BLOCK_HACHA_PIEDRA, BLOCK_WOOD) == 2);
    CHECK(desgasteHerramienta(BLOCK_HACHA_PIEDRA, BLOCK_STONE) == 0);

    // El pico al reves.
    CHECK(desgasteHerramienta(BLOCK_PICO_PIEDRA, BLOCK_STONE) == 2);
    CHECK(desgasteHerramienta(BLOCK_PICO_PIEDRA, BLOCK_WOOD) == 0);

    // El martillo no pica NADA: se gasta al craftear.
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PIEDRA, BLOCK_STONE) == 0);
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PIEDRA, BLOCK_WOOD) == 0);
}

TEST_CASE("Pico: aguanta exactamente 340 bloques de roca") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_PICO_PIEDRA, 1);

    for (int i = 0; i < 339; ++i) {
        CHECK_FALSE(inv.gastarHerramienta(desgastePico(BLOCK_STONE)));
    }
    CHECK(inv.gastarHerramienta(desgastePico(BLOCK_STONE)));
    CHECK(inv.at(0).isEmpty());
}

TEST_CASE("Pico: picar madera no le quita vida") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_PICO_PIEDRA, 1);

    inv.gastarHerramienta(desgastePico(BLOCK_STONE));   // estrenarlo
    const float tras1 = inv.vidaHerramienta();

    for (int i = 0; i < 500; ++i)
        CHECK_FALSE(inv.gastarHerramienta(desgastePico(BLOCK_WOOD)));

    CHECK(inv.vidaHerramienta() == doctest::Approx(tras1));
    CHECK(inv.at(0).blockType == BLOCK_PICO_PIEDRA);
}

TEST_CASE("Herramientas: las tres llevan barra, con su propia escala") {
    Inventory inv;
    inv.at(0).add(BLOCK_HACHA_PIEDRA, 1);
    inv.at(1).add(BLOCK_PICO_PIEDRA, 1);
    inv.at(2).add(BLOCK_MARTILLO_PIEDRA, 1);
    inv.at(3).add(BLOCK_PEDERNAL_AFILADO, 5);

    // Recien fabricadas: barra llena.
    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(1.0f));
    CHECK(inv.vidaFraccionSlot(1) == doctest::Approx(1.0f));
    CHECK(inv.vidaFraccionSlot(2) == doctest::Approx(1.0f));

    // El pedernal afilado NO es herramienta: no lleva barra.
    CHECK(inv.vidaFraccionSlot(3) == -1.0f);

    // La escala es la de CADA una: media vida del martillo son 75 usos,
    // media del pico son 170 bloques.
    inv.selectedSlot = 2;
    for (int i = 0; i < 75; ++i)
        inv.gastarHerramienta(2);
    CHECK(inv.vidaFraccionSlot(2) == doctest::Approx(0.5f));

    inv.selectedSlot = 1;
    for (int i = 0; i < 170; ++i)
        inv.gastarHerramienta(desgastePico(BLOCK_STONE));
    CHECK(inv.vidaFraccionSlot(1) == doctest::Approx(0.5f));
}

// ============================================================================
// LA BARRITA DE VIDA
// ============================================================================

TEST_CASE("Barra: un hacha recien fabricada sale con la barra llena") {
    Inventory inv;
    inv.at(0).add(BLOCK_HACHA_PIEDRA, 1);
    // vidaMedios = 0 significa "sin estrenar", y debe leerse como llena.
    CHECK(inv.at(0).vidaMedios == 0);
    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(1.0f));
}

TEST_CASE("Barra: baja a la mitad tras gastar la mitad de la vida") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PIEDRA, 1);

    for (int i = 0; i < 125; ++i)      // 125 de 250 bloques
        inv.gastarHerramienta(desgasteHacha(BLOCK_WOOD));

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(0.5f));
}

TEST_CASE("Barra: los slots que no son herramienta no llevan barra") {
    Inventory inv;
    inv.at(0).add(BLOCK_STONE, 10);
    CHECK(inv.vidaFraccionSlot(0) == -1.0f);   // -1 = no dibujar

    // Un slot vacio tampoco.
    CHECK(inv.vidaFraccionSlot(1) == -1.0f);
}

TEST_CASE("Barra: indice fuera de rango no revienta") {
    Inventory inv;
    CHECK(inv.vidaFraccionSlot(-1) == -1.0f);
    CHECK(inv.vidaFraccionSlot(99999) == -1.0f);
}

// ============================================================================
// LA MADERA YA NO TIENE NIVELES
// ============================================================================

TEST_CASE("Niveles: los troncos y tablones no admiten capas parciales") {
    // Los tres troncos
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD));
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD_ENCINO));
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD_OYAMEL));
    // Los tres tablones
    CHECK_FALSE(admiteNiveles(BLOCK_PLANKS));
    CHECK_FALSE(admiteNiveles(BLOCK_PLANKS_ENCINO));
    CHECK_FALSE(admiteNiveles(BLOCK_PLANKS_OYAMEL));

    // conNivel sobre madera devuelve el bloque entero, no una capa.
    CHECK(conNivel(BLOCK_WOOD, 3) == BLOCK_WOOD);
    CHECK(conNivel(BLOCK_PLANKS, 1) == BLOCK_PLANKS);

    // La madera NO puede ser la capa de ABAJO de una celda mixta: esa capa se
    // mide por niveles, y la madera no los tiene.
    CHECK(mixto(BLOCK_WOOD, 4, BLOCK_DIRT) == BLOCK_AIR);

    // Pero SI puede ser el RELLENO (la parte de arriba). Son dos cosas
    // distintas: un relleno no se parte en lonchas, ocupa de golpe desde
    // donde acaba la capa de abajo hasta el techo de la celda. Asi que un
    // tronco entero encima de una capa de tierra vale, y sigue siendo un
    // tronco entero -- nunca una loncha de 3 px.
    CHECK(mixto(BLOCK_DIRT, 4, BLOCK_PLANKS) != BLOCK_AIR);
    CHECK(mixtoRelleno(mixto(BLOCK_DIRT, 4, BLOCK_PLANKS)) == BLOCK_PLANKS);
}

TEST_CASE("Niveles: el terreno y los macizos SI siguen teniendolos") {
    CHECK(admiteNiveles(BLOCK_DIRT));
    CHECK(admiteNiveles(BLOCK_GRASS));
    CHECK(admiteNiveles(BLOCK_SAND));
    CHECK(admiteNiveles(BLOCK_GRAVEL));
    CHECK(admiteNiveles(BLOCK_STONE));
    CHECK(admiteNiveles(BLOCK_COBBLESTONE));

    // Y se siguen pudiendo entrelazar entre si.
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_SAND) != BLOCK_AIR);
    CHECK(mixto(BLOCK_GRAVEL, 5, BLOCK_GRASS) != BLOCK_AIR);
}

TEST_CASE("Niveles: la tabla sigue cabiendo en el hueco reservado") {
    // FAMILIAS_CON_NIVEL es el ANCHO reservado para el calculo de los IDs
    // mixtos, no el numero de familias vivas. Si la tabla lo superara, dos
    // parejas distintas darian el mismo ID y se corromperian los mundos.
    CHECK(familiasCaben());
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
