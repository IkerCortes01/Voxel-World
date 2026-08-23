#include <doctest/doctest.h>
#include "Inventory.h"

// ============================================================================
// EL HACHA DE PEDERNAL Y EL MAGUEY MADURO
// ============================================================================

TEST_CASE("Hacha de pedernal: aguanta 390 bloques, mas que la de piedra") {
    CHECK(HACHA_PEDERNAL_BLOQUES == 390);
    CHECK(HACHA_PEDERNAL_MEDIOS == 780);
    CHECK(vidaMaximaHerramienta(BLOCK_HACHA_PEDERNAL) == 780);

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

TEST_CASE("Hacha de pedernal: se rompe a los 390 bloques exactos") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_HACHA_PEDERNAL, 1);

    for (int i = 0; i < 389; ++i) {
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

    // Media vida son 195 bloques, no 125 como el hacha de piedra.
    for (int i = 0; i < 195; ++i)
        inv.gastarHerramienta(desgasteHerramienta(BLOCK_HACHA_PEDERNAL, BLOCK_WOOD));

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(0.5f));
}

// ============================================================================
// EL PICO DE PEDERNAL
// ============================================================================

TEST_CASE("Pico de pedernal: aguanta 550, mas que el de piedra") {
    CHECK(PICO_PEDERNAL_BLOQUES == 550);
    CHECK(PICO_PEDERNAL_MEDIOS == 1100);
    CHECK(vidaMaximaHerramienta(BLOCK_PICO_PEDERNAL) == 1100);

    CHECK(vidaMaximaHerramienta(BLOCK_PICO_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_PICO_PIEDRA));
}

TEST_CASE("Pico de pedernal: es un pico y rompe la misma roca") {
    CHECK(esPico(BLOCK_PICO_PEDERNAL));
    CHECK(esPico(BLOCK_PICO_PIEDRA));
    CHECK_FALSE(esPico(BLOCK_HACHA_PEDERNAL));
    CHECK_FALSE(esPico(BLOCK_MARTILLO_PIEDRA));

    // La roca le gasta vida igual que al de piedra.
    CHECK(desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_STONE) == 2);
    CHECK(desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_COAL_ORE) == 2);
    // Y lo organico no: eso es del hacha.
    CHECK(desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_WOOD) == 0);
    // Ni la punta del maguey, que es solo del hacha de pedernal.
    CHECK(desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_MAGUEY_PUNTA) == 0);
}

TEST_CASE("Pico de pedernal: se rompe a los 550 bloques exactos") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_PICO_PEDERNAL, 1);

    for (int i = 0; i < 549; ++i) {
        CHECK_FALSE(inv.gastarHerramienta(
            desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_STONE)));
    }
    CHECK(inv.gastarHerramienta(
        desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_STONE)));
    CHECK(inv.at(0).isEmpty());
}

TEST_CASE("Pico de pedernal: barrita con su propia escala") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_PICO_PEDERNAL, 1);

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(1.0f));

    // Media vida son 275 bloques, no 170 como el de piedra.
    for (int i = 0; i < 275; ++i)
        inv.gastarHerramienta(desgasteHerramienta(BLOCK_PICO_PEDERNAL, BLOCK_STONE));

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(0.5f));
}

// ============================================================================
// EL MARTILLO DE PEDERNAL
// ============================================================================

TEST_CASE("Martillo de pedernal: 250 usos, mas que los 150 del de piedra") {
    CHECK(MARTILLO_PEDERNAL_USOS == 250);
    CHECK(MARTILLO_PEDERNAL_MEDIOS == 500);
    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PEDERNAL) == 500);

    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_MARTILLO_PIEDRA));
}

TEST_CASE("Martillo de pedernal: es martillo, y los martillos no pican") {
    CHECK(esMartillo(BLOCK_MARTILLO_PEDERNAL));
    CHECK(esMartillo(BLOCK_MARTILLO_PIEDRA));
    CHECK_FALSE(esMartillo(BLOCK_HACHA_PEDERNAL));
    CHECK_FALSE(esMartillo(BLOCK_PICO_PEDERNAL));

    // Ningun bloque le gasta vida al romperlo: se gasta al CRAFTEAR.
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PEDERNAL, BLOCK_STONE) == 0);
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PEDERNAL, BLOCK_WOOD) == 0);
    CHECK(desgasteHerramienta(BLOCK_MARTILLO_PEDERNAL, BLOCK_MAGUEY_PUNTA) == 0);
}

TEST_CASE("Martillo de pedernal: aguanta 250 golpes de taller") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_MARTILLO_PEDERNAL, 1);

    // Un uso de taller = 2 medios, igual que el de piedra.
    for (int i = 0; i < 249; ++i)
        CHECK_FALSE(inv.gastarHerramienta(2));

    CHECK(inv.gastarHerramienta(2));
    CHECK(inv.at(0).isEmpty());
}

TEST_CASE("Martillo de pedernal: barrita con su propia escala") {
    Inventory inv;
    inv.selectedSlot = 0;
    inv.at(0).add(BLOCK_MARTILLO_PEDERNAL, 1);

    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(1.0f));

    // Media vida son 125 usos, no 75 como el de piedra.
    for (int i = 0; i < 125; ++i) inv.gastarHerramienta(2);
    CHECK(inv.vidaFraccionSlot(0) == doctest::Approx(0.5f));
}

