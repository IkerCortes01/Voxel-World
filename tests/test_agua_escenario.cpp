#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include <vector>
#include <queue>
#include <tuple>
#include <map>

// ============================================================================
// EL ESCENARIO COMPLETO: UNA REJILLA DE AGUA QUE SE SIMULA DE VERDAD
// ============================================================================
// Los otros dos tests del agua comprueban las piezas por separado: el
// empaquetado en el ID y la aritmetica de una regla suelta.
//
// Este monta un MUNDO PEQUENO -- una rejilla de bloques con su suelo -- y
// corre sobre el las MISMAS reglas que updateWaterFlow, tick a tick, hasta
// que el agua se para. Es lo que permite comprobar las propiedades que solo
// se ven cuando el sistema entero funciona junto:
//
//   - que un cubo derramado se EXTIENDE y baja de nivel
//   - que la simulacion TERMINA (no oscila para siempre)
//   - que el volumen se conserva a lo largo de toda la simulacion
//   - que la tierra se satura y deja de beber
//
// No se puede instanciar World (necesita OpenGL), asi que la rejilla es un
// mundo de juguete con la misma logica. Si el motor y esta copia se separan,
// lo que falla es la copia -- pero las REGLAS quedan fijadas aqui, y eso es
// lo que impide que alguien las cambie sin darse cuenta.
// ============================================================================

using namespace Compuesto;

namespace {

// Un mundo plano de ANCHO x ALTO celdas, visto de perfil (un corte vertical).
// La fila 0 es el suelo.
struct Mundo {
    static constexpr int ANCHO = 9;
    static constexpr int ALTO  = 6;

    // nivel[y][x] = octavos de agua en esa celda (0 = sin agua)
    int nivel[ALTO][ANCHO] = {};
    // solido[y][x] = hay bloque macizo (no entra agua)
    bool solido[ALTO][ANCHO] = {};
    // suelo[y][x] = de que es el bloque, para la absorcion
    BlockType material[ALTO][ANCHO] = {};

    void ponerSuelo(BlockType m) {
        for (int x = 0; x < ANCHO; x++) {
            solido[0][x]   = true;
            material[0][x] = m;
        }
    }

    int totalAgua() const {
        int t = 0;
        for (int y = 0; y < ALTO; y++)
            for (int x = 0; x < ANCHO; x++) t += nivel[y][x];
        return t;
    }

    bool puedeEntrar(int x, int y) const {
        if (x < 0 || x >= ANCHO || y < 0 || y >= ALTO) return false;
        return !solido[y][x];
    }

    // Un tick del motor, con las mismas prioridades que updateWaterFlow:
    // absorber -> caer -> repartirse a los lados.
    // Devuelve true si algo se movio.
    bool tick(bool conAbsorcion) {
        bool cambio = false;

        for (int y = 1; y < ALTO; y++) {
            for (int x = 0; x < ANCHO; x++) {
                int n = nivel[y][x];
                if (n <= 0) continue;

                // --- 1. la tierra de debajo bebe ---
                if (conAbsorcion && solido[y-1][x]) {
                    const BlockType seco   = material[y-1][x];
                    const BlockType mojado = versionMojada(seco);
                    if (mojado != seco) {
                        material[y-1][x] = mojado;
                        n--;
                        nivel[y][x] = n;
                        cambio = true;
                        if (n <= 0) continue;
                    }
                }

                // --- 2. caer ---
                if (puedeEntrar(x, y - 1)) {
                    const int hueco = (int)Agua::LLENA - nivel[y-1][x];
                    if (hueco > 0) {
                        const int baja = (n < hueco) ? n : hueco;
                        nivel[y-1][x] += baja;
                        n -= baja;
                        nivel[y][x] = n;
                        cambio = true;
                        if (n <= 0) continue;
                    }
                }

                // --- 3. repartirse a los lados ---
                if (n >= 2) {
                    const int dx[2] = { -1, 1 };
                    int mejorX = -1, mejorN = n;
                    for (int i = 0; i < 2; i++) {
                        const int nx = x + dx[i];
                        if (!puedeEntrar(nx, y)) continue;
                        if (nivel[y][nx] < mejorN) {
                            mejorN = nivel[y][nx];
                            mejorX = nx;
                        }
                    }
                    if (mejorX >= 0 && mejorN <= n - 2) {
                        nivel[y][mejorX]++;
                        nivel[y][x]--;
                        cambio = true;
                    }
                }
            }
        }
        return cambio;
    }

    // Corre hasta que se pare. Devuelve cuantos ticks tardo.
    // El tope existe para que un sistema que oscile falle el test en vez de
    // colgarlo para siempre.
    int simular(bool conAbsorcion, int tope = 500) {
        int t = 0;
        while (t < tope && tick(conAbsorcion)) t++;
        return t;
    }
};

} // namespace

TEST_CASE("un cubo derramado se extiende y baja de nivel") {
    // EL ESCENARIO QUE SE PIDIO, en pequeno: se suelta agua en un sitio y
    // tiene que repartirse alrededor en niveles mas bajos.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);          // piedra: no absorbe, para aislar

    m.nivel[1][4] = 8;                  // un cubo lleno en el centro

    const int antes = m.totalAgua();
    const int ticks = m.simular(false);

    CHECK(ticks < 500);                 // TERMINA
    CHECK(m.totalAgua() == antes);      // NO SE PIERDE NI UNA GOTA

    // Se extendio: hay agua en mas de una celda.
    int celdasConAgua = 0;
    for (int x = 0; x < Mundo::ANCHO; x++)
        if (m.nivel[1][x] > 0) celdasConAgua++;
    CHECK(celdasConAgua > 1);

    // Y donde estaba el cubo ya no hay 8: bajo de nivel al repartirse.
    CHECK(m.nivel[1][4] < 8);
}

