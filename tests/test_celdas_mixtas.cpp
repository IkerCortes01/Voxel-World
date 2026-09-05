#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// TESTS DE LAS CELDAS MIXTAS
// ============================================================================
// La regla que fijan: una celda mixta NUNCA deja hueco. Abajo una capa con su
// nivel, y de ahí al techo el relleno — sin aire en medio y sin nada flotando.
//
// El caso que motivó estos tests: los TRONCOS y TABLONES no tienen niveles
// propios (un tronco en lonchas de 3 px es irreconocible), y por eso no podían
// ser el relleno de una celda. Al poner un tronco sobre una capa de tierra de
// 5 px, el tronco se iba al voxel de arriba: 11 px de aire a la vista y un
// tronco flotando.
//
// El arreglo separa dos conceptos que estaban mezclados:
//   - la capa de ABAJO necesita niveles (su altura ES su nivel)
//   - el material de ARRIBA no, porque siempre llega hasta el techo
//
// ⚠️ COMPATIBILIDAD DE SAVES. El índice del relleno viaja DENTRO del ID mixto,
// que es lo que hay escrito en los mundos guardados. Si alguien reordena
// tablaRelleno() o inserta en medio, las celdas mixtas ya guardadas pasarían a
// leerse como otro material. Los tests de abajo fijan las posiciones exactas
// para que ese error no pueda colarse en silencio.

// ----------------------------------------------------------------------------
// LO QUE MOTIVÓ EL ARREGLO: LA MADERA COMO RELLENO
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: un tronco puede rellenar una celda sin tener niveles") {
    // La madera sigue SIN niveles propios: eso es deliberado y no cambia.
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD));
    CHECK_FALSE(admiteNiveles(BLOCK_PLANKS));

    // Pero AHORA sí puede ser el relleno de arriba.
    const BlockType celda = mixto(BLOCK_DIRT, 3, BLOCK_WOOD);
    REQUIRE(celda != BLOCK_AIR);          // antes daba BLOCK_AIR: el bug
    CHECK(esMixto(celda));

    // Y se descodifica entero: tronco, no una loncha.
    CHECK(mixtoBase(celda) == BLOCK_DIRT);
    CHECK(mixtoNivel(celda) == 3);
    CHECK(mixtoRelleno(celda) == BLOCK_WOOD);
}

TEST_CASE("Mixta: las seis maderas rellenan") {
    const BlockType maderas[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL,
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL,
    };
    for (BlockType m : maderas) {
        const BlockType celda = mixto(BLOCK_STONE, 4, m);
        REQUIRE(celda != BLOCK_AIR);
        CHECK(mixtoRelleno(celda) == m);
        CHECK(mixtoBase(celda) == BLOCK_STONE);
        CHECK(mixtoNivel(celda) == 4);
    }
}

// ----------------------------------------------------------------------------
// LA INVARIANTE DE FONDO: NI HUECOS NI BLOQUES FLOTANDO
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: la celda esta LLENA, sin hueco a ninguna altura") {
    // Una celda mixta ocupa su voxel entero: se anda por encima como por un
    // bloque normal. Si esto dejara de valer, habría un escalón invisible.
    for (int nivel = 1; nivel <= 7; ++nivel) {
        const BlockType celda = mixto(BLOCK_DIRT, nivel, BLOCK_WOOD);
        REQUIRE(celda != BLOCK_AIR);
        CHECK(alturaDe(celda) == doctest::Approx(1.0f));

        // El relleno arranca EXACTAMENTE donde acaba la capa de abajo.
        // Ese "exactamente" es lo que impide que quede aire en medio.
        const float finCapa = alturaBaseMixto(celda);
        const float altoNivel = (float)alturaNivelPx(nivel) / 16.0f;
        CHECK(finCapa == doctest::Approx(altoNivel));

        // Y de ahí al techo hay relleno de verdad, no aire.
        CHECK(finCapa < 1.0f);
        CHECK(mixtoRelleno(celda) != BLOCK_AIR);
    }
}

TEST_CASE("Mixta: todo nivel de toda familia admite relleno de madera") {
    // Barrido completo: ninguna combinación puede quedarse sin representar,
    // porque la que se quedara fuera volvería a producir un bloque flotando.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    for (int i = 0; i < nNiv; ++i) {
        for (int nivel = 1; nivel <= 7; ++nivel) {
            const BlockType celda = mixto(T[i].entero, nivel, BLOCK_WOOD);
            REQUIRE_MESSAGE(celda != BLOCK_AIR,
                            "familia ", i, " nivel ", nivel);
            CHECK(mixtoBase(celda) == T[i].entero);
            CHECK(mixtoNivel(celda) == nivel);
            CHECK(mixtoRelleno(celda) == BLOCK_WOOD);
        }
    }
}

// ----------------------------------------------------------------------------
// COMPATIBILIDAD: LOS MUNDOS YA GUARDADOS SE SIGUEN LEYENDO IGUAL
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: las 16 familias con niveles conservan su indice") {
    // El índice del relleno está escrito dentro del ID de cada celda mixta ya
    // guardada. Las 16 primeras posiciones de tablaRelleno() TIENEN que ser
    // las mismas que las de tablaNiveles(), y en el mismo orden, o los mundos
    // existentes leerían otro material.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    REQUIRE(nNiv == 16);

    for (int i = 0; i < nNiv; ++i) {
        CHECK_MESSAGE(indiceRelleno(T[i].entero) == i,
                      "la familia ", i, " cambio de indice: rompe saves");
    }
}

