// LA OBSIDIANA.
// PEDIDO: "anade en la generacion de pedazos regados por el mapa, no tan comun
// pero tampoco rara, pedazos de obsidiana en el pasto, arena y todos lados,
// pero aparece mas en montes, crateres y cuevas, y que con el martillo hagas su
// version afilada".
#include "doctest/doctest.h"

#include "BlockType.h"
#include "FisicaCaida.h"
#include "terrain/MountainGenerator.h"
#include "ui/NombreItem.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <string>

namespace {
std::string leerO(const char* relativo) {
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + relativo, std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}
} // namespace

TEST_CASE("Obsidiana: el pedazo es un guijarro mas, y la afilada un item, con nombre e imagen") {
    CHECK(esGuijarro(BLOCK_PEDAZO_OBSIDIANA));
    CHECK_FALSE(esGuijarro(BLOCK_OBSIDIANA_AFILADA));       // la lasca no es un canto del suelo
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_PEDAZO_OBSIDIANA), "OBSIDIANA") == 0);
    CHECK(std::strcmp(NombreItem::nombreDeItem(BLOCK_OBSIDIANA_AFILADA), "OBSIDIANA AFILADA") == 0);
    CHECK((int)BLOCK_OBSIDIANA_AFILADA < (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_OBSIDIANA == (int)BLOCK_OBSIDIANA_AFILADA - 1);
    for (const char* png : { "Obsidiana.png", "Obsidiana afilada.png" }) {
        INFO(png);
        CHECK_FALSE(leerO((std::string("resourcepacks/Textures/Items/") + png).c_str()).empty());
    }
    const std::string src = leerO("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("{ BLOCK_PEDAZO_OBSIDIANA, \"Obsidiana.png\"") != std::string::npos);
    CHECK(src.find("{ BLOCK_OBSIDIANA_AFILADA, \"Obsidiana afilada.png\"") != std::string::npos);
    // Al recogerla da obsidiana, no pedernal.
    CHECK(src.find("drops.push_back({BLOCK_PEDAZO_OBSIDIANA, 1, 1.0f});") != std::string::npos);
}

