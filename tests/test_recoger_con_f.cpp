#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <iterator>
#include <string>

// ============================================================================
// LOS PEDACITOS DEL SUELO SE RECOGEN CON F
// ============================================================================

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

TEST_CASE("Recoger con F: todos los pedacitos, y nada mas") {
    for (BlockType t : { BLOCK_PEDAZO_PIEDRA, BLOCK_PEDAZO_GRAVA, BLOCK_PEDAZO_PEDERNAL,
                         BLOCK_PEDAZO_CALIZA, BLOCK_PEDAZO_TIERRA, BLOCK_PEDAZO_COBRE,
                         BLOCK_PEDAZO_GOETHITA, BLOCK_PEDAZO_HEMATITE,
                         BLOCK_PEDAZO_LIMONITA, BLOCK_PEDAZO_NIEVE }) {
        INFO("id ", (int)t);
        CHECK(seRecogeConF(t));
    }
    CHECK_FALSE(seRecogeConF(BLOCK_STONE));
    CHECK_FALSE(seRecogeConF(BLOCK_DIRT));
    CHECK_FALSE(seRecogeConF(BLOCK_NOPAL_FRUTO));
}

TEST_CASE("Recoger con F: el clic no los pica, F los recoge y hay aviso") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    const size_t mina = src.find("void updateMining(GameState* state, float deltaTime) {");
    REQUIRE(mina != std::string::npos);
    CHECK(src.find("if (seRecogeConF(blockType)) {", mina) < src.find("void breakBlock(", mina));
    CHECK(src.find("recogerPedazoMirado(g_gameState);") != std::string::npos);
    CHECK(src.find("renderAvisoRecoger(g_gameState, width, height);") != std::string::npos);
    // Va al inventario; al suelo solo si no cabe.
    CHECK(src.find("if (!state->inventory.addItem(d.itemType, 1))") != std::string::npos);
}