TEST_CASE("Mixta: un ID guardado con el orden viejo se lee igual") {
    // Se reconstruye a mano un ID tal y como lo escribía el código anterior
    // (tierra nivel 3 + arena de relleno) y se comprueba que hoy significa
    // exactamente lo mismo. Es la prueba directa de que no se rompió nada.
    const int ib = indiceFamilia(BLOCK_DIRT);
    const int ir = 3;                       // arena: 4ª de la tabla, índice 3
    REQUIRE(ib >= 0);
    REQUIRE(indiceFamilia(BLOCK_SAND) == ir);

    const BlockType guardado = (BlockType)(BLOCK_MIXTO_BASE +
                               (ib * 7 + (3 - 1)) * FAMILIAS_CON_NIVEL + ir);

    CHECK(esMixto(guardado));
    CHECK(mixtoBase(guardado) == BLOCK_DIRT);
    CHECK(mixtoNivel(guardado) == 3);
    CHECK(mixtoRelleno(guardado) == BLOCK_SAND);

    // Y es el mismo que produce el código de hoy.
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_SAND) == guardado);
}

TEST_CASE("Mixta: las tablas caben en el hueco reservado") {
    // Si una tabla creciera por encima de FAMILIAS_CON_NIVEL, dos parejas
    // distintas de materiales darían el MISMO ID y los mundos se corromperían.
    CHECK(familiasCaben());

    int nRel = 0; tablaRelleno(nRel);
    CHECK(nRel <= FAMILIAS_CON_NIVEL);

    // Hoy la tabla de relleno está justo llena (16 + 6 maderas = 22). Este
    // check es el aviso: para añadir otro relleno hay que ampliar el hueco,
    // y eso SÍ exige migrar los saves.
    CHECK(nRel == 22);
}

TEST_CASE("Mixta: todo ID generado cae dentro del rango reservado") {
    // Un ID que se saliera del rango dejaría de reconocerse como mixto y la
    // celda se leería como un bloque cualquiera.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    int nRel = 0; const BlockType* R = tablaRelleno(nRel);

    for (int i = 0; i < nNiv; ++i)
        for (int nivel = 1; nivel <= 7; ++nivel)
            for (int j = 0; j < nRel; ++j) {
                const BlockType celda = mixto(T[i].entero, nivel, R[j]);
                REQUIRE(celda != BLOCK_AIR);
                CHECK((int)celda >= BLOCK_MIXTO_BASE);
                CHECK((int)celda < BLOCK_MIXTO_FIN);
                CHECK(esMixto(celda));
            }
}

TEST_CASE("Mixta: no hay dos combinaciones con el mismo ID") {
    // Colisión de IDs = dos materiales distintos que se leen como el mismo.
    // Se recorre el producto completo y se comprueba que la ida y la vuelta
    // coinciden siempre, que es lo mismo que decir que la codificación es
    // inyectiva.
    int nNiv = 0; const FamiliaNivel* T = tablaNiveles(nNiv);
    int nRel = 0; const BlockType* R = tablaRelleno(nRel);

    for (int i = 0; i < nNiv; ++i)
        for (int nivel = 1; nivel <= 7; ++nivel)
            for (int j = 0; j < nRel; ++j) {
                const BlockType celda = mixto(T[i].entero, nivel, R[j]);
                REQUIRE(celda != BLOCK_AIR);
                CHECK(mixtoBase(celda)    == T[i].entero);
                CHECK(mixtoNivel(celda)   == nivel);
                CHECK(mixtoRelleno(celda) == R[j]);
            }
}

// ----------------------------------------------------------------------------
// LO QUE NO DEBE CAMBIAR
// ----------------------------------------------------------------------------

TEST_CASE("Mixta: la madera sigue sin partirse en lonchas") {
    // El motivo por el que la madera salió de tablaNiveles() sigue vigente:
    // un tronco es un tronco entero. Poder RELLENAR no es tener niveles.
    CHECK(primerNivelDe(BLOCK_WOOD) == BLOCK_AIR);
    CHECK(primerNivelDe(BLOCK_PLANKS) == BLOCK_AIR);
    CHECK(indiceFamilia(BLOCK_WOOD) == -1);   // no puede ser la capa de abajo
    CHECK(indiceRelleno(BLOCK_WOOD) >= 0);    // pero sí el relleno de arriba

    // conNivel sobre madera devuelve el bloque entero, nunca una loncha.
    CHECK(conNivel(BLOCK_WOOD, 1) == BLOCK_WOOD);
    CHECK(conNivel(BLOCK_WOOD, 4) == BLOCK_WOOD);
    CHECK(alturaDe(BLOCK_WOOD) == doctest::Approx(1.0f));
}

TEST_CASE("Mixta: lo que no puede rellenar sigue sin poder") {
    // Plantas, agua y lava no rellenan: no son macizos. Devolver BLOCK_AIR es
    // la señal de "esta pareja no se representa", y el código que coloca la
    // usa para caer a su camino alternativo.
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_WATER)     == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_LAVA)      == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 3, BLOCK_TALLGRASS) == BLOCK_AIR);

    // Y la capa de abajo sigue exigiendo niveles: un tronco no puede serlo.
    CHECK(mixto(BLOCK_WOOD, 3, BLOCK_DIRT) == BLOCK_AIR);

    // El nivel 8 es la celda llena, no una mixta.
    CHECK(mixto(BLOCK_DIRT, 8, BLOCK_WOOD) == BLOCK_AIR);
    CHECK(mixto(BLOCK_DIRT, 0, BLOCK_WOOD) == BLOCK_AIR);
}
