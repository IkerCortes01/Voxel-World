#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

// ============================================================================
// LAS PUNTAS DEL MAGUEY
// ============================================================================
// La punta gruesa es lo que distingue a un maguey HECHO. Un brote no la
// tiene: si la tuviera, seria un maguey adulto en miniatura y se perderia
// la pista visual que permite reconocerlos de lejos.

using namespace Compuesto;

TEST_CASE("Maguey: los pequenos NO tienen punta") {
    CHECK(Maguey::puntasDeEtapa(Maguey::BROTE) == 0);
    CHECK(Maguey::puntasDeEtapa(Maguey::JOVEN) == 0);
}

TEST_CASE("Maguey: los grandes SI tienen punta") {
    CHECK(Maguey::puntasDeEtapa(Maguey::ADULTO) > 0);
    CHECK(Maguey::puntasDeEtapa(Maguey::MADURO) > 0);
    CHECK(Maguey::puntasDeEtapa(Maguey::PRODUCTOR) > 0);
}

TEST_CASE("Maguey: la punta llega con la edad, y va a mas") {
    // Ni una en las dos primeras etapas, y de ahi en aumento.
    CHECK(Maguey::puntasDeEtapa(Maguey::ADULTO) <=
          Maguey::puntasDeEtapa(Maguey::MADURO));
    CHECK(Maguey::puntasDeEtapa(Maguey::MADURO) <=
          Maguey::puntasDeEtapa(Maguey::PRODUCTOR));
}

TEST_CASE("Maguey: un brote recien generado sale sin puntas") {
    const BlockType brote = Maguey::nuevo(Maguey::BROTE, 0);
    CHECK(Maguey::puntasDe(brote) == 0);

    const BlockType joven = Maguey::nuevo(Maguey::JOVEN, 0);
    CHECK(Maguey::puntasDe(joven) == 0);
}

TEST_CASE("Maguey: al crecer a adulto le SALEN las puntas") {
    // Un brote sin puntas que crece hasta adulto tiene que ganarlas: si no,
    // un maguey que nace pequeno no tendria punta nunca.
    BlockType m = Maguey::nuevo(Maguey::BROTE, 0);
    CHECK(Maguey::puntasDe(m) == 0);

    m = Maguey::conEtapa(m, Maguey::ADULTO);
    CHECK(Maguey::puntasDe(m) == Maguey::puntasDeEtapa(Maguey::ADULTO));
    CHECK(Maguey::puntasDe(m) > 0);
}

TEST_CASE("Maguey: el maduro sale con sus puntas de fabrica") {
    const BlockType maduro = Maguey::nuevo(Maguey::MADURO, 0);
    CHECK(Maguey::puntasDe(maduro) == Maguey::puntasDeEtapa(Maguey::MADURO));
}

TEST_CASE("Maguey: caparlo le quita una punta") {
    BlockType m = Maguey::nuevo(Maguey::PRODUCTOR, 0);
    const uint16_t antes = Maguey::puntasDe(m);

    m = Maguey::capar(m);
    CHECK(Maguey::capadoDe(m));
    CHECK(Maguey::puntasDe(m) == antes - 1);
}

TEST_CASE("Maguey: capar un brote sin puntas no baja de cero") {
    // El brote no tiene puntas: capar no puede dejarle un numero negativo,
    // que en un campo sin signo daria un valor enorme.
    BlockType m = Maguey::nuevo(Maguey::BROTE, 0);
    m = Maguey::capar(m);
    CHECK(Maguey::puntasDe(m) == 0);
}
