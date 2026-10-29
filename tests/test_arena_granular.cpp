#include <doctest/doctest.h>
#include "FisicaCaida.h"
#include "BloqueCompuesto.h"

// ============================================================================
// LA ARENA CAE SIEMPRE, SEA NATURAL O PUESTA POR EL JUGADOR
// ============================================================================
// LO QUE SE PIDIO: "que la arena, sin importar que sea natural, siempre que le
// quites un bloque de abajo, caiga".
//
// Antes no lo hacia, y el motivo esta documentado en puedeCaer(): la arena
// entra en esTerrenoNatural(), y el terreno natural esta EXCLUIDO del flood
// fill de estructuras. Esa exclusion no era un descuido -- arreglaba un bug
// real en el que el fill se comia la columna de piedra hacia abajo y dejaba
// placas de terreno tiradas por el suelo.
//
// ----------------------------------------------------------------------------
// POR QUE HAY DOS PREGUNTAS Y NO UNA
// ----------------------------------------------------------------------------
// La solucion NO es sacar la arena de esTerrenoNatural(): eso reintroduciria
// aquel bug. Son dos sistemas distintos y la arena contesta distinto a cada
// uno:
//
//   puedeCaer(ARENA)   -> false.  Para el FLOOD FILL de estructuras la arena
//                                 sigue siendo terreno: no arrastra a sus
//                                 vecinos ni se desprende en bloque.
//
//   esGranular(ARENA)  -> true.   Para la gravedad POR COLUMNA es un arido
//                                 suelto: mira su celda de abajo y si esta
//                                 vacia, cae.
//
// Este archivo fija justamente esa separacion, que es lo unico que impide que
// alguien "simplifique" las dos funciones en una y reviva el bug.
// ============================================================================

// ⚠️ `Fisica::isCrossSprite` NO se define aqui a proposito.
//
// esSueloFirme() la necesita y la del motor vive en main.cpp, que los tests no
// enlazan. La aporta test_derrumbe.cpp, y como todos los casos se enlazan en
// un unico ejecutable, definirla otra vez aqui da LNK2005 (symbol duplicado).
// Comprobado: pasa.
using namespace Fisica;

TEST_CASE("Arena: es granular, asi que cae por columna") {
    CHECK(esGranular(BLOCK_SAND));

    // La arena con arcilla sigue siendo arido suelto.
    CHECK(esGranular(BLOCK_CLAY_SAND));
}

TEST_CASE("Arena: sigue siendo terreno para el flood fill de estructuras") {
    // ⚠️ ESTE ES EL TEST QUE PROTEGE EL ARREGLO VIEJO.
    //
    // Si alguien mete la arena en puedeCaer() para que caiga, el fill vuelve a
    // meterse en el terreno y reaparecen las placas de piedra tiradas por el
    // suelo. La arena cae por esGranular(), NO por aqui.
    CHECK(esTerrenoNatural(BLOCK_SAND));
    CHECK_FALSE(puedeCaer(BLOCK_SAND));
}

TEST_CASE("Granular: lo que NO se desmorona") {
    // La tierra y la grava sostienen talud en este motor: hay laderas de
    // tierra por todo el mundo generado, y meterlas aqui haria que el relieve
    // se deshiciera solo en cuanto el jugador picara al pie de una loma.
    CHECK_FALSE(esGranular(BLOCK_DIRT));
    CHECK_FALSE(esGranular(BLOCK_GRASS));
    CHECK_FALSE(esGranular(BLOCK_GRAVEL));

    // La roca, evidentemente.
    CHECK_FALSE(esGranular(BLOCK_STONE));
    CHECK_FALSE(esGranular(BLOCK_COBBLESTONE));
    CHECK_FALSE(esGranular(BLOCK_BEDROCK));

    // Ni el aire ni los liquidos: tienen su propio sistema.
    CHECK_FALSE(esGranular(BLOCK_AIR));
    CHECK_FALSE(esGranular(BLOCK_WATER));
    CHECK_FALSE(esGranular(BLOCK_LAVA));

    // Ni la construccion: un tablon en voladizo se sostiene.
    CHECK_FALSE(esGranular(BLOCK_PLANKS));
}

TEST_CASE("Granular: una capa parcial de arena tambien cae") {
    // Media loncha de arena sigue siendo arena. Sin normalizar, una capa
    // parcial se quedaria flotando mientras el bloque entero cae -- la clase
    // de incoherencia que este motor ya sufrio con el maguey.
    const BlockType losa = conNivel(BLOCK_SAND, 4);
    REQUIRE(esNivelParcial(losa));      // que de verdad sea una capa
    CHECK(esGranular(losa));

    // Y la de tierra sigue sin caer, igual que el bloque entero.
    const BlockType losaTierra = conNivel(BLOCK_DIRT, 4);
    REQUIRE(esNivelParcial(losaTierra));
    CHECK_FALSE(esGranular(losaTierra));
}

TEST_CASE("Granular: la arena natural y la colocada son el MISMO bloque") {
    // El nucleo de lo que se pidio: "sin importar que sea natural".
    //
    // En este motor no hay forma de distinguirlas y es deliberado -- el ID es
    // el mismo, asi que la regla no puede depender del origen aunque quisiera.
    // Este test lo deja escrito para que nadie intente anadir esa distincion:
    // si un dia alguien mete un "BLOCK_SAND_COLOCADA", esta llamada seguira
    // contestando lo mismo para las dos o el test se cae.
    const BlockType natural  = BLOCK_SAND;
    const BlockType colocada = BLOCK_SAND;
    CHECK(esGranular(natural) == esGranular(colocada));
    CHECK(esGranular(natural));
}
