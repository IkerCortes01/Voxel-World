#include <doctest/doctest.h>
#include "render/EspacioJugador.h"

// ============================================================================
// APILAR BAJO LOS PIES TE SUBE ENCIMA, NO TE ENTIERRA
// ============================================================================
// LO QUE SE PIDIO: colocando niveles en continuo (clic derecho mantenido)
// justo debajo del jugador, sin saltar, que el jugador se eleve lo necesario
// para no quedarse atascado dentro de lo que acaba de poner.
//
// Lo que se fija aqui es `subidaPorApilado`, la cuenta que decide CUANTO hay
// que subir. Es aritmetica pura sobre la caja del jugador y la altura solida
// del bloque, asi que se puede probar sin arrancar el juego -- igual que el
// resto de EspacioJugador.h.
//
// ----------------------------------------------------------------------------
// LA REGLA, EN UNA LINEA
// ----------------------------------------------------------------------------
// Si la cara de arriba del bloque queda POR ENCIMA de los pies y el bloque
// esta en la columna del jugador, se devuelve la diferencia. Si no, cero.
//
// El caso limite importa: un nivel cuya superficie coincide EXACTAMENTE con
// los pies no sube nada. Es la situacion normal de estar de pie sobre algo, y
// devolver un valor positivo ahi haria que el jugador flotara un poco mas en
// cada clic.
// ============================================================================

using Render::CajaJugador;
using Render::cajaDelJugador;
using Render::subidaPorApilado;

// El jugador de pie en (0.5, 10, 0.5), con la hitbox REAL de la fisica
// (0.6 x 1.8), no las constantes obsoletas de `struct Player`.
static CajaJugador jugadorEn(float y) {
    return cajaDelJugador(0.5f, y, 0.5f, 0.6f, 1.8f);
}

TEST_CASE("Apilado: un nivel que crece bajo los pies levanta al jugador") {
    const CajaJugador j = jugadorEn(10.0f);

    // La celda 9 pasa a tener 4/8 de altura: su superficie queda en 9.5,
    // medio bloque POR ENCIMA de los pies (que estan en 10.0)... no.
    // 9 + 0.5 = 9.5, que sigue por debajo de 10. No sube.
    CHECK(subidaPorApilado(0, 9, 0, 0.5f, j) == doctest::Approx(0.0f));

    // Pero la celda que el jugador PISA (la 10, donde estan sus pies) creciendo
    // a 4/8 pone su superficie en 10.5: medio bloque por encima. Sube 0.5.
    CHECK(subidaPorApilado(0, 10, 0, 0.5f, j) == doctest::Approx(0.5f));
}

TEST_CASE("Apilado: cada octavo sube exactamente un octavo") {
    const CajaJugador j = jugadorEn(10.0f);

    // Es el gesto del enunciado: niveles de 1 en 1 bajo los pies. Cada uno
    // levanta su parte y ninguno atasca.
    for (int n = 1; n <= 8; ++n) {
        const float altura = (float)n / 8.0f;
        CHECK(subidaPorApilado(0, 10, 0, altura, j) ==
              doctest::Approx(altura));
    }
}

TEST_CASE("Apilado: estar de pie sobre algo no levanta nada") {
    // ⚠️ EL CASO QUE EVITA QUE EL JUGADOR FLOTE.
    //
    // Los pies en 10.0 sobre un bloque entero en la celda 9: su superficie es
    // exactamente 10.0. La diferencia es cero, y tiene que salir cero -- si
    // saliera un epsilon positivo, cada clic subiria un poco al jugador y al
    // cabo de veinte estaria levitando.
    const CajaJugador j = jugadorEn(10.0f);
    CHECK(subidaPorApilado(0, 9, 0, 1.0f, j) == doctest::Approx(0.0f));

    // Y un bloque muy por debajo tampoco.
    CHECK(subidaPorApilado(0, 5, 0, 1.0f, j) == doctest::Approx(0.0f));
}

