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

TEST_CASE("Drops: el motor multiplica el drop por los octavos del nivel") {
    // getBlockDrops vive en main.cpp, que no se enlaza con los tests: se
    // comprueba leyendo el fuente (ver el hallazgo I3 de la auditoria).
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Se lee el nivel ANTES de normalizar a bloque entero...
    CHECK(src.find("octavosDelNivel = nivelDe(blockType)") != std::string::npos);
    // ...y se multiplica AL FINAL, para que valga en todos los caminos de drop
    // y no solo en el `default`.
    CHECK(src.find("d.count *= octavosDelNivel") != std::string::npos);
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
