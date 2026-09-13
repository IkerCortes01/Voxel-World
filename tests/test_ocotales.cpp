#include <doctest/doctest.h>
#include "terrain/BiomeTypes.h"
#include "terrain/BiomeGenerator.h"
#include "terrain/ClimateGenerator.h"
#include "terrain/DecorationSystem.h"
#include "fauna/PecariSpawn.h"
#include <algorithm>
#include <cstdio>
#include <map>
#include <string>

// ============================================================================
// LOS TRES BIOMAS DE OCOTAL
// ============================================================================
// Se comprueban cuatro cosas, y ninguna es un numero magico:
//
//   1. Que EXISTEN en el mundo de verdad, midiendo sobre el generador real.
//      Un bioma que se define pero al que nunca llega el arbol de decision es
//      exactamente el fallo que tuvieron los biomas de montana: declarados,
//      inalcanzables, y nadie se entero hasta que se vieron parches de piedra.
//
//   2. Que cada uno se ve de la especie que le da nombre.
//
//   3. Que el mixto queda SIEMPRE entre los dos puros por humedad, que es lo
//      que evita una frontera dura blanco/chino.
//
//   4. Que no se han comido el mundo: el bosque y las planicies siguen ahi.
// ============================================================================

using namespace TerrainGen;

namespace {

// Recorre una malla grande de tierra firme y cuenta biomas. Es el mismo metodo
// que uso el ajuste del desierto (documentado en BiomeGenerator.h), asi que las
// cifras son comparables con las de aquel.
struct Reparto {
    std::map<BiomeType, long long> conteo;
    long long tierraFirme = 0;

    double porcentaje(BiomeType b) const {
        if (!tierraFirme) return 0.0;
        auto it = conteo.find(b);
        if (it == conteo.end()) return 0.0;
        return 100.0 * (double)it->second / (double)tierraFirme;
    }
};

Reparto medirReparto(int seed, int lado, int paso) {
    ClimateGenerator clima(seed);
    BiomeGenerator biomas(seed);
    Reparto r;

    for (int i = 0; i < lado; ++i) {
        for (int j = 0; j < lado; ++j) {
            const float x = (float)(i * paso);
            const float z = (float)(j * paso);
            const BiomeType b = biomas.SampleBiome(clima, x, z).dominant;

            // El oceano no cuenta: lo que se reparte es la TIERRA FIRME, que es
            // donde el jugador camina y donde compiten estos biomas.
            if (IsOceanBiome(b)) continue;
            ++r.tierraFirme;
            ++r.conteo[b];
        }
    }
    return r;
}

} // namespace

TEST_CASE("Ocotales: los tres aparecen de verdad en el mundo") {
    const Reparto r = medirReparto(12345, 320, 24);

    const double blanco = r.porcentaje(BIOME_OCOTAL_BLANCO);
    const double chino  = r.porcentaje(BIOME_OCOTAL_CHINO);
    const double mixto  = r.porcentaje(BIOME_OCOTAL_MIXTO);

    printf("[OCOTAL] blanco=%.1f%% chino=%.1f%% mixto=%.1f%% (total %.1f%%)\n",
           blanco, chino, mixto, blanco + chino + mixto);
    printf("[OCOTAL] bosque=%.1f%% planicies=%.1f%% desierto=%.1f%%\n",
           r.porcentaje(BIOME_FOREST),
           r.porcentaje(BIOME_PLAINS),
           r.porcentaje(BIOME_DESERT));

    // ESTO ES LO QUE DE VERDAD IMPORTA: que ninguno sea inalcanzable.
    // Un bioma al 0% esta declarado pero muerto.
    CHECK(blanco > 0.0);
    CHECK(chino  > 0.0);
    CHECK(mixto  > 0.0);

    // Y que se encuentren sin buscarlos. Por debajo del 1% cada uno, el jugador
    // podria jugar horas sin pisar un ocotal.
    CHECK(blanco > 1.0);
    CHECK(chino  > 1.0);
    CHECK(mixto  > 1.0);
}

