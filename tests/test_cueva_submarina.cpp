#include <doctest/doctest.h>
#include "terrain/ChunkGenerator.h"
#include <cstdio>
#include <vector>
#include <cstdlib>

// ============================================================================
// UNA CUEVA BAJO EL MAR ESTA LLENA DE AGUA
// ============================================================================
// LO QUE SE PIDIO: "siempre que hay una cueva bajo el mar, que este totalmente
// llena de agua".
//
// Antes no lo estaba. `ChunkGenerator` tallaba la cueva poniendo AIRE sin
// mirar si tenia oceano encima, asi que bajo el lecho marino quedaba una
// burbuja seca: se picaba el fondo del mar y aparecia una caverna vacia con el
// agua flotando arriba sin caer dentro.
//
// ----------------------------------------------------------------------------
// LA CONDICION ES `col.isOcean`, NO `y < SEA_LEVEL`
// ----------------------------------------------------------------------------
// Es la decision de diseño de este arreglo y merece quedar fijada, porque las
// dos parecen lo mismo y no lo son:
//
//   col.isOcean   -> esta COLUMNA tiene mar encima. Es lo que se pidio.
//   y < SEA_LEVEL -> cualquier cueva bajo la cota 62, incluidas las de tierra
//                    firme. Las cuevas de este motor bajan hasta y=8, asi que
//                    eso anegaria casi todo el subsuelo del mundo y acabaria
//                    con la exploracion en seco.
//
// Los dos primeros tests fijan justamente esa diferencia.
// ============================================================================

namespace {

using namespace TerrainGen;

constexpr int ALTO = 128;   // basta para cubrir SEA_LEVEL (62) con holgura

// Genera una columna entera y devuelve sus bloques.
std::vector<Blocks::Id> columna(ChunkGenerator& gen, int wx, int wz,
                                ColumnData* fuera = nullptr) {
    // El Writer del generador escribe en un array [SX][SY][SZ]; aqui basta
    // una columna de 1x1.
    static Blocks::Id buffer[1][ALTO][1];
    for (int y = 0; y < ALTO; ++y) buffer[0][y][0] = Blocks::AIR;

    ArrayChunkWriter<Blocks::Id, 1, ALTO, 1> writer(buffer);
    const ColumnData col = gen.GenerateColumn(writer, wx, wz, 0, 0, ALTO);
    if (fuera) *fuera = col;

    std::vector<Blocks::Id> out(ALTO);
    for (int y = 0; y < ALTO; ++y) out[y] = buffer[0][y][0];
    return out;
}

// Busca columnas de oceano y de tierra firme barriendo el mundo, y cuenta como
// quedan sus huecos bajo la superficie.
struct Recuento {
    int columnas = 0;
    int huecosAire = 0;
    int huecosAgua = 0;
};

Recuento recuentoHuecos(int seed, bool queremosOceano, int maxColumnas) {
    ChunkGenerator gen(seed);
    Recuento r;

    for (int i = 0; i < 24000 && r.columnas < maxColumnas; ++i) {
        // Barrido disperso para tocar terrenos variados sin recorrer el mundo.
        const int wx = (i * 37) % 3000 - 1500;
        const int wz = (i * 91) % 3000 - 1500;

        ColumnData col;
        const std::vector<Blocks::Id> c = columna(gen, wx, wz, &col);
        if (col.isOcean != queremosOceano) continue;

        // Solo interesan las columnas con cueva de verdad: se miran las celdas
        // por debajo de la superficie, que es donde talla IsCave.
        int surfaceY = col.surfaceHeight;
        if (surfaceY > ALTO - 1) surfaceY = ALTO - 1;

        int aire = 0, agua = 0;
        for (int y = 8; y < surfaceY - 6; ++y) {
            if (c[y] == Blocks::AIR)        ++aire;
            else if (c[y] == Blocks::WATER) ++agua;
        }
        if (aire + agua == 0) continue;   // esta columna no tiene cueva

        ++r.columnas;
        r.huecosAire += aire;
        r.huecosAgua += agua;
    }
    return r;
}

} // namespace

