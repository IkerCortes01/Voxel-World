// La mini IA que juega sola en el fondo del menu principal (src/ui/MenuVivo.h).
// Es una cabecera sin motor: aqui juega de verdad, veinte minutos seguidos.
#include "doctest/doctest.h"

#include "ui/MenuVivo.h"

#include <cmath>

using namespace MenuVivo;

namespace {
void comprobarMundo(const Sim& s) {
    for (int i = 0; i < N; ++i) {
        REQUIRE(s.col[i].alto >= 0);
        REQUIRE(s.col[i].alto <= ALTO_MAX);
        // El suelo siempre es andable: un escalon como mucho.
        REQUIRE(std::abs(s.col[i].alto - s.col[mod(i + 1)].alto) <= 1);
        REQUIRE(s.col[i].arbol >= 0);
        REQUIRE(s.col[i].arbol <= ARBOL_MADURO);
    }
    REQUIRE(s.x >= 0.0f);
    REQUIRE(s.x < (float)N);
    REQUIRE(s.madera >= 0);
    REQUIRE(s.piedra >= 0);
    REQUIRE((int)s.casas.size() <= MAX_CASAS);
    for (const Casa& c : s.casas) {
        REQUIRE(c.piezas >= 0);
        REQUIRE(c.piezas <= PIEZAS_CASA);
        // La casa esta en llano y sin arboles dentro.
        for (int k = -1; k <= 1; ++k) {
            REQUIRE(s.col[mod(c.centro + k)].alto == s.col[mod(c.centro)].alto);
            REQUIRE(s.col[mod(c.centro + k)].arbol == 0);
        }
    }
}
}  // namespace

TEST_CASE("Menu vivo: el anillo se envuelve y mide por el camino corto") {
    CHECK(mod(-1) == N - 1);
    CHECK(mod(N) == 0);
    CHECK(delta(1.0f, 3.0f) == doctest::Approx(2.0f));
    CHECK(delta(1.0f, (float)N - 1.0f) == doctest::Approx(-2.0f));
    CHECK(delta((float)N - 1.0f, 1.0f) == doctest::Approx(2.0f));
    CHECK(envolver(-0.5f) == doctest::Approx((float)N - 0.5f));
}

TEST_CASE("Menu vivo: el mundo nace andable y con bosque") {
    for (uint32_t semilla = 1; semilla <= 40; ++semilla) {
        Sim s;
        s.iniciar(semilla * 7919u);
        comprobarMundo(s);
        CHECK(s.arboles() >= 8);
    }
}

TEST_CASE("Menu vivo: la IA juega veinte minutos, hace de todo y no se atasca") {
    for (uint32_t semilla : { 1u, 42u, 2026u, 99991u }) {
        Sim s;
        s.iniciar(semilla);
        float sinDecidir = 0.0f, peor = 0.0f, andado = 0.0f;
        int antes = s.decisiones, bosqueMin = 9999;
        const float dt = 1.0f / 60.0f;
        for (int f = 0; f < 60 * 60 * 20; ++f) {
            const float x0 = s.x;
            s.avanzar(dt);
            andado += std::fabs(delta(x0, s.x));
            if (s.decisiones != antes) { antes = s.decisiones; sinDecidir = 0.0f; }
            else sinDecidir += dt;
            peor = std::fmax(peor, sinDecidir);
            if (f % 600 == 0) { comprobarMundo(s); bosqueMin = (std::min)(bosqueMin, s.arboles()); }
        }
        comprobarMundo(s);
        CAPTURE(semilla);
        MESSAGE("semilla " << semilla << ": decisiones " << s.decisiones << ", talo " << s.hechas[(int)Tarea::TALAR]
                << ", planto " << s.hechas[(int)Tarea::PLANTAR] << ", piezas " << s.hechas[(int)Tarea::CONSTRUIR]
                << ", desmonto " << s.hechas[(int)Tarea::DESMONTAR] << ", allano " << s.hechas[(int)Tarea::ALLANAR]
                << ", descanso " << s.hechas[(int)Tarea::DESCANSAR] << ", bosque minimo " << bosqueMin
                << ", peor espera " << peor << " s, anduvo " << andado);
        // Hace de todo.
        CHECK(s.hechas[(int)Tarea::TALAR] >= 10);
        CHECK(s.hechas[(int)Tarea::PLANTAR] >= 10);
        CHECK(s.hechas[(int)Tarea::CONSTRUIR] >= 2 * PIEZAS_CASA);
        CHECK(s.hechas[(int)Tarea::DESMONTAR] >= 1);
        CHECK(s.hechas[(int)Tarea::DESCANSAR] >= 3);
        // No se atasca: nunca pasa mas de un minuto sin decidir algo nuevo
        // (cruzar medio anillo son 25 s y la tarea mas larga, 6).
        CHECK(peor < 60.0f);
        CHECK(s.decisiones > 150);
        CHECK(andado > 200.0f);
        // Y no acaba con el bosque.
        CHECK(bosqueMin >= 4);
    }
}

