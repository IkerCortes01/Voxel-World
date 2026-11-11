#include <doctest/doctest.h>
#include "player/Corazon.h"
#include "player/Temperatura.h"
#include <string>

// ============================================================================
// EL CORAZON Y LA TEMPERATURA DEL JUGADOR
// ============================================================================

using namespace CorazonJugador;
using namespace TemperaturaJugador;

namespace {
Corazon tras(float segundos, const Entradas& e) {
    Corazon c;
    for (float t = 0; t < segundos; t += 0.05f) c.avanzar(0.05f, e);
    return c;
}
} // namespace

TEST_CASE("Corazon: quieto y andando, normal; corriendo sube y sube mas cuanto mas rato") {
    Entradas quieto;
    Entradas andando; andando.rapidez = 4.3f;
    Entradas corre; corre.rapidez = 5.6f; corre.corriendo = true;
    CHECK(tras(30, quieto).pulso == doctest::Approx(70).epsilon(0.05));
    CHECK(tras(30, andando).pulso == doctest::Approx(78).epsilon(0.05));
    const float poco = tras(10, corre).pulso, mucho = tras(70, corre).pulso;
    CHECK(poco > 85.0f);
    CHECK(mucho > poco + 25.0f);
}

TEST_CASE("Corazon: durmiendo va lento; en lo alto fuerte pero no tan rapido") {
    Entradas duerme; duerme.durmiendo = true;
    const Corazon d = tras(60, duerme);
    CHECK(d.pulso < 58.0f);
    CHECK(d.fuerza < 0.4f);
    Entradas alto; alto.caida = 30.0f;
    const Corazon a = tras(30, alto);
    CHECK(a.fuerza > 0.8f);
    CHECK(a.pulso < 95.0f);
    CHECK(a.causa == Corazon::ALTURA);
}

TEST_CASE("Corazon: el frio acelera; el calor solo en los extremos; el miedo, segun la amenaza") {
    Entradas f; f.temperatura = -4.0f;
    CHECK(tras(30, f).pulso > 85.0f);
    Entradas cal; cal.temperatura = 3.0f;
    CHECK(tras(30, cal).pulso == doctest::Approx(70).epsilon(0.05));
    cal.temperatura = 5.0f;
    CHECK(tras(30, cal).pulso > 100.0f);
    Entradas poco; poco.amenaza = 0.3f;
    Entradas mucho; mucho.amenaza = 1.8f;
    const Corazon a = tras(20, poco), b = tras(20, mucho);
    CHECK(b.pulso > a.pulso + 30.0f);
    CHECK(b.fuerza > a.fuerza);
    CHECK(b.causa == Corazon::MIEDO);
}

TEST_CASE("Corazon: el latido es doble (lub-dub)") {
    Corazon c;
    float primero = 0, segundo = 0;
    for (int i = 0; i < 100; ++i) {
        c.fase = i / 100.0f;
        if (c.fase < 0.15f) primero = std::fmax(primero, c.latido());
        else if (c.fase > 0.18f && c.fase < 0.32f) segundo = std::fmax(segundo, c.latido());
    }
    CHECK(primero > 0.9f);
    CHECK(segundo > 0.4f);
    CHECK(segundo < primero);
}

TEST_CASE("Temperatura: 11 niveles con nombre, templado en medio") {
    CHECK(std::string(nombreNivel(0)) == "TEMPLADO");
    CHECK(std::string(nombreNivel(-5)) == "DEMASIADO FRIO");
    CHECK(std::string(nombreNivel(5)) == "DEMASIADO CALOR");
    CHECK(std::string(nombreNivel(-2)) == "POCO FRIO");
    // Los extremos: azul marino y naranja rojizo.
    const Color frio = colorNivel(-5.0f), calor = colorNivel(5.0f);
    CHECK(frio.b > frio.r * 3.0f);
    CHECK(frio.b < 0.6f);
    CHECK(calor.r > calor.g * 2.0f);
}

TEST_CASE("Temperatura: desierto al mediodia da calor; noche, nieve, agua y altura dan frio") {
    Entorno desierto; desierto.bioma = 0.95f; desierto.hora = 14.0f;
    CHECK(ambiente(desierto) >= 3.0f);
    Entorno templado;
    CHECK(std::fabs(ambiente(templado)) < 2.0f);
    Entorno noche = templado; noche.hora = 2.0f;
    CHECK(ambiente(noche) < ambiente(templado) - 2.0f);
    Entorno nevando = templado; nevando.nieve = 1.0f;
    CHECK(ambiente(nevando) < ambiente(templado) - 1.5f);
    Entorno agua = templado; agua.enAgua = true;
    CHECK(ambiente(agua) < ambiente(templado) - 1.5f);
    Entorno cumbre = templado; cumbre.altura = 200.0f;
    CHECK(ambiente(cumbre) < ambiente(templado) - 3.0f);
    // Bajo techo abriga de noche; el fuego calienta.
    Entorno casa = noche; casa.cielo = 0.0f;
    CHECK(ambiente(casa) > ambiente(noche) + 1.0f);
    Entorno fogata = noche; fogata.fuego = 1.0f;
    CHECK(ambiente(fogata) > ambiente(noche) + 3.0f);
}

TEST_CASE("Temperatura: el cuerpo sigue al ambiente con inercia, y mojado se enfria antes") {
    Entorno nieve; nieve.bioma = 0.1f; nieve.nieve = 1.0f; nieve.hora = 3.0f;
    Estado seco, mojado;
    mojado.mojado = 1.0f;
    for (int i = 0; i < 20 * 10; ++i) { seco.avanzar(0.05f, nieve, 0.0f); Entorno m = nieve; m.lluvia = 0; mojado.avanzar(0.05f, m, 0.0f); }
    CHECK(seco.cuerpo < -0.5f);
    CHECK(seco.cuerpo > -4.0f);       // en 10 s no llega aun
    CHECK(mojado.cuerpo < seco.cuerpo);
}

TEST_CASE("Temperatura: los efectos crecen con el nivel") {
    CHECK(efectos(0.0f).tinte[3] == 0.0f);
    CHECK(efectos(-5.0f).temblor > 0.9f);
    CHECK(efectos(-1.0f).temblor == 0.0f);
    CHECK(efectos(5.0f).ondulacion > 0.9f);
    CHECK(efectos(5.0f).tinte[3] > efectos(2.0f).tinte[3]);
    CHECK(efectos(-5.0f).vineta > 0.9f);
    CHECK(efectos(-5.0f).campo < 0.0f);
}