TEST_CASE("Obsidiana: se afila con cualquiera de los dos martillos") {
    const std::string src = leerO("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    size_t recetas = 0;
    for (size_t k = src.find("CraftingRecipe recipe(BLOCK_OBSIDIANA_AFILADA, 1, true);"); k != std::string::npos;
         k = src.find("CraftingRecipe recipe(BLOCK_OBSIDIANA_AFILADA, 1, true);", k + 1)) {
        const std::string r = src.substr(k, 260);
        CHECK(r.find("recipe.pattern[1] = BLOCK_PEDAZO_OBSIDIANA;") != std::string::npos);
        CHECK((r.find("recipe.pattern[0] = BLOCK_MARTILLO_PIEDRA;") != std::string::npos ||
               r.find("recipe.pattern[0] = BLOCK_MARTILLO_PEDERNAL;") != std::string::npos));
        ++recetas;
    }
    CHECK(recetas == 2);
    // Y la afilada corta, como la lasca de pedernal.
    size_t filos = 0;
    for (size_t k = src.find("herramienta == BLOCK_OBSIDIANA_AFILADA ||"); k != std::string::npos;
         k = src.find("herramienta == BLOCK_OBSIDIANA_AFILADA ||", k + 1)) ++filos;
    CHECK(filos == 3);
}

TEST_CASE("Obsidiana: sale en todos lados, y mas en montes, crateres y cuevas") {
    const std::string src = leerO("src/main.cpp");
    REQUIRE_FALSE(src.empty());
    // En cualquier suelo: entra en la lista sin mirar que hay debajo.
    CHECK((src.find("                        meter(BLOCK_PEDAZO_OBSIDIANA, 2);\n") != std::string::npos ||
           src.find("                        meter(BLOCK_PEDAZO_OBSIDIANA, 2);\r\n") != std::string::npos));
    // Y cada vez mas: montana, monte o volcan, crater.
    CHECK(src.find("if (enMontana) meter(BLOCK_PEDAZO_OBSIDIANA, 3);") != std::string::npos);
    CHECK(src.find("if (enMonte)   meter(BLOCK_PEDAZO_OBSIDIANA, 5);") != std::string::npos);
    CHECK(src.find("if (enCrater)  meter(BLOCK_PEDAZO_OBSIDIANA, 16);") != std::string::npos);
    // En el crater, ademas, hay muchos mas pedazos.
    CHECK(src.find("const unsigned cada = enCrater ? 9u") != std::string::npos);
    // En las cuevas: 4 de 14.
    const size_t cueva = src.find("static const BlockType DE_CUEVA[14] = {");
    REQUIRE(cueva != std::string::npos);
    const std::string lista = src.substr(cueva, 700);
    size_t n = 0;
    for (size_t k = lista.find("BLOCK_PEDAZO_OBSIDIANA"); k != std::string::npos;
         k = lista.find("BLOCK_PEDAZO_OBSIDIANA", k + 1)) ++n;
    CHECK(n == 4);
    CHECK(src.find("DE_CUEVA[(hc >> 9) % 14u]") != std::string::npos);
}

TEST_CASE("Relieve: se sabe donde hay monte, donde volcan y donde esta el crater") {
    using TerrainGen::MountainGenerator;
    const MountainGenerator m(12345);
    int vistos[4] = { 0, 0, 0, 0 };
    int crateres = 0, volcanes = 0;
    // Un barrido ancho: tiene que haber de todo.
    for (int i = -20; i <= 20; ++i)
        for (int j = -20; j <= 20; ++j) {
            const MountainGenerator::Elevacion e = m.ElevacionDeCelda(i, j);
            if (e.forma == MountainGenerator::NADA) continue;
            // En su centro manda ella (o algo mayor que la pisa).
            float u = 9.0f;
            const int forma = m.RelieveEn(e.cx, e.cz, &u);
            CHECK(forma >= e.forma);
            if (forma == e.forma) CHECK(u < 0.05f);
            ++vistos[forma];
            if (e.forma == MountainGenerator::VOLCAN) {
                ++volcanes;
                if (m.EnCrater(e.cx, e.cz)) ++crateres;
                // A media ladera ya no es crater.
                CHECK_FALSE(m.EnCrater(e.cx + e.radio * 0.6f, e.cz));
            } else {
                CHECK_FALSE((forma != MountainGenerator::VOLCAN && m.EnCrater(e.cx, e.cz)));
            }
            // Muy lejos de todo no hay relieve de esta celda.
            float lejos = 0.0f;
            (void)m.RelieveEn(e.cx, e.cz, &lejos);
            CHECK(lejos >= 0.0f);
        }
    CHECK(vistos[MountainGenerator::CERRO] > 0);
    CHECK(vistos[MountainGenerator::MONTE] > 0);
    CHECK(vistos[MountainGenerator::VOLCAN] > 0);
    REQUIRE(volcanes > 0);
    CHECK(crateres == volcanes);            // todo volcan tiene su crater en la cima
    // El crater es una parte pequena del volcan.
    CHECK(MountainGenerator::U_CRATER < 0.30f);
    CHECK(MountainGenerator::U_CRATER > 0.15f);
    // La misma semilla, el mismo relieve.
    const MountainGenerator otra(12345);
    float u1 = 0.0f, u2 = 0.0f;
    CHECK(m.RelieveEn(100.0f, -340.0f, &u1) == otra.RelieveEn(100.0f, -340.0f, &u2));
    CHECK(u1 == u2);
}

TEST_CASE("Obsidiana: pesa como vidrio volcanico") {
    const std::string fis = leerO("src/FisicaCaida.h");
    CHECK(fis.find("case BLOCK_PEDAZO_OBSIDIANA:  return 2400.0f;") != std::string::npos);
}
