#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

using namespace Compuesto;
namespace M = Compuesto::Maguey;

// ============================================================================
// TESTS DEL CICLO DE VIDA DEL MAGUEY
// ============================================================================
// La Fase 1 fijo el empaquetado de bits. Esto fija las REGLAS DE JUEGO que se
// montan encima: el recorrido completo de la planta, tal y como lo vive el
// jugador.
//
//   brote -> crece -> maduro -> se capa -> produce poco a poco ->
//   se recoge con el tazon -> vuelve a producir
//
// Lo que se comprueba aqui no es que los bits esten en su sitio (eso ya lo
// hace test_compuesto), sino que la SECUENCIA no tenga agujeros: que no se
// pueda sacar jugo antes de tiempo, que recoger no rompa la planta, y que el
// ciclo se pueda repetir indefinidamente.

// ----------------------------------------------------------------------------
// EL RECORRIDO COMPLETO
// ----------------------------------------------------------------------------

TEST_CASE("Ciclo: el recorrido entero de una planta, de brote a tazon") {
    // 1. Nace como brote: no da nada y no se puede capar con provecho.
    BlockType p = M::nuevo(M::BROTE, 0);
    CHECK(M::etapaDe(p) == M::BROTE);
    CHECK(M::capacidad(M::etapaDe(p)) == 0);
    CHECK_FALSE(M::produce(estadoDe(p)));

    // 2. Crece hasta productor.
    p = M::conEtapa(p, M::JOVEN);
    p = M::conEtapa(p, M::ADULTO);
    p = M::conEtapa(p, M::MADURO);
    p = M::conEtapa(p, M::PRODUCTOR);
    CHECK(M::etapaDe(p) == M::PRODUCTOR);

    // 3. Aun sin capar NO produce, por muy grande que sea.
    CHECK_FALSE(M::produce(estadoDe(p)));

    // 4. Se capa: ahora si.
    p = M::capar(p);
    CHECK(M::capadoDe(p));
    CHECK(M::produce(estadoDe(p)));

    // 5. Se llena POCO A POCO, un punto por tick.
    const uint16_t tope = M::capacidad(M::etapaDe(p));
    for (uint16_t i = 1; i <= tope; ++i) {
        p = M::conAguamiel(p, i);
        CHECK(M::aguamielDe(p) == i);
    }
    CHECK(M::aguamielDe(p) == tope);

    // 6. Hay para un tazon.
    CHECK(M::hayParaTazon(estadoDe(p)));

    // 7. Se recoge: la planta se vacia PERO SIGUE VIVA y capada.
    p = M::vaciado(p);
    CHECK(M::aguamielDe(p) == 0);
    CHECK(M::capadoDe(p));
    CHECK(M::etapaDe(p) == M::PRODUCTOR);

    // 8. Y vuelve a producir: el ciclo se repite.
    CHECK(M::produce(estadoDe(p)));
}

TEST_CASE("Ciclo: se puede ordeñar muchas veces seguidas") {
    // Un maguey capado no se agota: es una fuente renovable. Si tras N
    // recogidas dejara de producir, el jugador se quedaria sin entender por
    // que.
    BlockType p = M::capar(M::nuevo(M::PRODUCTOR, 0));

    for (int vuelta = 0; vuelta < 20; ++vuelta) {
        INFO("vuelta ", vuelta);
        // Se llena
        p = M::conAguamiel(p, M::capacidad(M::etapaDe(p)));
        REQUIRE(M::hayParaTazon(estadoDe(p)));
        // Se recoge
        p = M::vaciado(p);
        CHECK(M::aguamielDe(p) == 0);
        // Y sigue en condiciones de volver a llenarse
        CHECK(M::produce(estadoDe(p)));
    }
}

// ----------------------------------------------------------------------------
// LO QUE NO SE PUEDE HACER
// ----------------------------------------------------------------------------

TEST_CASE("Ciclo: un maguey joven no da jugo aunque se cape") {
    // Capar no es un atajo para saltarse el crecimiento. Las etapas jovenes
    // tienen capacidad 0, asi que aunque el jugador las abra no acumulan.
    const uint16_t JOVENES[] = { M::BROTE, M::JOVEN, M::ADULTO };
    for (uint16_t etapa : JOVENES) {
        const BlockType p = M::capar(M::nuevo(etapa, 0));
        INFO("etapa ", etapa);
        CHECK(M::capadoDe(p));                     // si, esta abierto
        CHECK(M::capacidad(M::etapaDe(p)) == 0);   // pero no cabe nada
        CHECK_FALSE(M::produce(estadoDe(p)));      // asi que no produce
    }
}