TEST_CASE("Ocotales: los tres salen en proporciones parecidas") {
    // ESTE TEST EXISTE POR UN FALLO REAL.
    //
    // La primera version ponia los cortes de humedad en 0.52/0.62, razonando
    // sobre el rango teorico [0,1]. Medido, el reparto salio 3.3% / 3.2% /
    // 16.9%: el blanco se llevaba cinco veces mas mundo que los otros dos
    // juntos, porque la humedad de la franja fria tiene la mediana en 0.787 y
    // no en 0.5.
    //
    // Los tests de "aparecen de verdad" NO lo detectaron: los tres pasaban del
    // 1% y por tanto estaban "vivos". Hacia falta comprobar el EQUILIBRIO, no
    // solo la existencia.
    //
    // Es tambien el guardian de los cuartiles: si algun dia cambia
    // ClimateGenerator::GetHumidity, los cortes dejan de ser cuartiles y este
    // test es lo que avisa.
    const Reparto r = medirReparto(12345, 320, 24);

    const double blanco = r.porcentaje(BIOME_OCOTAL_BLANCO);
    const double chino  = r.porcentaje(BIOME_OCOTAL_CHINO);
    const double mixto  = r.porcentaje(BIOME_OCOTAL_MIXTO);
    const double mayor  = std::max({blanco, chino, mixto});
    const double menor  = std::min({blanco, chino, mixto});

    printf("[OCOTAL] equilibrio: blanco=%.1f%% chino=%.1f%% mixto=%.1f%% "
           "(mayor/menor = %.2f)\n", blanco, chino, mixto, mayor / menor);

    // Ninguno puede ser mas del doble que otro. No se exige reparto exacto
    // -- son regiones de un campo de ruido, no porciones de tarta -- pero una
    // diferencia mayor significa que los cortes se han desalineado de la
    // distribucion real.
    CHECK(mayor / menor < 2.0);
}

TEST_CASE("Ocotales: no se han comido el mundo") {
    const Reparto r = medirReparto(12345, 320, 24);

    // El bosque mixto y las planicies siguen siendo biomas principales. Los
    // ocotales les quitan terreno a proposito, pero no los sustituyen.
    CHECK(r.porcentaje(BIOME_FOREST) > 10.0);
    CHECK(r.porcentaje(BIOME_PLAINS) > 10.0);

    // El desierto estaba medido en ~15% y NO se ha tocado su umbral: los
    // ocotales piden temperatura BAJA y el desierto ALTA, asi que no compiten.
    // Si esta cifra se moviera mucho, algo se habria solapado sin querer.
    const double desierto = r.porcentaje(BIOME_DESERT);
    printf("[OCOTAL] desierto tras el cambio = %.1f%%\n", desierto);
    CHECK(desierto > 8.0);
    CHECK(desierto < 25.0);

    // Los tres ocotales juntos son una parte relevante del mundo, pero no la
    // mayoria: sigue siendo un mundo templado con pinares dentro.
    const double ocotal = r.porcentaje(BIOME_OCOTAL_BLANCO)
                        + r.porcentaje(BIOME_OCOTAL_CHINO)
                        + r.porcentaje(BIOME_OCOTAL_MIXTO);
    CHECK(ocotal < 50.0);
}

