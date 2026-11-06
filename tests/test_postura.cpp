#include <doctest/doctest.h>
#include "player/Postura.h"
#include "player/PlayerController.h"
#include "player/ModeloJugador.h"
#include "player/MallaJugador.h"

using namespace PlayerSys;

namespace {
// Pulsar y soltar una tecla: dos frames.
ControlPostura::Salida pulsar(ControlPostura& c, bool shift, bool espacio, bool libre = true) {
    c.avanzar(shift, espacio, libre);
    return c.avanzar(false, false, libre);
}
} // namespace

TEST_CASE("Postura: SHIFT una vez agacha y se queda, dos veces tumba") {
    ControlPostura c;
    auto s = pulsar(c, true, false);
    CHECK(s.agachar);
    CHECK_FALSE(s.tumbar);
    // Sin tocar nada, sigue agachado.
    for (int k = 0; k < 100; ++k) s = c.avanzar(false, false, true);
    CHECK(s.agachar);
    s = pulsar(c, true, false);
    CHECK(s.agachar);
    CHECK(s.tumbar);
    CHECK(c.postura == Postura::TUMBADO);
    // Mas SHIFT no hace nada raro.
    s = pulsar(c, true, false);
    CHECK(c.postura == Postura::TUMBADO);
}

TEST_CASE("Postura: ESPACIO levanta de uno en uno y solo salta de pie") {
    ControlPostura c;
    pulsar(c, true, false);
    pulsar(c, true, false);
    REQUIRE(c.postura == Postura::TUMBADO);
    auto s = c.avanzar(false, true, true);           // espacio: a agachado
    CHECK(c.postura == Postura::AGACHADO);
    CHECK_FALSE(s.saltar);
    // Mantenerlo no hace nada mas.
    for (int k = 0; k < 30; ++k) s = c.avanzar(false, true, true);
    CHECK(c.postura == Postura::AGACHADO);
    CHECK_FALSE(s.saltar);
    c.avanzar(false, false, true);
    s = c.avanzar(false, true, true);                // espacio: de pie, sin saltar
    CHECK(c.postura == Postura::DE_PIE);
    CHECK_FALSE(s.saltar);
    CHECK_FALSE(s.agachar);
    s = c.avanzar(false, true, true);                // sigue pulsado: tampoco salta
    CHECK_FALSE(s.saltar);
    c.avanzar(false, false, true);
    s = c.avanzar(false, true, true);                // ahora si
    CHECK(s.saltar);
}

TEST_CASE("Postura: volando o nadando SHIFT vuelve a ser mantenido") {
    ControlPostura c;
    pulsar(c, true, false);
    REQUIRE(c.postura == Postura::AGACHADO);
    auto s = c.avanzar(true, false, false);
    CHECK(s.agachar);                                // bajar mientras se pulsa
    CHECK(c.postura == Postura::DE_PIE);             // y la postura se pierde
    s = c.avanzar(false, false, false);
    CHECK_FALSE(s.agachar);
    s = c.avanzar(false, true, false);
    CHECK(s.saltar);                                 // subir / nadar
}

TEST_CASE("Postura: tumbado la caja y el ojo bajan, y se arrastra despacio") {
    MovementConfig cfg;
    CHECK(cfg.proneHeight < cfg.crouchHeight);
    CHECK(cfg.proneEyeHeight < cfg.proneHeight);
    CHECK(cfg.proneSpeed < cfg.crouchSpeed);
    // El ojo agachado, por debajo del techo de la caja agachada.
    CHECK(cfg.crouchEyeHeight < cfg.crouchHeight - 0.05f);
}

