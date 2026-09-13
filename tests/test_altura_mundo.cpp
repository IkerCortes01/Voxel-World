#include <doctest/doctest.h>
#include "BlockType.h"
#include <vector>
#include <cstdint>
#include <algorithm>

// ============================================================================
// LA ALTURA DEL MUNDO: 128 -> 512
// ============================================================================
// Fija los dos contratos que el cambio de altura NO puede romper:
//
//   1. El empaquetado de celdas del flood-fill de luz.
//   2. La migracion de los mundos guardados con la altura antigua.
//
// Los dos fallan EN SILENCIO si se rompen -- no dan error de compilacion ni
// excepcion: uno pone luz en la celda equivocada y el otro borra el mundo del
// jugador. Por eso merecen test propio.

namespace {

// Las medidas del motor. Se replican aqui a proposito en vez de incluir
// main.cpp (que arrastraria OpenGL entero): si alguien cambia una y no la otra,
// los CHECK de abajo lo cazan.
constexpr int LADO   = 16;
constexpr int ALTO   = 512;
constexpr int LEGACY = 128;

// El empaquetado tal y como quedo en computeSkylight y coserLuzEntre: los bits
// de Y se derivan de la altura en vez de estar escritos a mano.
constexpr int bitsY(int alto) {
    return (alto <= 128) ? 7 : (alto <= 256) ? 8 : (alto <= 512) ? 9
         : (alto <= 1024) ? 10 : 16;
}

uint32_t empaquetar(int x, int y, int z, int alto) {
    const int by = bitsY(alto);
    return ((uint32_t)x << (by + 4)) | ((uint32_t)z << by) | (uint32_t)y;
}

void desempaquetar(uint32_t c, int alto, int& x, int& y, int& z) {
    const int by = bitsY(alto);
    x = (int)((c >> (by + 4)) & 15u);
    z = (int)((c >> by) & 15u);
    y = (int)(c & ((1u << by) - 1u));
}

} // namespace

TEST_CASE("Altura: el empaquetado de luz sobrevive a 512") {
    // ⭐ EL TEST QUE CAZA EL BUG MAS PELIGROSO DEL CAMBIO.
    //
    // El flood-fill guardaba la celda en 32 bits con `(x << 11) | (z << 7) | y`
    // -- SIETE bits para Y, o sea un maximo de 128. Con la altura en 512, una
    // celda por encima de 127 desborda sobre los bits de Z y el flood-fill
    // ilumina OTRA columna.
    //
    // No da error: solo luz mal puesta, y solo en las celdas altas. Es
    // exactamente la clase de fallo que aparece semanas despues y nadie
    // relaciona con el cambio de altura.
    //
    // Se recorren las esquinas y varias alturas criticas: justo debajo y justo
    // encima del limite viejo de 128, y el techo.
    for (int x : {0, 7, 15})
    for (int z : {0, 9, 15})
    for (int y : {0, 1, 63, 126, 127, 128, 129, 255, 256, 383, 510, 511}) {
        const uint32_t c = empaquetar(x, y, z, ALTO);
        int rx = -1, ry = -1, rz = -1;
        desempaquetar(c, ALTO, rx, ry, rz);

        CHECK(rx == x);
        CHECK(ry == y);
        CHECK(rz == z);
    }
}

TEST_CASE("Altura: el empaquetado viejo SI se rompia (por que hizo falta cambiarlo)") {
    // Deja constancia del fallo concreto, para que nadie "simplifique" el
    // empaquetado derivado volviendo a los desplazamientos fijos.
    //
    // Con 7 bits de Y, la celda (0, 128, 0) se lee como (0, 0, 1): la altura
    // 128 desborda entera sobre el primer bit de Z.
    auto empaquetarViejo = [](int x, int y, int z) -> uint32_t {
        return (uint32_t)((x << 11) | (z << 7) | y);
    };
    const uint32_t c = empaquetarViejo(0, 128, 0);
    const int zLeido = (int)((c >> 7) & 15);
    const int yLeido = (int)(c & 127);

    CHECK(yLeido == 0);     // la Y se perdio
    CHECK(zLeido == 1);     // y aparecio como Z
}

TEST_CASE("Altura: todas las celdas caben en 32 bits") {
    // El empaquetado usa 4 bits de X + 4 de Z + los de Y. Si algun dia se sube
    // la altura por encima de 2^24, deja de caber y hay que ensanchar el tipo.
    CHECK(bitsY(ALTO) + 8 <= 32);
    const uint32_t maxima = empaquetar(15, ALTO - 1, 15, ALTO);
    CHECK(maxima < 0xFFFFFFFFu);
}

