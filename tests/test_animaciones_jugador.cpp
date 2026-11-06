#include <doctest/doctest.h>
#include "player/ModeloJugador.h"
#include "player/MallaJugador.h"
#include "player/PlayerTypes.h"
#include <cmath>

using namespace ModeloJugador;

namespace {
Estado picando(Herramienta h, ClaseBloque c, float t) {
    Estado e;
    e.accion = Accion::PICAR;
    e.herramienta = h;
    e.bloque = c;
    e.tiempo = t;
    return e;
}
} // namespace

TEST_CASE("Animaciones: 5 herramientas x 4 bloques, todas distintas y validas") {
    for (int h = 0; h < (int)Herramienta::NUM; ++h) {
        for (int c = 0; c < (int)ClaseBloque::NUM; ++c) {
            const Golpe& g = golpeDe((Herramienta)h, (ClaseBloque)c);
            CHECK(g.periodo > 0.2f);
            CHECK(g.periodo < 1.0f);
            CHECK(g.carga > 0.3f);
            CHECK(g.carga < 0.8f);
            // Cada una se distingue de las otras tres de la misma herramienta.
            for (int o = 0; o < c; ++o) {
                const Golpe& k = golpeDe((Herramienta)h, (ClaseBloque)o);
                const float dif = std::fabs(g.flex[0] - k.flex[0]) + std::fabs(g.abd[0] - k.abd[0]) +
                                  std::fabs(g.codo[1] - k.codo[1]) + std::fabs(g.periodo - k.periodo);
                CHECK(dif > 0.05f);
            }
        }
    }
}

TEST_CASE("Animaciones: el pico en piedra carga desde arriba y descarga con todo") {
    float lo = 9, hi = -9, inclMax = 0;
    const Golpe& g = golpeDe(Herramienta::PICO, ClaseBloque::PIEDRA);
    for (int k = 0; k < 100; ++k) {
        const Pose p = poseDe(picando(Herramienta::PICO, ClaseBloque::PIEDRA, g.periodo * k / 100.0f));
        lo = std::fmin(lo, p.brazo[1].flex);
        hi = std::fmax(hi, p.brazo[1].flex);
        inclMax = std::fmax(inclMax, p.inclinacion);
        CHECK(p.apretar == 1.0f);
        CHECK(p.enMano == (uint8_t)Herramienta::PICO);
    }
    CHECK(hi - lo > 1.8f);
    CHECK(inclMax > 0.2f);
    // A dos manos: la izquierda sigue a la derecha.
    const Pose p = poseDe(picando(Herramienta::PICO, ClaseBloque::PIEDRA, 0.1f));
    CHECK(p.brazo[0].flex > 0.5f);
    // En lo construido, toques cortos: mucho menos recorrido.
    lo = 9; hi = -9;
    const Golpe& c = golpeDe(Herramienta::PICO, ClaseBloque::CONSTRUIDO);
    for (int k = 0; k < 100; ++k) {
        const float f = poseDe(picando(Herramienta::PICO, ClaseBloque::CONSTRUIDO, c.periodo * k / 100.0f)).brazo[1].flex;
        lo = std::fmin(lo, f); hi = std::fmax(hi, f);
    }
    CHECK(hi - lo < 0.8f);
}

TEST_CASE("Animaciones: el golpe acelera hacia el impacto") {
    const float carga = 0.6f;
    // Justo al empezar a golpear va despacio y al final deprisa.
    const float a = faseGolpe(carga + 0.02f, carga) - faseGolpe(carga, carga);
    const float b = faseGolpe(carga + 0.16f, carga) - faseGolpe(carga + 0.14f, carga);
    CHECK(b > a * 3.0f);
    CHECK(faseGolpe(0.0f, carga) == doctest::Approx(1.0f));
    CHECK(faseGolpe(carga, carga) == doctest::Approx(0.0f));
}

TEST_CASE("Animaciones: punetazo al aire, un directo que se estira y termina") {
    Estado e;
    e.accion = Accion::GOLPE_AIRE;
    e.tAccion = 0.07f + 0.10f;           // fin del golpe: el impacto
    const Pose p = poseDe(e);
    CHECK(p.brazo[1].codo < 0.15f);
    CHECK(p.brazo[1].flex > 1.4f);
    e.tAccion = 0.03f;                  // recogiendo el brazo
    CHECK(poseDe(e).brazo[1].codo > 1.0f);
    e.tAccion = 1.0f;                   // ya termino: brazo normal
    CHECK(poseDe(e).brazo[1].codo < 0.6f);
}

TEST_CASE("Animaciones: tres golpes distintos a un animal") {
    Estado e;
    e.accion = Accion::GOLPE_ANIMAL;
    e.tAccion = 0.10f + 0.12f;          // impacto
    e.variante = 0;
    const Pose directo = poseDe(e);
    CHECK(directo.brazo[1].flex > 1.4f);
    CHECK(directo.giroTorso > 0.15f);
    e.variante = 1;
    const Pose gancho = poseDe(e);
    CHECK(gancho.brazo[0].abd > 1.0f);       // de izquierda, el codo de lado
    CHECK(gancho.giroTorso < -0.2f);         // el torso gira al reves
    e.variante = 2;
    e.ladoGolpe = 0;
    const Pose abajoI = poseDe(e);
    CHECK(abajoI.brazo[0].flex > 1.8f);      // de abajo arriba, con la izquierda
    e.ladoGolpe = 1;
    const Pose abajoD = poseDe(e);
    CHECK(abajoD.brazo[1].flex > 1.8f);
    // Antes del impacto, el gancho de abajo arranca con el brazo bajo.
    e.tAccion = 0.10f;
    CHECK(poseDe(e).brazo[1].flex < 0.1f);
}

