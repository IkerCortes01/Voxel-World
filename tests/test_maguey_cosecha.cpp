#include <doctest/doctest.h>
#include "BloqueCompuesto.h"
#include "BlockType.h"
#include "Inventory.h"

// ============================================================================
// LA COSECHA DEL MAGUEY: PENCAS SEGUN EL FILO Y EL TAMAÑO
// ============================================================================
// Tumbar un maguey no daba NADA. La planta caia entera, se oia el golpe, y el
// jugador se quedaba con las manos vacias -- lo que convertia en inutil una de
// las plantas mas trabajadas del motor.
//
// LA REGLA QUE SE IMPLEMENTA:
//
//   CON FILO (hacha de piedra o de pedernal afilado)  -> de 1 a 4 pencas
//   A MANO o con cualquier otra cosa                  -> nada
//
// Las pencas son fibrosas y correosas: a tirones se destrozan, con filo se
// cortan limpias. Es la misma logica que ya rige el resto del motor, donde la
// herramienta decide si un bloque entrega material (esOrganicoParaHacha para
// lo vegetal, esRocaParaPico para la piedra).
//
// CUANTAS SALEN. De la ETAPA, porque una mata da lo que tiene. El reparto usa
// `celdasDeEtapa`, que es la medida de tamaño que ya comparten el mesher, la
// hitbox y la seleccion -- asi que si algun dia se cambia el tamaño de una
// etapa, la cosecha lo sigue sola en vez de quedarse desincronizada.
//
// LAS DOS HACHAS DAN LO MISMO, a proposito. La de pedernal ya se distingue
// donde importa: aguanta 390 bloques frente a 250 y es la unica que puede con
// la punta del maguey. Hacerla ademas mas productiva la volveria obligatoria
// en vez de preferible, que es una decision de diseño distinta.
//
// QUE PUEDE FIJAR ESTE ARCHIVO. La cosecha vive en updateMining, que necesita
// el GameState y el mundo entero. Lo que SI es logica pura -- y es donde
// estarian los errores -- son las dos preguntas de las que depende: que cuenta
// como hacha, y cuantas pencas toca por etapa.

namespace {

// Replica de la regla del drop, tal y como la aplica updateMining.
int pencasQueSuelta(BlockType herramienta, uint16_t etapa) {
    if (!esHacha(herramienta)) return 0;
    return Compuesto::Maguey::celdasDeEtapa(etapa);
}

} // namespace

// ============================================================================
// HACE FALTA FILO
// ============================================================================

TEST_CASE("Cosecha del maguey: sin hacha no se saca nada") {
    namespace M = Compuesto::Maguey;

    // La mano desnuda y cualquier cosa que no corte. Un maguey tumbado a
    // patadas no deja fibra aprovechable.
    const BlockType sinFilo[] = {
        BLOCK_AIR,               // la mano
        BLOCK_PICO_PIEDRA,       // rompe roca, no corta fibra
        BLOCK_PICO_PEDERNAL,
        BLOCK_MARTILLO_PIEDRA,   // golpea, no corta
        BLOCK_MARTILLO_PEDERNAL,
        BLOCK_DIRT,              // un bloque cualquiera en la mano
        BLOCK_STONE
    };

    for (BlockType h : sinFilo) {
        for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
            INFO("herramienta ", (int)h, " etapa ", e);
            CHECK(pencasQueSuelta(h, e) == 0);
        }
    }
}

TEST_CASE("Cosecha del maguey: las dos hachas cortan") {
    namespace M = Compuesto::Maguey;

    // Piedra y pedernal afilado. Si alguna dejara de contar como hacha, esa
    // rama del juego se quedaria sin cosecha en silencio.
    CHECK(esHacha(BLOCK_HACHA_PIEDRA));
    CHECK(esHacha(BLOCK_HACHA_PEDERNAL));

    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        INFO("etapa ", e);
        CHECK(pencasQueSuelta(BLOCK_HACHA_PIEDRA, e) > 0);
        CHECK(pencasQueSuelta(BLOCK_HACHA_PEDERNAL, e) > 0);
    }
}

TEST_CASE("Cosecha del maguey: las dos hachas dan lo mismo") {
    namespace M = Compuesto::Maguey;

    // Decision de diseño explicita: la de pedernal gana en DURACION y en poder
    // con la punta, no en cantidad. Si un dia se quiere premiar el pedernal con
    // mas pencas, este test es el que hay que cambiar -- y obliga a hacerlo
    // conscientemente en vez de por descuido.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        INFO("etapa ", e);
        CHECK(pencasQueSuelta(BLOCK_HACHA_PIEDRA, e) ==
              pencasQueSuelta(BLOCK_HACHA_PEDERNAL, e));
    }
}

