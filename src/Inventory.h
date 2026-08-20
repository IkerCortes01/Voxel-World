#pragma once

#include "BlockType.h"
#include <vector>

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

    // ⭐ VACIAR DE VERDAD
    //
    // Un slot con count = 0 pero blockType sin limpiar SIGUE contando como
    // "hay un bloque aqui" en todo el codigo que mira blockType sin pasar
    // por isEmpty(): la hotbar, lo que el jugador lleva en la mano, el
    // crafteo... De ahi que un hueco vacio se comportara como si tuviera
    // algo.
    //
    // Esto lo deja limpio: sin tipo, sin cuenta y sin vida de herramienta.
    void vaciar() {
        blockType = BLOCK_AIR;
        count = 0;
        vidaMedios = 0;
    }

    // Deja el slot coherente: si se ha quedado sin unidades, se vacia del
    // todo en vez de conservar el tipo.
    void normalizar() {
        if (count <= 0) vaciar();
    }

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
            // Al quedarse sin unidades, el slot se vacia DEL TODO: si se
            // dejara el blockType puesto, el hueco seguiria contando como
            // "hay un bloque aqui" en el codigo que mira el tipo sin pasar
            // por isEmpty().
            if (count <= 0) vaciar();
            return true;
        }
        return false;
    }
};

// Inventario del jugador
//
// ============================================================================
// SLOTS SIN FONDO: EL INVENTARIO CRECE SOLO
// ============================================================================
// Las 45 casillas de siempre (5 filas de 9) siguen ahí y son las de arriba,
// las que se ven al abrir. Por debajo, el inventario CRECE cuando hace falta:
// si se recoge algo y las filas actuales están llenas, aparece otra fila.
//
// "Infinito" de verdad no cabe en memoria, así que se hace lo único que sí es
// posible: reservar bajo demanda. Como el crecimiento no tiene tope fijado,
// desde dentro del juego es indistinguible de infinito -- se seguirán abriendo
// filas mientras quede memoria.
//
// La ventaja de crecer en vez de reservar un millón de casillas de golpe: un
// jugador que acaba de empezar gasta lo de 45 slots, no lo de un millón.
struct Inventory {
    // Las casillas PRINCIPALES: la parrilla que se ve de un vistazo.
    // Se conserva el nombre SLOTS porque medio motor lo usa.
    static const int SLOTS = 45;  // 45 slots (5 filas de 9)
    static const int COLUMNAS = 9;

    // Todas las casillas, las 45 de arriba y las que hayan ido saliendo.
    // Nunca baja de SLOTS: las principales existen siempre.
    std::vector<InventorySlot> slots;
    int selectedSlot;

    // Cuántas filas se han desplazado con la rueda del ratón.
    int scroll;

    Inventory() : slots((size_t)SLOTS), selectedSlot(0), scroll(0) {
        // ⭐ Inventario vacío al inicio - el jugador debe conseguir items minando/crafteo
        // Los slots se inicializan automáticamente con BLOCK_AIR y count=0
    }

    // Cuántas casillas hay ahora mismo.
    int total() const { return (int)slots.size(); }

    // Acceso por índice. Si se pide una casilla que aún no existe, el
    // inventario CRECE hasta ahí: por eso nunca se queda sin sitio.
    //
    // ⚠️ CON UN LÍMITE DE SALTO. Sin él, un solo at(999999) -- por ejemplo
    // desde el dibujado, si el scroll se va lejos -- reservaba un MILLÓN de
    // casillas de golpe. Medido en la prueba: el inventario pasaba de 45 a
    // 1.000.000 en una llamada.
    //
    // Ahora se crece como mucho una pantalla más allá de lo que ya hay. Eso
    // basta para que el inventario nunca se llene (cada aporte abre su
    // casilla), pero impide que una lectura suelta dispare la memoria.
    InventorySlot& at(int i) {
        if (i < 0) i = 0;
        if ((size_t)i >= slots.size()) {
            const int MAX_SALTO = SLOTS;      // una pantalla de margen
            const int tope = total() + MAX_SALTO;
            const int hasta = (i < tope) ? i : tope;
            slots.resize((size_t)hasta + 1);
            if (i > hasta) {
                // Se pidió algo absurdamente lejos: se devuelve la última
                // casilla real en vez de reservar hasta ahí.
                return slots.back();
            }
        }
        return slots[(size_t)i];
    }
    const InventorySlot& at(int i) const {
        static InventorySlot vacio;
        if (i < 0 || (size_t)i >= slots.size()) return vacio;
        return slots[(size_t)i];
    }

    // ⭐ Limpiar completamente el inventario (para mundos nuevos)
    void clear() {
        slots.assign((size_t)SLOTS, InventorySlot());
        selectedSlot = 0;
        scroll = 0;
    }

    bool addItem(BlockType type, int amount = 1) {
        // Intentar stackear en un slot que ya tenga de eso
        for (int i = 0; i < total(); i++) {
            if (slots[(size_t)i].canStack(type)) {
                slots[(size_t)i].add(type, amount);
                return true;
            }
        }
        // No cabe en lo que hay: se abre una casilla nueva. Aquí es donde el
        // inventario deja de tener fondo -- nunca devuelve "lleno".
        slots.push_back(InventorySlot());
        slots.back().add(type, amount);
        return true;
    }

    bool removeItem(BlockType type, int amount = 1) {
        for (int i = 0; i < total(); i++) {
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