TEST_CASE("Animaciones: agachado distinto en suelo natural, construido y agua") {
    Estado e;
    e.agachado = true;
    e.superficie = Superficie::NATURAL;
    const Pose n = poseDe(e);
    CHECK(n.brazo[0].munFlex < -0.8f);       // la palma plana en el suelo
    e.superficie = Superficie::CONSTRUCCION;
    const Pose c = poseDe(e);
    CHECK(std::fabs(c.brazo[0].codo - c.brazo[1].codo) < 0.01f);   // las dos en las rodillas
    CHECK(c.inclinacion < n.inclinacion);
    e.superficie = Superficie::LIQUIDO;
    const Pose l = poseDe(e);
    CHECK(l.brazo[0].abd > 0.8f);
    CHECK(l.brazo[1].abd > 0.8f);
    // El agua mece las munecas.
    e.tiempo = 0.5f;
    const float m1 = poseDe(e).brazo[0].munFlex;
    e.tiempo = 2.4f;
    CHECK(std::fabs(poseDe(e).brazo[0].munFlex - m1) > 0.1f);
}

TEST_CASE("Animaciones: subir un nivel levanta la pierna, bajarlo la estira") {
    Estado e;
    const Pose quieto = poseDe(e);
    e.escalon = 0.5f;
    e.escalonSube = true;
    e.piernaEscalon = 1;
    const Pose sube = poseDe(e);
    CHECK(sube.pierna[1].rodilla > quieto.pierna[1].rodilla + 0.8f);
    CHECK(sube.pierna[1].flex > quieto.pierna[1].flex + 0.4f);
    e.escalonSube = false;
    const Pose baja = poseDe(e);
    CHECK(baja.pierna[1].tobillo < quieto.pierna[1].tobillo - 0.3f);
    e.escalon = -1.0f;
    CHECK(poseDe(e).pierna[1].rodilla == doctest::Approx(quieto.pierna[1].rodilla));
}

TEST_CASE("Animaciones: todo dentro de los limites humanos") {
    for (int k = 0; k < 300; ++k) {
        Estado e;
        e.rapidez = (k % 7) * 0.9f;
        e.fasePaso = k * 0.31f;
        e.agachado = k % 5 == 0;
        e.superficie = (Superficie)(k % 3);
        e.accion = (Accion)(k % 4);
        e.herramienta = (Herramienta)(k % 5);
        e.bloque = (ClaseBloque)((k / 5) % 4);
        e.variante = k % 3;
        e.ladoGolpe = k % 2;
        e.tAccion = (k % 11) * 0.04f;
        e.tiempo = k * 0.053f;
        e.escalon = (k % 4) * 0.3f - 0.1f;
        const Pose p = poseDe(e);
        for (const Brazo& b : p.brazo) {
            CHECK(b.codo >= 0.0f);
            CHECK(b.codo <= Lim::CODO_MAX);
            CHECK(b.flex <= Lim::HOMBRO_FLEX_MAX);
        }
        for (const Pierna& g : p.pierna) {
            CHECK(g.rodilla >= 0.0f);
            CHECK(g.rodilla <= Lim::RODILLA_MAX);
        }
    }
}

TEST_CASE("Animaciones: la herramienta se dibuja en la mano derecha") {
    const Piel piel = generarPiel(3u);
    Fisiologia f;
    Malla sin, con;
    Estado e = picando(Herramienta::MANO, ClaseBloque::TIERRA, 0.1f);
    Pose p = poseDe(e);
    generar(resolver(p), p, piel, f, Calidad{}, sin);
    for (int h = 1; h < (int)Herramienta::NUM; ++h) {
        e.herramienta = (Herramienta)h;
        p = poseDe(e);
        const Esqueleto s = resolver(p);
        generar(s, p, piel, f, Calidad{}, con);
        CHECK(con.v.size() > sin.v.size() + 50);
        // Algun vertice de la herramienta queda a mas de 25 cm del puno.
        float lejos = 0.0f;
        for (size_t i = sin.v.size(); i < con.v.size(); ++i)
            lejos = std::fmax(lejos, largo(resta(con.v[i].p, s.mano[1].muneca.p)));
        CHECK(lejos > 0.25f);
    }
}

TEST_CASE("Escalon: los niveles se suben andando, los bloques completos no") {
    PlayerSys::MovementConfig cfg;
    CHECK(cfg.stepHeight > 13.0f / 16.0f);   // el nivel 7, el mas alto
    CHECK(cfg.stepHeight < 1.0f);            // un bloque entero se salta
}

TEST_CASE("Dedos: mas gruesos y separados que antes, sin chocar entre ellos") {
    const Esqueleto s = resolver(poseDe(Estado{}));
    for (int l = 0; l < 2; ++l) {
        for (int d = 0; d + 1 < 5; ++d) {
            const float sep = largo(resta(s.pie[l].dedo[d][0].p, s.pie[l].dedo[d + 1][0].p));
            CHECK(sep > radioDedoPie(d) * 0.9f);    // se ve el hueco entre ellos
        }
    }
    CHECK(radioDedoPie(0) > 0.015f);
}
