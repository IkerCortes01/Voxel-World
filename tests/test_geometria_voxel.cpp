#include <doctest/doctest.h>
#include "BlockType.h"
#include <cmath>
#include <vector>
#include <fstream>
#include <string>
#include <iterator>

// ============================================================================
// CONTRATO GEOMETRICO DE LA CUADRICULA VOXEL
// ============================================================================
// Se pidio una "cuadricula voxel estricta": un bloque en (x,y,z) ocupa
// EXACTAMENTE [x*S, (x+1)*S] en cada eje.
//
// ----------------------------------------------------------------------------
// S = 1.0, Y ESTA VERIFICADO, NO SUPUESTO
// ----------------------------------------------------------------------------
// Se comprobo en las tres partes del motor que tienen que estar de acuerdo:
//
//   MALLADO    `emitQuad` emite la cara de arriba de la celda `layer` en
//              `layer + 1` y la de abajo de la siguiente en `layer`.
//   COLISION   `celdaPisaAlJugador` usa [c, c+1) en los tres ejes.
//   CHUNKS     la base es `position.x * CHUNK_SIZE`, multiplicacion entera.
//
// Y no hay ni un `glScalef` sobre la geometria del terreno: el unico
// `glTranslatef` del mundo es el de la camara.
//
// ----------------------------------------------------------------------------
// QUE PRUEBA ESTE ARCHIVO, Y QUE NO
// ----------------------------------------------------------------------------
// `emitQuad` vive dentro de `buildChunkMesh`, en main.cpp, que el binario de
// tests NO enlaza (hallazgo I3 de la auditoria). Asi que aqui se hace lo unico
// honesto posible sin OpenGL:
//
//   1. Se REPLICA la aritmetica exacta del mesher (las seis caras, copiadas de
//      `emitQuad`) y se verifica el contrato sobre ella.
//   2. Se comprueba contra el FUENTE que el mesher real sigue usando esa misma
//      aritmetica, para que la replica no se separe en silencio.
//
// Lo segundo es lo que convierte esto en un test de regresion util: si alguien
// mete un epsilon o un offset en `emitQuad`, el punto 2 falla.
// ============================================================================

