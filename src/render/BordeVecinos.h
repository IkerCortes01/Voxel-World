#pragma once

#include "../BlockType.h"
#include <vector>
#include <cstdint>

// ============================================================================
// LA COPIA DEL BORDE DE LOS CUATRO VECINOS
// ============================================================================
// RESPONSABILIDAD UNICA: guardar los bloques del BORDE de los cuatro chunks
// vecinos, para que el mallado pueda consultarlos sin tocar el mundo.
//
// No conoce World, no conoce OpenGL, no guarda punteros a nada. Por eso puede
// viajar a un hilo de trabajo, y por eso se puede probar sin arrancar el
// juego.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA QUE RESUELVE
// ----------------------------------------------------------------------------
// El mesher guardaba PUNTEROS CRUDOS a los cuatro chunks vecinos y los leia
// durante las ~6.500 lineas siguientes:
//
//     Chunk* northChunk = getChunk(northChunkPos);
//     ...
//     return northChunk->getBlock(nx, ny, nz - CHUNK_SIZE);
//
// Mientras el mallado corria en el hilo principal eso era seguro: nadie podia
// descargar un chunk a mitad de la funcion, porque el que descarga es el
// mismo hilo.
//
// En cuanto el mallado se va a un worker, deja de serlo. `updateChunks` hace
// `chunks.erase(pos)` y devuelve el chunk al pool desde el hilo principal, y
// ese pool RECICLA objetos: el puntero del worker no solo apunta a memoria
// liberada, apunta a memoria que ya es OTRO chunk. Se mallaria con datos de un
// sitio distinto del mundo, o se cerraria el juego.
//
// No es un riesgo teorico. Este motor ya sufrio exactamente esa clase de fallo
// con el cache de chunks (dos claves apuntando al mismo objeto, doble
// liberacion al descargar), y esta anotado en el registro de decisiones de
// PENDIENTES.md.
//
// ----------------------------------------------------------------------------
// POR QUE SOLO EL BORDE
// ----------------------------------------------------------------------------
// Copiar los cuatro chunks enteros serian 4 x 16 x 128 x 16 bloques. Pero el
// mesher NO los usa enteros: solo pregunta por vecinos de celdas que estan
// pegadas a la frontera, o sea UNA columna de cada vecino.
//
// Se comprobo leyendo las cuatro ramas de getNeighborBlockCached: la del norte
// solo accede a z=0 del vecino, la del sur a z=15, la del este a x=0 y la del
// oeste a x=15. Nada mas.
//
// Asi que se copia una loncha de 16 x 128 por lado. Con BlockType de 4 bytes
// son 8 KB por vecino, 32 KB por chunk en vuelo. Con dos workers, 64 KB. Es
// memoria de sobra a cambio de eliminar la clase de fallo entera.
//
// ----------------------------------------------------------------------------
// LO QUE SE PIERDE, Y POR QUE NO IMPORTA
// ----------------------------------------------------------------------------
// La copia es una FOTO: si el jugador rompe un bloque en el vecino mientras
// este chunk se malla, la malla sale con el estado anterior.
//
// No importa, porque romper un bloque ya marca `needsRebuild` en los chunks
// afectados, asi que el chunk se vuelve a mallar inmediatamente despues con la
// foto nueva. Lo peor que puede pasar es que una cara del borde tarde un frame
// mas en actualizarse -- y eso solo si el cambio ocurre justo durante el
// mallado.
// ============================================================================

namespace Render {

// Las mismas medidas del motor. Se declaran aqui para que este header no
// dependa de main.cpp; los static_assert del punto de uso verifican que
// coinciden.
constexpr int BORDE_LADO  = 16;
constexpr int BORDE_ALTO  = 128;

// ----------------------------------------------------------------------------
// LA LONCHA DE UN VECINO
// ----------------------------------------------------------------------------
// Una pared de 16 x 128 bloques: la cara del vecino que da a este chunk.
struct CaraVecina {
    // Vacia = ese vecino no existe (borde del mundo cargado). Se distingue de
    // "existe y es todo aire", que no es lo mismo: el mesher decide si dibujar
    // la cara de la frontera segun eso.
    bool presente = false;

    // [lado][altura]. `lado` recorre X o Z segun de que vecino se trate.
    std::vector<BlockType> bloques;

    // ⭐ LA LUZ TAMBIEN VIAJA EN LA FOTO.
    //
    // El mesher tiene DOS funciones que leen los vecinos, no una:
    // getNeighborBlockCached (que bloque hay) y faceLightLevel (cuanta luz
    // tiene). Las dos desreferenciaban los mismos punteros, asi que copiar
    // solo los bloques habria dejado la mitad del problema en pie -- y la
    // mitad que queda sigue cerrando el juego igual.
    //
    // Es el mismo nivel crudo 0-18 que guarda el chunk.
    std::vector<uint8_t> luz;

    void reservar() {
        bloques.assign((size_t)BORDE_LADO * BORDE_ALTO, BLOCK_AIR);
        luz.assign((size_t)BORDE_LADO * BORDE_ALTO, 18);
        presente = true;
    }

