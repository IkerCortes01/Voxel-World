#include <doctest/doctest.h>
#include "render/TinteBioma.h"

#include <cmath>
#include <initializer_list>

using namespace Render;
using namespace TerrainGen;

// ============================================================================
// EL PASTO TOMA EL COLOR DE SU BIOMA
// ============================================================================
// Una sola textura de pasto para todo el mundo hace que un prado de altiplano,
// un pinar de montana y un matorral de desierto se vean EXACTAMENTE igual: es
// lo que delata que el bioma es una etiqueta y no un sitio.
//
// La textura aporta la FORMA; el color lo pone el bioma al dibujar. Es el
// mismo principio que usa Minecraft, pero con colores del altiplano mexicano,
// que es donde transcurre este juego.
//
// Estos tests fijan las propiedades que de verdad importan -- que los biomas
// se distingan, que el gradiente vaya en la direccion correcta y que nadie
// pueda anadir un bioma dejandose el color.

namespace {
// Luminancia percibida, para hablar de "mas claro" y "mas oscuro".
float luma(const TinteRGB& t) {
    return t.r * 0.30f + t.g * 0.59f + t.b * 0.11f;
}
// Saturacion aproximada (max - min), que es lo que mide "cuanto color tiene".
float sat(const TinteRGB& t) {
    const float mx = std::fmax(t.r, std::fmax(t.g, t.b));
    const float mn = std::fmin(t.r, std::fmin(t.g, t.b));
    return mx - mn;
}
} // namespace

// ----------------------------------------------------------------------------
// LA TABLA ESTA COMPLETA
// ----------------------------------------------------------------------------

TEST_CASE("Tinte: hay una fila por cada bioma del enum") {
    // ⭐ LA DEFENSA CONTRA EL DESCUIDO MAS PROBABLE.
    //
    // La tabla se declara como TABLA[BIOME_COUNT] con lista de
    // inicializadores. Si alguien anade un bioma al enum y olvida su fila, C++
    // NO da error: rellena con ceros, y ese bioma saldria con pasto NEGRO.
    //
    // Un fallo silencioso es justo lo que aqui no se quiere.
    CHECK(tablaTintesCompleta());

    // Y ninguna fila puede ser negra, por la misma razon.
    for (int i = 0; i < (int)BIOME_COUNT; ++i) {
        const TinteRGB t = tintePasto((BiomeType)i);
        INFO("bioma ", i, " = ", GetBiomeName((BiomeType)i));
        // Se precalcula: doctest no sabe descomponer una expresion con && y
        // aborta la compilacion ("Expression Too Complex").
        const bool esNegro = (t.r == 0.0f && t.g == 0.0f && t.b == 0.0f);
        CHECK(esNegro == false);
    }
}

TEST_CASE("Tinte: todos los valores estan en rango") {
    // Un canal por encima de 1 se satura a blanco en el pipeline fijo y
    // produce destellos; por debajo de 0, negro.
    for (int i = 0; i < (int)BIOME_COUNT; ++i) {
        const TinteRGB t = tintePasto((BiomeType)i);
        CHECK(t.r >= 0.0f); CHECK(t.r <= 1.0f);
        CHECK(t.g >= 0.0f); CHECK(t.g <= 1.0f);
        CHECK(t.b >= 0.0f); CHECK(t.b <= 1.0f);
    }
}

TEST_CASE("Tinte: un bioma invalido no tine, en vez de romper") {
    // Defensa: mejor el comportamiento de antes (pasto sin tenir) que un
    // acceso fuera de rango o un color absurdo.
    const TinteRGB t = tintePasto((BiomeType)999);
    CHECK(t.r == doctest::Approx(1.0f));
    CHECK(t.g == doctest::Approx(1.0f));
    CHECK(t.b == doctest::Approx(1.0f));
}

// ----------------------------------------------------------------------------
// LOS BIOMAS SE DISTINGUEN
// ----------------------------------------------------------------------------

