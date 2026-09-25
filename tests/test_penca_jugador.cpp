#include <doctest/doctest.h>
#include "render/PencaDelJugador.h"

#include <vector>

using namespace Render;

// ============================================================================
// LA PENCA DE LA MATA CONSERVA SU FORMA; LA DEL JUGADOR SE ACUESTA
// ============================================================================
// Se pidio que la penca de nopal quede acostada SOLO cuando la coloca el
// jugador, y que las generadas por un nopal conserven su forma de siempre.
//
// LAS DOS SON EL MISMO BLOQUE, asi que el ID no distingue. La diferencia real
// esta en el mundo: la generacion SOLO coloca pencas PEGADAS a un cladodio
// --comprueba `tocaCladodio` antes de escribir cada una-- mientras que la del
// jugador cae donde el apunte.
//
//     PEGADA A LA PLANTA  =  generada  -> conserva su forma
//     SUELTA              =  colocada  -> se acuesta
//
// ⭐ Y EL BUG QUE ESTOS TESTS FIJAN: la primera version miraba solo las SEIS
// CARAS RECTAS, mientras que la condicion que usaba la orientacion contaba
// ademas las diagonales. Dos reglas distintas para la misma pregunta.
//
// Se veia en un caso nada raro -- una penca en diagonal a un cladodio, que es
// justo como sale un brote del borde superior de otra: quedaba DE PIE por la
// orientacion y TUMBADA por el dibujo.

namespace {

// Monta una vecindad a mano: la lista de desplazamientos donde SI hay planta.
struct Vecindad {
    std::vector<VecindadPenca> conPlanta;

    bool operator()(int dx, int dy, int dz) const {
        for (const VecindadPenca& v : conPlanta)
            if (v.dx == dx && v.dy == dy && v.dz == dz) return true;
        return false;
    }
};

} // namespace

// ----------------------------------------------------------------------------
// LOS DOS CASOS BASE
// ----------------------------------------------------------------------------

TEST_CASE("Penca: sin nada alrededor esta SUELTA") {
    // La que coloca el jugador en campo abierto. Se acuesta.
    Vecindad v;   // nada
    CHECK(pencaSuelta(v) == true);
}

TEST_CASE("Penca: pegada por una cara NO esta suelta") {
    // La que nace de un nopal: pegada al tallo o a otro cladodio.
    for (VecindadPenca cara : { VecindadPenca{ 1,0,0}, VecindadPenca{-1,0,0},
                                VecindadPenca{0, 1,0}, VecindadPenca{0,-1,0},
                                VecindadPenca{0,0, 1}, VecindadPenca{0,0,-1} }) {
        Vecindad v;
        v.conPlanta.push_back(cara);
        INFO("pegada por (", cara.dx, ",", cara.dy, ",", cara.dz, ")");
        CHECK(pencaSuelta(v) == false);
    }
}

// ----------------------------------------------------------------------------
// EL BUG: LAS DIAGONALES
// ----------------------------------------------------------------------------

TEST_CASE("Penca: una DIAGONAL VERTICAL cuenta como pegada") {
    // ⭐⭐ EL CASO QUE PRODUCIA EL DESACUERDO.
    //
    // Un brote que sale del borde SUPERIOR de otra penca queda en diagonal, y
    // es la forma normal de crecer de un nopal. Con la regla de solo-caras,
    // esta penca salia "suelta" para el dibujo y "pegada" para la orientacion:
    // de pie por dentro, tumbada por fuera.
    for (VecindadPenca d : { VecindadPenca{ 1, 1,0}, VecindadPenca{-1, 1,0},
                             VecindadPenca{0, 1, 1}, VecindadPenca{0, 1,-1},
                             VecindadPenca{ 1,-1,0}, VecindadPenca{-1,-1,0},
                             VecindadPenca{0,-1, 1}, VecindadPenca{0,-1,-1} }) {
        Vecindad v;
        v.conPlanta.push_back(d);
        INFO("diagonal (", d.dx, ",", d.dy, ",", d.dz, ")");
        CHECK(pencaSuelta(v) == false);
    }
}

TEST_CASE("Penca: una ESQUINA del plano cuenta como pegada") {
    // Brotes que salen de una esquina. Mismo razonamiento.
    for (VecindadPenca e : { VecindadPenca{ 1,0, 1}, VecindadPenca{-1,0,-1},
                             VecindadPenca{ 1,0,-1}, VecindadPenca{-1,0, 1} }) {
        Vecindad v;
        v.conPlanta.push_back(e);
        INFO("esquina (", e.dx, ",", e.dy, ",", e.dz, ")");
        CHECK(pencaSuelta(v) == false);
    }
}

