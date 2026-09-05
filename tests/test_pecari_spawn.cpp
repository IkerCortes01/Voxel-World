#include <doctest/doctest.h>
#include "fauna/PecariSpawn.h"
#include <vector>
#include <utility>
#include <cmath>

// ============================================================================
// TESTS DE APARICION DEL PECARI DE COLLAR
// ============================================================================
// Lo que se verifica aqui NO es "el codigo compila": es que la generacion
// cumple las propiedades que la hacen correcta.
//
//   1. DETERMINISMO   - misma seed y posicion dan siempre lo mismo, en
//                       cualquier orden y desde cualquier hilo. Sin esto,
//                       dos chunks vecinos generarian manadas distintas en
//                       la frontera.
//   2. BIOMA          - no aparecen en el oceano ni en la nieve.
//   3. TAMANO         - los grupos respetan los rangos MEDIDOS por bioma.
//   4. SEPARACION     - dos manadas no quedan pegadas.
//   5. DENSIDAD       - la desviacion respecto al dato real esta acotada
//                       y es la declarada, no una deriva accidental.
//   6. COHESION       - los miembros salen juntos, como exige Byers y
//                       Bekoff 1981.
// ============================================================================

using namespace Fauna;
using namespace TerrainGen;

TEST_CASE("Pecari: la generacion es determinista") {
    PecariSpawn spawn(12345);

    // La misma consulta repetida mil veces debe dar exactamente lo mismo.
    // Es la propiedad de la que dependen los chunks: si falla, aparecen
    // manadas cortadas o duplicadas en las fronteras.
    for (int i = 0; i < 1000; ++i) {
        ManadaSpawn a = spawn.ConsultarManada(100, 200, BIOME_FOREST, 50.0f, 0.1f);
        ManadaSpawn b = spawn.ConsultarManada(100, 200, BIOME_FOREST, 50.0f, 0.1f);
        CHECK(a.existe   == b.existe);
        CHECK(a.miembros == b.miembros);
        CHECK(a.centroX  == b.centroX);
        CHECK(a.centroZ  == b.centroZ);
    }
}

TEST_CASE("Pecari: seeds distintas dan mundos distintos") {
    PecariSpawn a(1000);
    PecariSpawn b(2000);

    // Se recorre una franja y se cuenta en cuantas columnas discrepan.
    // Si dos seeds dieran el mismo mundo, la seed no serviria de nada.
    int diferencias = 0;
    for (int x = 0; x < 2000; x += 7) {
        for (int z = 0; z < 400; z += 11) {
            ManadaSpawn ma = a.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
            ManadaSpawn mb = b.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
            if (ma.existe != mb.existe) ++diferencias;
        }
    }
    CHECK(diferencias > 0);
}

TEST_CASE("Pecari: no aparece en biomas imposibles") {
    PecariSpawn spawn(777);

    // Se barre un area grande. En estos biomas el resultado debe ser CERO
    // manadas, no "pocas": un pecari nadando en el oceano profundo o sobre
    // un pico nevado seria un fallo visible de inmediato.
    const BiomeType prohibidos[] = {
        BIOME_OCEAN_DEEP, BIOME_OCEAN, BIOME_BEACH, BIOME_MOUNTAIN_PEAKS
    };

    for (BiomeType b : prohibidos) {
        int encontrados = 0;
        for (int x = 0; x < 4000; x += 3) {
            for (int z = 0; z < 4000; z += 3) {
                if (spawn.ConsultarManada(x, z, b, 50.0f, 0.1f).existe) ++encontrados;
            }
        }
        CHECK(encontrados == 0);
    }
}

TEST_CASE("Pecari: si aparece en los biomas que habita") {
    PecariSpawn spawn(777);

    // El complemento del test anterior: comprobar que NO se ha excluido de
    // todo por error. Un filtro demasiado agresivo dejaria el mundo vacio y
    // el test de arriba pasaria igual.
    const BiomeType habitados[] = {
        BIOME_FOREST, BIOME_DESERT, BIOME_PLAINS
    };

    for (BiomeType b : habitados) {
        int encontrados = 0;
        for (int x = 0; x < 4000; x += 3) {
            for (int z = 0; z < 2000; z += 3) {
                if (spawn.ConsultarManada(x, z, b, 50.0f, 0.1f).existe) ++encontrados;
            }
        }
        CHECK(encontrados > 0);
    }
}

TEST_CASE("Pecari: el tamano de manada respeta los rangos MEDIDOS") {
    PecariSpawn spawn(999);

    // Los rangos vienen de literatura publicada:
    //   selva     6-10  (10_PECARI_BIOLOGIA)
    //   semiarido 5-15  (10_PECARI_BIOLOGIA)
    // Un grupo fuera de rango significa que el muestreo esta mal.

    SUBCASE("selva: 6-10 individuos") {
        int comprobadas = 0;
        for (int x = 0; x < 6000 && comprobadas < 40; ++x) {
            for (int z = 0; z < 1200; ++z) {
                ManadaSpawn m = spawn.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
                if (m.existe) {
                    CHECK(m.miembros >= 6);
                    CHECK(m.miembros <= 10);
                    ++comprobadas;
                }
            }
        }
        CHECK(comprobadas > 0);
    }

    SUBCASE("desierto: 5-15 individuos") {
        int comprobadas = 0;
        for (int x = 0; x < 6000 && comprobadas < 40; ++x) {
            for (int z = 0; z < 1200; ++z) {
                ManadaSpawn m = spawn.ConsultarManada(x, z, BIOME_DESERT, 50.0f, 0.1f);
                if (m.existe) {
                    CHECK(m.miembros >= 5);
                    CHECK(m.miembros <= 15);
                    ++comprobadas;
                }
            }
        }
        CHECK(comprobadas > 0);
    }
}

