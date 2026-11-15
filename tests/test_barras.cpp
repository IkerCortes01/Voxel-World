// Las tres barras del jugador (src/player/Barras.h): vida, resistencia y
// energia. Cabecera sin motor: se prueba la de verdad.
#include "doctest/doctest.h"

#include "player/Barras.h"

using namespace BarrasJugador;

namespace {
void pasar(Estado& s, const Entradas& e, float segundos) {
    for (int i = 0; i < (int)(segundos * 60.0f + 0.5f); ++i) s.avanzar(1.0f / 60.0f, e);
}
}  // namespace

TEST_CASE("Barras: las tres empiezan en 100") {
    Estado s;
    CHECK(s.vida == 100.0f);
    CHECK(s.resistencia == 100.0f);
    CHECK(s.energia == 100.0f);
    CHECK(s.puedeCorrer());
    CHECK_FALSE(s.sinVida());
}

TEST_CASE("Barras: caer de poca altura no duele; de mucha, si, y mas cuanto mas alto") {
    CHECK(Estado::danoDeCaida(1.0f) == 0.0f);
    CHECK(Estado::danoDeCaida(3.5f) == 0.0f);
    CHECK(Estado::danoDeCaida(5.0f) == doctest::Approx(13.5f));
    CHECK(Estado::danoDeCaida(10.0f) > Estado::danoDeCaida(6.0f));
    Estado s;
    s.aterrizar(3.0f);
    CHECK(s.vida == 100.0f);
    s.aterrizar(8.0f);
    CHECK(s.vida == doctest::Approx(100.0f - 4.5f * 9.0f));
    CHECK(s.ultimaCausa == Causa::CAIDA);
    CHECK(s.golpe == 1.0f);
    s.aterrizar(40.0f);
    CHECK(s.vida == 0.0f);          // nunca baja de cero
    CHECK(s.sinVida());
}

TEST_CASE("Barras: un animal quita vida, y despues de un rato sin dano vuelve sola") {
    Estado s;
    s.danar(30.0f, Causa::ANIMAL);
    CHECK(s.vida == doctest::Approx(70.0f));
    Entradas quieto;
    pasar(s, quieto, SIN_DANO_PARA_CURAR - 1.0f);
    CHECK(s.vida == doctest::Approx(70.0f));               // todavia no
    pasar(s, quieto, 11.0f);
    CHECK(s.vida > 76.0f);
    CHECK(s.vida < 80.0f);
    // Durmiendo, mas deprisa.
    Estado d;
    d.danar(30.0f, Causa::ANIMAL);
    Entradas dormido;
    dormido.durmiendo = true;
    pasar(d, dormido, SIN_DANO_PARA_CURAR + 10.0f);
    CHECK(d.vida > s.vida + 10.0f);
    pasar(d, dormido, 60.0f);
    CHECK(d.vida == 100.0f);                               // y no pasa de 100
}

TEST_CASE("Barras: bajo el agua aguanta 20 s y luego se ahoga; al salir recupera el aire") {
    Estado s;
    Entradas buceo;
    buceo.nadando = buceo.bajoAgua = true;
    pasar(s, buceo, AIRE_SEG - 1.0f);
    CHECK(s.vida == 100.0f);
    pasar(s, buceo, 6.0f);
    CHECK(s.vida < 75.0f);
    CHECK(s.ultimaCausa == Causa::AHOGO);
    Entradas fuera;
    pasar(s, fuera, 5.0f);
    CHECK(s.aire == doctest::Approx(AIRE_SEG));
}

