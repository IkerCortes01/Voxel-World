#include <doctest/doctest.h>
#include "audio/Musica.h"
#include <set>

// ============================================================================
// LA MUSICA: AL AZAR, SIN REPETIR, CON SILENCIOS LARGOS
// ============================================================================

using Audio::SelectorMusica;

namespace {
// Avanza de segundo en segundo hasta que toque una pieza; devuelve cuanto tardo.
float hastaLaSiguiente(SelectorMusica& s, int& pieza) {
    float t = 0.0f;
    pieza = -1;
    while (pieza < 0 && t < 10000.0f) { pieza = s.avanzar(1.0f); t += 1.0f; }
    return t;
}
} // namespace

TEST_CASE("Musica: la primera suena entre 45 s y 2 min, y despues hay silencios de 2 a 6") {
    for (uint32_t semilla = 1; semilla < 30; ++semilla) {
        SelectorMusica s(semilla);
        s.poner(4);
        int p = -1;
        const float primera = hastaLaSiguiente(s, p);
        CHECK(primera >= SelectorMusica::PRIMERO_MIN);
        CHECK(primera <= SelectorMusica::PRIMERO_MAX + 1.0f);
        // Mientras suena no se pide otra.
        for (int i = 0; i < 600; ++i) CHECK(s.avanzar(1.0f) < 0);
        s.acabo();
        const float silencio = hastaLaSiguiente(s, p);
        CHECK(silencio >= SelectorMusica::SILENCIO_MIN);
        CHECK(silencio <= SelectorMusica::SILENCIO_MAX + 1.0f);
    }
}

TEST_CASE("Musica: salen todas antes de repetir, y nunca la misma dos veces seguidas") {
    for (uint32_t semilla = 1; semilla < 50; ++semilla) {
        SelectorMusica s(semilla);
        s.poner(4);
        int anterior = -1;
        for (int ronda = 0; ronda < 5; ++ronda) {
            std::set<int> vistas;
            for (int k = 0; k < 4; ++k) {
                int p = -1;
                hastaLaSiguiente(s, p);
                REQUIRE(p >= 0);
                REQUIRE(p < 4);
                CHECK(p != anterior);
                vistas.insert(p);
                anterior = p;
                s.acabo();
            }
            CHECK(vistas.size() == 4);
        }
    }
}

TEST_CASE("Musica: el orden cambia de una partida a otra") {
    std::set<std::vector<int>> ordenes;
    for (uint32_t semilla = 1; semilla < 20; ++semilla) {
        SelectorMusica s(semilla * 7919u);
        s.poner(4);
        std::vector<int> orden;
        for (int k = 0; k < 4; ++k) { int p = -1; hastaLaSiguiente(s, p); orden.push_back(p); s.acabo(); }
        ordenes.insert(orden);
    }
    CHECK(ordenes.size() > 5);
}

TEST_CASE("Musica: sin piezas no pide nada") {
    SelectorMusica s;
    s.poner(0);
    for (int i = 0; i < 1000; ++i) CHECK(s.avanzar(1.0f) < 0);
}
