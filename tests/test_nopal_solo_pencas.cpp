#include <doctest/doctest.h>
#include "BlockType.h"
#include "BloqueCompuesto.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// EL NOPAL SOLO DA PENCAS (Y LAS TUNAS SIGUEN SALIENDO)
// ============================================================================
// PEDIDO: "que los nopales solo te den este item y nunca los otros, pero aun
// te de tunas".
//
// EL PROBLEMA. Un nopal no es un bloque: es una planta de varias piezas --la
// base que lo ancla al suelo (una por tipo de terreno), el tallo y los
// cladodios sueltos, dobles, triples y en diagonal--. Al TALARLO se desprende
// entero, y cada pieza pasaba por el `default` de getBlockDrops, que suelta
// EL PROPIO BLOQUE: caian "tallo en pasto", "cladodio X2" y demas andamiaje
// del generador.
//
// POR QUE NO BASTABA getDroppedItem. Hay DOS tablas de drops y el talado pasa
// por la otra (ver PENDIENTES.md, "Dos tablas de drops que no coinciden").
// getDroppedItem ya traducia todo a penca, pero getBlockDrops no, y es esa la
// que usan el minado y el derrumbe estructural.
//
// Lo que estos tests fijan:
//   1. TODA pieza de nopal da BLOCK_NOPAL_FRUTO, y solo eso: es el item del
//      icono a rayas que se pidio, y el que se moja al colocarlo junto al agua.
//   2. La cuenta es la que se ve: un cladodio doble da 2 pencas, el triple 3,
//      y una mata compuesta tantas como pencas tenga.
//   3. LA TUNA SIGUE CAYENDO. Es el fruto, no la planta.
// ============================================================================

namespace {

std::string fuente() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}

}  // namespace

TEST_CASE("Nopal: todas sus piezas existen y son distintas de la penca") {
    // El andamiaje del generador: lo que NO debe llegar al inventario.
    const BlockType PIEZAS[] = {
        BLOCK_NOPAL_BASE_PASTO, BLOCK_NOPAL_BASE_TIERRA,
        BLOCK_NOPAL_BASE_ARENA, BLOCK_NOPAL_BASE_T_ARCILLA,
        BLOCK_NOPAL_BASE_A_ARCILLA, BLOCK_NOPAL_TALLO,
        BLOCK_NOPAL_CLADODIO_X2, BLOCK_NOPAL_CLADODIO_X3,
        BLOCK_NOPAL_CLADODIO_Z2, BLOCK_NOPAL_CLADODIO_Z3,
        BLOCK_NOPAL_CLADODIO_DIAG,
    };
    for (BlockType p : PIEZAS) {
        INFO("pieza ", (int)p);
        CHECK((int)p <= (int)BLOCK_TYPE_MAX);
        // Son IDs propios: por eso podian soltarse como item suelto.
        CHECK(p != BLOCK_NOPAL_CLADODIO);
    }
}

TEST_CASE("Nopal: getBlockDrops tiene caso propio para cada pieza") {
    // ⭐ EL ARREGLO. Sin estos `case`, las piezas caen al `default` y se
    // sueltan a si mismas.
    const std::string src = fuente();
    REQUIRE_MESSAGE(!src.empty(), "no se pudo leer src/main.cpp");

    const size_t m = src.find("EL NOPAL SOLO DA PENCAS. NUNCA OTRA COSA.");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "se quito el caso del nopal: volveria a soltar tallos "
                    "y bases al talarlo");

    const std::string bloque = src.substr(m, 3200);

    // Las cinco bases y el tallo.
    CHECK(bloque.find("case BLOCK_NOPAL_BASE_PASTO:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_BASE_TIERRA:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_BASE_ARENA:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_BASE_T_ARCILLA:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_BASE_A_ARCILLA:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_TALLO:") != std::string::npos);

    // Y los cladodios en todas sus formas.
    CHECK(bloque.find("case BLOCK_NOPAL_CLADODIO_X2:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_CLADODIO_X3:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_CLADODIO_Z2:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_CLADODIO_Z3:") != std::string::npos);
    CHECK(bloque.find("case BLOCK_NOPAL_CLADODIO_DIAG:") != std::string::npos);
}

