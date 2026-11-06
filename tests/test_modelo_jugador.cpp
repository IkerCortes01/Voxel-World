#include <doctest/doctest.h>
#include "player/ModeloJugador.h"
#include "player/MallaJugador.h"
#include <cmath>

using namespace ModeloJugador;

namespace {
float dist(V a, V b) { return largo(resta(a, b)); }

// Todos los giros de una pose dentro de los rangos humanos.
bool dentroDeLimites(const Pose& p) {
    using namespace Lim;
    auto en = [](float v, float lo, float hi) { return v >= lo - 1e-5f && v <= hi + 1e-5f; };
    bool ok = en(p.giroTorso, -TORSO_GIRO, TORSO_GIRO) && en(p.cuelloFlex, -CUELLO_FLEX, CUELLO_FLEX) &&
              en(p.inclinacion, INCLINA_MIN, INCLINA_MAX);
    for (const Brazo& b : p.brazo) {
        ok = ok && en(b.flex, HOMBRO_FLEX_MIN, HOMBRO_FLEX_MAX) && en(b.codo, 0, CODO_MAX) &&
             en(b.munFlex, MUNECA_FLEX_MIN, MUNECA_FLEX_MAX) && en(b.prono, PRONO_MIN, PRONO_MAX);
        for (int d = 0; d < 5; ++d)
            for (int f = 0; f < 3; ++f) {
                const auto& L = d == 4 ? PULGAR : DEDO;
                ok = ok && en(b.dedo[d][f], L[f][0], L[f][1]);
            }
    }
    for (const Pierna& g : p.pierna)
        ok = ok && en(g.flex, CADERA_FLEX_MIN, CADERA_FLEX_MAX) && en(g.rodilla, 0, RODILLA_MAX) &&
             en(g.tobillo, TOBILLO_MIN, TOBILLO_MAX) && en(g.dedos, DEDOS_PIE_MIN, DEDOS_PIE_MAX);
    return ok;
}

Estado andando(float fase, float rapidez = 4.3f) {
    Estado e;
    e.fasePaso = fase;
    e.rapidez = rapidez;
    return e;
}
} // namespace

TEST_CASE("Jugador: de pie, la planta toca el suelo y el ojo queda a la altura de la camara") {
    const Esqueleto s = resolver(poseDe(Estado{}));
    CHECK(detalle::plantaMasBaja(s) == doctest::Approx(0.0f).epsilon(0.001));
    CHECK(s.ojo == doctest::Approx(M::OJO).epsilon(0.03));
    CHECK(escalaVertical(s, 1.62f) == doctest::Approx(1.0f).epsilon(0.03));
}

TEST_CASE("Jugador: ningun hueso se despega ni se estira, en ninguna pose") {
    for (int k = 0; k < 40; ++k) {
        Estado e = andando(k * 0.37f, (k % 5) * 1.5f);
        e.golpeando = k % 3 == 0;
        e.agachado = k % 7 == 0;
        e.tiempo = k * 0.13f;
        const Esqueleto s = resolver(poseDe(e));
        for (int l = 0; l < 2; ++l) {
            CHECK(dist(s.hombro[l].p, s.codo[l].p) == doctest::Approx(M::BRAZO).epsilon(0.001));
            CHECK(dist(s.codo[l].p, s.mano[l].muneca.p) == doctest::Approx(M::ANTEBRAZO).epsilon(0.001));
            CHECK(dist(s.cadera[l].p, s.rodilla[l].p) == doctest::Approx(M::MUSLO).epsilon(0.001));
            CHECK(dist(s.rodilla[l].p, s.pie[l].tobillo.p) == doctest::Approx(M::PIERNA).epsilon(0.001));
            for (int d = 0; d < 5; ++d)
                for (int f = 0; f < 3; ++f)
                    CHECK(dist(s.mano[l].dedo[d][f].p, s.mano[l].dedo[d][f + 1].p) ==
                          doctest::Approx(M::DEDO[d][f]).epsilon(0.001));
        }
    }
}

