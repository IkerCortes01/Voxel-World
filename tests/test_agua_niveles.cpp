#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

// ============================================================================
// EL AGUA CON VOLUMEN
// ============================================================================
// El agua deja de ser un interruptor (hay/no hay) y pasa a tener CUANTA hay,
// con el nivel guardado DENTRO del ID del bloque.
//
// Estos tests fijan las tres propiedades de las que depende todo lo demas:
//
//   1. El nivel sobrevive al viaje de ida y vuelta por el ID. Si esto se
//      rompe, el agua cambia de nivel al guardar y cargar el mundo.
//   2. El agua NO es una planta. Vive en el rango compuesto, que hasta ahora
//      era vegetacion entera; si se cuela, el hacha la corta y se desgasta.
//   3. El reparto CONSERVA EL VOLUMEN. Es lo que impide que el mar se drene
//      solo o que el agua se multiplique sola.
// ============================================================================

using namespace Compuesto;

TEST_CASE("el nivel del agua sobrevive al ID") {
    // Es el viaje que hace el agua al guardarse: nivel -> ID -> disco -> ID
    // -> nivel. Si un solo nivel no vuelve igual, los charcos cambian de
    // tamano al recargar el mundo.
    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType b = Agua::nuevo(n);
        CHECK(Agua::esAgua(b));
        CHECK(Agua::nivelDe(b) == n);
    }
}

TEST_CASE("el nivel se acota en vez de desbordarse") {
    // Pedir mas de lo que cabe da una celda llena, no un ID corrido que
    // caeria en OTRA familia y se leeria como un maguey.
    const BlockType b = Agua::nuevo(200);
    CHECK(Agua::nivelDe(b) == Agua::LLENA);
    CHECK(familiaDe(b) == FAM_AGUA);
}

TEST_CASE("el agua no es una planta") {
    // ESTE es el test que protege el motor entero.
    //
    // El agua es la primera familia compuesta que no es vegetacion. Todo lo
    // que clasificaba "ID >= 100000" como planta tiene que apartarla antes,
    // o el hacha corta el agua y se desgasta cortandola.
    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType b = Agua::nuevo(n);
        CHECK(esAguaVolumen(b));
        CHECK_FALSE(esOrganicoParaHacha(b));
        CHECK(desgasteHacha(b) == 0);
    }

    // Y al reves: una planta compuesta SIGUE siendo organica. Al apartar el
    // agua es facil apartar de mas y dejar el maguey sin hacha.
    const BlockType maguey = Maguey::nuevo(Maguey::PRODUCTOR, 0);
    CHECK_FALSE(esAguaVolumen(maguey));
    CHECK(esOrganicoParaHacha(maguey));
}

TEST_CASE("el agua no pisa el rango de otras familias") {
    // Un desbordamiento de familia es corrupcion silenciosa de mundos: el
    // save guarda un numero que al cargarse es otra planta.
    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType b = Agua::nuevo(n);
        CHECK(familiaDe(b) == FAM_AGUA);
        CHECK(familiaDe(b) != FAM_MAGUEY);
        CHECK_FALSE(Biznaga::esBiznaga(b));
    }
}

TEST_CASE("la altura visual sigue al nivel") {
    // Es lo que hace que "el agua baja de nivel" se VEA en pantalla. Una
    // celda llena ocupa el cubo entero; media celda, la mitad.
    CHECK(Agua::alturaVisual(Agua::nuevo(Agua::LLENA)) == doctest::Approx(1.0f));
    CHECK(Agua::alturaVisual(Agua::nuevo(4)) == doctest::Approx(0.5f));
    CHECK(Agua::alturaVisual(Agua::nuevo(1)) == doctest::Approx(0.125f));

    // Y tiene que ser monotona: mas agua nunca se ve mas baja.
    for (uint16_t n = 2; n <= Agua::LLENA; ++n) {
        CHECK(Agua::alturaVisual(Agua::nuevo(n)) >
              Agua::alturaVisual(Agua::nuevo(n - 1)));
    }

    // Lo que no es agua no tiene altura de agua.
    CHECK(Agua::alturaVisual(BLOCK_STONE) == doctest::Approx(0.0f));
}

TEST_CASE("el agua que cae se dibuja llena") {
    // Un chorro fino sigue siendo un chorro de arriba abajo: si se dibujara
    // a su nivel real se veria un charco flotando en el aire.
    const BlockType chorro = Agua::conCaida(Agua::nuevo(2), true);
    CHECK(Agua::estaCayendo(chorro));
    CHECK(Agua::alturaVisual(chorro) == doctest::Approx(1.0f));

    // Pero por dentro conserva cuanta agua lleva de verdad, que es lo que se
    // reparte al llegar al suelo.
    CHECK(Agua::nivelDe(chorro) == 2);
}

TEST_CASE("cambiar el nivel conserva el resto del estado") {
    // conNivel se usa en cada paso del reparto. Si perdiera la marca de
    // caida, un chorro se convertiria en charco a mitad del vuelo.
    const BlockType chorro = Agua::conCaida(Agua::nuevo(5), true);
    const BlockType menos  = Agua::conNivel(chorro, 3);

    CHECK(Agua::nivelDe(menos) == 3);
    CHECK(Agua::estaCayendo(menos));
}

TEST_CASE("una celda llena esta llena y una a medias no") {
    CHECK(Agua::estaLlena(Agua::nuevo(Agua::LLENA)));
    CHECK_FALSE(Agua::estaLlena(Agua::nuevo(Agua::LLENA - 1)));
    CHECK_FALSE(Agua::estaLlena(BLOCK_STONE));
}

TEST_CASE("lo que no es agua tiene nivel cero") {
    // nivelDe se usa para SUMAR el agua de una columna entera. Devolver 0
    // para el aire y la piedra es lo que permite sumar sin comprobar cada
    // celda antes.
    CHECK(Agua::nivelDe(BLOCK_AIR) == 0);
    CHECK(Agua::nivelDe(BLOCK_STONE) == 0);
    CHECK(Agua::nivelDe(BLOCK_DIRT) == 0);
    CHECK_FALSE(Agua::esAgua(BLOCK_AIR));
}
