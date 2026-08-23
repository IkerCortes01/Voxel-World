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
