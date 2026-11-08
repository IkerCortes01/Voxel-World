#include <doctest/doctest.h>
#include "fauna/PecariRagdoll.h"
#include <cmath>

using namespace Fauna;

namespace {

// Suelo plano: todo lo que esta por debajo de y = 10 es solido.
struct SueloPlano : IMundoRagdoll {
    bool solido(int, int y, int) const override { return y < 10; }
};

EsqueletoReposo esqueletoAdulto() {
    ParametrosPecari p = Especies::aplicarEdad(Especies::pecariDeCollar(), 3);
    return EsqueletoDePecari(p);
}

float dist(const V3& a, const V3& b) {
    return V3(a.x - b.x, a.y - b.y, a.z - b.z).longitud();
}

} // namespace

TEST_CASE("Ragdoll: cae, se posa sobre el suelo y se duerme") {
    Ragdoll r;
    r.iniciar(esqueletoAdulto(), 1.0f, 5.0f, 10.0f, 5.0f, 0.4f, 0.0f, 0.0f, 2.0f, 0.0f);
    SueloPlano suelo;
    for (int i = 0; i < 60 * 10 && !r.dormido; ++i) r.avanzar(1.0f / 60.0f, suelo);
    CHECK(r.dormido);
    for (int i = 0; i < Ragdoll::N; ++i) {
        INFO("punto ", i);
        CHECK(r.punto(i).y >= 10.0f - 1e-3f);    // no atraviesa el suelo
        CHECK(r.punto(i).y < 13.0f);             // y no sale volando
    }
}

TEST_CASE("Ragdoll: sobre una losa de nivel bajo se apoya encima, con su grosor") {
    // Suelo macizo hasta y=10 y encima una losa de 3/8 (nivel 3).
    struct Losa : IMundoRagdoll {
        bool solido(int, int y, int) const override { return y < 10; }
        float alturaCelda(int, int y, int) const override {
            return y < 10 ? 1.0f : (y == 10 ? 0.375f : 0.0f);
        }
    } losa;
    Ragdoll r;
    r.iniciar(esqueletoAdulto(), 1.0f, 5.0f, 10.375f, 5.0f, 0.4f, 0.0f, 0.0f, 2.0f, 0.0f);
    for (int i = 0; i < 60 * 10 && !r.dormido; ++i) r.avanzar(1.0f / 60.0f, losa);
    const float radioTorso = 0.10f / 0.60f;
    for (int i = 0; i < Ragdoll::N; ++i) {
        INFO("punto ", i);
        CHECK(r.punto(i).y >= 10.375f - 1e-3f);
    }
    // Hombros y caderas no se hunden en la carne: quedan a su grosor.
    for (int i = Ragdoll::HI; i <= Ragdoll::CD; ++i)
        CHECK(r.punto(i).y >= 10.375f + radioTorso - 2e-3f);
}

TEST_CASE("Ragdoll: el torso no se deforma") {
    const EsqueletoReposo esq = esqueletoAdulto();
    Ragdoll r;
    r.iniciar(esq, 1.0f, 0.0f, 10.5f, 0.0f, 1.1f, 1.0f, 0.5f, -3.0f, 1.0f);
    SueloPlano suelo;
    const float esc = 1.0f / 0.60f;
    for (int i = 0; i < 60 * 6; ++i) r.avanzar(1.0f / 60.0f, suelo);
    const int T[4] = { Ragdoll::HI, Ragdoll::HD, Ragdoll::CI, Ragdoll::CD };
    for (int i = 0; i < 4; ++i)
        for (int j = i + 1; j < 4; ++j) {
            const float esperado = dist(r.puntoReposo(T[i]), r.puntoReposo(T[j])) * esc;
            CHECK(dist(r.punto(T[i]), r.punto(T[j])) == doctest::Approx(esperado).epsilon(0.03));
        }
}

