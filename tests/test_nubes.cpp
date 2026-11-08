#include <doctest/doctest.h>
#include "render/Nubes.h"
#include <cmath>
#include <vector>

using namespace Nubes;

namespace {
std::vector<Bola> cielo(double t, double x = 100.0, double y = 80.0, double z = -50.0) {
    std::vector<Bola> b(MAX_BOLAS);
    b.resize((size_t)generar(t, x, y, z, b.data(), MAX_BOLAS));
    return b;
}
// Una huella del cielo: cuanto "volumen visible" hay.
double huella(const std::vector<Bola>& b) {
    double s = 0.0;
    for (const Bola& o : b) s += o.alfa * o.radio * o.radio;
    return s;
}
} // namespace

TEST_CASE("Nubes: hay nubes, acotadas, por encima del ojo y dentro de la vista") {
    int conNubes = 0;
    for (int k = 0; k < 12; ++k) {
        const auto b = cielo(1000.0 + k * 700.0);
        CHECK(b.size() < (size_t)MAX_BOLAS);
        if (b.size() > 20) ++conNubes;
        for (const Bola& o : b) {
            CHECK(o.y > SOBRE_OJO * 0.9f);
            CHECK(std::sqrt(o.x * o.x + o.z * o.z) < RADIO_VISTA + 80.0f);
            CHECK(o.radio > 0.5f);
            CHECK(o.radio < 20.0f);
            CHECK(o.alfa > 0.0f);
            CHECK(o.alfa <= 1.0f);
        }
    }
    CHECK(conNubes >= 10);
}

TEST_CASE("Nubes: el mismo instante da el mismo cielo") {
    const auto a = cielo(5432.1), b = cielo(5432.1);
    REQUIRE(a.size() == b.size());
    for (size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].x == b[i].x);
        CHECK(a[i].radio == b[i].radio);
    }
}

TEST_CASE("Nubes: cambian de forma, pero sin tirones") {
    // De un frame al siguiente apenas cambia nada...
    for (int k = 0; k < 40; ++k) {
        const double t = 2000.0 + k * 97.3;
        const double h0 = huella(cielo(t)), h1 = huella(cielo(t + 1.0 / 60.0));
        CHECK(std::fabs(h1 - h0) <= 0.01 * h0 + 1.0);
    }
    // ...pero en unos minutos el cielo es otro.
    const auto a = cielo(3000.0), b = cielo(3000.0 + 600.0);
    CHECK(std::fabs(huella(a) - huella(b)) + std::fabs((double)a.size() - (double)b.size()) > 1.0);
}

TEST_CASE("Nubes: siempre por encima del jugador, aunque suba a una montana") {
    const auto b = cielo(4000.0, 0.0, 400.0, 0.0);
    for (const Bola& o : b) CHECK(o.y > SOBRE_OJO * 0.9f);
}

TEST_CASE("Nubes: ordenadas de atras hacia delante") {
    auto b = cielo(7000.0);
    REQUIRE(b.size() > 2);
    ordenar(b.data(), (int)b.size());
    for (size_t i = 1; i < b.size(); ++i) {
        const float d0 = b[i - 1].x * b[i - 1].x + b[i - 1].y * b[i - 1].y + b[i - 1].z * b[i - 1].z;
        const float d1 = b[i].x * b[i].x + b[i].y * b[i].y + b[i].z * b[i].z;
        CHECK(d0 >= d1);
    }
}

TEST_CASE("Nubes: la cobertura va de despejado a casi cubierto") {
    float lo = 1.0f, hi = 0.0f;
    for (int k = 0; k < 4000; ++k) {
        const float c = cobertura(k * 13.0);
        lo = std::fmin(lo, c);
        hi = std::fmax(hi, c);
    }
    CHECK(lo >= 0.19f);
    CHECK(hi <= 0.81f);
    CHECK(hi - lo > 0.35f);
}
