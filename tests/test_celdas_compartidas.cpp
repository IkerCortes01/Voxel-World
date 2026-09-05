#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// TESTS DEL SISTEMA DE CELDAS COMPARTIDAS (N PIEZAS EN UN VOXEL)
// ============================================================================
// Un voxel guarda UN solo BlockType, pero ese ID puede describir VARIOS
// bloques conviviendo en el mismo espacio, cada uno con su caja y su
// comportamiento, seleccionables por separado.
//
// Antes el sistema estaba cableado a exactamente DOS piezas (un bool
// "segunda"), lo que ponia un techo artificial: meter una tercera obligaba a
// tocar el raycast, el mesher y la rotura. Ahora se pregunta por INDICE.
//
// Lo que fijan estos tests:
//   1. La API por indice es coherente con los atajos de dos piezas.
//   2. Ninguna celda declara mas piezas de las que puede haber.
//   3. Quitar una acompañante deja la principal; quitar la principal cae todo.
//   4. El maguey capado cumple todo lo anterior siendo un caso real.

// ----------------------------------------------------------------------------
// LA API GENERAL: N PIEZAS
// ----------------------------------------------------------------------------

TEST_CASE("Compartidas: un bloque normal cuenta como UNA pieza") {
    // Que un bloque corriente responda "1 pieza" es lo que permite al motor
    // recorrer piezas sin tratar aparte el caso normal.
    CHECK(piezasDe(BLOCK_STONE) == 1);
    CHECK(piezasDe(BLOCK_DIRT)  == 1);
    CHECK(piezaN(BLOCK_STONE, 0) == BLOCK_STONE);

    // Y no tiene una segunda.
    CHECK(piezaN(BLOCK_STONE, 1) == BLOCK_AIR);
}

TEST_CASE("Compartidas: pedir una pieza que no existe da AIR, no basura") {
    // El raycast recorre indices; si uno fuera de rango devolviera un bloque
    // cualquiera, se seleccionarian piezas fantasma.
    CHECK(piezaN(BLOCK_AGUAMIEL, -1) == BLOCK_AIR);
    CHECK(piezaN(BLOCK_AGUAMIEL,  2) == BLOCK_AIR);
    CHECK(piezaN(BLOCK_AGUAMIEL, 99) == BLOCK_AIR);
    CHECK(piezaN(BLOCK_IXTLE_CON_HIERBA, 2) == BLOCK_AIR);
}

TEST_CASE("Compartidas: ninguna celda pasa del tope declarado") {
    // MAX_PIEZAS_CELDA es el tamaño de los bucles que recorren piezas. Si una
    // celda declarara mas, sus piezas de sobra no se probarian nunca: serian
    // invisibles al raycast.
    const BlockType CELDAS[] = {
        BLOCK_IXTLE_CON_HIERBA, BLOCK_IXTLE_CON_FLOR,
        BLOCK_IXTLE_DOBLE, BLOCK_AGUAMIEL
    };
    for (BlockType c : CELDAS) {
        CHECK(esCompartido(c));
        CHECK(piezasDe(c) >= 2);                 // o no seria compartida
        CHECK(piezasDe(c) <= MAX_PIEZAS_CELDA);
    }
}

TEST_CASE("Compartidas: todas las piezas declaradas son bloques reales") {
    // Una pieza que devolviera AIR dentro del rango dejaria un hueco
    // seleccionable pero vacio.
    const BlockType CELDAS[] = {
        BLOCK_IXTLE_CON_HIERBA, BLOCK_IXTLE_CON_FLOR,
        BLOCK_IXTLE_DOBLE, BLOCK_AGUAMIEL
    };
    for (BlockType c : CELDAS) {
        const int n = piezasDe(c);
        for (int i = 0; i < n; ++i) {
            INFO("celda ", (int)c, " pieza ", i);
            CHECK(piezaN(c, i) != BLOCK_AIR);
        }
    }
}

TEST_CASE("Compartidas: los atajos de dos piezas coinciden con el indice") {
    // piezaPrimera/piezaSegunda son azucar sobre piezaN. Si divergieran,
    // medio motor (que usa los atajos) veria una cosa y el raycast otra.
    const BlockType CELDAS[] = {
        BLOCK_IXTLE_CON_HIERBA, BLOCK_IXTLE_CON_FLOR,
        BLOCK_IXTLE_DOBLE, BLOCK_AGUAMIEL
    };
    for (BlockType c : CELDAS) {
        CHECK(piezaPrimera(c) == piezaN(c, 0));
        CHECK(piezaSegunda(c) == piezaN(c, 1));
    }
}

TEST_CASE("Compartidas: quitarPieza y quitarPiezaN dicen lo mismo") {
    const BlockType CELDAS[] = {
        BLOCK_IXTLE_CON_HIERBA, BLOCK_IXTLE_CON_FLOR,
        BLOCK_IXTLE_DOBLE, BLOCK_AGUAMIEL
    };
    for (BlockType c : CELDAS) {
        CHECK(quitarPieza(c, false) == quitarPiezaN(c, 0));
        CHECK(quitarPieza(c, true)  == quitarPiezaN(c, 1));
    }
}

// ----------------------------------------------------------------------------
// LA REGLA DE QUE QUEDA AL QUITAR
// ----------------------------------------------------------------------------

