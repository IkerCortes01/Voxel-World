#include <doctest/doctest.h>
#include "player/ModeloJugador.h"
#include "player/MallaJugador.h"
#include "player/PlayerTypes.h"
#include <cmath>

using namespace ModeloJugador;

namespace {
Estado picando(Herramienta h, ClaseBloque c, float t) {
    Estado e;
    e.accion = Accion::PICAR;
    e.herramienta = h;
    e.bloque = c;
    e.tiempo = t;
    return e;
}
} // namespace

TEST_CASE("Animaciones: 5 herramientas x 4 bloques, todas distintas y validas") {
    for (int h = 0; h < (int)Herramienta::NUM; ++h) {
        for (int c = 0; c < (int)ClaseBloque::NUM; ++c) {
            const Golpe& g = golpeDe((Herramienta)h, (ClaseBloque)c);
            CHECK(g.periodo > 0.2f);
            CHECK(g.periodo < 1.0f);
            CHECK(g.carga > 0.3f);
            CHECK(g.carga < 0.8f);
            // Cada una se distingue de las otras tres de la misma herramienta.
            for (int o = 0; o < c; ++o) {
                const Golpe& k = golpeDe((Herramienta)h, (ClaseBloque)o);
                const float dif = std::fabs(g.flex[0] - k.flex[0]) + std::fabs(g.abd[0] - k.abd[0]) +
                                  std::fabs(g.codo[1] - k.codo[1]) + std::fabs(g.periodo - k.periodo);
                CHECK(dif > 0.05f);
            }
        }
    }
}

TEST_CASE("Animaciones: el pico en piedra carga desde arriba y descarga con todo") {
    float lo = 9, hi = -9, inclMax = 0;
    const Golpe& g = golpeDe(Herramienta::PICO, ClaseBloque::PIEDRA);
    for (int k = 0; k < 100; ++k) {
        const Pose p = poseDe(picando(Herramienta::PICO, ClaseBloque::PIEDRA, g.periodo * k / 100.0f));
        lo = std::fmin(lo, p.brazo[1].flex);
        hi = std::fmax(hi, p.brazo[1].flex);
        inclMax = std::fmax(inclMax, p.inclinacion);
        CHECK(p.apretar == 1.0f);
        CHECK(p.enMano == (uint8_t)Herramienta::PICO);
    }
    // La carga ya no sube por encima de la cabeza (pedido: el golpe mas
    // alejado de la cabeza), pero el recorrido sigue siendo grande.
    CHECK(hi - lo > 1.2f);
    CHECK(inclMax < 0.02f);          // el cuerpo no se mueve: solo el brazo
    // A UNA mano, como se pidio: la izquierda no sigue al pico.
    const Pose p = poseDe(picando(Herramienta::PICO, ClaseBloque::PIEDRA, 0.1f));
    CHECK(std::fabs(p.brazo[0].flex) < 0.4f);
    for (int c = 0; c < (int)ClaseBloque::NUM; ++c)
        CHECK_FALSE(golpeDe(Herramienta::PICO, (ClaseBloque)c).dosManos);
    // El hacha si va a dos manos en la madera.
    CHECK(golpeDe(Herramienta::HACHA, ClaseBloque::MADERA).dosManos);
    // En lo construido, toques cortos: mucho menos recorrido.
    lo = 9; hi = -9;
    const Golpe& c = golpeDe(Herramienta::PICO, ClaseBloque::CONSTRUIDO);
    for (int k = 0; k < 100; ++k) {
        const float f = poseDe(picando(Herramienta::PICO, ClaseBloque::CONSTRUIDO, c.periodo * k / 100.0f)).brazo[1].flex;
        lo = std::fmin(lo, f); hi = std::fmax(hi, f);
    }
    CHECK(hi - lo < 1.0f);
}

TEST_CASE("Animaciones: el golpe va hacia donde se mira") {
    const Golpe& g = golpeDe(Herramienta::PICO, ClaseBloque::PIEDRA);
    const float tImpacto = g.periodo * (g.carga + (1.0f - g.carga) * 0.6f);
    Estado e = picando(Herramienta::PICO, ClaseBloque::PIEDRA, tImpacto);
    e.mirada = 0.0f;
    const float frente = poseDe(e).brazo[1].flex;
    e.mirada = 45.0f;
    const float arriba = poseDe(e).brazo[1].flex;
    e.mirada = -60.0f;
    const Pose abajo = poseDe(e);
    CHECK(arriba > frente + 0.4f);
    CHECK(abajo.brazo[1].flex < frente - 0.5f);
    CHECK(abajo.inclinacion < 0.02f);    // apunta el brazo, no el cuerpo
}

