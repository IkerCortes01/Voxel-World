#include <doctest/doctest.h>
#include "Dormir.h"
#include <vector>
#include <set>

// ============================================================================
// DORMIR: LAS REGLAS DEL REFUGIO Y EL CICLO DEL SUENO
// ============================================================================
// Toda la logica de Dormir.h es pura --no toca World, ni OpenGL, ni el reloj--
// asi que se puede probar entera aqui. Lo que se fija es lo que el jugador
// notaria si se rompiera:
//
//   sin la regla del techo    -> se puede dormir a cielo abierto y la noche
//                                deja de existir como problema
//   sin la del 1x1            -> basta cavar dos golpes en una pared
//   con la curva mal          -> la camara baja de golpe o el fundido parpadea
//   con la hora mal           -> el reloj retrocede y el dia salta al pasado

// ----------------------------------------------------------------------------
// UN MUNDO DE JUGUETE
// ----------------------------------------------------------------------------
// Lo minimo para responder a las dos preguntas que hace PuedeDormir. Se llena
// a mano en cada test, asi que cada escenario dice exactamente que hay donde.
struct MundoFalso {
    std::set<std::tuple<int,int,int>> solidos;
    std::set<std::tuple<int,int,int>> liquidos;

    void ponSolido(int x, int y, int z) { solidos.insert({x,y,z}); }
    void ponLiquido(int x, int y, int z) { liquidos.insert({x,y,z}); }

    bool esSolido(int x, int y, int z) const {
        return solidos.count({x,y,z}) != 0;
    }
    bool esLiquido(int x, int y, int z) const {
        return liquidos.count({x,y,z}) != 0;
    }

    // Una habitacion cerrada alrededor de (cx,cy,cz): suelo, techo y cuatro
    // paredes, con `radio` celdas de hueco a cada lado.
    void habitacion(int cx, int cy, int cz, int radio) {
        for (int x = cx - radio - 1; x <= cx + radio + 1; ++x)
            for (int z = cz - radio - 1; z <= cz + radio + 1; ++z) {
                ponSolido(x, cy - 1, z);        // suelo
                ponSolido(x, cy + 3, z);        // techo
                // Paredes: el borde del rectangulo.
                if (x == cx - radio - 1 || x == cx + radio + 1 ||
                    z == cz - radio - 1 || z == cz + radio + 1) {
                    for (int y = cy; y <= cy + 2; ++y) ponSolido(x, y, z);
                }
            }
    }

    Dormir::Motivo prueba(int px, int py, int pz) const {
        return Dormir::PuedeDormir(px, py, pz,
            [this](int x,int y,int z){ return esSolido(x,y,z); },
            [this](int x,int y,int z){ return esLiquido(x,y,z); });
    }
};

// ============================================================================
// DONDE SE PUEDE Y DONDE NO
// ============================================================================

TEST_CASE("Dormir: en una habitacion cerrada se puede") {
    // El caso que tiene que funcionar. Si este falla, la funcion es
    // inservible por muy bien que rechace lo demas.
    MundoFalso m;
    m.habitacion(0, 64, 0, 1);   // 3x3 de hueco
    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::PUEDE);
}

TEST_CASE("Dormir: a cielo abierto NO se puede") {
    // ⭐ LA REGLA QUE SOSTIENE TODO LO DEMAS.
    //
    // Sin ella basta pulsar Z en cualquier parte para saltarse la noche, y el
    // refugio deja de tener sentido como coste.
    MundoFalso m;
    // Solo suelo: nada encima.
    for (int x = -4; x <= 4; ++x)
        for (int z = -4; z <= 4; ++z)
            m.ponSolido(x, 63, z);

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::SIN_TECHO);
}

TEST_CASE("Dormir: con techo pero sin paredes NO se puede") {
    // Un tejado sobre cuatro postes no es un refugio.
    MundoFalso m;
    for (int x = -4; x <= 4; ++x)
        for (int z = -4; z <= 4; ++z) {
            m.ponSolido(x, 63, z);        // suelo
            m.ponSolido(x, 67, z);        // techo alto
        }
    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::SIN_PAREDES);
}

TEST_CASE("Dormir: en un agujero de 1x1 NO se puede") {
    // ⭐ EL CASO QUE SE PIDIO EXPLICITAMENTE.
    //
    // Un hueco de un bloque tiene techo y le sobran paredes -- las cuatro --
    // pero no hay donde tumbarse. Cavar 1x1 son dos golpes, asi que sin esta
    // regla el requisito del refugio no valdria nada.
    MundoFalso m;
    // Roca maciza en todo el volumen...
    for (int x = -2; x <= 2; ++x)
        for (int y = 60; y <= 70; ++y)
            for (int z = -2; z <= 2; ++z)
                m.ponSolido(x, y, z);

    // ...menos la columna del jugador: dos celdas de alto, una de ancho.
    m.solidos.erase({0, 64, 0});
    m.solidos.erase({0, 65, 0});

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::SIN_ESPACIO);
}

