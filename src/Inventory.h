#pragma once

#include "BlockType.h"

// ============================================================================
// INVENTARIO DEL JUGADOR
// ============================================================================
// Extraído de main.cpp: solo depende de BlockType, así que es testeable sin
// arrancar OpenGL (ver tests/test_inventory.cpp).
//
// Semántica propia de este motor (distinta del Inventory de Voxel-Genesis):
//  - MAX_STACK_SIZE es 187, no 100.
//  - Existen stacks INFINITOS para el modo creativo, marcados con un count
//    centinela; remove() sobre uno tiene éxito sin consumir.
//  - add() devuelve void y RECORTA al tope: los items que exceden
//    MAX_STACK_SIZE se descartan (el llamador decide qué dar por añadido con
//    canStack() antes de llamar).

// Tamaño máximo de pila por slot. Fuente única de verdad: antes el 100
// estaba repetido en canStack(), add() y en el handler de clicks del
// inventario, así que cambiarlo exigía tocar tres sitios y era fácil que
// se desincronizaran.
static const int MAX_STACK_SIZE = 187;

// ============================================================================
// STACK INFINITO (modo creativo)
// ============================================================================
// Valor centinela que marca un slot que nunca se agota. Se elige un número
// imposible de alcanzar por juego normal (el tope real es MAX_STACK_SIZE=187),
// de modo que no puede confundirse con una cantidad legítima.
//
// Se usa un centinela en vez de un bool por slot porque el inventario se
// serializa al guardar: un campo nuevo rompería el formato de los mundos ya
// existentes, mientras que un count distinto viaja por el mismo hueco.
static const int INFINITE_COUNT = 1000000;

inline bool isInfiniteStack(int count) { return count >= INFINITE_COUNT; }

struct InventorySlot {
    BlockType blockType;
    int count;

    // ⭐ VIDA DE LA HERRAMIENTA, en puntos x2.
    //
    // Se guarda en MEDIOS PUNTOS (enteros) en vez de en float porque el
    // desgaste se cuenta en pasos de 0.5: una raíz o una rama gastan medio
    // punto. Con enteros no hay error de redondeo acumulado tras cientos de
    // golpes, que es justo lo que arruinaría una barra de durabilidad.
    //
    // 0 significa "sin estrenar": la vida real la pone quien crea el item.
    int vidaMedios;

    InventorySlot() : blockType(BLOCK_AIR), count(0), vidaMedios(0) {}

    bool isEmpty() const { return blockType == BLOCK_AIR || count <= 0; }

    bool canStack(BlockType type) const {
        // Un slot infinito no admite más: sumarle destruiría el centinela.
        if (isInfiniteStack(count)) return false;
        return isEmpty() || (blockType == type && count < MAX_STACK_SIZE);
    }

    void add(BlockType type, int amount = 1) {
        if (isEmpty()) {
            blockType = type;
            count = amount;
            if (count > MAX_STACK_SIZE) count = MAX_STACK_SIZE;
        } else if (blockType == type) {
            count += amount;
            if (count > MAX_STACK_SIZE) count = MAX_STACK_SIZE;
        }
    }

    bool remove(int amount = 1) {
        // ⭐ STACK INFINITO: en creativo el slot no se consume nunca.
        // Se devuelve true (la operación tiene éxito: el jugador sí obtiene
        // el bloque) pero el contador no baja.
        if (isInfiniteStack(count)) return true;

        if (count >= amount) {
            count -= amount;
            if (count <= 0) {
                blockType = BLOCK_AIR;
                count = 0;
            }
            return true;
        }
        return false;
    }
};

// Inventario del jugador
struct Inventory {
    static const int SLOTS = 45;  // 45 slots (5 filas de 9)
    InventorySlot slots[SLOTS];
    int selectedSlot;

    Inventory() : selectedSlot(0) {
        // ⭐ Inventario vacío al inicio - el jugador debe conseguir items minando/crafteo
        // Los slots se inicializan automáticamente con BLOCK_AIR y count=0
    }

    // ⭐ Limpiar completamente el inventario (para mundos nuevos)
    void clear() {
        for (int i = 0; i < SLOTS; i++) {
            slots[i].blockType = BLOCK_AIR;
            slots[i].count = 0;
        }
        selectedSlot = 0;
    }

    bool addItem(BlockType type, int amount = 1) {
        // Intentar stackear en slot existente
        for (int i = 0; i < SLOTS; i++) {
            if (slots[i].canStack(type)) {
                slots[i].add(type, amount);
                return true;
            }
        }
        return false;
    }

    bool removeItem(BlockType type, int amount = 1) {
        for (int i = 0; i < SLOTS; i++) {
            if (slots[i].blockType == type && slots[i].count >= amount) {
                slots[i].remove(amount);
                return true;
            }
        }
        return false;
    }

    BlockType getSelectedBlock() const {
        if (selectedSlot >= 0 && selectedSlot < SLOTS) {
            return slots[selectedSlot].blockType;
        }
        return BLOCK_AIR;
    }

    // ⭐ GASTAR VIDA DE LA HERRAMIENTA QUE SE LLEVA EN LA MANO
    //
    // Devuelve true si la herramienta se ha ROTO con este golpe (vida <= 0),
    // en cuyo caso el slot se vacia: un hacha gastada desaparece, como en
    // cualquier juego del genero.
    //
    // Si en la mano no hay un hacha, no hace nada y devuelve false: romper
    // bloques a mano nunca gasta nada.
    bool gastarHerramienta(int medios) {
        if (selectedSlot < 0 || selectedSlot >= SLOTS) return false;
        InventorySlot& s = slots[selectedSlot];
        if (s.blockType != BLOCK_HACHA_PIEDRA || s.isEmpty()) return false;

        // Un hacha recien hecha entra con la vida a 0 (sin estrenar): la
        // primera vez que se usa se le pone la vida completa.
        if (s.vidaMedios <= 0) s.vidaMedios = HACHA_VIDA_MEDIOS;

        s.vidaMedios -= medios;
        if (s.vidaMedios <= 0) {
            // Se rompe: desaparece una del stack.
            s.count--;
            s.vidaMedios = 0;
            if (s.count <= 0) {
                s.blockType = BLOCK_AIR;
                s.count = 0;
            }
            return true;
        }
        return false;
    }

    // Vida que le queda al hacha de la mano, en PUNTOS (no medios).
    // Devuelve -1 si en la mano no hay un hacha.
    float vidaHerramienta() const {
        if (selectedSlot < 0 || selectedSlot >= SLOTS) return -1.0f;
        const InventorySlot& s = slots[selectedSlot];
        if (s.blockType != BLOCK_HACHA_PIEDRA || s.isEmpty()) return -1.0f;
        const int m = (s.vidaMedios <= 0) ? HACHA_VIDA_MEDIOS : s.vidaMedios;
        return (float)m * 0.5f;
    }

    bool hasSelectedBlock() const {
        if (selectedSlot >= 0 && selectedSlot < SLOTS) {
            return !slots[selectedSlot].isEmpty();
        }
        return false;
    }

    void consumeSelected() {
        if (selectedSlot >= 0 && selectedSlot < SLOTS) {
            slots[selectedSlot].remove(1);
        }
    }
};