TEST_CASE("Animaciones: punetazos al aire alternos que apuntan a la mirada") {
    Estado e;
    e.accion = Accion::GOLPE_AIRE;
    e.tAccion = 0.21f;                  // impacto (0.09 recoger + 0.12 soltar)
    e.variante = 0;
    const Pose d = poseDe(e);
    CHECK(d.brazo[1].codo < 0.15f);     // derecha estirada
    CHECK(d.brazo[0].codo < 0.6f);      // la izquierda sigue suelta, no se mueve
    e.variante = 1;
    const Pose i = poseDe(e);
    CHECK(i.brazo[0].codo < 0.15f);     // ahora la izquierda
    CHECK(i.brazo[1].codo < 0.6f);
    e.variante = 0;
    e.mirada = 50.0f;
    CHECK(poseDe(e).brazo[1].flex > d.brazo[1].flex + 0.6f);   // golpea arriba
}

TEST_CASE("Animaciones: colocar estira el brazo derecho con la mano abierta") {
    Estado e;
    e.accion = Accion::COLOCAR;
    e.tAccion = COLOCAR_SEG * 0.5f;
    const Pose p = poseDe(e);
    CHECK(p.brazo[1].flex > 1.0f);
    CHECK(p.brazo[1].codo < 0.4f);
    CHECK(p.brazo[1].dedo[1][1] < 0.4f);        // abierta
    e.tAccion = COLOCAR_SEG + 0.1f;
    CHECK(poseDe(e).brazo[1].flex < 0.4f);      // ya termino
    // Tambien tumbado.
    e.tumbado = true;
    e.tAccion = COLOCAR_SEG * 0.5f;
    Estado sin = e;
    sin.accion = Accion::NINGUNA;
    CHECK(std::fabs(poseDe(e).brazo[1].codo - poseDe(sin).brazo[1].codo) > 0.1f);
}

TEST_CASE("Animaciones: quieto, el cuerpo cambia de gesto y respira de verdad") {
    Estado e;
    e.quieto = 1.0f;
    const Pose al = poseDe(e);
    // A los pocos segundos empieza a hacer cosas, y cosas distintas.
    // Al azar: hay tramos sin gesto (no es una cadena continua) y, entre los
    // que tienen, salen todos.
    int gestos[NUM_GESTOS] = {}, nada = 0;
    for (int k = 0; k < 2000; ++k) {
        const int g = gestoDeReposo(REPOSO_INICIO + k * REPOSO_TRAMO + 1.0f);
        REQUIRE(g < NUM_GESTOS);
        if (g < 0) { ++nada; continue; }
        ++gestos[g];
    }
    for (int g = 0; g < NUM_GESTOS; ++g) CHECK(gestos[g] > 0);
    CHECK(nada > 800);
    CHECK(nada < 1500);
    CHECK(NUM_GESTOS >= 13);
    CHECK(gestoDeReposo(1.0f) == -1);
    // A mitad de un gesto la pose no es la de estar quieto.
    bool cambia = false;
    for (int k = 0; k < 200 && !cambia; ++k) {
        e.quieto = REPOSO_INICIO + k * REPOSO_TRAMO + REPOSO_TRAMO * 0.5f;
        const Pose p = poseDe(e);
        cambia = std::fabs(p.brazo[0].flex - al.brazo[0].flex) > 0.3f ||
                 std::fabs(p.pierna[0].rodilla - al.pierna[0].rodilla) > 0.2f ||
                 std::fabs(p.pierna[1].rodilla - al.pierna[1].rodilla) > 0.2f;
    }
    CHECK(cambia);
    // Respirar: se toma aire mas deprisa de lo que se suelta, y hay pausa.
    int subiendo = 0, bajando = 0, pausa = 0;
    float antes = ondaRespira(0.0f);
    for (int k = 1; k <= 1000; ++k) {
        const float r = ondaRespira(2.0f * PI * k / 1000.0f);
        if (r > antes + 1e-5f) ++subiendo; else if (r < antes - 1e-5f) ++bajando; else ++pausa;
        CHECK(r >= -1.0f); CHECK(r <= 1.0f);
        antes = r;
    }
    CHECK(subiendo < bajando);
    CHECK(pausa > 50);
}

