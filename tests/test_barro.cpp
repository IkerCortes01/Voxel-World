#include <doctest/doctest.h>
#include "BlockType.h"

// ============================================================================
// TESTS DEL BARRO
// ============================================================================
// La receta: un TAZON CON AGUA + POLVO DE TIERRA -> 1 pedazo de barro.
//
// Lo que fijan estos tests no es la receta en si (vive en CraftingSystem, que
// necesita medio motor para instanciarse), sino las piezas de BlockType.h de
// las que depende y que si son comprobables por separado:
//
//   - que el barro existe y es un item, no un bloque colocable
//   - que los tres tazones con agua se reconocen como tales
//   - que vaciar un tazon devuelve el MISMO tipo de madera
//
// Ese ultimo punto es el delicado: al craftear, el tazon no se consume sino
// que se vacia, y si la correspondencia lleno->vacio se rompiera, un tazon de
// encino podria volver convertido en uno de pino.

TEST_CASE("Barro: existe y entra en el rango que se valida") {
    // BLOCK_TYPE_MAX es el tope con el que el deserializador valida los IDs
    // que lee de disco. El barro tiene que quedar DENTRO, o un mundo con
    // barro guardado se rechazaria al cargar.
    //
    // (Antes esto comprobaba que el barro fuese el ULTIMO del enum. Dejo de
    // hacerlo a proposito: cada bloque nuevo va al final, asi que esa
    // afirmacion caduca sola. Lo que de verdad importa es que quepa.)
    CHECK((int)BLOCK_PEDAZO_BARRO <= BLOCK_TYPE_MAX);
    CHECK((int)BLOCK_PEDAZO_BARRO > (int)BLOCK_PYRITE_ORE);
}

TEST_CASE("Barro: es un ITEM, no un bloque del terreno") {
    // Va despues del ultimo colocable, que es lo que hace que no salga en el
    // recorrido de bloques del inventario creativo ni se pueda poner en el
    // suelo.
    CHECK((int)BLOCK_PEDAZO_BARRO > BLOCK_LAST_PLACEABLE);

    // Y no es una herramienta: no tiene vida ni se gasta.
    CHECK_FALSE(esHerramientaGastable(BLOCK_PEDAZO_BARRO));
    CHECK(vidaMaximaHerramienta(BLOCK_PEDAZO_BARRO) == 0);
}

TEST_CASE("Barro: los tres tazones con agua valen como ingrediente") {
    // La receta acepta cualquiera de los tres: lo que aporta es el AGUA, no
    // la madera. Si alguno dejara de reconocerse como tazon con agua, su
    // receta se quedaria sin efecto en silencio.
    CHECK(esTazonConAgua(BLOCK_TAZON_PINO_AGUA));
    CHECK(esTazonConAgua(BLOCK_TAZON_ENCINO_AGUA));
    CHECK(esTazonConAgua(BLOCK_TAZON_OYAMEL_AGUA));

    // Los vacios NO valen: sin agua no hay barro que amasar.
    CHECK_FALSE(esTazonConAgua(BLOCK_TAZON_PINO));
    CHECK_FALSE(esTazonConAgua(BLOCK_TAZON_ENCINO));
    CHECK_FALSE(esTazonConAgua(BLOCK_TAZON_OYAMEL));
}

TEST_CASE("Barro: al craftear, el tazon vuelve DE SU MISMA MADERA") {
    // El punto delicado. El tazon no se consume: se vacia en el sitio. Si
    // esta correspondencia se torciera, meter un tazon de encino devolveria
    // uno de pino -- un cambiazo silencioso que el jugador solo notaria al
    // mirar el inventario.
    CHECK(tazonVaciado(BLOCK_TAZON_PINO_AGUA)   == BLOCK_TAZON_PINO);
    CHECK(tazonVaciado(BLOCK_TAZON_ENCINO_AGUA) == BLOCK_TAZON_ENCINO);
    CHECK(tazonVaciado(BLOCK_TAZON_OYAMEL_AGUA) == BLOCK_TAZON_OYAMEL);

    // Y lo que sale es un tazon vacio de verdad, listo para volver a llenarse.
    CHECK(esTazonVacio(tazonVaciado(BLOCK_TAZON_PINO_AGUA)));
    CHECK(esTazonVacio(tazonVaciado(BLOCK_TAZON_ENCINO_AGUA)));
    CHECK(esTazonVacio(tazonVaciado(BLOCK_TAZON_OYAMEL_AGUA)));
}

TEST_CASE("Barro: vaciar y volver a llenar da la vuelta completa") {
    // Se puede hacer barro, rellenar el tazon en el rio y repetir. La ida y
    // la vuelta tienen que cerrar el circulo en las tres maderas.
    const BlockType LLENOS[] = {
        BLOCK_TAZON_PINO_AGUA, BLOCK_TAZON_ENCINO_AGUA, BLOCK_TAZON_OYAMEL_AGUA
    };
    for (BlockType lleno : LLENOS) {
        const BlockType vacio = tazonVaciado(lleno);
        REQUIRE(vacio != BLOCK_AIR);
        CHECK(tazonLleno(vacio) == lleno);
    }
}

TEST_CASE("Barro: el polvo de tierra sigue siendo el otro ingrediente") {
    // El polvo es un item puro; si dejara de existir o cambiara de naturaleza,
    // la receta se quedaria coja.
    CHECK((int)BLOCK_DIRT_POWDER > BLOCK_LAST_PLACEABLE);
    CHECK_FALSE(esHerramientaGastable(BLOCK_DIRT_POWDER));
}

TEST_CASE("Barro: vaciar algo que no es un tazon no inventa nada") {
    // tazonVaciado devuelve BLOCK_AIR para lo que no sea un tazon con agua.
    // Es la señal que usa executeCrafting para NO tocar el slot: sin ella, un
    // ingrediente cualquiera podria convertirse en otra cosa al craftear.
    CHECK(tazonVaciado(BLOCK_DIRT_POWDER)   == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_PEDAZO_BARRO)  == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_STONE)         == BLOCK_AIR);
    CHECK(tazonVaciado(BLOCK_TAZON_PINO)    == BLOCK_AIR);  // ya estaba vacio
}
