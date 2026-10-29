#include <doctest/doctest.h>
#include "ui/NombreItem.h"
#include "ui/InventarioLayout.h"
#include <cstring>
#include <string>

// ============================================================================
// EL NOMBRE DE CADA ITEM, Y LA ETIQUETA QUE LO MUESTRA
// ============================================================================
// PEDIDO: "todos los items deben tener -- bloques, herramientas, todo -- que
// cuando pasas el cursor encima de la casilla te muestre su nombre en una
// mini seccion abajo que se sobrepone al inventario".

using namespace NombreItem;
using namespace InventarioLayout;

namespace {
// Los IDs que ya no existen en el juego: minerales retirados y sus capas.
// No pueden llegar al inventario, asi que no necesitan nombre.
bool esReservado(BlockType t) {
    const int i = (int)t;
    if (i >= BLOCK_COAL_L1 && i <= BLOCK_SCRAP_L7) return true;   // capas de minerales retirados
    return t == BLOCK_RESERVADO_MINERAL_08 || t == BLOCK_RESERVADO_MINERAL_09 ||
           t == BLOCK_RESERVADO_MINERAL_20 || t == BLOCK_RESERVADO_MINERAL_21 ||
           t == BLOCK_RESERVADO_MINERAL_26 || t == BLOCK_RESERVADO_MINERAL_164;
}
}

TEST_CASE("Nombres: TODO item del juego tiene nombre") {
    // ⭐ EL PEDIDO: "todos los items deben tener". Se recorre el enum entero
    // -- es contiguo desde 0 -- y solo se perdonan los reservados y las capas
    // que cuelgan de ellos.
    int sinNombre = 0;
    for (int i = 1; i <= BLOCK_TYPE_MAX; ++i) {
        const BlockType t = (BlockType)i;
        const BlockType base = esNivelParcial(t) ? bloqueBaseDe(t) : t;
        // Se mira el ID tal cual Y su base: las capas de los minerales
        // retirados son niveles cuya base quedo anulada a AIRE.
        if (esReservado(t) || esReservado(base)) continue;
        if (nombreDeItem(t) == nullptr) {
            INFO("ID sin nombre: ", i);
            CHECK(nombreDeItem(t) != nullptr);
            ++sinNombre;
        }
    }
    CHECK(sinNombre == 0);
}

TEST_CASE("Nombres: solo usan letras que la fuente sabe dibujar") {
    // renderChar dibuja A-Z, 0-9, espacio, '-' y '+'. Una tilde, una Ñ o un
    // signo saldrian como un hueco en mitad del nombre.
    for (int i = 1; i <= BLOCK_TYPE_MAX; ++i) {
        const char* n = nombreDeItem((BlockType)i);
        if (!n) continue;
        INFO("ID ", i, ": ", n);
        CHECK(std::strlen(n) > 0);
        for (const char* p = n; *p; ++p) CHECK(caracterDibujable(*p));
    }
}

TEST_CASE("Nombres: una capa se llama como su bloque") {
    CHECK(std::string(nombreDeItem(conNivel(BLOCK_DIRT, 3))) == "TIERRA");
    CHECK(std::string(nombreDeItem(conNivel(BLOCK_PLANKS, 1))) == "TABLONES DE PINO");
}

TEST_CASE("Nombres: las capas viejas de tronco se llaman como su tronco") {
    // Ya no son niveles (esNivelParcial dice que no), pero un mundo viejo
    // podria tenerlas: no pueden quedarse sin nombre.
    CHECK(std::string(nombreDeItem(BLOCK_WOOD_L3))  == "TRONCO DE PINO");
    CHECK(std::string(nombreDeItem(BLOCK_WOODE_L1)) == "TRONCO DE ENCINO");
    CHECK(std::string(nombreDeItem(BLOCK_WOODO_L7)) == "TRONCO DE OYAMEL");
}

TEST_CASE("Nombres: las dos orientaciones del techo son el mismo item") {
    CHECK(std::string(nombreDeItem(BLOCK_TECHO_HOJAS)) ==
          std::string(nombreDeItem(BLOCK_TECHO_SUELO)));
}

TEST_CASE("Nombres: los compuestos se nombran por su familia") {
    const BlockType maguey = Compuesto::hacer(Compuesto::FAM_MAGUEY, 5);
    REQUIRE(nombreDeItem(maguey) != nullptr);
    CHECK(std::string(nombreDeItem(maguey)) == "MAGUEY PULQUERO");

    for (int f = 0; f < Compuesto::FAM_COUNT; ++f) {
        INFO("familia ", f);
        CHECK(nombreDeFamilia((Compuesto::Familia)f) != nullptr);
    }
}

TEST_CASE("Nombres: el aire no tiene nombre (casilla vacia, sin etiqueta)") {
    CHECK(nombreDeItem(BLOCK_AIR) == nullptr);
}

// ----------------------------------------------------------------------------
// QUE CASILLA HAY BAJO EL RATON
// ----------------------------------------------------------------------------
TEST_CASE("Casilla: el centro de cada casilla visible da su slot") {
    const float W = 1280.0f;
    const float x0 = startX(W);
    for (int fila = 0; fila < ROWS; ++fila)
        for (int col = 0; col < COLS; ++col) {
            const float mx = x0 + col * (SLOT_SIZE + SPACING) + SLOT_SIZE / 2;
            const float my = START_Y + fila * (SLOT_SIZE + SPACING) + SLOT_SIZE / 2;
            const Casilla c = casillaBajoCursor(mx, my, W, 0, 1000);
            CHECK(c.zona == Zona::INVENTARIO);
            CHECK(c.indice == fila * COLS + col);
        }
}