TEST_CASE("Animaciones: el golpe acelera hacia el impacto") {
    const float carga = 0.6f;
    // Justo al empezar a golpear va despacio y al final deprisa.
    const float a = faseGolpe(carga + 0.02f, carga) - faseGolpe(carga, carga);
    const float b = faseGolpe(carga + 0.16f, carga) - faseGolpe(carga + 0.14f, carga);
    CHECK(b > a * 3.0f);
    CHECK(faseGolpe(0.0f, carga) == doctest::Approx(1.0f));
    CHECK(faseGolpe(carga, carga) == doctest::Approx(0.0f));
}

TEST_CASE("Animaciones: punetazo al aire, un directo que se estira y termina") {
    Estado e;
    e.accion = Accion::GOLPE_AIRE;
    e.tAccion = 0.09f + 0.12f;           // fin del golpe: el impacto
    const Pose p = poseDe(e);
    CHECK(p.brazo[1].codo < 0.15f);
    CHECK(p.brazo[1].flex > 1.4f);
    e.tAccion = 0.07f;                  // recogiendo el brazo
    CHECK(poseDe(e).brazo[1].codo > 1.0f);
    e.tAccion = 1.0f;                   // ya termino: brazo normal
    CHECK(poseDe(e).brazo[1].codo < 0.6f);
}

TEST_CASE("Animaciones: tres golpes distintos a un animal") {
    Estado e;
    e.accion = Accion::GOLPE_ANIMAL;
    e.tAccion = 0.11f + 0.13f;          // impacto
    e.variante = 0;
    const Pose directo = poseDe(e);
    CHECK(directo.brazo[1].flex > 1.4f);
    CHECK(directo.giroTorso > 0.05f);       // un poco de pecho
    CHECK(directo.giroTorso < 0.15f);       // pero no todo el cuerpo
    e.variante = 1;
    const Pose gancho = poseDe(e);
    CHECK(gancho.brazo[0].abd > 1.0f);       // de izquierda, el codo de lado
    CHECK(gancho.giroTorso < -0.05f);        // el pecho gira al reves, un poco
    e.variante = 2;
    e.ladoGolpe = 0;
    const Pose abajoI = poseDe(e);
    CHECK(abajoI.brazo[0].flex > 1.8f);      // de abajo arriba, con la izquierda
    e.ladoGolpe = 1;
    const Pose abajoD = poseDe(e);
    CHECK(abajoD.brazo[1].flex > 1.8f);
    // Antes del impacto, el gancho de abajo arranca con el brazo bajo.
    e.tAccion = 0.11f;
    CHECK(poseDe(e).brazo[1].flex < 0.1f);
}

TEST_CASE("Animaciones: agachado distinto en suelo natural, construido y agua") {
    Estado e;
    e.agachado = true;
    e.superficie = Superficie::NATURAL;
    const Pose n = poseDe(e);
    // La izquierda, baja y colgando: el brazo casi abajo y la muneca caida.
    CHECK(n.brazo[0].flex < 0.8f + 0.36f);
    CHECK(n.brazo[0].munFlex > 0.0f);
    e.superficie = Superficie::CONSTRUCCION;
    const Pose c = poseDe(e);
    CHECK(c.brazo[1].codo > 0.6f);           // el antebrazo derecho sobre el muslo
    CHECK(c.brazo[0].codo < c.brazo[1].codo); // la izquierda cuelga
    CHECK(c.inclinacion < n.inclinacion);
    e.superficie = Superficie::LIQUIDO;
    const Pose l = poseDe(e);
    CHECK(l.brazo[0].abd > 0.8f);
    CHECK(l.brazo[1].abd > 0.8f);
    // El agua mece las munecas.
    e.tiempo = 0.5f;
    const float m1 = poseDe(e).brazo[0].munFlex;
    e.tiempo = 2.4f;
    CHECK(std::fabs(poseDe(e).brazo[0].munFlex - m1) > 0.1f);
}

TEST_CASE("Animaciones: subir un nivel levanta la pierna, bajarlo la estira") {
    Estado e;
    const Pose quieto = poseDe(e);
    e.escalon = 0.5f;
    e.escalonSube = true;
    e.piernaEscalon = 1;
    const Pose sube = poseDe(e);
    CHECK(sube.pierna[1].rodilla > quieto.pierna[1].rodilla + 0.8f);
    CHECK(sube.pierna[1].flex > quieto.pierna[1].flex + 0.4f);
    e.escalonSube = false;
    const Pose baja = poseDe(e);
    CHECK(baja.pierna[1].tobillo < quieto.pierna[1].tobillo - 0.3f);
    e.escalon = -1.0f;
    CHECK(poseDe(e).pierna[1].rodilla == doctest::Approx(quieto.pierna[1].rodilla));
}

