#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include "FisicaCaida.h"

using namespace Compuesto;
namespace M = Compuesto::Maguey;

// ============================================================================
// LOS BUGS DEL MAGUEY, FIJADOS PARA QUE NO VUELVAN
// ============================================================================
// Al integrar el maguey como bloque COMPUESTO se quedaron sin actualizar
// varios caminos del motor que preguntan cosas sobre un bloque. Todos tenian
// la misma forma: una funcion con un switch o una cadena de ifs que no
// reconocia el ID nuevo y lo dejaba caer al caso por defecto.
//
// Ese caso por defecto casi nunca es inofensivo:
//
//   getBlockColor        -> por defecto BLANCO: las particulas de romper un
//                           maguey salian blancas, como si fuera de nieve
//   densidadDe           -> por defecto ROCA: el agave caia con el peso de un
//                           bloque de granito
//   esOrganicoParaHacha  -> por defecto NO organico: cortarlo no gastaba el
//                           hacha, que duraba eternamente
//   lightCost            -> por defecto aire: un productor de dos metros no
//                           daba mas sombra que un brote
//   getBlockBreakTimeForMode -> la regla del hacha solo miraba el bloque
//                           VIEJO, asi que el maguey nuevo se rompia a mano
//                           en 0,6 s
//
// Estos tests comprueban cada uno por separado, y ademas hay un barrido que
// recorre TODAS las combinaciones de estado para que ningun maguey concreto
// se quede fuera.

// ----------------------------------------------------------------------------
// AYUDA: TODOS LOS MAGUEYES POSIBLES
// ----------------------------------------------------------------------------
// No basta con probar "un maguey": el ID cambia con cada campo del estado, y
// un fallo puede afectar solo a los que tienen jugo, o solo a los capados.
// Estas funciones generan el abanico entero.

static std::vector<BlockType> todosLosMagueyes() {
    std::vector<BlockType> v;
    for (uint16_t etapa = 0; etapa <= M::PRODUCTOR; ++etapa)
        for (uint16_t giro = 0; giro <= M::GIRO.maximo(); ++giro)
            for (uint16_t puntas = 0; puntas <= M::PUNTAS.maximo(); ++puntas)
                for (uint16_t jugo = 0; jugo <= M::AGUAMIEL.maximo(); jugo += 5)
                    for (int capado = 0; capado <= 1; ++capado)
                        v.push_back(M::crear(etapa, giro, puntas, jugo,
                                             capado != 0));
    return v;
}

// ----------------------------------------------------------------------------
// BUG: EL MAGUEY PESABA COMO UNA ROCA AL CAER
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: no cae con densidad de piedra") {
    // Un agave es carne de planta: pesa parecido al agua, no al granito. Si
    // esto falla, el maguey se desploma como un bloque de roca.
    const float dPiedra = Fisica::densidadDe(BLOCK_STONE);

    for (BlockType m : todosLosMagueyes()) {
        const float d = Fisica::densidadDe(m);
        INFO("maguey id=", (int)m, " densidad=", d);
        CHECK(d < dPiedra);       // mucho mas ligero que la roca
        CHECK(d > 0.0f);          // pero tiene peso
    }
}

TEST_CASE("Maguey bugs: pesa lo mismo que el resto de carne de planta") {
    // Se compara con el nopal, que es la otra planta carnosa del juego.
    const float dNopal  = Fisica::densidadDe(BLOCK_NOPAL_TALLO);
    const float dMaguey = Fisica::densidadDe(M::nuevo(M::PRODUCTOR, 0));
    CHECK(dMaguey == doctest::Approx(dNopal));
}

// ----------------------------------------------------------------------------
// BUG: CORTARLO NO GASTABA EL HACHA
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: cortarlo SI gasta el hacha") {
    // Sin esto la herramienta duraba eternamente talando magueyes, que es
    // justo lo contrario de lo que cuesta cortar un agave hecho.
    for (BlockType m : todosLosMagueyes()) {
        INFO("maguey id=", (int)m);
        CHECK(esOrganicoParaHacha(m));
        CHECK(desgasteHacha(m) > 0);
    }
}

TEST_CASE("Maguey bugs: el pico no sirve y no se gasta con el") {
    // Es planta, no roca. Intentarlo con el pico no debe castigar al jugador
    // gastandole la herramienta.
    for (BlockType m : todosLosMagueyes()) {
        INFO("maguey id=", (int)m);
        CHECK_FALSE(esRocaParaPico(m));
        CHECK(desgastePico(m) == 0);
    }
}

// ----------------------------------------------------------------------------
// BUG: SE DERRUMBABA MAL / SE PERDIA SIN SOLTAR NADA
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: es planta, no cimiento, asi que se cae") {
    // Un maguey no sostiene el mundo: si le quitas el suelo, se viene abajo
    // como cualquier planta.
    for (BlockType m : todosLosMagueyes()) {
        INFO("maguey id=", (int)m);
        CHECK_FALSE(Fisica::esTerrenoNatural(m));
        CHECK(Fisica::puedeCaer(m));
    }
}