TEST_CASE("Dormir: en un hueco de 1x2 SI se puede") {
    // La frontera del caso anterior: en cuanto hay UNA celda contigua libre,
    // ya hay donde tenderse. Es lo que separa un nicho de una habitacion
    // pequena, y conviene que la frontera este fijada por un test.
    MundoFalso m;
    for (int x = -2; x <= 2; ++x)
        for (int y = 60; y <= 70; ++y)
            for (int z = -2; z <= 2; ++z)
                m.ponSolido(x, y, z);

    // Dos celdas contiguas, las dos de dos de alto.
    m.solidos.erase({0, 64, 0});
    m.solidos.erase({0, 65, 0});
    m.solidos.erase({1, 64, 0});
    m.solidos.erase({1, 65, 0});

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::PUEDE);
}

TEST_CASE("Dormir: hacen falta 3 paredes, no 4") {
    // Con las cuatro cerradas no se puede ni construir la habitacion sin
    // quedarse dentro, asi que se pide una menos: una entrada abierta vale.
    MundoFalso m;
    m.habitacion(0, 64, 0, 1);

    // Se abre un boquete en una pared, a la altura del pecho.
    for (int y = 64; y <= 66; ++y) m.solidos.erase({2, y, 0});

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::PUEDE);
}

TEST_CASE("Dormir: con dos paredes abiertas ya NO se puede") {
    // El otro lado de la frontera. Un cobertizo con dos lados al aire no
    // protege de nada.
    MundoFalso m;
    m.habitacion(0, 64, 0, 1);

    for (int y = 64; y <= 66; ++y) {
        m.solidos.erase({ 2, y, 0});
        m.solidos.erase({-2, y, 0});
    }
    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::SIN_PAREDES);
}

TEST_CASE("Dormir: en el aire NO se puede") {
    // Sin suelo bajo los pies no hay donde tumbarse, aunque haya techo.
    MundoFalso m;
    m.habitacion(0, 64, 0, 1);
    m.solidos.erase({0, 63, 0});          // se quita el suelo justo debajo

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::EN_EL_AIRE);
}

TEST_CASE("Dormir: dentro del agua NO se puede") {
    MundoFalso m;
    m.habitacion(0, 64, 0, 1);
    m.ponLiquido(0, 64, 0);

    CHECK(m.prueba(0, 64, 0) == Dormir::Motivo::EN_AGUA);
}

TEST_CASE("Dormir: el motivo siempre tiene un texto que mostrar") {
    // El aviso dice POR QUE no se puede dormir. Un motivo sin texto dejaria al
    // jugador con un recuadro vacio.
    const Dormir::Motivo todos[] = {
        Dormir::Motivo::SIN_TECHO, Dormir::Motivo::SIN_PAREDES,
        Dormir::Motivo::SIN_ESPACIO, Dormir::Motivo::EN_EL_AIRE,
        Dormir::Motivo::EN_AGUA
    };
    for (Dormir::Motivo m : todos) {
        const char* t = Dormir::TextoDeMotivo(m);
        INFO("motivo ", (int)m);
        REQUIRE(t != nullptr);
        CHECK(t[0] != '\0');
    }
    // El caso "si se puede" no muestra nada.
    CHECK(Dormir::TextoDeMotivo(Dormir::Motivo::PUEDE)[0] == '\0');
}

// ============================================================================
// EL CICLO DEL SUENO
// ============================================================================

TEST_CASE("Dormir: la camara baja de arriba a abajo, sin saltos") {
    // Empieza de pie, acaba tumbada, y no retrocede por el camino. Un tramo no
    // monotono se veria como un tiron a mitad de acostarse.
    CHECK(Dormir::AlturaCamara(0.0f) == doctest::Approx(0.0f));
    CHECK(Dormir::AlturaCamara(Dormir::BAJADA_CAMARA) == doctest::Approx(1.0f));
    CHECK(Dormir::AlturaCamara(99.0f) == doctest::Approx(1.0f));

    float ant = -1.0f;
    for (float t = 0.0f; t <= Dormir::BAJADA_CAMARA + 0.5f; t += 0.05f) {
        const float v = Dormir::AlturaCamara(t);
        INFO("t=", t, " v=", v);
        CHECK(v >= ant - 1e-5f);       // nunca va hacia atras
        CHECK(v >= 0.0f);
        CHECK(v <= 1.0f);
        ant = v;
    }
}