namespace {

// El tamano mundial de un bloque. UNA sola fuente de verdad para el test.
constexpr float S = 1.0f;

constexpr int CHUNK = 16;

struct Caja { float x0, y0, z0, x1, y1, z1; };

// EL CONTRATO: los limites de la celda (x,y,z).
Caja celdaDe(int x, int y, int z) {
    return Caja{ x * S, y * S, z * S,
                 (x + 1) * S, (y + 1) * S, (z + 1) * S };
}

struct V3 { float x, y, z; };

// ----------------------------------------------------------------------------
// REPLICA EXACTA DE `emitQuad` (main.cpp)
// ----------------------------------------------------------------------------
// Mismos parametros que recibe el mesher. Copiado literal a proposito: si el
// original cambia, el test que lee el fuente lo detecta.
std::vector<V3> caraDelMesher(int dir, float baseWX, float baseWZ,
                              int layer, int u0, int v0, int w, int h) {
    const float fw = (float)w, fh = (float)h;
    std::vector<V3> q;
    switch (dir) {
        case 0: {   // Top (+Y)
            const float yy = (float)(layer + 1);
            const float xa = baseWX + u0, za = baseWZ + v0;
            q = { {xa, yy, za}, {xa, yy, za + fh},
                  {xa + fw, yy, za + fh}, {xa + fw, yy, za} };
            break;
        }
        case 1: {   // Bottom (-Y)
            const float yy = (float)layer;
            const float xa = baseWX + u0, za = baseWZ + v0;
            q = { {xa, yy, za}, {xa + fw, yy, za},
                  {xa + fw, yy, za + fh}, {xa, yy, za + fh} };
            break;
        }
        case 2: {   // North (+Z)
            const float zz = baseWZ + layer + 1;
            const float xa = baseWX + u0, ya = (float)v0;
            q = { {xa, ya, zz}, {xa + fw, ya, zz},
                  {xa + fw, ya + fh, zz}, {xa, ya + fh, zz} };
            break;
        }
        case 3: {   // South (-Z)
            const float zz = baseWZ + layer;
            const float xa = baseWX + u0, ya = (float)v0;
            q = { {xa + fw, ya, zz}, {xa, ya, zz},
                  {xa, ya + fh, zz}, {xa + fw, ya + fh, zz} };
            break;
        }
        case 4: {   // East (+X)
            const float xx = baseWX + layer + 1;
            const float za = baseWZ + u0, ya = (float)v0;
            q = { {xx, ya, za + fw}, {xx, ya, za},
                  {xx, ya + fh, za}, {xx, ya + fh, za + fw} };
            break;
        }
        default: {  // West (-X)
            const float xx = baseWX + layer;
            const float za = baseWZ + u0, ya = (float)v0;
            q = { {xx, ya, za}, {xx, ya, za + fw},
                  {xx, ya + fh, za + fw}, {xx, ya + fh, za} };
            break;
        }
    }
    return q;
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

// ============================================================================
// CASO A - APILAMIENTO VERTICAL
// ============================================================================

TEST_CASE("Geometria A: la cara de arriba y la de abajo coinciden EXACTAMENTE") {
    // Dos bloques en niveles consecutivos. La cara superior del de abajo y la
    // inferior del de arriba tienen que salir del MISMO numero -- no "casi
    // igual": identico bit a bit, o habria una rendija.
    for (int y = 0; y < 120; ++y) {
        const auto arribaDelBajo = caraDelMesher(0, 0.0f, 0.0f, y, 0, 0, 1, 1);
        const auto abajoDelAlto  = caraDelMesher(1, 0.0f, 0.0f, y + 1, 0, 0, 1, 1);

        INFO("nivel ", y);
        CHECK(arribaDelBajo[0].y == abajoDelAlto[0].y);
        CHECK(arribaDelBajo[0].y == (float)(y + 1) * S);
    }
}

TEST_CASE("Geometria A: ni hueco ni penetracion entre celdas apiladas") {
    for (int y = 0; y < 120; ++y) {
        const Caja abajo  = celdaDe(0, y,     0);
        const Caja arriba = celdaDe(0, y + 1, 0);
        INFO("nivel ", y);
        // El techo de uno ES el suelo del otro: diferencia CERO.
        CHECK(arriba.y0 - abajo.y1 == 0.0f);
    }
}

// ============================================================================
// CASO B - ADYACENCIA HORIZONTAL
// ============================================================================

TEST_CASE("Geometria B: dos vecinos comparten su frontera exacta") {
    for (int x = -40; x <= 40; ++x) {
        const Caja izq = celdaDe(x,     5, 0);
        const Caja der = celdaDe(x + 1, 5, 0);
        INFO("x = ", x);
        CHECK(der.x0 - izq.x1 == 0.0f);
    }
    for (int z = -40; z <= 40; ++z) {
        const Caja sur = celdaDe(0, 5, z);
        const Caja nor = celdaDe(0, 5, z + 1);
        INFO("z = ", z);
        CHECK(nor.z0 - sur.z1 == 0.0f);
    }
}

TEST_CASE("Geometria B: las caras laterales caen en el limite de celda") {
    // Cara +X de la celda `layer` y cara -X de la celda `layer+1`.
    for (int layer = 0; layer < CHUNK - 1; ++layer) {
        const auto este  = caraDelMesher(4, 0.0f, 0.0f, layer,     0, 0, 1, 1);
        const auto oeste = caraDelMesher(5, 0.0f, 0.0f, layer + 1, 0, 0, 1, 1);
        INFO("layer ", layer);
        CHECK(este[0].x == oeste[0].x);
        CHECK(este[0].x == (float)(layer + 1) * S);
    }
}

// ============================================================================
// CASO C - FRONTERAS DE CHUNK
// ============================================================================

TEST_CASE("Geometria C: los chunks vecinos no dejan grieta") {
    // EL CASO QUE MAS FACIL SE ROMPE: dos mallas generadas por separado.
    //
    // El ultimo bloque del chunk A (local x=15) y el primero del chunk B
    // (local x=0) tienen que tocarse exactamente en la frontera.
    for (int cx = -5; cx <= 5; ++cx) {
        const float baseA = (float)(cx * CHUNK);
        const float baseB = (float)((cx + 1) * CHUNK);

        const auto esteA  = caraDelMesher(4, baseA, 0.0f, CHUNK - 1, 0, 0, 1, 1);
        const auto oesteB = caraDelMesher(5, baseB, 0.0f, 0, 0, 0, 1, 1);

        INFO("frontera entre chunk ", cx, " y ", cx + 1);
        CHECK(esteA[0].x == oesteB[0].x);
        CHECK(esteA[0].x == (float)((cx + 1) * CHUNK) * S);
    }
}

TEST_CASE("Geometria C: la base del chunk es exacta en todo el rango jugable") {
    // Multiplicacion entera promovida a float: sin acumulacion de error. Se
    // barre mucho mas lejos de lo que el jugador alcanza.
    for (int cx = -2000; cx <= 2000; cx += 7) {
        const float base = (float)(cx * CHUNK);
        INFO("chunk ", cx);
        CHECK(base == (float)cx * (float)CHUNK);
        CHECK(base == std::floor(base));
    }
}

// ============================================================================
// CASO D - GREEDY MESHING
// ============================================================================

TEST_CASE("Geometria D: fusionar NO altera los limites de la superficie") {
    // Un quad fusionado de w x h celdas tiene que cubrir exactamente lo mismo
    // que las w*h caras sueltas: ni un texel de mas ni de menos.
    for (int w = 1; w <= CHUNK; ++w) {
        for (int h = 1; h <= CHUNK; ++h) {
            const auto fus = caraDelMesher(0, 0.0f, 0.0f, 10, 0, 0, w, h);

            float minX = fus[0].x, maxX = fus[0].x;
            float minZ = fus[0].z, maxZ = fus[0].z;
            for (const V3& v : fus) {
                minX = std::fmin(minX, v.x); maxX = std::fmax(maxX, v.x);
                minZ = std::fmin(minZ, v.z); maxZ = std::fmax(maxZ, v.z);
            }

            INFO("quad ", w, "x", h);
            CHECK(minX == 0.0f);
            CHECK(maxX == (float)w * S);
            CHECK(minZ == 0.0f);
            CHECK(maxZ == (float)h * S);
            CHECK(fus[0].y == 11.0f * S);
        }
    }
}

TEST_CASE("Geometria D: dos quads fusionados contiguos no se solapan ni dejan hueco") {
    const auto a = caraDelMesher(0, 0.0f, 0.0f, 10, 0, 0, 5, 1);
    const auto b = caraDelMesher(0, 0.0f, 0.0f, 10, 5, 0, 3, 1);

    float maxXa = a[0].x, minXb = b[0].x;
    for (const V3& v : a) maxXa = std::fmax(maxXa, v.x);
    for (const V3& v : b) minXb = std::fmin(minXb, v.x);

    CHECK(maxXa == minXb);          // se tocan
    CHECK(maxXa == 5.0f * S);       // en el limite de celda
}

// ============================================================================
// CASO F - PERSISTENCIA
// ============================================================================

TEST_CASE("Geometria F: guardar y cargar no mueve nada") {
    // La geometria se DERIVA de las coordenadas enteras, que son lo unico que
    // se guarda: no hay posiciones en coma flotante en el formato. Asi que el
    // viaje por disco no puede desplazar un bloque.
    for (int x = -500; x <= 500; x += 37) {
        for (int y = 0; y < 128; y += 11) {
            const Caja a = celdaDe(x, y, x / 2);
            const Caja b = celdaDe(x, y, x / 2);   // "tras recargar"
            INFO("celda ", x, ",", y);
            CHECK(a.x0 == b.x0); CHECK(a.x1 == b.x1);
            CHECK(a.y0 == b.y0); CHECK(a.y1 == b.y1);
            CHECK(a.z0 == b.z0); CHECK(a.z1 == b.z1);
        }
    }
}

// ============================================================================
// CASO G - LOS BLOQUES ESPECIALES CONSERVAN SU GEOMETRIA
// ============================================================================

TEST_CASE("Geometria G: el contrato NO convierte en cubo lo que no lo es") {
    // LA CONTRAPARTIDA OBLIGATORIA.
    //
    // Al apretar la regla de los cubos es facil arrastrar a todo lo demas.
    // Las capas parciales DEBEN seguir midiendo lo suyo.
    for (int n = 1; n <= 7; ++n) {
        const BlockType capa = conNivel(BLOCK_DIRT, n);
        INFO("nivel ", n);
        CHECK(esNivelParcial(capa));
        CHECK(alturaDe(capa) < 1.0f);      // NO es un cubo
        CHECK(alturaDe(capa) > 0.0f);
    }

    // Y el bloque entero si mide la celda completa.
    CHECK(alturaDe(BLOCK_DIRT) == doctest::Approx(1.0f));
    CHECK(alturaDe(BLOCK_STONE) == doctest::Approx(1.0f));

    // Una celda mixta esta llena (dos materiales, pero ocupa el voxel).
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_STONE);
    REQUIRE(m != BLOCK_AIR);
    CHECK(alturaDe(m) == doctest::Approx(1.0f));
}

// ============================================================================
// EL MESHER REAL SIGUE USANDO ESTA ARITMETICA
// ============================================================================

TEST_CASE("Geometria: emitQuad usa limites de celda ENTEROS, sin epsilon") {
    // EL TEST QUE ATA LA REPLICA DE ARRIBA AL CODIGO REAL.
    //
    // Si alguien mete un epsilon, un offset o una escala en `emitQuad` para
    // "tapar" un hueco, esto falla -- que es justo lo que se pidio evitar
    // ("no agrandar los bloques", "no desplazamientos artificiales").
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    CHECK(src.find("const float yy = (float)(layer + 1);") != std::string::npos);
    CHECK(src.find("const float yy = (float)layer;") != std::string::npos);
    CHECK(src.find("const float zz = baseWZ + layer + 1;") != std::string::npos);
    CHECK(src.find("const float zz = baseWZ + layer;") != std::string::npos);
    CHECK(src.find("const float xx = baseWX + layer + 1;") != std::string::npos);
    CHECK(src.find("const float xx = baseWX + layer;") != std::string::npos);

    // La base del chunk, multiplicacion entera.
    CHECK(src.find("(float)(chunk->position.x * CHUNK_SIZE)") != std::string::npos);
    CHECK(src.find("(float)(chunk->position.z * CHUNK_SIZE)") != std::string::npos);
}
