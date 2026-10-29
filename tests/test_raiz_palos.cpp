#include <doctest/doctest.h>
#include "BlockType.h"
#include "Inventory.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// LA RAIZ DA PALOS, Y EL INVENTARIO DE SUPERVIVENCIA EMPIEZA VACIO
// ============================================================================
// Dos cosas sin relacion entre si, en el mismo archivo porque las dos se
// comprueban leyendo main.cpp: el binario de tests no lo enlaza, asi que ni
// CraftingSystem ni el bucle de la pantalla de carga estan disponibles como
// simbolos. Es el mismo camino que ya siguen test_creativo_completo.cpp y
// test_techo_hojas.cpp.
//
//   1. UNA RAIZ, DEL TAMAÑO QUE SEA, DA 4 PALOS. Es la via temprana al
//      palo: sin ella hace falta un hacha para talar, y el hacha pide un
//      palo. Las cuatro recetas (una por tamaño) tienen que existir, porque
//      el emparejado compara el bloque EXACTO.
//
//   2. EN SUPERVIVENCIA EL INVENTARIO NO HEREDA NADA DEL CREATIVO. El bug:
//      `GameState` sobrevive a salir al menu, el inventario creativo crece a
//      ~115 casillas, y al entrar a un mundo de supervivencia las casillas
//      por encima de las 45 se quedaban llenas de items infinitos.
// ============================================================================

namespace {

std::string raizProyecto() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    return r;
#else
    return {};
#endif
}

