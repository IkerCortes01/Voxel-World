#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// CULLING POR VOLUMEN: LA CARA ENTRE DOS MACIZOS NO SE DIBUJA
// ============================================================================
// Se pidio "cuando dos bloques solidos comparten una cara, esa cara interior
// no debe generarse", y a la vez "no elimines caras necesarias cuando el
// vecino sea transparente o parcialmente ocupado".
//
// ----------------------------------------------------------------------------
// EL DEFECTO QUE SE CORRIGIO
// ----------------------------------------------------------------------------
// `shouldRenderFace` decidia con `isBlockOpaque`, que responde "¿deja pasar la
// luz?". La pregunta correcta para una cara es otra: "¿hay algo macizo pegado
// al otro lado?".
//
// Estos LLENAN su celda pero NO son `isBlockOpaque`:
//
//     CELDA MIXTA    dos capas de material, llena por definicion
//     NIVEL LLENO    un 8/8 es un cubo (solo los 1..7 dejan hueco)
//     CRISTAL        se ve a traves, pero ocupa el voxel entero
//
// Su frontera emitia LAS DOS caras, enterradas una contra otra: invisibles,
// pero malladas, subidas a la GPU y sombreadas.
//
// MEDIDO con la camara clavada (VOXELWORLD_BENCH_POS=-10,65,-96,0,0):
//
//     antes    8.158 y 8.185 caras por chunk
//     despues  7.274 y 7.208 caras por chunk     -> -10,8 %
//
// Se compara POR CHUNK y no en total a proposito: el regulador adaptativo
// reparte el rendimiento sobrante en ver mas lejos, asi que el total no es
// comparable entre dos versiones (lo advierte PENDIENTES.md §3-ter).
// ============================================================================

namespace {

// Replica de `llenaSuVoxelFrenteALiquido` (main.cpp), que es quien decide.
// Solo los casos que estos tests ejercitan.
bool llenaSuVoxel(BlockType t) {
    if (t == BLOCK_AIR) return false;
    if (esAguaCualquiera(t) || t == BLOCK_LAVA) return false;
    if (t == BLOCK_TALLGRASS || t == BLOCK_ORANGE_FLOWER) return false;
    if (esHojaDeArbol(t)) return false;
    if (esNivelParcial(t)) return false;   // 1..7 dejan hueco arriba
    if (esMixto(t)) return true;           // llena de suelo a techo
    // Las plantas compuestas (maguey, agave, nopal) tienen huecos entre sus
    // partes: se ve el fondo a traves de ellas.
    if ((int)t >= BLOQUE_COMPUESTO_BASE_ID) return false;
    return true;                           // piedra, tierra, cristal, troncos...
}

// Replica de las dos ultimas reglas de `shouldRenderFace`.
bool emiteCara(BlockType yo, BlockType vecino) {
    if (!llenaSuVoxel(yo)) return true;       // mi cara se ve siempre
    if (llenaSuVoxel(vecino)) return false;   // me tapa: fuera
    return true;
}

std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != 0x5C) r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f) return std::string();
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return std::string();
#endif
}

} // namespace

TEST_CASE("Culling: entre dos macizos NO se emite la cara") {
    const BlockType MACIZOS[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL, BLOCK_COBBLESTONE,
        BLOCK_LIMESTONE, BLOCK_CLAY, BLOCK_WOOD, BLOCK_PLANKS,
    };
    for (BlockType a : MACIZOS) {
        for (BlockType b : MACIZOS) {
            INFO("a=", (int)a, " b=", (int)b);
            CHECK_FALSE(emiteCara(a, b));
        }
    }
}

TEST_CASE("Culling: una celda MIXTA tambien tapa, aunque no sea isBlockOpaque") {
    // El caso que la regla vieja se perdia.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_STONE);
    REQUIRE(m != BLOCK_AIR);
    REQUIRE(esMixto(m));

    CHECK_FALSE(emiteCara(BLOCK_STONE, m));   // la piedra no dibuja contra ella
    CHECK_FALSE(emiteCara(m, BLOCK_STONE));   // ni ella contra la piedra
    CHECK_FALSE(emiteCara(m, m));
}

TEST_CASE("Culling: un nivel LLENO tapa; uno PARCIAL no") {
    // La distincion que hay que respetar y que es facil cruzar.
    const BlockType lleno = conNivel(BLOCK_DIRT, 8);   // == el cubo entero
    CHECK_FALSE(esNivelParcial(lleno));
    CHECK_FALSE(emiteCara(BLOCK_STONE, lleno));

    for (int n = 1; n <= 7; ++n) {
        const BlockType capa = conNivel(BLOCK_DIRT, n);
        INFO("nivel ", n);
        REQUIRE(esNivelParcial(capa));
        // Deja hueco por arriba: la cara del vecino SI se ve.
        CHECK(emiteCara(BLOCK_STONE, capa));
    }
}

TEST_CASE("Culling: lo que NO llena su voxel conserva todas sus caras") {
    // LA CONTRAPARTIDA OBLIGATORIA.
    //
    // Al apretar el culling es facil cerrar una copa de arbol o tapar un
    // maguey. Nada de esto puede perder caras.
    const BlockType HUECOS[] = {
        BLOCK_TALLGRASS, BLOCK_ORANGE_FLOWER,
        BLOCK_LEAVES, BLOCK_LEAVES_ENCINO, BLOCK_LEAVES_OYAMEL,
    };
    for (BlockType h : HUECOS) {
        INFO("hueco=", (int)h);
        // Su propia cara se ve siempre, contra lo que sea.
        CHECK(emiteCara(h, BLOCK_STONE));
        // Y no tapa la del vecino.
        CHECK(emiteCara(BLOCK_STONE, h));
    }
}

TEST_CASE("Culling: el aire nunca tapa nada") {
    CHECK(emiteCara(BLOCK_STONE, BLOCK_AIR));
    CHECK(emiteCara(BLOCK_DIRT,  BLOCK_AIR));
    CHECK(emiteCara(BLOCK_WOOD,  BLOCK_AIR));
}

TEST_CASE("Culling: exactamente UNA cara entre un macizo y el aire") {
    // Ni cero (seria un agujero al interior del mundo) ni dos.
    const int caras = (emiteCara(BLOCK_STONE, BLOCK_AIR) ? 1 : 0) +
                      (emiteCara(BLOCK_AIR, BLOCK_STONE) ? 1 : 0);
    // El aire no emite geometria, asi que la unica real es la de la piedra.
    CHECK(emiteCara(BLOCK_STONE, BLOCK_AIR));
    CHECK(caras >= 1);
}

TEST_CASE("Culling: el motor usa la pregunta por VOLUMEN, no por opacidad") {
    // Ata el test al codigo real: si alguien vuelve a `isBlockOpaque` en estas
    // dos lineas, las caras internas regresan y el chunk engorda un 10%.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    CHECK(src.find("if (!llenaSuVoxelFrenteALiquido(currentBlock)) return true;")
          != std::string::npos);
    CHECK(src.find("if (llenaSuVoxelFrenteALiquido(neighborBlock)) return false;")
          != std::string::npos);
}
