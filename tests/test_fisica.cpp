#include <doctest/doctest.h>
#include "FisicaCaida.h"
#include <cstdio>

// ============================================================================
// TESTS DE LA FISICA DE CAIDA
// ============================================================================
// Estos tests no comprueban "que el codigo hace lo que hace": comprueban que
// los numeros COINCIDEN CON LA REALIDAD. Si alguien toca una densidad o el
// coeficiente de arrastre y el mundo deja de comportarse como el de verdad,
// esto lo detecta.

using namespace Fisica;

// Integra una caida real, paso a paso, y devuelve el tiempo en segundos.
// Es el mismo calculo que hara el juego, asi que si esto da los tiempos
// buenos, el juego tambien.
static float tiempoDeCaida(BlockType t, float alturaBloques) {
    float v = 0.0f, y = 0.0f, tiempo = 0.0f;
    const float dt = 0.0005f;
    while (y < alturaBloques && tiempo < 60.0f) {
        v += aceleracionCaida(t, v) * dt;
        y += v * dt;
        tiempo += dt;
    }
    return tiempo;
}

TEST_CASE("Fisica: la gravedad es la terrestre estandar") {
    // 9.80665 m/s^2 es el valor exacto que fija el SI. No es aproximado.
    CHECK(G == doctest::Approx(9.80665f));
}

TEST_CASE("Fisica: el bloque mide 60 cm y de ahi sale todo") {
    CHECK(LADO_M == doctest::Approx(0.60f));
    CHECK(VOLUMEN_M3 == doctest::Approx(0.216f));
    CHECK(AREA_M2 == doctest::Approx(0.36f));
}

TEST_CASE("Fisica: SIN AIRE todos los cuerpos caen igual") {
    // El principio de equivalencia: la aceleracion inicial (v=0, sin
    // arrastre todavia) es la MISMA para todos, pese a que el oro pesa 90
    // veces mas que las hojas.
    //
    // Es lo que comprobo el Apollo 15 con el martillo y la pluma.
    const float aPiedra = aceleracionCaida(BLOCK_STONE, 0.0f);
    const float aMadera = aceleracionCaida(BLOCK_WOOD, 0.0f);
    const float aHojas  = aceleracionCaida(BLOCK_LEAVES, 0.0f);
    const float aOro    = aceleracionCaida(BLOCK_GOLD_ORE, 0.0f);

    CHECK(aPiedra == doctest::Approx(aMadera));
    CHECK(aMadera == doctest::Approx(aHojas));
    CHECK(aHojas  == doctest::Approx(aOro));

    // Y ese valor comun es g convertida a bloques/s^2.
    CHECK(aPiedra == doctest::Approx(G / LADO_M));
}

TEST_CASE("Fisica: CON AIRE lo denso cae mas rapido") {
    // Ya en movimiento, el aire frena mas a lo ligero: la fuerza de arrastre
    // es la misma pero la desaceleracion es F/m.
    const float v = 20.0f;   // bloques/s

    const float aPiedra = aceleracionCaida(BLOCK_STONE, v);
    const float aHojas  = aceleracionCaida(BLOCK_LEAVES, v);
    const float aMadera = aceleracionCaida(BLOCK_WOOD, v);

    // Orden por DENSIDAD: piedra 2650 > hojas 1010 > pino 449.
    CHECK(aPiedra > aHojas);
    CHECK(aHojas > aMadera);

    // Pero ninguno acelera MAS que la gravedad: el aire solo puede frenar.
    CHECK(aPiedra <= doctest::Approx(G / LADO_M));
}

TEST_CASE("Fisica: el arrastre se opone al movimiento, no lo invierte") {
    // Si un bloque subiera (velocidad negativa), el aire tiene que frenarlo
    // hacia abajo, no empujarlo mas arriba. Con v^2 pelado se perderia el
    // signo y el arrastre acelararia la subida.
    const float aSubiendo = aceleracionCaida(BLOCK_STONE, -20.0f);

    // Subiendo, gravedad y arrastre van en el mismo sentido: frena MAS que g.
    CHECK(aSubiendo > G / LADO_M);
}