TEST_CASE("Pecari: las manadas guardan separacion entre si") {
    PecariSpawn spawn(4242);

    // Se recogen todos los centros de un area y se mide la distancia minima
    // entre dos cualesquiera.
    //
    // POR QUE IMPORTA: son grupos sociales estables y territoriales, con
    // solapamiento de ambitos del 20-40% (MEDIDO). Dos manadas pegadas
    // romperian esa estructura y ademas se verian como una sola masa.
    std::vector<std::pair<int,int>> centros;
    for (int x = 0; x < 2000; ++x) {
        for (int z = 0; z < 2000; ++z) {
            ManadaSpawn m = spawn.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
            if (m.existe) centros.push_back({m.centroX, m.centroZ});
        }
    }

    REQUIRE(centros.size() >= 2);

    long long minDistSq = -1;
    for (size_t i = 0; i < centros.size(); ++i) {
        for (size_t j = i + 1; j < centros.size(); ++j) {
            const long long dx = centros[i].first  - centros[j].first;
            const long long dz = centros[i].second - centros[j].second;
            const long long d2 = dx*dx + dz*dz;
            if (minDistSq < 0 || d2 < minDistSq) minDistSq = d2;
        }
    }

    // Con MANADA_CELL=96 y margen de celda/4, la separacion minima
    // garantizada entre celdas vecinas es 2*24 = 48 bloques (28.8 m).
    //
    // Ese margen sigue siendo 3x el diametro de una manada (16 bloques), que
    // es lo que hace que dos grupos vecinos se lean como grupos SEPARADOS y
    // no como una masa continua.
    const double minDist = std::sqrt((double)minDistSq);
    CHECK(minDist >= 48.0);

    // Y la comprobacion que de verdad importa: la separacion supera el
    // diametro de un grupo. Si esto falla, las manadas se estan fundiendo.
    CHECK(minDist > 2.0 * 8.0);
}

TEST_CASE("Pecari: la densidad efectiva es la declarada") {
    // Este test existe para que la compresion de escala no derive en
    // silencio. El archivo DECLARA que comprime la densidad real ~38.8x
    // lineal respecto a los 0.2 manadas/km2 medidos en Quintana Roo.
    //
    // Si alguien cambia MANADA_CELL sin pensar, esto lo detecta.
    const float densidad = PecariSpawn::DensidadEfectivaManadasPorKm2();

    // Con celda de 96 bloques a 0.60 m: 57.6 m de lado = 0.003318 km2
    // 1 / 0.003318 = ~301.4 manadas/km2
    CHECK(densidad > 290.0f);
    CHECK(densidad < 315.0f);

    // Y la compresion respecto al dato real debe coincidir con la declarada.
    const float densidadReal = PecariDatos::MANADAS_POR_KM2;   // 0.2
    const float compresionArea = densidad / densidadReal;
    const float compresionLineal = std::sqrt(compresionArea);

    // La constante declarada en el header es 19.4
    CHECK(compresionLineal == doctest::Approx(PecariSpawn::COMPRESION_LINEAL).epsilon(0.05));
}

TEST_CASE("Pecari: los miembros salen cohesionados, no dispersos") {
    PecariSpawn spawn(555);

    // MEDIDO (Byers y Bekoff 1981): "la unidad social es una manada cohesiva
    // en la que se mantienen distancias interindividuales PEQUENAS".
    //
    // Si los miembros salieran repartidos por medio mundo, no seria una
    // manada: seria ruido con el mismo nombre.
    ManadaSpawn m;
    bool encontrada = false;
    for (int x = 0; x < 2000 && !encontrada; ++x) {
        for (int z = 0; z < 500; ++z) {
            m = spawn.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
            if (m.existe) { encontrada = true; break; }
        }
    }
    REQUIRE(encontrada);

    for (int i = 0; i < m.miembros; ++i) {
        int px = 0, pz = 0;
        spawn.PosicionMiembro(m, i, px, pz);

        const float dx = (float)(px - m.centroX);
        const float dz = (float)(pz - m.centroZ);
        const float dist = std::sqrt(dx*dx + dz*dz);

        // Nadie mas lejos del centro que el radio de dispersion (+1 de
        // margen por el jitter y el redondeo a entero).
        CHECK(dist <= m.dispersionBloques + 1.5f);
    }
}