TEST_CASE("Menu vivo: talar da madera y con madera se levanta una casa") {
    Sim s;
    s.iniciar(5u);
    for (Columna& c : s.col) { c = Columna(); }
    s.casas.clear();
    s.x = 10.5f;
    s.col[14].arbol = ARBOL_MADURO;
    s.ir(14, Tarea::TALAR, true);
    for (int f = 0; f < 600 && s.hechas[(int)Tarea::TALAR] == 0; ++f) s.avanzar(1.0f / 60.0f);
    CHECK(s.hechas[(int)Tarea::TALAR] == 1);
    CHECK(s.col[14].arbol == 0);
    CHECK(s.madera == MADERA_POR_ARBOL);
    CHECK(std::fabs(delta(s.x, 14.5f)) == doctest::Approx(1.0f));    // se paro al lado, no encima
    CHECK(s.dir == 1);                                               // mirando al arbol

    s.madera = PIEZAS_CASA;
    s.pensar(0.0f);
    for (int f = 0; f < 60 * 60 && (s.casas.empty() || s.casas[0].piezas < PIEZAS_CASA); ++f) s.avanzar(1.0f / 60.0f);
    REQUIRE(s.casas.size() == 1);
    CHECK(s.casas[0].piezas == PIEZAS_CASA);
    CHECK(s.madera == 0);
}

TEST_CASE("Menu vivo: allanar pica el bulto y rellena el hoyo sin romper el suelo") {
    Sim s;
    s.iniciar(9u);
    for (Columna& c : s.col) { c = Columna(); c.alto = 2; }
    s.casas.clear();
    s.col[20].alto = 3;
    s.col[30].alto = 1;
    CHECK(s.allanable(20) == -1);
    CHECK(s.allanable(30) == 1);
    CHECK(s.allanable(25) == 0);
    s.x = 17.5f;
    s.ir(20, Tarea::ALLANAR, true);
    for (int f = 0; f < 600 && s.hechas[(int)Tarea::ALLANAR] == 0; ++f) s.avanzar(1.0f / 60.0f);
    CHECK(s.col[20].alto == 2);
    CHECK(s.piedra == 1);
    s.ir(30, Tarea::ALLANAR, true);
    for (int f = 0; f < 900 && s.hechas[(int)Tarea::ALLANAR] == 1; ++f) s.avanzar(1.0f / 60.0f);
    CHECK(s.col[30].alto == 2);
    CHECK(s.piedra == 0);
}

TEST_CASE("Menu vivo: un retono crece hasta ser un arbol") {
    Sim s;
    s.iniciar(3u);
    for (Columna& c : s.col) c = Columna();
    s.col[50].arbol = 1;
    s.tarea = Tarea::DESCANSAR;
    s.reloj = 1e9f;                      // que no haga nada mas
    for (int f = 0; f < (int)(2.2f * CRECER_SEG * 60.0f); ++f) s.avanzar(1.0f / 60.0f);
    CHECK(s.col[50].arbol == ARBOL_MADURO);
}
