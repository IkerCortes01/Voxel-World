#include <doctest/doctest.h>
#include "render/BordeVecinos.h"

// ============================================================================
// LA COPIA DEL BORDE DE LOS VECINOS
// ============================================================================
// Es la pieza que hace posible mallar en un hilo de trabajo sin punteros
// colgantes. Lo que se verifica:
//
//   1. EQUIVALENCIA - responde lo MISMO que respondian los punteros
//   2. SEGURIDAD    - fuera de rango nunca revienta
//   3. AUSENCIA     - un vecino que no esta se distingue de uno vacio
//   4. COSTE        - la copia cabe en el presupuesto de memoria
//
// La 1 es la importante: si el snapshot respondiera distinto que el codigo
// viejo, la malla saldria distinta y apareceria una costura entre chunks.
// ============================================================================

using namespace Render;

namespace {

// Rellena una cara con un patron reconocible, para poder comprobar que cada
// consulta devuelve EXACTAMENTE la celda que toca y no su vecina.
void rellenarPatron(CaraVecina& cara, int base) {
    cara.reservar();
    for (int lado = 0; lado < BORDE_LADO; ++lado)
        for (int y = 0; y < BORDE_ALTO; ++y)
            cara.poner(lado, y, (BlockType)(base + lado * 1000 + y));
}

} // namespace

// ============================================================================
// 1. EQUIVALENCIA CON EL ACCESO POR PUNTERO
// ============================================================================

TEST_CASE("Borde: cada lado responde con SU vecino") {
    BordeVecinos b;
    rellenarPatron(b.norte, 10);
    rellenarPatron(b.sur,   20);
    rellenarPatron(b.este,  30);
    rellenarPatron(b.oeste, 40);

    // Norte es el vecino en +Z: se consulta con nz >= 16, y el indice de lado
    // es la X. Reproduce `northChunk->getBlock(nx, ny, nz - CHUNK_SIZE)`.
    CHECK(b.consultar(5, 7, 16) == (BlockType)(10 + 5 * 1000 + 7));

    // Sur es -Z: nz < 0, indice por X.
    CHECK(b.consultar(5, 7, -1) == (BlockType)(20 + 5 * 1000 + 7));

    // Este es +X: nx >= 16, indice por Z.
    CHECK(b.consultar(16, 7, 5) == (BlockType)(30 + 5 * 1000 + 7));

    // Oeste es -X: nx < 0, indice por Z.
    CHECK(b.consultar(-1, 7, 5) == (BlockType)(40 + 5 * 1000 + 7));
}

TEST_CASE("Borde: no se confunden los lados entre si") {
    // Si los indices estuvieran cruzados (X donde va Z), el mesher dibujaria
    // caras del bloque equivocado justo en la frontera -- que es como se ve
    // una costura entre chunks.
    BordeVecinos b;
    b.norte.reservar();
    b.sur.reservar();
    b.este.reservar();
    b.oeste.reservar();

    // Una sola marca en cada lado, en una posicion asimetrica (lado 3,
    // altura 90) para que un cruce de indices salte a la vista.
    b.norte.poner(3, 90, BLOCK_STONE);
    b.sur.poner(3, 90, BLOCK_DIRT);
    b.este.poner(3, 90, BLOCK_SAND);
    b.oeste.poner(3, 90, BLOCK_GRAVEL);

    CHECK(b.consultar(3, 90, 16) == BLOCK_STONE);    // norte
    CHECK(b.consultar(3, 90, -1) == BLOCK_DIRT);     // sur
    CHECK(b.consultar(16, 90, 3) == BLOCK_SAND);     // este
    CHECK(b.consultar(-1, 90, 3) == BLOCK_GRAVEL);   // oeste

    // Y en la posicion espejo NO hay nada: confirma que el indice no esta
    // invertido dentro del lado.
    CHECK(b.consultar(12, 90, 16) == BLOCK_AIR);
}

TEST_CASE("Borde: la esquina diagonal devuelve AIRE") {
    // Es lo que ya hacia el mesher, y a proposito: dibujar la cara evita
    // huecos visuales, y copiar las cuatro esquinas costaria mas de lo que
    // aporta. El comportamiento tiene que ser IDENTICO al de antes.
    BordeVecinos b;
    rellenarPatron(b.norte, 10);
    rellenarPatron(b.este,  30);

    // Los dos ejes fuera a la vez.
    CHECK(b.consultar(16, 50, 16) == BLOCK_AIR);
    CHECK(b.consultar(-1, 50, -1) == BLOCK_AIR);
    CHECK(b.consultar(16, 50, -1) == BLOCK_AIR);
    CHECK(b.consultar(-1, 50, 16) == BLOCK_AIR);
}

TEST_CASE("Borde: fuera del mundo por arriba o por abajo es AIRE") {
    // Mismo comportamiento que el bounds check vertical del mesher.
    BordeVecinos b;
    rellenarPatron(b.norte, 10);

    CHECK(b.consultar(5, -1, 16) == BLOCK_AIR);
    CHECK(b.consultar(5, BORDE_ALTO, 16) == BLOCK_AIR);
    CHECK(b.consultar(5, 9999, 16) == BLOCK_AIR);
}

