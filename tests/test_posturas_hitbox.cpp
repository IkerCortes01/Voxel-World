#include <doctest/doctest.h>
#include "player/MallaImportada.h"

// ============================================================================
// LAS POSTURAS CABEN EN SU HITBOX (ver hundirHasta)
// ============================================================================
// Agachado la hitbox mide 0.95 con la camara a 0.86; tumbado, 0.55 y 0.42.
// La pose tiene que dejar el ojo ahi sin aplastar el cuerpo.

using namespace ModeloJugador;

namespace {
Esqueleto postura(bool agachado, bool tumbado, Pose* sale = nullptr) {
    Estado e;
    e.enSuelo = true;
    e.agachado = agachado;
    e.tumbado = tumbado;
    e.tiempo = 5.0f;
    Pose p = poseDe(e);
    limitar(p);
    if (tumbado) hundirHasta(p, 0.42f, true);
    else if (agachado) hundirHasta(p, 0.86f, false);
    if (sale) *sale = p;
    return resolver(p, true);
}
} // namespace

TEST_CASE("Posturas: agachado, el ojo queda en la camara y no hay que aplastar") {
    const Esqueleto s = postura(true, false);
    CHECK(s.ojo == doctest::Approx(0.86f).epsilon(0.03));
    CHECK(escalaVertical(s, 0.86f) > 0.95f);
    // Los pies en el suelo.
    CHECK(detalle::plantaMasBaja(s) == doctest::Approx(0.0f).epsilon(0.01));
}

TEST_CASE("Posturas: tumbado, igual") {
    const Esqueleto s = postura(false, true);
    CHECK(s.ojo == doctest::Approx(0.42f).epsilon(0.05));
}

TEST_CASE("Posturas: si el ojo ya cabe, no se toca nada") {
    Estado e;
    e.enSuelo = true;
    Pose p = poseDe(e);
    limitar(p);
    const Pose antes = p;
    hundirHasta(p, 1.70f, false);
    CHECK(p.pierna[0].rodilla == doctest::Approx(antes.pierna[0].rodilla));
    CHECK(p.inclinacion == doctest::Approx(antes.inclinacion));
}

TEST_CASE("Posturas: el centro en planta queda entre la pelvis y lo de delante") {
    const Esqueleto de = postura(false, false), ag = postura(true, false);
    const V c = centroEnPlanta(de);
    CHECK(std::fabs(c.x) < 0.02f);
    CHECK(std::fabs(c.z - de.pelvis.p.z) < 0.08f);
    // Agachado el centro se adelanta: las rodillas y la cabeza van delante.
    CHECK(centroEnPlanta(ag).z > c.z + 0.05f);
}