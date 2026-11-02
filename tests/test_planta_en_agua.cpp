#include <doctest/doctest.h>
#include "AguaFlujo.h"
#include <initializer_list>

// ============================================================================
// UNA PIEZA DEL NOPAL O DEL MAGUEY DENTRO DEL AGUA
// ============================================================================
// PEDIDO: "que el maguey y nopal pueda estar dentro del agua y que no genere aire".

using namespace Compuesto;

namespace {
template <typename F>
BlockType paso(F get) { return AguaFlujo::estadoNuevo(get); }
}

TEST_CASE("Planta en agua: el ID guarda la planta y el agua") {
    for (BlockType p : { BLOCK_NOPAL_MOJADO, BLOCK_MAGUEY_PUESTO,
                         BLOCK_AGAVE_AZUL_PUESTO, BLOCK_NOPAL_TIRAS }) {
        for (int n = 1; n <= 8; ++n) {
            const BlockType c = plantaConAgua(p, n);
            CHECK(esPlantaEnAgua(c));
            CHECK(plantaSeca(c) == p);
            CHECK(nivelDePlantaEnAgua(c) == n);
            CHECK(esAguaCualquiera(c));
            CHECK_FALSE(Agua::esAgua(c));
        }
    }
    CHECK(plantaSeca(BLOCK_STONE) == BLOCK_STONE);
    CHECK_FALSE(esPlantaEnAgua(Agua::nuevo(8)));
    CHECK_FALSE(esPlantaEnAgua(BLOCK_WATER));
}

TEST_CASE("Planta en agua: la familia coincide con el enum y no pisa a nadie") {
    CHECK((int)FAM_PLANTA_EN_AGUA == BLOQUE_COMPUESTO_FAM_PLANTA_EN_AGUA);
    const BlockType c = plantaConAgua(BLOCK_MAGUEY_PUESTO, 8);
    CHECK(familiaDe(c) == FAM_PLANTA_EN_AGUA);
    CHECK(familiaTieneModelo(FAM_PLANTA_EN_AGUA));
    CHECK_FALSE(Nopal::esNopal(c));
    CHECK_FALSE(esAguaVolumen(c));
}

TEST_CASE("Planta en agua: se puede con las piezas, no con cualquier bloque") {
    CHECK(sePuedeInundar(BLOCK_NOPAL_MOJADO));
    CHECK(sePuedeInundar(BLOCK_NOPAL_TIRAS));
    CHECK(sePuedeInundar(BLOCK_MAGUEY_PUESTO));
    CHECK(sePuedeInundar(BLOCK_AGAVE_AZUL_PUESTO));
    CHECK_FALSE(sePuedeInundar(BLOCK_STONE));
    CHECK_FALSE(sePuedeInundar(BLOCK_DIRT));
}

TEST_CASE("Planta en agua: en una fuente es fuente y alimenta a sus vecinas") {
    const BlockType c = plantaConAgua(BLOCK_MAGUEY_PUESTO, 8);
    CHECK(AguaFlujo::esFuente(c));
    CHECK(AguaFlujo::fuerza(c) == 8);
    CHECK(Agua::alturaVisual(c) == doctest::Approx(1.0f));

    // Una celda de flujo al lado sigue viva: no se abre un hueco.
    auto vecina = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return Agua::nuevo(7);
        if (dx == 1 && dy == 0 && dz == 0) return c;
        return dy < 0 ? BLOCK_STONE : BLOCK_AIR;
    };
    CHECK(paso(vecina) == Agua::nuevo(7));

    // Y se extiende al aire como cualquier fuente, sin perder la planta.
    auto sola = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return c;
        return dy < 0 ? BLOCK_STONE : BLOCK_AIR;
    };
    const AguaFlujo::Plan p = AguaFlujo::planear(sola);
    CHECK(p.propia == c);
    CHECK(p.n == 4);
}

TEST_CASE("Planta en agua: en un flujo sigue el nivel y al secarse queda la planta") {
    const BlockType c = plantaConAgua(BLOCK_NOPAL_MOJADO, 5);
    CHECK_FALSE(AguaFlujo::esFuente(c));
    CHECK(Agua::alturaVisual(c) == doctest::Approx(5.0f / 8.0f));

    auto conFuente = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return c;
        if (dx == 1 && dy == 0 && dz == 0) return Agua::nuevo(8);
        return dy < 0 ? BLOCK_STONE : BLOCK_AIR;
    };
    CHECK(paso(conFuente) == plantaConAgua(BLOCK_NOPAL_MOJADO, 7));

    auto sinNada = [&](int dx, int dy, int dz) -> BlockType {
        if (dx == 0 && dy == 0 && dz == 0) return c;
        return dy < 0 ? BLOCK_STONE : BLOCK_AIR;
    };
    CHECK(paso(sinNada) == BLOCK_NOPAL_MOJADO);
    CHECK(AguaFlujo::planear(sinNada).n == 0);
}

TEST_CASE("Planta en agua: el agua no entra en una planta seca ni pisa una inundada") {
    CHECK_FALSE(AguaFlujo::puedePasar(BLOCK_MAGUEY_PUESTO));
    CHECK_FALSE(AguaFlujo::puedePasar(plantaConAgua(BLOCK_MAGUEY_PUESTO, 8)));
}

TEST_CASE("Planta en agua: la pieza que cambia sola conserva el agua") {
    const BlockType t = plantaConAgua(BLOCK_NOPAL_TIRAS, 8);
    CHECK(conLaMismaAgua(t, BLOCK_NOPAL_SIN_BABA) ==
          plantaConAgua(BLOCK_NOPAL_SIN_BABA, 8));
    CHECK(conLaMismaAgua(BLOCK_NOPAL_TIRAS, BLOCK_NOPAL_SIN_BABA) ==
          BLOCK_NOPAL_SIN_BABA);
}
