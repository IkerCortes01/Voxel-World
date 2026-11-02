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
    CHECK(src.find("blockToPlace = agaveAlColocar(blockToPlace);") != std::string::npos);
    CHECK(src.find("if (esAgavePuesto(tipo)) return true;") != std::string::npos);
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