TEST_CASE("Pecari: el reparto de miembros es determinista") {
    PecariSpawn spawn(31337);

    ManadaSpawn m;
    bool encontrada = false;
    for (int x = 0; x < 2000 && !encontrada; ++x) {
        for (int z = 0; z < 500; ++z) {
            m = spawn.ConsultarManada(x, z, BIOME_DESERT, 50.0f, 0.1f);
            if (m.existe) { encontrada = true; break; }
        }
    }
    REQUIRE(encontrada);

    // Mismo indice, misma posicion, siempre.
    for (int i = 0; i < m.miembros; ++i) {
        int x1 = 0, z1 = 0, x2 = 0, z2 = 0;
        spawn.PosicionMiembro(m, i, x1, z1);
        spawn.PosicionMiembro(m, i, x2, z2);
        CHECK(x1 == x2);
        CHECK(z1 == z2);
    }
}

TEST_CASE("Pecari: el limite de altitud sigue el dato MEDIDO") {
    PecariSpawn spawn(8080);

    // MEDIDO: limite tipico ~1800 m, maximo documentado 2335 m.
    // El motor mide en bloques de 0.60 m, asi que:
    //   1800 m = 3000 bloques
    //   2335 m = 3891.67 bloques

    SUBCASE("por encima del maximo absoluto no aparece nunca") {
        int encontrados = 0;
        for (int x = 0; x < 4000; x += 3) {
            for (int z = 0; z < 2000; z += 3) {
                // 4000 bloques = 2400 m, por encima de los 2335 m
                if (spawn.ConsultarManada(x, z, BIOME_MOUNTAINS, 4000.0f, 0.1f).existe) {
                    ++encontrados;
                }
            }
        }
        CHECK(encontrados == 0);
    }

    SUBCASE("a baja altitud aparece mas que en la franja de decaimiento") {
        // Se comparan dos alturas en el MISMO bioma: baja (500 bloques =
        // 300 m) y en plena franja de decaimiento (3500 bloques = 2100 m).
        int bajos = 0, altos = 0;
        for (int x = 0; x < 6000; x += 3) {
            for (int z = 0; z < 2000; z += 3) {
                if (spawn.ConsultarManada(x, z, BIOME_MOUNTAINS, 500.0f,  0.1f).existe) ++bajos;
                if (spawn.ConsultarManada(x, z, BIOME_MOUNTAINS, 3500.0f, 0.1f).existe) ++altos;
            }
        }
        CHECK(bajos > altos);
    }
}

TEST_CASE("Pecari: la pendiente fuerte lo excluye") {
    PecariSpawn spawn(6161);

    // Un ungulado de patas cortas no vive en una pared vertical.
    int enPared = 0;
    for (int x = 0; x < 4000; x += 3) {
        for (int z = 0; z < 2000; z += 3) {
            if (spawn.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.9f).existe) ++enPared;
        }
    }
    CHECK(enPared == 0);
}

TEST_CASE("Pecari: funciona con coordenadas negativas") {
    PecariSpawn spawn(2024);

    // El bug clasico: en C++ `-1 / 192` da 0, no -1, asi que sin el floor
    // correcto la celda 0 se duplicaria al cruzar el origen y habria una
    // franja anomala de manadas en x=-191..0.
    //
    // Se comprueba que hay manadas en territorio negativo y que la
    // separacion se mantiene tambien alli.
    std::vector<std::pair<int,int>> centros;
    for (int x = -2000; x < 0; ++x) {
        for (int z = -2000; z < 0; ++z) {
            ManadaSpawn m = spawn.ConsultarManada(x, z, BIOME_FOREST, 50.0f, 0.1f);
            if (m.existe) centros.push_back({m.centroX, m.centroZ});
        }
    }
    REQUIRE(centros.size() >= 2);

    long long minDistSq = -1;
    for (size_t i = 0; i < centros.size(); ++i) {
        for (size_t j = i + 1; j < centros.size(); ++j) {
            const long long dx = centros[i].first  - centros[j].first;
            const long long dz = centros[i].second - centros[j].second;
            const long long d2 = dx*dx + dz*dz;
            if (minDistSq < 0 || d2 < minDistSq) minDistSq = d2;
        }
    }
    CHECK(std::sqrt((double)minDistSq) >= 48.0);
}

TEST_CASE("Pecari: la idoneidad de bioma es coherente con la literatura") {
    // La selva es el optimo (donde se midio la densidad de referencia) y el
    // desierto casi igual (donde el nopal le resuelve comida Y sed).
    // Que el desierto quede alto no es un descuido: es el dato.
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_FOREST) >
          PecariSpawn::IdoneidadBioma(BIOME_PLAINS));
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_DESERT) >
          PecariSpawn::IdoneidadBioma(BIOME_PLAINS));
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_PLAINS) >
          PecariSpawn::IdoneidadBioma(BIOME_MOUNTAINS));

    // El desierto debe estar muy cerca de la selva, no ser marginal.
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_DESERT) >= 0.9f);

    // Y los excluidos, exactamente cero.
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_OCEAN)          == 0.0f);
    CHECK(PecariSpawn::IdoneidadBioma(BIOME_MOUNTAIN_PEAKS) == 0.0f);
}