TEST_CASE("Cueva submarina: bajo el mar los huecos se llenan de agua") {
    const Recuento r = recuentoHuecos(12345, /*oceano=*/true, 60);

    REQUIRE(r.columnas > 10);   // que la muestra sea representativa
    printf("[CUEVA-MAR] oceano: %d columnas, agua=%d aire=%d\n",
           r.columnas, r.huecosAgua, r.huecosAire);

    // TODO hueco bajo el mar tiene que ser agua. Ni uno de aire: un solo hueco
    // seco es la burbuja que este arreglo elimina.
    CHECK(r.huecosAgua > 0);
    CHECK(r.huecosAire == 0);
}

TEST_CASE("Cueva submarina: en tierra firme la cueva se recorre en seco") {
    // ⚠️ LA MITAD QUE PROTEGE LA EXPLORACION.
    //
    // Si alguien cambia la condicion del mar a `y < SEA_LEVEL`, este test se
    // cae: las cuevas de tierra firme por debajo de la cota 62 se anegarian
    // enteras y el subsuelo dejaria de ser transitable.
    //
    // ⭐ NO se exige agua CERO, y es deliberado: bajo tierra hay lagos
    // subterraneos (ver CaveGenerator::NivelLagoEn), que son una mejora
    // buscada. Lo que se exige es que el aire SIGA MANDANDO -- o sea, que el
    // agua sea un accidente del fondo y no el estado normal de la cueva.
    const Recuento r = recuentoHuecos(12345, /*oceano=*/false, 60);

    REQUIRE(r.columnas > 10);
    printf("[CUEVA-MAR] tierra: %d columnas, agua=%d aire=%d\n",
           r.columnas, r.huecosAgua, r.huecosAire);

    CHECK(r.huecosAire > 0);
    // La mayor parte del hueco bajo tierra tiene que seguir siendo aire.
    CHECK(r.huecosAire > r.huecosAgua);
}

TEST_CASE("Lago subterraneo: la lamina es plana y esta acotada") {
    TerrainGen::CaveGenerator caves(4242);

    // La cota siempre dentro de su rango. Por encima del maximo el agua
    // asomaria por las bocas de cueva y se derramaria por la ladera.
    for (int i = 0; i < 400; ++i) {
        const float x = (float)((i * 71) % 4000 - 2000);
        const float z = (float)((i * 37) % 4000 - 2000);
        const int n = caves.NivelLagoEn(x, z);
        CHECK(n >= TerrainGen::CaveGenerator::NIVEL_LAGO_MIN);
        CHECK(n <= TerrainGen::CaveGenerator::NIVEL_LAGO_MAX);
    }

    // Y varia despacio: dos columnas vecinas no pueden tener cotas dispares,
    // o la lamina del lago saldria dentada en vez de plana.
    for (int i = 0; i < 200; ++i) {
        const float x = (float)(i * 13);
        const float z = (float)(i * 7);
        const int a = caves.NivelLagoEn(x, z);
        const int b = caves.NivelLagoEn(x + 1.0f, z);
        const int c = caves.NivelLagoEn(x, z + 1.0f);
        CHECK(std::abs(a - b) <= 1);
        CHECK(std::abs(a - c) <= 1);
    }
}

TEST_CASE("Cueva submarina: sigue siendo determinista") {
    // El mundo se regenera al cargar, asi que dos pasadas con la misma semilla
    // tienen que dar exactamente lo mismo -- agua incluida. Si la inundacion
    // dependiera de algo no determinista, un mundo guardado cambiaria al
    // volver a entrar.
    ChunkGenerator a(777), b(777);

    for (int i = 0; i < 120; ++i) {
        const int wx = (i * 53) % 900 - 450;
        const int wz = (i * 29) % 900 - 450;
        CHECK(columna(a, wx, wz) == columna(b, wx, wz));
    }
}