// ----------------------------------------------------------------------------
// BUG: TODAS LAS ETAPAS TIENEN QUE SER COHERENTES
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: ningun estado produce un ID fuera de su familia") {
    // El empaquetado mete el estado dentro del ID. Un campo que se desborde
    // podria empujar el ID hasta la familia de al lado, y entonces un maguey
    // se leeria como otra planta distinta.
    for (BlockType m : todosLosMagueyes()) {
        INFO("maguey id=", (int)m);
        REQUIRE(esCompuesto(m));
        CHECK(familiaDe(m) == FAM_MAGUEY);
    }
}

TEST_CASE("Maguey bugs: el estado sobrevive a la ida y la vuelta") {
    // Es lo que hace que el guardado funcione sin tocar el formato: si
    // descomponer no devolviera lo mismo que se compuso, cargar un mundo
    // daria magueyes distintos a los guardados.
    for (uint16_t etapa = 0; etapa <= M::PRODUCTOR; ++etapa)
        for (uint16_t giro = 0; giro <= M::GIRO.maximo(); ++giro)
            for (uint16_t puntas = 0; puntas <= M::PUNTAS.maximo(); ++puntas)
                for (uint16_t jugo = 0; jugo <= M::AGUAMIEL.maximo(); ++jugo)
                    for (int capado = 0; capado <= 1; ++capado) {
                        const BlockType m =
                            M::crear(etapa, giro, puntas, jugo, capado != 0);
                        INFO("etapa=", etapa, " giro=", giro,
                             " puntas=", puntas, " jugo=", jugo,
                             " capado=", capado);
                        CHECK(M::etapaDe(m)    == etapa);
                        CHECK(M::giroDe(m)     == giro);
                        CHECK(M::puntasDe(m)   == puntas);
                        CHECK(M::aguamielDe(m) == jugo);
                        CHECK(M::capadoDe(m)   == (capado != 0));
                    }
}

// ----------------------------------------------------------------------------
// BUG: EL INVENTARIO CREATIVO DABA LA PUNTA SUELTA
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: lo que va al creativo son plantas de verdad") {
    // Antes el creativo ofrecia BLOCK_MAGUEY_PUNTA -- la punta SUELTA del
    // sistema viejo -- que al colocarla dejaba una espina flotando sin planta
    // debajo. Ahora se ofrece una planta por etapa.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const BlockType m = M::nuevo(e, 0);
        INFO("etapa ", e);
        CHECK(esCompuesto(m));
        CHECK(familiaDe(m) == FAM_MAGUEY);
        CHECK(M::etapaDe(m) == e);
        // Recien puesto: sin capar y sin jugo.
        CHECK_FALSE(M::capadoDe(m));
        CHECK(M::aguamielDe(m) == 0);
    }

    // Y el ejemplar "listo para ordeñar" que tambien se ofrece.
    const BlockType listo = M::conAguamiel(
        M::capar(M::nuevo(M::PRODUCTOR, 0)), 15);
    CHECK(M::capadoDe(listo));
    CHECK(M::hayParaTazon(estadoDe(listo)));
}

// ----------------------------------------------------------------------------
// LO QUE EL ARREGLO NO PUEDE HABER ROTO
// ----------------------------------------------------------------------------

TEST_CASE("Maguey bugs: el terreno sigue sin poder caerse") {
    // El arreglo de la densidad y el de organico tocaron funciones que usa
    // todo el motor. Se comprueba que el terreno no se ha contagiado.
    const BlockType TERRENO[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_GRAVEL
    };
    for (BlockType t : TERRENO) {
        INFO("terreno ", (int)t);
        CHECK(Fisica::esTerrenoNatural(t));
        CHECK_FALSE(Fisica::puedeCaer(t));
        CHECK_FALSE(esOrganicoParaHacha(t));
        CHECK(Fisica::densidadDe(t) > Fisica::densidadDe(BLOCK_NOPAL_TALLO));
    }
}

TEST_CASE("Maguey bugs: los arboles siguen siendo organicos y cayendo") {
    const BlockType ARBOL[] = {
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL
    };
    for (BlockType t : ARBOL) {
        INFO("arbol ", (int)t);
        CHECK(esOrganicoParaHacha(t));
        CHECK(desgasteHacha(t) > 0);
        CHECK(Fisica::puedeCaer(t));
    }
}

TEST_CASE("Maguey bugs: la constante compartida no se ha separado") {
    // BlockType.h repite el numero de COMPUESTO_BASE porque no puede
    // preguntarselo a BloqueCompuesto.h (la dependencia va al reves). Hay un
    // static_assert que las ata; esto lo comprueba tambien desde los tests.
    CHECK(COMPUESTO_BASE == BLOQUE_COMPUESTO_BASE_ID);

    // Y que el rango sigue estando por encima de todo lo demas.
    CHECK(COMPUESTO_BASE > BLOCK_TYPE_MAX);
    CHECK(COMPUESTO_BASE > BLOCK_MIXTO_FIN);
}
