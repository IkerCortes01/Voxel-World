#include <doctest/doctest.h>
#include "ModeloItem3D.h"
#include <cmath>

using namespace ModeloItem3D;

namespace {
std::vector<unsigned char> mascara(int w, int h, bool (*dentro)(int, int)) {
    std::vector<unsigned char> m((size_t)w * h, 0);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) m[(size_t)y * w + x] = dentro(x, y) ? 1 : 0;
    return m;
}
} // namespace

TEST_CASE("Item 3D: un mango de 2 pixeles sale redondo, no plano") {
    // Un palo en diagonal, de 2 pixeles de grueso, como el de las texturas.
    const auto m = mascara(16, 16, [](int x, int y) { return std::abs((15 - y) - x) <= 1 && x > 1 && x < 14; });
    const Malla malla = construir(m, 16, 16, radioPara(Forma::HERRAMIENTA, 16));
    REQUIRE(malla.valido);
    CHECK(malla.altoMax > 0.05f);                      // tiene cuerpo
    // Ancho del palo ~2 px = 0.25 del cuadro; grosor total parecido (redondo).
    const float grosor = 2.0f * malla.altoMax;
    CHECK(grosor > 0.10f);
    CHECK(grosor < 0.34f);                             // como mucho, lo que es de ancho
}

TEST_CASE("Item 3D: cerrado por el borde, abombado por dentro y con tope") {
    // Un cuadrado de 12 px: lejos del borde la altura llega al tope y se queda.
    const auto m = mascara(16, 16, [](int x, int y) { return x >= 2 && x < 14 && y >= 2 && y < 14; });
    const float r = radioPara(Forma::REDONDO, 16);
    const Malla malla = construir(m, 16, 16, r);
    REQUIRE(malla.valido);
    float zBorde = 0.0f, zCentro = 0.0f;
    for (const Vert& v : malla.v) {
        // Las esquinas del cuadrado (borde exacto) tienen altura 0: cerrado.
        if (std::fabs(std::fabs(v.x) - 0.75f) < 1e-4f && std::fabs(std::fabs(v.y) - 0.75f) < 1e-4f)
            zBorde = std::fmax(zBorde, std::fabs(v.z));
        if (std::fabs(v.x) < 0.07f && std::fabs(v.y) < 0.07f) zCentro = std::fmax(zCentro, std::fabs(v.z));
        // Normales unitarias y UV dentro de la textura.
        CHECK(std::fabs(v.nx * v.nx + v.ny * v.ny + v.nz * v.nz - 1.0f) < 1e-3f);
        CHECK(v.u >= 0.0f); CHECK(v.u <= 1.0f);
        CHECK(v.v >= 0.0f); CHECK(v.v <= 1.0f);
    }
    CHECK(zBorde == doctest::Approx(0.0f));
    CHECK(zCentro == doctest::Approx(r * 2.0f / 16.0f).epsilon(0.02));   // el tope, en unidades del cuadro
    CHECK(malla.i.size() % 3 == 0);
    for (uint32_t i : malla.i) CHECK(i < malla.v.size());
}

TEST_CASE("Item 3D: lo redondo se hincha mas que una placa") {
    const auto m = mascara(16, 16, [](int x, int y) { const int dx = x - 8, dy = y - 8; return dx * dx + dy * dy < 36; });
    const Malla redondo = construir(m, 16, 16, radioPara(Forma::REDONDO, 16));
    const Malla placa = construir(m, 16, 16, radioPara(Forma::PLACA, 16));
    CHECK(redondo.altoMax > placa.altoMax * 2.0f);
    // Mascara vacia: no hay modelo.
    const std::vector<unsigned char> vacia(256, 0);
    CHECK_FALSE(construir(vacia, 16, 16, 2.0f).valido);
}

TEST_CASE("Item 3D: sabe por donde se agarra (el extremo fino) y se reduce si es grande") {
    // Un martillo: mango fino de abajo-derecha a arriba-izquierda y una cabeza
    // gorda arriba a la izquierda (como el pico de las texturas).
    const auto m = mascara(32, 32, [](int x, int y) {
        const bool mango = std::abs(x - y) <= 1 && x > 4 && x < 30;
        const bool cabeza = x >= 2 && x < 12 && y >= 2 && y < 12;
        return mango || cabeza;
    });
    const Malla malla = construir(m, 32, 32, radioPara(Forma::HERRAMIENTA, 32));
    REQUIRE(malla.valido);
    // El agarre esta abajo a la derecha (x>0, y<0 en el cuadro).
    CHECK(malla.agarreX > 0.5f);
    CHECK(malla.agarreY < -0.5f);
    // Y el eje va hacia la cabeza (arriba a la izquierda).
    CHECK(malla.ejeX < -0.5f);
    CHECK(malla.ejeY > 0.5f);
    // Reducir 256 a 32.
    std::vector<unsigned char> grande(256 * 256, 1);
    int rw = 0, rh = 0;
    const auto chica = reducir(grande, 256, 256, 32, rw, rh);
    CHECK(rw == 32);
    CHECK(rh == 32);
    CHECK(chica.size() == 32u * 32u);
}

TEST_CASE("Item 3D: sabe de que lado del mango cae el filo de un hacha") {
    // Mango de abajo-derecha a arriba-izquierda y una hoja que sale hacia
    // abajo-izquierda de la cabeza, como en las texturas del hacha.
    const auto m = mascara(32, 32, [](int x, int y) {
        const bool mango = std::abs(x - y) <= 1 && x > 6 && x < 30;
        const bool hoja = x >= 1 && x < 10 && y >= 4 && y < 16 && (x + y) > 8;
        return mango || hoja;
    });
    const Malla malla = construir(m, 32, 32, radioPara(Forma::HERRAMIENTA, 32));
    REQUIRE(malla.valido);
    // La perpendicular (-ejeY, ejeX) del eje hacia arriba-izquierda apunta
    // abajo-izquierda: el lado de la hoja.
    CHECK(malla.ladoCabeza > 0.0f);
}