TEST_CASE("Ragdoll: la pose sale finita y la rotacion es una rotacion") {
    Ragdoll r;
    r.iniciar(esqueletoAdulto(), 1.0f, 0.0f, 10.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f, 3.0f);
    SueloPlano suelo;
    for (int paso = 0; paso < 240; ++paso) {
        r.avanzar(1.0f / 60.0f, suelo);
        float m[9], tx, ty, tz;
        PoseEsqueleto pose;
        r.resolver(m, tx, ty, tz, pose);
        REQUIRE(std::isfinite(tx));
        REQUIRE(std::isfinite(ty));
        for (int i = 0; i < NUM_HUESOS; ++i) {
            REQUIRE(std::isfinite(pose.giroX[i]));
            REQUIRE(std::isfinite(pose.giroY[i]));
        }
        // Columnas unitarias y perpendiculares.
        for (int c = 0; c < 3; ++c) {
            const float l = m[c] * m[c] + m[3 + c] * m[3 + c] + m[6 + c] * m[6 + c];
            CHECK(l == doctest::Approx(1.0f).epsilon(0.01));
        }
        const float d01 = m[0] * m[1] + m[3] * m[4] + m[6] * m[7];
        CHECK(std::fabs(d01) < 0.01f);
    }
}

TEST_CASE("Ragdoll: recien creado reproduce la postura del vivo") {
    // Antes de moverse, la rotacion es la de su orientacion y las patas no giran.
    const float orient = 0.7f;
    Ragdoll r;
    r.iniciar(esqueletoAdulto(), 1.0f, 3.0f, 20.0f, 4.0f, orient, 0, 0, 0, 0);
    float m[9], tx, ty, tz;
    PoseEsqueleto pose;
    r.resolver(m, tx, ty, tz, pose);
    CHECK(m[0] == doctest::Approx(std::cos(orient)).epsilon(0.01));
    CHECK(m[2] == doctest::Approx(std::sin(orient)).epsilon(0.01));
    CHECK(m[4] == doctest::Approx(1.0f).epsilon(0.01));
    CHECK(tx == doctest::Approx(3.0f).epsilon(0.01));
    CHECK(tz == doctest::Approx(4.0f).epsilon(0.01));
    CHECK(std::fabs(pose.giroX[(int)HuesoAnimal::HOMBRO_DI]) < 0.01f);
    CHECK(std::fabs(pose.giroX[(int)HuesoAnimal::CADERA_TD]) < 0.01f);
    CHECK(std::fabs(pose.giroX[(int)HuesoAnimal::CUELLO]) < 0.01f);
}

// ----------------------------------------------------------------------------
// EN EL MUNDO DE LOS PECARIES
// ----------------------------------------------------------------------------
#include "fauna/PecariMundo.h"

namespace {
class LlanoRagdoll : public IPecariMundo {
public:
    float alturaSuelo(int, int) const override { return 64.0f; }
    BiomeType biomaEn(int, int) const override { return BIOME_FOREST; }
    float pendienteEn(int, int) const override { return 0.0f; }
    bool esSolido(int, int y, int) const override { return y < 64; }
};
} // namespace

TEST_CASE("Ragdoll: al morir, el pecari deja un cuerpo que cae y se dibuja") {
    LlanoRagdoll m;
    MundoPecaries mundo(777);
    for (int cx = 0; cx < 30 && mundo.cuantos() == 0; ++cx)
        for (int cz = 0; cz < 30 && mundo.cuantos() == 0; ++cz)
            mundo.alCargarChunk(cx, cz, m, 500.0f, 0.0);
    REQUIRE(mundo.cuantos() > 0);
    const PecariAgente vivo = mundo.todos().front();

    REQUIRE(mundo.recibirDano(vivo.id, 1000, vivo.x - 5.0f, vivo.z, m));
    REQUIRE(mundo.cuerpos().size() == 1);

    for (int i = 0; i < 60 * 8; ++i) mundo.actualizar(1.0f / 60.0f, m);
    const Ragdoll& r = mundo.cuerpos().front();
    CHECK(r.dormido);
    CHECK(r.centro().y >= 64.0f);
    CHECK(r.centro().y < 66.0f);
    // El golpe vino del oeste: el cuerpo acaba al este de donde murio.
    CHECK(r.centro().x > vivo.x);

    CacheMallas cache;
    std::vector<MundoPecaries::InstanciaDibujo> lista;
    const V3 c = r.centro();
    mundo.prepararDibujo(lista, cache, c.x - 3.0f, c.z, 1.0f, 0.0f, 200.0f);
    bool hayCuerpo = false;
    for (const auto& d : lista) if (d.libre) hayCuerpo = true;
    CHECK(hayCuerpo);
}