TEST_CASE("Tinte: los biomas con pasto NO comparten color") {
    // Si dos biomas dieran el mismo color, el jugador no podria distinguirlos
    // -- que es exactamente el problema que esto viene a resolver.
    const BiomeType CON_PASTO[] = {
        BIOME_BEACH, BIOME_PLAINS, BIOME_FOREST, BIOME_DESERT,
        BIOME_MOUNTAINS, BIOME_MOUNTAIN_PEAKS,
        BIOME_OCOTAL_BLANCO, BIOME_OCOTAL_CHINO, BIOME_OCOTAL_MIXTO
    };
    const int N = (int)(sizeof(CON_PASTO) / sizeof(CON_PASTO[0]));

    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            const TinteRGB a = tintePasto(CON_PASTO[i]);
            const TinteRGB b = tintePasto(CON_PASTO[j]);
            const float d = std::fabs(a.r - b.r) + std::fabs(a.g - b.g) +
                            std::fabs(a.b - b.b);
            INFO(GetBiomeName(CON_PASTO[i]), " vs ", GetBiomeName(CON_PASTO[j]),
                 "  distancia = ", d);
            CHECK(d > 0.02f);   // se distinguen a simple vista
        }
    }
}

TEST_CASE("Tinte: los oceanos NO se tinen") {
    // Bajo el agua no hay pasto que tenir. Dejarlos en 1,1,1 es lo que
    // garantiza que el fondo marino salga exactamente como antes.
    for (BiomeType b : { BIOME_OCEAN_DEEP, BIOME_OCEAN }) {
        const TinteRGB t = tintePasto(b);
        CHECK(t.r == doctest::Approx(1.0f));
        CHECK(t.g == doctest::Approx(1.0f));
        CHECK(t.b == doctest::Approx(1.0f));
    }
}

// ----------------------------------------------------------------------------
// EL GRADIENTE VA EN LA DIRECCION CORRECTA
// ----------------------------------------------------------------------------
// Son las dos reglas que gobiernan el color de la vegetacion en el mundo real,
// y las mismas dos que codifica el colormap de Minecraft:
//
//     SEQUEDAD -> AMARILLO
//     FRIO     -> GRIS AZULADO DESATURADO

TEST_CASE("Tinte: la SEQUEDAD tira a amarillo") {
    // El desierto (matorral xerofilo chihuahuense) contra el bosque de encino.
    //
    // En el desierto lo que se ve entre arbusto y arbusto es SUELO calcareo y
    // pasto curado; en el encinar hay herbaceas verdes de verdad. El amarillo
    // se mide como "el rojo sube respecto al verde".
    const TinteRGB desierto = tintePasto(BIOME_DESERT);
    const TinteRGB bosque   = tintePasto(BIOME_FOREST);

    INFO("desierto R/G = ", desierto.r / desierto.g,
         "   bosque R/G = ", bosque.r / bosque.g);
    CHECK(desierto.r / desierto.g > bosque.r / bosque.g);

    // Y el desierto es mas CLARO: el suelo desnudo refleja mas que la hoja.
    CHECK(luma(desierto) > luma(bosque));
}

TEST_CASE("Tinte: el FRIO desatura hacia el gris azulado") {
    // El zacatonal alpino de los picos (Festuca y Calamagrostis tolucensis)
    // tiene "aspecto xerofitico reflejado por la vaina de hojas secas": es
    // sequia por congelacion. Verde grisaceo y apagado.
    const TinteRGB picos  = tintePasto(BIOME_MOUNTAIN_PEAKS);
    const TinteRGB bosque = tintePasto(BIOME_FOREST);

    INFO("saturacion picos = ", sat(picos),
         "   bosque = ", sat(bosque));
    CHECK(sat(picos) < sat(bosque));

    // Y el azul sube: es lo que distingue el frio del simple "apagado".
    CHECK(picos.b > bosque.b);
}

TEST_CASE("Tinte: el bosque de encino es el verde mas vivo de tierra firme") {
    // Copa semiabierta, suelo mesico y nitrogeno de hojarasca: es el bioma
    // terrestre mas exuberante de este mundo. Si otro lo superara, el
    // gradiente estaria mal montado.
    const TinteRGB bosque = tintePasto(BIOME_FOREST);

    const BiomeType OTROS[] = {
        BIOME_BEACH, BIOME_PLAINS, BIOME_DESERT, BIOME_MOUNTAINS,
        BIOME_MOUNTAIN_PEAKS, BIOME_OCOTAL_BLANCO, BIOME_OCOTAL_CHINO,
        BIOME_OCOTAL_MIXTO
    };
    for (BiomeType b : OTROS) {
        const TinteRGB t = tintePasto(b);
        INFO("bosque G-R = ", bosque.g - bosque.r,
             "   ", GetBiomeName(b), " G-R = ", t.g - t.r);
        // "Mas verde" = el verde destaca mas sobre el rojo.
        CHECK(bosque.g - bosque.r >= t.g - t.r);
    }
}

