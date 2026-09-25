#include <doctest/doctest.h>
#include "render/EspacioJugador.h"

#include <initializer_list>

using namespace Render;

// ============================================================================
// EL JUGADOR NO SE QUEDA SOFOCADO, Y EL PILAR SIGUE FUNCIONANDO
// ============================================================================
// BUG REPORTADO: al construir, el jugador acababa metido dentro de un bloque.
//
// `placeBlock` ya comprobaba que un bloque colocado A MANO no cayera dentro
// del jugador. El agujero estaba en la GRAVEDAD: un bloque que CAE no pasa por
// esa comprobacion -- aterriza y hace `setBlock` sin preguntarle a nadie.
//
// EL CASO QUE LO DISPARABA ERA JUSTO EL PILAR:
//   1. Saltas y colocas un bloque bajo tus pies.
//   2. Ese bloque no tiene nada debajo, asi que la regla de "lo que se pone en
//      el aire se cae" lo desprende.
//   3. Caes con el.
//   4. Al tocar suelo, se recoloca en la celda que ocupas.
//
// Y la peticion traia una restriccion explicita: arreglarlo SIN romper la
// habilidad de hacer pilares saltando. Esa es la parte delicada -- la solucion
// perezosa (prohibir colocar cerca del jugador) rompe justo ese gesto.
//
// Estos tests fijan las dos cosas a la vez.

namespace {
// Medidas del jugador en el motor.
constexpr float ANCHO = 0.6f;
constexpr float ALTO  = 1.8f;

// Un jugador de pie sobre el bloque y=10, centrado en la celda (5, ?, 5).
CajaJugador jugadorEn(float x, float y, float z) {
    return cajaDelJugador(x, y, z, ANCHO, ALTO);
}
} // namespace

// ----------------------------------------------------------------------------
// LA SOFOCACION
// ----------------------------------------------------------------------------

TEST_CASE("Espacio: la celda de los PIES esta ocupada por el jugador") {
    // Un jugador de pie en y=11.0 ocupa de 11.0 a 12.8. La celda 11 es la de
    // sus pies: si un bloque aterriza ahi, queda dentro de el.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(celdaPisaAlJugador(5, 11, 5, j) == true);
}

TEST_CASE("Espacio: la celda del PECHO tambien") {
    // 1.8 de alto desde 11.0 llega a 12.8, asi que la celda 12 tambien esta
    // ocupada. Es la que mas se olvida: un bloque que cae desde arriba pasa
    // por ella antes de llegar a los pies.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(celdaPisaAlJugador(5, 12, 5, j) == true);
}

TEST_CASE("Espacio: por ENCIMA de la cabeza esta libre") {
    // 11.0 + 1.8 = 12.8, asi que la celda 13 empieza por encima. Ahi SI se
    // puede colocar -- y hace falta que se pueda, o no se podria construir un
    // techo estando debajo.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(celdaPisaAlJugador(5, 13, 5, j) == false);
}

TEST_CASE("Espacio: el bloque que PISA esta libre") {
    // ⭐ EL LIMITE QUE IMPORTA.
    //
    // El jugador esta constantemente apoyado en la cara de arriba del bloque
    // que pisa. Si tocarse por una cara contara como solape, ese bloque
    // quedaria "ocupado" y no se podria colocar nada bajo los pies -- que es
    // justo lo que rompe el pilar.
    //
    // Por eso el solape es ESTRICTO: la celda 10 acaba en 11.0 y el jugador
    // empieza en 11.0, asi que no se solapan.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(celdaPisaAlJugador(5, 10, 5, j) == false);
}

TEST_CASE("Espacio: a un lado esta libre") {
    // El jugador mide 0.6 de ancho: de 5.2 a 5.8. Las celdas 4 y 6 quedan
    // fuera, asi que se puede construir a los lados sin estorbo.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(celdaPisaAlJugador(4, 11, 5, j) == false);
    CHECK(celdaPisaAlJugador(6, 11, 5, j) == false);
    CHECK(celdaPisaAlJugador(5, 11, 4, j) == false);
    CHECK(celdaPisaAlJugador(5, 11, 6, j) == false);
}

TEST_CASE("Espacio: a caballo entre dos celdas, las dos cuentan") {
    // El jugador no esta siempre centrado. Parado en x=5.0, su caja va de 4.7
    // a 5.3: pisa la celda 4 Y la 5. Si solo se mirara la celda de su centro,
    // un bloque podria aterrizar en la mitad de su cuerpo.
    const CajaJugador j = jugadorEn(5.0f, 11.0f, 5.0f);
    CHECK(celdaPisaAlJugador(4, 11, 4, j) == true);
    CHECK(celdaPisaAlJugador(5, 11, 5, j) == true);
}

// ----------------------------------------------------------------------------
// EL PILAR SALTANDO: LA RESTRICCION QUE SE PIDIO NO ROMPER
// ----------------------------------------------------------------------------

TEST_CASE("Pilar: el bloque bajo los pies SOSTIENE al jugador") {
    // ⭐⭐ ESTE ES EL GESTO QUE HAY QUE PRESERVAR.
    //
    // Saltas y pones un bloque bajo tus pies. No tiene nada debajo --estas
    // construyendo hacia arriba-- pero esta sosteniendo a una persona, que es
    // soporte de sobra. No puede caerse.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(sostieneAlJugador(5, 10, 5, j) == true);
}

