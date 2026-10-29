#include <doctest/doctest.h>
#include "BlockType.h"
#include "BloqueCompuesto.h"
#include <initializer_list>

// ============================================================================
// CELDAS DE VARIAS CAPAS ("PILAS")
// ============================================================================
// PEDIDO: "elimina el bug de que cuando combinas 2 niveles y quieres poner
// otros, no te deje y solo estire el nivel 2 diferente; haz que sean por
// separado pero esten juntos los niveles".
//
// Una celda mixta guarda dos materiales. La pila guarda hasta tres, en un
// rango de IDs nuevo, sin tocar los IDs de siempre.

namespace {

int octavosDe(BlockType t) { return octavosOcupados(t); }

Capas capas(std::initializer_list<Capa> l) {
    Capas k;
    for (const Capa& c : l) k.c[k.n++] = c;
    return k;
}

} // namespace

// ----------------------------------------------------------------------------
// LECTURA: capasDe vale para cualquier celda
// ----------------------------------------------------------------------------
TEST_CASE("Capas: un nivel es una capa, un bloque entero es una capa de 8") {
    const Capas a = capasDe(conNivel(BLOCK_DIRT, 3));
    REQUIRE(a.n == 1);
    CHECK(a.c[0].material == BLOCK_DIRT);
    CHECK(a.c[0].octavos == 3);

    const Capas b = capasDe(BLOCK_STONE);
    REQUIRE(b.n == 1);
    CHECK(b.c[0].octavos == 8);

    // Las maderas sin niveles tambien son capas posibles.
    CHECK(capasDe(BLOCK_WOOD_PATULA).n == 1);
}

TEST_CASE("Capas: la mixta de siempre son dos capas") {
    const Capas k = capasDe(mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
    REQUIRE(k.n == 2);
    CHECK(k.c[0].material == BLOCK_DIRT);  CHECK(k.c[0].octavos == 3);
    CHECK(k.c[1].material == BLOCK_SAND);  CHECK(k.c[1].octavos == 2);
}

TEST_CASE("Capas: aire, plantas y agua no son celdas de capas") {
    CHECK(capasDe(BLOCK_AIR).n == 0);
    CHECK(capasDe(BLOCK_TALLGRASS).n == 0);
    CHECK(capasDe(BLOCK_WATER).n == 0);
}

// ----------------------------------------------------------------------------
// ESCRITURA: celdaDeCapas elige siempre la forma mas vieja que sirve
// ----------------------------------------------------------------------------
TEST_CASE("Capas: ningun mundo guardado cambia -- las formas viejas se conservan") {
    // Una capa -> el nivel de siempre.
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 3}})) == conNivel(BLOCK_DIRT, 3));
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 8}})) == BLOCK_DIRT);

    // Dos capas que caben en la mixta -> EXACTAMENTE el ID mixto de siempre.
    for (int a = 1; a <= 6; ++a)
        for (int b = 1; a + b <= 8; ++b) {
            INFO("tierra ", a, " + arena ", b);
            CHECK(celdaDeCapas(capas({{BLOCK_DIRT, a}, {BLOCK_SAND, b}}))
                  == mixto(BLOCK_DIRT, a, BLOCK_SAND, b));
        }
}

TEST_CASE("Capas: tres capas van a la pila, y se leen igual que se escribieron") {
    const BlockType p = celdaDeCapas(capas({
        {BLOCK_DIRT, 3}, {BLOCK_SAND, 2}, {BLOCK_STONE, 1}}));
    REQUIRE(esPila(p));
    CHECK_FALSE(esMixto(p));
    CHECK(esCeldaDeCapas(p));

    const Capas k = capasDe(p);
    REQUIRE(k.n == 3);
    CHECK(k.c[0].material == BLOCK_DIRT);   CHECK(k.c[0].octavos == 3);
    CHECK(k.c[1].material == BLOCK_SAND);   CHECK(k.c[1].octavos == 2);
    CHECK(k.c[2].material == BLOCK_STONE);  CHECK(k.c[2].octavos == 1);
    CHECK(octavosDe(p) == 6);
}