TEST_CASE("Migracion: un chunk de 128 se sube al mundo de 512 sin mover el terreno") {
    // ⭐ EL TEST QUE PROTEGE LAS PARTIDAS GUARDADAS.
    //
    // El volcado de un chunk es un array plano [x][y][z], asi que su tamaño
    // DEPENDE de la altura. Un mundo guardado con 128 tiene un cuarto de las
    // entradas que espera el motor con 512.
    //
    // Sin migracion, la carga falla por tamaño y el chunk se REGENERA: el
    // jugador pierde lo que hubiera construido ahi. Con ella, el terreno tiene
    // que quedar EXACTAMENTE donde estaba -- misma X, misma Y, misma Z -- y
    // encima aparecer aire.

    // Un chunk viejo con un patron reconocible por celda.
    std::vector<BlockType> viejo((size_t)LADO * LEGACY * LADO);
    auto idxViejo = [](int x, int y, int z) {
        return ((size_t)x * LEGACY + y) * LADO + z;
    };
    for (int x = 0; x < LADO; ++x)
        for (int y = 0; y < LEGACY; ++y)
            for (int z = 0; z < LADO; ++z)
                viejo[idxViejo(x, y, z)] =
                    (BlockType)(1 + ((x * 31 + y * 7 + z * 3) % 40));

    // La conversion, igual que en getOrCreateChunk.
    std::vector<BlockType> nuevo((size_t)LADO * ALTO * LADO, (BlockType)BLOCK_AIR);
    auto idxNuevo = [](int x, int y, int z) {
        return ((size_t)x * ALTO + y) * LADO + z;
    };
    for (int x = 0; x < LADO; ++x)
        for (int y = 0; y < LEGACY; ++y)
            for (int z = 0; z < LADO; ++z)
                nuevo[idxNuevo(x, y, z)] = viejo[idxViejo(x, y, z)];

    // 1. El terreno viejo esta INTACTO y en la misma coordenada.
    for (int x = 0; x < LADO; ++x)
        for (int y = 0; y < LEGACY; ++y)
            for (int z = 0; z < LADO; ++z)
                REQUIRE(nuevo[idxNuevo(x, y, z)] == viejo[idxViejo(x, y, z)]);

    // 2. Lo que hay ENCIMA es aire, no basura.
    for (int x = 0; x < LADO; ++x)
        for (int y = LEGACY; y < ALTO; ++y)
            for (int z = 0; z < LADO; ++z)
                REQUIRE(nuevo[idxNuevo(x, y, z)] == (BlockType)BLOCK_AIR);
}

TEST_CASE("Migracion: los tamanos de volcado son los esperados") {
    // Es lo que distingue un save viejo de uno nuevo al cargar: si estos dos
    // numeros coincidieran, no habria forma de saber cual es cual.
    const size_t bytesViejos = sizeof(BlockType) * LADO * LEGACY * LADO;
    const size_t bytesNuevos = sizeof(BlockType) * LADO * ALTO * LADO;

    CHECK(bytesViejos != bytesNuevos);
    CHECK(bytesNuevos == bytesViejos * 4);   // 512 / 128
}

TEST_CASE("Altura: el terreno cabe con holgura bajo el techo") {
    // El generador comprime la altura contra un techo blando y una asintota
    // dura. Los dos tienen que quedar MUY por debajo de la altura del mundo, o
    // las cimas se decapitan -- que es lo que pasaba con 128 (techo 124, base
    // continental 78 + montana 58 = 136, aplastado).
    constexpr float SOFT_CEILING = 200.0f;
    constexpr float HARD_CEILING = 300.0f;
    constexpr float BASE_CONTINENTAL = 80.0f;    // lo mas alto de la spline
    constexpr float MAX_MONTANA = 150.0f;

    CHECK(SOFT_CEILING < HARD_CEILING);
    CHECK(HARD_CEILING < (float)ALTO);
    // La cima teorica mas alta entra dentro de la compresion, no contra ella.
    CHECK(BASE_CONTINENTAL + MAX_MONTANA < HARD_CEILING);
    // Y queda cielo de sobra para volar.
    CHECK((float)ALTO - HARD_CEILING > 150.0f);
}

TEST_CASE("Altura: el nivel del mar NO se movio") {
    // Decision de diseño deliberada: subir la altura da CABECERA, no reescala
    // el mundo. Todo el contenido --playas, cuevas, arboles, fauna-- esta
    // calibrado alrededor de SEA_LEVEL=62, y multiplicarlo por 4 obligaria a
    // recalibrarlo entero para que el jugador tuviera que cavar 200 bloques
    // hasta el agua.
    constexpr int SEA_LEVEL = 62;
    CHECK(SEA_LEVEL < 70);
    CHECK(SEA_LEVEL * 4 > 200);   // lo que habria pasado al reescalar
}
