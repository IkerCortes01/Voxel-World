#pragma once

// ============================================================================
// DONDE ESTA CADA CASILLA DEL INVENTARIO
// ============================================================================
// El inventario se dibuja en un sitio (el bucle principal) y se clica en otro
// (mouseButtonCallback), y los dos llevaban su propia copia de las medidas con
// un comentario de "debe coincidir con el renderizado". El tooltip de nombres
// es el tercer sitio que necesita saber que casilla hay bajo el raton, asi que
// las medidas pasan aqui y los tres las comparten: si un dia cambia el tamano
// de una casilla, se cambia una vez.
//
// Funciones puras (sin OpenGL ni estado del juego) para poder probarlas.

namespace InventarioLayout {

// --- La cuadricula del inventario: 9 columnas, ventana de 5 filas con scroll ---
constexpr int   COLS        = 9;
constexpr int   ROWS        = 5;
constexpr float SLOT_SIZE   = 50.0f;
constexpr float SPACING     = 5.0f;
constexpr float GRID_WIDTH  = COLS * (SLOT_SIZE + SPACING) - SPACING;
constexpr float START_Y     = 120.0f;

inline float startX(float anchoPantalla) {
    return (anchoPantalla - GRID_WIDTH) / 2.0f;
}

// --- La rejilla de crafteo 3x3, a la derecha del inventario ---
constexpr float CRAFT_SLOT_SIZE = 50.0f;
constexpr float CRAFT_SPACING   = 5.0f;
constexpr float CRAFT_GRID_SIZE = 3 * (CRAFT_SLOT_SIZE + CRAFT_SPACING) - CRAFT_SPACING;

inline float craftStartX(float anchoPantalla) {
    return startX(anchoPantalla) + GRID_WIDTH + 80.0f;
}
constexpr float CRAFT_START_Y = START_Y + 30.0f;

// --- La casilla del resultado, tras la flecha ---
inline float resultadoX(float anchoPantalla) {
    return craftStartX(anchoPantalla) + CRAFT_GRID_SIZE + 15.0f + 60.0f;
}
constexpr float RESULTADO_Y =
    CRAFT_START_Y + CRAFT_GRID_SIZE / 2.0f - CRAFT_SLOT_SIZE / 2.0f;

// ----------------------------------------------------------------------------
// QUE CASILLA HAY BAJO EL RATON
// ----------------------------------------------------------------------------
enum class Zona { NINGUNA, INVENTARIO, CRAFTEO, RESULTADO };

struct Casilla {
    Zona  zona   = Zona::NINGUNA;
    int   indice = -1;      // slot del inventario, o 0..8 del crafteo
    float x = 0, y = 0;     // esquina superior izquierda en pantalla
    float lado = 0;
};

inline bool dentro(float mx, float my, float x, float y, float lado) {
    return mx >= x && mx <= x + lado && my >= y && my <= y + lado;
}

// `scroll` es la primera fila visible; `totalSlots`, cuantos hay de verdad
// (una casilla mas alla no existe, aunque caiga en la ventana).
inline Casilla casillaBajoCursor(float mx, float my, float anchoPantalla,
                                 int scroll, int totalSlots) {
    Casilla c;

    const float x0 = startX(anchoPantalla);
    for (int fila = 0; fila < ROWS; ++fila)
        for (int col = 0; col < COLS; ++col) {
            const int slot = (scroll + fila) * COLS + col;
            if (slot >= totalSlots) continue;
            const float x = x0 + col * (SLOT_SIZE + SPACING);
            const float y = START_Y + fila * (SLOT_SIZE + SPACING);
            if (dentro(mx, my, x, y, SLOT_SIZE)) {
                c.zona = Zona::INVENTARIO; c.indice = slot;
                c.x = x; c.y = y; c.lado = SLOT_SIZE;
                return c;
            }
        }

    const float cx0 = craftStartX(anchoPantalla);
    for (int i = 0; i < 9; ++i) {
        const float x = cx0 + (i % 3) * (CRAFT_SLOT_SIZE + CRAFT_SPACING);
        const float y = CRAFT_START_Y + (i / 3) * (CRAFT_SLOT_SIZE + CRAFT_SPACING);
        if (dentro(mx, my, x, y, CRAFT_SLOT_SIZE)) {
            c.zona = Zona::CRAFTEO; c.indice = i;
            c.x = x; c.y = y; c.lado = CRAFT_SLOT_SIZE;
            return c;
        }
    }

    const float rx = resultadoX(anchoPantalla);
    if (dentro(mx, my, rx, RESULTADO_Y, CRAFT_SLOT_SIZE)) {
        c.zona = Zona::RESULTADO; c.indice = 0;
        c.x = rx; c.y = RESULTADO_Y; c.lado = CRAFT_SLOT_SIZE;
    }
    return c;
}

// ----------------------------------------------------------------------------
// DONDE VA LA ETIQUETA CON EL NOMBRE
// ----------------------------------------------------------------------------
// PEDIDO: "una mini seccion abajo que se sobrepondra del inventario, o sea
// tapara levemente lo que este atras".
//
// Va DEBAJO de la casilla, centrada con ella y pegada a su borde, de modo que
// tapa media fila de abajo y nunca la casilla que se esta mirando. Si no cabe
// abajo (ultima fila con la ventana pequena) sube encima; si se sale por un
// lado, se empuja hacia dentro. Nunca queda cortada por el borde.
constexpr float TOOLTIP_TEXTO   = 14.0f;  // tamano de letra (renderText)
constexpr float TOOLTIP_PAD_X   = 8.0f;
constexpr float TOOLTIP_PAD_Y   = 6.0f;
constexpr float TOOLTIP_SEPARA  = 4.0f;   // hueco entre casilla y etiqueta
constexpr float TOOLTIP_MARGEN  = 4.0f;   // distancia minima al borde

// Ancho del texto con la fuente del juego: cada caracter avanza 0.7*tamano
// (renderText), pero el ultimo solo ocupa su glifo, 0.6*tamano (renderChar).
inline float anchoTexto(int caracteres, float tamano) {
    if (caracteres <= 0) return 0.0f;
    return (caracteres - 1) * tamano * 0.7f + tamano * 0.6f;
}

struct Etiqueta { float x, y, ancho, alto; };

inline Etiqueta colocarEtiqueta(const Casilla& c, int caracteres,
                                float anchoPantalla, float altoPantalla) {
    Etiqueta e;
    e.ancho = anchoTexto(caracteres, TOOLTIP_TEXTO) + 2.0f * TOOLTIP_PAD_X;
    e.alto  = TOOLTIP_TEXTO + 2.0f * TOOLTIP_PAD_Y;

    e.x = c.x + c.lado * 0.5f - e.ancho * 0.5f;
    e.y = c.y + c.lado + TOOLTIP_SEPARA;

    // Sin sitio abajo: encima de la casilla.
    if (e.y + e.alto > altoPantalla - TOOLTIP_MARGEN)
        e.y = c.y - TOOLTIP_SEPARA - e.alto;

    // Empujar hacia dentro por los lados.
    if (e.x + e.ancho > anchoPantalla - TOOLTIP_MARGEN)
        e.x = anchoPantalla - TOOLTIP_MARGEN - e.ancho;
    if (e.x < TOOLTIP_MARGEN) e.x = TOOLTIP_MARGEN;
    if (e.y < TOOLTIP_MARGEN) e.y = TOOLTIP_MARGEN;
    return e;
}

} // namespace InventarioLayout
