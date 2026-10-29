#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include "BlockType.h"

// ============================================================================
// EL AGUA NO ES UNA VENTANA NI UN ESPEJO
// ============================================================================
// BUG QUE ESTO PROTEGE: "el agua y sus niveles dejan ver otros bloques dentro,
// como si fuera un espejo, y hay caras sin textura".
//
// LA CAUSA eran DOS defectos que se sumaban en la frontera agua/solido:
//
//   1. El bloque solido no siempre emitia su cara contra el agua. La condicion
//      era `isBlockOpaque`, que responde "¿deja pasar la luz?" en vez de
//      "¿ocupa el volumen?". El cristal, los niveles llenos y varias formas
//      propias caian del lado equivocado.
//
//   2. El agua SI emitia sus seis caras contra cualquier cosa que no fuera
//      agua, incluida la roca maciza que la tapa por completo.
//
// Sumados, la cara del solido y la del agua acababan EN EL MISMO PLANO, sin
// orden definido entre ellas. Segun el angulo de camara ganaba una o la otra;
// cuando ganaba el agua, se veia el interior del terreno a traves de ella. Eso
// es el "espejo" del reporte.
//
// LA REGLA QUE FIJAN ESTOS TESTS, y es simetrica:
//
//     entre un solido que llena su voxel y un liquido,
//     la cara la dibuja UNO SOLO de los dos: el SOLIDO.
//
// ----------------------------------------------------------------------------
// POR QUE ESTO ES UNA REIMPLEMENTACION, Y QUE SIGNIFICA
// ----------------------------------------------------------------------------
// `shouldRenderFace` y `llenaSuVoxelFrenteALiquido` viven en main.cpp, que no
// se enlaza con los tests (ver el hallazgo I3 de la auditoria). Asi que aqui se
// replica la REGLA, no la funcion: lo que se fija es el contrato --quien dibuja
// y quien calla-- para que nadie lo invierta sin que salte un test.
//
// La copia tiene el riesgo conocido de separarse del motor. Se acota
// comprobando la propiedad que importa (exclusion mutua) en vez de comparar
// valores sueltos: si el motor cambia de criterio, la exclusion se rompe y esto
// falla.
// ============================================================================

using namespace Compuesto;

namespace {

// Replica de `llenaSuVoxelFrenteALiquido` (main.cpp). Solo los casos que los
// tests de abajo ejercitan: terreno, cristal, niveles y liquidos.
bool llenaFrenteALiquido(BlockType t) {
    if (t == BLOCK_AIR) return false;
    if (esAguaCualquiera(t)) return false;
    if (t == BLOCK_LAVA) return false;
    if (esNivelParcial(t)) return false;   // 1..7 octavos: dejan hueco arriba
    if (esMixto(t)) return true;           // llena de suelo a techo
    if (esCompuesto(t)) return false;      // plantas: huecos entre sus partes
    return true;                           // piedra, tierra, arena, cristal...
}

// Replica de la regla de `shouldRenderFace` para la frontera con un liquido.
// Devuelve si BLOQUE emite su cara contra VECINO.
bool emiteCara(BlockType bloque, BlockType vecino) {
    if (esAguaCualquiera(bloque)) {
        if (!esAguaCualquiera(vecino)) {
            // Contra un solido que llena su voxel, el agua calla.
            return !llenaFrenteALiquido(vecino);
        }
        // Agua contra agua: manda la altura (lo cubre test_agua_niveles).
        return Agua::alturaVisual(bloque) != Agua::alturaVisual(vecino);
    }
    if (esAguaCualquiera(vecino)) {
        // Un solido que llena su voxel SIEMPRE dibuja su cara contra el agua.
        return llenaFrenteALiquido(bloque);
    }
    return true;
}

} // namespace

TEST_CASE("el agua no dibuja caras contra un solido que la tapa") {
    // ESTE es el defecto 2. Una celda de agua enterrada en roca emitia sus
    // caras dentro de la piedra: geometria invisible que se colaba por las
    // rendijas del z-fighting y pintaba agua DENTRO del terreno.
    const BlockType solidos[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRASS,
        BLOCK_COBBLESTONE, BLOCK_GLASS, BLOCK_LIMESTONE,
    };

    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType agua = Agua::nuevo(n);
        for (BlockType s : solidos) {
            INFO("nivel de agua: ", (int)n);
            CHECK_FALSE(emiteCara(agua, s));
        }
    }

    // Y el agua legacy (BLOCK_WATER, los mundos viejos) se comporta igual.
    for (BlockType s : solidos)
        CHECK_FALSE(emiteCara(BLOCK_WATER, s));
}