TEST_CASE("Dormir: la camara arranca y frena despacio") {
    // Es un smoothstep: al principio y al final se mueve MENOS que en el
    // medio. Con una rampa lineal el movimiento empieza y para en seco, que se
    // ve mecanico en vez de como un cuerpo que se acuesta.
    const float B = Dormir::BAJADA_CAMARA;
    const float alPrincipio = Dormir::AlturaCamara(B * 0.1f);
    const float enMedio     = Dormir::AlturaCamara(B * 0.5f);
    const float alFinal     = Dormir::AlturaCamara(B * 0.9f);

    // A la decima parte del tiempo ha recorrido MUCHO menos de la decima parte.
    CHECK(alPrincipio < 0.10f);
    CHECK(enMedio == doctest::Approx(0.5f).epsilon(0.01f));
    CHECK(alFinal > 0.90f);
}

TEST_CASE("Dormir: la pantalla se oscurece despues de empezar a acostarse") {
    // Primero se ve el movimiento, luego se apaga la luz. Si el fundido
    // empezara en t=0 taparia justo la parte que se quiere ver.
    CHECK(Dormir::Oscuridad(0.0f) == doctest::Approx(0.0f));
    CHECK(Dormir::FUNDIDO_INICIO > 0.0f);
    CHECK(Dormir::Oscuridad(Dormir::FUNDIDO_INICIO) == doctest::Approx(0.0f));

    // Y acaba en negro total antes de que termine el sueno, para que no se
    // despierte a mitad de un fundido.
    CHECK(Dormir::FUNDIDO_FIN < Dormir::DURACION);
    CHECK(Dormir::Oscuridad(Dormir::FUNDIDO_FIN) == doctest::Approx(1.0f));
    CHECK(Dormir::Oscuridad(Dormir::DURACION) == doctest::Approx(1.0f));
}

TEST_CASE("Dormir: el fundido es monotono") {
    // Un retroceso aqui se veria como un parpadeo de la pantalla.
    float ant = -1.0f;
    for (float t = 0.0f; t <= Dormir::DURACION; t += 0.1f) {
        const float v = Dormir::Oscuridad(t);
        INFO("t=", t, " v=", v);
        CHECK(v >= ant - 1e-5f);
        CHECK(v >= 0.0f);
        CHECK(v <= 1.0f);
        ant = v;
    }
}

TEST_CASE("Dormir: dura 10 segundos") {
    // Lo pedido, fijado para que no se mueva sin querer.
    CHECK(Dormir::DURACION == doctest::Approx(10.0f));
    CHECK_FALSE(Dormir::Terminado(9.9f));
    CHECK(Dormir::Terminado(10.0f));
    CHECK(Dormir::Terminado(12.0f));
}

TEST_CASE("Dormir: soltar a los 5-6 segundos ya cuenta como haber dormido") {
    // Lo pedido: despertarse antes de tiempo tambien adelanta el reloj, solo
    // que a una hora mas temprana.
    CHECK(Dormir::DESPERTAR_TEMPRANO == doctest::Approx(5.0f));

    CHECK_FALSE(Dormir::CuentaComoDormido(2.0f));
    CHECK_FALSE(Dormir::CuentaComoDormido(4.9f));
    CHECK(Dormir::CuentaComoDormido(5.0f));
    CHECK(Dormir::CuentaComoDormido(6.0f));      // el caso citado
    CHECK(Dormir::CuentaComoDormido(9.0f));
}

TEST_CASE("Dormir: se despierta mas temprano si se corta el sueno") {
    // Las dos salidas del ciclo dan HORAS DISTINTAS, y esa diferencia es lo
    // que hace que dormir del tiron merezca la pena.
    const double completo = Dormir::HoraAlDespertar(true);
    const double cortado  = Dormir::HoraAlDespertar(false);

    INFO("completo ", completo, " cortado ", cortado);
    CHECK(cortado < completo);

    // Las dos son de madrugada o manana temprana: dormir nunca deja al jugador
    // en mitad de la tarde.
    CHECK(cortado >= 4.0);
    CHECK(completo <= 9.0);
}

TEST_CASE("Dormir: el fundido acaba antes de que termine el sueno") {
    // Coherencia entre las tres constantes de tiempo. Si el fundido durara mas
    // que el sueno, el jugador despertaria con la pantalla a medio apagar.
    CHECK(Dormir::FUNDIDO_INICIO < Dormir::FUNDIDO_FIN);
    CHECK(Dormir::FUNDIDO_FIN <= Dormir::DURACION);
    CHECK(Dormir::BAJADA_CAMARA < Dormir::DURACION);
    // Y el punto de "ya cuenta" cae dentro del sueno, no despues.
    CHECK(Dormir::DESPERTAR_TEMPRANO < Dormir::DURACION);
}
