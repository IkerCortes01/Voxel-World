#pragma once

#include "BlockType.h"
#include "BloqueCompuesto.h"

#include <cmath>
#include <cstdint>

// ============================================================================
// ⭐ LAS FORMAS DE LA MESA DE REFINERIA
// ============================================================================
// PEDIDO: "este bloque, al interactuar con el [...] te dara opciones para
// modificarlo como quieras con diferentes usos y formas; bloques de madera y
// tierra y bloques completos en si te daran estas opciones: Escalera, Pared,
// Piso, Techo, Columna. Y tu creas 50 para cada item y bloque".
//
// Un bloque entero (tablones, tierra, piedra...) pasa por la mesa y sale
// convertido en una PIEZA con otra forma. Hay 50 formas, 10 de cada una de las
// cinco clases que se pidieron, y valen para CUALQUIER material de la lista:
// son 50 piezas por cada bloque.
//
// COMO ESTA HECHO. Una forma es una lista de hasta 4 CAJAS, en dieciseisavos
// de bloque. Con eso se hace todo: se dibuja (el mesher pinta cada caja con la
// textura del material), se choca (formaDeBloque da esas mismas cajas, asi
// que por una escalera se SUBE y bajo un techo se PASA) y se selecciona.
//
// La pieza es un bloque COMPUESTO (Compuesto::FAM_FORMA): en sus 16 bits van
// el material, la forma y hacia donde mira. No gasta entradas del enum y no
// hay que tocar nada para anadir materiales o cambiar una forma.
//
//     MATERIAL  7 bits  0..127  que bloque es (ver MATERIALES)
//     FORMA     6 bits  0..63   cual de las 50
//     GIRO      2 bits  0..3    cuartos de vuelta, segun mira quien la pone
//
// ⚠️ MATERIALES y FORMAS solo crecen POR EL FINAL: su posicion va guardada en
// los mundos.
//
// Sin motor: lo prueba tests/test_formas.cpp.
// ============================================================================

