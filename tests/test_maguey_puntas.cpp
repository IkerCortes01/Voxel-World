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

// ============================================================================
// LA BIZNAGA
// ============================================================================
// Cactus de barril del desierto. Crece en tres etapas, cada 48 minutos.

TEST_CASE("Biznaga: se reconoce y no se confunde con el maguey") {
    const BlockType b = Biznaga::nuevo(Biznaga::PEQUENA, 0, 3);
    CHECK(Biznaga::esBiznaga(b));

    // Un maguey NO es una biznaga, aunque ambos sean compuestos.
    const BlockType m = Maguey::nuevo(Maguey::ADULTO, 0);
    CHECK_FALSE(Biznaga::esBiznaga(m));

    // Y un bloque normal tampoco.
    CHECK_FALSE(Biznaga::esBiznaga(BLOCK_STONE));
    CHECK_FALSE(Biznaga::esBiznaga(BLOCK_SAND));
}

TEST_CASE("Biznaga: crece de pequena a adulta, y ahi se para") {
    BlockType b = Biznaga::nuevo(Biznaga::PEQUENA, 0, 3);
    CHECK(Biznaga::etapaDe(b) == Biznaga::PEQUENA);
    CHECK_FALSE(Biznaga::estaHecha(b));

    b = Biznaga::crecida(b);
    CHECK(Biznaga::etapaDe(b) == Biznaga::MEDIANA);
    CHECK_FALSE(Biznaga::estaHecha(b));

    b = Biznaga::crecida(b);
    CHECK(Biznaga::etapaDe(b) == Biznaga::ADULTA);
    CHECK(Biznaga::estaHecha(b));

    // Ya no crece mas: una adulta se queda como esta.
    const BlockType antes = b;
    b = Biznaga::crecida(b);
    CHECK(b == antes);
}

TEST_CASE("Biznaga: al crecer conserva su giro y sus costillas") {
    // El numero de costillas se fija al brotar y no cambia en toda su vida,
    // como en una biznaga de verdad. Si se perdiera al crecer, la planta
    // cambiaria de forma de golpe.
    BlockType b = Biznaga::nuevo(Biznaga::PEQUENA, 2, 5);
    const uint16_t giro = Biznaga::giroDe(b);
    const int cost = Biznaga::costillasReales(b);

    b = Biznaga::crecida(b);
    CHECK(Biznaga::giroDe(b) == giro);
    CHECK(Biznaga::costillasReales(b) == cost);
}

TEST_CASE("Biznaga: cada etapa es mas grande que la anterior") {
    CHECK(Biznaga::escalaDeEtapa(Biznaga::PEQUENA) <
          Biznaga::escalaDeEtapa(Biznaga::MEDIANA));
    CHECK(Biznaga::escalaDeEtapa(Biznaga::MEDIANA) <
          Biznaga::escalaDeEtapa(Biznaga::ADULTA));

    // Y ninguna se sale del voxel.
    CHECK(Biznaga::escalaDeEtapa(Biznaga::ADULTA) <= 1.0f);
}

TEST_CASE("Biznaga: las costillas caen en el rango real de la especie") {
    // Un Ferocactus tiene entre 13 y 21 costillas. El campo son 3 bits, asi
    // que se mapean a 13..20.
    for (uint16_t v = 0; v < 8; ++v) {
        const int c = Biznaga::costillasDe(v);
        CHECK(c >= 13);
        CHECK(c <= 20);
    }
}

TEST_CASE("Biznaga: su ID no pisa al del maguey") {
    // Cada familia tiene su bloque de IDs. Si se solaparan, una biznaga se
    // leeria como un maguey al cargar el mundo.
    const BlockType b = Biznaga::nuevo(Biznaga::ADULTA, 3, 7);
    const BlockType m = Maguey::nuevo(Maguey::PRODUCTOR, 3);

    CHECK(familiaDe(b) == FAM_BIZNAGA);
    CHECK(familiaDe(m) == FAM_MAGUEY);
    CHECK(b != m);
}