TEST_CASE("Casilla: respeta el scroll, como el dibujado") {
    // Con la ventana bajada 3 filas, la primera casilla visible es la 27. Es
    // el error que tuvo el clic: dibujar desde el scroll y probar desde 0.
    const float W = 1280.0f;
    const float mx = startX(W) + SLOT_SIZE / 2;
    const float my = START_Y + SLOT_SIZE / 2;
    CHECK(casillaBajoCursor(mx, my, W, 3, 1000).indice == 27);
}

TEST_CASE("Casilla: entre casillas y fuera del inventario no hay nada") {
    const float W = 1280.0f;
    const float x0 = startX(W);
    // En el hueco entre la primera y la segunda columna.
    CHECK(casillaBajoCursor(x0 + SLOT_SIZE + SPACING / 2, START_Y + 10, W, 0, 1000).zona
          == Zona::NINGUNA);
    // En la esquina de la pantalla.
    CHECK(casillaBajoCursor(2, 2, W, 0, 1000).zona == Zona::NINGUNA);
}

TEST_CASE("Casilla: un slot que no existe no se detecta") {
    // Inventario corto: solo 10 slots. La casilla 12 cae en la ventana pero
    // no existe, asi que no debe dar etiqueta.
    const float W = 1280.0f;
    const float mx = startX(W) + 3 * (SLOT_SIZE + SPACING) + SLOT_SIZE / 2;
    const float my = START_Y + 1 * (SLOT_SIZE + SPACING) + SLOT_SIZE / 2;
    CHECK(casillaBajoCursor(mx, my, W, 0, 10).zona == Zona::NINGUNA);
}

TEST_CASE("Casilla: la rejilla de crafteo y el resultado tambien") {
    const float W = 1280.0f;
    const float cx = craftStartX(W);
    const Casilla c4 = casillaBajoCursor(
        cx + 1 * (CRAFT_SLOT_SIZE + CRAFT_SPACING) + 10,
        CRAFT_START_Y + 1 * (CRAFT_SLOT_SIZE + CRAFT_SPACING) + 10, W, 0, 1000);
    CHECK(c4.zona == Zona::CRAFTEO);
    CHECK(c4.indice == 4);   // la del centro

    const Casilla r = casillaBajoCursor(resultadoX(W) + 10, RESULTADO_Y + 10, W, 0, 1000);
    CHECK(r.zona == Zona::RESULTADO);
}

// ----------------------------------------------------------------------------
// DONDE VA LA ETIQUETA
// ----------------------------------------------------------------------------
TEST_CASE("Etiqueta: va DEBAJO de la casilla, sin taparla") {
    // "una mini seccion abajo": empieza donde acaba la casilla.
    Casilla c; c.x = 400; c.y = 200; c.lado = SLOT_SIZE;
    const Etiqueta e = colocarEtiqueta(c, 10, 1280, 720);
    CHECK(e.y >= c.y + c.lado);
    // Centrada con la casilla.
    CHECK(e.x + e.ancho / 2 == doctest::Approx(c.x + c.lado / 2));
}

TEST_CASE("Etiqueta: el ancho se ajusta al nombre") {
    Casilla c; c.x = 400; c.y = 200; c.lado = SLOT_SIZE;
    const Etiqueta corta = colocarEtiqueta(c, 4, 1280, 720);
    const Etiqueta larga = colocarEtiqueta(c, 30, 1280, 720);
    CHECK(larga.ancho > corta.ancho);
    // Y cabe el texto con su margen.
    CHECK(larga.ancho >= anchoTexto(30, TOOLTIP_TEXTO));
}

TEST_CASE("Etiqueta: nunca se sale de la pantalla") {
    // El nombre mas largo de la tabla, en las cuatro esquinas.
    size_t maxLen = 0;
    for (int i = 1; i <= BLOCK_TYPE_MAX; ++i)
        if (const char* n = nombreDeItem((BlockType)i))
            if (std::strlen(n) > maxLen) maxLen = std::strlen(n);

    const float W = 1280, H = 720;
    const float esquinas[4][2] = { {0, 0}, {W - SLOT_SIZE, 0},
                                   {0, H - SLOT_SIZE}, {W - SLOT_SIZE, H - SLOT_SIZE} };
    for (const auto& p : esquinas) {
        Casilla c; c.x = p[0]; c.y = p[1]; c.lado = SLOT_SIZE;
        const Etiqueta e = colocarEtiqueta(c, (int)maxLen, W, H);
        INFO("casilla en ", p[0], ",", p[1]);
        CHECK(e.x >= 0);
        CHECK(e.y >= 0);
        CHECK(e.x + e.ancho <= W);
        CHECK(e.y + e.alto <= H);
    }
}

TEST_CASE("Etiqueta: sin sitio abajo, sube encima de la casilla") {
    Casilla c; c.x = 400; c.y = 720 - SLOT_SIZE; c.lado = SLOT_SIZE;
    const Etiqueta e = colocarEtiqueta(c, 10, 1280, 720);
    CHECK(e.y + e.alto <= c.y);
}