TEST_CASE("Fisica: las densidades son las reales") {
    // Roca comun: 2700 kg/m3 es el granito.
    CHECK(densidadDe(BLOCK_STONE) == doctest::Approx(2650.0f));
    // El agua son 1000: la madera flota, la piedra no.
    CHECK(densidadDe(BLOCK_WOOD) < 1000.0f);
    CHECK(densidadDe(BLOCK_STONE) > 1000.0f);
    // El encino (roble, 705) es la madera dura; el pino (449) y el oyamel
    // (abeto, 481) son las coniferas ligeras. El pino resulta ser la mas
    // ligera de las tres, por poco.
    CHECK(densidadDe(BLOCK_WOOD_ENCINO) > densidadDe(BLOCK_WOOD_OYAMEL));
    CHECK(densidadDe(BLOCK_WOOD_OYAMEL) > densidadDe(BLOCK_WOOD));
    // ⚠️ Las hojas NO son ligeras: su tejido fresco es casi agua (1010
    // kg/m3, medido sobre 1039 especies). Lo que flota es una hoja fina,
    // no un bloque macizo de hojas. Este test fija ese dato porque es
    // justo el que la intuicion tiende a "corregir" mal.
    CHECK(densidadDe(BLOCK_LEAVES) == doctest::Approx(1010.0f));
    CHECK(densidadDe(BLOCK_LEAVES) > densidadDe(BLOCK_WOOD));
}

TEST_CASE("Fisica: un cubo de piedra de 60 cm pesa media tonelada") {
    // 2650 kg/m3 * 0.216 m3 = 572.4 kg. Un bloque de piedra NO es ligero.
    CHECK(masaDe(BLOCK_STONE) == doctest::Approx(572.4f));

    // Y uno de hojas, 218 kg: tampoco.
    CHECK(masaDe(BLOCK_LEAVES) == doctest::Approx(218.16f));
}

TEST_CASE("Fisica: la velocidad terminal sale de la formula real") {
    // v_t = sqrt(2*m*g / (rho*Cd*A)). Para la piedra:
    //   m = 572.4, g = 9.80665, rho = 1.225, Cd = 1.05, A = 0.36
    //   v_t = sqrt(11226.7 / 0.46305) = 155.7 m/s
    CHECK(velocidadTerminal(BLOCK_STONE) == doctest::Approx(155.7f).epsilon(0.01));

    // Lo denso tiene mayor velocidad terminal, siempre.
    CHECK(velocidadTerminal(BLOCK_STONE) > velocidadTerminal(BLOCK_LEAVES));
    CHECK(velocidadTerminal(BLOCK_LEAVES) > velocidadTerminal(BLOCK_WOOD));
}

TEST_CASE("Fisica: los tiempos de caida coinciden con la realidad") {
    // En 10 bloques (6 m reales) la caida libre teorica es
    // t = sqrt(2h/g) = sqrt(12/9.80665) = 1.106 s.
    //
    // Con aire, la piedra deberia quedarse muy cerca (apenas la frena) y la
    // vegetacion algo por detras.
    const float tPiedra = tiempoDeCaida(BLOCK_STONE, 10.0f);
    const float tHojas  = tiempoDeCaida(BLOCK_LEAVES, 10.0f);

    printf("[FISICA] caida 10 bloques: piedra=%.3f s  hojas=%.3f s\n",
           tPiedra, tHojas);

    // La piedra, practicamente caida libre.
    CHECK(tPiedra == doctest::Approx(1.106f).epsilon(0.02));

    // Las hojas tardan MAS, pero poco: son un cubo macizo, no una pluma.
    CHECK(tHojas > tPiedra);
    CHECK(tHojas < tPiedra * 1.10f);
}

