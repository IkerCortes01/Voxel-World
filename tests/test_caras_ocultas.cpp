#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// CARAS OCULTAS: VER EL INTERIOR DE UN BLOQUE POR EL HUECO QUE DEJAN
// ============================================================================
//
// EL PROBLEMA QUE RESUELVE
// Un bloque puede declarar que no dibuja alguna de sus caras. Hasta ahora eso
// dejaba un agujero por el que se veia el VACIO, no el interior: el mundo se
// dibuja con GL_CULL_FACE(GL_BACK), asi que OpenGL descarta toda cara cuyo
// recorrido se vea horario desde el ojo -- y desde dentro de un bloque, las
// cinco caras restantes se ven exactamente asi. El bloque quedaba hueco y
// transparente.
//
// La solucion es emitir cada cara DOS veces en los bloques huecos: la normal,
// visible desde fuera, y su gemela con los vertices invertidos, visible desde
// dentro. Ver emitQuad() en main.cpp.
//
// ----------------------------------------------------------------------------
// QUE FIJAN ESTOS TESTS
// ----------------------------------------------------------------------------
// La parte de GPU no se puede probar sin contexto de OpenGL, asi que estos
// tests cubren la CAPA DE DATOS, que es donde se decide todo:
//
//   1. Que la mascara de caras y sus consultas son coherentes entre si.
//   2. Que un bloque normal NO paga nada: sin caras ocultas, sin gemelas.
//   3. Que el ocote conserva su comportamiento -- era un caso hardcodeado en
//      el mesher y ahora sale de la tabla; si la migracion se hizo mal, la
//      copa del arbol vuelve a tener suelo y nadie se entera hasta verlo.
//   4. Que las tres funciones publicas no se contradicen para ningun bloque
//      del enum, ni se salen de rango con indices invalidos.

TEST_CASE("Las constantes de cara cubren las seis direcciones sin solaparse") {
    // Cada bit corresponde a un indice `dir` del mesher (ver DIR_VEC):
    //   0 = +Y arriba   1 = -Y abajo   2 = +Z norte
    //   3 = -Z sur      4 = +X este    5 = -X oeste
    CHECK(CARA_ARRIBA == (1u << 0));
    CHECK(CARA_ABAJO  == (1u << 1));
    CHECK(CARA_NORTE  == (1u << 2));
    CHECK(CARA_SUR    == (1u << 3));
    CHECK(CARA_ESTE   == (1u << 4));
    CHECK(CARA_OESTE  == (1u << 5));

    // Ninguna comparte bit con otra: si dos coincidieran, ocultar una cara
    // ocultaria tambien la otra sin que nadie lo pidiera.
    const CaraMask todas[6] = { CARA_ARRIBA, CARA_ABAJO, CARA_NORTE,
                                CARA_SUR, CARA_ESTE, CARA_OESTE };
    for (int i = 0; i < 6; ++i)
        for (int j = i + 1; j < 6; ++j)
            CHECK((todas[i] & todas[j]) == 0u);

    CHECK(CARA_NINGUNA == 0u);
}

TEST_CASE("Un bloque solido normal no oculta ninguna cara") {
    // La piedra es el caso de referencia: seis caras, ningun interior, y por
    // tanto cero geometria extra. Si esto fallara, TODO el mundo empezaria a
    // emitir gemelas y el coste se dispararia.
    CHECK(carasOcultas(BLOCK_STONE) == CARA_NINGUNA);
    CHECK(muestraInterior(BLOCK_STONE) == false);

    for (int dir = 0; dir < 6; ++dir)
        CHECK(caraEstaOculta(BLOCK_STONE, dir) == false);

    CHECK(carasOcultas(BLOCK_DIRT) == CARA_NINGUNA);
    CHECK(muestraInterior(BLOCK_DIRT) == false);
    CHECK(muestraInterior(BLOCK_AIR) == false);
}