TEST_CASE("Animaciones: todo dentro de los limites humanos") {
    for (int k = 0; k < 300; ++k) {
        Estado e;
        e.rapidez = (k % 7) * 0.9f;
        e.fasePaso = k * 0.31f;
        e.agachado = k % 5 == 0;
        e.superficie = (Superficie)(k % 3);
        e.accion = (Accion)(k % 4);
        e.herramienta = (Herramienta)(k % 5);
        e.bloque = (ClaseBloque)((k / 5) % 4);
        e.variante = k % 3;
        e.ladoGolpe = k % 2;
        e.tAccion = (k % 11) * 0.04f;
        e.tiempo = k * 0.053f;
        e.escalon = (k % 4) * 0.3f - 0.1f;
        const Pose p = poseDe(e);
        for (const Brazo& b : p.brazo) {
            CHECK(b.codo >= 0.0f);
            CHECK(b.codo <= Lim::CODO_MAX);
            CHECK(b.flex <= Lim::HOMBRO_FLEX_MAX);
        }
        for (const Pierna& g : p.pierna) {
            CHECK(g.rodilla >= 0.0f);
            CHECK(g.rodilla <= Lim::RODILLA_MAX);
        }
    }
}

TEST_CASE("Animaciones: el item de la mano va en el puno derecho y sigue a la mano") {
    // El item se dibuja con su textura en main.cpp (dibujarItem3D); aqui se
    // comprueba el marco donde se pone.
    for (int k = 0; k < 10; ++k) {
        Estado e = picando(Herramienta::PICO, ClaseBloque::PIEDRA, k * 0.07f);
        const Esqueleto s = resolver(poseDe(e));
        const Junta g = agarre(s);
        const Mano& m = s.mano[1];
        // Dentro del puno: entre la muneca y los nudillos.
        const float d = largo(resta(g.p, m.muneca.p));
        CHECK(d > 0.04f);
        CHECK(d < 0.11f);
        // +X del agarre es el lado del pulgar.
        const V pulgar = resta(m.dedo[4][0].p, m.muneca.p);
        CHECK(punto(pulgar, g.r.aplicar({ 1, 0, 0 })) > 0.0f);
        // +Y hacia el codo.
        const V codo = resta(s.codo[1].p, m.muneca.p);
        CHECK(punto(codo, g.r.aplicar({ 0, 1, 0 })) > 0.0f);
    }
}

TEST_CASE("Recoger: tres variantes, de pie, agachado y tumbado") {
    Estado e;
    e.tRecoger = RECOGER_SEG * 0.5f;              // abajo del todo
    const Pose pie = poseDe(e);
    CHECK(pie.inclinacion > 0.8f);                // se dobla por la cintura
    CHECK(pie.brazo[1].codo < 0.4f);              // el brazo estirado hacia el suelo
    e.agachado = true;
    const Pose ag = poseDe(e);
    CHECK(ag.brazo[1].flex > 1.4f);
    e.agachado = false;
    e.tumbado = true;
    const Pose tu = poseDe(e);
    CHECK(tu.brazo[1].flex > 2.6f);               // alarga el brazo por el suelo
    // La mano se cierra al final y se abre al empezar.
    e.tumbado = false;
    e.tRecoger = RECOGER_SEG * 0.15f;
    const float abierta = poseDe(e).brazo[1].dedo[1][1];
    e.tRecoger = RECOGER_SEG * 0.75f;
    CHECK(poseDe(e).brazo[1].dedo[1][1] > abierta + 0.6f);
    // Y al terminar vuelve a estar derecho.
    e.tRecoger = -1.0f;
    CHECK(poseDe(e).inclinacion < 0.1f);
    // Agachado, recoger cuesta menos: el cuerpo se dobla menos que de pie.
    e.tRecoger = RECOGER_SEG * 0.5f;
    e.agachado = true;
    Estado sin = e;
    sin.tRecoger = -1.0f;
    CHECK(poseDe(e).inclinacion - poseDe(sin).inclinacion < 0.4f);
}

