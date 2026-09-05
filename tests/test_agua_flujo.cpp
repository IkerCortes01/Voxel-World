#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

// ============================================================================
// LAS REGLAS DEL FLUJO DEL AGUA
// ============================================================================
// El motor de flujo vive dentro de World, que necesita OpenGL y no se puede
// instanciar en un test. Pero las REGLAS de las que depende no dependen de
// World: son aritmetica sobre niveles.
//
// Aqui se reproducen esas reglas exactamente como las aplica updateWaterFlow,
// y se comprueba que cumplen las tres propiedades que sostienen el sistema:
//
//   1. CONSERVACION. Repartir agua no la crea ni la destruye.
//   2. CONVERGENCIA. El reparto se para; no oscila para siempre.
//   3. SATURACION. La tierra bebe una vez y deja de beber.
//
// La 1 y la 2 son las que impiden los dos desastres del sistema anterior: que
// el mar se drene solo, y que dos celdas se pasen el mismo octavo eternamente
// tirando los FPS al suelo.
// ============================================================================

using namespace Compuesto;

namespace {

// El reparto lateral, tal como lo hace updateWaterFlow: se mueve un octavo al
// vecino MAS VACIO, y solo si la diferencia es de 2 o mas.
//
// Devuelve true si movio algo.
bool repartirUnPaso(int& origen, int& destino) {
    if (origen < 2) return false;
    if (destino > origen - 2) return false;
    origen--;
    destino++;
    return true;
}

// La absorcion: la tierra seca bebe un octavo y queda saturada.
bool absorber(BlockType& suelo, int& nivel) {
    if (nivel <= 0) return false;
    const BlockType mojado = versionMojada(suelo);
    if (mojado == suelo) return false;   // impermeable o ya saturado
    suelo = mojado;
    nivel--;
    return true;
}

} // namespace

TEST_CASE("repartir agua conserva el volumen") {
    // LA PROPIEDAD CENTRAL. Si esto falla, el agua se crea o se destruye sola
    // y cualquier otra garantia del sistema deja de valer.
    int a = 8, b = 0;
    const int total = a + b;

    while (repartirUnPaso(a, b)) {
        CHECK(a + b == total);      // en CADA paso, no solo al final
    }
    CHECK(a + b == total);
}

TEST_CASE("el reparto converge y no oscila") {
    // El sistema viejo podia dejar dos celdas pasandose el mismo octavo. Con
    // el umbral de diferencia >= 2 eso no puede pasar: el reparto se para.
    //
    // Se acota el numero de pasos para que un bucle infinito falle el test en
    // vez de colgarlo.
    int a = 8, b = 0;
    int pasos = 0;
    while (repartirUnPaso(a, b)) {
        REQUIRE(++pasos < 100);
    }

    // Al pararse, las dos celdas quedan a diferencia de 1 como mucho.
    CHECK(a - b <= 1);
    CHECK(a >= b);
}

TEST_CASE("dos celdas iguales no se mueven nada") {
    // Una superficie plana esta en equilibrio: si se moviera agua, el mar
    // herviria eternamente y se comeria el frame.
    int a = 4, b = 4;
    CHECK_FALSE(repartirUnPaso(a, b));
    CHECK(a == 4);
    CHECK(b == 4);
}

TEST_CASE("una lamina de un octavo no se parte") {
    // No existen los medios octavos. Una celda de 1 no puede repartirse sin
    // moverse ENTERA, y eso seria saltar de sitio, no fluir.
    int a = 1, b = 0;
    CHECK_FALSE(repartirUnPaso(a, b));
    CHECK(a == 1);
}

TEST_CASE("el agua fluye cuesta abajo, nunca cuesta arriba") {
    // Del que tiene menos al que tiene mas: jamas.
    int poco = 2, mucho = 8;
    CHECK_FALSE(repartirUnPaso(poco, mucho));
    CHECK(poco == 2);
    CHECK(mucho == 8);
}

TEST_CASE("la tierra bebe una vez y se satura") {
    // ESTE es el test que protege los oceanos.
    //
    // Si la tierra bebiera sin limite, la orilla de un mar -- millones de
    // celdas de agua tocando arena -- se tragaria el mar entero. Al saturarse,
    // bebe un octavo y deja de beber para siempre.
    BlockType suelo = BLOCK_DIRT;
    int nivel = 8;

    CHECK(absorber(suelo, nivel));      // el primer trago si
    CHECK(nivel == 7);
    CHECK(suelo == BLOCK_DIRT_MOJADA);

    // Y ya no bebe mas, por mucha agua que le pongan encima.
    for (int i = 0; i < 20; i++) {
        CHECK_FALSE(absorber(suelo, nivel));
    }
    CHECK(nivel == 7);                  // el agua se quedo donde estaba
}