TEST_CASE("Las copas de ocote no tienen suelo y ensenan su interior") {
    // Este comportamiento existia antes como un `if` hardcodeado dentro del
    // mesher, atado al indice de cara 1. Al pasarlo a la tabla hay que
    // comprobar que las cuatro variantes siguen igual: son las que forman la
    // copa, y si una se queda fuera el follaje muestra una tapa lisa donde
    // deberia verse el ramaje.
    const BlockType copas[4] = {
        BLOCK_LEAVES_OCOTE_CHINO,
        BLOCK_LEAVES_OCOTE_CHINO_RAMA,
        BLOCK_LEAVES_OCOTE,
        BLOCK_LEAVES_OCOTE_RAMA,
    };

    for (BlockType hoja : copas) {
        CHECK(carasOcultas(hoja) == CARA_ABAJO);

        // dir 1 es la cara de ABAJO: la unica que se oculta.
        CHECK(caraEstaOculta(hoja, 1) == true);

        // Las otras cinco se dibujan con normalidad.
        CHECK(caraEstaOculta(hoja, 0) == false);   // arriba
        CHECK(caraEstaOculta(hoja, 2) == false);   // norte
        CHECK(caraEstaOculta(hoja, 3) == false);   // sur
        CHECK(caraEstaOculta(hoja, 4) == false);   // este
        CHECK(caraEstaOculta(hoja, 5) == false);   // oeste

        // Y como oculta una cara, sus caras necesitan gemela interior: es lo
        // que hace que al mirar hacia arriba se vea el interior de la copa.
        CHECK(muestraInterior(hoja) == true);
    }
}

TEST_CASE("Un indice de cara fuera de rango no se sale de la mascara") {
    // El mesher siempre pasa 0..5, pero la funcion es publica y un indice
    // invalido no debe leer bits que no existen ni desplazar de mas.
    CHECK(caraEstaOculta(BLOCK_LEAVES_OCOTE, -1) == false);
    CHECK(caraEstaOculta(BLOCK_LEAVES_OCOTE, 6)  == false);
    CHECK(caraEstaOculta(BLOCK_LEAVES_OCOTE, 99) == false);
    CHECK(caraEstaOculta(BLOCK_STONE, -100) == false);
}

TEST_CASE("Las tres consultas concuerdan para todos los bloques del juego") {
    // Barrido completo del enum: las tres funciones publicas describen el
    // mismo hecho desde angulos distintos, asi que no pueden contradecirse
    // para ningun bloque -- ni para los que se anadan despues.
    int conInterior = 0;

    for (int i = 0; i <= BLOCK_TYPE_MAX; ++i) {
        const BlockType b = static_cast<BlockType>(i);
        const CaraMask m = carasOcultas(b);

        // muestraInterior() es exactamente "tiene alguna cara oculta".
        CHECK(muestraInterior(b) == (m != CARA_NINGUNA));

        // caraEstaOculta() debe coincidir bit a bit con la mascara.
        for (int dir = 0; dir < 6; ++dir) {
            const bool porBit = (m & (1u << dir)) != 0u;
            CHECK(caraEstaOculta(b, dir) == porBit);
        }

        // Ningun bloque puede ocultar sus SEIS caras: seria invisible por
        // fuera y por dentro, y ocuparia sitio sin dibujar nada.
        CHECK(m != (CARA_ARRIBA | CARA_ABAJO | CARA_NORTE |
                    CARA_SUR | CARA_ESTE | CARA_OESTE));

        // La mascara no usa bits por encima de los seis validos.
        CHECK((m & ~0x3Fu) == 0u);

        if (m != CARA_NINGUNA) ++conInterior;
    }

    // Los bloques huecos son la excepcion, no la regla: si un cambio los
    // multiplicara, cada uno paga hasta cinco quads extra y conviene enterarse
    // aqui y no por una caida de FPS.
    CHECK(conInterior > 0);            // el ocote, al menos
    CHECK(conInterior < 32);           // pero no medio juego
}
