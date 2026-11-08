#include <doctest/doctest.h>
#include "audio/Viento.h"
#include "render/Atmosfera.h"
#include <cmath>
#include <vector>

using namespace Audio;

TEST_CASE("Viento: callado a cero, acotado, y con rafagas que cambian") {
    GeneradorViento v;
    std::vector<float> buf(22050, 0.0f);
    v.llenar(buf.data(), (int)buf.size(), 22050, 0.0f);
    for (float x : buf) CHECK(x == 0.0f);

    // Con viento: se oye, sin salirse de lo que cabe en 16 bits.
    std::vector<float> fuerte(22050 * 4, 0.0f);
    v.llenar(fuerte.data(), (int)fuerte.size(), 22050, 1.0f);
    double energia = 0.0;
    float pico = 0.0f;
    for (float x : fuerte) { energia += (double)x * x; pico = std::fmax(pico, std::fabs(x)); }
    CHECK(energia / fuerte.size() > 1000.0);
    CHECK(pico < 20000.0f);
    // Las rafagas: la energia de un segundo a otro no es la misma.
    double seg[4] = {};
    for (int s = 0; s < 4; ++s)
        for (int i = 0; i < 22050; ++i) seg[s] += (double)fuerte[s * 22050 + i] * fuerte[s * 22050 + i];
    double lo = 1e30, hi = 0;
    for (double e : seg) { lo = std::fmin(lo, e); hi = std::fmax(hi, e); }
    CHECK(hi > lo * 1.1);
    // Bajar el viento no corta de golpe: se apaga en un par de segundos.
    std::vector<float> baja(2205, 0.0f);
    v.llenar(baja.data(), (int)baja.size(), 22050, 0.0f);
    CHECK(v.intensidad > 0.5f);
}

TEST_CASE("Viento: mas en lo alto y a la intemperie, casi nada en una cueva") {
    CHECK(intensidadViento(150.0f, 1.0f) > intensidadViento(70.0f, 1.0f) + 0.2f);
    CHECK(intensidadViento(70.0f, 1.0f) > intensidadViento(70.0f, 0.1f) + 0.2f);
    CHECK(intensidadViento(20.0f, 0.0f) < 0.12f);
    CHECK(intensidadViento(400.0f, 1.0f) <= 1.0f);
}

TEST_CASE("Cielo: el halo del sol y el brillo nocturno no tocan el horizonte") {
    using namespace Atmosfera;
    const Cielo dia = cieloPara(1.0f);
    // En el horizonte, mirando al sol, igual que antes: la niebla del terreno
    // (que repite esa cuenta en el shader) no cambia.
    float h[3], n[3];
    colorEn(dia, 0.0f, 1.0f, h);
    colorNiebla(dia, 1.0f, n);
    for (int i = 0; i < 3; ++i) CHECK(h[i] == doctest::Approx(n[i]));
    // Por encima, al lado del sol, el cielo es mas claro que lejos de el.
    float cerca[3], lejos[3];
    colorEn(dia, 0.3f, 0.99f, cerca);
    colorEn(dia, 0.3f, 0.0f, lejos);
    CHECK(cerca[0] + cerca[1] + cerca[2] > lejos[0] + lejos[1] + lejos[2] + 0.1f);
    // De dia el cenit es un azul hondo.
    CHECK(dia.cenit[2] > dia.cenit[0] * 3.0f);
    // De noche, una franja tenue sobre el horizonte.
    const Cielo noche = cieloPara(0.0f);
    float franja[3], arriba[3];
    colorEn(noche, 0.06f, 0.0f, franja);
    colorEn(noche, 0.45f, 0.0f, arriba);
    CHECK(franja[1] > arriba[1]);
    CHECK(franja[1] < 0.15f);
}