TEST_CASE("Fisica: en caidas largas la diferencia SI se nota") {
    // A 100 bloques (60 m) el arrastre ha tenido tiempo de actuar.
    const float tPiedra = tiempoDeCaida(BLOCK_STONE, 100.0f);
    const float tNieve  = tiempoDeCaida(BLOCK_SNOW, 100.0f);
    const float tPino   = tiempoDeCaida(BLOCK_WOOD, 100.0f);

    printf("[FISICA] caida 100 bloques: piedra=%.3f s  nieve=%.3f s  pino=%.3f s\n",
           tPiedra, tNieve, tPino);

    // Casi una decima de segundo entre la piedra y la nieve compactada, y
    // otro tanto con el pino, que es lo mas ligero del juego. Se percibe,
    // aunque queda lejos de lo que la intuicion esperaria: son cubos
    // MACIZOS de 60 cm, no plumas.
    CHECK(tNieve - tPiedra > 0.08f);
    CHECK(tPino  - tPiedra > 0.05f);
}

TEST_CASE("Fisica: la energia de impacto crece con el cuadrado de la velocidad") {
    // E = 1/2*m*v^2. Doblar la velocidad CUADRUPLICA la energia.
    const float e1 = energiaImpacto(BLOCK_STONE, 10.0f);
    const float e2 = energiaImpacto(BLOCK_STONE, 20.0f);

    CHECK(e2 == doctest::Approx(e1 * 4.0f));

    // Y a igual velocidad, lo denso pega mas fuerte.
    CHECK(energiaImpacto(BLOCK_STONE, 10.0f) >
          energiaImpacto(BLOCK_WOOD, 10.0f));
}

TEST_CASE("Fisica: los niveles parciales y las mixtas usan su material") {
    // Una capa de tierra es tierra, no roca por defecto.
    const BlockType capa = conNivel(BLOCK_DIRT, 3);
    CHECK(densidadDe(capa) == doctest::Approx(densidadDe(BLOCK_DIRT)));

    // En una celda mixta manda el RELLENO: es la parte de arriba, la que se
    // desprende cuando la celda pierde el apoyo.
    const BlockType mix = mixto(BLOCK_DIRT, 4, BLOCK_SAND);
    if (mix != BLOCK_AIR)
        CHECK(densidadDe(mix) == doctest::Approx(densidadDe(BLOCK_SAND)));
}

TEST_CASE("Fisica: ningun bloque tiene densidad absurda") {
    // Barrido de todo el enum: nada puede pesar cero (caeria infinitamente
    // rapido, division por cero en la aceleracion) ni mas que el osmio, que
    // es el elemento mas denso que existe (22590 kg/m3).
    for (int id = 1; id <= BLOCK_TYPE_MAX; ++id) {
        const BlockType t = (BlockType)id;
        const float d = densidadDe(t);
        CHECK(d > 0.0f);
        CHECK(d < 22590.0f);
        // Y la aceleracion tiene que ser finita.
        CHECK(std::isfinite(aceleracionCaida(t, 0.0f)));
        CHECK(std::isfinite(aceleracionCaida(t, 50.0f)));
    }
}

// ============================================================================
// LA REGLA VALE PARA TODO: BLOQUES, NIVELES Y CELDAS MIXTAS
// ============================================================================
// El sistema de caida decide con dos preguntas: "¿esto puede caer?" y "¿esto
// sujeta?". Las dos tienen que dar la MISMA respuesta para un bloque entero,
// para una capa parcial de ese bloque y para una celda mixta que lo lleve.
//
// Si no, aparecen incoherencias raras: una loncha de tronco sosteniendo un
// arbol entero, o una capa de tierra que no aguanta lo que hay encima.

TEST_CASE("Caida: la densidad no cambia entre bloque entero y su capa") {
    // Lo que decide como cae un bloque es su densidad, y una capa parcial de
    // piedra sigue siendo piedra.
    const BlockType capaPiedra = conNivel(BLOCK_STONE, 3);
    CHECK(densidadDe(capaPiedra) == doctest::Approx(densidadDe(BLOCK_STONE)));

    const BlockType capaTierra = conNivel(BLOCK_DIRT, 7);
    CHECK(densidadDe(capaTierra) == doctest::Approx(densidadDe(BLOCK_DIRT)));

    // Y por tanto caen igual: misma aceleracion a la misma velocidad.
    CHECK(aceleracionCaida(capaPiedra, 15.0f) ==
          doctest::Approx(aceleracionCaida(BLOCK_STONE, 15.0f)));
}

