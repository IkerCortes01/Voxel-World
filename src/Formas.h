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
constexpr int NUM_MATERIALES = 43;
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
        // ⭐ PEDIDO: "que todos los bloques, no items, tengan la capacidad de
        // ser modificados". El resto de bloques enteros del juego: los
        // troncos y tablones que faltaban, las hojas de todos los arboles,
        // los techos, la nieve, el vidrio y el mineral.
        // ⚠️ SE ANADEN AL FINAL: el numero de cada material va guardado en las
        // piezas ya puestas en los mundos, y no puede cambiar.
        BLOCK_WOOD_OCOTE_CHINO, BLOCK_WOOD_PATULA, BLOCK_WOOD_PINONERO, BLOCK_WOOD_ALTURAS,
        BLOCK_WOOD_PSEUDOSTROBUS, BLOCK_PLANKS_ALTURAS,
        BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL, BLOCK_LEAVES_OCOTE,
        BLOCK_LEAVES_OCOTE_CHINO, BLOCK_LEAVES_PATULA, BLOCK_LEAVES_PINONERO, BLOCK_LEAVES_ALTURAS,
        BLOCK_LEAVES_PSEUDOSTROBUS,
        BLOCK_TECHO_HOJAS, BLOCK_TECHO_SUELO,
        BLOCK_SNOW, BLOCK_GLASS, BLOCK_IRON_ORE, BLOCK_BEDROCK,
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
// ============================================================================
// ⭐⭐ EL ESTADO DE UNA PIEZA: MATERIAL, FORMA Y ORIENTACION
// ============================================================================
// PEDIDO: "que escaleras, paredes y miles de modificaciones de bloques mas,
// como en Minecraft, se coloquen con la cara viendo frente al jugador" (el
// sistema universal de orientacion), y "que al presionar clic derecho con el
// martillo cambies la orientacion de norte, sur, este, oeste, noreste,
// sureste, suroeste, noroeste".
//
// Los 16 bits del estado de la pieza:
//
//   bits 0..5    MATERIAL    de que bloque esta hecha (hasta 64)
//   bit  6       DIAGONAL    girada 45 grados mas (NE, SE, SO, NO)
//   bits 7..12   FORMA       cual de las 50
//   bits 13..14  GIRO        0 mira a +Z (sur), 1 a -X (oeste), 2 a -Z
//                            (norte), 3 a +X (este)
//   bit  15      INVERTIDA   cabeza abajo (la escalera colgada del techo)
//
// ⚠️ COMPATIBLE CON LO YA GUARDADO. Antes el material ocupaba los bits 0..6 y
// el 15 no se usaba; como nunca hubo mas de 22 materiales, en todas las piezas
// ya puestas los bits 6 y 15 estan a cero: se leen como "ni diagonal ni
// invertida", que es lo que eran. No hace falta migrar ningun mundo.
//
// Todo lo que orienta una pieza -- colocarla, la vista previa, el martillo --
// pasa por las funciones de aqui: no hay dos sitios que calculen el giro.
constexpr Compuesto::Campo C_MATERIAL  = { 0, 6 };
constexpr Compuesto::Campo C_DIAGONAL  = { 6, 1 };
constexpr Compuesto::Campo C_FORMA     = { 7, 6 };
constexpr Compuesto::Campo C_GIRO      = { 13, 2 };
constexpr Compuesto::Campo C_INVERTIDA = { 15, 1 };