TEST_CASE("el agua cae antes de extenderse") {
    // La gravedad manda. Si el agua se extendiera primero, se quedaria
    // haciendo una lamina fina arriba en vez de caer por el agujero.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);

    // Una torre de agua en el aire, a 4 de altura.
    m.nivel[4][4] = 8;

    const int antes = m.totalAgua();
    m.simular(false);

    CHECK(m.totalAgua() == antes);
    // Acabo abajo, sobre el suelo.
    CHECK(m.nivel[1][4] > 0);
    // Y arriba no quedo nada flotando.
    CHECK(m.nivel[4][4] == 0);
}

TEST_CASE("la simulacion siempre termina") {
    // Un sistema de fluidos que oscila es un bug de FPS silencioso: no se ve
    // nada raro en pantalla pero el motor no para de trabajar.
    //
    // Se prueban varias configuraciones, incluida una asimetrica, que es
    // donde un reparto mal planteado se pone a oscilar.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);
    m.nivel[1][0] = 8;
    m.nivel[1][1] = 5;
    m.nivel[1][4] = 8;
    m.nivel[2][2] = 3;
    m.nivel[3][7] = 8;

    const int antes = m.totalAgua();
    const int ticks = m.simular(false);

    CHECK(ticks < 500);
    CHECK(m.totalAgua() == antes);
}

TEST_CASE("el volumen se conserva en CADA tick, no solo al final") {
    // Un fallo que solo aparece a mitad de la simulacion (crear agua y
    // destruirla despues) daria el total correcto al final y estaria mal
    // igualmente.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);
    m.nivel[3][4] = 8;
    m.nivel[3][5] = 6;

    const int antes = m.totalAgua();
    for (int i = 0; i < 100; i++) {
        if (!m.tick(false)) break;
        REQUIRE(m.totalAgua() == antes);
    }
}

TEST_CASE("la tierra se satura y el agua deja de perderse") {
    // LA PROPIEDAD QUE SALVA LOS OCEANOS.
    //
    // Sobre tierra, el agua pierde volumen mientras el suelo bebe. Pero el
    // suelo se satura, y a partir de ahi el agua que queda ya no se va: el
    // total se estabiliza en vez de bajar hasta cero.
    Mundo m;
    m.ponerSuelo(BLOCK_DIRT);           // tierra: SI absorbe

    for (int x = 0; x < Mundo::ANCHO; x++) m.nivel[1][x] = 8;

    const int antes = m.totalAgua();
    m.simular(true);
    const int despues = m.totalAgua();

    // Bebio algo...
    CHECK(despues < antes);
    // ...pero solo un octavo por celda de suelo, no todo.
    CHECK(despues == antes - Mundo::ANCHO);
    // Y queda muchisima agua: el "mar" sigue ahi.
    CHECK(despues > 0);

    // Todo el suelo quedo mojado.
    for (int x = 0; x < Mundo::ANCHO; x++) {
        CHECK(estaMojado(m.material[0][x]));
    }

    // Y si se sigue simulando, ya no se pierde ni una gota mas.
    m.simular(true);
    CHECK(m.totalAgua() == despues);
}

TEST_CASE("un charco pequeno sobre tierra si se seca del todo") {
    // La otra cara de la moneda: poca agua sobre tierra seca desaparece.
    // Es lo que hace que derramar un tazon en el desierto no deje un charco
    // eterno.
    Mundo m;
    m.ponerSuelo(BLOCK_SAND);

    m.nivel[1][4] = 1;                  // una lamina finisima

    m.simular(true);

    CHECK(m.totalAgua() == 0);          // se la bebio la arena
    CHECK(estaMojado(m.material[0][4]));
}

TEST_CASE("un estanque de piedra no se filtra") {
    // La piedra es impermeable: es lo que permite construir algo que
    // contenga agua. Si se filtrara, no habria forma de hacer un deposito.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);
    // Paredes de piedra a los lados, formando un vaso.
    for (int y = 1; y < Mundo::ALTO; y++) {
        m.solido[y][3] = true; m.material[y][3] = BLOCK_STONE;
        m.solido[y][5] = true; m.material[y][5] = BLOCK_STONE;
    }

    m.nivel[1][4] = 8;
    const int antes = m.totalAgua();

    m.simular(true);                    // con absorcion activada

    CHECK(m.totalAgua() == antes);      // ni una gota
    CHECK(m.nivel[1][4] == 8);          // y sigue lleno
}

TEST_CASE("el agua se reparte hacia el lado que tiene hueco") {
    // Con una pared a un lado, el agua tiene que irse toda al otro. Si se
    // repartiera "en abanico" sin mirar, intentaria atravesar la pared.
    Mundo m;
    m.ponerSuelo(BLOCK_STONE);
    for (int y = 1; y < Mundo::ALTO; y++) {
        m.solido[y][3] = true;          // pared a la izquierda
        m.material[y][3] = BLOCK_STONE;
    }

    m.nivel[1][4] = 8;
    const int antes = m.totalAgua();
    m.simular(false);

    CHECK(m.totalAgua() == antes);
    // Nada atraveso la pared.
    CHECK(m.nivel[1][3] == 0);
    for (int x = 0; x < 3; x++) CHECK(m.nivel[1][x] == 0);
    // Y si se fue hacia la derecha.
    CHECK(m.nivel[1][5] > 0);
}