namespace Formas {

struct Caja { uint8_t x0, y0, z0, x1, y1, z1; };      // en dieciseisavos, de 0 a 16

enum Clase { ESCALERA, PARED, PISO, TECHO, COLUMNA, NUM_CLASES };
constexpr int POR_CLASE = 10;
constexpr int NUM = NUM_CLASES * POR_CLASE;           // 50

struct Forma {
    const char* nombre;
    int n;                // cuantas cajas
    Caja c[4];
};

inline const char* nombreDeClase(int clase) {
    static const char* const N[NUM_CLASES] = { "ESCALERA", "PARED", "PISO", "TECHO", "COLUMNA" };
    return clase >= 0 && clase < NUM_CLASES ? N[clase] : "";
}

// Las 50. Sin girar, "atras" es +Z: una escalera sube hacia +Z y una pared
// queda en el lado +Z de su celda.
inline const Forma& forma(int i) {
    static const Forma F[NUM] = {
        // ------------------------------ ESCALERA ------------------------------
        { "ESCALERA",             2, { {0,0,0,16,8,16}, {0,8,8,16,16,16} } },
        { "ESCALERA DE 3",        3, { {0,0,0,16,5,16}, {0,5,5,16,11,16}, {0,11,11,16,16,16} } },
        { "ESCALERA DE 4",        4, { {0,0,0,16,4,16}, {0,4,4,16,8,16}, {0,8,8,16,12,16}, {0,12,12,16,16,16} } },
        { "ESCALERA BAJA",        2, { {0,0,0,16,4,16}, {0,4,8,16,8,16} } },
        { "ESCALERA ESTRECHA",    2, { {4,0,0,12,8,16}, {4,8,8,12,16,16} } },
        { "ESCALERA DE ESQUINA",  2, { {0,0,0,16,8,16}, {8,8,8,16,16,16} } },
        { "ESCALERA DE RINCON",   3, { {0,0,0,16,8,16}, {0,8,8,16,16,16}, {8,8,0,16,16,8} } },
        { "PELDANO",              1, { {0,0,8,16,8,16} } },
        { "ESCALERA INVERTIDA",   2, { {0,8,0,16,16,16}, {0,0,8,16,8,16} } },
        { "RAMPA ESCALONADA",     4, { {0,0,0,16,2,16}, {0,2,4,16,4,16}, {0,4,8,16,6,16}, {0,6,12,16,8,16} } },
        // -------------------------------- PARED -------------------------------
        { "PARED",                1, { {0,0,12,16,16,16} } },
        { "PARED FINA",           1, { {0,0,14,16,16,16} } },
        { "PARED GRUESA",         1, { {0,0,8,16,16,16} } },
        { "PARED CENTRAL",        1, { {0,0,6,16,16,10} } },
        { "PARED EN ESQUINA",     2, { {0,0,12,16,16,16}, {12,0,0,16,16,12} } },
        { "MEDIA PARED",          1, { {0,0,12,16,8,16} } },
        { "MURETE",               1, { {0,0,5,16,8,11} } },
        { "PARED CON VENTANA",    4, { {0,0,12,16,5,16}, {0,11,12,16,16,16}, {0,5,12,4,11,16}, {12,5,12,16,11,16} } },
        { "PARED CON PUERTA",     3, { {0,0,12,4,16,16}, {12,0,12,16,16,16}, {4,12,12,12,16,16} } },
        { "ALMENA",               3, { {0,0,12,16,8,16}, {0,8,12,6,16,16}, {10,8,12,16,16,16} } },
        // -------------------------------- PISO --------------------------------
        { "PISO",                 1, { {0,0,0,16,4,16} } },
        { "PISO FINO",            1, { {0,0,0,16,2,16} } },
        { "MEDIO BLOQUE",         1, { {0,0,0,16,8,16} } },
        { "PISO GRUESO",          1, { {0,0,0,16,12,16} } },
        { "BALDOSA",              1, { {2,0,2,14,2,14} } },
        { "PISO A MEDIAS",        1, { {0,0,8,16,4,16} } },
        { "PISO EN ESQUINA",      1, { {8,0,8,16,4,16} } },
        { "PASARELA",             1, { {5,0,0,11,4,16} } },
        { "TARIMA",               3, { {0,6,0,16,8,16}, {0,0,0,2,6,16}, {14,0,0,16,6,16} } },
        { "ESCALON",              1, { {0,0,0,16,6,16} } },
        // -------------------------------- TECHO -------------------------------
        { "TECHO",                1, { {0,12,0,16,16,16} } },
        { "TECHO FINO",           1, { {0,14,0,16,16,16} } },
        { "MEDIO TECHO",          1, { {0,8,0,16,16,16} } },
        { "TECHO GRUESO",         1, { {0,4,0,16,16,16} } },
        { "TECHO INCLINADO",      2, { {0,8,0,16,12,8}, {0,12,8,16,16,16} } },
        { "ALERO",                1, { {0,12,8,16,16,16} } },
        { "VIGA",                 1, { {6,10,0,10,16,16} } },
        { "VIGAS CRUZADAS",       3, { {6,10,0,10,16,16}, {0,10,6,6,16,10}, {10,10,6,16,16,10} } },
        { "TECHO EN ESQUINA",     1, { {8,12,8,16,16,16} } },
        { "CUMBRERA",             2, { {0,8,0,16,12,16}, {4,12,0,12,16,16} } },
        // ------------------------------- COLUMNA ------------------------------
        { "COLUMNA",              1, { {4,0,4,12,16,12} } },
        { "COLUMNA FINA",         1, { {6,0,6,10,16,10} } },
        { "COLUMNA GRUESA",       1, { {2,0,2,14,16,14} } },
        { "POSTE",                1, { {7,0,7,9,16,9} } },
        { "COLUMNA CON BASE",     2, { {2,0,2,14,4,14}, {5,4,5,11,16,11} } },
        { "COLUMNA CON CAPITEL",  2, { {5,0,5,11,12,11}, {2,12,2,14,16,14} } },
        { "COLUMNA CLASICA",      3, { {2,0,2,14,3,14}, {5,3,5,11,13,11}, {2,13,2,14,16,14} } },
        { "MEDIA COLUMNA",        1, { {4,0,4,12,8,12} } },
        { "COLUMNA DE ESQUINA",   1, { {10,0,10,16,16,16} } },
        { "PILAR ADOSADO",        1, { {4,0,10,12,16,16} } },
    };
    return F[i >= 0 && i < NUM ? i : 0];
}
inline int claseDe(int formaIdx) { return formaIdx / POR_CLASE; }

// Una caja girada `giro` cuartos de vuelta alrededor del eje vertical.
inline Caja girar(Caja c, int giro) {
    for (int k = 0; k < (giro & 3); ++k) {
        // (x, z) -> (16 - z, x)
        const Caja o = c;
        c.x0 = (uint8_t)(16 - o.z1); c.x1 = (uint8_t)(16 - o.z0);
        c.z0 = o.x0; c.z1 = o.x1;
    }
    return c;
}

// Cuanto del bloque ocupa (de 4096) y cuantas piezas salen de un bloque: las
// que caben en el, hasta 8. De un bloque salen 4 pisos, pero solo 1 escalera.
inline int volumen(int formaIdx) {
    const Forma& f = forma(formaIdx);
    int v = 0;
    for (int i = 0; i < f.n; ++i)
        v += (f.c[i].x1 - f.c[i].x0) * (f.c[i].y1 - f.c[i].y0) * (f.c[i].z1 - f.c[i].z0);
    return v;
}
inline int rinde(int formaIdx) {
    const int v = volumen(formaIdx);
    const int n = v > 0 ? 4096 / v : 1;
    return n < 1 ? 1 : (n > 8 ? 8 : n);
}

// ---------------------------------------------------------------------------
// LOS MATERIALES: los bloques enteros que la mesa sabe trabajar.
// ---------------------------------------------------------------------------
constexpr int NUM_MATERIALES = 22;
inline BlockType bloqueDeMaterial(int m) {
    static const BlockType M[NUM_MATERIALES] = {
        // Madera: tablones y troncos.
        BLOCK_PLANKS, BLOCK_PLANKS_ENCINO, BLOCK_PLANKS_OYAMEL, BLOCK_PLANKS_OCOTE,
        BLOCK_PLANKS_PATULA, BLOCK_PLANKS_PINONERO, BLOCK_PLANKS_PSEUDOSTROBUS,
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OYAMEL, BLOCK_WOOD_OCOTE,
        // Tierra y sueltos.
        BLOCK_DIRT, BLOCK_CLAY_DIRT, BLOCK_CLAY, BLOCK_SAND, BLOCK_CLAY_SAND, BLOCK_GRAVEL,
        // Piedra.
        BLOCK_STONE, BLOCK_COBBLESTONE, BLOCK_LIMESTONE, BLOCK_BRICKS,
        // Pasto: se trabaja como tierra con su capa verde.
        BLOCK_GRASS,
    };
    return m >= 0 && m < NUM_MATERIALES ? M[m] : BLOCK_AIR;
}
inline int materialDe(BlockType b) {
    for (int m = 0; m < NUM_MATERIALES; ++m)
        if (bloqueDeMaterial(m) == b) return m;
    return -1;
}

// ---------------------------------------------------------------------------
// LA PIEZA COMO BLOQUE
// ---------------------------------------------------------------------------
constexpr Compuesto::Campo C_MATERIAL = { 0, 7 };
constexpr Compuesto::Campo C_FORMA    = { 7, 6 };
constexpr Compuesto::Campo C_GIRO     = { 13, 2 };

inline bool esPieza(BlockType t) {
    return Compuesto::esCompuesto(t) && Compuesto::familiaDe(t) == Compuesto::FAM_FORMA;
}
inline BlockType pieza(int material, int formaIdx, int giro = 0) {
    uint16_t e = 0;
    e = Compuesto::escribir(e, C_MATERIAL, (uint16_t)material);
    e = Compuesto::escribir(e, C_FORMA, (uint16_t)formaIdx);
    e = Compuesto::escribir(e, C_GIRO, (uint16_t)(giro & 3));
    return Compuesto::hacer(Compuesto::FAM_FORMA, e);
}
inline int materialDePieza(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_MATERIAL); }
inline int formaDePieza(BlockType t) {
    const int f = Compuesto::leer(Compuesto::estadoDe(t), C_FORMA);
    return f < NUM ? f : 0;
}
inline int giroDePieza(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_GIRO); }
inline BlockType conGiro(BlockType t, int giro) {
    return pieza(materialDePieza(t), formaDePieza(t), giro);
}
// El bloque del que esta hecha (su textura, su sonido, lo que es).
inline BlockType materialBloque(BlockType t) {
    const BlockType b = bloqueDeMaterial(materialDePieza(t));
    return b == BLOCK_AIR ? BLOCK_STONE : b;
}
// Las cajas de una pieza ya girada, en fracciones de bloque (0..1). Devuelve
// cuantas escribe en `out` (hasta 4), cada una como x0,y0,z0,x1,y1,z1.
inline int cajasDePieza(BlockType t, float out[4][6]) {
    const Forma& f = forma(formaDePieza(t));
    const int g = giroDePieza(t);
    for (int i = 0; i < f.n; ++i) {
        const Caja c = girar(f.c[i], g);
        out[i][0] = c.x0 / 16.0f; out[i][1] = c.y0 / 16.0f; out[i][2] = c.z0 / 16.0f;
        out[i][3] = c.x1 / 16.0f; out[i][4] = c.y1 / 16.0f; out[i][5] = c.z1 / 16.0f;
    }
    return f.n;
}
// Hacia donde mira quien la pone -> el giro. `yaw` en grados, como el del
// jugador (0 mira a -Z y crece hacia +X). La pieza queda "de frente": una
// escalera sube alejandose de quien la coloca.
inline int giroDeYaw(float yaw) {
    float a = std::fmod(yaw, 360.0f);
    if (a < 0.0f) a += 360.0f;
    const int cuadrante = (int)((a + 45.0f) / 90.0f) & 3;     // mira a 0: -Z, 1: -X, 2: +Z, 3: +X
    static const int G[4] = { 2, 1, 0, 3 };
    return G[cuadrante];
}

}  // namespace Formas
