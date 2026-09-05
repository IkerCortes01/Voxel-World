#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

using namespace Compuesto;

// ============================================================================
// APILAR SIN DEJAR HUECOS
// ============================================================================
// BUG QUE ESTO PROTEGE: al poner un bloque sobre otro, si en la celda de
// encima habia una brizna de HIERBA, el bloque nuevo se colocaba UN VOXEL MAS
// ALTO y quedaba un hueco de aire en medio, con la hierba dentro. Dos piedras
// flotando separadas.
//
// La causa era una comprobacion demasiado estricta:
//
//     if (encima == BLOCK_AIR)     // <-- la hierba NO es aire
//
// Y el mundo esta sembrado de hierba, asi que pasaba constantemente al
// construir sobre el suelo.
//
// LA REGLA CORRECTA, que es lo que fijan estos tests:
//
//     una celda esta LIBRE para apilar si tiene aire
//     o vegetacion que no ocupa volumen (se sustituye)
//
//     pero NO si tiene algo que si ocupa sitio: ramas, nopal, maguey.
//     Eso son piezas reales y machacarlas al apilar seria destruir cosas
//     sin querer.
//
// La funcion que decide vive dentro de placeBlock (en main.cpp, que los tests
// no enlazan), asi que aqui se replica su criterio EXACTO. Si alguien cambia
// uno de los dos sin el otro, estos tests dejan de describir el juego -- por
// eso el criterio se mantiene corto y se comenta en los dos sitios.

// El mismo criterio que usa placeBlock. Se replica aqui porque la original
// esta dentro de una funcion de 900 lineas que necesita el motor entero.
static bool ocupaSitio(BlockType encima) {
    return esCladodio(encima) || encima == BLOCK_NOPAL_FRUTO ||
           esRaiz(encima) || esCompuesto(encima);
}

// ¿Se puede apilar sobre esta celda sin dejar hueco?
// (La version del motor añade isRama(), que vive en main.cpp; aqui las ramas
//  se cubren aparte en su propio test.)
static bool celdaLibre(BlockType encima, bool esVegetacionPlana) {
    if (encima == BLOCK_AIR) return true;
    return esVegetacionPlana && !ocupaSitio(encima);
}

// ----------------------------------------------------------------------------
// EL CASO DEL BUG
// ----------------------------------------------------------------------------

TEST_CASE("Apilado: la hierba NO impide apilar") {
    // El caso exacto de la captura: piedra, hierba encima, y se intenta poner
    // otra piedra. Tiene que quedar pegada, sustituyendo la hierba.
    CHECK(celdaLibre(BLOCK_TALLGRASS, true));
}

TEST_CASE("Apilado: el aire sigue siendo el caso normal") {
    CHECK(celdaLibre(BLOCK_AIR, false));
    CHECK(celdaLibre(BLOCK_AIR, true));
}

TEST_CASE("Apilado: lo solido SI impide apilar en esa celda") {
    // Si encima hay piedra, no se puede meter otro bloque ahi: el camino
    // normal se encarga de buscarle sitio.
    CHECK_FALSE(celdaLibre(BLOCK_STONE, false));
    CHECK_FALSE(celdaLibre(BLOCK_DIRT, false));
    CHECK_FALSE(celdaLibre(BLOCK_WOOD, false));
}

// ----------------------------------------------------------------------------
// LO QUE NO SE PUEDE MACHACAR
// ----------------------------------------------------------------------------
// La vegetacion plana se sustituye, pero hay plantas que SI ocupan volumen.
// Esas son piezas reales y aplastarlas al apilar seria destruir cosas sin
// querer.

TEST_CASE("Apilado: el nopal no se machaca") {
    // Un cladodio es una losa de 5/16, no un plano: ocupa sitio de verdad.
    CHECK(ocupaSitio(BLOCK_NOPAL_CLADODIO));
    CHECK(ocupaSitio(BLOCK_NOPAL_FRUTO));
    CHECK_FALSE(celdaLibre(BLOCK_NOPAL_CLADODIO, true));
    CHECK_FALSE(celdaLibre(BLOCK_NOPAL_FRUTO, true));
}

TEST_CASE("Apilado: las raices no se machacan") {
    const BlockType RAICES[] = {
        BLOCK_RAIZ_PEQUENA, BLOCK_RAIZ_MEDIANA,
        BLOCK_RAIZ_GRANDE, BLOCK_RAIZ_ENORME
    };
    for (BlockType r : RAICES) {
        INFO("raiz ", (int)r);
        CHECK(ocupaSitio(r));
        CHECK_FALSE(celdaLibre(r, true));
    }
}

TEST_CASE("Apilado: un maguey no se machaca al construir encima") {
    // Es el caso mas caro de perder: un maguey productor tarda minutos en
    // llenarse de aguamiel. Aplastarlo poniendo un bloque seria un destrozo
    // silencioso.
    for (uint16_t e = Maguey::BROTE; e <= Maguey::PRODUCTOR; ++e) {
        const BlockType m = Maguey::nuevo(e, 0);
        INFO("maguey etapa ", e);
        CHECK(ocupaSitio(m));
        CHECK_FALSE(celdaLibre(m, true));
    }

    // Ni siquiera uno capado y lleno, que es el mas valioso.
    const BlockType lleno = Maguey::conAguamiel(
        Maguey::capar(Maguey::nuevo(Maguey::PRODUCTOR, 0)), 15);
    CHECK(ocupaSitio(lleno));
    CHECK_FALSE(celdaLibre(lleno, true));
}

// ----------------------------------------------------------------------------
// LA REGLA DE FONDO
// ----------------------------------------------------------------------------

TEST_CASE("Apilado: nada que ocupe volumen se considera celda libre") {
    // Barrido: si un bloque ocupa sitio, JAMAS puede contar como libre,
    // aunque el motor lo clasifique como sprite.
    const BlockType CANDIDATOS[] = {
        BLOCK_NOPAL_CLADODIO, BLOCK_NOPAL_CLADODIO_X2,
        BLOCK_NOPAL_CLADODIO_X3, BLOCK_NOPAL_CLADODIO_Z2,
        BLOCK_NOPAL_CLADODIO_Z3, BLOCK_NOPAL_CLADODIO_DIAG,
        BLOCK_NOPAL_FRUTO,
        BLOCK_RAIZ_PEQUENA, BLOCK_RAIZ_ENORME
    };
    for (BlockType t : CANDIDATOS) {
        INFO("bloque ", (int)t);
        if (ocupaSitio(t)) CHECK_FALSE(celdaLibre(t, true));
    }
}

TEST_CASE("Apilado: el criterio distingue plano de volumetrico") {
    // La hierba es un plano: se atraviesa, no ocupa nada, se sustituye.
    CHECK_FALSE(ocupaSitio(BLOCK_TALLGRASS));

    // El nopal y el maguey ocupan volumen: se respetan.
    CHECK(ocupaSitio(BLOCK_NOPAL_CLADODIO));
    CHECK(ocupaSitio(Maguey::nuevo(Maguey::ADULTO, 0)));
}