TEST_CASE("Jugador: todas las articulaciones dentro de los limites humanos") {
    for (int k = 0; k < 200; ++k) {
        Estado e = andando(k * 0.29f, (k % 9) * 0.9f);
        e.enSuelo = k % 11 != 0;
        e.volando = k % 13 == 0;
        e.golpeando = k % 4 == 0;
        e.agachado = k % 6 == 0;
        e.tiempo = k * 0.077f;
        e.giroCuello = std::sin(k * 0.5f) * 3.0f;      // mas de lo posible
        e.mirada = -90.0f + (k % 19) * 10.0f;
        e.faseRespira = k * 0.4f;
        CHECK(dentroDeLimites(poseDe(e)));
    }
    // limitar corrige lo imposible: un codo hacia atras, una rodilla al reves.
    Pose p;
    p.brazo[0].codo = -1.0f;
    p.pierna[1].rodilla = -0.5f;
    p.brazo[1].dedo[1][1] = -1.0f;
    limitar(p);
    CHECK(p.brazo[0].codo == 0.0f);
    CHECK(p.pierna[1].rodilla == 0.0f);
    CHECK(p.brazo[1].dedo[1][1] == 0.0f);
}

TEST_CASE("Jugador: al andar, cada brazo va con la pierna contraria") {
    // fasePaso = PI: la derecha apoya el talon (pierna delante).
    const Pose p = poseDe(andando(PI));
    CHECK(p.pierna[1].flex > p.pierna[0].flex + 0.3f);
    CHECK(p.brazo[0].flex > p.brazo[1].flex + 0.2f);
    // Parado no mueve las piernas.
    const Pose q = poseDe(Estado{});
    CHECK(std::fabs(q.pierna[0].flex) < 1e-4f);
}

TEST_CASE("Jugador: la marcha tiene sus picos de rodilla, tobillo y dedos") {
    // Ciclo de la pierna izquierda: t = fasePaso / 2PI.
    auto en = [](float t) { return poseDe(andando(t * 2.0f * PI)).pierna[0]; };
    CHECK(en(0.72f).rodilla > 0.9f);                    // la pierna pasa doblada
    CHECK(en(0.13f).rodilla > en(0.30f).rodilla);       // el pequeno pico al cargar
    CHECK(en(0.62f).tobillo < -0.2f);                   // empuja con la punta
    CHECK(en(0.57f).dedos > 0.4f);                      // los dedos se doblan al despegar
    // Al apoyar el talon la punta del pie va hacia arriba.
    const Esqueleto s = resolver(poseDe(andando(0.0f)));
    const V adelante = s.pie[0].tobillo.r.aplicar({ 0, 0, 1 });
    CHECK(adelante.y > 0.15f);
    // En toda la zancada algun pie toca el suelo.
    for (int k = 0; k < 24; ++k)
        CHECK(detalle::plantaMasBaja(resolver(poseDe(andando(k * 0.26f)))) ==
              doctest::Approx(0.0f).epsilon(0.001));
}

TEST_CASE("Jugador: agachado baja la pelvis y la camara queda sobre los hombros") {
    Estado e;
    e.agachado = true;
    const Esqueleto de_pie = resolver(poseDe(Estado{}));
    const Esqueleto abajo = resolver(poseDe(e));
    CHECK(abajo.pelvis.p.y < de_pie.pelvis.p.y - 0.25f);
    const float ky = escalaVertical(abajo, 0.80f);
    CHECK(abajo.hombro[0].p.y * ky < 0.80f - 0.05f);
}

TEST_CASE("Jugador: picar cierra el puno derecho y deja la izquierda relajada") {
    Estado e;
    e.golpeando = true;
    e.tiempo = 0.13f;
    const Pose p = poseDe(e);
    CHECK(p.brazo[1].dedo[1][1] > 1.5f);
    CHECK(p.brazo[0].dedo[1][1] < 1.0f);
    CHECK(p.apretar == 1.0f);
    // El golpe pasa por cargar y descargar: el hombro recorre mucho.
    float lo = 9, hi = -9;
    for (int k = 0; k < 50; ++k) {
        e.tiempo = k * 0.01f;
        const float f = poseDe(e).brazo[1].flex;
        lo = std::fmin(lo, f); hi = std::fmax(hi, f);
    }
    CHECK(hi - lo > 0.9f);
}

TEST_CASE("Jugador: 5 dedos con el pulgar del lado de fuera y las yemas hacia la palma") {
    const Esqueleto s = resolver(poseDe(Estado{}));
    for (int l = 0; l < 2; ++l) {
        const Mano& m = s.mano[l];
        // El pulgar sale del lado indicado por `lado`, en el marco de la mano.
        const V rel = resta(m.dedo[4][0].p, m.muneca.p);
        const float enX = punto(rel, { m.muneca.r.m[0], m.muneca.r.m[3], m.muneca.r.m[6] });
        CHECK(enX * m.lado > 0.0f);
        // Doblar los dedos los lleva hacia la palma (+Z de la mano).
        const V punta = resta(m.dedo[1][3].p, m.dedo[1][0].p);
        const float haciaPalma = punto(punta, { m.muneca.r.m[2], m.muneca.r.m[5], m.muneca.r.m[8] });
        CHECK(haciaPalma > 0.0f);
    }
    // Brazo colgando y relajado: el pulgar de la mano derecha apunta adelante.
    const Mano& d = s.mano[1];
    CHECK(resta(d.dedo[4][3].p, d.dedo[1][0].p).z > 0.0f);
}