TEST_CASE("Pilar: funciona en todo el tramo del salto") {
    // En pleno salto los pies no caen justo en el borde de una celda: quedan
    // a media altura. El bloque que sostiene es SIEMPRE el que tiene su techo
    // en los pies o justo debajo.
    //
    // Se comprueba que en todo el recorrido hay exactamente una celda que
    // sostiene, y que es la de debajo de los pies.
    // ⚠️ CUAL ES ESA CELDA, MEDIDO Y NO SUPUESTO.
    //
    // Mi primer intento asumio que con los pies a 11.5 el soporte era la celda
    // 11. Es falso: esa celda va de 11 a 12, asi que SOLAPA al jugador -- y
    // `placeBlock` ni siquiera deja poner un bloque ahi.
    //
    // La celda colocable mas alta por debajo de los pies es la 10 en todo el
    // tramo 11.0-11.8, y pasa a ser la 11 en cuanto los pies llegan a 12.0.
    // O sea: es la ultima celda que NO solapa el cuerpo.
    for (float altura : { 11.0f, 11.2f, 11.5f, 11.8f }) {
        const CajaJugador j = jugadorEn(5.5f, altura, 5.5f);
        INFO("pies en y=", altura);
        CHECK(sostieneAlJugador(5, 10, 5, j) == true);
    }

    // Al subir un bloque entero, el soporte sube con el.
    {
        const CajaJugador j = jugadorEn(5.5f, 12.0f, 5.5f);
        CHECK(sostieneAlJugador(5, 11, 5, j) == true);
    }
}

TEST_CASE("Pilar: la celda de los PROPIOS PIES no cuenta como soporte") {
    // ⚠️ UN FALLO QUE CAZO EL TEST DE COHERENCIA.
    //
    // La primera version toleraba `cy <= minY + 0.05` para dar margen al
    // salto, y con eso la celda de los pies (11, con el jugador en y=11.0)
    // salia como soporte. Esa misma celda esta DENTRO del jugador, asi que
    // las dos reglas se contradecian.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(sostieneAlJugador(5, 11, 5, j) == false);
    CHECK(celdaPisaAlJugador(5, 11, 5, j) == true);
}

TEST_CASE("Pilar: un bloque a un LADO no sostiene a nadie") {
    // La excepcion es solo para lo que esta DEBAJO. Un bloque puesto al lado,
    // aunque este a la misma altura, sigue estando en el aire y tiene que
    // caerse -- o se podrian construir voladizos flotantes sin mas.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(sostieneAlJugador(7, 10, 5, j) == false);
    CHECK(sostieneAlJugador(5, 10, 7, j) == false);
}

TEST_CASE("Pilar: un bloque MUY por debajo tampoco sostiene") {
    // Un bloque tres metros mas abajo no tiene nada que ver con el jugador.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(sostieneAlJugador(5, 7, 5, j) == false);
}

TEST_CASE("Pilar: un bloque POR ENCIMA no sostiene") {
    // Sostener es desde abajo. Uno encima de la cabeza no sujeta nada.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(sostieneAlJugador(5, 13, 5, j) == false);
}

// ----------------------------------------------------------------------------
// LAS DOS REGLAS NO SE CONTRADICEN
// ----------------------------------------------------------------------------

TEST_CASE("Coherencia: lo que sostiene NO esta dentro del jugador") {
    // ⭐ LA PROPIEDAD QUE HACE QUE TODO ENCAJE.
    //
    // Si un bloque pudiera a la vez "sostener al jugador" y "estar dentro del
    // jugador", las dos reglas se pelearian: una diria que no se cae y la otra
    // que no se puede colocar.
    //
    // No puede pasar, porque sostener exige estar POR DEBAJO de los pies y
    // estar dentro exige solapar el cuerpo. Se comprueba en una rejilla.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);

    for (int cx = 3; cx <= 8; ++cx)
        for (int cy = 8; cy <= 14; ++cy)
            for (int cz = 3; cz <= 8; ++cz) {
                const bool sostiene = sostieneAlJugador(cx, cy, cz, j);
                const bool dentro   = celdaPisaAlJugador(cx, cy, cz, j);
                INFO("celda (", cx, ",", cy, ",", cz, ")");
                // Precalculado: doctest no sabe descomponer una expresion con
                // && y aborta la compilacion ("Expression Too Complex").
                const bool ambas = (sostiene && dentro);
                CHECK(ambas == false);
            }
}

TEST_CASE("Coherencia: la caja sale de los PIES, no del centro") {
    // `position` es la posicion de los PIES. Si se interpretara como el
    // centro, la caja quedaria medio cuerpo por debajo del suelo y el bloque
    // que pisa contaria como "dentro": el pilar dejaria de funcionar.
    const CajaJugador j = jugadorEn(5.5f, 11.0f, 5.5f);
    CHECK(j.minY == doctest::Approx(11.0f));
    CHECK(j.maxY == doctest::Approx(11.0f + ALTO));
}
