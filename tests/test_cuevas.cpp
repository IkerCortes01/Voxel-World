#include <doctest/doctest.h>
#include "terrain/CaveGenerator.h"
#include <cstdio>
#include <vector>

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

// ============================================================================
// NIVELES DE CUEVA (pisos)
// ============================================================================
// Antes la densidad solo dependia de la profundidad: el subsuelo era una nube
// de huecos homogenea y cavar en cualquier sitio daba lo mismo. Ahora la red
// se organiza en PISOS horizontales separados por bancos de roca maciza, que
// es como se estructura un sistema karstico real (varios niveles heredados de
// antiguas posiciones del nivel freatico).
//
// Lo que se comprueba NO es un numero concreto de pisos -- eso depende de la
// semilla y de la ondulacion -- sino la PROPIEDAD: que la densidad varie con
// la altura formando maximos y minimos, en vez de ser plana.

static double huecoEnAltura(int seed, int y, int surfaceHeight) {
    TerrainGen::CaveGenerator caves(seed);
    long total = 0, huecos = 0;
    for (int x = 0; x < 140; ++x)
        for (int z = 0; z < 140; ++z) {
            ++total;
            if (caves.IsCave((float)x, y, (float)z, surfaceHeight)) ++huecos;
        }
    return (double)huecos / (double)total;
}

TEST_CASE("Cuevas: la densidad forma pisos, no una nube uniforme") {
    const int SUP = 70;

    // Se muestrea el subsuelo cada 2 bloques y se buscan maximos y minimos
    // locales. Una distribucion en pisos tiene varios; una nube plana, casi
    // ninguno.
    std::vector<double> perfil;
    for (int y = 60; y >= 10; y -= 2) perfil.push_back(huecoEnAltura(1234, y, SUP));

    REQUIRE(perfil.size() > 10);

    int maximos = 0, minimos = 0;
    for (size_t i = 1; i + 1 < perfil.size(); ++i) {
        if (perfil[i] > perfil[i-1] && perfil[i] > perfil[i+1]) ++maximos;
        if (perfil[i] < perfil[i-1] && perfil[i] < perfil[i+1]) ++minimos;
    }

    INFO("maximos locales ", maximos, " minimos locales ", minimos);
    // Al menos un piso y un banco de roca entre pisos.
    CHECK(maximos >= 1);
    CHECK(minimos >= 1);

    // Y el contraste tiene que ser real: el piso mas hueco al menos duplica
    // al banco mas macizo. Si no, los "pisos" serian una ondulacion
    // imperceptible.
    double mx = 0.0, mn = 1.0;
    for (double v : perfil) { if (v > mx) mx = v; if (v < mn) mn = v; }
    INFO("piso mas hueco ", mx, " banco mas macizo ", mn);
    CHECK(mx > mn * 2.0);
}

TEST_CASE("Cuevas: los pisos siguen comunicados entre si") {
    // ⚠️ EL RIESGO DE LOS NIVELES: si el factor de piso llegara a CERO entre
    // bandas, cada nivel quedaria estanco y no habria forma de bajar de uno a
    // otro -- peor que no tener niveles.
    //
    // Por eso el refuerzo tiene un suelo de 0.35 y nunca se anula. Se
    // comprueba que NINGUNA altura del subsuelo se queda sin hueco.
    const int SUP = 70;
    for (int y = 56; y >= 12; y -= 4) {
        const double h = huecoEnAltura(1234, y, SUP);
        INFO("altura ", y, " hueco ", h);
        CHECK(h > 0.01);   // siempre queda paso
    }
}

// ============================================================================
// ENTRADAS NATURALES EN SUPERFICIE
// ============================================================================

static bool columnaTieneBoca(const TerrainGen::CaveGenerator& c,
                             int x, int z, int surfaceHeight) {
    for (int y = surfaceHeight - 1; y >= surfaceHeight - 22; --y)
        if (c.IsCaveEntrance((float)x, y, (float)z, surfaceHeight)) return true;
    return false;
}

TEST_CASE("Cuevas: hay bocas naturales, y se encuentran paseando") {
    TerrainGen::CaveGenerator caves(1234);
    const int SUP = 70;

    long conBoca = 0, total = 0;
    for (int x = 0; x < 260; ++x)
        for (int z = 0; z < 260; ++z) {
            ++total;
            if (columnaTieneBoca(caves, x, z, SUP)) ++conBoca;
        }

    const double pct = 100.0 * conBoca / total;
    INFO("columnas con boca: ", pct, " %");

    // ⭐ ESTE TEST NACIO DE UN BUG REAL.
    //
    // El umbral se calibro para un ruido 3D y luego el campo paso a ser 2D,
    // cuyo maximo real es ~0.42: el umbral de 0.52 era INALCANZABLE y el
    // mundo se quedo con CERO bocas. No fallaba nada, no habia error en el
    // log -- simplemente no se podia entrar a ninguna cueva.
    //
    // De ahi la cota inferior: la propiedad que importa es que EXISTAN.
    CHECK(conBoca > 0);
    CHECK(pct > 1.0);
    // Y que no sea un colador: el terreno tiene que seguir siendo terreno.
    CHECK(pct < 25.0);
}

