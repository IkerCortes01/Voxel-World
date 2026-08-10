#include <doctest/doctest.h>

#include <cstdlib>
#include <initializer_list>
#include <utility>

// ============================================================================
// Compatibilidad del RNG usado por PerlinNoise
// ============================================================================
// PerlinNoise barajaba su tabla de permutación con srand()/rand(), estado
// global del proceso. Se sustituyó por un LCG local que reproduce exactamente
// la secuencia de rand() de MSVC, para que la permutación —y por tanto el
// terreno de los mundos ya creados— no cambie.
//
// Si este test falla, la generación de terreno dejará de coincidir con la de
// los mundos existentes: hay que versionar el formato del mundo, no "arreglar"
// el test.

namespace {
struct MsvcLcg {
    unsigned int state;
    explicit MsvcLcg(unsigned int seed) : state(seed) {}
    int next() {
        state = state * 214013u + 2531011u;
        return static_cast<int>((state >> 16) & 0x7FFF);
    }
};
}

TEST_CASE("El LCG local reproduce la secuencia de rand() de la CRT") {
    for (unsigned int seed : {0u, 1u, 12345u, 777u, 4294967295u}) {
        CAPTURE(seed);
        srand(seed);
        MsvcLcg lcg(seed);
        for (int i = 0; i < 512; i++) {
            CHECK(lcg.next() == rand());
        }
    }
}

TEST_CASE("El barajado de la permutacion coincide con el original") {
    // Réplica exacta del bucle del constructor de PerlinNoise, comparando el
    // resultado con la versión que usaba rand().
    const unsigned int seed = 12345 + 777;

    int viaRand[256];
    for (int i = 0; i < 256; i++) viaRand[i] = i;
    srand(seed);
    for (int i = 255; i > 0; i--) {
        int j = rand() % (i + 1);
        std::swap(viaRand[i], viaRand[j]);
    }

    int viaLcg[256];
    for (int i = 0; i < 256; i++) viaLcg[i] = i;
    MsvcLcg lcg(seed);
    for (int i = 255; i > 0; i--) {
        int j = lcg.next() % (i + 1);
        std::swap(viaLcg[i], viaLcg[j]);
    }

    bool identical = true;
    for (int i = 0; i < 256; i++) {
        if (viaRand[i] != viaLcg[i]) { identical = false; break; }
    }
    CHECK(identical);
}

TEST_CASE("El LCG local no toca el estado global de rand()") {
    srand(999);
    int expected[8];
    for (int i = 0; i < 8; i++) expected[i] = rand();

    srand(999);
    int first = rand();
    MsvcLcg lcg(4242);          // no debe alterar la secuencia global
    for (int i = 0; i < 50; i++) lcg.next();

    CHECK(first == expected[0]);
    for (int i = 1; i < 8; i++) {
        CHECK(rand() == expected[i]);
    }
}