TEST_CASE("Capas: dos capas seguidas del mismo material se unen") {
    // Tierra 2 + tierra 3 = una capa de tierra 5, no dos.
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 2}, {BLOCK_DIRT, 3}}))
          == conNivel(BLOCK_DIRT, 5));
}

TEST_CASE("Capas: lo que no cabe se rechaza, no se inventa") {
    // Mas de 8 octavos no es una celda: se rechaza en vez de recortar.
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 5}, {BLOCK_SAND, 4}})) == BLOCK_AIR);
    // Un material que no puede formar capa (hojas) tampoco.
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 3}, {BLOCK_LEAVES, 5}})) == BLOCK_AIR);
    // (La cuarta capa no se puede ni expresar: Capas tiene sitio para tres.
    //  Lo que pasa al intentarlo lo fija "una cuarta crece la de arriba".)
}

TEST_CASE("Capas: el rango de la pila no pisa nada") {
    CHECK(BLOCK_PILA_BASE > BLOCK_MIXTO_FIN);
    CHECK(BLOCK_PILA_BASE > Compuesto::COMPUESTO_FIN);
    CHECK_FALSE(esPila(BLOCK_DIRT));
    CHECK_FALSE(esPila(mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2)));
    CHECK_FALSE(esPila(Compuesto::hacer(Compuesto::FAM_MAGUEY, 3)));
}

// ----------------------------------------------------------------------------
// ⭐ EL BUG: EL SEGUNDO NIVEL SE ESTIRABA
// ----------------------------------------------------------------------------
TEST_CASE("Bug: un tercer material va en SU capa, la arena ya no se estira") {
    // Tierra 3 + arena 2, y el jugador pone piedra (1 octavo).
    const BlockType antes = mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2);
    const Fusion f = fusionarNiveles(conNivel(BLOCK_STONE, 1), antes);

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(f.sobrante == BLOCK_AIR);
    const Capas k = capasDe(f.resultado);
    REQUIRE(k.n == 3);
    CHECK(k.c[0].material == BLOCK_DIRT);  CHECK(k.c[0].octavos == 3);
    CHECK(k.c[1].material == BLOCK_SAND);  CHECK(k.c[1].octavos == 2);   // NO crece
    CHECK(k.c[2].material == BLOCK_STONE); CHECK(k.c[2].octavos == 1);   // aparece
}

