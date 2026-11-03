#include <doctest/doctest.h>
#include "fauna/PecariMundo.h"
#include <cmath>

using namespace Fauna;

namespace {

// Un cuadrado de 1 m en el plano y = 0, partido en dos triangulos hacia arriba.
MallaAnimal cuadrado() {
    MallaAnimal m;
    const float P[4][2] = { {-0.5f, -0.5f}, {0.5f, -0.5f}, {0.5f, 0.5f}, {-0.5f, 0.5f} };
    for (const auto& p : P) {
        VerticeAnimal v{};
        v.pos = V3(p[0], 0.0f, p[1]);
        v.normal = V3(0, 1, 0);
        m.vertices.push_back(v);
    }
    // Sentido para que la normal de cara apunte a +Y.
    m.indices = { 0, 2, 1, 0, 3, 2 };
    return m;
}

class Llano : public IPecariMundo {
public:
    float alturaSuelo(int, int) const override { return 64.0f; }
    BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
    float pendienteEn(int, int) const override { return 0.0f; }
    bool esSolido(int, int y, int) const override { return y < 64; }
};

} // namespace

TEST_CASE("Heridas: la calcomania queda dentro de su caja y de la malla") {
    const MallaAnimal m = cuadrado();
    Herida h;
    h.centro = V3(0.1f, 0.0f, -0.1f);
    h.normal = V3(0, 1, 0);
    h.tam = 0.2f;
    h.giro = 0.7f;
    std::vector<PuntoCalca> pts;
    ProyectarHerida(m, h, pts);
    REQUIRE(pts.size() >= 6);
    CHECK(pts.size() % 3 == 0);
    for (const PuntoCalca& q : pts) {
        CHECK(q.wb >= -1e-4f);
        CHECK(q.wc >= -1e-4f);
        CHECK(q.wb + q.wc <= 1.0f + 1e-4f);
        CHECK(q.u >= -1e-4f); CHECK(q.u <= 1.0f + 1e-4f);
        CHECK(q.v >= -1e-4f); CHECK(q.v <= 1.0f + 1e-4f);
        // El punto reconstruido cae cerca del centro: dentro de la caja girada.
        const V3& A = m.vertices[q.a].pos;
        const V3& B = m.vertices[q.b].pos;
        const V3& C = m.vertices[q.c].pos;
        const float wa = 1 - q.wb - q.wc;
        const float x = A.x * wa + B.x * q.wb + C.x * q.wc;
        const float z = A.z * wa + B.z * q.wb + C.z * q.wc;
        CHECK(std::hypot(x - h.centro.x, z - h.centro.z) <= h.tam * 0.7072f + 1e-3f);
    }
}

TEST_CASE("Heridas: lejos o de espaldas no marca nada") {
    const MallaAnimal m = cuadrado();
    Herida lejos;
    lejos.centro = V3(3, 0, 3);
    lejos.normal = V3(0, 1, 0);
    std::vector<PuntoCalca> pts;
    ProyectarHerida(m, lejos, pts);
    CHECK(pts.empty());

    Herida espaldas;
    espaldas.centro = V3(0, 0, 0);
    espaldas.normal = V3(0, -1, 0);
    ProyectarHerida(m, espaldas, pts);
    CHECK(pts.empty());
}

TEST_CASE("Heridas: el rayo encuentra el punto y la normal mira al que golpea") {
    const MallaAnimal m = cuadrado();
    V3 p, n;
    REQUIRE(ImpactoEnMalla(m, V3(0.2f, 1.0f, 0.1f), V3(0, -1, 0), p, n));
    CHECK(p.x == doctest::Approx(0.2f));
    CHECK(p.y == doctest::Approx(0.0f));
    CHECK(n.y > 0.99f);
    CHECK_FALSE(ImpactoEnMalla(m, V3(2, 1, 2), V3(0, -1, 0), p, n));
}

TEST_CASE("Heridas: un golpe al pecari real deja una marca que pasa al cuerpo") {
    Llano mundoT;
    MundoPecaries mundo(4242);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, mundoT, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() > 0);
    const PecariAgente a = mundo.todos().front();

    // Desde un costado, a la altura del lomo, apuntando al centro.
    CacheMallas cache;
    const float altura = a.y + 0.45f * a.escala / 0.6f;
    REQUIRE(mundo.marcarHerida(a.id, a.x - 3.0f, altura, a.z, 1.0f, 0.0f, 0.0f, cache));
    const PecariAgente* vivo = nullptr;
    for (const PecariAgente& p : mundo.todos()) if (p.id == a.id) vivo = &p;
    REQUIRE(vivo != nullptr);
    REQUIRE(vivo->heridas.size() == 1);
    const MallaAnimal* malla = cache.obtener(EspecieAnimal::PECARI_COLLAR, (int)a.etapa, 0);
    REQUIRE(malla != nullptr);
    CHECK(GeometriaHerida(*malla, vivo->heridas[0]).size() >= 3);

    // No se pasa de seis.
    for (int i = 0; i < 10; ++i)
        mundo.marcarHerida(a.id, a.x - 3.0f, altura, a.z, 1.0f, 0.0f, 0.0f, cache);
    for (const PecariAgente& p : mundo.todos()) if (p.id == a.id) CHECK(p.heridas.size() == 6);

    REQUIRE(mundo.recibirDano(a.id, 1000, a.x - 3.0f, a.z, mundoT));
    REQUIRE(mundo.cuerpos().size() == 1);
    CHECK(mundo.cuerpos().front().heridas.size() == 6);
}
