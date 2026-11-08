#include <doctest/doctest.h>
#include "render/Precipitacion.h"
#include "audio/Lluvia.h"
#include <cmath>
#include <vector>

using namespace Precipitacion;

namespace {
Entrada entrada(float agua, float granizo = 0.0f, float nieve = 0.0f, float polvo = 0.0f) {
    Entrada e;
    e.t = 1234.5;
    e.ojoX = 100.3; e.ojoY = 70.0; e.ojoZ = -40.7;
    e.agua = agua; e.granizo = granizo; e.nieve = nieve; e.polvo = polvo;
    e.vientoX = 4.0f; e.vientoZ = 1.5f;
    e.luz = 1.0f;
    return e;
}
Techos cieloAbierto() {
    Techos t;
    t.renovar(100, -41, [](int, int) { return -100000; });
    return t;
}
} // namespace

TEST_CASE("Precipitacion: mas lluvia, mas gotas; sin clima, ninguna") {
    std::vector<Trazo> tr(MAX_GOTAS);
    const Techos t = cieloAbierto();
    CHECK(generar(entrada(0.0f), t, tr.data(), MAX_GOTAS) == 0);
    const int llovizna = generar(entrada(0.2f), t, tr.data(), MAX_GOTAS);
    const int aguacero = generar(entrada(0.9f), t, tr.data(), MAX_GOTAS);
    CHECK(llovizna > 100);
    CHECK(aguacero > 3 * llovizna);
    // Todas cerca del jugador y dentro de la columna.
    for (int i = 0; i < aguacero; ++i) {
        CHECK(std::fabs(tr[i].x1) < LADO);
        CHECK(std::fabs(tr[i].z1) < LADO);
        CHECK(tr[i].y1 <= ALTO - DEBAJO + 0.01f);
        CHECK(tr[i].y1 >= -DEBAJO - 0.01f);
        CHECK(tr[i].clase == 0);
    }
}

TEST_CASE("Precipitacion: bajo techo no llueve") {
    std::vector<Trazo> tr(MAX_GOTAS);
    Techos techo;
    techo.renovar(100, -41, [](int, int) { return 70 + 30; });   // un techo muy por encima
    CHECK(generar(entrada(1.0f, 1.0f, 1.0f), techo, tr.data(), MAX_GOTAS) == 0);
}

TEST_CASE("Precipitacion: la lluvia cae y se inclina con el viento") {
    std::vector<Trazo> tr(MAX_GOTAS);
    const Techos t = cieloAbierto();
    const int n = generar(entrada(0.6f), t, tr.data(), MAX_GOTAS);
    REQUIRE(n > 0);
    for (int i = 0; i < n; ++i) {
        CHECK(tr[i].y0 > tr[i].y1);            // la cola, arriba
        CHECK(tr[i].x0 < tr[i].x1);            // viene de contra el viento (+X)
    }
    // Un instante despues, la misma gota esta mas abajo (o volvio a empezar).
    Entrada e = entrada(0.6f);
    std::vector<Trazo> a(MAX_GOTAS), b(MAX_GOTAS);
    generar(e, t, a.data(), MAX_GOTAS);
    e.t += 0.01;
    generar(e, t, b.data(), MAX_GOTAS);
    int bajan = 0;
    for (int i = 0; i < 200; ++i) bajan += (b[i].y1 < a[i].y1) ? 1 : 0;
    CHECK(bajan > 180);
}

TEST_CASE("Precipitacion: granizo, nieve y polvo tienen su clase") {
    std::vector<Trazo> tr(MAX_GOTAS);
    const Techos t = cieloAbierto();
    int vistas[4] = {};
    const int n = generar(entrada(0.3f, 0.5f, 0.5f, 0.5f), t, tr.data(), MAX_GOTAS);
    for (int i = 0; i < n; ++i) ++vistas[tr[i].clase];
    for (int c = 0; c < 4; ++c) CHECK(vistas[c] > 0);
}

TEST_CASE("Sonido de lluvia: callada sin lluvia, mas fuerte con aguacero, sorda bajo techo") {
    auto energia = [](float agua, float granizo, float cielo) {
        Audio::GeneradorLluvia g;
        std::vector<float> b(22050 * 3, 0.0f);
        g.llenar(b.data(), (int)b.size(), 22050, agua, granizo, cielo);
        double e = 0.0;
        for (size_t i = 22050 * 2; i < b.size(); ++i) e += (double)b[i] * b[i];
        return e / 22050.0;
    };
    CHECK(energia(0.0f, 0.0f, 1.0f) == 0.0);
    CHECK(energia(0.9f, 0.0f, 1.0f) > 2.0 * energia(0.2f, 0.0f, 1.0f));
    CHECK(energia(0.9f, 0.0f, 0.0f) < energia(0.9f, 0.0f, 1.0f));
    CHECK(energia(0.0f, 0.8f, 1.0f) > 1000.0);
}

TEST_CASE("Sonido de trueno: suena, se apaga solo, y el cercano pega mas") {
    auto pico = [](float fuerza) {
        Audio::GeneradorTrueno g;
        g.disparar(fuerza);
        std::vector<float> b(22050 * 12, 0.0f);
        g.llenar(b.data(), (int)b.size(), 22050, 1.0f);
        float p = 0.0f;
        for (float x : b) p = std::fmax(p, std::fabs(x));
        bool callado = true;
        for (int k = 0; k < Audio::GeneradorTrueno::MAX; ++k) callado = callado && !g.truenos[k].activo;
        return std::make_pair(p, callado);
    };
    const auto cerca = pico(1.0f), lejos = pico(0.15f);
    CHECK(cerca.second);
    CHECK(lejos.second);
    CHECK(cerca.first > lejos.first);
    CHECK(lejos.first > 100.0f);
    CHECK(cerca.first < 32000.0f);
}
