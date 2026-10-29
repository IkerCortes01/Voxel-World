#include <doctest/doctest.h>
#include "BlockType.h"
#include "render/EspacioJugador.h"
#include <fstream>
#include <string>
#include <iterator>
#include <cmath>

// ============================================================================
// EL PILAR SE PARA CUANDO HAY TECHO
// ============================================================================
// PEDIDO: "si colocas bloques debajo de ti y subes, y arriba en la cabeza hay
// un nivel o bloque, que ya no te permita colocar bloques encima tuyo".
//
// EL CASO: construyendo un pilar bajo una cornisa, el ultimo bloque sube al
// jugador y su cabeza entra en el techo. Quedaba SOFOCADO -- dentro de la
// roca, sin aire y sin salida salvo picando.
//
// Hay DOS caminos que levantan al jugador y los dos necesitaban la guarda:
//
//   1. El pilar con BLOQUES ENTEROS (la rama de `intersects` en placeBlock).
//   2. El apilado NIVEL A NIVEL (`subirSiApilaBajoLosPies`).
//
// La logica vive en main.cpp, que el binario de tests no enlaza (ver el
// hallazgo I3 de la auditoria). Asi que aqui se comprueban dos cosas:
// que la guarda EXISTE en los dos caminos, y que la REGLA que modela es la
// correcta.
// ============================================================================

namespace {
std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != 0x5C) r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f) return std::string();
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return std::string();
#endif
}

// Replica de la regla: ¿hay techo justo encima de la cabeza tras subir?
// `techo` es lo que haya en la celda `cabeza + 1`.
//
// ⚠️ `isBlockSolid` vive en main.cpp y no se enlaza con los tests, asi que
// aqui se modela con lo que SI esta en BlockType.h. El criterio es el mismo:
// cuenta como techo lo que OCUPA su celda.
bool bloqueaElPilar(BlockType techo) {
    if (techo == BLOCK_AIR) return false;
    // La vegetacion se atraviesa: no sofoca a nadie. Se listan las dos que
    // usan los tests en vez de llamar a `isCrossSprite`, que vive en main.cpp.
    if (techo == BLOCK_TALLGRASS || techo == BLOCK_ORANGE_FLOWER) return false;
    // Agua y lava tampoco son techo solido.
    if (esAguaCualquiera(techo) || techo == BLOCK_LAVA) return false;
    // Lo demas ocupa la celda: cubo macizo, capa parcial o celda mixta. Un
    // nivel de 3 px basta para golpearse la cabeza, que es literalmente lo
    // que se pidio ("un nivel/bloque").
    return true;
}
} // namespace

TEST_CASE("Techo: la guarda existe en el pilar de bloques enteros") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Se mira la celda SIGUIENTE a la cabeza, no la cabeza (esa ya la cubre el
    // bucle de `pies..cabeza`).
    CHECK(src.find("const int sobreLaCabeza = cabeza + 1;") != std::string::npos);
    // Y un nivel parcial cuenta como techo.
    CHECK(src.find("isBlockSolid(techo) || esNivelParcial(techo)") != std::string::npos);
}

TEST_CASE("Techo: la guarda existe tambien en el apilado nivel a nivel") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // `subirSiApilaBajoLosPies` es el otro camino que levanta al jugador.
    const size_t f = src.find("auto subirSiApilaBajoLosPies");
    REQUIRE(f != std::string::npos);

    // Su guarda tiene que estar DENTRO de esa lambda, antes de mover al
    // jugador. Se busca entre su inicio y la linea que aplica la subida.
    const size_t aplica = src.find("state->player.position.y = nuevaY;", f);
    REQUIRE(aplica != std::string::npos);

    const std::string cuerpo = src.substr(f, aplica - f);
    INFO("la guarda del techo falta en subirSiApilaBajoLosPies");
    CHECK(cuerpo.find("sobreLaCabeza") != std::string::npos);
}

TEST_CASE("Techo: un bloque macizo encima bloquea el pilar") {
    CHECK(bloqueaElPilar(BLOCK_STONE));
    CHECK(bloqueaElPilar(BLOCK_DIRT));
    CHECK(bloqueaElPilar(BLOCK_PLANKS));
    CHECK(bloqueaElPilar(BLOCK_COBBLESTONE));
}

TEST_CASE("Techo: un NIVEL encima tambien bloquea") {
    // "arriba en la cabeza hay un nivel/bloque": los dos cuentan. Un nivel de
    // 3 px es suficiente para golpearse.
    for (int n = 1; n <= 7; ++n) {
        const BlockType capa = conNivel(BLOCK_DIRT, n);
        INFO("nivel ", n);
        CHECK(bloqueaElPilar(capa));
    }
}

TEST_CASE("Techo: una celda MIXTA encima tambien bloquea") {
    // Una mixta esta llena de suelo a techo: es tan solida como un cubo.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_STONE);
    REQUIRE(m != BLOCK_AIR);
    CHECK(bloqueaElPilar(m));
}

TEST_CASE("Techo: el AIRE no bloquea -- el pilar sigue creciendo") {
    // ⭐ LA CONTRAPARTIDA OBLIGATORIA.
    //
    // Al cerrar un caso es facil pasarse y romper el pilar saltando, que es
    // una mecanica central. Con cielo despejado tiene que poder subir siempre.
    CHECK_FALSE(bloqueaElPilar(BLOCK_AIR));
}

TEST_CASE("Techo: la vegetacion NO bloquea, se atraviesa") {
    // Una brizna de hierba o una flor sobre la cabeza no sofocan a nadie: el
    // jugador las atraviesa. Bloquear ahi seria impedir construir en un prado.
    CHECK_FALSE(bloqueaElPilar(BLOCK_TALLGRASS));
}

TEST_CASE("Techo: subir sin obstaculo sigue funcionando igual") {
    // La cuenta de CUANTO se sube no la toca este cambio: se comprueba que
    // sigue dando lo de siempre, para que el arreglo no haya movido la fisica.
    const Render::CajaJugador j =
        Render::cajaDelJugador(0.5f, 10.0f, 0.5f, 0.6f, 1.8f);

    // Un bloque entero bajo los pies levanta un bloque.
    const float sube = Render::subidaPorApilado(0, 10, 0, 1.0f, j);
    CHECK(sube == doctest::Approx(1.0f));
}