// ============================================================================
// 2. SEGURIDAD: NADA DE ESTO PUEDE REVENTAR
// ============================================================================
// El snapshot se lee desde un hilo de trabajo. Un acceso fuera de rango aqui
// no es un bloque mal dibujado: es el juego cerrandose.

TEST_CASE("Borde: coordenadas absurdas no revientan") {
    BordeVecinos b;
    rellenarPatron(b.norte, 10);
    rellenarPatron(b.sur,   20);
    rellenarPatron(b.este,  30);
    rellenarPatron(b.oeste, 40);

    // Nada de esto debe leer fuera del vector.
    CHECK_NOTHROW(b.consultar(999, 50, 16));
    CHECK_NOTHROW(b.consultar(-999, 50, 5));
    CHECK_NOTHROW(b.consultar(5, 50, 999));
    CHECK_NOTHROW(b.consultar(-999, -999, -999));
    CHECK_NOTHROW(b.consultar(999, 999, 999));
}

TEST_CASE("Borde: un lado con indice fuera de rango da AIRE, no basura") {
    BordeVecinos b;
    rellenarPatron(b.norte, 10);

    // nz>=16 manda al norte, pero el indice de lado (nx) esta fuera.
    CHECK(b.norte.leer(-1, 50) == BLOCK_AIR);
    CHECK(b.norte.leer(BORDE_LADO, 50) == BLOCK_AIR);
    CHECK(b.norte.leer(5, -1) == BLOCK_AIR);
    CHECK(b.norte.leer(5, BORDE_ALTO) == BLOCK_AIR);
}

// ============================================================================
// 3. AUSENCIA: "NO ESTA" NO ES LO MISMO QUE "ESTA VACIO"
// ============================================================================

TEST_CASE("Borde: un vecino ausente devuelve AIRE") {
    BordeVecinos b;   // ninguno reservado

    CHECK_FALSE(b.norte.presente);
    CHECK(b.consultar(5, 50, 16) == BLOCK_AIR);
    CHECK(b.consultar(5, 50, -1) == BLOCK_AIR);
    CHECK(b.consultar(16, 50, 5) == BLOCK_AIR);
    CHECK(b.consultar(-1, 50, 5) == BLOCK_AIR);
}

TEST_CASE("Borde: completo() distingue los cuatro presentes") {
    BordeVecinos b;
    CHECK_FALSE(b.completo());

    b.norte.reservar();
    CHECK_FALSE(b.completo());
    b.sur.reservar();
    CHECK_FALSE(b.completo());
    b.este.reservar();
    CHECK_FALSE(b.completo());

    b.oeste.reservar();
    CHECK(b.completo());          // ahora si
}

TEST_CASE("Borde: un vecino presente pero de puro aire NO es lo mismo que ausente") {
    // Importa porque el mesher decide si esperar a los vecinos segun esto. Un
    // vecino de aire es un dato valido (se dibuja la cara); uno ausente
    // significa "todavia no ha cargado" (hay que esperar o reintentar).
    BordeVecinos b;
    b.norte.reservar();           // presente, todo AIR por defecto
    b.sur.reservar();
    b.este.reservar();
    b.oeste.reservar();

    CHECK(b.completo());
    CHECK(b.consultar(5, 50, 16) == BLOCK_AIR);   // el dato es aire...
    CHECK(b.norte.presente);                       // ...pero el vecino esta
}

TEST_CASE("Borde: limpiar deja los cuatro ausentes") {
    BordeVecinos b;
    rellenarPatron(b.norte, 10);
    rellenarPatron(b.sur,   20);
    rellenarPatron(b.este,  30);
    rellenarPatron(b.oeste, 40);
    REQUIRE(b.completo());

    b.limpiar();

    CHECK_FALSE(b.completo());
    CHECK_FALSE(b.norte.presente);
    CHECK(b.bytes() == 0);
}

// ============================================================================
// 4. COSTE EN MEMORIA
// ============================================================================
// Copiar en vez de apuntar cuesta memoria. Aqui se fija cuanta, para que el
// arreglo del puntero colgante no se convierta en un problema de RAM.

