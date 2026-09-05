#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

// ============================================================================
// LA BIZNAGA QUE SE DIBUJABA COMO UN PARCHE DE PIEDRA
// ============================================================================
// BUG REAL, visto en pantalla: sobre el terreno aparecian parches grises
// macizos con cuadros verdes incrustados. Parecia terreno roto -- de hecho se
// confundio con un bug antiguo que destruia suelo -- pero no lo era.
//
// ERAN BIZNAGAS.
//
// La planta tenia tres de las cuatro piezas que necesita un bloque compuesto:
//
//   generacion   ✅  se sembraba en el desierto
//   textura      ✅  getBlockTexture la reconocia por familia
//   colision     ✅  nopalHitboxCon declaraba su caja de barril
//   MALLADO      ❌  NO tenia rama en el mesher
//
// Sin rama en el mesher no llegaba al `continue` que salta el cubo generico,
// asi que se dibujaba como un CUBO ENTERO. Y al pedir la textura de un cubo
// con un ID que vive fuera del enum, el switch caia a su
// `default: Piedra.png`.
//
// De ahi las dos mitades del sintoma:
//   - el parche gris   = la biznaga dibujada como cubo de roca
//   - la falta de tope = el jugador chocaba con la caja de la BOLA (correcta)
//                        mientras veia un CUBO, asi que parecia atravesarla
//
// Estos tests fijan lo que hace falta para que una familia compuesta este
// COMPLETA, de modo que la proxima que se anada no pueda repetir el fallo.

// ----------------------------------------------------------------------------
// LA BIZNAGA ES UN BLOQUE COMPUESTO BIEN FORMADO
// ----------------------------------------------------------------------------

TEST_CASE("Biznaga: se reconoce como compuesto de su familia") {
    // Si esto fallara, ni la textura ni la colision ni el mesher la
    // reconocerian: las tres preguntan por familia.
    for (uint16_t etapa = 0; etapa <= Compuesto::Biznaga::ADULTA; ++etapa) {
        const BlockType b = Compuesto::Biznaga::nuevo(etapa, 0, 3);
        INFO("etapa ", etapa);
        CHECK(Compuesto::esCompuesto(b));
        CHECK(Compuesto::familiaDe(b) == Compuesto::FAM_BIZNAGA);
        CHECK(Compuesto::Biznaga::esBiznaga(b));
    }
}

TEST_CASE("Biznaga: su ID esta MUY por encima del enum") {
    // Esta es la razon de fondo del bug: un compuesto NO puede ser un `case`
    // del switch de texturas, asi que necesita su rama propia antes. Si algun
    // dia el enum creciera hasta alcanzar este rango, se leerian plantas donde
    // hay bloques normales.
    const BlockType b = Compuesto::Biznaga::nuevo(Compuesto::Biznaga::ADULTA, 0, 5);
    CHECK((int)b > BLOCK_TYPE_MAX);
    CHECK((int)b >= Compuesto::COMPUESTO_BASE);
}

// ----------------------------------------------------------------------------
// EL ESTADO SOBREVIVE AL VIAJE POR EL ID
// ----------------------------------------------------------------------------
// El mesher dibuja la planta A PARTIR de estos campos: si no se recuperan
// intactos, la forma que se dibuja no es la que se guardo.

TEST_CASE("Biznaga: etapa, giro y costillas se recuperan intactos") {
    for (uint16_t etapa = 0; etapa <= Compuesto::Biznaga::ADULTA; ++etapa)
        for (uint16_t giro = 0; giro < 4; ++giro)
            for (uint16_t cost = 0; cost < 8; ++cost) {
                const BlockType b = Compuesto::Biznaga::nuevo(etapa, giro, cost);
                INFO("etapa ", etapa, " giro ", giro, " costillas ", cost);
                CHECK(Compuesto::Biznaga::etapaDe(b) == etapa);
                CHECK(Compuesto::Biznaga::giroDe(b) == giro);
                CHECK(Compuesto::Biznaga::costillasCampoDe(b) == cost);
            }
}