TEST_CASE("Pedernal: las tres herramientas de pedernal aguantan mas") {
    // La rama de pedernal entera mejora a la de piedra, sin excepciones.
    CHECK(vidaMaximaHerramienta(BLOCK_HACHA_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_HACHA_PIEDRA));
    CHECK(vidaMaximaHerramienta(BLOCK_PICO_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_PICO_PIEDRA));
    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PEDERNAL) >
          vidaMaximaHerramienta(BLOCK_MARTILLO_PIEDRA));
}

TEST_CASE("Los tazones: uno por especie de madera") {
    // Son tres bloques distintos, no uno generico: el tazon sale de la
    // madera que se use, igual que los tablones salen de su tronco.
    CHECK(BLOCK_TAZON_PINO != BLOCK_TAZON_ENCINO);
    CHECK(BLOCK_TAZON_ENCINO != BLOCK_TAZON_OYAMEL);
    CHECK(BLOCK_TAZON_PINO != BLOCK_TAZON_OYAMEL);

    // Y ninguno es herramienta: se acumulan como items normales.
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_PINO));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_ENCINO));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_OYAMEL));
}

// ============================================================================
// LOS TAZONES CON AGUA
// ============================================================================

TEST_CASE("Tazones: vacio y lleno se corresponden, cada uno con su madera") {
    // Ida: cada tazon vacio da SU version con agua, no la de otra madera.
    CHECK(tazonLleno(BLOCK_TAZON_PINO)   == BLOCK_TAZON_PINO_AGUA);
    CHECK(tazonLleno(BLOCK_TAZON_ENCINO) == BLOCK_TAZON_ENCINO_AGUA);
    CHECK(tazonLleno(BLOCK_TAZON_OYAMEL) == BLOCK_TAZON_OYAMEL_AGUA);

    // Vuelta: y al vaciarlo se recupera exactamente el de partida.
    CHECK(tazonVaciado(BLOCK_TAZON_PINO_AGUA)   == BLOCK_TAZON_PINO);
    CHECK(tazonVaciado(BLOCK_TAZON_ENCINO_AGUA) == BLOCK_TAZON_ENCINO);
    CHECK(tazonVaciado(BLOCK_TAZON_OYAMEL_AGUA) == BLOCK_TAZON_OYAMEL);
}

TEST_CASE("Tazones: la ida y la vuelta son inversas exactas") {
    // Llenar y vaciar tiene que devolver el mismo tazon: si no, la madera se
    // perderia por el camino y un tazon de encino volveria siendo de pino.
    const BlockType VACIOS[3] = { BLOCK_TAZON_PINO, BLOCK_TAZON_ENCINO,
                                  BLOCK_TAZON_OYAMEL };
    for (BlockType v : VACIOS)
        CHECK(tazonVaciado(tazonLleno(v)) == v);
}

TEST_CASE("Tazones: lo que no es tazon no se convierte en nada") {
    CHECK(tazonLleno(BLOCK_STONE) == BLOCK_AIR);
    CHECK(tazonLleno(BLOCK_HACHA_PIEDRA) == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_WATER) == BLOCK_AIR);
    // Un tazon YA lleno no se puede volver a llenar.
    CHECK(tazonLleno(BLOCK_TAZON_PINO_AGUA) == BLOCK_AIR);
    // Ni uno vacio vaciarse mas.
    CHECK(tazonVaciado(BLOCK_TAZON_PINO) == BLOCK_AIR);
}

TEST_CASE("Tazones: las tres preguntas distinguen vacio, lleno y ninguno") {
    CHECK(esTazonVacio(BLOCK_TAZON_PINO));
    CHECK_FALSE(esTazonVacio(BLOCK_TAZON_PINO_AGUA));

    CHECK(esTazonConAgua(BLOCK_TAZON_OYAMEL_AGUA));
    CHECK_FALSE(esTazonConAgua(BLOCK_TAZON_OYAMEL));

    // esTazon cubre los dos estados.
    CHECK(esTazon(BLOCK_TAZON_ENCINO));
    CHECK(esTazon(BLOCK_TAZON_ENCINO_AGUA));
    CHECK_FALSE(esTazon(BLOCK_STONE));
}

TEST_CASE("Tazones: ninguno es roca para el pico, ni lleno ni vacio") {
    // El pico define "roca" por EXCLUSION, asi que cada objeto nuevo entra
    // solo en su lista si nadie lo excluye. Ya paso una vez con el maguey.
    CHECK_FALSE(esRocaParaPico(BLOCK_TAZON_PINO));
    CHECK_FALSE(esRocaParaPico(BLOCK_TAZON_PINO_AGUA));
    CHECK_FALSE(esRocaParaPico(BLOCK_TAZON_ENCINO_AGUA));
    CHECK_FALSE(esRocaParaPico(BLOCK_TAZON_OYAMEL_AGUA));
}

TEST_CASE("Tazones con agua: no son herramientas, se apilan como items") {
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_PINO_AGUA));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_ENCINO_AGUA));
    CHECK_FALSE(esHerramientaGastable(BLOCK_TAZON_OYAMEL_AGUA));

    Inventory inv;
    inv.at(0).add(BLOCK_TAZON_PINO_AGUA, 3);
    CHECK(inv.vidaFraccionSlot(0) == -1.0f);   // -1 = sin barra
    CHECK(inv.at(0).count == 3);
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
    CHECK((int)BLOCK_PICO_PEDERNAL <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_MARTILLO_PEDERNAL <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_TAZON_OYAMEL_AGUA <= BLOCK_TYPE_MAX);
}