TEST_CASE("Ciclo: el jugo nunca pasa de la capacidad de su etapa") {
    // Un maduro cabe menos que un productor. Pedirle mas no debe desbordar al
    // campo de al lado ni darle una cosecha que no le toca.
    const uint16_t MAYORES[] = { M::MADURO, M::PRODUCTOR };
    for (uint16_t etapa : MAYORES) {
        const uint16_t tope = M::capacidad(etapa);
        BlockType p = M::capar(M::nuevo(etapa, 0));

        // Se intenta meter mucho mas de lo que cabe.
        p = M::conAguamiel(p, 15);
        INFO("etapa ", etapa, " tope ", tope);
        CHECK(M::aguamielDe(p) <= M::AGUAMIEL.maximo());

        // Y el resto del estado sigue intacto.
        CHECK(M::etapaDe(p) == etapa);
        CHECK(M::capadoDe(p));
    }
}

TEST_CASE("Ciclo: recoger con poco jugo no vacia la planta") {
    // La regla de "hace falta un minimo": si el jugador pudiera recoger con
    // un dedo de jugo, el tazon saldria medio vacio y el maguey volveria a
    // cero. hayParaTazon es lo que lo impide.
    BlockType p = M::capar(M::nuevo(M::PRODUCTOR, 0));

    for (uint16_t v = 0; v < M::AGUAMIEL_PARA_TAZON; ++v) {
        p = M::conAguamiel(p, v);
        INFO("con ", v, " de jugo");
        CHECK_FALSE(M::hayParaTazon(estadoDe(p)));
    }
    // Justo en el umbral, si.
    p = M::conAguamiel(p, M::AGUAMIEL_PARA_TAZON);
    CHECK(M::hayParaTazon(estadoDe(p)));
}

// ----------------------------------------------------------------------------
// EL ESTADO SOBREVIVE A TODO
// ----------------------------------------------------------------------------

TEST_CASE("Ciclo: cada operacion conserva lo que no toca") {
    // El estado va empaquetado en un solo entero, asi que una operacion mal
    // hecha puede llevarse por delante campos que no le incumben. Se
    // comprueba operacion a operacion.
    const BlockType base = M::crear(M::MADURO, 2, 5, 4, true);

    SUBCASE("cambiar el jugo") {
        const BlockType r = M::conAguamiel(base, 7);
        CHECK(M::aguamielDe(r) == 7);
        CHECK(M::etapaDe(r)  == M::MADURO);
        CHECK(M::giroDe(r)   == 2);
        CHECK(M::puntasDe(r) == 5);
        CHECK(M::capadoDe(r));
    }
    SUBCASE("vaciar") {
        const BlockType r = M::vaciado(base);
        CHECK(M::aguamielDe(r) == 0);
        CHECK(M::etapaDe(r)  == M::MADURO);
        CHECK(M::giroDe(r)   == 2);
        CHECK(M::puntasDe(r) == 5);
        CHECK(M::capadoDe(r));
    }
    SUBCASE("quitar una punta") {
        const BlockType r = M::conPuntas(base, 3);
        CHECK(M::puntasDe(r) == 3);
        CHECK(M::etapaDe(r)    == M::MADURO);
        CHECK(M::giroDe(r)     == 2);
        CHECK(M::aguamielDe(r) == 4);
        CHECK(M::capadoDe(r));
    }
    SUBCASE("crecer") {
        const BlockType r = M::conEtapa(base, M::PRODUCTOR);
        CHECK(M::etapaDe(r) == M::PRODUCTOR);
        CHECK(M::giroDe(r)     == 2);
        CHECK(M::aguamielDe(r) == 4);   // no pierde lo que ya tenia
        CHECK(M::capadoDe(r));
    }
}