TEST_CASE("Bug: volver a poner el material de abajo tambien va por separado") {
    // Tierra 3 + arena 2, y otra vez tierra: capa nueva de tierra ENCIMA de
    // la arena, no la arena estirandose.
    const Fusion f = fusionarNiveles(conNivel(BLOCK_DIRT, 1),
                                     mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
    const Capas k = capasDe(f.resultado);
    REQUIRE(k.n == 3);
    CHECK(k.c[2].material == BLOCK_DIRT);
    CHECK(k.c[1].octavos == 2);
}

TEST_CASE("Bug: el mismo material que la capa de arriba SI la hace crecer") {
    // Arena sobre tierra+arena: es la misma capa de arena, mas alta.
    const Fusion f = fusionarNiveles(conNivel(BLOCK_SAND, 1),
                                     mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
    CHECK(f.resultado == mixto(BLOCK_DIRT, 3, BLOCK_SAND, 3));
}

TEST_CASE("Bug: las capas quedan pegadas -- la celda mide su suma, sin hueco") {
    const Fusion f = fusionarNiveles(conNivel(BLOCK_STONE, 2),
                                     mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
    CHECK(octavosDe(f.resultado) == 7);
    CHECK(alturaDe(f.resultado) == doctest::Approx((float)alturaNivelPx(7) / 16.0f));
}

TEST_CASE("Capas: lo que no cabe en la celda sube, como siempre") {
    // Tierra 3 + arena 2 + piedra entera (8): caben 3, suben 5.
    const Fusion f = fusionarNiveles(BLOCK_COBBLESTONE,
                                     mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(octavosDe(f.resultado) == 8);
    CHECK(capasDe(f.resultado).n == 3);
    CHECK(f.sobrante == conNivel(BLOCK_COBBLESTONE, 5));
}

TEST_CASE("Capas: ni un octavo se pierde ni se inventa, en ninguna combinacion") {
    const BlockType mats[] = { BLOCK_DIRT, BLOCK_SAND, BLOCK_STONE, BLOCK_GRAVEL };
    for (BlockType a : mats) for (BlockType b : mats) for (BlockType c : mats) {
        if (a == b) continue;
        for (int na = 1; na <= 5; ++na) for (int nb = 1; na + nb <= 7; ++nb)
        for (int nc = 1; nc <= 7; ++nc) {
            const BlockType destino = celdaDeCapas(capas({{a, na}, {b, nb}}));
            REQUIRE(destino != BLOCK_AIR);
            const Fusion f = fusionarNiveles(conNivel(c, nc), destino);
            INFO(na, "+", nb, " + ", nc);
            CHECK(octavosDe(f.resultado) +
                  (f.sobrante == BLOCK_AIR ? 0 : octavosDe(f.sobrante))
                  == na + nb + nc);
        }
    }
}

TEST_CASE("Capas: con tres capas, una cuarta crece la de arriba (sin perder nada)") {
    const BlockType tres = celdaDeCapas(capas({
        {BLOCK_DIRT, 2}, {BLOCK_SAND, 2}, {BLOCK_STONE, 1}}));
    const Fusion f = fusionarNiveles(conNivel(BLOCK_GRAVEL, 1), tres);
    CHECK(octavosDe(f.resultado) == 6);
    CHECK(capasDe(f.resultado).n == 3);
}

// ----------------------------------------------------------------------------
// LO QUE LA PILA DESBLOQUEA: LAS OCHO MADERAS ENCIMA DE UNA CAPA
// ----------------------------------------------------------------------------
TEST_CASE("Maderas: un tronco de cualquier pino se pega encima de una capa") {
    // El pino patula no cabe en la tabla de la mixta: antes flotaba encima.
    const BlockType p = celdaDeCapas(capas({{BLOCK_DIRT, 3}, {BLOCK_WOOD_PATULA, 5}}));
    REQUIRE(p != BLOCK_AIR);
    const Capas k = capasDe(p);
    REQUIRE(k.n == 2);
    CHECK(k.c[0].octavos == 3);                     // la capa, intacta
    CHECK(k.c[1].material == BLOCK_WOOD_PATULA);
    CHECK(octavosDe(p) == 8);                       // pegado, sin hueco
}

TEST_CASE("Maderas: el pino de siempre sigue usando la mixta de siempre") {
    CHECK(celdaDeCapas(capas({{BLOCK_DIRT, 3}, {BLOCK_WOOD, 5}}))
          == mixto(BLOCK_DIRT, 3, BLOCK_WOOD));
}

// ----------------------------------------------------------------------------
// EL RESTO DEL MOTOR LA ENTIENDE
// ----------------------------------------------------------------------------
TEST_CASE("Capas: arriba es lo que se pica y se pisa; abajo, lo que nombra") {
    const BlockType p = celdaDeCapas(capas({
        {BLOCK_DIRT, 3}, {BLOCK_SAND, 2}, {BLOCK_STONE, 1}}));
    CHECK(materialArriba(p) == BLOCK_STONE);
    CHECK(materialAbajo(p)  == BLOCK_DIRT);
    CHECK_FALSE(celdaLlena(p));
    CHECK(celdaAdmiteMas(p));
}

TEST_CASE("Capas: al picar la de arriba, queda la mixta de antes") {
    const BlockType p = celdaDeCapas(capas({
        {BLOCK_DIRT, 3}, {BLOCK_SAND, 2}, {BLOCK_STONE, 1}}));
    Capas resto = capasDe(p);
    --resto.n;
    CHECK(celdaDeCapas(resto) == mixto(BLOCK_DIRT, 3, BLOCK_SAND, 2));
}
