#include <doctest/doctest.h>
#include "player/ModeloJugador.h"
#include <cmath>

using namespace ModeloJugador;

namespace {
// La parte mas baja de una caja girada.
float fondo(const Caja& c) {
    float y = 1e9f;
    for (int i = 0; i < 8; ++i) {
        const V esq = { (i & 1 ? 1.f : -1.f) * c.medio.x, (i & 2 ? 1.f : -1.f) * c.medio.y,
                        (i & 4 ? 1.f : -1.f) * c.medio.z };
        const float yy = c.centro.y + c.rot.aplicar(esq).y;
        if (yy < y) y = yy;
    }
    return y;
}
float masBajo(const std::vector<Caja>& cs) {
    float y = 1e9f;
    for (const Caja& c : cs) y = std::fmin(y, fondo(c));
    return y;
}
} // namespace

TEST_CASE("Jugador: de pie, los pies tocan el suelo y la cabeza no esta") {
    std::vector<Caja> cs;
    construir(poseDe(Estado{}), cs);
    // Torso + 2 x (brazo, antebrazo, mano) + 2 x (muslo, pierna, pie).
    CHECK(cs.size() == 13);
    CHECK(masBajo(cs) == doctest::Approx(0.0f).epsilon(0.03));
    for (const Caja& c : cs) CHECK(c.centro.y + c.medio.y < 1.60f);   // nada en la cabeza
}

TEST_CASE("Jugador: el pie queda plano y por delante del tobillo") {
    std::vector<Caja> cs;
    Estado e;
    e.rapidez = 4.3f;
    e.fasePaso = 1.2f;
    construir(poseDe(e), cs);
    int pies = 0;
    for (const Caja& c : cs) {
        if (c.material != ZAPATO) continue;
        ++pies;
        const V adelante = c.rot.aplicar({ 0, 0, 1 });
        CHECK(std::fabs(adelante.y) < 0.02f);     // plano
        CHECK(adelante.z > 0.99f);
    }
    CHECK(pies == 2);
}

TEST_CASE("Jugador: al andar, cada brazo va al reves que su pierna") {
    Estado e;
    e.rapidez = 4.3f;
    e.fasePaso = 1.5708f;
    const Pose p = poseDe(e);
    CHECK(p.caderaI > 0.2f);          // pierna izquierda adelante
    CHECK(p.hombroI < -0.2f);         // brazo izquierdo atras
    CHECK(p.caderaD < -0.2f);
    CHECK(p.hombroD > 0.2f);
    CHECK(p.rodillaI >= 0.0f);
    CHECK(p.rodillaD >= 0.0f);
    // Parado no mueve las piernas.
    const Pose q = poseDe(Estado{});
    CHECK(std::fabs(q.caderaI) < 1e-4f);
}

TEST_CASE("Jugador: agachado, la cadera baja y las rodillas se doblan") {
    Estado de_pie, abajo;
    abajo.agachado = true;
    std::vector<Caja> a, b;
    construir(poseDe(de_pie), a);
    construir(poseDe(abajo), b);
    CHECK(b[0].centro.y < a[0].centro.y - 0.2f);    // el torso baja
    CHECK(poseDe(abajo).rodillaI > 1.0f);
    CHECK(masBajo(b) > -0.15f);                      // sin hundirse en el suelo
}

TEST_CASE("Jugador: picar mueve el brazo derecho y no el izquierdo") {
    Estado e;
    e.golpeando = true;
    e.tiempo = 0.13f;
    const Pose p = poseDe(e);
    CHECK(p.hombroD > 0.8f);
    CHECK(std::fabs(p.hombroI) < 0.1f);
}

TEST_CASE("Parpadeo: ritmo humano, corto, y nunca se queda cerrado") {
    Parpadeo par;
    int parpadeos = 0, cerradoSeguido = 0, maxSeguido = 0, cerrados = 0;
    bool antes = false;
    const float dt = 1.0f / 120.0f;
    for (int i = 0; i < 120 * 60; ++i) {          // un minuto
        const float c = par.avanzar(dt);
        const bool ahora = c > 0.0f;
        if (ahora && !antes) ++parpadeos;
        cerradoSeguido = ahora ? cerradoSeguido + 1 : 0;
        if (cerradoSeguido > maxSeguido) maxSeguido = cerradoSeguido;
        if (c > 0.5f) ++cerrados;
        CHECK(c >= 0.0f); CHECK(c <= 1.0f);
        antes = ahora;
    }
    CHECK(parpadeos >= 9);              // ~15 por minuto, como una persona
    CHECK(parpadeos <= 22);
    CHECK(maxSeguido * dt <= 0.2f);     // cada uno dura menos de 0.2 s
    CHECK(cerrados * dt < 2.0f);        // en un minuto, apenas 1-2 s de ojos cerrados
    // Un tiron enorme no lo deja cerrado.
    Parpadeo t;
    t.proximo = 0.0f;
    t.avanzar(0.01f);
    CHECK(t.avanzar(5.0f) < 1.0f);
}

TEST_CASE("Jugador: agachado, el ojo de la camara queda por encima de los hombros") {
    Estado e;
    e.agachado = true;
    const Pose p = poseDe(e);
    std::vector<Caja> cs;
    construir(p, cs);
    const float ky = escalaVertical(p, 0.80f);
    for (const Caja& c : cs) CHECK((c.centro.y + c.medio.y) * ky < 0.80f);
    CHECK(escalaVertical(poseDe(Estado{}), 1.62f) == doctest::Approx(1.0f));
}