TEST_CASE("Apilado: un bloque al lado no levanta a nadie") {
    // El jugador esta en x=0.5, z=0.5, o sea en la columna (0,0). Un nivel que
    // crece en la columna de al lado no lo toca, por muy alto que sea.
    const CajaJugador j = jugadorEn(10.0f);

    CHECK(subidaPorApilado(2, 10, 0, 1.0f, j) == doctest::Approx(0.0f));
    CHECK(subidaPorApilado(0, 10, 2, 1.0f, j) == doctest::Approx(0.0f));
    CHECK(subidaPorApilado(-3, 10, -3, 1.0f, j) == doctest::Approx(0.0f));
}

TEST_CASE("Apilado: la columna se mira por SOLAPE, no por celda del centro") {
    // El jugador mide 0.6 de ancho, asi que a caballo entre dos celdas ocupa
    // las dos. Un nivel que crece en cualquiera de ellas lo sostiene.
    //
    // Pies en x=1.0: la caja va de 0.7 a 1.3, o sea toca la celda 0 (por su
    // borde 0.7..1.0) y la celda 1.
    const CajaJugador j = cajaDelJugador(1.0f, 10.0f, 0.5f, 0.6f, 1.8f);

    CHECK(subidaPorApilado(0, 10, 0, 0.5f, j) == doctest::Approx(0.5f));
    CHECK(subidaPorApilado(1, 10, 0, 0.5f, j) == doctest::Approx(0.5f));

    // La celda 2 ya no la toca (la caja acaba en 1.3).
    CHECK(subidaPorApilado(2, 10, 0, 0.5f, j) == doctest::Approx(0.0f));
}

TEST_CASE("Apilado: mas de un bloque de golpe NO es un escalon") {
    // ⚠️ EL TOPE QUE IMPIDE QUE ESTO SEA UN TELETRANSPORTE.
    //
    // Subir al jugador mas de lo que subiria andando (stepHeight = 1.05) seria
    // moverlo a un sitio que no ha pedido. Si algun dia una celda creciera de
    // golpe mas que eso, se prefiere no tocarlo y que la fisica resuelva.
    const CajaJugador j = jugadorEn(10.0f);

    // Justo en el limite: 1.05 entra.
    CHECK(subidaPorApilado(0, 10, 0, 1.05f, j) == doctest::Approx(1.05f));

    // Por encima del limite, no se mueve.
    CHECK(subidaPorApilado(0, 10, 0, 1.5f, j) == doctest::Approx(0.0f));
    CHECK(subidaPorApilado(0, 11, 0, 1.0f, j) == doctest::Approx(0.0f));
}

TEST_CASE("Apilado: agachado la caja es mas baja pero los pies mandan") {
    // La subida depende de los PIES, no de la altura del cuerpo: agachado o de
    // pie, el suelo que crece bajo uno lo levanta igual. Lo que cambia con la
    // altura es si cabe arriba, y eso lo comprueba el motor aparte.
    const CajaJugador dePie    = cajaDelJugador(0.5f, 10.0f, 0.5f, 0.6f, 1.8f);
    const CajaJugador agachado = cajaDelJugador(0.5f, 10.0f, 0.5f, 0.6f, 0.95f);

    CHECK(subidaPorApilado(0, 10, 0, 0.5f, dePie) ==
          doctest::Approx(subidaPorApilado(0, 10, 0, 0.5f, agachado)));
}

TEST_CASE("Apilado: coherencia con celdaPisaAlJugador") {
    // ⭐ LAS DOS REGLAS TIENEN QUE CONTAR LA MISMA HISTORIA.
    //
    // Si una celda se solapa con el jugador (celdaPisaAlJugador) es porque su
    // contenido lo atraparia -- y es exactamente el caso en el que subirlo
    // tiene sentido. Este test ata las dos funciones: cuando la celda que se
    // apila es la de los pies y se llena, las dos tienen que estar de acuerdo
    // en que hay un problema que resolver.
    const CajaJugador j = jugadorEn(10.0f);

    // Celda de los pies, llena: pisa al jugador Y hay que subirlo.
    CHECK(Render::celdaPisaAlJugador(0, 10, 0, j));
    CHECK(subidaPorApilado(0, 10, 0, 1.0f, j) > 0.0f);

    // Celda de debajo, llena: NO pisa al jugador y NO hay que subirlo.
    CHECK_FALSE(Render::celdaPisaAlJugador(0, 9, 0, j));
    CHECK(subidaPorApilado(0, 9, 0, 1.0f, j) == doctest::Approx(0.0f));
}