TEST_CASE("Cuevas: la boca tiene forma de embudo") {
    TerrainGen::CaveGenerator caves(1234);
    const int SUP = 70;

    // Una sima real es ancha arriba (donde el techo se desplomo) y estrecha
    // abajo (donde engancha con la galeria). Se mide cuantas columnas siguen
    // abiertas a cada profundidad: tiene que ir bajando.
    auto abiertasA = [&](int prof) {
        long ab = 0;
        for (int x = 0; x < 260; ++x)
            for (int z = 0; z < 260; ++z)
                if (caves.IsCaveEntrance((float)x, SUP - 1 - prof,
                                         (float)z, SUP)) ++ab;
        return ab;
    };

    const long arriba = abiertasA(0);
    const long medio  = abiertasA(10);
    const long fondo  = abiertasA(20);

    INFO("arriba ", arriba, " medio ", medio, " fondo ", fondo);
    CHECK(arriba > 0);
    CHECK(medio  < arriba);   // se estrecha
    CHECK(fondo  < medio);    // y sigue estrechandose
    // Pero no se cierra del todo: si el cuello llegara a cero, la boca seria
    // un hoyo sin salida que nunca alcanza la galeria.
    CHECK(fondo > 0);
}

TEST_CASE("Cuevas: las bocas son deterministas") {
    // Misma semilla y posicion, misma boca: si no, el mundo cambiaria segun
    // el orden en que se exploren los chunks.
    TerrainGen::CaveGenerator a(777), b(777);
    const int SUP = 70;
    for (int x = 0; x < 60; ++x)
        for (int z = 0; z < 60; ++z) {
            INFO("(", x, ",", z, ")");
            CHECK(columnaTieneBoca(a, x, z, SUP) ==
                  columnaTieneBoca(b, x, z, SUP));
        }
}

// ============================================================================
// SIN LAVA GENERADA
// ============================================================================

TEST_CASE("Cuevas: el generador ya no inunda el fondo de lava") {
    // La lava del fondo se retiro: dejaba la parte baja intransitable y
    // convertia el descenso en una carrera de obstaculos.
    //
    // Lo que se comprueba aqui es la CONSECUENCIA observable: que a la altura
    // donde antes habia lagos, el generador siga abriendo cueva (aire) con
    // normalidad. La ausencia del bloque LAVA se verifica en el generador
    // completo, no aqui, porque IsCave solo dice "hay hueco" -- no coloca
    // bloques.
    TerrainGen::CaveGenerator caves(1234);
    const int SUP = 70;

    long huecos = 0, total = 0;
    for (int y = 7; y <= TerrainGen::CaveGenerator::LAVA_LEVEL; ++y)
        for (int x = 0; x < 100; ++x)
            for (int z = 0; z < 100; ++z) {
                ++total;
                if (caves.IsCave((float)x, y, (float)z, SUP)) ++huecos;
            }

    const double pct = 100.0 * huecos / total;
    INFO("hueco bajo el antiguo nivel de lava: ", pct, " %");
    // Sigue habiendo galeria ahi abajo -- ahora transitable.
    CHECK(huecos > 0);
    CHECK(pct > 5.0);
}

// ============================================================================
// LAS BOCAS SE ABREN DE VERDAD Y LLEVAN A ALGUNA PARTE
// ============================================================================
// Dos fallos distintos que se veian igual desde fuera -- "las cuevas no se
// encuentran" -- y que estaban medidos:
//
//   1. LA BOCA QUEDABA TAPADA. El rango empezaba en surfaceHeight-1, asi que
//      la columna de superficie sobrevivia: sobre cada pozo quedaba una tapa
//      de pasto o de arena. El agujero estaba, pero cubierto.
//
//   2. EL POZO NO LLEGABA. El embudo estrangulaba el 93.7% de los pozos a los
//      10.9 bloques de media, y las galerias estan entre 10 y 20 de hondura.
//      Solo el 27.3% de las bocas tocaba la cueva; el resto moria en roca con
//      7.47 bloques de piedra de por medio.
//
// Estos tests fijan las dos propiedades. Son estadisticos a proposito: lo que
// importa no es una columna concreta sino la PROPORCION, que es lo que decide
// si el jugador encuentra cuevas paseando.

