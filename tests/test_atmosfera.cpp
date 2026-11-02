#include <doctest/doctest.h>
#include "render/Atmosfera.h"
#include <cmath>
#include <fstream>
#include <iterator>
#include <string>

using namespace Atmosfera;

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

TEST_CASE("Atmosfera: de dia el horizonte es mas claro que el cenit") {
    const Cielo c = cieloPara(1.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.horizonte[i] > c.cenit[i]);
    CHECK(c.resplandor[0] == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: de noche es oscuro y las estrellas se ven") {
    const Cielo c = cieloPara(0.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.horizonte[i] < 0.15f);
    CHECK(brilloEstrellas(0.0f) == doctest::Approx(1.0f));
    CHECK(brilloEstrellas(1.0f) == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: el ocaso enrojece el horizonte, mas del lado del sol") {
    const Cielo c = cieloPara(0.5f);
    CHECK(c.horizonte[0] > c.horizonte[2]);
    float alSol[3], contra[3];
    colorEn(c, 0.05f, 1.0f, alSol);
    colorEn(c, 0.05f, -1.0f, contra);
    CHECK(alSol[0] > contra[0]);
}

TEST_CASE("Atmosfera: el horizonte coincide con la niebla y el color no se sale") {
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.05f) {
        const Cielo c = cieloPara(luz);
        float h[3];
        colorEn(c, 0.0f, -1.0f, h);
        for (int i = 0; i < 3; ++i) {
            CHECK(h[i] == doctest::Approx(c.horizonte[i]));
            float v[3];
            colorEn(c, 0.3f, 1.0f, v);
            CHECK(v[i] >= 0.0f);
            CHECK(v[i] <= 1.0f);
        }
    }
}

TEST_CASE("Atmosfera: las estrellas son unitarias y siempre las mismas") {
    for (int i = 0; i < 320; ++i) {
        float a[4], b[4];
        estrella(i, a);
        estrella(i, b);
        CHECK(a[0] == b[0]);
        CHECK(a[0] * a[0] + a[1] * a[1] + a[2] * a[2] == doctest::Approx(1.0f));
        CHECK(a[3] > 0.0f);
    }
}

TEST_CASE("Sonido: romper bloques ya no suena") {
    // PEDIDO: "sobre todo elimina el sonido de romper bloques".
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("playBreakBlock") == std::string::npos);
    CHECK(src.find("playBreakingLoop") == std::string::npos);
    CHECK(src.find("playPlaceBlock") != std::string::npos);   // colocar sigue
}