TEST_CASE("Ocotales: existen tambien lejos del origen") {
    // El reparto global se mide alrededor del origen. Este test comprueba que
    // no es una propiedad LOCAL: un bioma que solo saliera cerca de (0,0)
    // dejaria el resto del mundo sin pinares, y nadie se enteraria hasta
    // caminar 50.000 bloques.
    //
    // No se exige que salgan en TODA region: hay comarcas legitimamente
    // calidas donde un pinar no debe existir -- se midio que a 15.000 y 25.000
    // bloques la temperatura media sube a 0.78 y los ocotales desaparecen, que
    // es lo correcto. Lo que se exige es que aparezcan en VARIAS regiones
    // lejanas distintas.
    struct Zona { int cx, cz; };
    const Zona zonas[] = { {0,0}, {50000,50000}, {-40000,30000}, {100000,100000} };

    int regionesConOcotal = 0;
    for (const Zona& zn : zonas) {
        ClimateGenerator clima(12345);
        BiomeGenerator biomas(12345);
        bool hay = false;
        for (int i = 0; i < 90 && !hay; ++i) {
            for (int j = 0; j < 90 && !hay; ++j) {
                const float x = (float)(zn.cx + i * 14);
                const float z = (float)(zn.cz + j * 14);
                if (EsOcotal(biomas.SampleBiome(clima, x, z).dominant)) hay = true;
            }
        }
        if (hay) ++regionesConOcotal;
    }

    printf("[OCOTAL] regiones lejanas con ocotal: %d de %d\n",
           regionesConOcotal, (int)(sizeof(zonas) / sizeof(zonas[0])));

    // Al menos la mitad de las regiones probadas tienen pinares.
    CHECK(regionesConOcotal >= 2);
}

TEST_CASE("Ocotales: el mixto queda SIEMPRE entre los dos puros") {
    // Es la propiedad que hace gradual la transicion. Se comprueba sobre el
    // selector directamente, barriendo la humedad a temperatura fria.
    //
    // Si el mixto no estuviera en medio, existiria una frontera directa
    // blanco/chino y se veria un corte seco de una especie a otra.
    BiomeGenerator g(1);

    double ultimaHumedadChino = -1.0, primeraHumedadBlanco = 2.0;
    double minMixto = 2.0, maxMixto = -1.0;

    for (int i = 0; i <= 1000; ++i) {
        const float h = (float)i / 1000.0f;

        ClimateData c;
        c.continentalness = 0.80f;   // tierra adentro
        c.temperature     = 0.30f;   // frio: dentro de la franja de ocotal
        c.humidity        = h;
        c.erosion         = 0.60f;   // sin montana
        c.weirdness       = 0.50f;
        c.baseHeight      = 0.0f;

        const BiomeType b = g.SelectBiome(c);
        if (b == BIOME_OCOTAL_CHINO)  ultimaHumedadChino = h;
        if (b == BIOME_OCOTAL_MIXTO) {
            if (h < minMixto) minMixto = h;
            if (h > maxMixto) maxMixto = h;
        }
        if (b == BIOME_OCOTAL_BLANCO && h < primeraHumedadBlanco)
            primeraHumedadBlanco = h;
    }

    printf("[OCOTAL] chino hasta %.3f | mixto %.3f-%.3f | blanco desde %.3f\n",
           ultimaHumedadChino, minMixto, maxMixto, primeraHumedadBlanco);

    // Los tres tramos existen.
    REQUIRE(ultimaHumedadChino   >= 0.0);
    REQUIRE(maxMixto             >= 0.0);
    REQUIRE(primeraHumedadBlanco <= 1.0);

    // Y estan en este orden, sin solaparse: chino < mixto < blanco.
    CHECK(ultimaHumedadChino < minMixto);
    CHECK(maxMixto < primeraHumedadBlanco);
}