TEST_CASE("Tinte: el altiplano es pajizo, no pradera de regadio") {
    // ⭐ LA DECISION QUE SEPARA ESTE MUNDO DE MINECRAFT.
    //
    // El pastizal de altiplano (navajita, Bouteloua gracilis) esta verde unos
    // cuatro meses al ano por la estacionalidad monzonica; su estado de reposo
    // es pajizo. Ademas Bouteloua SE CURA EN PIE, asi que arrastra paja muerta
    // incluso mientras crece.
    //
    // El Plains de Minecraft (#91BD59) ronda el 54% de saturacion y se lee
    // como prado ingles de regadio. Aqui tiene que ser mas apagado.
    const TinteRGB plan = tintePasto(BIOME_PLAINS);

    // Referencia: el #91BD59 de Minecraft.
    const TinteRGB mcPlains{ 0.569f, 0.741f, 0.349f };

    INFO("saturacion altiplano = ", sat(plan),
         "   Minecraft Plains = ", sat(mcPlains));
    CHECK(sat(plan) < sat(mcPlains));
}

TEST_CASE("Tinte: los tres ocotales se ordenan por humedad") {
    // Los tres pinares se separan por la lluvia que reciben, y el color lo
    // refleja:
    //   BLANCO (Pinus montezumae) -- el mas humedo, 800-1000+ mm -> mas verde
    //   MIXTO                     -- intermedio, mosaico pino-encino
    //   CHINO  (Pinus leiophylla) -- cota media y seco, dosel abierto y
    //                                zacate curado -> el mas oliva
    const TinteRGB blanco = tintePasto(BIOME_OCOTAL_BLANCO);
    const TinteRGB mixto  = tintePasto(BIOME_OCOTAL_MIXTO);
    const TinteRGB chino  = tintePasto(BIOME_OCOTAL_CHINO);

    // "Mas verde" = el verde destaca mas sobre el rojo.
    const float vBlanco = blanco.g - blanco.r;
    const float vMixto  = mixto.g  - mixto.r;
    const float vChino  = chino.g  - chino.r;

    INFO("verdor blanco=", vBlanco, " mixto=", vMixto, " chino=", vChino);
    CHECK(vBlanco > vMixto);
    CHECK(vMixto  > vChino);
}

TEST_CASE("Tinte: el ocotal mixto queda ENTRE los dos puros") {
    // Es un mosaico de los otros dos, asi que su color no puede salirse del
    // rango que ellos marcan: seria un bioma con color propio inventado.
    const TinteRGB blanco = tintePasto(BIOME_OCOTAL_BLANCO);
    const TinteRGB mixto  = tintePasto(BIOME_OCOTAL_MIXTO);
    const TinteRGB chino  = tintePasto(BIOME_OCOTAL_CHINO);

    const float lo = std::fmin(luma(blanco), luma(chino));
    const float hi = std::fmax(luma(blanco), luma(chino));
    INFO("luma mixto = ", luma(mixto), " entre ", lo, " y ", hi);
    CHECK(luma(mixto) >= lo);
    CHECK(luma(mixto) <= hi);
}

// ----------------------------------------------------------------------------
// QUE SE TINE Y QUE NO
// ----------------------------------------------------------------------------

TEST_CASE("Tinte: la cara de ABAJO del pasto es tierra y NO se tine") {
    // ⭐ EL ERROR QUE ESTO EVITA.
    //
    // Un bloque de pasto tiene tierra debajo. Tenirla de verde la pondria del
    // color de la hierba, que es justo el fallo que el tinte viene a arreglar
    // -- solo que al reves.
    CHECK(caraSeTine(BLOCK_GRASS, 0) == true);    // arriba: hierba
    CHECK(caraSeTine(BLOCK_GRASS, 1) == false);   // abajo: TIERRA
    for (int cara = 2; cara < 6; ++cara)
        CHECK(caraSeTine(BLOCK_GRASS, cara) == true);   // lados: hierba
}

