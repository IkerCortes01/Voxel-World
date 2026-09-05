#include <doctest/doctest.h>
#include "CristalMineral.h"

// ============================================================================
// LOS CRISTALES DE MINERAL
// ============================================================================
// El mineral dejo de ser una textura: de la roca salen cristales, y lo que el
// bloque suelta al picarlo depende de cuantos tenia.
//
// Aqui se fija la logica, que es pura sobre BlockType y una posicion. La
// geometria vive en el mesher y no se puede testear sin OpenGL.

TEST_CASE("Cristales: los siete minerales los llevan") {
    const BlockType MINERALES[] = {
        BLOCK_COAL_ORE, BLOCK_PYRITE_ORE, BLOCK_SCRAP_METAL,
        BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_SILVER_ORE, BLOCK_DIAMOND_ORE
    };
    for (BlockType t : MINERALES) {
        INFO("mineral ", (int)t);
        CHECK(Cristal::tieneCristales(t));
    }
}

TEST_CASE("Cristales: la roca corriente NO los lleva") {
    // Si la piedra llevara cristales, el subsuelo entero se llenaria de
    // geometria y el frame rate se hundiria.
    const BlockType NO[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_LIMESTONE,
        BLOCK_AIR, BLOCK_WATER, BLOCK_COBBLESTONE
    };
    for (BlockType t : NO) {
        INFO("no deberia llevar cristales: ", (int)t);
        CHECK_FALSE(Cristal::tieneCristales(t));
    }
}

TEST_CASE("Cristales: la forma sale de la mineralogia, no del azar") {
    using namespace Cristal;
    // El carbon NO cristaliza: es roca organica. Darle prismas seria
    // inventarse geologia.
    CHECK(EspecieDe(BLOCK_COAL_ORE).habito == Habito::MASA);
    // La pirita es EL cristal cubico por excelencia.
    CHECK(EspecieDe(BLOCK_PYRITE_ORE).habito == Habito::CUBO);
    // El diamante cristaliza en octaedros.
    CHECK(EspecieDe(BLOCK_DIAMOND_ORE).habito == Habito::OCTAEDRO);
    // El oro y la plata nativos son dendriticos, no prismaticos.
    CHECK(EspecieDe(BLOCK_GOLD_ORE).habito   == Habito::DENDRITA);
    CHECK(EspecieDe(BLOCK_SILVER_ORE).habito == Habito::DENDRITA);
}

TEST_CASE("Cristales: los metales relucen mas que el carbon") {
    using namespace Cristal;
    // El brillo no es decorativo: distingue un metal nativo de una masa mate.
    CHECK(EspecieDe(BLOCK_DIAMOND_ORE).brillo > EspecieDe(BLOCK_COAL_ORE).brillo);
    CHECK(EspecieDe(BLOCK_GOLD_ORE).brillo    > EspecieDe(BLOCK_COAL_ORE).brillo);
    CHECK(EspecieDe(BLOCK_COAL_ORE).brillo    < 1.0f);   // el carbon es mate
}

TEST_CASE("Cristales: el numero varia de un bloque a otro") {
    // Si todos los bloques tuvieran los mismos cristales, la veta se veria
    // como un sello repetido y la ley no significaria nada.
    int minimo = 99, maximo = -1;
    for (int x = 0; x < 40; ++x)
        for (int z = 0; z < 40; ++z) {
            const int n = Cristal::CuantosEn(BLOCK_PYRITE_ORE, x, 30, z);
            if (n < minimo) minimo = n;
            if (n > maximo) maximo = n;
        }
    INFO("min=", minimo, " max=", maximo);
    CHECK(maximo > minimo);     // hay variedad
    CHECK(minimo >= 1);         // nunca cero: seria un mineral sin cristales
    CHECK(maximo <= 6);         // el tope duro que acota el coste
}

TEST_CASE("Cristales: el mismo bloque tiene siempre los mismos") {
    // Determinismo: es lo que permite no guardarlos en disco y que el mundo
    // se vea igual al recargarlo.
    for (int i = 0; i < 50; ++i) {
        const int a = Cristal::CuantosEn(BLOCK_GOLD_ORE, i, 20, i * 3);
        const int b = Cristal::CuantosEn(BLOCK_GOLD_ORE, i, 20, i * 3);
        CHECK(a == b);
    }
}

TEST_CASE("Ley: un bloque nunca da menos de uno") {
    // Picar un mineral y no llevarse nada seria un bug que el jugador leeria
    // como que el juego le ha robado el bloque.
    const BlockType MINERALES[] = {
        BLOCK_COAL_ORE, BLOCK_PYRITE_ORE, BLOCK_SCRAP_METAL,
        BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_SILVER_ORE, BLOCK_DIAMOND_ORE
    };
    for (BlockType t : MINERALES)
        for (int x = 0; x < 30; ++x)
            for (int z = 0; z < 30; ++z) {
                const int n = Cristal::LeyDelBloque(t, x, 25, z);
                INFO("mineral ", (int)t, " en ", x, ",", z, " da ", n);
                CHECK(n >= 1);
            }
}

TEST_CASE("Ley: el diamante nunca da montones") {
    // Es el mineral raro por excelencia: si un solo bloque diera ocho, dejaria
    // de significar nada encontrarlo.
    for (int x = 0; x < 60; ++x)
        for (int z = 0; z < 60; ++z)
            CHECK(Cristal::LeyDelBloque(BLOCK_DIAMOND_ORE, x, 10, z) <= 3);
}

TEST_CASE("Ley: la cantidad VARIA entre bloques") {
    // Es lo que se pidio: un bloque con veta rica da mas que uno con un
    // cristal suelto, y la diferencia se ve antes de picar.
    int minimo = 99, maximo = -1;
    for (int x = 0; x < 60; ++x)
        for (int z = 0; z < 60; ++z) {
            const int n = Cristal::LeyDelBloque(BLOCK_COAL_ORE, x, 30, z);
            if (n < minimo) minimo = n;
            if (n > maximo) maximo = n;
        }
    INFO("carbon: min=", minimo, " max=", maximo);
    CHECK(maximo > minimo);
}

TEST_CASE("Ley: lo que no es mineral da uno y ya") {
    // La piedra corriente no pasa por la ley: se suelta a si misma.
    CHECK(Cristal::LeyDelBloque(BLOCK_STONE, 5, 5, 5) == 1);
    CHECK(Cristal::LeyDelBloque(BLOCK_DIRT,  7, 9, 3) == 1);
}