TEST_CASE("el solido SI dibuja su cara contra el agua") {
    // La contrapartida obligatoria del test anterior. Si el agua calla y el
    // solido tambien, queda un AGUJERO por el que se ve el interior del mundo
    // -- que es peor que el bug original.
    const BlockType solidos[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRASS,
        BLOCK_COBBLESTONE, BLOCK_GLASS, BLOCK_LIMESTONE,
    };

    for (BlockType s : solidos) {
        CHECK(emiteCara(s, BLOCK_WATER));
        for (uint16_t n = 1; n <= Agua::LLENA; ++n)
            CHECK(emiteCara(s, Agua::nuevo(n)));
    }
}

TEST_CASE("ni las dos caras ni ninguna: exactamente una") {
    // ⭐ LA PROPIEDAD QUE IMPORTA, y la que el bug rompia.
    //
    // En la frontera entre agua y un solido que llena su voxel tiene que haber
    // UNA cara. Dos es el espejo (z-fighting entre dos superficies coplanares);
    // cero es un agujero al interior del mundo.
    const BlockType solidos[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRASS,
        BLOCK_COBBLESTONE, BLOCK_GLASS, BLOCK_LIMESTONE,
        BLOCK_PLANKS, BLOCK_WOOD, BLOCK_GRAVEL,
    };

    for (BlockType s : solidos) {
        for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
            const BlockType agua = Agua::nuevo(n);
            const int caras = (emiteCara(agua, s) ? 1 : 0) +
                              (emiteCara(s, agua) ? 1 : 0);
            INFO("solido vs nivel de agua: ", (int)n);
            CHECK(caras == 1);
        }
        // Tambien con el agua legacy.
        const int carasLegacy = (emiteCara(BLOCK_WATER, s) ? 1 : 0) +
                                (emiteCara(s, BLOCK_WATER) ? 1 : 0);
        CHECK(carasLegacy == 1);
    }
}

TEST_CASE("un nivel PARCIAL no tapa el agua: deja hueco por arriba") {
    // La excepcion que hay que respetar, y es facil de romper al arreglar lo
    // demas. Un nivel de 1..7 octavos NO llena su cubo: por el hueco de arriba
    // se ve el agua, asi que el agua SI tiene que dibujarse contra el.
    //
    // Si esto se rompiera, el borde de un charco contra una capa de tierra a
    // medias se veria como un corte transparente al vacio.
    const BlockType parcial = conNivel(BLOCK_DIRT, 4);
    REQUIRE(esNivelParcial(parcial));

    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType agua = Agua::nuevo(n);
        CHECK(emiteCara(agua, parcial));   // el agua se ve por el hueco
    }
}

TEST_CASE("una celda MIXTA si tapa el agua") {
    // Una mixta esta llena por definicion: capa de abajo con su nivel mas
    // relleno hasta el techo. Tapa el agua igual que un cubo normal.
    //
    // Va aparte porque `esNivelParcial` y `esMixto` se parecen y es el sitio
    // donde se confunden: la mixta LLENA, la parcial NO.
    const BlockType mezcla = mixto(BLOCK_DIRT, 4, BLOCK_SAND);
    REQUIRE(mezcla != BLOCK_AIR);
    REQUIRE(esMixto(mezcla));

    for (uint16_t n = 1; n <= Agua::LLENA; ++n) {
        const BlockType agua = Agua::nuevo(n);
        CHECK_FALSE(emiteCara(agua, mezcla));   // el agua calla
        CHECK(emiteCara(mezcla, agua));          // la mixta dibuja
    }
}

TEST_CASE("el aire no tapa nada") {
    // La regla de siempre, fijada para que el arreglo no se pase de celoso: el
    // agua contra aire SIEMPRE dibuja. Es la superficie de un charco vista
    // desde fuera.
    for (uint16_t n = 1; n <= Agua::LLENA; ++n)
        CHECK(emiteCara(Agua::nuevo(n), BLOCK_AIR));
    CHECK(emiteCara(BLOCK_WATER, BLOCK_AIR));
}
