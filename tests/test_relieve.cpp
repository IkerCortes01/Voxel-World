#include <doctest/doctest.h>
#include "terrain/MountainGenerator.h"
#include <cmath>

// ============================================================================
// CERROS, MONTES Y VOLCANES INACTIVOS
// ============================================================================

using namespace TerrainGen;

namespace {
ClimateData tierraFirme() {
    ClimateData c{};
    c.continentalness = 0.75f;
    c.erosion = 0.6f;       // fuera de las cordilleras
    c.weirdness = 0.5f;
    return c;
}
} // namespace

TEST_CASE("Relieve: hay cerros, montes y volcanes, y los cerros son lo mas comun") {
    const MountainGenerator m(12345);
    int cuenta[4] = { 0, 0, 0, 0 };
    for (int i = -30; i < 30; ++i)
        for (int j = -30; j < 30; ++j) ++cuenta[m.ElevacionDeCelda(i, j).forma];
    const int total = 60 * 60;
    CHECK(cuenta[MountainGenerator::CERRO] > total / 4);
    CHECK(cuenta[MountainGenerator::MONTE] > total / 6);
    CHECK(cuenta[MountainGenerator::VOLCAN] > total / 20);
    CHECK(cuenta[MountainGenerator::VOLCAN] < cuenta[MountainGenerator::CERRO]);
}

TEST_CASE("Relieve: un volcan tiene crater, y su borde es lo mas alto") {
    const MountainGenerator m(777);
    int vistos = 0;
    for (int i = 0; i < 80 && vistos < 5; ++i) {
        const MountainGenerator::Elevacion e = m.ElevacionDeCelda(i, 3);
        if (e.forma != MountainGenerator::VOLCAN) continue;
        ++vistos;
        const float centro = m.PerfilDe(e, e.cx, e.cz);
        float borde = 0.0f;
        for (int k = 0; k < 16; ++k) {
            const float a = k * 0.3927f;
            borde = std::fmax(borde, m.PerfilDe(e, e.cx + std::cos(a) * e.radio * 0.17f,
                                                   e.cz + std::sin(a) * e.radio * 0.17f));
        }
        CHECK(borde > centro + e.alto * 0.15f);    // hundido en medio
        CHECK(borde > e.alto * 0.5f);              // y alto
        // Al pie, nada.
        CHECK(m.PerfilDe(e, e.cx + e.radio * 1.3f, e.cz) == 0.0f);
    }
    CHECK(vistos > 0);
}

TEST_CASE("Relieve: sin escalones y nada en el mar") {
    const MountainGenerator m(4242);
    const ClimateData c = tierraFirme();
    float maximo = 0.0f;
    for (int k = 0; k < 4000; ++k) {
        const float x = -2000.0f + k * 1.0f, z = 137.0f;
        const float a = m.GetLandformHeight(x, z, c), b = m.GetLandformHeight(x + 1.0f, z, c);
        CHECK(std::fabs(a - b) < 3.5f);   // como mucho una ladera empinada
        maximo = std::fmax(maximo, a);
    }
    CHECK(maximo > 15.0f);
    ClimateData mar = c;
    mar.continentalness = 0.3f;
    for (int k = 0; k < 200; ++k) CHECK(m.GetLandformHeight(k * 23.0f, k * 7.0f, mar) == 0.0f);
}