TEST_CASE("Ciclo: el giro no cambia nunca solo") {
    // El giro define como se dibuja la planta. Si alguna operacion lo tocara,
    // el maguey giraria de golpe al llenarse de jugo o al perder una punta.
    for (uint16_t g = 0; g <= M::GIRO.maximo(); ++g) {
        BlockType p = M::nuevo(M::PRODUCTOR, g);
        REQUIRE(M::giroDe(p) == g);

        p = M::capar(p);                CHECK(M::giroDe(p) == g);
        p = M::conAguamiel(p, 9);       CHECK(M::giroDe(p) == g);
        p = M::vaciado(p);              CHECK(M::giroDe(p) == g);
        p = M::conPuntas(p, 2);         CHECK(M::giroDe(p) == g);
        p = M::conEtapa(p, M::MADURO);  CHECK(M::giroDe(p) == g);
    }
}

// ----------------------------------------------------------------------------
// LOS COMPONENTES SIGUEN AL ESTADO
// ----------------------------------------------------------------------------

TEST_CASE("Ciclo: los componentes aparecen y desaparecen con la planta") {
    // Es lo que hace que el raycast no pruebe cajas de partes que no estan.
    BlockType p = M::crear(M::PRODUCTOR, 0, 0, 0, false);
    CHECK(componentesDe(p) == 1);            // solo cuerpo

    p = M::conPuntas(p, 4);
    CHECK(componentesDe(p) == 2);            // + puntas

    p = M::capar(p);
    p = M::conAguamiel(p, 10);
    CHECK(componentesDe(p) == 3);            // + jugo

    // Al recogerlo, el fluido desaparece de los componentes.
    p = M::vaciado(p);
    CHECK(componentesDe(p) == 2);

    // Y si se arrancan todas las puntas, se queda solo el cuerpo.
    p = M::conPuntas(p, 0);
    CHECK(componentesDe(p) == 1);
}

TEST_CASE("Ciclo: arrancar puntas una a una no rompe la planta") {
    // Cada golpe a una punta se lleva UNA; la planta sigue en pie hasta que
    // se golpea el cuerpo.
    BlockType p = M::nuevo(M::PRODUCTOR, 0);
    uint16_t n = M::puntasDe(p);
    REQUIRE(n > 0);

    while (n > 0) {
        p = M::conPuntas(p, (uint16_t)(n - 1));
        --n;
        CHECK(M::puntasDe(p) == n);
        // La planta conserva su etapa: sigue siendo el mismo maguey.
        CHECK(M::etapaDe(p) == M::PRODUCTOR);
    }
    CHECK(componentesDe(p) == 1);   // ya solo queda el cuerpo
}

// ----------------------------------------------------------------------------
// GENERACION PROCEDURAL
// ----------------------------------------------------------------------------

TEST_CASE("Generacion: las cinco etapas producen plantas validas") {
    // El generador reparte etapas por hash. Todas tienen que dar un bloque
    // coherente: nada de puntas de mas ni estados imposibles.
    for (uint16_t etapa = 0; etapa <= M::PRODUCTOR; ++etapa)
        for (uint16_t giro = 0; giro <= M::GIRO.maximo(); ++giro) {
            const BlockType p = M::nuevo(etapa, giro);
            INFO("etapa ", etapa, " giro ", giro);

            REQUIRE(esCompuesto(p));
            CHECK(familiaDe(p) == FAM_MAGUEY);
            CHECK(M::etapaDe(p) == etapa);
            CHECK(M::giroDe(p)  == giro);
            // Recien nacida: sin capar y sin jugo.
            CHECK(M::aguamielDe(p) == 0);
            CHECK_FALSE(M::capadoDe(p));
            // Con las puntas que le tocan, dentro del campo.
            CHECK(M::puntasDe(p) <= M::PUNTAS.maximo());
        }
}

TEST_CASE("Generacion: las plantas mayores tienen mas puntas y son mas grandes") {
    // Es la pista visual que permite al jugador elegir a cual acercarse.
    for (uint16_t e = 1; e <= M::PRODUCTOR; ++e) {
        INFO("etapa ", e);
        CHECK(M::puntasDeEtapa(e)  >= M::puntasDeEtapa((uint16_t)(e - 1)));
        CHECK(M::escalaDeEtapa(e)  >  M::escalaDeEtapa((uint16_t)(e - 1)));
    }
}
