#include <doctest/doctest.h>
#include "BlockType.h"
#include "BloqueCompuesto.h"

// ============================================================================
// ROTURA INSTANTANEA EN CREATIVO, Y EL AGUA QUE ESCURRE
// ============================================================================
// Dos cambios que se pidieron juntos y que comparten una misma idea: el agua
// no es un bloque como los demas.
//
//   1. En CREATIVO todo se rompe al instante... MENOS EL AGUA.
//   2. El agua no cae "con gravedad" como un solido: escurre por niveles
//      pequeños.
//
// ----------------------------------------------------------------------------
// POR QUE EL AGUA ES LA EXCEPCION
// ----------------------------------------------------------------------------
// El volumen de agua de este motor es FINITO y se conserva: moverla es restar
// de una celda y sumar lo mismo en otra (ver updateWaterFlow). Esa invariante
// es lo que hace que un oceano siga siendo un oceano y que llenar un tazon
// saque agua de verdad.
//
// Dejar que un clic en creativo la borrase al instante seria la unica via del
// juego capaz de DESTRUIR agua. De ahi que se le de un tiempo de rotura
// inalcanzable en vez de instantaneo.

// El tiempo "inalcanzable" del motor: 13 minutos. Vive en main.cpp, que el
// binario de tests no enlaza, asi que se replica aqui. Lo que importa no es el
// numero exacto sino que sea ENORME frente al instantaneo.
constexpr float TIEMPO_INALCANZABLE = 780.0f;

// La replica de la regla de getBlockBreakTimeForMode para el modo creativo.
// gameMode: 0 = Survival, 1 = Creative, 2 = Adventure.
static float tiempoCreativo(BlockType t) {
    if (esAguaCualquiera(t)) return TIEMPO_INALCANZABLE;
    return 0.0f;
}

// ============================================================================
// 1. CREATIVO: TODO AL INSTANTE MENOS EL AGUA
// ============================================================================

TEST_CASE("Creativo: los bloques normales se rompen al instante") {
    // El creativo es para construir: esperar a que caiga un bloque estorba.
    //
    // El umbral del motor es 0.1 s -- por debajo de eso updateMining pone el
    // progreso a 1 directamente (y de paso evita dividir por cero).
    const BlockType MUESTRA[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_SAND, BLOCK_GRAVEL,
        BLOCK_WOOD, BLOCK_WOOD_ENCINO, BLOCK_WOOD_OCOTE,
        BLOCK_LEAVES, BLOCK_LEAVES_OCOTE,
        BLOCK_PLANKS, BLOCK_COBBLESTONE, BLOCK_LIMESTONE,
        // Aqui iban tres minerales de veta (carbon, oro y diamante), los
        // bloques mas duros del juego en supervivencia. Retirados.
        BLOCK_CLAY_DIRT, BLOCK_CLAY_SAND, BLOCK_BEDROCK,
        BLOCK_TALLGRASS, BLOCK_SNOW, BLOCK_CLAY,
    };
    for (BlockType b : MUESTRA) {
        INFO("bloque ", (int)b);
        CHECK(tiempoCreativo(b) < 0.1f);
    }
}

TEST_CASE("Creativo: hasta lo mas duro cae al instante") {
    // En supervivencia estos cuestan minutos (talar a mano es inviable, la
    // punta del maguey exige hacha de pedernal). En creativo, no.
    CHECK(tiempoCreativo(BLOCK_MAGUEY_PUNTA) < 0.1f);
    CHECK(tiempoCreativo(BLOCK_WOOD_OCOTE_DENTRO) < 0.1f);
    // Era BLOCK_SCRAP_METAL, retirado con el resto de los minerales.
    CHECK(tiempoCreativo(BLOCK_BEDROCK) < 0.1f);

    // Y tambien un bloque compuesto (maguey adulto), que en survival pide
    // hacha buena.
    const BlockType maguey =
        Compuesto::Maguey::nuevo(Compuesto::Maguey::PRODUCTOR, 0);
    CHECK(tiempoCreativo(maguey) < 0.1f);
}

TEST_CASE("Creativo: el agua NO se rompe") {
    // ⭐ LA EXCEPCION QUE PROTEGE LA CONSERVACION DEL VOLUMEN.
    //
    // Si el agua se rompiera al instante, un clic la haria desaparecer -- y
    // seria la unica forma del juego de destruir agua. El resto del motor
    // solo la MUEVE (restar aqui, sumar alli) o la RECOGE con un tazon.
    CHECK(tiempoCreativo(BLOCK_WATER) > 100.0f);

    // Y tambien el agua con volumen, en todos sus niveles: es la que usa el
    // motor de verdad desde que el nivel viaja dentro del ID.
    for (uint16_t n = 1; n <= Compuesto::Agua::LLENA; ++n) {
        const BlockType agua = Compuesto::Agua::nuevo(n);
        INFO("agua de nivel ", n);
        CHECK(esAguaCualquiera(agua));
        CHECK(tiempoCreativo(agua) > 100.0f);
    }
}

TEST_CASE("Creativo: la lava tampoco es agua, asi que si se rompe") {
    // La distincion importa: la regla es sobre el AGUA concretamente, no
    // sobre "los liquidos". La lava no tiene el sistema de volumen finito.
    CHECK_FALSE(esAguaCualquiera(BLOCK_LAVA));
    CHECK(tiempoCreativo(BLOCK_LAVA) < 0.1f);
}