TEST_CASE("Saltos: tres distintos") {
    Estado e;
    e.enSuelo = false;
    e.tAire = 0.4f;
    Pose p[3];
    for (int v = 0; v < 3; ++v) { e.saltoVariante = v; p[v] = poseDe(e); }
    // Zancada: una pierna delante y otra atras.
    CHECK(p[1].pierna[0].flex - p[1].pierna[1].flex > 1.0f);
    // Encogido: las dos rodillas al pecho.
    CHECK(p[2].pierna[0].rodilla > 1.7f);
    CHECK(p[2].pierna[1].rodilla > 1.7f);
    // Distintas entre si.
    CHECK(std::fabs(p[0].pierna[1].flex - p[1].pierna[1].flex) > 0.3f);
    CHECK(std::fabs(p[0].pierna[1].rodilla - p[2].pierna[1].rodilla) > 0.5f);
    // Recien despegado aun no ha llegado a la pose.
    e.saltoVariante = 2;
    e.tAire = 0.02f;
    CHECK(poseDe(e).pierna[0].rodilla < 1.3f);
}

TEST_CASE("Sigilo: pasos suaves, e inspeccion medio agachado") {
    Estado e;
    e.rapidez = 1.9f;
    e.fasePaso = 1.0f;
    const Pose normal = poseDe(e);
    e.sigilo = true;
    const Pose sig = poseDe(e);
    CHECK(sig.pierna[0].rodilla > normal.pierna[0].rodilla + 0.1f);
    CHECK(sig.brazo[0].codo > normal.brazo[0].codo + 0.2f);
    e.inspeccion = true;
    const Pose ins = poseDe(e);
    CHECK(ins.inclinacion > sig.inclinacion + 0.3f);
    CHECK(ins.pierna[0].rodilla > sig.pierna[0].rodilla + 0.6f);
    CHECK(ins.brazo[0].flex > 0.6f);                  // la mano delante, tanteando
    // Medio agachado baja de verdad el cuerpo.
    CHECK(resolver(ins).pelvis.p.y < resolver(sig).pelvis.p.y - 0.12f);
    // Mira alrededor despacio.
    e.tiempo = 0.0f;
    const float a = poseDe(e).giroTorso;
    e.tiempo = 2.6f;
    CHECK(std::fabs(poseDe(e).giroTorso - a) > 0.1f);
}

TEST_CASE("Escalon: los niveles se suben andando, los bloques completos no") {
    PlayerSys::MovementConfig cfg;
    CHECK(cfg.stepHeight > 13.0f / 16.0f);   // el nivel 7, el mas alto
    CHECK(cfg.stepHeight < 1.0f);            // un bloque entero se salta
}

TEST_CASE("Dedos: mas gruesos y separados que antes, sin chocar entre ellos") {
    const Esqueleto s = resolver(poseDe(Estado{}));
    for (int l = 0; l < 2; ++l) {
        for (int d = 0; d + 1 < 5; ++d) {
            const float sep = largo(resta(s.pie[l].dedo[d][0].p, s.pie[l].dedo[d + 1][0].p));
            CHECK(sep > radioDedoPie(d) * 0.9f);    // se ve el hueco entre ellos
        }
    }
    CHECK(radioDedoPie(0) > 0.015f);
}

TEST_CASE("Transiciones: cada articulacion llega con inercia, sin saltos") {
    SuavizadoPose s;
    Estado e;
    const Pose dePie = poseDe(e);
    s.avanzar(dePie, 1.0f / 60.0f, false);
    // Agacharse: el objetivo cambia de golpe, la pose no.
    e.agachado = true;
    const Pose agachado = poseDe(e);
    const Pose& p1 = s.avanzar(agachado, 1.0f / 60.0f, false);
    CHECK(p1.pierna[0].rodilla < agachado.pierna[0].rodilla - 0.5f);   // no salta
    for (int k = 0; k < 120; ++k) s.avanzar(agachado, 1.0f / 60.0f, false);
    CHECK(s.actual.pierna[0].rodilla == doctest::Approx(agachado.pierna[0].rodilla).epsilon(0.02));
    CHECK(s.actual.agachado == doctest::Approx(1.0f).epsilon(0.02));
    // Un brazo que se para en seco se pasa un poco y vuelve (follow-through).
    SuavizadoPose b;
    Pose a = dePie, z = dePie;
    z.brazo[1].flex = 1.5f;
    b.avanzar(a, 1.0f / 60.0f, false);
    float maximo = 0.0f;
    for (int k = 0; k < 120; ++k) maximo = std::fmax(maximo, b.avanzar(z, 1.0f / 60.0f, false).brazo[1].flex);
    CHECK(maximo > 1.5f);            // se pasa
    CHECK(maximo < 1.75f);           // pero poco
    CHECK(b.actual.brazo[1].flex == doctest::Approx(1.5f).epsilon(0.01));
    // Estable aunque el frame tarde mucho.
    SuavizadoPose lento;
    lento.avanzar(a, 0.1f, true);
    for (int k = 0; k < 50; ++k) lento.avanzar((k % 2) ? z : a, 0.1f, true);
    CHECK(std::isfinite(lento.actual.brazo[1].flex));
    CHECK(std::fabs(lento.actual.brazo[1].flex) < 3.1f);
}