TEST_CASE("Barras: la lava, el fuego y la temperatura extrema quitan vida") {
    Entradas lava; lava.enLava = true;
    Estado a; pasar(a, lava, 2.0f);
    CHECK(a.vida == doctest::Approx(50.0f).epsilon(0.02));
    CHECK(a.ultimaCausa == Causa::LAVA);

    Entradas fuego; fuego.fuego = 1.0f;
    Estado b; pasar(b, fuego, 5.0f);
    CHECK(b.vida == doctest::Approx(80.0f).epsilon(0.02));
    Entradas lejos; lejos.fuego = 0.3f;                    // el horno a unos pasos no quema
    Estado c; pasar(c, lejos, 5.0f);
    CHECK(c.vida == 100.0f);

    Entradas frio; frio.temperatura = -5.0f;
    Estado f; pasar(f, frio, 10.0f);
    CHECK(f.vida == doctest::Approx(85.0f).epsilon(0.02));
    CHECK(f.ultimaCausa == Causa::FRIO);
    Entradas fresco; fresco.temperatura = -3.0f;
    Estado g; pasar(g, fresco, 10.0f);
    CHECK(g.vida == 100.0f);
    Entradas calor; calor.temperatura = 5.0f;
    Estado h; pasar(h, calor, 10.0f);
    CHECK(h.ultimaCausa == Causa::CALOR);
}

TEST_CASE("Barras: correr y saltar gastan energia; a cero se agota y no puede correr") {
    Estado s;
    Entradas corre;
    corre.corriendo = true; corre.rapidez = 5.6f;
    pasar(s, corre, 5.0f);
    CHECK(s.energia == doctest::Approx(65.0f).epsilon(0.02));
    s.saltar();
    CHECK(s.energia == doctest::Approx(60.0f).epsilon(0.02));
    CHECK(s.puedeCorrer());
    pasar(s, corre, 12.0f);
    CHECK(s.energia == 0.0f);
    CHECK_FALSE(s.puedeCorrer());
    // Recupera un poco: sigue agotado hasta llegar a 20.
    Entradas quieto;
    pasar(s, quieto, 1.0f);
    CHECK(s.energia == doctest::Approx(9.0f).epsilon(0.03));
    CHECK_FALSE(s.puedeCorrer());
    pasar(s, quieto, 1.5f);
    CHECK(s.puedeCorrer());
    // Saltar sin energia no la deja negativa.
    Estado v; v.energia = 2.0f;
    v.saltar();
    CHECK(v.energia == 0.0f);
}

TEST_CASE("Barras: se recarga parado, dormido y andando con sigilo; andando normal, no") {
    auto tras = [](const Entradas& e) {
        Estado s;
        s.energia = 10.0f;
        pasar(s, e, 4.0f);
        return s.energia;
    };
    Entradas quieto;
    Entradas dormido; dormido.durmiendo = true;
    Entradas sigilo; sigilo.sigilo = true; sigilo.rapidez = 1.5f;
    Entradas andando; andando.rapidez = 4.3f;
    Entradas saltando; saltando.rapidez = 0.0f; saltando.enSuelo = false;
    CHECK(tras(quieto) == doctest::Approx(46.0f).epsilon(0.02));
    CHECK(tras(dormido) == doctest::Approx(74.0f).epsilon(0.02));
    CHECK(tras(sigilo) == doctest::Approx(30.0f).epsilon(0.02));
    CHECK(tras(andando) == doctest::Approx(10.0f));
    CHECK(tras(saltando) == doctest::Approx(10.0f));       // en el aire no descansa
    CHECK(tras(dormido) > tras(quieto));
    CHECK(tras(quieto) > tras(sigilo));
    // Golpear gasta, aunque este parado.
    Entradas picando; picando.golpeando = true;
    CHECK(tras(picando) < 10.0f);
    // Nadar gasta.
    Entradas nada; nada.nadando = true; nada.rapidez = 2.0f;
    CHECK(tras(nada) == 0.0f);
}

TEST_CASE("Barras: la resistencia todavia no se mueve con nada") {
    Estado s;
    Entradas corre; corre.corriendo = true; corre.rapidez = 5.6f; corre.golpeando = true;
    pasar(s, corre, 30.0f);
    s.danar(20.0f, Causa::ANIMAL);
    s.saltar();
    CHECK(s.resistencia == 100.0f);
}

TEST_CASE("Barras: en creativo no baja nada") {
    Estado s;
    s.vida = 40.0f; s.energia = 0.0f; s.agotado = true;
    Entradas e;
    e.creativo = true; e.enLava = true; e.bajoAgua = true; e.corriendo = true; e.rapidez = 6.0f;
    pasar(s, e, 30.0f);
    CHECK(s.vida == 100.0f);
    CHECK(s.energia == 100.0f);
    CHECK(s.puedeCorrer());
}