TEST_CASE("Tinte: la hierba alta se tine entera") {
    // Es un sprite en cruz y es toda ella herbacea.
    for (int cara = 0; cara < 6; ++cara)
        CHECK(caraSeTine(BLOCK_TALLGRASS, cara) == true);
}

TEST_CASE("Tinte: lo que no es hierba NO se tine") {
    // Tenir la piedra, la arena o un tronco los pondria del color del pasto.
    // Es la regla mas importante de toda la lista.
    const BlockType NO_HIERBA[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_WOOD, BLOCK_LEAVES,
        BLOCK_WATER, BLOCK_SNOW, BLOCK_GRAVEL, BLOCK_COBBLESTONE
    };
    for (BlockType b : NO_HIERBA)
        for (int cara = 0; cara < 6; ++cara)
            CHECK(caraSeTine(b, cara) == false);
}

// ----------------------------------------------------------------------------
// LA MEZCLA DE FRONTERA
// ----------------------------------------------------------------------------

TEST_CASE("Mezcla: un solo bioma devuelve su color exacto") {
    // En el interior de un bioma el promedio no puede desviar el color: si lo
    // hiciera, ningun bioma se veria del color que declara su tabla.
    const TinteRGB uno = tintePasto(BIOME_DESERT);
    TinteRGB iguales[9];
    for (int i = 0; i < 9; ++i) iguales[i] = uno;

    const TinteRGB m = mezclarTintes(iguales, 9);
    CHECK(m.r == doctest::Approx(uno.r));
    CHECK(m.g == doctest::Approx(uno.g));
    CHECK(m.b == doctest::Approx(uno.b));
}

TEST_CASE("Mezcla: en la frontera el color queda ENTRE los dos biomas") {
    // ⭐ ES LO QUE EVITA EL RECORTE DE TIJERA.
    //
    // Sin mezclar, el color cambia en la linea exacta donde cambia el bioma y
    // se ve como si alguien hubiera recortado el prado con unas tijeras.
    const TinteRGB des = tintePasto(BIOME_DESERT);
    const TinteRGB bos = tintePasto(BIOME_FOREST);

    TinteRGB frontera[2] = { des, bos };
    const TinteRGB m = mezclarTintes(frontera, 2);

    CHECK(m.r > std::fmin(des.r, bos.r));
    CHECK(m.r < std::fmax(des.r, bos.r));
    CHECK(m.g > std::fmin(des.g, bos.g));
    CHECK(m.g < std::fmax(des.g, bos.g));
}

TEST_CASE("Mezcla: el degradado es progresivo, no un salto") {
    // Al cruzar la frontera, la proporcion de vecinos de cada bioma cambia
    // poco a poco. El color tiene que seguir esa progresion de forma
    // monotona: si diera un salto, se veria una banda.
    const TinteRGB des = tintePasto(BIOME_DESERT);
    const TinteRGB bos = tintePasto(BIOME_FOREST);

    float anterior = -1.0f;
    for (int nDesierto = 0; nDesierto <= 9; ++nDesierto) {
        TinteRGB v[9];
        for (int i = 0; i < 9; ++i) v[i] = (i < nDesierto) ? des : bos;

        const TinteRGB m = mezclarTintes(v, 9);
        // El desierto es mas claro, asi que la luma sube de forma monotona.
        const float l = luma(m);
        if (anterior >= 0.0f) CHECK(l >= anterior - 1e-5f);
        anterior = l;
    }
}

TEST_CASE("Mezcla: cero vecinos no rompe nada") {
    // Defensa: devolver "sin tenir" es el comportamiento anterior, que es lo
    // correcto ante una entrada absurda.
    const TinteRGB m = mezclarTintes(nullptr, 0);
    CHECK(m.r == doctest::Approx(1.0f));
    CHECK(m.g == doctest::Approx(1.0f));
    CHECK(m.b == doctest::Approx(1.0f));
}
