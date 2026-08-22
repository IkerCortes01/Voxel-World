#include <doctest/doctest.h>
#include "Inventory.h"
#include <set>

// ============================================================================
// EL INVENTARIO CREATIVO
// ============================================================================
// Reproduce el mismo recorrido que hace el juego al entrar en un mundo
// creativo. Fija dos cosas que antes fallaban:
//
//   1. CABEN TODOS. El bucle cortaba en Inventory::SLOTS (45) y los 113
//      bloques colocables no entraban: 68 se quedaban fuera.
//   2. CADA UNO EN SU CASILLA. Ni se agrupan ni se comparten: un objeto por
//      slot, con su propio contador infinito.

static int llenarComoElCreativo(Inventory& inv) {
    int n = 0;
    auto poner = [&](BlockType bt) {
        InventorySlot& s = inv.at(n);
        s.blockType = bt;
        s.count = INFINITE_COUNT;
        s.vidaMedios = 0;
        ++n;
    };

    for (int id = 1; id <= BLOCK_LAST_PLACEABLE; ++id) {
        const BlockType bt = (BlockType)id;
        if (esNivelParcial(bt)) continue;
        poner(bt);
    }
    poner(BLOCK_HORNO);
    poner(BLOCK_HACHA_PIEDRA);
    poner(BLOCK_PICO_PIEDRA);
    poner(BLOCK_MARTILLO_PIEDRA);
    poner(BLOCK_PEDERNAL_AFILADO);
    return n;
}

TEST_CASE("Creativo: caben todos los bloques, no solo 45") {
    Inventory inv;
    const int n = llenarComoElCreativo(inv);

    // Con el limite viejo esto era 45 y se perdian mas de 70 objetos.
    CHECK(n > Inventory::SLOTS);
    CHECK(n >= 113);
    CHECK(inv.total() >= n);
}

TEST_CASE("Creativo: cada objeto en su casilla, sin compartir") {
    Inventory inv;
    const int n = llenarComoElCreativo(inv);

    // Ni un solo tipo repetido: si dos casillas tuvieran el mismo objeto,
    // estarian "compartiendo" en vez de ser independientes.
    std::set<int> vistos;
    for (int i = 0; i < n; ++i) {
        const BlockType bt = inv.at(i).blockType;
        CHECK(bt != BLOCK_AIR);
        CHECK(vistos.count((int)bt) == 0);
        vistos.insert((int)bt);
    }
    CHECK((int)vistos.size() == n);
}

TEST_CASE("Creativo: todos los stacks son infinitos e independientes") {
    Inventory inv;
    const int n = llenarComoElCreativo(inv);

    for (int i = 0; i < n; ++i)
        CHECK(isInfiniteStack(inv.at(i).count));

    // Gastar de una casilla no toca a las demas: son independientes.
    inv.selectedSlot = 5;
    const BlockType antes = inv.at(6).blockType;
    for (int k = 0; k < 50; ++k) inv.consumeSelected();

    CHECK(isInfiniteStack(inv.at(5).count));   // el infinito no baja
    CHECK(inv.at(6).blockType == antes);       // el vecino, intacto
    CHECK(isInfiniteStack(inv.at(6).count));
}

TEST_CASE("Creativo: no se dispara la memoria") {
    Inventory inv;
    const int n = llenarComoElCreativo(inv);

    // El inventario crece a lo que hace falta y poco mas: at() tiene tope de
    // salto justamente para que una lectura lejana no reserve de golpe.
    CHECK(inv.total() < n + Inventory::SLOTS + 1);

    // Y lo que ocupa es despreciable: unas 120 casillas de 12 bytes.
    CHECK(inv.total() * (int)sizeof(InventorySlot) < 64 * 1024);
}
