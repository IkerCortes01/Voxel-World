#include <doctest/doctest.h>
#include "terrain/CaveGenerator.h"
#include <cstdio>

// ============================================================================
// DENSIDAD DE CUEVAS
// ============================================================================
// Mide que fraccion del subsuelo queda hueca. No comprueba un numero magico:
// comprueba que este DENTRO DE UN RANGO razonable, que es lo que de verdad
// importa. Muy poco = no hay cuevas; demasiado = la roca se deshace y el
// terreno se vuelve inestable y feo.
//
// Ademas fija el determinismo: la misma semilla y posicion tienen que dar
// siempre la misma cueva, o el mundo cambiaria segun el orden de exploracion.

static double fraccionHueca(int seed, int surfaceHeight) {
    TerrainGen::CaveGenerator caves(seed);
    long long total = 0, huecos = 0;

    // Una loncha de 96x96 columnas del mundo, por debajo de la superficie.
    for (int x = 0; x < 96; ++x) {
        for (int z = 0; z < 96; ++z) {
            for (int y = 8; y < surfaceHeight - 6; ++y) {
                ++total;
                if (caves.IsCave((float)(x * 3), y, (float)(z * 3), surfaceHeight))
                    ++huecos;
            }
        }
    }
    return total ? (double)huecos / (double)total : 0.0;
}

TEST_CASE("Cuevas: el subsuelo queda muy hueco, pero no deshecho") {
    const double f = fraccionHueca(12345, 80);
    printf("[CUEVAS] fraccion hueca = %.1f%%\n", f * 100.0);

    // Con el ajuste "super comunes" se busca bastante mas que el ~15% previo.
    CHECK(f > 0.20);
    // Y un techo: por encima de esto el terreno se cae a pedazos.
    CHECK(f < 0.60);
}

TEST_CASE("Cuevas: la misma semilla da siempre la misma cueva") {
    TerrainGen::CaveGenerator a(999), b(999);
    for (int i = 0; i < 400; ++i) {
        const float x = (float)(i * 7 % 200);
        const int   y = 10 + (i % 60);
        const float z = (float)(i * 13 % 200);
        CHECK(a.IsCave(x, y, z, 80) == b.IsCave(x, y, z, 80));
    }
}

TEST_CASE("Cuevas: semillas distintas dan mundos distintos") {
    TerrainGen::CaveGenerator a(1), b(2);
    int diferencias = 0;
    for (int i = 0; i < 400; ++i) {
        const float x = (float)(i * 7 % 200);
        const int   y = 10 + (i % 60);
        const float z = (float)(i * 13 % 200);
        if (a.IsCave(x, y, z, 80) != b.IsCave(x, y, z, 80)) ++diferencias;
    }
    CHECK(diferencias > 20);
}

TEST_CASE("Cuevas: no perforan la superficie ni la bedrock") {
    TerrainGen::CaveGenerator caves(4242);
    const int sup = 80;

    for (int x = 0; x < 60; ++x) {
        for (int z = 0; z < 60; ++z) {
            // Justo bajo la hierba: tiene que quedar techo solido.
            for (int y = sup - 4; y <= sup; ++y)
                CHECK_FALSE(caves.IsCave((float)x, y, (float)z, sup));

            // El fondo del mundo no se vacia.
            CHECK_FALSE(caves.IsCave((float)x, 3, (float)z, sup));
        }
    }
}