TEST_CASE("Postura: tumbado, el cuerpo queda boca abajo con el pecho en el suelo") {
    using namespace ModeloJugador;
    Estado e;
    e.tumbado = true;
    const Pose p = poseDe(e);
    CHECK(p.tumbado == 1.0f);
    const Esqueleto s = resolver(p);
    // Horizontal: el cuello delante de la pelvis y casi a su altura.
    CHECK(s.cuello.p.z > s.pelvis.p.z + 0.4f);
    CHECK(std::fabs(s.cuello.p.y - s.pelvis.p.y) < 0.15f);
    // Bajo: nada del tronco por encima de 0.35.
    CHECK(s.pelvis.p.y < 0.30f);
    CHECK(s.cuello.p.y < 0.35f);
    // Los pies, detras.
    CHECK(s.pie[0].tobillo.p.z < s.pelvis.p.z - 0.6f);
    // Malla bien formada y sin hundirse.
    Malla m;
    Fisiologia f;
    generar(s, p, generarPiel(7u), f, Calidad{}, m);
    float yMin = 9;
    for (const VerticeJ& v : m.v) yMin = std::fmin(yMin, v.p.y);
    CHECK(yMin > -0.06f);
    // Arrastrarse alterna codo y rodilla contrarios.
    e.rapidez = 0.75f;
    e.fasePaso = 1.5708f;
    const Pose q = poseDe(e);
    CHECK(q.brazo[0].flex > q.brazo[1].flex);
    CHECK(q.pierna[1].rodilla > q.pierna[0].rodilla);
}

TEST_CASE("Postura: agachado, el cuello queda detras del ojo aunque el torso se incline") {
    using namespace ModeloJugador;
    Estado e;
    e.agachado = true;
    const Esqueleto s = resolver(poseDe(e));
    // El anclaje del render pone el cuello ATRAS por detras del ojo; los
    // hombros no pueden quedar por delante del cuello mas que su grosor.
    for (int l = 0; l < 2; ++l) CHECK(s.hombro[l].p.z - s.cuello.p.z < 0.06f);
    const float ky = escalaVertical(s, 0.86f);
    CHECK(s.cuello.p.y * ky < 0.86f - 0.08f);
}

TEST_CASE("Postura: CTRL tocado y W anda con sigilo; SHIFT encima, inspecciona") {
    ControlPostura c;
    double t = 0.0;
    auto paso = [&](bool shift, bool espacio, bool ctrl, bool w) {
        t += 0.05;
        return c.avanzar(shift, espacio, true, ctrl, w, t);
    };
    // Toque de CTRL: pulsar y soltar sin W.
    paso(false, false, true, false);
    auto s = paso(false, false, false, false);
    CHECK(s.sigilo);
    s = paso(false, false, false, true);            // ahora W: anda despacio
    CHECK(s.sigilo);
    CHECK_FALSE(s.correr);
    // SHIFT en sigilo: inspecciona, no se agacha.
    paso(true, false, false, true);
    s = paso(false, false, false, true);
    CHECK(s.inspeccion);
    CHECK_FALSE(s.agachar);
    CHECK(c.postura == Postura::DE_PIE);
    // ESPACIO: se endereza sin saltar.
    s = paso(false, true, false, true);
    CHECK_FALSE(s.inspeccion);
    CHECK_FALSE(s.saltar);
    CHECK(s.sigilo);
    paso(false, false, false, false);
    // Otro toque de CTRL lo quita.
    paso(false, false, true, false);
    s = paso(false, false, false, false);
    CHECK_FALSE(s.sigilo);
    // Y fuera del sigilo SHIFT vuelve a agachar.
    paso(true, false, false, false);
    s = paso(false, false, false, false);
    CHECK(s.agachar);
}

TEST_CASE("Postura: CTRL mantenido con W sigue siendo correr") {
    ControlPostura c;
    double t = 0.0;
    auto paso = [&](bool ctrl, bool w) { t += 0.05; return c.avanzar(false, false, true, ctrl, w, t); };
    paso(true, false);
    auto s = paso(true, true);        // CTRL pulsado y luego W, sin soltar
    CHECK(s.correr);
    CHECK_FALSE(s.sigilo);
    paso(false, true);                // suelta CTRL: no era un toque
    s = paso(false, true);
    CHECK_FALSE(s.sigilo);
    // Un CTRL largo (mas de medio segundo) tampoco es un toque.
    paso(true, false);
    for (int k = 0; k < 15; ++k) paso(true, false);
    s = paso(false, false);
    CHECK_FALSE(s.sigilo);
}

TEST_CASE("Postura: sigilo e inspeccion, mas lentos que andar") {
    MovementConfig cfg;
    CHECK(cfg.sigiloSpeed < cfg.walkSpeed);
    CHECK(cfg.inspeccionSpeed < cfg.sigiloSpeed);
    CHECK(cfg.inspeccionEyeHeight < cfg.eyeHeight - 0.2f);
    CHECK(cfg.inspeccionEyeHeight > cfg.crouchEyeHeight);
}
