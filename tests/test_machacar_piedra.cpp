#include <doctest/doctest.h>
#include "BlockType.h"
#include "Inventory.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// MACHACAR PIEDRA LABRADA CON EL MARTILLO
// ============================================================================
// Martillo + piedra labrada -> 5 pedazos de piedra.
//
// Es la via para FABRICAR pedazos en vez de depender de los que caen en el
// terreno (~4 por chunk). Importa porque el pedazo de piedra es ingrediente
// de la cadena temprana: el hilo de ixtle pide 3, y el hacha y el pico de
// piedra piden los suyos.
//
// Lo que estos tests fijan:
//
//   1. Los DOS martillos valen. El emparejado compara el tipo exacto, asi
//      que sin una receta por cada uno la herramienta MEJOR no serviria para
//      el trabajo de la peor.
//   2. Da CINCO, no otra cosa.
//   3. El martillo NO se consume: se le quita un uso. Lo resuelve
//      executeCrafting reconociendo las herramientas gastables.
//
// Se lee el propio main.cpp: el binario de tests no enlaza el motor, asi que
// CraftingSystem no esta disponible como simbolo. Mismo camino que
// test_techo_hojas.cpp y test_creativo_completo.cpp.
// ============================================================================

namespace {

std::string fuenteDelMotor() {
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

TEST_CASE("Machacar: los bloques de la receta existen y son distintos") {
    CHECK((int)BLOCK_COBBLESTONE      <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_PIEDRA    <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_MARTILLO_PIEDRA  <= (int)BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_MARTILLO_PEDERNAL <= (int)BLOCK_TYPE_MAX);

    // Lo que entra y lo que sale no pueden ser el mismo bloque, o la receta
    // seria un bucle que multiplica material de la nada.
    CHECK(BLOCK_COBBLESTONE != BLOCK_PEDAZO_PIEDRA);
    CHECK(BLOCK_MARTILLO_PIEDRA != BLOCK_MARTILLO_PEDERNAL);
}

TEST_CASE("Machacar: la receta esta registrada y da 5") {
    const std::string src = fuenteDelMotor();
    REQUIRE_MESSAGE(!src.empty(), "no se pudo leer src/main.cpp");

    const size_t m = src.find("MACHACAR PIEDRA LABRADA: 1 BLOQUE -> 5 PEDAZOS");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "no esta la receta de machacar piedra labrada");

    const std::string cuerpo = src.substr(m, 2200);

    // Cinco pedazos, y sin forma.
    CHECK_MESSAGE(
        cuerpo.find("CraftingRecipe recipe(BLOCK_PEDAZO_PIEDRA, 5, true)")
            != std::string::npos,
        "la receta debe dar 5 pedazos de piedra y ser shapeless");

    // El ingrediente es la piedra LABRADA, no la piedra en bruto.
    CHECK(cuerpo.find("recipe.pattern[1] = BLOCK_COBBLESTONE;") != std::string::npos);
    CHECK(cuerpo.find("recipe.pattern[0] = martillo;") != std::string::npos);
}

TEST_CASE("Machacar: valen los dos martillos") {
    const std::string src = fuenteDelMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("MACHACAR PIEDRA LABRADA: 1 BLOQUE -> 5 PEDAZOS");
    REQUIRE(m != std::string::npos);
    const std::string cuerpo = src.substr(m, 2200);

    // El bucle da de alta una receta por martillo.
    CHECK(cuerpo.find("BLOCK_MARTILLO_PIEDRA") != std::string::npos);
    CHECK(cuerpo.find("BLOCK_MARTILLO_PEDERNAL") != std::string::npos);
}

TEST_CASE("Machacar: no da 1 ni 8 por despiste") {
    // 5 es la cifra pedida. Este test existe para que cambiarla sea una
    // decision consciente y no el resultado de copiar otra receta.
    const std::string src = fuenteDelMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("MACHACAR PIEDRA LABRADA: 1 BLOQUE -> 5 PEDAZOS");
    REQUIRE(m != std::string::npos);
    const std::string cuerpo = src.substr(m, 2200);

    CHECK(cuerpo.find("CraftingRecipe recipe(BLOCK_PEDAZO_PIEDRA, 1,") == std::string::npos);
    CHECK(cuerpo.find("CraftingRecipe recipe(BLOCK_PEDAZO_PIEDRA, 8,") == std::string::npos);
}

TEST_CASE("Machacar: el martillo es una herramienta que se desgasta") {
    // De esto depende que el martillo NO se consuma como ingrediente.
    // `executeCrafting` decide por `esHerramientaGastable`: si el martillo
    // dejara de contar como tal, cada piedra machacada se comeria un martillo.
    CHECK(esHerramientaGastable(BLOCK_MARTILLO_PIEDRA));
    CHECK(esHerramientaGastable(BLOCK_MARTILLO_PEDERNAL));

    // Y la piedra labrada NO es herramienta: tiene que consumirse.
    CHECK_FALSE(esHerramientaGastable(BLOCK_COBBLESTONE));

    // Los dos martillos tienen vida, o el descuento de un uso no tendria
    // de donde restar.
    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PIEDRA) > 0);
    CHECK(vidaMaximaHerramienta(BLOCK_MARTILLO_PEDERNAL) > 0);
}

