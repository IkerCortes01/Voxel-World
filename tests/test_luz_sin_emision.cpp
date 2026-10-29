#include <doctest/doctest.h>
#include "LuzCielo.h"
#include <fstream>
#include <iterator>
#include <set>
#include <string>

// ============================================================================
// NINGUN BLOQUE DA LUZ, Y COLOCAR A OSCURAS NO ENCIENDE NADA
// ============================================================================
// PEDIDO: "en la noche o en oscuridad total coloco bloques de techo o
// cualquier bloque, y por alguna razon genera iluminacion rara; haz que los
// bloques jamas puedan de ninguna manera hacer luz".
//
// Dos frentes:
//
//   1. EL BUG. recalcularLuzLocal empezaba cada columna con cielo pleno (18)
//      en el borde de arriba de su caja, aunque ese borde estuviera bajo un
//      tejado o dentro de una cueva. Ahora la luz que entra la calcula
//      `luzQueEntraPorArriba` mirando lo que hay encima.
//
//   2. LA GARANTIA. El sistema de iluminacion global muerto (con torchlight y
//      una tabla donde la piedra emitia luz) se borro. Sin escritores de
//      torchlight, ningun camino puede hacer que un bloque ilumine.

namespace {

// Columna de mentira: el conjunto de alturas opacas y de follaje.
struct Columna {
    std::set<int> opacos, follaje;
    int entra(int desde, int techo) const {
        return luzQueEntraPorArriba(
            [&](int y) { return opacos.count(y) > 0; },
            [&](int y) { return follaje.count(y) > 0; },
            desde, techo, 3);
    }
};

// Mismo localizador que test_techo_hojas: RAIZ_PROYECTO lo define CMake.
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

} // namespace

TEST_CASE("Luz: bajo un tejado no entra cielo pleno (el bug)") {
    // Casa: tejado en y=75, el jugador coloca en y=64. La caja se recorta a
    // 64+18 = 82... pero en una zona con una montana cerca el techo de la
    // zona esta mas alto y el borde de la caja puede quedar bajo el tejado:
    // aqui, desde y=70. Antes la columna empezaba con 18 en y=70, dentro de
    // la casa a oscuras. Ahora ve el tejado y entra con 0.
    Columna c;
    c.opacos = { 75 };
    CHECK(c.entra(70, 120) == 0);
}

TEST_CASE("Luz: en una cueva no entra nada") {
    Columna c;
    for (int y = 40; y <= 90; ++y) c.opacos.insert(y);  // roca encima
    CHECK(c.entra(30, 90) == 0);
}

TEST_CASE("Luz: a cielo abierto entra plena, como antes") {
    Columna c;
    CHECK(c.entra(70, 120) == LUZ_CIELO_PLENA);
}

TEST_CASE("Luz: si el borde ya esta sobre el techo no se recorre nada") {
    // yDesde > yTecho: el bucle no entra, y la columna empieza en 18. Es el
    // caso normal en superficie y no debe cambiar.
    Columna c;
    c.opacos = { 50 };   // muy por debajo: no cuenta
    CHECK(c.entra(100, 90) == LUZ_CIELO_PLENA);
}

TEST_CASE("Luz: el follaje de encima atenua lo que entra") {
    Columna c;
    c.follaje = { 80, 81 };
    CHECK(c.entra(70, 120) == LUZ_CIELO_PLENA - 6);

    // Una copa espesa la apaga del todo, sin bajar de 0.
    for (int y = 80; y <= 90; ++y) c.follaje.insert(y);
    CHECK(c.entra(70, 120) == 0);
}

TEST_CASE("Luz: la luz que entra nunca supera el cielo pleno") {
    // Garantia de "los bloques jamas hacen luz": la unica fuente es el cielo,
    // y nada en el recorrido puede SUMAR luz.
    Columna c;
    for (int desde = 0; desde < 100; desde += 7) {
        CHECK(c.entra(desde, 120) <= LUZ_CIELO_PLENA);
        c.follaje.insert(desde + 3);
        CHECK(c.entra(desde, 120) <= LUZ_CIELO_PLENA);
    }
}

// ----------------------------------------------------------------------------
// DE NIVEL A BRILLO: LA CURVA UNICA
// ----------------------------------------------------------------------------
// PEDIDO (con capturas): de noche, las paredes de un cuarto cerrado se ven
// iluminadas mientras afuera esta oscuro.

