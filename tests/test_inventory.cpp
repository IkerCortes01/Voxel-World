#include <doctest/doctest.h>
#include "Inventory.h"

// ============================================================================
// TESTS DEL INVENTARIO — semántica propia de este motor
// ============================================================================
// Este motor difiere del Inventory de Voxel-Genesis a propósito:
//  - MAX_STACK_SIZE = 187 (no 100)
//  - stacks infinitos para creativo (count centinela, remove no consume)
//  - add() devuelve void y recorta al tope (el exceso se descarta; el
//    llamador decide con canStack() antes)
// Los tests fijan ESE comportamiento: si alguien lo cambia sin querer
// (p. ej. "arreglando" add para que devuelva lo añadido), esto lo detecta.

TEST_CASE("InventorySlot: estado inicial vacio") {
    InventorySlot s;
    CHECK(s.isEmpty());
    CHECK(s.blockType == BLOCK_AIR);
    CHECK(s.count == 0);
}

TEST_CASE("InventorySlot: add en slot vacio fija tipo y cantidad") {
    InventorySlot s;
    s.add(BLOCK_STONE, 5);
    CHECK_FALSE(s.isEmpty());
    CHECK(s.blockType == BLOCK_STONE);
    CHECK(s.count == 5);
}

TEST_CASE("InventorySlot: add acumula hasta MAX_STACK_SIZE y recorta el exceso") {
    InventorySlot s;
    s.add(BLOCK_DIRT, 100);
    s.add(BLOCK_DIRT, 100);          // 200 > 187: recorta
    CHECK(s.count == MAX_STACK_SIZE);
    CHECK(MAX_STACK_SIZE == 187);    // el tope de ESTE motor, no 100

    // Un solo add por encima del tope también recorta
    InventorySlot s2;
    s2.add(BLOCK_STONE, 999);
    CHECK(s2.count == MAX_STACK_SIZE);
}

TEST_CASE("InventorySlot: add de tipo distinto sobre slot ocupado no hace nada") {
    InventorySlot s;
    s.add(BLOCK_STONE, 3);
    s.add(BLOCK_DIRT, 5);            // tipo distinto: ignorado
    CHECK(s.blockType == BLOCK_STONE);
    CHECK(s.count == 3);
}

TEST_CASE("InventorySlot: canStack respeta tipo, tope y slot infinito") {
    InventorySlot vacio;
    CHECK(vacio.canStack(BLOCK_STONE));

    InventorySlot mismo;
    mismo.add(BLOCK_STONE, 10);
    CHECK(mismo.canStack(BLOCK_STONE));
    CHECK_FALSE(mismo.canStack(BLOCK_DIRT));

    InventorySlot lleno;
    lleno.add(BLOCK_STONE, MAX_STACK_SIZE);
    CHECK_FALSE(lleno.canStack(BLOCK_STONE));

    // Un slot infinito NUNCA admite más: sumarle destruiría el centinela
    InventorySlot inf;
    inf.blockType = BLOCK_STONE;
    inf.count = INFINITE_COUNT;
    CHECK_FALSE(inf.canStack(BLOCK_STONE));
}

TEST_CASE("InventorySlot: remove consume y vacia al llegar a cero") {
    InventorySlot s;
    s.add(BLOCK_SAND, 2);
    CHECK(s.remove(1));
    CHECK(s.count == 1);
    CHECK(s.remove(1));
    CHECK(s.isEmpty());
    CHECK(s.blockType == BLOCK_AIR);   // el slot se resetea, no queda tipo huerfano
    CHECK_FALSE(s.remove(1));          // vacio: no hay nada que quitar
}

TEST_CASE("InventorySlot: remove de mas de lo que hay falla sin tocar el slot") {
    InventorySlot s;
    s.add(BLOCK_SAND, 3);
    CHECK_FALSE(s.remove(5));
    CHECK(s.count == 3);
}