std::string leerArchivo(const std::string& ruta) {
    std::ifstream f(ruta, std::ios::binary);
    if (!f) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. LA RECETA
// ---------------------------------------------------------------------------

TEST_CASE("Raiz->palos: los cuatro tamanos son bloques validos y distintos") {
    // Si dos tamanos colapsaran al mismo ID, el bucle de la receta
    // registraria la misma entrada varias veces y uno de ellos se quedaria
    // sin forma de craftearse.
    const BlockType R[] = { BLOCK_RAIZ_PEQUENA, BLOCK_RAIZ_MEDIANA,
                            BLOCK_RAIZ_GRANDE,  BLOCK_RAIZ_ENORME };
    for (int i = 0; i < 4; ++i) {
        INFO("tamano ", i);
        CHECK(esRaiz(R[i]));
        CHECK((int)R[i] <= (int)BLOCK_TYPE_MAX);
        for (int j = i + 1; j < 4; ++j) CHECK(R[i] != R[j]);
    }
}

TEST_CASE("Raiz->palos: existe la receta para los cuatro tamanos") {
    const std::string raiz = raizProyecto();
    REQUIRE_MESSAGE(!raiz.empty(),
                    "RAIZ_PROYECTO sin definir: el test no puede leer main.cpp");
    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE(!fuente.empty());

    // El bucle que registra las cuatro recetas. Se busca por su forma exacta
    // para que no lo de por bueno cualquier otra mencion de las raices.
    const size_t bucle = fuente.find("for (BlockType raiz : { BLOCK_RAIZ_PEQUENA");
    REQUIRE_MESSAGE(bucle != std::string::npos,
                    "no esta el bucle que da de alta la receta raiz->palos");

    // Los cuatro tamanos, dentro del propio bucle.
    const std::string cabecera = fuente.substr(bucle, 220);
    CHECK(cabecera.find("BLOCK_RAIZ_PEQUENA") != std::string::npos);
    CHECK(cabecera.find("BLOCK_RAIZ_MEDIANA") != std::string::npos);
    CHECK(cabecera.find("BLOCK_RAIZ_GRANDE")  != std::string::npos);
    CHECK(cabecera.find("BLOCK_RAIZ_ENORME")  != std::string::npos);

    // Y el cuerpo: cuatro palos, sin forma.
    const std::string cuerpo = fuente.substr(bucle, 420);
    CHECK_MESSAGE(cuerpo.find("CraftingRecipe recipe(BLOCK_STICK, 4, true)")
                      != std::string::npos,
                  "la receta debe dar 4 palos y ser shapeless");
    CHECK(cuerpo.find("recipe.pattern[0] = raiz;") != std::string::npos);
}

TEST_CASE("Raiz->palos: da 4, no 8") {
    // 8 es lo que dan DOS TABLONES, y para tener tablones hay que talar.
    // La raiz esta tirada en el suelo desde el primer minuto: si diera lo
    // mismo, nadie volveria a usar madera para hacer palos.
    const std::string raiz = raizProyecto();
    REQUIRE(!raiz.empty());
    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE(!fuente.empty());

    const size_t bucle = fuente.find("for (BlockType raiz : { BLOCK_RAIZ_PEQUENA");
    REQUIRE(bucle != std::string::npos);
    const std::string cuerpo = fuente.substr(bucle, 420);
    CHECK(cuerpo.find("BLOCK_STICK, 8") == std::string::npos);
}

// ---------------------------------------------------------------------------
// 2. EL INVENTARIO DE SUPERVIVENCIA
// ---------------------------------------------------------------------------

TEST_CASE("Supervivencia: el inventario reglamentario son 45 casillas") {
    // El numero que separa "lo normal" de "esto viene del creativo".
    CHECK(Inventory::SLOTS == 45);
}

TEST_CASE("Supervivencia: clear() deja el inventario en 45 casillas vacias") {
    // `clear()` hace assign(SLOTS), asi que ENCOGE un inventario que venga
    // crecido del creativo. De eso depende el caso del mundo recien creado,
    // que no tiene player.dat que leer.
    Inventory inv;
    for (int i = 0; i < 115; ++i) {
        InventorySlot& s = inv.at(i);
        s.blockType = BLOCK_STONE;
        s.count = INFINITE_COUNT;
    }
    REQUIRE(inv.total() >= 115);

    inv.clear();

    CHECK(inv.total() == Inventory::SLOTS);
    CHECK(inv.selectedSlot == 0);
    for (int i = 0; i < inv.total(); ++i) {
        INFO("casilla ", i);
        CHECK(inv.slots[(size_t)i].blockType == BLOCK_AIR);
    }
}

TEST_CASE("Supervivencia: la carga del player.dat ajusta el tamano EXACTO") {
    // ⭐ EL BUG. Antes era `if (total() < numSlots) resize(numSlots)`, que
    // solo crecia: un inventario de 115 casillas heredado del creativo se
    // quedaba en 115 al cargar un player.dat de 45, y las casillas 45..115
    // conservaban los items infinitos.
    const std::string raiz = raizProyecto();
    REQUIRE(!raiz.empty());
    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE(!fuente.empty());

    CHECK_MESSAGE(
        fuente.find("state->inventory.slots.assign((size_t)numSlots, InventorySlot());")
            != std::string::npos,
        "la carga debe fijar el tamano exacto (assign), no solo crecer");

    // Y que no haya vuelto el resize condicional que causaba el bug.
    CHECK_MESSAGE(
        fuente.find("if (state->inventory.total() < numSlots)") == std::string::npos,
        "ha vuelto el resize que solo crece: el bug del creativo reaparece");
}

TEST_CASE("Supervivencia: la pantalla de carga recorta lo heredado") {
    // La segunda red, para el mundo recien creado y para cualquier camino que
    // no pase por player.dat.
    const std::string raiz = raizProyecto();
    REQUIRE(!raiz.empty());
    const std::string fuente = leerArchivo(raiz + "src/main.cpp");
    REQUIRE(!fuente.empty());

    const size_t marca = fuente.find("SUPERVIVENCIA: EL INVENTARIO EMPIEZA VACIO");
    REQUIRE_MESSAGE(marca != std::string::npos,
                    "no esta la rama de supervivencia de la pantalla de carga");

    // Margen amplio: entre la marca y el codigo hay un comentario largo que
    // explica por que aqui no se puede llamar a clear().
    const std::string cuerpo = fuente.substr(marca, 3000);
    CHECK(cuerpo.find("inv.total() > Inventory::SLOTS") != std::string::npos);
    CHECK(cuerpo.find("inv.slots.resize((size_t)Inventory::SLOTS);") != std::string::npos);

    // ⚠️ Y que NO se llame a clear() ahi: ese codigo corre DESPUES de
    // loadWorldData, asi que un clear() borraria la partida recien cargada
    // del disco. Es el error facil de cometer al tocar esto.
    //
    // Se mira solo hasta el cierre de la rama, no los 3000 caracteres: mas
    // alla empieza otro bloque y un clear() suyo daria un fallo falso.
    const size_t finRama = cuerpo.find("if (inv.selectedSlot");
    REQUIRE(finRama != std::string::npos);
    CHECK_MESSAGE(cuerpo.substr(0, finRama).find("inventory.clear()") == std::string::npos,
                  "clear() ahi borraria la partida cargada: solo recortar");
}

TEST_CASE("Supervivencia: recortar a 45 conserva lo que el jugador tenia") {
    // El recorte no puede llevarse por delante lo que loadWorldData acaba de
    // escribir en las 45 primeras casillas.
    Inventory inv;
    inv.clear();

    // Lo que el jugador consiguio por su cuenta.
    inv.slots[0].blockType = BLOCK_DIRT;   inv.slots[0].count = 12;
    inv.slots[7].blockType = BLOCK_STICK;  inv.slots[7].count = 4;

    // Y la cola heredada de una partida creativa anterior.
    for (int i = 45; i < 115; ++i) {
        InventorySlot& s = inv.at(i);
        s.blockType = BLOCK_STONE;
        s.count = INFINITE_COUNT;
    }
    REQUIRE(inv.total() >= 115);

    // El recorte, tal como lo hace la pantalla de carga.
    if (inv.total() > Inventory::SLOTS)
        inv.slots.resize((size_t)Inventory::SLOTS);

    CHECK(inv.total() == Inventory::SLOTS);
    CHECK(inv.slots[0].blockType == BLOCK_DIRT);
    CHECK(inv.slots[0].count == 12);
    CHECK(inv.slots[7].blockType == BLOCK_STICK);
    CHECK(inv.slots[7].count == 4);

    // Y ni una sola casilla con stack infinito: eso solo existe en creativo.
    for (int i = 0; i < inv.total(); ++i) {
        INFO("casilla ", i);
        CHECK(inv.slots[(size_t)i].count != INFINITE_COUNT);
    }
}