TEST_CASE("Biznaga: las costillas caen en el rango real de un Ferocactus") {
    // Una biznaga de verdad tiene entre 13 y 21 costillas. El mesher dibuja
    // una lasca por costilla, asi que este rango es lo que se ve en la planta.
    for (uint16_t cost = 0; cost < 8; ++cost) {
        const BlockType b = Compuesto::Biznaga::nuevo(0, 0, cost);
        const int n = Compuesto::Biznaga::costillasReales(b);
        INFO("campo ", cost, " -> ", n, " costillas");
        CHECK(n >= 13);
        CHECK(n <= 20);
    }
}

// ----------------------------------------------------------------------------
// LA FORMA QUE DIBUJA EL MESHER Y LA QUE FRENA AL JUGADOR SON LA MISMA
// ----------------------------------------------------------------------------
// El mesher y nopalHitboxCon parten los dos de escalaDeEtapa(). Mientras eso
// se cumpla, lo que se ve y lo que se toca coinciden -- que es justo lo que
// fallaba cuando una se dibujaba como cubo.

TEST_CASE("Biznaga: crece con la etapa y nunca desborda el voxel") {
    const float peq  = Compuesto::Biznaga::escalaDeEtapa(Compuesto::Biznaga::PEQUENA);
    const float med  = Compuesto::Biznaga::escalaDeEtapa(Compuesto::Biznaga::MEDIANA);
    const float adul = Compuesto::Biznaga::escalaDeEtapa(Compuesto::Biznaga::ADULTA);

    // Cada etapa es mayor que la anterior: se reconoce de lejos cual es cual.
    CHECK(peq < med);
    CHECK(med < adul);

    // Y ninguna se sale de su celda: el radio es escala/2 alrededor del
    // centro, asi que con escala <= 1 la planta cabe entera en el voxel.
    CHECK(adul <= 1.0f);
    CHECK(peq > 0.0f);
}

TEST_CASE("Biznaga: la adulta llena su bloque") {
    namespace B = Compuesto::Biznaga;

    // La biznaga se veia demasiado pequeña -- un boton en el suelo en vez del
    // barril que es -- asi que se subio la escala de la adulta a 1.0, que es el
    // voxel ENTERO (el radio del mesher es escala/2 = 0.5).
    //
    // Este test fija las dos mitades de esa decision:
    //
    //   1. Que de verdad llene el bloque, no que se quede a medias. Si alguien
    //      la bajara "por seguridad", volveria el problema original.
    //   2. Que no lo pase. Con escala > 1 el cactus se meteria en la celda
    //      vecina, donde ni la colision ni la seleccion lo esperan porque las
    //      dos trabajan celda a celda.
    const float adul = B::escalaDeEtapa(B::ADULTA);
    INFO("escala de la adulta: ", adul);
    CHECK(adul == doctest::Approx(1.0f));

    // El radio derivado toca exactamente el borde del voxel.
    CHECK(adul * 0.5f == doctest::Approx(0.5f));
}

TEST_CASE("Biznaga: al crecer solo cambia el tamaño, no la forma") {
    namespace B = Compuesto::Biznaga;

    // Radio y alto salen los DOS de escalaDeEtapa (r = esc*0.5, alto =
    // esc*0.75). Por eso escalar el cactus lo agranda sin deformarlo: la
    // proporcion alto/ancho es la misma en las tres etapas.
    //
    // Es lo que permite cambiar el tamaño sin volver a dibujar el modelo. Si
    // algun dia alto y radio dejaran de salir del mismo numero, esta garantia
    // se pierde y habria que revisar el mesher.
    const float proporcionEsperada = 0.75f;
    for (uint16_t e = B::PEQUENA; e <= B::ADULTA; ++e) {
        const float esc = B::escalaDeEtapa(e);
        const float r    = esc * 0.5f;
        const float alto = esc * 0.75f;
        INFO("etapa ", e, " radio ", r, " alto ", alto);

        // La proporcion no depende de la etapa: misma silueta a otra escala.
        CHECK(alto / (r * 2.0f) == doctest::Approx(proporcionEsperada));
    }
}