    void poner(int lado, int y, BlockType b) {
        if (lado < 0 || lado >= BORDE_LADO || y < 0 || y >= BORDE_ALTO) return;
        bloques[(size_t)lado * BORDE_ALTO + y] = b;
    }

    void ponerLuz(int lado, int y, uint8_t nivel) {
        if (lado < 0 || lado >= BORDE_LADO || y < 0 || y >= BORDE_ALTO) return;
        luz[(size_t)lado * BORDE_ALTO + y] = nivel;
    }

    // Fuera de rango devuelve AIRE, que es lo que ya hacia el mesher para
    // alturas invalidas.
    BlockType leer(int lado, int y) const {
        if (!presente) return BLOCK_AIR;
        if (lado < 0 || lado >= BORDE_LADO || y < 0 || y >= BORDE_ALTO)
            return BLOCK_AIR;
        return bloques[(size_t)lado * BORDE_ALTO + y];
    }

    // Sin dato, LUZ PLENA (18). Es lo que ya hacia el mesher, y a proposito:
    // iluminar de mas en un borde es mucho menos visible que pintarlo negro,
    // y el remallado al integrarse el vecino lo corrige.
    uint8_t leerLuz(int lado, int y) const {
        if (!presente) return 18;
        if (lado < 0 || lado >= BORDE_LADO || y < 0 || y >= BORDE_ALTO)
            return 18;
        return luz[(size_t)lado * BORDE_ALTO + y];
    }

    size_t bytes() const {
        return bloques.size() * sizeof(BlockType) + luz.size();
    }
};

// ----------------------------------------------------------------------------
// LOS CUATRO LADOS
// ----------------------------------------------------------------------------
struct BordeVecinos {
    CaraVecina norte;   // vecino en +Z: se copia su z=0
    CaraVecina sur;     // vecino en -Z: se copia su z=15
    CaraVecina este;    // vecino en +X: se copia su x=0
    CaraVecina oeste;   // vecino en -X: se copia su x=15

    // ¿Estaban los cuatro cuando se tomo la foto? El mesher lo usa para
    // decidir si construye ya o espera a que carguen (que es lo que evita las
    // costuras entre chunks).
    bool completo() const {
        return norte.presente && sur.presente &&
               este.presente && oeste.presente;
    }

    void limpiar() {
        norte = CaraVecina{};
        sur   = CaraVecina{};
        este  = CaraVecina{};
        oeste = CaraVecina{};
    }

    size_t bytes() const {
        return norte.bytes() + sur.bytes() + este.bytes() + oeste.bytes();
    }

    // ------------------------------------------------------------------------
    // LA CONSULTA QUE USA EL MESHER
    // ------------------------------------------------------------------------
    // Sustituye a las cuatro ramas de getNeighborBlockCached que leian
    // punteros. Recibe coordenadas LOCALES del chunk que se esta mallando, ya
    // desplazadas (o sea, pueden salirse de [0,16)).
    //
    // Devuelve el bloque del vecino que corresponda, o AIRE si no hay vecino.
    BlockType consultar(int nx, int ny, int nz) const {
        if (ny < 0 || ny >= BORDE_ALTO) return BLOCK_AIR;

        const bool fueraX = (nx < 0 || nx >= BORDE_LADO);
        const bool fueraZ = (nz < 0 || nz >= BORDE_LADO);

        // Esquina diagonal: los dos ejes fuera. El mesher ya devolvia AIRE
        // aqui a proposito -- dibujar la cara evita huecos visuales, y copiar
        // las cuatro esquinas costaria mas de lo que aporta.
        if (fueraX && fueraZ) return BLOCK_AIR;

        if (nz >= BORDE_LADO) return norte.leer(nx, ny);
        if (nz < 0)           return sur.leer(nx, ny);
        if (nx >= BORDE_LADO) return este.leer(nz, ny);
        if (nx < 0)           return oeste.leer(nz, ny);

        // No deberia llegarse aqui: si ninguno de los ejes esta fuera, la
        // celda es del propio chunk y no se pregunta por aqui. Se devuelve
        // PIEDRA por lo mismo que hacia el mesher: es el lado seguro (no
        // dibujar una cara de mas) ante coordenadas imposibles.
        return BLOCK_STONE;
    }

    // ------------------------------------------------------------------------
    // LA MISMA CONSULTA, PARA LA LUZ
    // ------------------------------------------------------------------------
    // Sustituye a las cuatro ramas de faceLightLevel que leian punteros.
    //
    // Devuelve 18 (luz plena) cuando no hay dato, que es exactamente lo que
    // hacia el mesher: "iluminar de mas antes que pintar negro un borde; el
    // rebuild al integrarse el vecino lo corrige".
    uint8_t consultarLuz(int nx, int ny, int nz) const {
        if (ny >= BORDE_ALTO) return 18;   // por encima del mundo: cielo
        if (ny < 0)           return 0;    // fondo del mundo

        if (nz >= BORDE_LADO) return norte.leerLuz(nx, ny);
        if (nz < 0)           return sur.leerLuz(nx, ny);
        if (nx >= BORDE_LADO) return este.leerLuz(nz, ny);
        if (nx < 0)           return oeste.leerLuz(nz, ny);

        return 18;
    }
};

} // namespace Render