namespace {
// La curva vieja, copiada tal cual estaba en el mesher, para fijar el bug.
float curvaVieja(int nivel, float sol) {
    const float raw = (float)nivel / 18.0f;
    float f = raw;
    if (f < 0.15f) f = 0.15f;
    const float delCielo = f * raw;
    const float propia   = f - delCielo;
    return propia + delCielo * (0.12f + 0.88f * sol);
}
}

TEST_CASE("Brillo: la curva vieja hacia brillar los interiores de noche") {
    // Es la evidencia del bug, en numeros: un cuarto a media luz se veia mas
    // del doble de claro que la intemperie, y uno cerrado tambien mas claro.
    CHECK(curvaVieja(10, 0.0f) > 2.0f * curvaVieja(18, 0.0f));
    CHECK(curvaVieja(0, 0.0f) > curvaVieja(18, 0.0f));
}

TEST_CASE("Brillo: de noche ningun interior supera a la intemperie") {
    const float afuera = brilloDeNivel(18, 0.0f);
    for (int n = 0; n <= 18; ++n) {
        INFO("nivel ", n);
        CHECK(brilloDeNivel(n, 0.0f) <= afuera);
    }
}

TEST_CASE("Brillo: mas luz nunca da menos brillo, a ninguna hora") {
    // La propiedad que faltaba: creciente en el nivel...
    for (float sol = 0.0f; sol <= 1.0f; sol += 0.125f)
        for (int n = 0; n < 18; ++n) {
            INFO("sol ", sol, " nivel ", n);
            CHECK(brilloDeNivel(n, sol) <= brilloDeNivel(n + 1, sol));
        }
    // ...y en la hora: nada se aclara al anochecer.
    for (int n = 0; n <= 18; ++n)
        for (float sol = 0.0f; sol < 1.0f; sol += 0.125f) {
            INFO("nivel ", n, " sol ", sol);
            CHECK(brilloDeNivel(n, sol) <= brilloDeNivel(n, sol + 0.125f));
        }
}

TEST_CASE("Brillo: un cuarto cerrado es lo mas oscuro, y no cambia con la hora") {
    // Sin cielo no hay sol que quitar: igual de oscuro a mediodia que de
    // noche. Es la garantia de "los bloques no dan luz".
    CHECK(brilloDeNivel(0, 1.0f) == doctest::Approx(LUZ_AMBIENTE_MINIMA));
    CHECK(brilloDeNivel(0, 0.0f) == doctest::Approx(LUZ_AMBIENTE_MINIMA));
}

TEST_CASE("Brillo: a cielo abierto de dia sigue valiendo 1, como antes") {
    CHECK(brilloDeNivel(18, 1.0f) == doctest::Approx(1.0f));
    CHECK(brilloDeNivel(18, 1.0f) == doctest::Approx(curvaVieja(18, 1.0f)));
}

TEST_CASE("Brillo: el motor usa la curva unica en los cuatro sitios") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // Ninguna copia local de la curva vieja sobrevive.
    CHECK(src.find("const float propia") == std::string::npos);
    CHECK(src.find("LUZ_DE_LUNA_ITEM") == std::string::npos);
    CHECK(src.find("LUZ_DE_LUNA_CAIDA") == std::string::npos);
    CHECK(src.find("LUZ_DE_LUNA_FAUNA") == std::string::npos);

    // Terreno, items, caida y animales pasan por LuzCielo.h.
    CHECK(src.find("return brilloDeNivel(light, sol);") != std::string::npos);
    CHECK(src.find("brilloDeNivel(nivel, solItems)") != std::string::npos);
    CHECK(src.find("return brilloDeNivel(nivel, solAhora);") != std::string::npos);
    CHECK(src.find("brilloDeLuz(raw, luzSolar())") != std::string::npos);
}

TEST_CASE("Luz: el motor no tiene ninguna fuente de luz por bloque") {
    const std::string src = fuenteMotor();
    REQUIRE_FALSE(src.empty());

    // El sistema de iluminacion global, con su tabla de emisores, ya no esta.
    CHECK(src.find("getBlockEmission") == std::string::npos);
    CHECK(src.find("EmissiveBlock") == std::string::npos);
    CHECK(src.find("propagateTorchlight") == std::string::npos);
    CHECK(src.find("calculateWorldLightingThreaded") == std::string::npos);

    // Nadie escribe torchlight: ni el setter, ni el campo a mano.
    CHECK(src.find("void setTorchlight(") == std::string::npos);
    CHECK(src.find(".torchlight =") == std::string::npos);

    // recalcularLuzLocal ya no arranca a ciegas con cielo pleno.
    CHECK(src.find("luzQueEntraPorArriba(") != std::string::npos);
}