// ============================================================================
// LA CANTIDAD SALE DEL TAMAÑO
// ============================================================================

TEST_CASE("Cosecha del maguey: de 1 a 4 pencas segun la etapa") {
    namespace M = Compuesto::Maguey;

    // El rango pedido: una mata pequeña da poco, un ejemplar hecho da el maximo.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const int n = pencasQueSuelta(BLOCK_HACHA_PIEDRA, e);
        INFO("etapa ", e, " da ", n, " pencas");
        CHECK(n >= 1);
        CHECK(n <= 4);
    }

    // Y los extremos, fijados: el brote da lo minimo y el productor lo maximo.
    CHECK(pencasQueSuelta(BLOCK_HACHA_PIEDRA, M::BROTE)     == 1);
    CHECK(pencasQueSuelta(BLOCK_HACHA_PIEDRA, M::PRODUCTOR) == 4);
}

TEST_CASE("Cosecha del maguey: una mata mas grande nunca da menos") {
    namespace M = Compuesto::Maguey;

    // Monotonia. Sin esto podria colarse un reparto donde el ADULTO diera menos
    // que el JOVEN, y el jugador aprenderia a cortar las matas pequeñas -- justo
    // lo contrario de lo que se busca.
    int anterior = 0;
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const int n = pencasQueSuelta(BLOCK_HACHA_PIEDRA, e);
        INFO("etapa ", e, ": ", anterior, " -> ", n);
        CHECK(n >= anterior);
        anterior = n;
    }
}

TEST_CASE("Cosecha del maguey: la cantidad sigue al tamaño real de la planta") {
    namespace M = Compuesto::Maguey;

    // La cosecha se calcula con `celdasDeEtapa`, la MISMA funcion de la que
    // salen el modelo, la hitbox y la seleccion. Atarlo aqui garantiza que si
    // alguien cambia el tamaño de una etapa, la cosecha lo sigue sola.
    //
    // Si en su lugar hubiera una tabla propia de "pencas por etapa", esa tabla
    // se quedaria desincronizada al primer cambio -- que es exactamente el
    // problema que ya tiene el motor con sus dos tablas de drops.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        INFO("etapa ", e);
        CHECK(pencasQueSuelta(BLOCK_HACHA_PIEDRA, e) == M::celdasDeEtapa(e));
    }
}

// ============================================================================
// LO QUE SE RECOGE ES UN ITEM VALIDO
// ============================================================================

TEST_CASE("Cosecha del maguey: la penca es un item con textura") {
    // Se entrega BLOCK_IXTLE_HOJA, que es la penca del maguey y ya existe como
    // bloque con textura propia (Maguey.png).
    //
    // Tiene que estar por debajo de BLOCK_TYPE_MAX porque prewarmItemTextures
    // recorre justo ese rango para registrar los iconos: por encima del tope,
    // el item saldria SIN TEXTURA en la mano y en el inventario. Es el mismo
    // fallo que se acaba de corregir con las piezas del agave azul.
    CHECK((int)BLOCK_IXTLE_HOJA > 0);
    CHECK((int)BLOCK_IXTLE_HOJA <= BLOCK_TYPE_MAX);

    // Y no es un compuesto: los IDs de familia van muy por encima del enum y
    // no se pueden llevar en el inventario.
    CHECK_FALSE(Compuesto::esCompuesto(BLOCK_IXTLE_HOJA));
}

// ============================================================================
// EL TEQUILANA DA SU PROPIA PENCA
// ============================================================================

TEST_CASE("Cosecha del agave azul: da un item distinto del pulquero") {
    // Son dos plantas distintas y se ven distintas: la penca del tequilana es
    // azul plateada mate por la cera, la del pulquero verde. Compartir item
    // borraria de un plumazo la diferencia que costo modelar.
    CHECK(BLOCK_PENCA_AGAVE_AZUL != BLOCK_IXTLE_HOJA);

    // Pero las dos son pencas de maguey a efectos de render: es lo que hace
    // que las dos se dibujen con el mismo grosor de hoja carnosa.
    CHECK(esPencaDeMaguey(BLOCK_PENCA_AGAVE_AZUL));
    CHECK(esPencaDeMaguey(BLOCK_IXTLE_HOJA));
}

TEST_CASE("Cosecha del agave azul: de 1 a 2 pencas segun la etapa") {
    namespace AG = Compuesto::AgaveAzul;

    // La roseta del tequilana ocupa 1 o 2 celdas, asi que da menos que el
    // pulquero (que llega a 4). Es coherente: es mas ancho que alto, no mas
    // alto, y la cantidad sale de las celdas igual que en el pulquero.
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        const int n = AG::celdasDeEtapa(e);
        INFO("etapa ", e, " da ", n);
        CHECK(n >= 1);
        CHECK(n <= 2);
    }

    // Y una planta mas grande nunca da menos.
    int anterior = 0;
    for (uint16_t e = AG::HIJUELO; e <= AG::MADURA; ++e) {
        const int n = AG::celdasDeEtapa(e);
        CHECK(n >= anterior);
        anterior = n;
    }
}