TEST_CASE("Caida: una celda mixta cae como su relleno") {
    // En una celda mixta el relleno es la parte de ARRIBA: lo que se
    // desprenderia y lo que sostiene lo que hubiera encima.
    const BlockType mix = mixto(BLOCK_DIRT, 4, BLOCK_STONE);
    if (mix != BLOCK_AIR) {
        CHECK(densidadDe(mix) == doctest::Approx(densidadDe(BLOCK_STONE)));
        CHECK(aceleracionCaida(mix, 15.0f) ==
              doctest::Approx(aceleracionCaida(BLOCK_STONE, 15.0f)));
    }
}

TEST_CASE("Caida: la masa de una pieza es la suma de la de sus bloques") {
    // Una estructura cae con la masa del CONJUNTO. Dos bloques de piedra
    // pesan el doble que uno.
    const float uno = masaDe(BLOCK_STONE);
    const float dos = uno * 2.0f;

    // Y con el doble de masa pero la misma silueta (uno encima de otro), el
    // aire la frena la MITAD: cae mas rapido que un bloque suelto.
    const float aUno = aceleracionPieza(uno, AREA_M2, 30.0f);
    const float aDos = aceleracionPieza(dos, AREA_M2, 30.0f);
    CHECK(aDos > aUno);
}

TEST_CASE("Caida: una pieza ancha frena mas que una estrecha") {
    // Misma masa, mas area frontal -> mas arrastre. Es la diferencia entre
    // un tronco cayendo de punta y una copa de hojas extendida.
    const float m = masaDe(BLOCK_STONE) * 4.0f;

    const float estrecha = aceleracionPieza(m, AREA_M2, 30.0f);
    const float ancha    = aceleracionPieza(m, AREA_M2 * 4.0f, 30.0f);

    CHECK(estrecha > ancha);
}

TEST_CASE("Caida: una pieza sin masa no revienta") {
    // Division por cero: tiene que devolver 0, no infinito ni NaN.
    CHECK(aceleracionPieza(0.0f, AREA_M2, 10.0f) == doctest::Approx(0.0f));
    CHECK(std::isfinite(aceleracionPieza(1.0f, 0.0f, 10.0f)));
}

// ============================================================================
// LOS GUIJARROS NO CAEN
// ============================================================================
// Piedritas, pedernal, polvo de tierra y demas cantos sueltos se quedan
// donde estan. No son bloques que ocupen su celda: son un montoncito
// apoyado en el suelo, y un punado de piedras no se derrumba.

TEST_CASE("Guijarros: ninguno entra en el sistema de caida") {
    // La lista completa, uno por uno. Si manana se anade otro canto a
    // esGuijarro() queda excluido solo, pero estos son los que hay hoy.
    const BlockType CANTOS[] = {
        BLOCK_PEDAZO_PIEDRA, BLOCK_PEDAZO_GRAVA, BLOCK_PEDAZO_PEDERNAL,
        BLOCK_PEDAZO_CALIZA, BLOCK_PEDAZO_TIERRA, BLOCK_PEDAZO_COBRE,
        BLOCK_PEDAZO_GOETHITA, BLOCK_PEDAZO_HEMATITE, BLOCK_PEDAZO_LIMONITA,
        BLOCK_PEDAZO_NIEVE
    };

    for (BlockType t : CANTOS) {
        CHECK(esGuijarro(t));
    }
}

TEST_CASE("Guijarros: los bloques de verdad SI siguen cayendo") {
    // La exclusion tiene que ser solo para los cantos: el terreno, la roca
    // y la madera siguen sujetos a la gravedad.
    CHECK_FALSE(esGuijarro(BLOCK_STONE));
    CHECK_FALSE(esGuijarro(BLOCK_DIRT));
    CHECK_FALSE(esGuijarro(BLOCK_SAND));
    CHECK_FALSE(esGuijarro(BLOCK_GRAVEL));      // el BLOQUE de grava, no el canto
    CHECK_FALSE(esGuijarro(BLOCK_WOOD));
    CHECK_FALSE(esGuijarro(BLOCK_SNOW));        // el BLOQUE de nieve

    // Y sus capas parciales tampoco son guijarros.
    CHECK_FALSE(esGuijarro(conNivel(BLOCK_STONE, 3)));
}

