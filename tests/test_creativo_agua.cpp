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
        BLOCK_COAL_ORE, BLOCK_GOLD_ORE, BLOCK_DIAMOND_ORE,
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
    CHECK(tiempoCreativo(BLOCK_SCRAP_METAL) < 0.1f);

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

// ============================================================================
// 2. EL AGUA ESCURRE, NO CAE COMO UN BLOQUE
// ============================================================================
// Antes bajaba TODO lo que cupiera en un solo tick: una celda llena se
// vaciaba de golpe y parecia un solido cayendo. Ahora se limita el trasvase a
// CAUDAL_CAIDA octavos por tick.

namespace {
constexpr int CAUDAL_CAIDA = 2;   // el mismo valor que usa updateWaterFlow

// Replica del paso 2 de updateWaterFlow: cuanto baja de una celda a la de
// debajo en UN tick.
int bajaEnUnTick(int nivelArriba, int nivelAbajo) {
    const int hueco = (int)Compuesto::Agua::LLENA - nivelAbajo;
    if (hueco <= 0) return 0;
    int baja = (nivelArriba < hueco) ? nivelArriba : hueco;
    if (baja > CAUDAL_CAIDA) baja = CAUDAL_CAIDA;
    return baja;
}
} // namespace

TEST_CASE("Agua: no se vacia una celda entera de golpe") {
    // ⭐ ESTE ES EL CAMBIO PEDIDO.
    //
    // Una celda llena (8 octavos) sobre una vacia bajaba los 8 de una vez.
    // Ahora baja de CAUDAL_CAIDA en CAUDAL_CAIDA, asi que se ve escurrir.
    const int baja = bajaEnUnTick(8, 0);
    INFO("baja en un tick: ", baja, " de 8");
    CHECK(baja == CAUDAL_CAIDA);
    CHECK(baja < 8);
}

TEST_CASE("Agua: tarda varios ticks en descolgarse") {
    // Se simula la caida completa contando ticks. Con 8 octavos y caudal 2,
    // hacen falta 4 -- que a 0,5 s por tick son ~2 s de hilo de agua.
    int arriba = 8, abajo = 0, ticks = 0;
    while (arriba > 0 && ticks < 100) {
        const int b = bajaEnUnTick(arriba, abajo);
        if (b <= 0) break;
        arriba -= b;
        abajo  += b;
        ++ticks;
    }
    INFO("ticks hasta vaciarse: ", ticks);
    CHECK(ticks == 4);
    CHECK(arriba == 0);
    CHECK(abajo == 8);
}

TEST_CASE("Agua: el volumen se conserva en cada tick") {
    // ⚠️ LA INVARIANTE QUE NO SE PUEDE ROMPER.
    //
    // Limitar el caudal cambia CUANTA agua se mueve por tick, no cuanta hay.
    // Si el total variara, el agua dejaria de ser finita y un oceano podria
    // vaciarse o desbordarse solo.
    for (int arriba = 1; arriba <= 8; ++arriba) {
        for (int abajo = 0; abajo <= 8; ++abajo) {
            const int antes = arriba + abajo;
            const int b = bajaEnUnTick(arriba, abajo);
            const int despues = (arriba - b) + (abajo + b);
            INFO("arriba ", arriba, " abajo ", abajo, " baja ", b);
            CHECK(despues == antes);
        }
    }
}

TEST_CASE("Agua: nunca se pasa del tope de la celda de abajo") {
    // Meter mas de 8 octavos en una celda desbordaria el campo NIVEL (4 bits)
    // y el agua se leeria con otro nivel al recargar.
    for (int arriba = 1; arriba <= 8; ++arriba) {
        for (int abajo = 0; abajo <= 8; ++abajo) {
            const int b = bajaEnUnTick(arriba, abajo);
            INFO("arriba ", arriba, " abajo ", abajo);
            CHECK(abajo + b <= (int)Compuesto::Agua::LLENA);
            CHECK(b >= 0);
            CHECK(b <= arriba);         // no puede bajar mas de lo que hay
        }
    }
}

TEST_CASE("Agua: sobre una celda llena no baja nada") {
    // Sin hueco no hay trasvase. Es lo que hace que el agua se pare al llegar
    // al fondo en vez de seguir empujando y gastando CPU.
    for (int arriba = 1; arriba <= 8; ++arriba) {
        INFO("arriba ", arriba);
        CHECK(bajaEnUnTick(arriba, 8) == 0);
    }
}

TEST_CASE("Agua: el ultimo octavo tambien baja") {
    // Un hilo de agua de 1 octavo tiene que poder descolgarse: si el caudal
    // minimo fuera mayor que lo que queda, el resto se quedaria colgado para
    // siempre.
    CHECK(bajaEnUnTick(1, 0) == 1);
    CHECK(bajaEnUnTick(1, 7) == 1);
}

TEST_CASE("Agua: el nivel que cae se marca como chorro") {
    // El mesher dibuja el agua marcada como CAYENDO ocupando la celda entera:
    // un hilo fino sigue siendo un chorro de arriba abajo, no un charco
    // flotando en el aire. Sin esa marca, el goteo se veria como laminas
    // sueltas separadas.
    const BlockType cayendo = Compuesto::Agua::nuevo(2, true);
    CHECK(Compuesto::Agua::estaCayendo(cayendo));
    CHECK(Compuesto::Agua::alturaVisual(cayendo) == doctest::Approx(1.0f));

    // La misma agua en reposo se dibuja a su altura real.
    const BlockType quieta = Compuesto::Agua::nuevo(2, false);
    CHECK_FALSE(Compuesto::Agua::estaCayendo(quieta));
    CHECK(Compuesto::Agua::alturaVisual(quieta) < 1.0f);
}
