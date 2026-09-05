#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// LO QUE CAE SE ACOPLA AL NIVEL DONDE SE POSA
// ============================================================================
// El terreno de este motor no va de celda en celda: una celda puede llevar una
// CAPA de 1 a 7 octavos de alto. Un bloque que caia sobre una capa asi se
// plantaba en la celda de ARRIBA, y entre los dos quedaba a la vista el hueco
// que la capa no llena -- hasta 14 px de aire.
//
//     ANTES                      AHORA
//     ┌────────────┐             ┌────────────┐
//     │ ▓▓ CAIDO ▓▓│  <- flota   │            │
//     ├────────────┤             ├────────────┤
//     │            │  <- hueco   │ ▓▓ CAIDO ▓▓│  <- pegado
//     ├─ ─ ─ ─ ─ ─ ┤             ├─ ─ ─ ─ ─ ─ ┤
//     │███ CAPA ███│             │███ CAPA ███│
//     └────────────┘             └────────────┘
//
// El aterrizaje usa ahora el MISMO criterio que colocar a mano:
//
//   - mismo material  -> la capa CRECE (tierra sobre tierra engorda la capa)
//   - material distinto -> celda MIXTA (abajo la capa, arriba lo que cayo)
//
// Aqui se fijan las dos reglas sobre las funciones que las implementan. La
// integracion con el mundo vive en main.cpp y necesita el motor entero, pero
// la aritmetica de niveles -- que es donde estan los errores de un pixel -- se
// comprueba sin arrancar nada.

// ----------------------------------------------------------------------------
// MISMO MATERIAL: LA CAPA CRECE
// ----------------------------------------------------------------------------

TEST_CASE("Aterrizaje: dos capas del mismo material suman su altura") {
    // Una capa de 3 con una de 1 encima tiene que dar una sola capa de 4, no
    // dos capas apiladas con un escalon entre ellas.
    const BlockType capa3 = conNivel(BLOCK_DIRT, 3);
    const BlockType capa1 = conNivel(BLOCK_DIRT, 1);

    REQUIRE(esNivelParcial(capa3));
    REQUIRE(esNivelParcial(capa1));

    const int suma = nivelDe(capa3) + nivelDe(capa1);
    CHECK(suma == 4);

    const BlockType resultado = conNivel(bloqueBaseDe(capa3), suma);
    CHECK(nivelDe(resultado) == 4);
    CHECK(bloqueBaseDe(resultado) == BLOCK_DIRT);
}

TEST_CASE("Aterrizaje: al llegar a 8 la celda queda llena, no parcial") {
    // El tope de una celda son 8 octavos. Al alcanzarlo deja de ser una capa
    // y pasa a ser el bloque entero: si siguiera siendo "capa de 8" el motor
    // la dibujaria con hueco por arriba.
    const BlockType lleno = conNivel(BLOCK_DIRT, 8);
    CHECK_FALSE(esNivelParcial(lleno));
    CHECK(lleno == BLOCK_DIRT);
    CHECK(nivelDe(lleno) == 8);
}

TEST_CASE("Aterrizaje: la suma no puede pasar de 8") {
    // Es la condicion que protege el acoplamiento: si dos capas no caben en
    // una celda, NO se fusionan y el bloque se queda en la celda de arriba.
    // Sin esa guarda, conNivel recortaria la altura y se perderia material.
    const BlockType capa6 = conNivel(BLOCK_DIRT, 6);
    const BlockType capa5 = conNivel(BLOCK_DIRT, 5);
    const int suma = nivelDe(capa6) + nivelDe(capa5);
    CHECK(suma == 11);
    CHECK(suma > 8);          // no cabe: se apila aparte
}

// ----------------------------------------------------------------------------
// MATERIAL DISTINTO: CELDA MIXTA
// ----------------------------------------------------------------------------

TEST_CASE("Aterrizaje: arena sobre una capa de tierra comparte celda") {
    // Los dos materiales conviven en la misma celda: abajo la capa de tierra
    // que ya estaba, y de ahi al techo la arena recien caida. Al empezar
    // EXACTAMENTE donde acaba la otra, no queda hueco.
    const BlockType capaTierra = conNivel(BLOCK_DIRT, 3);
    const BlockType mezcla =
        mixto(bloqueBaseDe(capaTierra), nivelDe(capaTierra), BLOCK_SAND);

    REQUIRE(mezcla != BLOCK_AIR);
    CHECK(esMixto(mezcla));

    // La celda mixta recuerda las dos cosas: la capa de abajo y el relleno.
    CHECK(mixtoRelleno(mezcla) == BLOCK_SAND);
}

TEST_CASE("Aterrizaje: la celda mixta conserva el nivel de la capa de abajo") {
    // Si el nivel se perdiera, la capa de abajo cambiaria de altura al
    // recibir algo encima y el suelo daria un salto a la vista.
    for (int nivel = 1; nivel <= 7; ++nivel) {
        const BlockType mezcla = mixto(BLOCK_DIRT, nivel, BLOCK_SAND);
        if (mezcla == BLOCK_AIR) continue;   // pareja no admitida
        INFO("nivel de la capa ", nivel);
        CHECK(esMixto(mezcla));
        CHECK(mixtoNivel(mezcla) == nivel);
    }
}

// ----------------------------------------------------------------------------
// LO QUE NO DEBE CAMBIAR
// ----------------------------------------------------------------------------

TEST_CASE("Aterrizaje: sobre suelo entero no se acopla nada") {
    // El acoplamiento solo entra cuando debajo hay una CAPA. Sobre un bloque
    // entero no hay hueco que cerrar, asi que lo que cae se queda en su celda
    // como siempre. Esta es la guarda que lo decide.
    CHECK_FALSE(esNivelParcial(BLOCK_DIRT));
    CHECK_FALSE(esNivelParcial(BLOCK_STONE));
    CHECK_FALSE(esNivelParcial(BLOCK_AIR));
}

TEST_CASE("Aterrizaje: un material sin niveles no se acopla") {
    // Las plantas y demas no admiten niveles: conNivel las devuelve tal cual.
    // El acoplamiento tiene que dejarlas en paz en vez de inventarles capas.
    CHECK_FALSE(admiteNiveles(BLOCK_TALLGRASS));
    CHECK(conNivel(BLOCK_TALLGRASS, 3) == BLOCK_TALLGRASS);
}