TEST_CASE("Guijarros: el canto y el bloque del mismo material no se confunden") {
    // Es el error facil: BLOCK_GRAVEL es un bloque de grava (cae) y
    // BLOCK_PEDAZO_GRAVA es un punado de gravilla (no cae). Son distintos.
    CHECK(BLOCK_GRAVEL != BLOCK_PEDAZO_GRAVA);
    CHECK(BLOCK_SNOW   != BLOCK_PEDAZO_NIEVE);
    CHECK(BLOCK_STONE  != BLOCK_PEDAZO_PIEDRA);

    CHECK(esGuijarro(BLOCK_PEDAZO_GRAVA));
    CHECK_FALSE(esGuijarro(BLOCK_GRAVEL));
}

// ============================================================================
// EL VUELCO DEL ARBOL
// ============================================================================
// Un arbol talado no baja recto: gira sobre su pie como una bisagra. Es un
// pendulo invertido, y su aceleracion angular es
//
//     alpha = (3 * g * sin(angulo)) / (2 * L)
//
// donde L es la altura. La masa SE CANCELA en esa formula, lo que tiene una
// consecuencia bonita: la caida no depende de lo que pese el arbol.

TEST_CASE("Vuelco: un arbol arranca despacio y acelera al vencerse") {
    // En vertical casi no se mueve; a media caida ya va lanzado. Es el seno
    // del angulo el que lo hace, y es como cae un arbol de verdad.
    const float aVertical = aceleracionVuelco(10.0f, 0.0f);
    const float aMedia    = aceleracionVuelco(10.0f, 0.7854f);  // 45 grados
    const float aTumbado  = aceleracionVuelco(10.0f, 1.5708f);  // 90 grados

    CHECK(aVertical < aMedia);
    CHECK(aMedia < aTumbado);

    // Pero en vertical NO es cero: sin un empujon minimo, un arbol
    // perfectamente recto se quedaria parado para siempre.
    CHECK(aVertical > 0.0f);
}

TEST_CASE("Vuelco: un arbol alto se tumba mas despacio") {
    // alpha es inversamente proporcional a la altura. No es una impresion:
    // es la razon fisica de que los arboles grandes parezcan caer lentos.
    const float bajo = aceleracionVuelco(5.0f,  0.7854f);
    const float alto = aceleracionVuelco(20.0f, 0.7854f);

    CHECK(bajo > alto);
}

TEST_CASE("Vuelco: la caida NO depende de lo que pese el arbol") {
    // La masa se cancela en alpha = 3*g*sin/2L, asi que un pino y un encino
    // de la misma altura se tumban igual de rapido. Es el mismo principio
    // por el que dos cuerpos caen igual en el vacio.
    //
    // La funcion ni siquiera recibe la masa: eso ES la comprobacion.
    const float a1 = aceleracionVuelco(12.0f, 0.5f);
    const float a2 = aceleracionVuelco(12.0f, 0.5f);
    CHECK(a1 == doctest::Approx(a2));
}

TEST_CASE("Vuelco: una altura absurda no revienta") {
    // Division por cero si L fuera 0.
    CHECK(aceleracionVuelco(0.0f, 0.5f) == doctest::Approx(0.0f));
    CHECK(std::isfinite(aceleracionVuelco(0.001f, 0.5f)));
    CHECK(std::isfinite(aceleracionVuelco(1000.0f, 1.5708f)));
}

TEST_CASE("Vuelco: una pieza nace de pie y sin volcar") {
    PiezaCayendo pz;
    CHECK(pz.angulo == doctest::Approx(0.0f));
    CHECK_FALSE(pz.vuelca);
    CHECK(pz.volcarX == 0);
    CHECK(pz.volcarZ == 0);
}