TEST_CASE("Variacion: los golpes de una serie no son todos iguales") {
    const Golpe& g = golpeDe(Herramienta::PICO, ClaseBloque::PIEDRA);
    Estado e;
    e.accion = Accion::PICAR;
    e.herramienta = Herramienta::PICO;
    e.bloque = ClaseBloque::PIEDRA;
    // El mismo instante de dos golpes seguidos.
    e.tiempo = g.periodo * 0.30f;
    const float a = poseDe(e).brazo[1].flex;
    e.tiempo = g.periodo * 1.30f;
    const float b = poseDe(e).brazo[1].flex;
    e.tiempo = g.periodo * 2.30f;
    const float c = poseDe(e).brazo[1].flex;
    CHECK((std::fabs(a - b) > 0.01f || std::fabs(b - c) > 0.01f));
    CHECK(std::fabs(a - b) < 0.6f);   // pero el mismo movimiento
}

TEST_CASE("Variacion: cuatro punetazos al aire distintos, con los dos brazos") {
    Estado e;
    e.accion = Accion::GOLPE_AIRE;
    e.tAccion = 0.21f;
    Pose p[8];
    for (int v = 0; v < 8; ++v) { e.variante = v; p[v] = poseDe(e); }
    CHECK(p[4].brazo[1].abd > p[0].brazo[1].abd + 0.5f);     // gancho: el codo de lado
    CHECK(std::fabs(p[2].giroTorso) > std::fabs(p[0].giroTorso) + 0.05f);   // cruzado: mas cuerpo
    CHECK(p[5].brazo[0].abd > 0.8f);                          // gancho con la izquierda
    e.tAccion = 0.05f;                                        // uppercut: las piernas no se mueven
    e.variante = 6;
    CHECK(poseDe(e).pierna[0].rodilla < 0.15f);
}

TEST_CASE("Reacciones: aterrizar, acelerar y girar sobre si mismo") {
    Estado e;
    const Pose base = poseDe(e);
    e.aterrizaje = 0.10f;
    e.fuerzaCaida = 1.0f;
    CHECK(poseDe(e).pierna[0].rodilla > base.pierna[0].rodilla + 0.6f);
    e.fuerzaCaida = 0.2f;
    CHECK(poseDe(e).pierna[0].rodilla < base.pierna[0].rodilla + 0.3f);
    e.aterrizaje = 2.0f;
    CHECK(poseDe(e).pierna[0].rodilla == doctest::Approx(base.pierna[0].rodilla));
    Estado a;
    a.aceleracion = 6.0f;
    CHECK(poseDe(a).inclinacion > base.inclinacion + 0.1f);
    a.aceleracion = -6.0f;
    CHECK(poseDe(a).inclinacion < base.inclinacion - 0.08f);
    Estado g;
    g.giroVel = 120.0f;
    g.faseGiro = 1.5708f;
    const Pose gp = poseDe(g);
    CHECK(std::fmax(gp.pierna[0].rodilla, gp.pierna[1].rodilla) > base.pierna[0].rodilla + 0.3f);
}

TEST_CASE("Marcha: la cadera se balancea y cada personaje anda a su manera") {
    Estado e;
    e.rapidez = 4.3f;
    e.fasePaso = PI * 0.5f;
    const Pose p = poseDe(e);
    CHECK(std::fabs(p.pelvisLado) > 0.03f);
    // La caida de la cadera no despega los huesos.
    const Esqueleto s = resolver(p);
    CHECK(largo(resta(s.cadera[0].p, s.rodilla[0].p)) == doctest::Approx(M::MUSLO).epsilon(0.001));
    // Dos estilos, dos maneras de mover los brazos (medido con el brazo en
    // lo mas alto del balanceo).
    e.fasePaso = 0.0f;
    float lo = 9, hi = -9;
    for (uint32_t k = 0; k < 10; ++k) {
        e.estilo = k * 7919u + 1u;
        const float f = poseDe(e).brazo[0].flex;
        lo = std::fmin(lo, f); hi = std::fmax(hi, f);
    }
    CHECK(hi - lo > 0.02f);
}