// ----------------------------------------------------------------------------
// LA LISTA DE VECINDADES
// ----------------------------------------------------------------------------

TEST_CASE("Penca: se miran 18 vecindades, ni mas ni menos") {
    // 6 caras + 8 diagonales verticales + 4 esquinas del plano.
    //
    // El numero importa porque es lo que tiene que coincidir con lo que mira
    // `sueltaEnPlanta` en el mesher. Si alguien anade o quita una, este test
    // lo dice.
    int n = 0;
    vecindadesPenca(n);
    CHECK(n == 18);
}

TEST_CASE("Penca: no hay vecindades repetidas") {
    // Una repetida no rompe nada --la respuesta seria la misma-- pero delata
    // que la lista se escribio a mano sin revisar, y la siguiente vez podria
    // ser una que FALTE en vez de una que sobre.
    int n = 0;
    const VecindadPenca* v = vecindadesPenca(n);

    for (int i = 0; i < n; ++i)
        for (int j = i + 1; j < n; ++j) {
            const bool iguales = (v[i].dx == v[j].dx &&
                                  v[i].dy == v[j].dy &&
                                  v[i].dz == v[j].dz);
            INFO("indices ", i, " y ", j);
            CHECK(iguales == false);
        }
}

TEST_CASE("Penca: ninguna vecindad es el propio bloque") {
    // (0,0,0) seria preguntarse a si misma: siempre daria "pegada" y ninguna
    // penca se acostaria nunca.
    int n = 0;
    const VecindadPenca* v = vecindadesPenca(n);
    for (int i = 0; i < n; ++i) {
        const bool esElMismo = (v[i].dx == 0 && v[i].dy == 0 && v[i].dz == 0);
        CHECK(esElMismo == false);
    }
}

TEST_CASE("Penca: todas las vecindades son adyacentes") {
    // Ningun desplazamiento puede pasar de 1 en ningun eje: mirar dos bloques
    // mas alla haria que una penca se considerara "de la mata" por una planta
    // que ni la toca.
    int n = 0;
    const VecindadPenca* v = vecindadesPenca(n);
    for (int i = 0; i < n; ++i) {
        CHECK(v[i].dx >= -1); CHECK(v[i].dx <= 1);
        CHECK(v[i].dy >= -1); CHECK(v[i].dy <= 1);
        CHECK(v[i].dz >= -1); CHECK(v[i].dz <= 1);
    }
}

// ----------------------------------------------------------------------------
// CASOS DE JUEGO
// ----------------------------------------------------------------------------

TEST_CASE("Penca: rodeada de planta, claramente pegada") {
    // Una penca en mitad de una mata. No hay duda posible.
    int n = 0;
    const VecindadPenca* todas = vecindadesPenca(n);

    Vecindad v;
    for (int i = 0; i < n; ++i) v.conPlanta.push_back(todas[i]);

    CHECK(pencaSuelta(v) == false);
}

TEST_CASE("Penca: un bloque cualquiera al lado NO la ata a la planta") {
    // ⭐ LA DISTINCION QUE HACE UTIL LA REGLA.
    //
    // `encadena` responde si el vecino es PARTE DE LA MISMA PLANTA, no si hay
    // algo solido. Una penca puesta contra un muro de piedra, o sobre tierra,
    // sigue siendo del jugador y tiene que acostarse.
    //
    // Aqui se simula con una vecindad vacia: el muro existe en el mundo, pero
    // `nopalEncadena` devuelve false para el, asi que no aparece en la lista.
    Vecindad v;   // hay piedra alrededor, pero no es planta
    CHECK(pencaSuelta(v) == true);
}

TEST_CASE("Penca: al romper la mata, la que queda pasa a estar suelta") {
    // La clasificacion sale del SITIO, no de una marca guardada. Si el jugador
    // tala el nopal alrededor, lo que queda deja de estar pegado a nada.
    //
    // Es una consecuencia del diseno, no un accidente: una marca en el bloque
    // habria dejado esa penca "de la planta" para siempre, de pie en mitad de
    // un claro.
    Vecindad conMata;
    conMata.conPlanta.push_back({ 0,-1, 0 });   // cladodio debajo
    CHECK(pencaSuelta(conMata) == false);

    Vecindad talada;                             // ya no hay nada
    CHECK(pencaSuelta(talada) == true);
}
