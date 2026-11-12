#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// TABLONES CON NIVELES, Y DROPS PROPORCIONALES AL NIVEL
// ============================================================================
// Dos cambios que van juntos porque los dos tocan el apilado:
//
//   1. Los TABLONES volvieron a `tablaNiveles`. Sin niveles, el apilado los
//      trataba como cubos: entre una capa de 3 px y un tablon de 16 quedaban
//      13 px de aire, visibles Y fisicos. Los TRONCOS siguen fuera.
//
//   2. Picar un nivel N devuelve N items. Antes devolvia 1 siempre, asi que
//      apilar y volver a picar BORRABA material.
// ============================================================================

namespace {
std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f) return std::string();
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return std::string();
#endif
}
} // namespace

TEST_CASE("Tablones: los tres admiten niveles") {
    CHECK(admiteNiveles(BLOCK_PLANKS));
    CHECK(admiteNiveles(BLOCK_PLANKS_ENCINO));
    CHECK(admiteNiveles(BLOCK_PLANKS_OYAMEL));
}

TEST_CASE("Tablones: los TRONCOS siguen SIN niveles") {
    // El argumento original --"un tronco es un tronco entero"-- sigue vigente
    // para el tronco. Lo que cambio es que no se aplicaba igual al tablon.
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD));
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD_ENCINO));
    CHECK_FALSE(admiteNiveles(BLOCK_WOOD_OYAMEL));
}

TEST_CASE("Tablones: sus siete niveles se resuelven bien") {
    for (int n = 1; n <= 7; ++n) {
        const BlockType b = conNivel(BLOCK_PLANKS, n);
        INFO("nivel ", n);
        CHECK(esNivelParcial(b));
        CHECK(nivelDe(b) == n);
        CHECK(bloqueBaseDe(b) == BLOCK_PLANKS);
    }
    CHECK(conNivel(BLOCK_PLANKS, 8) == BLOCK_PLANKS);
}

TEST_CASE("Tablones: un tablon SE JUNTA con una capa, sin hueco") {
    // ⭐⭐ EL BUG REPORTADO, fijado.
    //
    // Antes `fusionarNiveles` devolvia NO_FUSIONA porque el tablon no contaba
    // como nivel: se quedaba ENCIMA y dejaba 13 px de aire.
    const Fusion f = fusionarNiveles(BLOCK_PLANKS, conNivel(BLOCK_DIRT, 3));
    CHECK(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(f.resultado != BLOCK_AIR);

    // Y entre dos capas de tablon se suman las alturas, sin hueco.
    const Fusion g = fusionarNiveles(conNivel(BLOCK_PLANKS, 2),
                                     conNivel(BLOCK_PLANKS, 3));
    CHECK(g.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(nivelDe(g.resultado) == 5);
    CHECK(bloqueBaseDe(g.resultado) == BLOCK_PLANKS);
}

TEST_CASE("Tablones: las lonchas ya no son bloques muertos") {
    // Al volver a la tabla dejaron de ser "huerfanas". Si siguieran marcadas,
    // el filtro del creativo las sacaria y `esNivelParcial` se contradiria con
    // `esNivelDeMaderaHuerfano`.
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_PLANKS_L1));
    CHECK_FALSE(esNivelDeMaderaHuerfano(BLOCK_PLANKO_L7));
    CHECK(esNivelParcial(BLOCK_PLANKS_L1));
    // Los troncos si siguen huerfanos.
    CHECK(esNivelDeMaderaHuerfano(BLOCK_WOOD_L1));
    CHECK(esNivelDeMaderaHuerfano(BLOCK_WOODO_L7));
}

TEST_CASE("Drops: un nivel apilado suelta tantas capas como octavos; un entero, uno") {
    // PEDIDO: "regresa el sistema que dependiendo del nivel te da la cantidad
    // de items por su tamano, pero [...] si hay un bloque completo [...] te de
    // uno". getBlockDrops vive en main.cpp, que no se enlaza con los tests: se
    // comprueba leyendo el fuente.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("UN NIVEL SUELTA TANTAS CAPAS COMO OCTAVOS TIENE; UN BLOQUE ENTERO, UNO") != std::string::npos);
    // Cualquier nivel, del jugador o del terreno, suelta tantas como mide.
    CHECK(src.find("capasQueSuelta = nivelDe(blockType);") != std::string::npos);
    // Y el pasto no se consigue: suelta tierra.
    CHECK(src.find("case BLOCK_GRASS:\n            drops.push_back({BLOCK_DIRT, 1, 1.0f});") != std::string::npos);
    CHECK(src.find("d.count *= capasQueSuelta") != std::string::npos);
    CHECK(src.find("d.itemType = conNivel(d.itemType, 1)") != std::string::npos);
}

TEST_CASE("Drops: el nivel vive en el ID, asi que natural y colocado son iguales") {
    // "Que se mantenga si se colocan bloques no naturales": una capa generada
    // por el mundo y otra apilada a mano son EL MISMO ID, asi que no hay forma
    // de que suelten distinto. Esto lo fija.
    for (int n = 1; n <= 7; ++n) {
        const BlockType natural  = conNivel(BLOCK_DIRT, n);
        const BlockType colocado = conNivel(bloqueBaseDe(natural), n);
        CHECK(natural == colocado);
        CHECK(nivelDe(colocado) == n);
    }
}

TEST_CASE("Tablones: solo con hacha o martillo, y en menos de un segundo") {
    // getBlockBreakTimeForMode vive en main.cpp, que no se enlaza con los
    // tests: se comprueba leyendo el fuente.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("constexpr float TABLON_HERRAMIENTA_BREAK_TIME = 0.8f;") != std::string::npos);
    const size_t regla = src.find("    if (esTablon(type)) {\n        return (esHacha(herramienta) || esMartillo(herramienta))");
    const size_t reglaCrlf = src.find("    if (esTablon(type)) {\r\n        return (esHacha(herramienta) || esMartillo(herramienta))");
    const size_t p = regla != std::string::npos ? regla : reglaCrlf;
    REQUIRE(p != std::string::npos);
    // Va dentro de getBlockBreakTimeForMode, despues del creativo y antes de
    // las reglas generales del hacha y el pico.
    const size_t funcion = src.find("float getBlockBreakTimeForMode(");
    const size_t hachaGeneral = src.find("if (isSurvival && esHacha(herramienta))");
    CHECK(funcion < p);
    CHECK(p < hachaGeneral);
}