TEST_CASE("InventorySlot: stack infinito tiene exito al remover y no se consume") {
    InventorySlot s;
    s.blockType = BLOCK_STONE;
    s.count = INFINITE_COUNT;

    CHECK(isInfiniteStack(s.count));
    for (int i = 0; i < 1000; i++) {
        CHECK(s.remove(1));            // siempre exito...
    }
    CHECK(s.count == INFINITE_COUNT);  // ...y nunca baja
    CHECK_FALSE(s.isEmpty());
}

TEST_CASE("isInfiniteStack: el centinela no se confunde con cantidades legitimas") {
    CHECK_FALSE(isInfiniteStack(0));
    CHECK_FALSE(isInfiniteStack(MAX_STACK_SIZE));
    CHECK(isInfiniteStack(INFINITE_COUNT));
    CHECK(isInfiniteStack(INFINITE_COUNT + 1));
    // El centinela queda MUY lejos del tope real: no hay zona ambigua
    CHECK(INFINITE_COUNT > MAX_STACK_SIZE * 1000);
}

TEST_CASE("Inventory: addItem stackea en el primer slot compatible") {
    Inventory inv;
    CHECK(inv.addItem(BLOCK_STONE, 10));
    CHECK(inv.addItem(BLOCK_STONE, 10));
    CHECK(inv.slots[0].count == 20);       // mismo slot, no dos slots
    CHECK(inv.slots[1].isEmpty());

    CHECK(inv.addItem(BLOCK_DIRT, 1));     // tipo nuevo: siguiente slot libre
    CHECK(inv.slots[1].blockType == BLOCK_DIRT);
}

TEST_CASE("Inventory: addItem falla con el inventario lleno de otros tipos") {
    Inventory inv;
    // Llenar los 45 slots al tope con piedra
    for (int i = 0; i < Inventory::SLOTS; i++) {
        inv.slots[i].blockType = BLOCK_STONE;
        inv.slots[i].count = MAX_STACK_SIZE;
    }
    CHECK_FALSE(inv.addItem(BLOCK_DIRT, 1));
    CHECK_FALSE(inv.addItem(BLOCK_STONE, 1));  // tambien: todos al tope
}

TEST_CASE("Inventory: removeItem exige un slot con cantidad suficiente") {
    Inventory inv;
    inv.addItem(BLOCK_SAND, 5);
    CHECK(inv.removeItem(BLOCK_SAND, 3));
    CHECK(inv.slots[0].count == 2);
    CHECK_FALSE(inv.removeItem(BLOCK_SAND, 10));   // no alcanza
    CHECK_FALSE(inv.removeItem(BLOCK_DIRT, 1));    // no existe
}

TEST_CASE("Inventory: seleccion y consumo del slot activo") {
    Inventory inv;
    inv.addItem(BLOCK_PLANKS, 2);
    inv.selectedSlot = 0;

    CHECK(inv.hasSelectedBlock());
    CHECK(inv.getSelectedBlock() == BLOCK_PLANKS);

    inv.consumeSelected();
    CHECK(inv.slots[0].count == 1);
    inv.consumeSelected();
    CHECK_FALSE(inv.hasSelectedBlock());
    CHECK(inv.getSelectedBlock() == BLOCK_AIR);

    // Seleccion fuera de rango: comportamiento seguro
    inv.selectedSlot = -1;
    CHECK_FALSE(inv.hasSelectedBlock());
    CHECK(inv.getSelectedBlock() == BLOCK_AIR);
    inv.consumeSelected();                          // no debe crashear
}

TEST_CASE("Inventory: clear vacia todos los slots y resetea la seleccion") {
    Inventory inv;
    inv.addItem(BLOCK_STONE, 10);
    inv.addItem(BLOCK_DIRT, 10);
    inv.selectedSlot = 7;

    inv.clear();
    for (int i = 0; i < Inventory::SLOTS; i++) {
        CHECK(inv.slots[i].isEmpty());
    }
    CHECK(inv.selectedSlot == 0);
}
