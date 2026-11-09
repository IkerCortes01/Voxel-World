#include <doctest/doctest.h>
#include "player/Movimiento.h"
#include "player/CabezaJugador.h"
#include <cmath>

// ============================================================================
// LAS ARTICULACIONES, LOS LIMITES ACOPLADOS Y LAS ANIMACIONES PROGRAMABLES
// ============================================================================

using namespace ModeloJugador;

namespace {
float dist(V a, V b) { return largo(resta(a, b)); }
}

TEST_CASE("Movimiento: cada canal se lee como se pone") {
    const auto& C = Movimiento::canales();
    REQUIRE(C.size() > 40);
    for (int c = 0; c < (int)C.size(); ++c) {
        Pose p;
        const float v = C[c].minimo + (C[c].maximo - C[c].minimo) * 0.4f;
        Movimiento::poner(p, c, v);
        CHECK(Movimiento::leer(p, c) == doctest::Approx(v).epsilon(0.001));
        CHECK(Movimiento::buscarCanal(C[c].nombre) == c);
    }
}

TEST_CASE("Movimiento: las animaciones del juego se leen sin errores") {
    const auto& v = Movimiento::incluidas();
    CHECK(v.size() >= 8);
    for (const auto& a : v) {
        std::string err;
        Movimiento::Animacion b;
        CHECK(Movimiento::leerAnimacion(Movimiento::textoDe(a), b, &err));
        CHECK(err.empty());
        CHECK(b.claves.size() == a.claves.size());
    }
}

TEST_CASE("Movimiento: un canal mal escrito se avisa y '.a' son los dos lados") {
    Movimiento::Animacion a;
    std::string err;
    CHECK(Movimiento::leerAnimacion("nombre X\nduracion 1\n0.5 codo.a=90 brazo.z.flex=3\n", a, &err));
    CHECK(err.find("BRAZO.Z.FLEX") == std::string::npos);   // el aviso va en minusculas, tal cual
    CHECK(err.find("brazo.z.flex") != std::string::npos);
    REQUIRE(a.claves.size() == 1);
    CHECK(a.claves[0].valores.size() == 2);
}

TEST_CASE("Movimiento: sin bucle entra y sale desde la pose del juego, sin saltos") {
    const Movimiento::Animacion* saludo = nullptr;
    for (const auto& a : Movimiento::incluidas()) if (a.nombre == "SALUDAR") saludo = &a;
    REQUIRE(saludo);
    Pose base;
    limitar(base);
    const int abd = Movimiento::buscarCanal("brazo.d.abd");
    Pose p0 = base; Movimiento::aplicar(*saludo, 0.0f, p0);
    Pose p1 = base; Movimiento::aplicar(*saludo, saludo->duracion, p1);
    Pose pm = base; Movimiento::aplicar(*saludo, 1.0f, pm);
    CHECK(Movimiento::leer(p0, abd) == doctest::Approx(Movimiento::leer(base, abd)).epsilon(0.01));
    CHECK(Movimiento::leer(p1, abd) == doctest::Approx(Movimiento::leer(base, abd)).epsilon(0.01));
    CHECK(Movimiento::leer(pm, abd) > 80.0f);
    // Ningun canal salta de un frame a otro (60 fps).
    float maxSalto = 0.0f;
    Pose antes = base;
    Movimiento::aplicar(*saludo, 0.0f, antes);
    for (int i = 1; i <= (int)(saludo->duracion * 60.0f); ++i) {
        Pose p = base;
        Movimiento::aplicar(*saludo, i / 60.0f, p);
        for (int c = 0; c < (int)Movimiento::canales().size(); ++c)
            if (Movimiento::canales()[c].unidad == Movimiento::GRADOS)
                maxSalto = std::fmax(maxSalto, std::fabs(Movimiento::leer(p, c) - Movimiento::leer(antes, c)));
        antes = p;
    }
    CHECK(maxSalto < 9.0f);
}