TEST_CASE("Compartidas: quitar la PRINCIPAL se lleva la celda entera") {
    // Las acompañantes se apoyan en ella: la hierba crece entre las hojas del
    // ixtle, el jugo vive dentro del cuenco. Sin la principal no pueden
    // quedarse flotando.
    CHECK(quitarPiezaN(BLOCK_IXTLE_CON_HIERBA, 0) == BLOCK_AIR);
    CHECK(quitarPiezaN(BLOCK_IXTLE_CON_FLOR,   0) == BLOCK_AIR);
    CHECK(quitarPiezaN(BLOCK_IXTLE_DOBLE,      0) == BLOCK_AIR);
    CHECK(quitarPiezaN(BLOCK_AGUAMIEL,         0) == BLOCK_AIR);
}

TEST_CASE("Compartidas: quitar una ACOMPANANTE deja la principal en pie") {
    CHECK(quitarPiezaN(BLOCK_IXTLE_CON_HIERBA, 1) == BLOCK_IXTLE_HOJA);
    CHECK(quitarPiezaN(BLOCK_IXTLE_CON_FLOR,   1) == BLOCK_IXTLE_HOJA);
    CHECK(quitarPiezaN(BLOCK_IXTLE_DOBLE,      1) == BLOCK_IXTLE_HOJA);
    // El maguey: quitarle el jugo deja el cuenco, listo para volver a manar.
    CHECK(quitarPiezaN(BLOCK_AGUAMIEL,         1) == BLOCK_MAGUEY_HUECO);
}

TEST_CASE("Compartidas: lo que queda nunca es la propia celda") {
    // Si quitar una pieza devolviera la celda intacta, el bloque seria
    // indestructible sin querer: golpear no cambiaria nada.
    const BlockType CELDAS[] = {
        BLOCK_IXTLE_CON_HIERBA, BLOCK_IXTLE_CON_FLOR,
        BLOCK_IXTLE_DOBLE, BLOCK_AGUAMIEL
    };
    for (BlockType c : CELDAS) {
        const int n = piezasDe(c);
        for (int i = 0; i < n; ++i) {
            INFO("celda ", (int)c, " quitando pieza ", i);
            CHECK(quitarPiezaN(c, i) != c);
        }
    }
}

// ----------------------------------------------------------------------------
// EL MAGUEY CAPADO COMO CASO REAL
// ----------------------------------------------------------------------------

TEST_CASE("Maguey: el cuenco y el jugo son dos bloques distintos") {
    // Es la propiedad pedida: dos bloques en el mismo espacio, cada uno
    // seleccionable.
    REQUIRE(esCompartido(BLOCK_AGUAMIEL));
    CHECK(piezasDe(BLOCK_AGUAMIEL) == 2);
    CHECK(piezaN(BLOCK_AGUAMIEL, 0) == BLOCK_MAGUEY_HUECO);
    CHECK(piezaN(BLOCK_AGUAMIEL, 1) == BLOCK_AGUAMIEL);
    CHECK(piezaN(BLOCK_AGUAMIEL, 0) != piezaN(BLOCK_AGUAMIEL, 1));
}

TEST_CASE("Maguey: capar no suelta espinas") {
    // Capar ABRE la planta para que mane; no es cosecharla. Darle ademas un
    // puñado de espinas convertia el capado en material gratis.
    //
    // (getBlockDrops vive en main.cpp; aqui se fija la parte comprobable: la
    // punta no es roca ni gasta herramienta, y el drop se cierra en las dos
    // tablas del motor.)
    CHECK_FALSE(esRocaParaPico(BLOCK_MAGUEY_PUNTA));
    CHECK(desgastePico(BLOCK_MAGUEY_PUNTA) == 0);
    CHECK(desgasteHacha(BLOCK_MAGUEY_HUECO) == 0);
}

TEST_CASE("Maguey: el jugo cabe dentro del cuenco sin tocar el borde") {
    // Es lo que hace que se pueda seleccionar por separado: si el liquido
    // llegara al borde, su caja y la del cuenco se solaparian y el rayo no
    // podria distinguirlos.
    CHECK(CAJETE_PARED  > 0.0f);
    CHECK(CAJETE_SUELO  > 0.0f);
    CHECK(AGUAMIEL_ALTO > CAJETE_SUELO);   // hay liquido
    CHECK(AGUAMIEL_ALTO < 1.0f);           // no rebosa

    // Y queda hueco de verdad entre las paredes.
    CHECK(1.0f - 2.0f * CAJETE_PARED > 0.0f);
}

TEST_CASE("Maguey: cualquier tazon recoge el jugo y conserva su madera") {
    // La unica via para sacarlo: no se puede romper.
    CHECK(tazonConAguamiel(BLOCK_TAZON_PINO)        == BLOCK_TAZON_PINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_PINO_AGUA)   == BLOCK_TAZON_PINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_ENCINO)      == BLOCK_TAZON_ENCINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_ENCINO_AGUA) == BLOCK_TAZON_ENCINO_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_OYAMEL)      == BLOCK_TAZON_OYAMEL_AGUAMIEL);
    CHECK(tazonConAguamiel(BLOCK_TAZON_OYAMEL_AGUA) == BLOCK_TAZON_OYAMEL_AGUAMIEL);
}