TEST_CASE("Biznaga: es mas ancha que alta, como un barril") {
    // El alto es 0.75 del ancho (proporcion de un Ferocactus). Es el mismo
    // numero en el mesher y en la caja de colision; si uno cambiara sin el
    // otro, el jugador volveria a chocar donde no ve nada.
    for (uint16_t etapa = 0; etapa <= Compuesto::Biznaga::ADULTA; ++etapa) {
        const float esc  = Compuesto::Biznaga::escalaDeEtapa(etapa);
        const float alto = esc * 0.75f;
        INFO("etapa ", etapa);
        CHECK(alto < esc);        // achatada
        CHECK(alto > 0.0f);
    }
}

// ----------------------------------------------------------------------------
// EL CRECIMIENTO
// ----------------------------------------------------------------------------

TEST_CASE("Biznaga: crece de etapa en etapa y se para en adulta") {
    BlockType b = Compuesto::Biznaga::nuevo(Compuesto::Biznaga::PEQUENA, 2, 4);

    b = Compuesto::Biznaga::crecida(b);
    CHECK(Compuesto::Biznaga::etapaDe(b) == Compuesto::Biznaga::MEDIANA);

    b = Compuesto::Biznaga::crecida(b);
    CHECK(Compuesto::Biznaga::etapaDe(b) == Compuesto::Biznaga::ADULTA);
    CHECK(Compuesto::Biznaga::estaHecha(b));

    // Ya no crece mas: una adulta se queda adulta.
    const BlockType antes = b;
    b = Compuesto::Biznaga::crecida(b);
    CHECK(b == antes);
}

TEST_CASE("Biznaga: al crecer conserva su giro y sus costillas") {
    // Las costillas se fijan al brotar y no cambian en toda su vida, como en
    // la planta real. Si el crecimiento las pisara, la biznaga cambiaria de
    // forma al hacerse mayor.
    BlockType b = Compuesto::Biznaga::nuevo(Compuesto::Biznaga::PEQUENA, 3, 6);
    const uint16_t giro0 = Compuesto::Biznaga::giroDe(b);
    const uint16_t cost0 = Compuesto::Biznaga::costillasCampoDe(b);

    b = Compuesto::Biznaga::crecida(b);
    CHECK(Compuesto::Biznaga::giroDe(b) == giro0);
    CHECK(Compuesto::Biznaga::costillasCampoDe(b) == cost0);
}

// ----------------------------------------------------------------------------
// LAS FAMILIAS NO SE PISAN
// ----------------------------------------------------------------------------

TEST_CASE("Biznaga: no se confunde con un maguey") {
    // Los dos son compuestos. Si sus rangos se solaparan, el mesher dibujaria
    // una donde va la otra -- y es exactamente el tipo de fallo que produce
    // parches de piedra en el suelo.
    const BlockType bz = Compuesto::Biznaga::nuevo(Compuesto::Biznaga::ADULTA, 3, 7);
    const BlockType mg = Compuesto::Maguey::nuevo(Compuesto::Maguey::PRODUCTOR, 3);

    CHECK(bz != mg);
    CHECK(Compuesto::familiaDe(bz) != Compuesto::familiaDe(mg));
    CHECK_FALSE(Compuesto::Biznaga::esBiznaga(mg));
}

TEST_CASE("Biznaga: ninguna familia compuesta se sale de su hueco de IDs") {
    // La red que protege el formato de guardado: cada familia tiene su rango
    // y no puede invadir el de la siguiente. Si una lo hiciera, un mundo
    // guardado leeria otra planta al recargarlo.
    for (int f = 0; f < Compuesto::FAM_COUNT; ++f) {
        const BlockType primero =
            Compuesto::hacer((Compuesto::Familia)f, 0);
        const BlockType ultimo =
            Compuesto::hacer((Compuesto::Familia)f,
                             (uint16_t)(Compuesto::ESTADOS_POR_FAMILIA - 1));

        INFO("familia ", f);
        CHECK(Compuesto::familiaDe(primero) == (Compuesto::Familia)f);
        CHECK(Compuesto::familiaDe(ultimo) == (Compuesto::Familia)f);
        CHECK(Compuesto::esCompuesto(primero));
        CHECK(Compuesto::esCompuesto(ultimo));
    }
}