TEST_CASE("Cuello: la cabeza no pasa de su limite y el cuerpo la sigue") {
    GiroCuerpo g;
    g.avanzar(0.0f, false, 0.016f);
    // Girar la camara 180 grados de golpe, quieto.
    float d = 0;
    for (int k = 0; k <= 18; ++k) d = g.avanzar(k * 10.0f, false, 0.016f);
    CHECK(std::fabs(d) <= GiroCuerpo::LIMITE * PI / 180.0f + 1e-4f);
    CHECK(std::fabs(GiroCuerpo::envolver(g.yaw - 85.0f)) < 1.0f);   // arrastrado
    // Quieto, se va enderezando.
    for (int k = 0; k < 300; ++k) d = g.avanzar(180.0f, false, 0.016f);
    CHECK(std::fabs(d) < 20.0f * PI / 180.0f);
    // Andando se pone de frente.
    for (int k = 0; k < 60; ++k) d = g.avanzar(180.0f, true, 0.016f);
    CHECK(std::fabs(d) < 0.02f);
    // Los hombros acompanan, pero el torso nunca pasa de su limite.
    Estado e;
    e.giroCuello = 95.0f * PI / 180.0f;
    CHECK(std::fabs(poseDe(e).giroTorso) <= Lim::TORSO_GIRO + 1e-5f);
}

TEST_CASE("Fisiologia: el pulso sube con el esfuerzo y baja al parar") {
    Fisiologia f;
    CHECK(f.bpm() == doctest::Approx(68.0f));
    for (int k = 0; k < 60 * 30; ++k) f.avanzar(1.0f / 60.0f, 5.6f, false);   // 30 s corriendo
    CHECK(f.bpm() > 120.0f);
    CHECK(f.calor > 0.3f);
    const float alto = f.bpm();
    for (int k = 0; k < 60 * 20; ++k) f.avanzar(1.0f / 60.0f, 0.0f, false);
    CHECK(f.bpm() < alto - 20.0f);
    // El latido es una onda acotada que pasa por cero.
    float lo = 9, hi = -9;
    for (int k = 0; k < 200; ++k) {
        f.avanzar(0.005f, 0.0f, false);
        lo = std::fmin(lo, f.latido());
        hi = std::fmax(hi, f.latido());
    }
    CHECK(lo < 0.01f);
    CHECK(hi > 0.8f);
    CHECK(hi < 1.5f);
}

TEST_CASE("Musculos: el biceps se hincha al doblar el codo y el gemelo al empujar") {
    float dx, dz, dx2, dz2;
    seccionBrazo(0.5f, DELANTE, 1.0f, 0.0f, dx, dz);
    seccionBrazo(0.5f, DELANTE, 1.0f, 1.0f, dx2, dz2);
    CHECK(dz2 > dz + 0.006f);
    // El gemelo interno de la pierna derecha (detras y hacia dentro, -X).
    seccionPierna(0.27f, DETRAS - 0.785f, 1.0f, 0.0f, dx, dz);
    seccionPierna(0.27f, DETRAS - 0.785f, 1.0f, 1.0f, dx2, dz2);
    CHECK(std::hypot(dx2, dz2) > std::hypot(dx, dz) + 0.002f);
    // Con el puno cerrado los tendones de la muneca saltan.
    seccionAntebrazo(0.86f, DELANTE, 1.0f, 0.0f, dx, dz);
    seccionAntebrazo(0.86f, DELANTE, 1.0f, 1.0f, dx2, dz2);
    CHECK(dz2 > dz + 0.001f);
}

