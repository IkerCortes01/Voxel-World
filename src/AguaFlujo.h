#pragma once

#include "BloqueCompuesto.h"

// ============================================================================
// LAS REGLAS DEL AGUA, IGUALES QUE EN MINECRAFT
// ============================================================================
// PEDIDO: "que el agua sea de nuevo un bloque completo [...] si detecta siempre
// un bloque de aire se expandira ahi con un desnivel, [...] como en minecraft,
// el maximo de expansion es 7 bloques".
//
//   FUENTE   bloque completo. No se gasta ni se mueve: solo alimenta.
//   FLUJO    niveles 7 a 1, uno menos por cada bloque. Son los 7 de alcance.
//   CAIDA    columna completa. Al aterrizar vuelve a extenderse 7 bloques.
//
// El agua solo entra en el aire y en otra agua mas baja: cualquier otro bloque
// le corta el paso, tambien un nopal o una hoja de maguey.
//
// Sin mundo ni OpenGL: `get(dx,dy,dz)` lee relativo a la celda, asi que los
// tests ejecutan estas mismas reglas sobre una rejilla.
// ============================================================================

namespace AguaFlujo {

namespace A = Compuesto::Agua;

// Hasta donde se busca un hueco por el que bajar antes de elegir direccion.
constexpr int BUSQUEDA_HUECO = 4;

// Agua que se sostiene sola. Los rios y la orilla los pone el generador y son
// naturales aunque un mundo viejo los guardara con nivel bajo.
inline bool esFuente(BlockType b) {
    if (b == BLOCK_WATER) return true;
    if (!A::esAgua(b)) return false;
    if (A::esOrilla(b) || A::tieneCorriente(b)) return true;
    return A::nivelDe(b) >= A::LLENA && !A::estaCayendo(b);
}

// Cuanto alimenta a sus vecinas: fuente y caida 8, el flujo su nivel.
inline int fuerza(BlockType b) {
    if (esFuente(b)) return (int)A::LLENA;
    if (!A::esAgua(b)) return 0;
    if (A::estaCayendo(b)) return (int)A::LLENA;
    return (int)A::nivelDe(b);
}

// ¿Puede pasar el agua por aqui?
inline bool puedePasar(BlockType b) {
    return b == BLOCK_AIR || (A::esAgua(b) && !esFuente(b));
}

struct Escritura {
    int dx, dy, dz;
    BlockType tipo;
};

struct Plan {
    BlockType propia = BLOCK_AIR;   // en que queda esta celda
    Escritura esc[5];               // y a donde se extiende
    int n = 0;
};

constexpr int LADOS[4][2] = { {1, 0}, {-1, 0}, {0, 1}, {0, -1} };

// En que deberia estar esta celda segun lo que la rodea.
template <typename TGet>
BlockType estadoNuevo(TGet get) {
    const BlockType b = get(0, 0, 0);
    if (esFuente(b)) return b;

    // Con agua encima, cae.
    if (esAguaCualquiera(get(0, 1, 0))) return A::nuevo(A::LLENA, true);

    int fuentes = 0, mayor = 0;
    for (const auto& l : LADOS) {
        const BlockType v = get(l[0], 0, l[1]);
        if (esFuente(v)) ++fuentes;
        const int f = fuerza(v);
        if (f > mayor) mayor = f;
    }

    // Entre dos fuentes, y con apoyo, nace una fuente nueva.
    if (fuentes >= 2) {
        const BlockType abajo = get(0, -1, 0);
        if (esFuente(abajo) || (abajo != BLOCK_AIR && !A::esAgua(abajo)))
            return A::nuevo(A::LLENA);
    }

    const int nivel = mayor - 1;
    return nivel <= 0 ? BLOCK_AIR : A::nuevo((uint16_t)nivel);
}

// A cuantos pasos esta el hueco mas cercano, sin volver por donde se llego.
template <typename TGet>
int distanciaAlHueco(TGet get, int x, int z, int desde, int paso) {
    int mejor = 1000;
    for (int i = 0; i < 4; ++i) {
        if (desde >= 0 && i == (desde ^ 1)) continue;   // el lado opuesto
        const int nx = x + LADOS[i][0], nz = z + LADOS[i][1];
        if (!puedePasar(get(nx, 0, nz))) continue;
        if (puedePasar(get(nx, -1, nz))) return paso;
        if (paso < BUSQUEDA_HUECO) {
            const int d = distanciaAlHueco(get, nx, nz, i, paso + 1);
            if (d < mejor) mejor = d;
        }
    }
    return mejor;
}

// Cuanto le queda a cada lado. Solo se fluye hacia el hueco mas cercano; sin
// hueco a la vista, hacia todos.
template <typename TGet>
void ladosDeFlujo(TGet get, bool (&fluye)[4]) {
    int dist[4];
    int mejor = 1000;
    for (int i = 0; i < 4; ++i) {
        dist[i] = 2000;
        const int nx = LADOS[i][0], nz = LADOS[i][1];
        if (!puedePasar(get(nx, 0, nz))) continue;
        if (puedePasar(get(nx, -1, nz))) dist[i] = 0;
        else dist[i] = 1 + distanciaAlHueco(get, nx, nz, i, 1);
        if (dist[i] < mejor) mejor = dist[i];
    }
    for (int i = 0; i < 4; ++i)
        fluye[i] = dist[i] < 2000 && (mejor >= 1000 || dist[i] == mejor);
}

template <typename TGet>
void extenderALosLados(TGet get, BlockType propia, Plan& plan) {
    const int nivel = fuerza(propia) - 1;
    if (nivel <= 0) return;

    bool fluye[4];
    ladosDeFlujo(get, fluye);
    for (int i = 0; i < 4; ++i) {
        if (!fluye[i]) continue;
        const int nx = LADOS[i][0], nz = LADOS[i][1];
        const BlockType v = get(nx, 0, nz);
        const bool entra = v == BLOCK_AIR ||
            (A::esAgua(v) && !esFuente(v) && !A::estaCayendo(v) &&
             (int)A::nivelDe(v) < nivel);
        if (entra)
            plan.esc[plan.n++] = { nx, 0, nz, A::nuevo((uint16_t)nivel) };
    }
}

// Un paso de esta celda.
template <typename TGet>
Plan planear(TGet get) {
    Plan plan;
    plan.propia = estadoNuevo(get);
    if (plan.propia == BLOCK_AIR) return plan;

    int fuentes = 0;
    for (const auto& l : LADOS)
        if (esFuente(get(l[0], 0, l[1]))) ++fuentes;

    const BlockType abajo = get(0, -1, 0);

    // Primero abajo. Con hueco debajo ya no se extiende, salvo en medio de
    // tres fuentes o mas.
    if (abajo == BLOCK_AIR ||
        (A::esAgua(abajo) && !esFuente(abajo) && !A::estaCayendo(abajo))) {
        plan.esc[plan.n++] = { 0, -1, 0, A::nuevo(A::LLENA, true) };
        if (fuentes >= 3) extenderALosLados(get, plan.propia, plan);
        return plan;
    }

    // Sobre agua solo se extiende una fuente: el flujo se quedaria en la orilla
    // de un estanque en vez de cubrirlo por encima.
    if (!esAguaCualquiera(abajo) || esFuente(plan.propia))
        extenderALosLados(get, plan.propia, plan);
    return plan;
}

} // namespace AguaFlujo