inline bool esPieza(BlockType t) {
    return Compuesto::esCompuesto(t) && Compuesto::familiaDe(t) == Compuesto::FAM_FORMA;
}
inline BlockType pieza(int material, int formaIdx, int giro = 0, bool invertida = false, bool diagonal = false) {
    uint16_t e = 0;
    e = Compuesto::escribir(e, C_MATERIAL, (uint16_t)material);
    e = Compuesto::escribir(e, C_FORMA, (uint16_t)formaIdx);
    e = Compuesto::escribir(e, C_GIRO, (uint16_t)(giro & 3));
    e = Compuesto::escribir(e, C_INVERTIDA, (uint16_t)(invertida ? 1 : 0));
    e = Compuesto::escribir(e, C_DIAGONAL, (uint16_t)(diagonal ? 1 : 0));
    return Compuesto::hacer(Compuesto::FAM_FORMA, e);
}
inline int materialDePieza(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_MATERIAL); }
inline int formaDePieza(BlockType t) {
    const int f = Compuesto::leer(Compuesto::estadoDe(t), C_FORMA);
    return f < NUM ? f : 0;
}
inline int giroDePieza(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_GIRO); }
inline bool invertidaDe(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_INVERTIDA) != 0; }
inline bool diagonalDe(BlockType t) { return Compuesto::leer(Compuesto::estadoDe(t), C_DIAGONAL) != 0; }
// La misma pieza con ese giro y NADA mas: ni invertida ni en diagonal. Es
// como se guarda en el inventario (conGiro(t, 0)): la orientacion se decide
// al colocarla, no viaja con el item.
inline BlockType conGiro(BlockType t, int giro) {
    return pieza(materialDePieza(t), formaDePieza(t), giro);
}
// La misma pieza con la orientacion entera.
inline BlockType conOrientacion(BlockType t, int giro, bool invertida, bool diagonal) {
    return pieza(materialDePieza(t), formaDePieza(t), giro, invertida, diagonal);
}
// El bloque del que esta hecha (su textura, su sonido, lo que es).
inline BlockType materialBloque(BlockType t) {
    const BlockType b = bloqueDeMaterial(materialDePieza(t));
    return b == BLOCK_AIR ? BLOCK_STONE : b;
}
// Una caja de la forma, ya girada y (si toca) cabeza abajo, de 0 a 1.
inline void cajaColocada(Caja c, int giro, bool invertida, float out[6]) {
    c = girar(c, giro);
    float y0 = c.y0 / 16.0f, y1 = c.y1 / 16.0f;
    if (invertida) { const float a = 1.0f - y1; y1 = 1.0f - y0; y0 = a; }
    out[0] = c.x0 / 16.0f; out[1] = y0; out[2] = c.z0 / 16.0f;
    out[3] = c.x1 / 16.0f; out[4] = y1; out[5] = c.z1 / 16.0f;
}
// Las cajas de una pieza ya girada e invertida, en fracciones de bloque
// (0..1). Devuelve cuantas escribe en `out` (hasta 4), cada una como
// x0,y0,z0,x1,y1,z1. (Sin el giro de 45 grados ni las uniones de escalera con
// las vecinas: para eso, cajasDePiezaEnMundo y cajasDeChoque, mas abajo.)
inline int cajasDePieza(BlockType t, float out[4][6]) {
    const Forma& f = forma(formaDePieza(t));
    const int g = giroDePieza(t);
    const bool inv = invertidaDe(t);
    for (int i = 0; i < f.n; ++i) cajaColocada(f.c[i], g, inv, out[i]);
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

// ⭐ EL GIRO SEGUN A DONDE SE MIRA. PEDIDO: "que dependiendo de donde vea el
// jugador, se colocara frente el bloque modificado".
//
// (dx, dz) es la direccion de la mirada en el plano del suelo -- el mismo rayo
// con el que se apunta al colocar --. La pieza queda DE FRENTE al jugador: la
// escalera sube alejandose de el, la pared queda al fondo, el alero vuela
// hacia donde mira. Se usa el rayo y no el angulo guardado del jugador porque
// el rayo es, exactamente, a donde se esta mirando en ese instante.
inline int giroDeMirada(float dx, float dz) {
    const float ax = dx < 0.0f ? -dx : dx, az = dz < 0.0f ? -dz : dz;
    if (az >= ax) return dz < 0.0f ? 2 : 0;       // mira a -Z : a +Z
    return dx < 0.0f ? 1 : 3;                     // mira a -X : a +X
}

// Lo que mide de alto una pieza: hasta donde llega su caja mas alta (0..1).
inline float altoDePieza(BlockType t) {
    float c[4][6];
    const int n = cajasDePieza(t, c);
    float y = 0.0f;
    for (int i = 0; i < n; ++i) if (c[i][4] > y) y = c[i][4];
    return y;
}

// ============================================================================
// ⭐ COLOCAR: LA ORIENTACION SALE DE LA MIRADA Y DE DONDE SE APUNTA
// ============================================================================
// UNA sola funcion, para colocar y para la vista previa (asi lo que se ve
// antes es lo que queda despues):
//
//   - El GIRO, de la mirada (giroDeMirada): la pieza queda de frente. La
//     escalera sube alejandose de quien la pone y la pared queda al fondo.
//   - INVERTIDA o no, de la cara a la que se apunta, como en Minecraft:
//         sobre la cara de ARRIBA de un bloque ........ normal
//         contra la cara de ABAJO (el techo) .......... invertida
//         contra una cara de LADO: segun la altura del punto apuntado --
//         en la mitad de arriba, invertida; en la de abajo, normal.
//
// `normalY` es la de la cara tocada (+1 arriba, -1 abajo, 0 de lado) y
// `alturaImpacto` la altura del punto apuntado dentro de su casilla (0..1).
// El umbral lleva un margen (0,5) fijo: mover un pelo la camara no la voltea
// salvo que se cruce de verdad la mitad.
inline bool invertidaAlColocar(int normalY, float alturaImpacto) {
    if (normalY > 0) return false;
    if (normalY < 0) return true;
    return alturaImpacto > 0.5f;
}
inline BlockType alColocar(BlockType t, float miradaX, float miradaZ, int normalY, float alturaImpacto) {
    return conOrientacion(t, giroDeMirada(miradaX, miradaZ), invertidaAlColocar(normalY, alturaImpacto), false);
}

// ============================================================================
// ⭐ LAS OCHO ORIENTACIONES Y EL MARTILLO
// ============================================================================
// En el orden pedido: norte, sur, este, oeste, noreste, sureste, suroeste,
// noroeste. El norte es -Z, como en Minecraft. Las cuatro diagonales son la
// cardinal de al lado girada 45 grados mas (bit DIAGONAL).
constexpr int NUM_ORIENTACIONES = 8;
inline const char* nombreDeOrientacion(int o) {
    static const char* const N[NUM_ORIENTACIONES] = { "NORTE", "SUR", "ESTE", "OESTE",
                                                       "NORESTE", "SURESTE", "SUROESTE", "NOROESTE" };
    return o >= 0 && o < NUM_ORIENTACIONES ? N[o] : "";
}
// (giro, diagonal) de cada una. Un giro son 90 grados en el sentido
// sur -> oeste -> norte -> este; la diagonal suma 45 en ese mismo sentido.
inline void giroDeOrientacion(int o, int& giro, bool& diagonal) {
    static const int G[NUM_ORIENTACIONES] = { 2, 0, 3, 1,   2, 3, 0, 1 };
    giro = G[o & 7];
    diagonal = (o & 7) >= 4;
}
inline int orientacionDe(BlockType t) {
    const int g = giroDePieza(t);
    if (!diagonalDe(t)) { static const int O[4] = { 1, 3, 0, 2 }; return O[g]; }       // sur, oeste, norte, este
    static const int OD[4] = { 6, 7, 4, 5 };                                           // SO, NO, NE, SE
    return OD[g];
}
inline BlockType conOrientacionN(BlockType t, int o) {
    int g = 0; bool d = false;
    giroDeOrientacion(o, g, d);
    return conOrientacion(t, g, invertidaDe(t), d);
}
// Un golpe de martillo: la orientacion siguiente de la lista (y de la ultima,
// otra vez a la primera). Lo demas de la pieza -- material, forma, si esta
// invertida -- no cambia.
inline BlockType siguienteOrientacion(BlockType t) {
    return conOrientacionN(t, (orientacionDe(t) + 1) % NUM_ORIENTACIONES);
}

// Girar un punto 45 grados alrededor del centro de la casilla, en el mismo
// sentido que `girar`. Es lo que hace el dibujo con cada vertice de una
// pieza en diagonal.
inline void puntoEnDiagonal(float& x, float& z) {
    constexpr float K = 0.70710678f;
    const float dx = x - 0.5f, dz = z - 0.5f;
    x = 0.5f + K * dx - K * dz;
    z = 0.5f + K * dx + K * dz;
}

// ============================================================================
// ⭐ ESCALERAS QUE SE UNEN EN LAS ESQUINAS
// ============================================================================
// Como en Minecraft: dos escaleras que forman esquina cambian de forma solas.
// NO se guarda nada: la forma se CALCULA cada vez a partir de la pieza y de
// sus dos vecinas (la de delante y la de detras), asi que al quitar una, la
// otra vuelve a ser recta sin que nadie la toque.
//
// Vale para las escaleras de peldanos completos (la de 2, la de 3 y la de 4):
// las tres con la MISMA regla; el numero de peldanos solo cambia la geometria.
// Se unen dos del mismo numero de peldanos y de la misma mitad (las dos
// normales o las dos invertidas); el material da igual. Las piezas en
// diagonal no se unen.
enum Union { RECTA = 0, EXTERIOR_IZQ, EXTERIOR_DER, INTERIOR_IZQ, INTERIOR_DER };
inline bool escaleraQueSeUne(BlockType t) {
    return esPieza(t) && formaDePieza(t) <= 2 && !diagonalDe(t);
}
// Hacia donde SUBE una pieza con ese giro (su lado alto).
inline void direccionDeGiro(int giro, int& dx, int& dz) {
    static const int DX[4] = { 0, -1, 0, 1 }, DZ[4] = { 1, 0, -1, 0 };
    dx = DX[giro & 3]; dz = DZ[giro & 3];
}
// La "izquierda" de un giro, con la convencion de Minecraft (la del sur es el
// este): un cuarto de vuelta hacia atras.
inline int giroIzquierda(int giro) { return (giro + 3) & 3; }

// `get(dx, dy, dz)` da el bloque vecino, relativo a la pieza.
template <typename TGet>
Union unionDeEscalera(BlockType t, TGet get) {
    if (!escaleraQueSeUne(t)) return RECTA;
    const int g = giroDePieza(t), f = formaDePieza(t);
    const bool inv = invertidaDe(t);
    auto companera = [&](BlockType v) {
        return escaleraQueSeUne(v) && formaDePieza(v) == f && invertidaDe(v) == inv;
    };
    // ¿Puede tomar esa forma? Si al lado, en esa direccion, hay otra igual
    // que mira a lo mismo, las dos siguen rectas: son un tramo, no una esquina.
    auto puede = [&](int giroLado) {
        int ex = 0, ez = 0;
        direccionDeGiro(giroLado, ex, ez);
        const BlockType v = get(ex, 0, ez);
        return !companera(v) || giroDePieza(v) != g;
    };
    int fx = 0, fz = 0;
    direccionDeGiro(g, fx, fz);
    // La de DELANTE (hacia donde sube) manda la esquina EXTERIOR.
    const BlockType delante = get(fx, 0, fz);
    if (companera(delante)) {
        const int gd = giroDePieza(delante);
        if (((gd - g) & 1) != 0 && puede((gd + 2) & 3))
            return gd == giroIzquierda(g) ? EXTERIOR_IZQ : EXTERIOR_DER;
    }
    // La de DETRAS, la INTERIOR.
    const BlockType detras = get(-fx, 0, -fz);
    if (companera(detras)) {
        const int gd = giroDePieza(detras);
        if (((gd - g) & 1) != 0 && puede(gd))
            return gd == giroIzquierda(g) ? INTERIOR_IZQ : INTERIOR_DER;
    }
    return RECTA;
}

// Las cajas de una escalera con esa union, ya giradas e invertidas. En la
// forma base la escalera sube hacia +Z y su izquierda es +X; el peldano i
// empieza en z = z0 y ocupa de ahi al fondo.
//   EXTERIOR  el peldano se queda solo en su rincon (se recorta tambien a lo ancho)
//   INTERIOR  el peldano dobla la esquina: se le suma el tramo del otro lado
// Devuelve cuantas escribe (hasta 7 con la de 4 peldanos: `out` es de 8).
inline int cajasDeEscaleraUnida(BlockType t, Union u, float out[8][6]) {
    const Forma& f = forma(formaDePieza(t));
    const int g = giroDePieza(t);
    const bool inv = invertidaDe(t);
    int n = 0;
    for (int i = 0; i < f.n && n < 8; ++i) {
        Caja c = f.c[i];
        const uint8_t z0 = c.z0;                      // donde empieza este peldano
        if (u == EXTERIOR_IZQ)      c.x0 = z0;                        // solo desde ahi hacia +X
        else if (u == EXTERIOR_DER) c.x1 = (uint8_t)(16 - z0);        // solo hasta ahi desde -X
        cajaColocada(c, g, inv, out[n++]);
        if ((u == INTERIOR_IZQ || u == INTERIOR_DER) && z0 > 0 && n < 8) {
            Caja extra = c;
            extra.z0 = 0; extra.z1 = z0;              // el tramo de delante...
            if (u == INTERIOR_IZQ) { extra.x0 = z0; extra.x1 = 16; }             // ...en el lado de la izquierda
            else                   { extra.x0 = 0;  extra.x1 = (uint8_t)(16 - z0); }
            cajaColocada(extra, g, inv, out[n++]);
        }
    }
    return n;
}

// Las cajas de una pieza TAL COMO ESTA EN EL MUNDO: giradas, invertidas y, si
// es una escalera que hace esquina con sus vecinas, con la esquina. Es lo que
// se dibuja. (El giro de 45 grados lo aplica el dibujo vertice a vertice, con
// puntoEnDiagonal: estas cajas van sin el.) `out` es de 8.
template <typename TGet>
int cajasDePiezaEnMundo(BlockType t, TGet get, float out[8][6]) {
    const Union u = unionDeEscalera(t, get);
    if (u != RECTA) return cajasDeEscaleraUnida(t, u, out);
    float c[4][6];
    const int n = cajasDePieza(t, c);
    for (int i = 0; i < n; ++i) for (int k = 0; k < 6; ++k) out[i][k] = c[i][k];
    return n;
}

// Las cajas con las que se CHOCA y a las que se APUNTA. Las mismas que se
// dibujan; y si la pieza esta en diagonal -- una caja girada 45 grados no es
// una caja alineada --, cada una se trocea en TIRAS que siguen su contorno:
// se choca con la pared en diagonal, no con el cuadrado que la envuelve.
// Devuelve cuantas escribe, hasta `maximo`.
constexpr int TIRAS_DIAGONAL = 6;
template <typename TGet>
int cajasDeChoque(BlockType t, TGet get, float out[][6], int maximo) {
    float c[8][6];
    const int n = cajasDePiezaEnMundo(t, get, c);
    int m = 0;
    if (!diagonalDe(t)) {
        for (int i = 0; i < n && m < maximo; ++i, ++m) for (int k = 0; k < 6; ++k) out[m][k] = c[i][k];
        return m;
    }
    constexpr float K = 0.70710678f;
    for (int i = 0; i < n; ++i) {
        // La caja, respecto al centro: a lo ancho [a0, a1], a lo largo [b0, b1].
        const float a0 = c[i][0] - 0.5f, a1 = c[i][3] - 0.5f, b0 = c[i][2] - 0.5f, b1 = c[i][5] - 0.5f;
        for (int k = 0; k < TIRAS_DIAGONAL && m < maximo; ++k) {
            const float z0 = (float)k / TIRAS_DIAGONAL, z1 = (float)(k + 1) / TIRAS_DIAGONAL;
            const float Z = (z0 + z1) * 0.5f - 0.5f;            // el centro de la tira
            // Los X de esa tira que caen dentro de la caja girada.
            float x0 = (a0 - K * Z) / K, x1 = (a1 - K * Z) / K;
            const float y0 = (K * Z - b1) / K, y1 = (K * Z - b0) / K;
            if (y0 > x0) x0 = y0;
            if (y1 < x1) x1 = y1;
            if (x0 < -0.5f) x0 = -0.5f;
            if (x1 > 0.5f) x1 = 0.5f;
            if (x1 - x0 < 0.02f) continue;                      // la tira no toca la caja
            out[m][0] = x0 + 0.5f; out[m][1] = c[i][1]; out[m][2] = z0;
            out[m][3] = x1 + 0.5f; out[m][4] = c[i][4]; out[m][5] = z1;
            ++m;
        }
    }
    return m;
}


}  // namespace Formas
