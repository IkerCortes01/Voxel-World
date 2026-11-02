#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <iterator>
#include <string>

// ============================================================================
// EL ITEM NOPAL DE CASTILLA: TODO LO DEL NOPAL, MENOS UNIRSE A LA PLANTA
// ============================================================================
// PEDIDO: "si esta en agua se convierta en nopal mojado, pueda sacar baba de
// nopal etc y todo lo del nopal, excepto que se junte y una como la planta".

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

// Un ID cualquiera de agua con nivel.
BlockType aguaConNivel() {
    return (BlockType)(BLOQUE_COMPUESTO_BASE_ID +
                       BLOQUE_COMPUESTO_FAM_AGUA * BLOQUE_COMPUESTO_ESTADOS + 3);
}

} // namespace

TEST_CASE("Nopal de Castilla: colocado seco queda como pieza que no es planta") {
    CHECK(nopalAlColocar(false) == BLOCK_NOPAL_PUESTO);
    CHECK(esNopalDeCastilla(BLOCK_NOPAL_PUESTO));
    CHECK(esNopalDeCastilla(BLOCK_NOPAL_FRUTO));
    CHECK_FALSE(esNopalDeCastilla(BLOCK_NOPAL_CLADODIO));
}

TEST_CASE("Nopal de Castilla: colocado en agua sale mojado") {
    CHECK(nopalAlColocar(true) == BLOCK_NOPAL_MOJADO);
}

TEST_CASE("Nopal de Castilla: cuenta cualquier agua, tambien la que tiene nivel") {
    // Antes solo se miraba BLOCK_WATER: el agua que fluye o la de un cubo no
    // mojaba nada.
    REQUIRE(esAguaVolumen(aguaConNivel()));
    for (BlockType agua : { BLOCK_WATER, aguaConNivel() }) {
        CHECK(tocaAgua([&](int, int, int) { return agua; }));
        CHECK(tocaAgua([&](int dx, int, int) { return dx == 1 ? agua : BLOCK_AIR; }));
        CHECK(tocaAgua([&](int, int dy, int) { return dy == -1 ? agua : BLOCK_AIR; }));
        // En diagonal no se toca.
        CHECK_FALSE(tocaAgua([&](int dx, int dy, int) {
            return (dx == 1 && dy == 1) ? agua : BLOCK_AIR;
        }));
    }
    CHECK_FALSE(tocaAgua([](int, int, int) { return BLOCK_AIR; }));
}

TEST_CASE("Nopal de Castilla: el colocado esta en el enum y no sale en creativo") {
    CHECK((int)BLOCK_NOPAL_PUESTO <= (int)BLOCK_TYPE_MAX);
    CHECK(esBloqueRetirado(BLOCK_NOPAL_PUESTO));
    // Los mundos guardados: va al final, no corre ningun ID.
    CHECK((int)BLOCK_NOPAL_PUESTO == (int)BLOCK_TECHO_SUELO_L7 + 1);
}

TEST_CASE("Nopal de Castilla: el motor usa estas reglas") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // No se une: ni ella a la mata, ni la mata a ella.
    const size_t e = src.find("inline bool nopalEncadena(");
    REQUIRE(e != std::string::npos);
    const std::string encadena = src.substr(e, 1600);
    CHECK(encadena.find("propio == BLOCK_NOPAL_PUESTO || propio == BLOCK_NOPAL_MOJADO")
          != std::string::npos);
    CHECK(encadena.find("b == BLOCK_NOPAL_PUESTO") == std::string::npos);

    // Al colocar se decide con cualquier agua.
    CHECK(src.find("blockToPlace = nopalAlColocar(tocaAgua(") != std::string::npos);

    // Roto vuelve a ser el item.
    CHECK(src.find("case BLOCK_NOPAL_PUESTO:\n            drops.push_back({BLOCK_NOPAL_FRUTO, 1, 1.0f});")
          != std::string::npos);
    CHECK(src.find("case BLOCK_NOPAL_PUESTO:\n                return BLOCK_NOPAL_FRUTO;")
          != std::string::npos);

    // Tirado al agua, o si el agua llega despues, tambien se moja.
    CHECK(src.find("items[i].blockType = BLOCK_NOPAL_MOJADO;") != std::string::npos);
    CHECK(src.find("EL NOPAL REACCIONA AL AGUA AUNQUE LE LLEGUE DESPUES") != std::string::npos);

    // Y la cadena sigue: mojado -> tiras -> baba y espinas.
    CHECK(src.find("CraftingRecipe recipe(BLOCK_NOPAL_TIRAS, 4, true);") != std::string::npos);
    CHECK(src.find("state->spawnItem(pos, BLOCK_NOPAL_BABA);") != std::string::npos);
}