TEST_CASE("Venas: al azar por mundo, pero donde van en una persona") {
    const Piel a = generarPiel(1234u), b = generarPiel(1234u), c = generarPiel(98765u);
    REQUIRE(a.venas.size() == b.venas.size());
    for (size_t i = 0; i < a.venas.size(); ++i) CHECK(a.venas[i].a0 == b.venas[i].a0);
    bool distinta = a.venas.size() != c.venas.size();
    for (size_t i = 0; !distinta && i < a.venas.size(); ++i) distinta = a.venas[i].a0 != c.venas[i].a0;
    CHECK(distinta);
    int porRegion[NUM_REGIONES] = {}, arterias = 0;
    for (const Vena& v : a.venas) {
        REQUIRE(v.region >= 0);
        REQUIRE(v.region < NUM_REGIONES);
        CHECK(v.t0 >= 0.0f);
        CHECK(v.t1 <= 1.0f);
        CHECK(v.t0 < v.t1);
        CHECK(v.ancho < 0.004f);
        ++porRegion[v.region];
        arterias += v.arteria;
    }
    for (int r : { ANTE_I, ANTE_D, MANO_I, MANO_D, PIERNA_I, PIERNA_D, PIE_I, PIE_D })
        CHECK(porRegion[r] >= 2);
    CHECK(arterias == 4);                   // radial y pedia, a cada lado
    CHECK(a.prominencia >= 0.45f);
}

TEST_CASE("Malla: completa, sin basura y con un tamano que se puede dibujar cada frame") {
    const Piel piel = generarPiel(42u);
    Fisiologia fis;
    for (int det = 0; det <= 2; ++det) {
        Calidad q;
        q.detalle = det;
        q.venas = det >= 1;
        Estado e = andando(1.0f);
        e.golpeando = true;
        const Pose p = poseDe(e);
        const Esqueleto s = resolver(p);
        Malla m;
        generar(s, p, piel, fis, q, m);
        REQUIRE(m.v.size() > 1000);
        CHECK(m.v.size() < 26000);
        CHECK(m.i.size() % 3 == 0);
        bool indicesOk = true, finitos = true;
        for (uint32_t i : m.i) indicesOk = indicesOk && i < m.v.size();
        float yMin = 9, yMax = -9;
        for (const VerticeJ& v : m.v) {
            finitos = finitos && std::isfinite(v.p.x) && std::isfinite(v.p.y) && std::isfinite(v.p.z) &&
                      std::isfinite(v.n.x);
            yMin = std::fmin(yMin, v.p.y);
            yMax = std::fmax(yMax, v.p.y);
        }
        CHECK(indicesOk);
        CHECK(finitos);
        CHECK(yMin > -0.03f);               // nada se hunde en el suelo
        CHECK(yMin < 0.03f);
        CHECK(yMax < s.ojo - 0.04f);        // la cabeza no esta: la camara va dentro
    }
}

TEST_CASE("Parpados: arriba y abajo a la vez, y cerrados no dejan rendija") {
    CHECK(parpados(0.0f).arriba == 0.0f);
    CHECK(parpados(0.0f).abajo == 0.0f);
    for (float c : { 0.1f, 0.4f, 0.8f }) {
        const Parpados p = parpados(c);
        CHECK(p.arriba > 0.0f);
        CHECK(p.abajo > 0.0f);
        CHECK(p.arriba > p.abajo);          // el de arriba recorre mas
    }
    const Parpados p = parpados(1.0f);
    // En la esquina (la curva retrasa el borde) siguen juntandose.
    CHECK(p.arriba + p.abajo - 2.0f * curvaParpado(0.0f) >= 1.0f - 1e-4f);
}

TEST_CASE("Parpadeo: ritmo humano, corto, y nunca se queda cerrado") {
    Parpadeo par;
    int parpadeos = 0, cerradoSeguido = 0, maxSeguido = 0, cerrados = 0;
    bool antes = false;
    const float dt = 1.0f / 120.0f;
    for (int i = 0; i < 120 * 60; ++i) {          // un minuto
        const float c = par.avanzar(dt);
        const bool ahora = c > 0.0f;
        if (ahora && !antes) ++parpadeos;
        cerradoSeguido = ahora ? cerradoSeguido + 1 : 0;
        if (cerradoSeguido > maxSeguido) maxSeguido = cerradoSeguido;
        if (c > 0.5f) ++cerrados;
        CHECK(c >= 0.0f);
        CHECK(c <= 1.0f);
        antes = ahora;
    }
    CHECK(parpadeos >= 9);              // ~15 por minuto, como una persona
    CHECK(parpadeos <= 22);
    CHECK(maxSeguido * dt <= 0.2f);     // cada uno dura menos de 0.2 s
    CHECK(cerrados * dt < 2.0f);        // en un minuto, apenas 1-2 s de ojos cerrados
    Parpadeo t;
    t.proximo = 0.0f;
    t.avanzar(0.01f);
    CHECK(t.avanzar(5.0f) < 1.0f);
}
