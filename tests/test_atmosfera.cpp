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
    // La niebla ya no es de un solo color: es la del cielo que hay detras, en
    // cada direccion. Lo que importa es que coincidan donde se tocan -- el
    // horizonte --, mire uno hacia donde mire.
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.05f) {
        const Cielo c = cieloPara(luz);
        for (float haciaSol = -1.0f; haciaSol <= 1.0f; haciaSol += 0.25f) {
            float h[3], n[3];
            colorEn(c, 0.0f, haciaSol, h);
            colorNiebla(c, haciaSol, n);
            for (int i = 0; i < 3; ++i) CHECK(n[i] == doctest::Approx(h[i]));
        }
        // De lado al sol no hay brillo de ninguno de los dos: es el horizonte.
        float lado[3];
        colorNiebla(c, 0.0f, lado);
        for (int i = 0; i < 3; ++i) {
            CHECK(lado[i] == doctest::Approx(c.horizonte[i]));
            float v[3];
            colorEn(c, 0.3f, 1.0f, v);
            CHECK(v[i] >= 0.0f);
            CHECK(v[i] <= 1.0f);
        }
    }
}

TEST_CASE("Atmosfera: a mediodia la luz del sol es blanca") {
    // El terreno se ve con sus colores de siempre: el tinte es neutro.
    const Cielo c = cieloPara(1.0f);
    for (int i = 0; i < 3; ++i) CHECK(c.luzSol[i] == doctest::Approx(1.0f));
    for (int i = 0; i < 3; ++i) CHECK(c.contraluz[i] == doctest::Approx(0.0f));
}

TEST_CASE("Atmosfera: el ocaso dora la luz y la noche la enfria") {
    const Cielo ocaso = cieloPara(0.5f);
    CHECK(ocaso.luzSol[0] > ocaso.luzSol[2]);          // calida
    const Cielo noche = cieloPara(0.0f);
    CHECK(noche.luzSol[2] > noche.luzSol[0]);          // azulada
}

TEST_CASE("Atmosfera: el tinte cambia el color de la luz, no cuanta hay") {
    // La noche ya la oscurece LuzCielo.h. Si el tinte bajara la luminancia,
    // la noche saldria oscurecida dos veces.
    for (float luz = 0.0f; luz <= 1.0f; luz += 0.1f) {
        const Cielo c = cieloPara(luz);
        const float y = 0.299f * c.luzSol[0] + 0.587f * c.luzSol[1] +
                        0.114f * c.luzSol[2];
        CHECK(y > 0.88f);
        CHECK(y < 1.05f);
    }
}

TEST_CASE("Atmosfera: de espaldas al sol poniente el cielo bajo se tine de rosa") {
    // El cinturon de Venus: del lado contrario al sol, abajo, sube el rojo y
    // el azul a la vez. A mediodia no existe.
    const Cielo c = cieloPara(0.5f);
    float contra[3], lado[3];
    colorEn(c, 0.05f, -1.0f, contra);
    colorEn(c, 0.05f, 0.0f, lado);
    CHECK(contra[0] > lado[0]);
    CHECK(contra[2] > lado[2]);
    // Y en el cenit no se nota.
    float arribaContra[3], arribaLado[3];
    colorEn(c, 1.0f, -1.0f, arribaContra);
    colorEn(c, 1.0f, 0.0f, arribaLado);
    for (int i = 0; i < 3; ++i)
        CHECK(arribaContra[i] == doctest::Approx(arribaLado[i]));
}

TEST_CASE("Atmosfera: el shader repite la cuenta de la niebla") {
    // Misma formula en dos lenguajes: si una cambia y la otra no, el terreno
    // lejano deja de fundirse con el cielo. Se vigila el texto del shader.
    std::string r;
#ifdef RAIZ_PROYECTO
    r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
#endif
    std::ifstream f(r + "src/render/TerrenoGL.h", std::ios::binary);
    REQUIRE(f.good());
    const std::string src((std::istreambuf_iterator<char>(f)),
                          std::istreambuf_iterator<char>());
    CHECK(src.find("uResplandor * (s * s * s)") != std::string::npos);
    CHECK(src.find("uContraluz * (e * e)") != std::string::npos);
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