TEST_CASE("Bocas: se abren EN la superficie, sin techo encima") {
    // Si una sola boca del muestreo no llega hasta arriba, es que la columna
    // de superficie vuelve a sobrevivir y las tapa.
    TerrainGen::CaveGenerator cg(12345);
    const int SUP = 70;
    const int H = TerrainGen::CaveGenerator::ENTRADA_MAX_HONDURA;

    int conBoca = 0, abiertas = 0;
    for (int x = 0; x < 200; ++x)
    for (int z = 0; z < 200; ++z) {
        bool boca = false, arriba = false;
        for (int y = SUP; y >= SUP - H; --y) {
            if (!cg.IsCaveEntrance((float)x, y, (float)z, SUP)) continue;
            boca = true;
            if (y == SUP) arriba = true;
        }
        if (!boca) continue;
        ++conBoca;
        if (arriba) ++abiertas;
    }

    REQUIRE(conBoca > 0);
    INFO("bocas=", conBoca, " abiertas arriba=", abiertas);
    // TODA boca tiene que estar abierta en su cota de superficie.
    CHECK(abiertas == conBoca);
}

TEST_CASE("Bocas: la mayoria conecta con la galeria") {
    // La propiedad que de verdad importa: que el pozo lleve a alguna parte.
    // Antes del arreglo esto daba 27.3%.
    TerrainGen::CaveGenerator cg(12345);
    const int SUP = 70;
    const int H = TerrainGen::CaveGenerator::ENTRADA_MAX_HONDURA;

    int conBoca = 0, conectan = 0;
    for (int x = 0; x < 200; ++x)
    for (int z = 0; z < 200; ++z) {
        int masHondo = 9999;
        for (int y = SUP; y >= SUP - H; --y)
            if (cg.IsCaveEntrance((float)x, y, (float)z, SUP))
                if (y < masHondo) masHondo = y;
        if (masHondo == 9999) continue;
        ++conBoca;

        int techo = -1;
        for (int y = SUP - 6; y >= 6; --y)
            if (cg.IsCave((float)x, y, (float)z, SUP)) { techo = y; break; }
        if (techo < 0) continue;

        if (masHondo - techo - 1 <= 0) ++conectan;
    }

    REQUIRE(conBoca > 0);
    const double frac = (double)conectan / conBoca;
    INFO("bocas=", conBoca, " conectan=", conectan,
         " fraccion=", frac);

    // Medido: 81.6%. Se exige holgadamente por encima de la mitad, que es lo
    // que separa "la mayoria lleva a la cueva" de "la mayoria es un hoyo".
    CHECK(frac > 0.6);
}

TEST_CASE("Bocas: siguen siendo minoria del mundo") {
    // El reverso, y es igual de importante: abrir mas cuevas no puede
    // convertir el mapa en un colador. Si esto se disparara, el terreno
    // quedaria agujereado y las bocas dejarian de ser un hallazgo.
    TerrainGen::CaveGenerator cg(4242);
    const int SUP = 70;
    const int H = TerrainGen::CaveGenerator::ENTRADA_MAX_HONDURA;

    int columnas = 0, conBoca = 0;
    for (int x = 0; x < 200; ++x)
    for (int z = 0; z < 200; ++z) {
        ++columnas;
        for (int y = SUP; y >= SUP - H; --y)
            if (cg.IsCaveEntrance((float)x, y, (float)z, SUP)) { ++conBoca; break; }
    }

    const double frac = (double)conBoca / columnas;
    INFO("fraccion de columnas con boca = ", frac);
    // Medido: 7.88%. Se deja margen, pero muy lejos de agujerear el mundo.
    CHECK(frac > 0.01);
    CHECK(frac < 0.20);
}

TEST_CASE("Bocas: el pozo sigue siendo un embudo") {
    // El arreglo aflojo el cuello (0.34 -> 0.25) y alargo el tramo (22 -> 34).
    // Lo que NO puede haberse perdido es la forma: ancha arriba, estrecha
    // abajo. Si el pozo fuera un tubo recto, dejaria de leerse como una sima.
    TerrainGen::CaveGenerator cg(777);
    const int SUP = 70;

    int anchoArriba = 0, anchoHondo = 0;
    for (int x = 0; x < 200; ++x)
    for (int z = 0; z < 200; ++z) {
        if (cg.IsCaveEntrance((float)x, SUP, (float)z, SUP))      ++anchoArriba;
        if (cg.IsCaveEntrance((float)x, SUP - 30, (float)z, SUP)) ++anchoHondo;
    }

    INFO("columnas abiertas arriba=", anchoArriba, " a 30 de hondo=", anchoHondo);
    REQUIRE(anchoArriba > 0);
    // A 30 bloques tiene que quedar bastante menos abierto que en superficie.
    CHECK(anchoHondo < anchoArriba);
}