TEST_CASE("Machacar: los 5 pedazos caben de sobra en un stack") {
    // Si 5 no cupieran en una casilla, el crafteo perderia material en
    // silencio. El tope es 187, asi que hay margen amplio.
    CHECK(MAX_STACK_SIZE >= 5);

    InventorySlot s;
    s.blockType = BLOCK_PEDAZO_PIEDRA;
    s.count = 0;
    s.add(BLOCK_PEDAZO_PIEDRA, 5);
    CHECK(s.count == 5);
}

// ============================================================================
// LA INVERSA: 4 PEDAZOS -> 1 PIEDRA LABRADA
// ============================================================================
// Cuatro cascotes encajados dan de nuevo un adoquin.
//
// EL CICLO NO ES NEUTRO, Y ES DELIBERADO:
//
//     1 piedra labrada  --martillo-->  5 pedazos
//     4 pedazos         --juntar--->   1 piedra labrada
//
// Se gana un pedazo por vuelta, pero cada machacado gasta un uso del martillo
// (uno de sus 150), asi que el beneficio esta topado por la durabilidad y
// acaba costando otro martillo. Sin esa ganancia, desmenuzar y rehacer seria
// perdida neta y nadie usaria ninguna de las dos recetas.
// ============================================================================

TEST_CASE("Juntar: la receta esta registrada y da 1 piedra labrada") {
    const std::string src = fuenteDelMotor();
    REQUIRE_MESSAGE(!src.empty(), "no se pudo leer src/main.cpp");

    const size_t m = src.find("JUNTAR CASCOTES: 4 PEDAZOS -> 1 PIEDRA LABRADA");
    REQUIRE_MESSAGE(m != std::string::npos,
                    "no esta la receta de juntar cascotes");

    const std::string cuerpo = src.substr(m, 2600);

    // UNA piedra labrada, y sin forma.
    CHECK_MESSAGE(
        cuerpo.find("CraftingRecipe recipe(BLOCK_COBBLESTONE, 1, true)")
            != std::string::npos,
        "debe dar 1 piedra labrada y ser shapeless");
}

TEST_CASE("Juntar: son CUATRO pedazos, en cuatro casillas") {
    // El matcher shapeless cuenta SLOTS OCUPADOS, no unidades: cuatro
    // casillas con un pedazo cada una. Un solo slot con 4 pedazos dentro NO
    // activa la receta, que es lo que se ve en la imagen de referencia.
    const std::string src = fuenteDelMotor();
    REQUIRE(!src.empty());

    const size_t m = src.find("JUNTAR CASCOTES: 4 PEDAZOS -> 1 PIEDRA LABRADA");
    REQUIRE(m != std::string::npos);
    const std::string cuerpo = src.substr(m, 2600);

    // Las cuatro posiciones del patron.
    CHECK(cuerpo.find("recipe.pattern[0] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(cuerpo.find("recipe.pattern[1] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(cuerpo.find("recipe.pattern[2] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);
    CHECK(cuerpo.find("recipe.pattern[3] = BLOCK_PEDAZO_PIEDRA;") != std::string::npos);

    // Y ninguna quinta: con 5 el recuento de slots no cuadraria.
    CHECK(cuerpo.find("recipe.pattern[4] = BLOCK_PEDAZO_PIEDRA;") == std::string::npos);
}

TEST_CASE("Juntar: no choca con la receta del hilo de ixtle") {
    // El hilo usa TRES pedazos mas una hoja de maguey. El matcher shapeless
    // compara el recuento de slots POR TIPO, asi que 4-pedazos y
    // 3-pedazos-mas-hoja nunca empatan. Si alguien bajara esta receta a 3
    // pedazos, las dos competirian por la misma rejilla.
    const std::string src = fuenteDelMotor();
    REQUIRE(!src.empty());

    // La del ixtle sigue siendo 3 pedazos + 1 hoja.
    const size_t ix = src.find("CraftingRecipe recipe(BLOCK_HILO_IXTLE, 4, true)");
    REQUIRE(ix != std::string::npos);
    const std::string hilo = src.substr(ix, 400);
    CHECK(hilo.find("recipe.pattern[3] = h;") != std::string::npos);
}

TEST_CASE("Juntar: el ciclo machacar/rehacer no pierde material") {
    // 1 labrada -> 5 pedazos -> (4) -> 1 labrada, y sobra 1 pedazo.
    // Es lo que hace que las dos recetas se usen. Si rehacer costara 5, el
    // ciclo seria neutro y no habria motivo para machacar nada.
    constexpr int DA_AL_MACHACAR = 5;
    constexpr int CUESTA_REHACER = 4;
    CHECK(DA_AL_MACHACAR > CUESTA_REHACER);
    CHECK(DA_AL_MACHACAR - CUESTA_REHACER == 1);

    // Y la piedra labrada cabe de sobra en un stack.
    CHECK(MAX_STACK_SIZE >= 1);
}

TEST_CASE("Juntar: la piedra labrada es un bloque colocable de verdad") {
    // Si el resultado no fuera colocable, la receta daria un item muerto.
    CHECK((int)BLOCK_COBBLESTONE <= (int)BLOCK_TYPE_MAX);
    CHECK(BLOCK_COBBLESTONE != BLOCK_AIR);
    CHECK(BLOCK_COBBLESTONE != BLOCK_PEDAZO_PIEDRA);
    // No es una herramienta: se consume al usarse en otras recetas.
    CHECK_FALSE(esHerramientaGastable(BLOCK_COBBLESTONE));
}