TEST_CASE("Nopal: lo unico que sueltan es la penca") {
    // Ni tallos, ni bases, ni espinas, ni baba: en todo el bloque del arreglo
    // el UNICO item que se empuja es BLOCK_NOPAL_FRUTO.
    const std::string src = fuente();
    REQUIRE(!src.empty());

    const size_t m = src.find("EL NOPAL SOLO DA PENCAS. NUNCA OTRA COSA.");
    REQUIRE(m != std::string::npos);
    const std::string bloque = src.substr(m, 3200);

    // Ningun push_back de otra cosa dentro del bloque del nopal.
    CHECK(bloque.find("drops.push_back({BLOCK_NOPAL_CLADODIO") == std::string::npos);
    CHECK(bloque.find("drops.push_back({BLOCK_NOPAL_TALLO") == std::string::npos);
    CHECK(bloque.find("drops.push_back({BLOCK_ESPINAS_NOPAL") == std::string::npos);
    CHECK(bloque.find("drops.push_back({BLOCK_NOPAL_BABA") == std::string::npos);
    CHECK(bloque.find("drops.push_back({BLOCK_NOPAL_SECO") == std::string::npos);
    CHECK(bloque.find("drops.push_back({BLOCK_NOPAL_TIRAS") == std::string::npos);
}

TEST_CASE("Nopal: las dos tablas de drops dan el mismo item") {
    // Antes salian dos items distintos segun por donde pasara cada pieza.
    const std::string src = fuente();
    REQUIRE(!src.empty());

    const size_t m = src.find("if (Compuesto::Nopal::esNopal(brokenBlock))");
    REQUIRE(m != std::string::npos);
    CHECK(src.substr(m, 120).find("return BLOCK_NOPAL_FRUTO;") != std::string::npos);
    CHECK(src.find("return BLOCK_NOPAL_CLADODIO;") == std::string::npos);
    CHECK(src.find("{BLOCK_NOPAL_CLADODIO, pencas, 1.0f}") == std::string::npos);
    CHECK(src.find("{BLOCK_NOPAL_FRUTO, pencas, 1.0f}") != std::string::npos);
}

TEST_CASE("Nopal: su item se ve como penca, no como tuna") {
    CHECK(esPencaDeNopal(BLOCK_NOPAL_FRUTO));
    CHECK_FALSE(esTuna(BLOCK_NOPAL_FRUTO));
}

TEST_CASE("Nopal: la cuenta de pencas es la que se ve en la planta") {
    const std::string src = fuente();
    REQUIRE(!src.empty());

    const size_t m = src.find("EL NOPAL SOLO DA PENCAS. NUNCA OTRA COSA.");
    REQUIRE(m != std::string::npos);
    const std::string bloque = src.substr(m, 3200);

    // Suelto y diagonal: una penca.
    CHECK(bloque.find("{BLOCK_NOPAL_FRUTO, 1, 1.0f}") != std::string::npos);
    // Dobles: dos.
    CHECK(bloque.find("{BLOCK_NOPAL_FRUTO, 2, 1.0f}") != std::string::npos);
    // Triples: tres.
    CHECK(bloque.find("{BLOCK_NOPAL_FRUTO, 3, 1.0f}") != std::string::npos);
}

TEST_CASE("Nopal: la mata compuesta suelta todas sus pencas") {
    // La mata es un COMPUESTO: su numero de pencas viaja dentro del ID. Sin
    // su caso caeria en el corte de los compuestos con el vector vacio, y el
    // drop lo decidiria getDroppedItem -- que da UNA penca mida lo que mida.
    const std::string src = fuente();
    REQUIRE(!src.empty());

    const size_t m = src.find("LA MATA DE NOPAL SI SUELTA, Y SUELTA LAS PENCAS QUE SE VEN");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "la mata volveria a soltar una sola penca");

    const std::string bloque = src.substr(m, 1400);
    CHECK(bloque.find("Compuesto::Nopal::esNopal(blockType)") != std::string::npos);
    // extrasDe son las ADICIONALES a la primera, de ahi el +1.
    CHECK(bloque.find("1 + (int)Compuesto::Nopal::extrasDe(blockType)")
          != std::string::npos);

    // ⚠️ Y va ANTES del corte de los compuestos, o no se alcanzaria nunca.
    const size_t corte = src.find("if (Compuesto::esCompuesto(blockType)) return drops;");
    REQUIRE(corte != std::string::npos);
    CHECK_MESSAGE(m < corte,
                  "el caso de la mata quedo DESPUES del corte de compuestos: "
                  "es codigo inalcanzable");
}

