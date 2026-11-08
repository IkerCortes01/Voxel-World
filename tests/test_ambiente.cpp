#include <doctest/doctest.h>
#include "audio/Ambiente.h"
#include <cmath>
#include <vector>

using namespace Audio;

namespace {
double energia(float hojas, float pajaros, float grillos, double segundos = 12.0) {
    GeneradorAmbiente g;
    std::vector<float> b((size_t)(22050 * segundos), 0.0f);
    // A bloques, como el mezclador.
    for (size_t i = 0; i < b.size(); i += 512)
        g.llenar(b.data() + i, (int)std::min<size_t>(512, b.size() - i), 22050, hojas, pajaros, grillos);
    double e = 0.0;
    for (size_t i = b.size() / 3; i < b.size(); ++i) e += (double)b[i] * b[i];
    return e / (double)(b.size() - b.size() / 3);
}
} // namespace

TEST_CASE("Ambiente: callado sin nada; las hojas suenan mas en el bosque que con un arbol") {
    CHECK(energia(0.0f, 0.0f, 0.0f) == 0.0);
    const double unArbol = energia(0.25f, 0.0f, 0.0f);
    const double bosque = energia(0.9f, 0.0f, 0.0f);
    CHECK(unArbol > 100.0);
    CHECK(bosque > 4.0 * unArbol);
}

TEST_CASE("Ambiente: los pajaros y los grillos cantan") {
    CHECK(energia(0.0f, 0.8f, 0.0f) > 1000.0);
    CHECK(energia(0.0f, 0.0f, 0.8f) > 1000.0);
    // Sin saturar los 16 bits, con todo a la vez.
    GeneradorAmbiente g;
    std::vector<float> b(22050 * 6, 0.0f);
    for (size_t i = 0; i < b.size(); i += 512)
        g.llenar(b.data() + i, (int)std::min<size_t>(512, b.size() - i), 22050, 1.0f, 1.0f, 1.0f);
    float pico = 0.0f;
    for (float x : b) pico = std::fmax(pico, std::fabs(x));
    CHECK(pico < 20000.0f);
}

TEST_CASE("Ambiente: las rachas del dia van de la calma al ventarron, sin saltos") {
    float lo = 1.0f, hi = 0.0f;
    float antes = rachaDelDia(0.0);
    for (int k = 1; k < 200000; ++k) {
        const double t = k * 0.25;
        const float r = rachaDelDia(t);
        lo = std::fmin(lo, r);
        hi = std::fmax(hi, r);
        CHECK(std::fabs(r - antes) < 0.01f);
        antes = r;
    }
    CHECK(lo < 0.15f);
    CHECK(hi > 0.9f);
}
