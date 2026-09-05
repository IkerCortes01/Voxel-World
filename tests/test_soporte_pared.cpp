#include <doctest/doctest.h>
#include "FisicaCaida.h"

// ============================================================================
// LO QUE SE PEGA A UNA PARED NO SE QUEDA PEGADO
// ============================================================================
// La pregunta que fija este archivo: si pongo un bloque al lado de una pared
// o de una columna alta, ¿se cae?
//
// La respuesta del motor es SI, y la razon es una sola regla:
//
//     UNA PARED NO SUJETA NADA. SOLO SUJETA LO QUE ESTA DEBAJO.
//
// El apoyo se mira siempre hacia ABAJO, nunca de lado. Da igual que el bloque
// toque una pared por una cara o por las cuatro: si no tiene suelo, cae.
//
// Esto se comprueba en dos sitios distintos del motor, y los dos tienen que
// estar de acuerdo:
//
//   1. AL COLOCAR   (placeBlock): mira el bloque de debajo con esSueloFirme.
//   2. AL ROMPER    (desprenderEstructura): el flood fill solo cuenta el
//      contacto con `ny < p.y`, es decir, por debajo.
//
// Aqui se fija la pieza comun de los dos: esSueloFirme, que es quien decide
// que es apoyo. Si alguien la ablandara para que una pared sujete, los
// bloques se quedarian flotando otra vez y estos tests lo cazan.

// ⚠️ isCrossSprite NO se define aqui.
//
// Es un simbolo GLOBAL del binario de tests, y test_derrumbe.cpp ya le da
// cuerpo para todos. Definirla otra vez rompe el enlazado (LNK2005), asi que
// este archivo solo la usa: basta con que el header la declare.

using Fisica::esSueloFirme;
using Fisica::puedeCaer;

// ----------------------------------------------------------------------------
// EL AIRE NO SUJETA: ESA ES LA REGLA QUE HACE CAER LO PEGADO A UNA PARED
// ----------------------------------------------------------------------------
// Cuando pegas un bloque al costado de una pared, lo que tiene DEBAJO es aire
// -- la pared esta al lado, no abajo. Por eso cae: no porque el motor
// reconozca "paredes", sino porque debajo no hay nada.

TEST_CASE("Soporte: debajo de un bloque pegado a una pared solo hay aire") {
    // Es el caso literal de la pregunta, reducido a lo que decide el motor.
    CHECK_FALSE(esSueloFirme(BLOCK_AIR));
}

TEST_CASE("Soporte: la pared en si SI sujetaria, pero solo por debajo") {
    // La piedra de la pared aguanta peso. Lo que no aguanta es hacerlo DE
    // LADO: eso lo decide el llamador mirando (x, y-1, z) y nunca (x+-1, y, z).
    //
    // Este test deja constancia de que el material no es el problema: el
    // mismo bloque que sujeta desde abajo no sujeta desde el costado, porque
    // nadie le pregunta desde el costado.
    const BlockType PAREDES[] = {
        BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_LIMESTONE,
        BLOCK_DIRT, BLOCK_PLANKS, BLOCK_PLANKS_ENCINO
    };
    for (BlockType t : PAREDES) {
        INFO("material de pared ", (int)t);
        CHECK(esSueloFirme(t));
    }
}

// ----------------------------------------------------------------------------
// UNA COLUMNA ALTA TAMPOCO SUJETA POR EL COSTADO
// ----------------------------------------------------------------------------

TEST_CASE("Soporte: la altura de la columna da igual") {
    // No hay nada en el motor que dependa de lo alta que sea la pared: la
    // comprobacion es local, mira UNA celda hacia abajo y ya. Una columna de
    // 3 bloques y una de 100 sujetan exactamente lo mismo por el costado:
    // nada.
    //
    // Se fija aqui para que quede escrito que la altura no entra en la
    // decision, que es justo lo que hace el comportamiento predecible.
    CHECK_FALSE(esSueloFirme(BLOCK_AIR));   // lo que hay bajo el bloque pegado
    CHECK(esSueloFirme(BLOCK_STONE));       // lo que hay al lado, y no cuenta
}

// ----------------------------------------------------------------------------
// LO QUE SE COLOCA HOY ES UNA CAPA, NO UN CUBO
// ----------------------------------------------------------------------------

TEST_CASE("Soporte: una capa parcial sujeta igual que el bloque entero") {
    // ⚠️ ESTE ES EL CASO QUE MAS FACIL SE ROMPE.
    //
    // placeBlock convierte casi todo lo que colocas en su nivel 1 (una capa
    // de 3 px). Si esSueloFirme no normalizara los niveles, una capa de
    // tierra no contaria como suelo y TODO lo que apilaras encima se caeria:
    // seria imposible construir.
    //
    // Por eso normaliza con bloqueBaseDe antes de decidir. Aqui se fija.
    const BlockType BASES[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_GRAVEL
    };
    for (BlockType base : BASES)
        for (int nivel = 1; nivel <= 7; ++nivel) {
            const BlockType capa = conNivel(base, nivel);
            INFO("capa de ", (int)base, " nivel ", nivel);
            CHECK(esSueloFirme(capa));
        }
}

TEST_CASE("Soporte: una celda mixta tambien sujeta") {
    // Una celda mixta esta llena de material: es suelo a todos los efectos.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_SAND);
    REQUIRE(m != BLOCK_AIR);
    CHECK(esSueloFirme(m));
}

// ----------------------------------------------------------------------------
// LO QUE NO SUJETA: SI TE APOYAS EN ESTO, TE CAES
// ----------------------------------------------------------------------------

TEST_CASE("Soporte: los liquidos no sujetan") {
    // Poner un bloque sobre agua lo hace caer: el agua no es suelo.
    CHECK_FALSE(esSueloFirme(BLOCK_WATER));
    CHECK_FALSE(esSueloFirme(BLOCK_LAVA));
}

TEST_CASE("Soporte: el follaje no sujeta") {
    // Un tronco o unas hojas no sostienen construccion. Es lo que permite que
    // un arbol se venga abajo entero en vez de sostenerse a si mismo.
    const BlockType ARBOL[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL
    };
    for (BlockType t : ARBOL) {
        INFO("pieza de arbol ", (int)t);
        CHECK_FALSE(esSueloFirme(t));
    }
}

TEST_CASE("Soporte: las plantas no sujetan") {
    // Apoyarse en una mata de hierba no sostiene nada.
    CHECK_FALSE(esSueloFirme(BLOCK_TALLGRASS));
}

// ----------------------------------------------------------------------------
// LA COHERENCIA CON EL DERRUMBE
// ----------------------------------------------------------------------------

TEST_CASE("Soporte: lo que se cuelga de una pared es material que puede caer") {
    // Los dos sistemas tienen que estar de acuerdo: si un material se puede
    // colocar colgando, tiene que poder caer. Un material que NO pudiera caer
    // se quedaria flotando pegado a la pared para siempre.
    //
    // ⚠️ El terreno natural es la excepcion DELIBERADA: no cae nunca (es
    // cimiento). Y por eso mismo placeBlock no usa el derrumbe de
    // estructuras, sino su propia comprobacion: asi una piedra recien puesta
    // en el aire SI cae, aunque la piedra del mundo nunca se derrumbe.
    const BlockType CONSTRUCCION[] = {
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL
    };
    for (BlockType t : CONSTRUCCION) {
        INFO("construccion ", (int)t);
        CHECK(esSueloFirme(t));   // sujeta a otros...
        CHECK(puedeCaer(t));      // ...pero ella misma se cae sin apoyo
    }
}
