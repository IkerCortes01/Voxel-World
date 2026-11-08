#include <doctest/doctest.h>
#include "render/HojasSecas.h"
#include <cmath>

using namespace HojasSecas;

namespace {
// Suelo macizo hasta y=10, una losa de nivel 3 en x=3 y una copa de arbol
// en y=16..18 sobre x,z en [0, 4).
float mundo(int x, int y, int z) {
    if (y < 10) return 1.0f;
    if (y == 10 && x == 3) return 0.375f;
    if (y >= 16 && y <= 18 && x >= 0 && x < 4 && z >= 0 && z < 4) return -1.0f;
    return 0.0f;
}
bool esHoja(int x, int y, int z) { return mundo(x, y, z) < 0.0f; }

void simular(Sistema& s, float segundos, float viento = 0.5f, float racha = 0.3f,
             float jx = 50.0f, float jvx = 0.0f) {
    for (int k = 0; k < (int)(segundos * 60.0f); ++k)
        s.avanzar(1.0f / 60.0f, viento, viento * 0.3f, racha, jx, 10.0f, 2.0f, jvx, 0.0f, mundo);
}
} // namespace

TEST_CASE("Hojas secas: caen despacio, en vaiven, y quedan tiradas en el suelo") {
    static Sistema s;
    s.soltar(1, 16, 1);
    Hoja& o = s.h[1];
    REQUIRE(o.estado == CAYENDO);
    // Medio segundo despues no ha caido como una piedra (6 m/s2 seria 0.75).
    simular(s, 0.5f, 0.0f, 0.0f, 1.0f);
    CHECK(o.y > 16.0f - 0.75f);
    CHECK(o.estado == CAYENDO);
    // Y acaba en el suelo, plana, encima y no dentro.
    simular(s, 12.0f, 0.0f, 0.0f, 1.0f);
    CHECK(o.estado == TIRADA);
    CHECK(o.y >= 10.0f);
    CHECK(o.y < 10.05f);
    CHECK(std::fabs(o.cabeceo) < 0.2f);
}

TEST_CASE("Hojas secas: se posan sobre una losa de nivel, no dentro") {
    static Sistema s;
    s.soltar(3, 14, 1);
    Hoja& o = s.h[1];
    o.x = 3.5f; o.z = 1.5f;
    for (int k = 0; k < 60 * 12 && o.estado != TIRADA; ++k)
        s.avanzar(1.0f / 60.0f, 0.0f, 0.0f, 0.0f, 3.0f, 10.0f, 2.0f, 0.0f, 0.0f, mundo);
    REQUIRE(o.estado == TIRADA);
    if ((int)std::floor(o.x) == 3) CHECK(o.y >= 10.375f);
    else CHECK(o.y >= 10.0f);
}

TEST_CASE("Hojas secas: el arbol las suelta, se amontonan y no pasan del maximo") {
    static Sistema s;
    // Jugador bajo el arbol: con muchos intentos salen hojas de la copa.
    for (int k = 0; k < 600; ++k) {
        s.buscarArboles(2.0f, 12.0f, 2.0f, 400, esHoja, mundo);
        s.avanzar(1.0f / 30.0f, 0.5f, 0.2f, 0.2f, 2.0f, 10.0f, 2.0f, 0.0f, 0.0f, mundo);
    }
    int tiradas = 0, total = 0;
    for (const Hoja& o : s.h) {
        if (o.estado == LIBRE) continue;
        ++total;
        if (o.estado == TIRADA) ++tiradas;
        CHECK(o.y > 9.99f);       // ninguna se cuela en el suelo
    }
    CHECK(tiradas > 50);
    CHECK(total <= MAX);
}

TEST_CASE("Hojas secas: el jugador corriendo las patea") {
    static Sistema s;
    s.soltar(1, 11, 1);
    Hoja& o = s.h[1];
    simular(s, 6.0f, 0.0f, 0.0f, 1.0f);
    REQUIRE(o.estado == TIRADA);
    s.avanzar(1.0f / 60.0f, 0.0f, 0.0f, 0.0f, o.x - 0.1f, 10.0f, o.z, 4.0f, 0.0f, mundo);
    CHECK(o.estado == CAYENDO);
    CHECK(o.vy > 0.5f);
}
