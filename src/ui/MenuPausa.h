#pragma once

#include <algorithm>
#include <cmath>

// ============================================================================
// EL MENU DE PAUSA: 20 BOTONES EN PARED DE LADRILLO
// ============================================================================
// PEDIDO: "cambia totalmente el pausa del juego y que tenga 20 botones,
// ordenados como minecraft hace pero asi, uno arriba enmedio que sera el de
// Volver a jugar [...], otros 2 que se ordenaran como pared de ladrillo, uno
// sera opciones y otro sera de Estadisticas [...], otros 2 colocados abajo de
// esos uno dira Logros, que por ahora estara vacio, otro sera de Reportar bugs
// que estara vacia su seccion y los otros inventalos y dales funcion".
//
// Aqui vive SOLO la disposicion: que botones hay, en que fila va cada uno y
// que rectangulo ocupa segun el tamano de la ventana. El dibujo y lo que hace
// cada boton estan en main.cpp (necesitan el motor). Es la misma regla que en
// Opciones e InventarioLayout: UNA funcion para dibujar y para los clics, asi
// no se desalinean -- y sin OpenGL, para poder probarla sola.
//
// LA PARED: nueve filas de 1, 2, 2, 3, 2, 3, 2, 3 y 2 botones, todas del mismo
// ancho total. Una fila de 2 tiene la junta en el centro; una de 3, a un tercio
// y a dos tercios. Al alternarlas ninguna junta cae encima de otra, que es lo
// que hace que se lea como ladrillo y no como rejilla.
namespace MenuPausa {

enum Boton {
    VOLVER,                                   // fila 1
    OPCIONES, ESTADISTICAS,                   // fila 2
    LOGROS, BUGS,                             // fila 3
    GUARDAR, CARPETA_MUNDO, INFO_MUNDO,       // fila 4
    CONTROLES, CAPTURA,                       // fila 5
    VER_CAPTURAS, PANTALLA_COMPLETA, HUD,     // fila 6
    MUSICA, SIGUIENTE_CANCION,                // fila 7
    DEPURACION, RECARGAR_CHUNKS, ACERCA,      // fila 8
    SALIR_MENU, SALIR_JUEGO,                  // fila 9
    NUM
};
static_assert(NUM == 20, "el menu de pausa tiene 20 botones");

constexpr int NUM_FILAS = 9;
constexpr int POR_FILA[NUM_FILAS] = { 1, 2, 2, 3, 2, 3, 2, 3, 2 };

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
    bool contiene(float px, float py) const {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

struct Disposicion {
    Rect boton[NUM];
    float tituloY = 0;      // donde va "PAUSA"
    float avisoY = 0;       // la linea de aviso bajo la pared ("PARTIDA GUARDADA")
    float letra = 16;       // tamano de letra maximo de un boton
};

inline Disposicion disponer(int ancho, int alto) {
    Disposicion d;
    constexpr float TITULO = 58.0f, AVISO = 30.0f;
    const float W = (std::min)(820.0f, (std::max)(200.0f, ancho - 40.0f));
    // El paso de fila se encoge en ventanas bajas: las 9 filas caben siempre.
    float paso = (alto - 16.0f - TITULO - AVISO) / NUM_FILAS;
    paso = std::clamp(paso, 22.0f, 62.0f);
    // PEDIDO: "que en el menu de pausa esten un poco mas separados". La junta
    // era de 8 px fijos; ahora es un cuarto largo del paso (16 px a 720 de
    // alto) y se encoge con la ventana, para que el boton no se quede sin alto.
    const float JUNTA = std::clamp(paso * 0.27f, 6.0f, 16.0f);
    const float h = paso - JUNTA;
    const float total = TITULO + NUM_FILAS * paso + AVISO;
    const float y0 = (std::max)(8.0f, (alto - total) / 2.0f);
    const float x0 = (ancho - W) / 2.0f;

    d.tituloY = y0;
    d.letra = std::clamp(h * 0.42f, 8.0f, 18.0f);
    int b = 0;
    for (int f = 0; f < NUM_FILAS; ++f) {
        const int n = POR_FILA[f];
        const float y = y0 + TITULO + f * paso;
        if (n == 1) {                         // el de arriba: medio ancho, centrado
            d.boton[b++] = { x0 + W * 0.25f, y, W * 0.5f, h };
            continue;
        }
        const float w = (W - (n - 1) * JUNTA) / n;
        for (int i = 0; i < n; ++i) d.boton[b++] = { x0 + i * (w + JUNTA), y, w, h };
    }
    d.avisoY = y0 + TITULO + NUM_FILAS * paso + 4.0f;
    return d;
}

// Que boton hay bajo el cursor (-1 si ninguno).
inline int botonEn(const Disposicion& d, float x, float y) {
    for (int i = 0; i < NUM; ++i) if (d.boton[i].contiene(x, y)) return i;
    return -1;
}

// La letra que cabe en un boton: la de la disposicion, o menos si el rotulo es
// largo (cada caracter de la fuente del juego ocupa 0.7 de su tamano).
inline float letraQueCabe(const Rect& r, int caracteres, float maxima) {
    if (caracteres <= 0) return maxima;
    return (std::min)(maxima, (r.w - 12.0f) / (caracteres * 0.7f));
}

// ----------------------------------------------------------------------------
// Las pantallas de dentro (estadisticas, logros, controles...): un titulo, una
// lista que puede tener varias paginas, y abajo ANTERIOR / VOLVER / SIGUIENTE.
// ----------------------------------------------------------------------------
struct Listado {
    Rect volver, anterior, siguiente;
    float tituloY = 0;
    float x0 = 0, y0 = 0;        // esquina de la primera fila
    float anchoColumna = 0, pasoFila = 22, letra = 12;
    int filas = 1, columnas = 1, porPagina = 1, paginas = 1;
};

inline Listado listar(int ancho, int alto, int elementos) {
    Listado l;
    const float W = (std::min)(1040.0f, (std::max)(200.0f, ancho - 40.0f));
    l.columnas = W >= 880.0f ? 2 : 1;
    l.tituloY = (std::max)(8.0f, alto * 0.04f);
    l.y0 = l.tituloY + 56.0f;
    const float yBotones = alto - 62.0f;
    l.letra = 12.0f;
    l.pasoFila = 22.0f;
    l.filas = (std::max)(1, (int)std::floor((yBotones - 12.0f - l.y0) / l.pasoFila));
    l.porPagina = l.filas * l.columnas;
    l.paginas = (std::max)(1, (elementos + l.porPagina - 1) / l.porPagina);
    l.anchoColumna = (W - (l.columnas - 1) * 30.0f) / l.columnas;
    l.x0 = (ancho - W) / 2.0f;
    const float cx = ancho / 2.0f;
    l.volver    = { cx - 110.0f, yBotones, 220.0f, 44.0f };
    l.anterior  = { cx - 110.0f - 12.0f - 190.0f, yBotones, 190.0f, 44.0f };
    l.siguiente = { cx + 110.0f + 12.0f, yBotones, 190.0f, 44.0f };
    return l;
}

// La pagina, dentro de lo que hay (la rueda y los botones pasan por aqui).
inline int paginaValida(int pagina, int paginas) {
    return std::clamp(pagina, 0, (std::max)(0, paginas - 1));
}

} // namespace MenuPausa
