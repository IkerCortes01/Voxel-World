#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// TESTS DEL AGUAMIEL Y EL MAGUEY CAPADO
// ============================================================================
// Reglas que fijan:
//
//   1. El aguamiel SOLO se recoge con un tazon. Romperlo a mano no da nada.
//   2. El cajete es un CUENCO: suelo, cuatro paredes y un agujero en medio.
//   3. El aguamiel vive DENTRO de ese agujero, sin rebosar.
//   4. Solo 1 de cada 4 magueyes capados mana, y de forma DETERMINISTA.
//
// La regla 1 es la que mas facil se rompe sin querer: el motor tiene DOS
// tablas de drops (getBlockDrops y GameState::getDroppedItem) y minar pasa
// por las dos. Cerrar solo una deja el agujero abierto por la otra.

// ----------------------------------------------------------------------------
// LA GEOMETRIA DEL CAJETE
// ----------------------------------------------------------------------------

TEST_CASE("Cajete: las paredes dejan un agujero real en medio") {
    // Si las paredes se comieran el voxel entero no habria hueco donde
    // juntar el jugo, y el "cuenco" seria un cubo macizo.
    CHECK(CAJETE_PARED > 0.0f);
    CHECK(CAJETE_PARED < 0.5f);          // dos paredes no pueden juntarse

    const float huecoInterior = 1.0f - 2.0f * CAJETE_PARED;
    CHECK(huecoInterior > 0.0f);
    CHECK(huecoInterior == doctest::Approx(10.0f / 16.0f));
}

TEST_CASE("Cajete: el fondo esta por debajo del borde") {
    // El suelo del cuenco tiene que dejar sitio por encima; si llegara a 1.0
    // el cajete estaria lleno de maguey y no cabria nada.
    CHECK(CAJETE_SUELO > 0.0f);
    CHECK(CAJETE_SUELO < 1.0f);
}

TEST_CASE("Aguamiel: se queda DENTRO del cuenco, sin rebosar") {
    // El liquido arranca en el fondo del cajete y no llega al borde. Esos dos
    // hechos son los que hacen que se vea DENTRO del maguey y no como una
    // capa encima del bloque.
    CHECK(AGUAMIEL_ALTO > CAJETE_SUELO);   // hay liquido de verdad
    CHECK(AGUAMIEL_ALTO < 1.0f);           // no rebosa por el borde

    // Y su lamina cabe holgada dentro del agujero.
    const float profundidad = AGUAMIEL_ALTO - CAJETE_SUELO;
    CHECK(profundidad > 0.0f);
    CHECK(profundidad == doctest::Approx(6.0f / 16.0f));
}

// ----------------------------------------------------------------------------
// EL MAGUEY CAPADO NO ES UN CUBO
// ----------------------------------------------------------------------------

TEST_CASE("Cajete: ni el hueco ni el aguamiel son cubos macizos") {
    // esBloqueMacizoOpaco decide quien puede dibujarse sin recorte por alfa
    // (y quien tapa las caras de sus vecinos). Un cuenco con agujero no
    // puede: por el hueco se ve el interior, y taparia con negro lo de al
    // lado.
    //
    // No se puede llamar a esa funcion desde aqui (vive en main.cpp), pero si
    // comprobar la propiedad de la que depende: que NO son roca ni bloques
    // normales de terreno.
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_HUECO));
    CHECK_FALSE(esRocaParaPico(BLOCK_AGUAMIEL));
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_PUNTA));
}

TEST_CASE("Maguey: el pico no sirve, y no se gasta intentandolo") {
    // La punta es planta, no piedra. Ademas de que el pico no deba poder con
    // ella, lo importante es que NO se gaste: castigar al jugador por probar
    // seria peor que decirle que no sirve.
    CHECK(desgastePico(BLOCK_MAGUEY_PUNTA) == 0);
    CHECK(desgastePico(BLOCK_MAGUEY_HUECO) == 0);
    CHECK(desgastePico(BLOCK_AGUAMIEL)     == 0);
}

// ----------------------------------------------------------------------------
// EL 25%: DETERMINISTA Y BIEN REPARTIDO
// ----------------------------------------------------------------------------
//
// El dado que decide si un maguey capado mana sale del HASH DE SU POSICION,
// no de rand(). Eso importa por dos motivos:
//   - un maguey concreto da aguamiel o no SIEMPRE, asi que el jugador no
//     puede capar y recapar hasta que le toque
//   - dos partidas con la misma semilla dan el mismo mundo
//
// Aqui se replica ese hash exactamente igual que en main.cpp y se comprueban
// las dos propiedades.

static bool magueyMana(int x, int y, int z) {
    unsigned h = (unsigned)(x * 73856093)
               ^ (unsigned)(y * 19349663)
               ^ (unsigned)(z * 83492791);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
    return (h % 100u) < 25u;
}

TEST_CASE("Aguamiel: el mismo maguey da SIEMPRE el mismo resultado") {
    // Sin esto, capar seria una tragaperras: el jugador volveria a intentarlo
    // hasta que saliera.
    for (int i = 0; i < 200; ++i) {
        const int x = i * 7 - 500, y = 64 + (i % 13), z = i * 3 - 200;
        const bool primera = magueyMana(x, y, z);
        CHECK(magueyMana(x, y, z) == primera);
        CHECK(magueyMana(x, y, z) == primera);
    }
}

TEST_CASE("Aguamiel: sale aproximadamente 1 de cada 4") {
    // Se recorre una malla grande y se cuenta. No tiene que dar 25.00% clavado
    // -- es un hash, no un reparto exacto -- pero si quedarse cerca: si
    // saliera 5% o 60%, el ritmo del juego cambiaria por completo.
    int manan = 0, total = 0;
    for (int x = -40; x < 40; ++x)
        for (int z = -40; z < 40; ++z) {
            if (magueyMana(x, 70, z)) ++manan;
            ++total;
        }

    const double pct = 100.0 * manan / total;
    INFO("porcentaje medido: ", pct, "%");
    CHECK(pct > 20.0);
    CHECK(pct < 30.0);
}

TEST_CASE("Aguamiel: la altura tambien cuenta en el dado") {
    // Dos magueyes en la misma columna pero a distinta altura son plantas
    // distintas, asi que su suerte tiene que poder diferir. Si el hash
    // ignorara la Y, todas las matas de una columna compartirian destino.
    int distintos = 0;
    for (int y = 0; y < 100; ++y)
        if (magueyMana(10, y, 20) != magueyMana(10, y + 1, 20)) ++distintos;
    CHECK(distintos > 0);
}

// ----------------------------------------------------------------------------
// LO QUE NO DEBE CAMBIAR
// ----------------------------------------------------------------------------

TEST_CASE("Aguamiel: sigue sin poder colocarse como bloque") {
    // Es un liquido dentro de la planta, no algo que el jugador ponga en el
    // suelo. (isPlaceableItem vive en main.cpp; lo que se fija aqui es que
    // esta fuera del rango de bloques colocables del terreno.)
    CHECK((int)BLOCK_AGUAMIEL > BLOCK_LAST_PLACEABLE);
}

TEST_CASE("Aguamiel: los tazones vacios siguen sirviendo para recogerlo") {
    // La unica via para conseguirlo. Si un tazon dejara de reconocerse, el
    // aguamiel se volveria inalcanzable: no se puede coger a mano.
    CHECK(esTazonVacio(BLOCK_TAZON_PINO));
    CHECK(esTazonVacio(BLOCK_TAZON_ENCINO));
    CHECK(esTazonVacio(BLOCK_TAZON_OYAMEL));
}