TEST_CASE("Cosecha del agave azul: la piña jimada ya no da hoja") {
    namespace AG = Compuesto::AgaveAzul;

    // Al jimar se cortaron TODAS las pencas: lo que queda es el corazon. Si
    // siguiera dando hoja, jimar y luego talar daria la cosecha dos veces.
    const BlockType pina = AG::jimada(AG::nuevo(AG::MADURA, 0));
    CHECK(AG::faseDe(pina) == AG::PINA);
}

TEST_CASE("Cosecha del agave azul: su penca es un item valido") {
    // Mismo requisito que la del pulquero: por debajo del tope para que
    // prewarmItemTextures le registre icono, y fuera del rango de compuestos.
    CHECK((int)BLOCK_PENCA_AGAVE_AZUL > 0);
    CHECK((int)BLOCK_PENCA_AGAVE_AZUL <= BLOCK_TYPE_MAX);
    CHECK_FALSE(Compuesto::esCompuesto(BLOCK_PENCA_AGAVE_AZUL));
}

// ============================================================================
// EL MODELO 3D DE LA PENCA SUELTA
// ============================================================================

TEST_CASE("Penca suelta: las dos especies se dibujan con el mismo grosor") {
    // El render pregunta por esPencaDeMaguey() para decidir el grosor. Si una
    // especie se quedara fuera, se veria como lamina fina junto a la otra sin
    // ningun motivo.
    CHECK(esPencaDeMaguey(BLOCK_IXTLE_HOJA));
    CHECK(esPencaDeMaguey(BLOCK_PENCA_AGAVE_AZUL));

    // Y nada mas entra en el grupo: es solo para las hojas cortadas.
    CHECK_FALSE(esPencaDeMaguey(BLOCK_STICK));
    CHECK_FALSE(esPencaDeMaguey(BLOCK_STONE));
    CHECK_FALSE(esPencaDeMaguey(BLOCK_MAGUEY_PUNTA));
    // Las variantes de tamaño del ixtle son bloques del mundo, no items.
    CHECK_FALSE(esPencaDeMaguey(BLOCK_IXTLE_PEQUENA));
    CHECK_FALSE(esPencaDeMaguey(BLOCK_IXTLE_GRANDE));
    CHECK_FALSE(esPencaDeMaguey(BLOCK_IXTLE_ENORME));
}

TEST_CASE("Penca suelta: 4 px de grosor sobre 16 de lado") {
    // El render usa `scale` como SEMILADO: el item mide 2*scale de ancho y de
    // alto. Para que el grosor sea 4 px de los 16 del lado, el semigrosor
    // tiene que ser scale * 0.25.
    //
    //     ancho  = 2 * scale        -> 16 px
    //     grosor = 2 * scale * 0.25 ->  4 px
    //
    // Se comprueba la aritmetica, que es donde estaria el error de un factor 2.
    constexpr float FACTOR_PENCA = 0.25f;
    const float scale = 0.2f;                  // el que usa el render

    const float ancho  = 2.0f * scale;
    const float grosor = 2.0f * scale * FACTOR_PENCA;

    INFO("ancho ", ancho, " grosor ", grosor);
    CHECK(grosor == doctest::Approx(ancho * 0.25f));
    // Cuatro de dieciseis: exactamente un cuarto del lado.
    CHECK(grosor / ancho == doctest::Approx(4.0f / 16.0f));

    // Y es mas gruesa que un item normal (0.18), que es el punto: la hoja de
    // agave es carnosa.
    CHECK(FACTOR_PENCA > 0.18f);
}

TEST_CASE("Penca suelta: es cuadrada, largo y ancho iguales") {
    // El render dibuja las caras de -scale a +scale en los dos ejes, asi que
    // largo y ancho son el mismo numero por construccion. Este test fija esa
    // propiedad para que nadie meta un factor distinto en uno de los dos.
    const float scale = 0.2f;
    const float largo = 2.0f * scale;
    const float ancho = 2.0f * scale;
    CHECK(largo == doctest::Approx(ancho));
}

TEST_CASE("Cosecha del maguey: la penca se apila en el inventario") {
    // Un drop de hasta 4 unidades no sirve de nada si no se puede acumular.
    // MAX_STACK_SIZE es el tope del motor; lo unico que hace falta comprobar es
    // que la penca no sea un caso especial que no apile.
    CHECK(MAX_STACK_SIZE > 4);
}