TEST_CASE("Nopal: LA TUNA SIGUE CAYENDO") {
    // ⭐ LO QUE SE PIDIO CONSERVAR: "pero aun te de tunas".
    //
    // La tuna es el FRUTO, no la planta. No entra en el caso del nopal: cae
    // por su propio camino y se recoge entera.
    const std::string src = fuente();
    REQUIRE(!src.empty());

    // Sigue teniendo su caso, devolviendose a si misma.
    const size_t t = src.find("La tuna se recoge entera");
    REQUIRE_MESSAGE(t != std::string::npos,
                    "desaparecio el caso de la tuna: dejaria de recogerse");

    const std::string cerca = src.substr(t, 400);
    CHECK(cerca.find("case BLOCK_TUNA:") != std::string::npos);
    CHECK(cerca.find("case BLOCK_TUNA_AMARILLA:") != std::string::npos);
    CHECK(cerca.find("case BLOCK_TUNA_ROJA:") != std::string::npos);
    CHECK(cerca.find("return brokenBlock;") != std::string::npos);

    // Y NO se la tragó el caso del nopal: una tuna no es una penca.
    const size_t m = src.find("EL NOPAL SOLO DA PENCAS. NUNCA OTRA COSA.");
    REQUIRE(m != std::string::npos);
    const std::string bloqueNopal = src.substr(m, 3200);
    CHECK_MESSAGE(bloqueNopal.find("case BLOCK_TUNA") == std::string::npos,
                  "la tuna entro en el caso del nopal: daria penca en vez "
                  "de fruta");
}

TEST_CASE("Nopal: las tunas son bloques propios, no partes de la mata") {
    // De esto depende que el caso de la mata compuesta no se las lleve por
    // delante: si una tuna fuera un compuesto de FAM_NOPAL, mi caso la
    // convertiria en pencas.
    CHECK(esTuna(BLOCK_TUNA));
    CHECK(esTuna(BLOCK_TUNA_AMARILLA));
    CHECK(esTuna(BLOCK_TUNA_ROJA));

    CHECK_FALSE(Compuesto::Nopal::esNopal(BLOCK_TUNA));
    CHECK_FALSE(Compuesto::Nopal::esNopal(BLOCK_TUNA_AMARILLA));
    CHECK_FALSE(Compuesto::Nopal::esNopal(BLOCK_TUNA_ROJA));

    // Y una tuna no es una penca.
    CHECK_FALSE(esCladodio(BLOCK_TUNA));
}

TEST_CASE("Nopal: el procesado voluntario de la penca sigue existiendo") {
    // ⚠️ LO QUE NO SE TOCA.
    //
    // Nopal seco, tiras, sin baba, baba y espinas NO desaparecen del juego:
    // son el resultado de PROCESAR una penca a proposito (lavarla, cortarla,
    // ponerla a desbabar y secar). Lo que se arreglo es que no salgan de
    // ROMPER la planta, que es distinto.
    CHECK((int)BLOCK_NOPAL_MOJADO    <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_NOPAL_TIRAS     <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_NOPAL_SIN_BABA  <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_NOPAL_BABA      <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_NOPAL_SECO      <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_ESPINAS_NOPAL   <= (int)BLOCK_TYPE_MAX);

    // La via que los produce sigue en pie: las tiras sueltan baba y espinas
    // al terminar de desbabar.
    const std::string src = fuente();
    REQUIRE(!src.empty());
    CHECK(src.find("state->spawnItem(pos, BLOCK_NOPAL_BABA);") != std::string::npos);
    CHECK(src.find("state->spawnItem(pos, BLOCK_ESPINAS_NOPAL);") != std::string::npos);
}
