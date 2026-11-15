#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <iterator>
#include <string>

// ============================================================================
// LA HOJA DE MAGUEY SE COLOCA COMO EL NOPAL
// ============================================================================
// PEDIDO: "que sea colocable el maguey como el nopal".

namespace {
std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}
} // namespace

TEST_CASE("Maguey: cada especie se coloca como su propia pieza") {
    CHECK(agaveAlColocar(BLOCK_IXTLE_HOJA) == BLOCK_MAGUEY_PUESTO);
    CHECK(agaveAlColocar(BLOCK_PENCA_AGAVE_AZUL) == BLOCK_AGAVE_AZUL_PUESTO);
    CHECK(esAgavePuesto(BLOCK_MAGUEY_PUESTO));
    CHECK(esAgavePuesto(BLOCK_AGAVE_AZUL_PUESTO));
    CHECK_FALSE(esAgavePuesto(BLOCK_IXTLE_HOJA));
}

TEST_CASE("Maguey: lo colocado no sale en creativo y va al final del enum") {
    CHECK(esBloqueRetirado(BLOCK_MAGUEY_PUESTO));
    CHECK(esBloqueRetirado(BLOCK_AGAVE_AZUL_PUESTO));
    CHECK((int)BLOCK_MAGUEY_PUESTO == (int)BLOCK_NOPAL_PUESTO + 1);
    CHECK((int)BLOCK_AGAVE_AZUL_PUESTO <= (int)BLOCK_TYPE_MAX);
}

TEST_CASE("Maguey: el motor la coloca, la dibuja acostada y la devuelve") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("const BlockType posado = posadoAlColocar(blockToPlace, tocaAgua(") != std::string::npos);
    CHECK(src.find("if (esPiezaPosada(tipo)) return true;") != std::string::npos);
    CHECK(src.find("drops.push_back({BLOCK_IXTLE_HOJA, 1, 1.0f});") != std::string::npos);
    CHECK(src.find("drops.push_back({BLOCK_PENCA_AGAVE_AZUL, 1, 1.0f});") != std::string::npos);

    // PEDIDO: "elimina que el maguey copie los sistemas de agua, sin espinas etc
    // del nopal". El barrido del agua convertia la hoja en nopal mojado, porque
    // pencaPuestaPorJugador tambien la da por suelta.
    const size_t b = src.find("EL NOPAL REACCIONA AL AGUA AUNQUE LE LLEGUE DESPUES");
    REQUIRE(b != std::string::npos);
    const std::string barrido = src.substr(b, 3000);
    CHECK(barrido.find("const bool suelto = esNopalDeCastilla(b) &&") != std::string::npos);
    CHECK(src.find("esAgavePuesto(blockToPlace)") == std::string::npos);

    // El item ya no esta en la lista de los que no se colocan.
    const size_t i = src.find("bool isPlaceableItem(BlockType type) {");
    REQUIRE(i != std::string::npos);
    const std::string lista = src.substr(i, 3500);
    CHECK(lista.find("case BLOCK_IXTLE_HOJA:") == std::string::npos);
    CHECK(lista.find("case BLOCK_PENCA_AGAVE_AZUL:") == std::string::npos);
}

// ⭐ UN SOLO CODIGO PARA EL NOPAL Y EL MAGUEY. PEDIDO: "que el maguey pueda ser
// colocado como la Penca de Nopal y tengan el mismo codigo de colocarlo, y
// arregla que la penca de nopal no coloque el bloque de nivel raro, que de el
// item 3D acostado y el maguey tambien".
TEST_CASE("Posados: los cuatro items se convierten con la misma funcion, y vuelven a ser su item") {
    const BlockType ITEMS[4] = { BLOCK_NOPAL_FRUTO, BLOCK_NOPAL_CLADODIO, BLOCK_IXTLE_HOJA, BLOCK_PENCA_AGAVE_AZUL };
    const BlockType PUESTOS[4] = { BLOCK_NOPAL_PUESTO, BLOCK_PENCA_NOPAL_PUESTA, BLOCK_MAGUEY_PUESTO,
                                   BLOCK_AGAVE_AZUL_PUESTO };
    for (int i = 0; i < 4; ++i) {
        CHECK(posadoAlColocar(ITEMS[i], false) == PUESTOS[i]);
        CHECK(esItemPosado(PUESTOS[i]));
        CHECK_FALSE(esItemPosado(ITEMS[i]));
        CHECK(itemDePosado(PUESTOS[i]) == ITEMS[i]);        // lo que se ve y lo que devuelve
        CHECK(esBloqueRetirado(PUESTOS[i]));                // no sale en creativo
        CHECK((int)PUESTOS[i] <= (int)BLOCK_TYPE_MAX);
    }
    // Lo demas se coloca como es.
    CHECK(posadoAlColocar(BLOCK_STONE, false) == BLOCK_STONE);
    CHECK(posadoAlColocar(BLOCK_PLANKS, true) == BLOCK_PLANKS);
    CHECK(itemDePosado(BLOCK_STONE) == BLOCK_AIR);
    // La penca de nopal ya no es la base del nopal al colocarla.
    CHECK(posadoAlColocar(BLOCK_NOPAL_CLADODIO, false) != BLOCK_NOPAL_BASE_TIERRA);
    CHECK(posadoAlColocar(BLOCK_NOPAL_CLADODIO, true) == BLOCK_PENCA_NOPAL_PUESTA);
    // El nuevo va al final del enum: los mundos guardados no cambian.
    CHECK((int)BLOCK_PENCA_NOPAL_PUESTA <= (int)BLOCK_TYPE_MAX);
}

TEST_CASE("Posados: el motor las coloca, las dibuja y las cuenta con un solo camino") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    // Al colocar ya no se pone la base del nopal.
    const size_t c = src.find("if (blockToPlace == BLOCK_PENCA_NOPAL_PUESTA) {");
    REQUIRE(c != std::string::npos);
    const std::string plantar = src.substr(c, 700);
    CHECK(plantar.find("baseParaSuelo") == std::string::npos);
    CHECK(plantar.find("g_nopalesCreciendo.push_back") != std::string::npos);   // pero sigue plantada
    CHECK(src.find("if (blockToPlace == BLOCK_NOPAL_CLADODIO) {") == std::string::npos);
    // Brota despues: es el crecimiento quien la cambia por la base.
    CHECK(src.find("const bool semilla = enCelda == BLOCK_PENCA_NOPAL_PUESTA;") != std::string::npos);
    // El mallado no las lleva y el dibujo es el del item, para las cuatro.
    CHECK(src.find("if (esItemPosado(block)) continue;") != std::string::npos);
    CHECK(src.find("dibujarItem3D(suItem, luzDelItem(") != std::string::npos);
    CHECK(src.find("dibujarItem3D(BLOCK_NOPAL_FRUTO, luzDelItem(") == std::string::npos);
    CHECK(src.find("if (esItemPosado(blockToPlace))") != std::string::npos);
    // Al romperla vuelve la penca.
    CHECK(src.find("drops.push_back({BLOCK_NOPAL_CLADODIO, 1, 1.0f});") != std::string::npos);
}