TEST_CASE("Borde: la copia de los cuatro lados cabe en el presupuesto") {
    BordeVecinos b;
    rellenarPatron(b.norte, 10);
    rellenarPatron(b.sur,   20);
    rellenarPatron(b.este,  30);
    rellenarPatron(b.oeste, 40);

    const size_t total = b.bytes();
    INFO("los cuatro lados = ", total, " bytes (",
         total / 1024, " KB) por chunk en vuelo");

    // Una loncha son BORDE_LADO x BORDE_ALTO celdas, y de cada una se copian
    // DOS cosas: el bloque (4 bytes) y su nivel de luz (1 byte). Las dos hacen
    // falta porque el mesher tiene dos funciones que leian los vecinos.
    const size_t porCara = (size_t)BORDE_LADO * BORDE_ALTO
                         * (sizeof(BlockType) + sizeof(uint8_t));
    CHECK(total == 4u * porCara);

    // ⭐ TOPE SUBIDO DE 64 KB A 192 KB AL SUBIR LA ALTURA DEL MUNDO.
    //
    // Este CHECK hizo su trabajo: al pasar la altura de 128 a 512 fallo
    // solo, avisando de que el coste se habia CUADRUPLICADO (40 -> 160 KB por
    // chunk en vuelo). Es exactamente para lo que existe.
    //
    // Se sube el tope en vez de rebajar la copia, y el razonamiento es este:
    //
    //   - Lo que importa es el total VIVO, no el de un chunk. Los chunks en
    //     vuelo son como mucho los que quepan en la cola de mallado (16) mas
    //     uno por worker (3): 19 x 160 KB = ~3 MB. Sobre un motor que gasta
    //     cientos de MB en chunks, es ruido.
    //
    //   - La alternativa seria copiar solo la franja de alturas que el chunk
    //     usa de verdad (la mayoria del mundo vertical es aire). Es una
    //     optimizacion real y esta anotada, pero complica la foto del borde --
    //     que es la pieza que hace SEGURO el mallado en hilos -- y no se toca
    //     algo asi para ahorrar 3 MB.
    //
    // 192 KB deja margen para una altura de 512 sin que el tope sea decorativo:
    // si alguien la subiera a 1024 sin pensar, este test volveria a saltar.
    CHECK(total <= 192u * 1024u);

    // Y el total vivo, que es el numero que de verdad importa.
    const size_t chunksEnVuelo = 19;   // cola de mallado (16) + 3 workers
    CHECK(total * chunksEnVuelo <= 4u * 1024u * 1024u);   // < 4 MB
}

TEST_CASE("Borde: copiar SOLO el borde y no el chunk entero") {
    // La razon de que el coste sea aceptable. Un chunk entero son
    // 16 x 128 x 16 bloques; el borde es una sola columna de cada vecino.
    BordeVecinos b;
    rellenarPatron(b.norte, 10);

    const size_t unaCara     = b.norte.bytes();
    const size_t chunkEntero = (size_t)BORDE_LADO * BORDE_ALTO * BORDE_LADO
                             * (sizeof(BlockType) + sizeof(uint8_t));

    INFO("una cara = ", unaCara, " bytes;  chunk entero = ", chunkEntero);
    CHECK(unaCara * 16 == chunkEntero);   // exactamente 1/16
}

// ============================================================================
// 5. LA LUZ TAMBIEN VIAJA EN LA FOTO
// ============================================================================
// El mesher tiene DOS funciones que leian los punteros de los vecinos:
// getNeighborBlockCached (que bloque hay) y faceLightLevel (cuanta luz tiene).
// Copiar solo los bloques habria dejado la mitad del problema en pie, y esa
// mitad cierra el juego igual.

TEST_CASE("Borde: cada lado devuelve SU luz") {
    BordeVecinos b;
    b.norte.reservar();
    b.sur.reservar();
    b.este.reservar();
    b.oeste.reservar();

    b.norte.ponerLuz(3, 90, 5);
    b.sur.ponerLuz(3, 90, 6);
    b.este.ponerLuz(3, 90, 7);
    b.oeste.ponerLuz(3, 90, 8);

    CHECK(b.consultarLuz(3, 90, 16) == 5);
    CHECK(b.consultarLuz(3, 90, -1) == 6);
    CHECK(b.consultarLuz(16, 90, 3) == 7);
    CHECK(b.consultarLuz(-1, 90, 3) == 8);
}

TEST_CASE("Borde: sin vecino, LUZ PLENA y no negro") {
    // Es lo que ya hacia el mesher, y a proposito: "iluminar de mas antes que
    // pintar negro un borde; el rebuild al integrarse el vecino lo corrige".
    // Un borde negro es muy visible; uno iluminado de mas, casi nada.
    BordeVecinos b;   // ninguno presente

    CHECK(b.consultarLuz(5, 50, 16) == 18);
    CHECK(b.consultarLuz(5, 50, -1) == 18);
    CHECK(b.consultarLuz(16, 50, 5) == 18);
    CHECK(b.consultarLuz(-1, 50, 5) == 18);
}

TEST_CASE("Borde: la luz respeta el techo y el fondo del mundo") {
    BordeVecinos b;
    b.norte.reservar();

    CHECK(b.consultarLuz(5, BORDE_ALTO, 16) == 18);   // sobre el mundo: cielo
    CHECK(b.consultarLuz(5, 9999, 16) == 18);
    CHECK(b.consultarLuz(5, -1, 16) == 0);            // bajo el mundo: negro
}

TEST_CASE("Borde: la luz aguanta coordenadas absurdas") {
    BordeVecinos b;
    b.norte.reservar();
    b.sur.reservar();
    b.este.reservar();
    b.oeste.reservar();

    CHECK_NOTHROW(b.consultarLuz(999, 50, 16));
    CHECK_NOTHROW(b.consultarLuz(-999, 50, 5));
    CHECK_NOTHROW(b.consultarLuz(5, 50, -999));
    CHECK_NOTHROW(b.consultarLuz(-999, -999, -999));
}
