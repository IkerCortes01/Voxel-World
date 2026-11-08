#include <doctest/doctest.h>
#include "Clima.h"
#include <cmath>
#include <vector>

using namespace Clima;

namespace {
// Cuantas veces sale cada tiempo en muchos tramos de un lugar.
std::vector<int> recuento(const Lugar& l, int tramos = 4000) {
    std::vector<int> c(NUM_TIPOS, 0);
    for (int k = 0; k < tramos; ++k) ++c[tipoDeTramo(k, region(3, -7), l)];
    return c;
}
int suma(const std::vector<int>& c, std::initializer_list<int> tipos) {
    int s = 0;
    for (int t : tipos) s += c[t];
    return s;
}
const Lugar DESIERTO{0.92f, 0.05f}, BOSQUE{0.45f, 0.80f}, HELADO{0.08f, 0.55f}, SELVA{0.95f, 0.95f};
} // namespace

TEST_CASE("Clima: el desierto casi siempre esta despejado, y tiene tolvaneras") {
    const auto c = recuento(DESIERTO);
    CHECK(c[DESPEJADO] > 4000 / 3);
    CHECK(c[TOLVANERA] > 100);
    CHECK(c[NEVADA] + c[VENTISCA] == 0);
    CHECK(suma(c, {LLUVIA, AGUACERO, LLOVIZNA}) < c[DESPEJADO] / 5);
}

TEST_CASE("Clima: en el bosque humedo llueve mucho mas que en el desierto") {
    const auto b = recuento(BOSQUE), d = recuento(DESIERTO);
    const int llueveB = suma(b, {LLOVIZNA, LLUVIA, AGUACERO, TORMENTA, TORMENTA_ELECTRICA, GRANIZADA});
    const int llueveD = suma(d, {LLOVIZNA, LLUVIA, AGUACERO, TORMENTA, TORMENTA_ELECTRICA, GRANIZADA});
    CHECK(llueveB > 3 * llueveD);
    CHECK(b[GRANIZADA] > 0);
    CHECK(b[NEBLINA] > 0);
}

TEST_CASE("Clima: donde hiela no llueve, nieva") {
    const auto c = recuento(HELADO);
    CHECK(c[NEVADA] + c[VENTISCA] > 4000 / 4);
    CHECK(suma(c, {LLUVIA, AGUACERO, LLOVIZNA, TORMENTA_ELECTRICA, TORMENTA_TROPICAL}) == 0);
}

TEST_CASE("Clima: el tropico tiene tormentas tropicales; el bosque templado casi no") {
    CHECK(recuento(SELVA)[TORMENTA_TROPICAL] > 3 * (recuento(BOSQUE)[TORMENTA_TROPICAL] + 1));
}

TEST_CASE("Clima: las tormentas electricas son de tarde") {
    float w[NUM_TIPOS];
    pesos(BOSQUE, 17.0f, w);
    const float tarde = w[TORMENTA_ELECTRICA];
    pesos(BOSQUE, 3.0f, w);
    CHECK(tarde > 2.5f * w[TORMENTA_ELECTRICA]);
    // Y la neblina, de manana.
    pesos(BOSQUE, 7.0f, w);
    const float manana = w[NEBLINA];
    pesos(BOSQUE, 15.0f, w);
    CHECK(manana > 3.0f * w[NEBLINA]);
}

TEST_CASE("Clima: el tiempo cambia poco a poco, en el tiempo y andando") {
    // De un frame al siguiente, nada salta.
    for (int k = 0; k < 3000; ++k) {
        const double t = 100.0 + k * 7.31;
        const Perfil a = perfilEn(t, 1000.0, -2000.0, BOSQUE);
        const Perfil b = perfilEn(t + 1.0 / 30.0, 1000.0, -2000.0, BOSQUE);
        CHECK(std::fabs(a.agua - b.agua) < 0.02f);
        CHECK(std::fabs(a.cobertura - b.cobertura) < 0.02f);
        CHECK(std::fabs(a.rayos - b.rayos) < 0.3f);
    }
    // Un paso de un bloque tampoco.
    for (int k = 0; k < 2000; ++k) {
        const double x = k * 3.7;
        const Perfil a = perfilEn(5000.0, x, 300.0, BOSQUE);
        const Perfil b = perfilEn(5000.0, x + 1.0, 300.0, BOSQUE);
        CHECK(std::fabs(a.agua - b.agua) < 0.02f);
    }
}

TEST_CASE("Clima: con el tiempo pasan muchos tiempos distintos") {
    std::vector<int> vistos(NUM_TIPOS, 0);
    for (int k = 0; k < 600; ++k) {
        int tipo = -1;
        perfilEn(k * 200.0, 0.0, 0.0, BOSQUE, &tipo);
        REQUIRE(tipo >= 0);
        REQUIRE(tipo < NUM_TIPOS);
        vistos[tipo] = 1;
    }
    int distintos = 0;
    for (int v : vistos) distintos += v;
    CHECK(distintos >= 7);
}

TEST_CASE("Clima: los rayos solo caen con tormenta, y el trueno llega despues") {
    std::vector<Rayo> r(256);
    // Sin rayos por minuto, ninguno.
    CHECK(rayosEntre(0.0, 600.0, [](double) { return 0.0f; }, r.data(), 256) == 0);
    // Con 8 por minuto, unos 8 por minuto.
    const int n = rayosEntre(0.0, 600.0, [](double) { return 8.0f; }, r.data(), 256);
    CHECK(n > 50);
    CHECK(n < 110);
    for (int i = 0; i < n; ++i) {
        CHECK(r[i].t > 0.0);
        CHECK(r[i].t <= 600.0);
        CHECK(r[i].distancia >= DISTANCIA_MIN);
        CHECK(r[i].distancia <= DISTANCIA_MAX);
        CHECK(retrasoTrueno(r[i]) >= DISTANCIA_MIN / SONIDO_MS);
    }
    // Partir el intervalo da los mismos rayos: sirve frame a frame.
    const int a = rayosEntre(0.0, 300.0, [](double) { return 8.0f; }, r.data(), 256);
    const int b = rayosEntre(300.0, 600.0, [](double) { return 8.0f; }, r.data(), 256);
    CHECK(a + b == n);
}

TEST_CASE("Clima: el destello del rayo dura menos de un segundo") {
    Rayo r{10.0, 800.0f, 0.0f, 3u};
    CHECK(destello(r, 0.0) > 0.4f);
    CHECK(destello(r, 0.7) == 0.0f);
    CHECK(destello(r, -0.1) == 0.0f);
    Rayo lejos{10.0, 6000.0f, 0.0f, 0u};
    CHECK(destello(lejos, 0.0) < destello(r, 0.0));
    CHECK(fuerzaTrueno(lejos) < fuerzaTrueno(r));
}