TEST_CASE("Anatomia: los isquiotibiales frenan la cadera con la rodilla recta") {
    Pose p;
    p.pierna[0].flex = 2.0f;
    p.pierna[0].rodilla = 0.0f;
    limitar(p);
    CHECK(p.pierna[0].flex == doctest::Approx(Lim::CADERA_FLEX_RODILLA_RECTA));
    p.pierna[0].flex = 2.0f;
    p.pierna[0].rodilla = 1.6f;
    limitar(p);
    CHECK(p.pierna[0].flex == doctest::Approx(2.0f));
}

TEST_CASE("Anatomia: los gemelos frenan el tobillo con la rodilla recta") {
    Pose p;
    p.pierna[1].tobillo = 0.35f;
    p.pierna[1].rodilla = 0.0f;
    limitar(p);
    CHECK(p.pierna[1].tobillo == doctest::Approx(Lim::TOBILLO_MAX_RODILLA_RECTA));
    p.pierna[1].tobillo = 0.35f;
    p.pierna[1].rodilla = 1.2f;
    limitar(p);
    CHECK(p.pierna[1].tobillo == doctest::Approx(0.35f));
}

TEST_CASE("Anatomia: la ultima falange no se dobla sola") {
    Pose p;
    p.brazo[1].dedo[0][1] = 0.0f;
    p.brazo[1].dedo[0][2] = 1.4f;
    limitar(p);
    CHECK(p.brazo[1].dedo[0][2] < 0.4f);
}

TEST_CASE("Anatomia: el codo no se dobla hacia atras aunque la animacion lo pida") {
    Movimiento::Animacion a;
    REQUIRE(Movimiento::leerAnimacion("nombre MAL\nduracion 1\n0.5 codo.d=-60\n", a));
    Pose p;
    Movimiento::aplicar(a, 0.5f, p);
    CHECK(p.brazo[1].codo >= 0.0f);
}

TEST_CASE("Anatomia: al subir el brazo la escapula sube con el") {
    Pose abajo, arriba;
    arriba.brazo[1].flex = 2.9f;
    const Esqueleto a = resolver(abajo, false), b = resolver(arriba, false);
    CHECK(b.hombro[1].p.y > a.hombro[1].p.y + 0.015f);
}

TEST_CASE("Anatomia: cada hueso mide lo que dicen las proporciones") {
    Pose p;
    p.prop.muslo = 1.1f;
    p.prop.brazo = 0.9f;
    const Esqueleto s = resolver(p, true);
    CHECK(dist(s.cadera[0].p, s.rodilla[0].p) == doctest::Approx(M::MUSLO * 1.1f).epsilon(0.001));
    CHECK(dist(s.hombro[1].p, s.codo[1].p) == doctest::Approx(M::BRAZO * 0.9f).epsilon(0.001));
    // De pie, los pies siguen en el suelo y la cadera sube.
    const Esqueleto n = resolver(Pose{}, true);
    CHECK(s.pelvis.p.y > n.pelvis.p.y + 0.03f);
}

TEST_CASE("Anatomia: la flexibilidad baja con la edad") {
    Avatar::Datos d;
    Pose joven, viejo;
    d.poner(Avatar::CUERPO_EDAD, 25.0f);
    aplicarProporciones(d, joven);
    d.poner(Avatar::CUERPO_EDAD, 80.0f);
    aplicarProporciones(d, viejo);
    CHECK(viejo.flexibilidad < joven.flexibilidad * 0.85f);
}

TEST_CASE("Movimiento: el cuerpo sale sano en todos los extremos de cada canal") {
    Calidad q;
    q.detalle = 0;
    q.venas = false;
    const Piel piel = generarPiel(3);
    Fisiologia fis;
    for (int c = 0; c < (int)Movimiento::canales().size(); ++c) {
        for (int lado = 0; lado < 2; ++lado) {
            Pose p;
            const auto& ca = Movimiento::canales()[c];
            Movimiento::poner(p, c, lado ? ca.maximo : ca.minimo);
            limitar(p);
            const Esqueleto s = resolver(p, true);
            Malla m;
            generar(s, p, piel, fis, q, m);
            bool sana = !m.i.empty();
            for (const auto& v : m.v)
                if (!std::isfinite(v.p.x) || !std::isfinite(v.p.y) || !std::isfinite(v.p.z)) sana = false;
            CHECK(sana);
        }
    }
}