TEST_CASE("la arena y el pasto tambien beben; la piedra no") {
    BlockType arena = BLOCK_SAND;
    int n1 = 8;
    CHECK(absorber(arena, n1));
    CHECK(arena == BLOCK_SAND_MOJADA);

    BlockType pasto = BLOCK_GRASS;
    int n2 = 8;
    CHECK(absorber(pasto, n2));
    CHECK(pasto == BLOCK_GRASS_MOJADA);

    // La piedra es impermeable: es lo que permite hacer un estanque que no se
    // filtra. Si bebiera, no habria forma de contener agua en ningun sitio.
    BlockType piedra = BLOCK_STONE;
    int n3 = 8;
    CHECK_FALSE(absorber(piedra, n3));
    CHECK(piedra == BLOCK_STONE);
    CHECK(n3 == 8);
}

TEST_CASE("mojar y secar son inversos") {
    // Romper tierra mojada tiene que soltar tierra normal, no un bloque
    // "mojado" que se acumule en el inventario.
    CHECK(versionSeca(versionMojada(BLOCK_DIRT))  == BLOCK_DIRT);
    CHECK(versionSeca(versionMojada(BLOCK_SAND))  == BLOCK_SAND);
    CHECK(versionSeca(versionMojada(BLOCK_GRASS)) == BLOCK_GRASS);

    // Y lo que nunca se mojo se queda como esta.
    CHECK(versionSeca(BLOCK_STONE) == BLOCK_STONE);
}

TEST_CASE("un charco se seca del todo al absorberse") {
    // Una celda de agua sobre tierra seca acaba desapareciendo si no le llega
    // mas agua: la tierra se lleva un octavo y el resto se reparte. Es lo que
    // hace que un cubo derramado en el desierto no se quede ahi para siempre.
    int nivel = 1;
    BlockType suelo = BLOCK_SAND;

    CHECK(absorber(suelo, nivel));
    CHECK(nivel == 0);          // no queda agua: la celda se seca
}

TEST_CASE("un bloque en el agua no destruye el agua") {
    // Al meter un bloque en una celda con agua, esa agua se reparte a los
    // vecinos. Lo que se comprueba aqui es la aritmetica del reparto: lo que
    // sale de la celda tapada entra en las de al lado.
    const int desplazada = 8;

    // Tres vecinos con hueco: 8/8 lleno, 6/8 y 2/8.
    int vecinos[3] = { 8, 6, 2 };
    const int antes = vecinos[0] + vecinos[1] + vecinos[2] + desplazada;

    int queda = desplazada;
    for (int i = 0; i < 3 && queda > 0; i++) {
        const int hueco = (int)Agua::LLENA - vecinos[i];
        if (hueco <= 0) continue;
        const int pasa = (queda < hueco) ? queda : hueco;
        vecinos[i] += pasa;
        queda -= pasa;
    }

    // Nada se perdio por el camino: lo repartido mas lo que sobra es lo que
    // habia. (Lo que sobra se pierde a proposito -- el bloque la expulso --
    // pero tiene que estar CONTADO, no desaparecer sin mas.)
    CHECK(vecinos[0] + vecinos[1] + vecinos[2] + queda == antes);

    // El vecino lleno no admitio nada; los otros dos se llenaron.
    // Los huecos eran 0 + 2 + 6 = 8, justo los 8 octavos desplazados, asi que
    // esta vez cupo todo y no se perdio nada.
    CHECK(vecinos[0] == 8);
    CHECK(vecinos[1] == 8);
    CHECK(vecinos[2] == 8);
    CHECK(queda == 0);
}

TEST_CASE("el agua que no cabe en ningun vecino se pierde, pero contada") {
    // Cuando los vecinos ya estan llenos, el agua desplazada no tiene donde
    // ir: el bloque la ha echado del mundo. Es correcto que se pierda -- es lo
    // que pasa al meter un ladrillo en un vaso lleno hasta el borde.
    //
    // Lo que NO puede pasar es que se pierda sin que el motor lo sepa: si un
    // dia se quiere un contador de agua total, esta es la unica fuga legitima.
    const int desplazada = 8;
    int vecinos[3] = { 8, 8, 8 };       // todos llenos
    const int antes = 24 + desplazada;

    int queda = desplazada;
    for (int i = 0; i < 3 && queda > 0; i++) {
        const int hueco = (int)Agua::LLENA - vecinos[i];
        if (hueco <= 0) continue;
        const int pasa = (queda < hueco) ? queda : hueco;
        vecinos[i] += pasa;
        queda -= pasa;
    }

    CHECK(vecinos[0] + vecinos[1] + vecinos[2] + queda == antes);
    CHECK(queda == 8);          // los 8 octavos se derramaron fuera
}

TEST_CASE("una columna de agua cae entera sin perder volumen") {
    // El agua que cae se suma a la celda de abajo hasta llenarla, y lo que no
    // cabe se queda arriba. Ni se duplica ni se pierde.
    int arriba = 5, abajo = 6;
    const int total = arriba + abajo;

    const int hueco = (int)Agua::LLENA - abajo;
    const int baja  = (arriba < hueco) ? arriba : hueco;
    abajo  += baja;
    arriba -= baja;

    CHECK(arriba + abajo == total);
    CHECK(abajo == (int)Agua::LLENA);   // se lleno
    CHECK(arriba == 3);                 // y lo que no cupo se quedo
}
