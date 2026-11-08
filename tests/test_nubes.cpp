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
            CHECK(o.radio < 30.0f);
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

// ============================================================================
// SE JUNTAN, SE SEPARAN, Y SOLO LLUEVE BAJO LAS GRANDES
// ============================================================================
TEST_CASE("Nubes: miles de bolas en el cielo, sin pasarse del tope") {
    for (float cob : { 0.3f, 0.6f, 0.9f }) {
        std::vector<Bola> b(MAX_BOLAS);
        const int n = generar(9000.0, 0.0, 80.0, 0.0, b.data(), MAX_BOLAS, cob, 0.0f);
        INFO("cobertura ", cob, ": ", n, " bolas");
        CHECK(n < MAX_BOLAS);
        if (cob >= 0.6f) CHECK(n > 1000);
    }
}

TEST_CASE("Nubes: con poco cielo cubierto no llueve; cubierto, llueve a ratos") {
    auto mojado = [](float cob) {
        int si = 0, total = 0;
        for (int i = 0; i < 60; ++i)
            for (int j = 0; j < 60; ++j, ++total)
                if (lluviaEn(i * 37.0, j * 41.0, 5000.0, cob) > 0.5f) ++si;
        return (float)si / (float)total;
    };
    CHECK(mojado(0.2f) < 0.03f);
    const float cubierto = mojado(0.9f);
    CHECK(cubierto > 0.3f);
    CHECK(cubierto < 0.995f);    // hay claros: fuera de la nube no llueve
    // Y la lluvia empieza y acaba poco a poco al andar.
    for (int k = 0; k < 200; ++k) {
        const double x = k * 3.0;
        CHECK(std::fabs(lluviaEn(x, 0.0, 5000.0, 0.8f) - lluviaEn(x + 1.0, 0.0, 5000.0, 0.8f)) < 0.08f);
    }
}

TEST_CASE("Nubes: donde llueve hay una nube grande encima; donde no, poca o ninguna") {
    const float cob = 0.75f;
    const double t = 7777.0;
    double bajoLluvia = 0.0, alSeco = 0.0;
    int nLluvia = 0, nSeco = 0;
    std::vector<Bola> b(MAX_BOLAS);
    for (int k = 0; k < 400 && (nLluvia < 6 || nSeco < 6); ++k) {
        const double x = k * 53.0, z = k * 29.0;
        const float ll = lluviaEn(x, z, t, cob);
        if (!(ll > 0.9f || ll <= 0.0f)) continue;
        if (ll > 0.9f && nLluvia >= 6) continue;
        if (ll <= 0.0f && nSeco >= 6) continue;
        const int n = generar(t, x, 70.0, z, b.data(), MAX_BOLAS, cob, 0.0f);
        double encima = 0.0;
        for (int i = 0; i < n; ++i)
            if (b[i].x * b[i].x + b[i].z * b[i].z < 45.0f * 45.0f) encima += b[i].alfa * b[i].radio * b[i].radio;
        if (ll > 0.9f) { bajoLluvia += encima; ++nLluvia; } else { alSeco += encima; ++nSeco; }
    }
    REQUIRE(nLluvia > 0);
    REQUIRE(nSeco > 0);
    CHECK(bajoLluvia / nLluvia > 3.0 * (alSeco / nSeco + 1.0));
}

TEST_CASE("Nubes: las nubes crecen con la masa: grandes juntas, pequenas sueltas") {
    // Con poca cobertura las bolas son pequenas; con mucha, bastante mayores.
    auto radioMedio = [](float cob) {
        std::vector<Bola> b(MAX_BOLAS);
        const int n = generar(3333.0, 0.0, 80.0, 0.0, b.data(), MAX_BOLAS, cob, 0.0f);
        double s = 0.0, w = 0.0;
        for (int i = 0; i < n; ++i) { s += b[i].radio * b[i].alfa; w += b[i].alfa; }
        return w > 0.0 ? s / w : 0.0;
    };
    CHECK(radioMedio(0.85f) > 1.5 * radioMedio(0.25f));
}
