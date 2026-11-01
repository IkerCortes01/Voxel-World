#include <doctest/doctest.h>
#include "BlockType.h"
#include <fstream>
#include <iterator>
#include <string>

// ============================================================================
// UN BLOQUE COMPLETO SOBRE UNA CAPA: SE JUNTAN SIN HUECO Y SIN SOLAPARSE
// ============================================================================
// PEDIDO: "que los bloques completos al ser colocados sobre niveles no floten
// ni dejen espacio, pero que no se sobreponga ni el bloque completo ni el
// nivel, que se junten bien".
//
// Antes, apuntando a la tapa de una capa:
//   - piedra labrada / techo de hojas: se recortaban a UNA lamina de 1/8;
//   - troncos, hojas, horno: subian enteros a la celda de arriba y flotaban
//     con 13 px de aire debajo.

namespace {

std::string fuenteMotor() {
#ifdef RAIZ_PROYECTO
    std::string r = RAIZ_PROYECTO;
    if (!r.empty() && r.back() != '/' && r.back() != '\\') r += '/';
    std::ifstream f(r + "src/main.cpp", std::ios::binary);
    if (!f.good()) return {};
    return std::string((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
#else
    return {};
#endif
}

int octavos(BlockType t) {
    if (t == BLOCK_AIR) return 0;
    if (esMixto(t)) return mixtoOctavosTotales(t);
    return nivelDe(t);
}

} // namespace

// ----------------------------------------------------------------------------
// QUIEN VA ENTERO
// ----------------------------------------------------------------------------
TEST_CASE("Entero: piedra labrada y techo de hojas se colocan enteros") {
    CHECK(seColocaEntero(BLOCK_COBBLESTONE));
    CHECK(seColocaEntero(BLOCK_TECHO_HOJAS));
    CHECK(seColocaEntero(BLOCK_TECHO_SUELO));
}

TEST_CASE("Entero: los tablones de TODAS las maderas son bloques completos") {
    // PEDIDO: "que los tablones de todas las maderas sean bloques completos".
    // Los tres con niveles entran en la regla; los otros cinco no tienen
    // niveles, asi que ya se colocaban enteros por definicion.
    CHECK(seColocaEntero(BLOCK_PLANKS));
    CHECK(seColocaEntero(BLOCK_PLANKS_ENCINO));
    CHECK(seColocaEntero(BLOCK_PLANKS_OYAMEL));
    for (BlockType t : { BLOCK_PLANKS_OCOTE, BLOCK_PLANKS_PATULA,
                         BLOCK_PLANKS_PINONERO, BLOCK_PLANKS_ALTURAS,
                         BLOCK_PLANKS_PSEUDOSTROBUS }) {
        INFO("tablon ", (int)t);
        CHECK(esTablon(t));
        CHECK_FALSE(admiteNiveles(t));   // sin niveles: siempre enteros
    }
}

TEST_CASE("Entero: el resto de bloques con niveles se coloca como capa") {
    CHECK_FALSE(seColocaEntero(BLOCK_DIRT));
    CHECK_FALSE(seColocaEntero(BLOCK_SAND));
    CHECK_FALSE(seColocaEntero(BLOCK_STONE));
}

TEST_CASE("Entero: una capa concreta no es 'entera' aunque sea de piedra labrada") {
    CHECK_FALSE(seColocaEntero(conNivel(BLOCK_COBBLESTONE, 3)));
    CHECK_FALSE(seColocaEntero(conNivel(BLOCK_TECHO_HOJAS, 2)));
}

TEST_CASE("Entero: lo que no admite niveles no entra en esta regla") {
    // El tronco va entero siempre, pero por otro camino (no puede mezclarse):
    // ver esMacizoSinCapas en main.cpp.
    CHECK_FALSE(seColocaEntero(BLOCK_WOOD));
}

// ----------------------------------------------------------------------------
// LA ARITMETICA: 8 OCTAVOS SOBRE UNA CAPA
// ----------------------------------------------------------------------------
TEST_CASE("Entero sobre capa: la celda queda llena y lo que sobra sube") {
    // Capa de tierra 3/8 + piedra labrada 8/8 = 11/8.
    const BlockType capa = conNivel(BLOCK_DIRT, 3);
    const Fusion f = fusionarNiveles(BLOCK_COBBLESTONE, capa);

    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    // La celda de la capa queda LLENA: sin hueco encima de la tierra.
    CHECK(octavos(f.resultado) == 8);
    // Tierra abajo, piedra encima: ninguna tapa a la otra.
    REQUIRE(esMixto(f.resultado));
    CHECK(mixtoBase(f.resultado) == BLOCK_DIRT);
    CHECK(mixtoNivel(f.resultado) == 3);
    CHECK(bloqueBaseDe(mixtoRelleno(f.resultado)) == BLOCK_COBBLESTONE);
    // Los 3/8 que no cupieron suben como capa de piedra.
    CHECK(bloqueBaseDe(f.sobrante) == BLOCK_COBBLESTONE);
    CHECK(octavos(f.sobrante) == 3);
}

TEST_CASE("Entero sobre capa: no se pierde ni se inventa material") {
    // Para cualquier capa de 1 a 7, lo que queda + lo que sube = capa + 8.
    for (int n = 1; n <= 7; ++n) {
        const Fusion f = fusionarNiveles(BLOCK_COBBLESTONE, conNivel(BLOCK_SAND, n));
        INFO("capa de arena nivel ", n);
        REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
        CHECK(octavos(f.resultado) + octavos(f.sobrante) == n + 8);
    }
}

TEST_CASE("Entero sobre su propia capa: suma OCHO, no uno") {
    // Era el atajo de "mismo material: +1 octavo". Piedra labrada entera
    // sobre una capa de piedra labrada 3/8 llena la celda y sube 3/8.
    const Fusion f = fusionarNiveles(BLOCK_COBBLESTONE, conNivel(BLOCK_COBBLESTONE, 3));
    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(f.resultado == BLOCK_COBBLESTONE);
    CHECK(f.sobrante == conNivel(BLOCK_COBBLESTONE, 3));
}

TEST_CASE("Entero sobre capa: el techo de hojas tambien se junta entero") {
    const Fusion f = fusionarNiveles(BLOCK_TECHO_HOJAS, conNivel(BLOCK_DIRT, 2));
    REQUIRE(f.tipo == ResultadoFusion::FUSIONADOS);
    CHECK(octavos(f.resultado) + octavos(f.sobrante) == 10);
}

// ----------------------------------------------------------------------------
// EL MOTOR USA ESTAS REGLAS
// ----------------------------------------------------------------------------
TEST_CASE("Entero sobre capa: placeBlock ya no recorta el bloque a una lamina") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    const size_t m = src.find("UN BLOQUE ENTERO SOBRE UNA CAPA: SE JUNTAN SIN HUECO");
    REQUIRE(m != std::string::npos);
    const std::string rama = src.substr(m, 9000);

    // 1. Los que admiten niveles entran con sus 8 octavos.
    CHECK(rama.find("if (seColocaEntero(enMano)) {") != std::string::npos);
    CHECK(rama.find("result.blockPos.z, entero);") != std::string::npos);

    // 2. Los que no: se pegan ENCIMA de la capa, en su celda, sin borrarla.
    //    PEDIDO: "que el bloque no elimine el nivel, que solo se pegue encima".
    CHECK(rama.find("pegarEncimaDeCapas(enMano, tocado)") != std::string::npos);
    // Y ya no se suelta la capa como items.
    CHECK(rama.find("getBlockDrops(t, bx, by, bz)") == std::string::npos);
}

TEST_CASE("Entero sobre capa: un tronco se pega encima y la capa sigue") {
    // La celda queda: la capa intacta abajo y el tronco de ahi al techo.
    const BlockType m = mixto(BLOCK_DIRT, 3, BLOCK_WOOD);
    REQUIRE(m != BLOCK_AIR);
    CHECK(mixtoBase(m) == BLOCK_DIRT);
    CHECK(mixtoNivel(m) == 3);              // la capa no se toca
    CHECK(mixtoRelleno(m) == BLOCK_WOOD);
    CHECK(mixtoOctavosTotales(m) == 8);     // pegado: sin aire en medio
}

TEST_CASE("Entero sobre capa: todas las ramas usan la misma excepcion") {
    // Antes cada rama repetia "salvo la piedra labrada" a mano y en una se
    // quedo fuera el techo. Ahora no queda ninguna copia de esa comparacion.
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());
    CHECK(src.find("bloqueBaseDe(aPoner) != BLOCK_COBBLESTONE") == std::string::npos);
    CHECK(src.find("bloqueBaseDe(blockToPlace) != BLOCK_COBBLESTONE") == std::string::npos);
}