TEST_CASE("Ocotales: cada uno se ve de la especie que le da nombre") {
    DecorationSystem deco(4242);

    // Cuenta que especie sale en cada bioma sobre muchas columnas.
    auto reparto = [&](BiomeType bioma) {
        std::map<TreeType, int> n;
        int total = 0;
        for (int x = 0; x < 220; ++x) {
            for (int z = 0; z < 220; ++z) {
                // temperatura fria, que es la de estos biomas
                ++n[deco.GetTreeType(x * 7, z * 7, bioma, 0.30f)];
                ++total;
            }
        }
        return std::make_pair(n, total);
    };

    {
        auto [n, total] = reparto(BIOME_OCOTAL_BLANCO);
        const double pc = 100.0 * n[TREE_OCOTE] / total;
        printf("[OCOTAL] blanco: %.1f%% ocote blanco\n", pc);
        // Domina con claridad, pero NO es monoespecifico: tiene que haber
        // acompanantes o el bosque se ve como una plantacion.
        CHECK(pc > 65.0);
        CHECK(pc < 85.0);
        CHECK(n[TREE_OCOTE_CHINO] > 0);   // el otro ocote asoma
    }

    {
        auto [n, total] = reparto(BIOME_OCOTAL_CHINO);
        const double pc = 100.0 * n[TREE_OCOTE_CHINO] / total;
        printf("[OCOTAL] chino: %.1f%% ocote chino\n", pc);
        CHECK(pc > 65.0);
        CHECK(pc < 85.0);
        CHECK(n[TREE_OCOTE] > 0);
    }

    {
        auto [n, total] = reparto(BIOME_OCOTAL_MIXTO);
        const double blanco = 100.0 * n[TREE_OCOTE] / total;
        const double chino  = 100.0 * n[TREE_OCOTE_CHINO] / total;
        printf("[OCOTAL] mixto: %.1f%% blanco / %.1f%% chino\n", blanco, chino);

        // LO QUE DEFINE AL MIXTO: ninguno de los dos manda. Si uno se llevara
        // mucho mas, se veria como un ocotal puro con intrusos y este bioma no
        // tendria razon de existir.
        CHECK(blanco > 30.0);
        CHECK(chino  > 30.0);
        const double diferencia = blanco > chino ? blanco - chino : chino - blanco;
        CHECK(diferencia < 10.0);
    }
}

TEST_CASE("Ocotales: los pecaries viven en ellos") {
    // Sin esto los switch de PecariSpawn devuelven 0.0f por su rama por
    // defecto y los ocotales quedan sin un solo animal: un bosque entero
    // vacio, y sin ningun error que lo delate.
    CHECK(Fauna::PecariSpawn::IdoneidadBioma(BIOME_OCOTAL_BLANCO) > 0.0f);
    CHECK(Fauna::PecariSpawn::IdoneidadBioma(BIOME_OCOTAL_CHINO)  > 0.0f);
    CHECK(Fauna::PecariSpawn::IdoneidadBioma(BIOME_OCOTAL_MIXTO)  > 0.0f);

    // Y con manadas de tamano real, no de cero.
    for (BiomeType b : { BIOME_OCOTAL_BLANCO, BIOME_OCOTAL_CHINO, BIOME_OCOTAL_MIXTO }) {
        int lo = 0, hi = 0;
        Fauna::PecariSpawn::RangoManada(b, lo, hi);
        CHECK(lo > 0);
        CHECK(hi >= lo);
    }
}

TEST_CASE("Ocotales: la tabla de biomas los describe como pinares") {
    for (BiomeType b : { BIOME_OCOTAL_BLANCO, BIOME_OCOTAL_CHINO, BIOME_OCOTAL_MIXTO }) {
        const BiomeDefinition& d = GetBiome(b);

        // Tienen nombre propio para el HUD.
        REQUIRE(d.name != nullptr);
        CHECK(std::string(d.name).find("Ocotal") != std::string::npos);

        // Son bosque cerrado: mas arbolado que el bosque mixto.
        CHECK(d.treeDensity >= GetBiome(BIOME_FOREST).treeDensity);

        // Y con el suelo mas pelado: la acicula de pino ahoga la hierba.
        CHECK(d.grassDensity < GetBiome(BIOME_FOREST).grassDensity);

        // No son oceano ni nieve.
        CHECK_FALSE(d.isOcean);
        CHECK_FALSE(d.isSnowy);

        // Los helpers los reconocen.
        CHECK(EsOcotal(b));
        CHECK(EsBoscoso(b));
    }

    // Y no confunden al bosque normal con un ocotal.
    CHECK_FALSE(EsOcotal(BIOME_FOREST));
    CHECK(EsBoscoso(BIOME_FOREST));
}
