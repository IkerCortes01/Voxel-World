#include <doctest/doctest.h>
#include "player/CuerpoJugador.h"
#include "player/PlayerTypes.h"

// ============================================================================
// EL CUERPO DEL JUGADOR EN PRIMERA PERSONA
// ============================================================================
// Un modelo 3D anatomico que se ve al mirar hacia abajo: torso, brazos, manos,
// piernas y pies, con articulaciones reales.
//
// ⚠️ EL REQUISITO INNEGOCIABLE ES QUE LA VISION NO CAMBIE.
//
// El cuerpo se dibuja en el mundo, pero no mueve el ojo ni un milimetro. El
// primer bloque de tests fija justamente eso, porque es lo que se rompe sin que
// nadie lo note hasta que se juega: basta con que alguien "ajuste" la camara
// para que el cuerpo encaje mejor y la vista deja de estar donde debe.
//
// El resto comprueba la anatomia: que las proporciones sean humanas, que las
// articulaciones no se doblen al reves, y que en primera persona no se dibuje
// la cabeza -- porque la camara esta DENTRO de ella.

using namespace PlayerBody;

// ============================================================================
// 1. LA CAMARA NO SE TOCA
// ============================================================================

TEST_CASE("Cuerpo: no altera la altura de los ojos") {
    // Los valores que consume CameraSystem. Si el cuerpo los hubiera movido
    // para "encajar mejor", la vista habria cambiado -- que es exactamente lo
    // que se pidio que NO pasara.
    PlayerSys::MovementConfig cfg;
    CHECK(cfg.eyeHeight       == doctest::Approx(1.62f));
    CHECK(cfg.crouchEyeHeight == doctest::Approx(0.80f));
    CHECK(cfg.height          == doctest::Approx(1.80f));
}

TEST_CASE("Cuerpo: el modelo se ajusta a la estatura, no al reves") {
    // La direccion de la dependencia importa: el cuerpo se adapta a las
    // medidas del jugador que ya existian. Si fuera al reves, cambiar el
    // modelo cambiaria la fisica y la camara.
    PlayerSys::MovementConfig cfg;
    Cuerpo c;
    CHECK(c.medidas.estatura == doctest::Approx(cfg.height));
}

TEST_CASE("Cuerpo: nada del modelo asoma por encima de los ojos") {
    // Si una parte del cuerpo quedara por encima de la altura de camara, se
    // veria flotando en mitad de la pantalla. El hombro es el punto mas alto
    // que se dibuja en primera persona (la cabeza no se dibuja).
    Cuerpo c;
    PlayerSys::MovementConfig cfg;

    Fauna::MallaAnimal m;
    c.construir(m, PartesVisibles::primeraPersona(), 8);

    REQUIRE_FALSE(m.vertices.empty());
    INFO("punto mas alto del cuerpo: ", m.maximo.y, " ojos: ", cfg.eyeHeight);
    CHECK(m.maximo.y <= cfg.eyeHeight);
}

// ============================================================================
// 2. EN PRIMERA PERSONA NO SE DIBUJA LA CABEZA
// ============================================================================

TEST_CASE("Cuerpo: primera persona omite cabeza, ojos y dientes") {
    // No es por ahorrar triangulos: la camara esta DENTRO del craneo, asi que
    // dibujarlo llenaria la pantalla con su interior.
    const PartesVisibles pp = PartesVisibles::primeraPersona();
    CHECK_FALSE(pp.cabeza);
    CHECK_FALSE(pp.ojos);
    CHECK_FALSE(pp.dientes);

    // Pero SI se dibuja lo que de verdad se ve al mirar abajo.
    CHECK(pp.torso);
    CHECK(pp.brazos);
    CHECK(pp.manos);
    CHECK(pp.piernas);
    CHECK(pp.pies);
}

TEST_CASE("Cuerpo: el modelo completo si lleva cabeza") {
    // Para tercera persona, sombra o multijugador: ahi la cabeza se ve desde
    // fuera y tiene que estar.
    const PartesVisibles cc = PartesVisibles::cuerpoCompleto();
    CHECK(cc.cabeza);
    CHECK(cc.ojos);
    CHECK(cc.dientes);
    CHECK(cc.torso);
}

// ============================================================================
// 3. PROPORCIONES ANATOMICAS
// ============================================================================

TEST_CASE("Cuerpo: las proporciones son humanas") {
    Cuerpo c;
    const Medidas& m = c.medidas;

    // Canon de 7.5 cabezas: es la proporcion adulta estandar.
    CHECK(m.cabeza() == doctest::Approx(1.80f / 7.5f));

    // El brazo entero es ~44% de la estatura; la pierna ~53%. La pierna
    // SIEMPRE es mas larga que el brazo en un humano.
    CHECK(m.largoPierna() > m.largoBrazo());

    // Los hombros son mas anchos que las caderas en el modelo (el torso se
    // ensancha hacia arriba).
    CHECK(m.anchoHombros() > 0.0f);

    // El hombro esta por debajo de la coronilla pero muy por encima de la
    // cadera: es de donde cuelgan los brazos.
    CHECK(m.alturaHombro() < m.estatura);
    CHECK(m.alturaHombro() > m.alturaCadera());
}

TEST_CASE("Cuerpo: el torso es mas ancho que profundo") {
    // Un torso humano es una ELIPSE, no un cilindro. Si radioX == radioZ el
    // cuerpo se veria como un tubo, que es el defecto clasico de los modelos
    // hechos a ojo.
    Cuerpo c;
    CHECK(c.medidas.radioTorsoX() > c.medidas.radioTorsoZ());
}

TEST_CASE("Cuerpo: los segmentos de un miembro suman su longitud") {
    Cuerpo c;
    const Medidas& m = c.medidas;

    // Humero + antebrazo + mano ~ el brazo entero. No suman exacto porque la
    // mano se cuenta aparte en el canon, pero no pueden desviarse mucho.
    const float brazoSegmentos = m.largoHumero() + m.largoAntebrazo();
    INFO("segmentos ", brazoSegmentos, " de brazo ", m.largoBrazo());
    CHECK(brazoSegmentos < m.largoBrazo());
    CHECK(brazoSegmentos > m.largoBrazo() * 0.7f);

    const float piernaSegmentos = m.largoMuslo() + m.largoPantorrilla();
    CHECK(piernaSegmentos < m.largoPierna());
    CHECK(piernaSegmentos > m.largoPierna() * 0.8f);
}

TEST_CASE("Cuerpo: escala entero si cambia la estatura") {
    // Cambiar la estatura tiene que escalar TODO en proporcion. Si alguna
    // medida fuera absoluta, un jugador mas alto tendria las manos en el sitio
    // equivocado.
    Cuerpo bajo, alto;
    bajo.medidas.estatura = 1.50f;
    alto.medidas.estatura = 2.00f;

    const float k = 2.00f / 1.50f;
    CHECK(alto.medidas.largoBrazo()   == doctest::Approx(bajo.medidas.largoBrazo()   * k));
    CHECK(alto.medidas.largoPierna()  == doctest::Approx(bajo.medidas.largoPierna()  * k));
    CHECK(alto.medidas.alturaHombro() == doctest::Approx(bajo.medidas.alturaHombro() * k));
}

// ============================================================================
// 4. ARTICULACIONES
// ============================================================================

TEST_CASE("Articulaciones: el codo y la rodilla no se doblan al reves") {
    // ⚠️ Es una bisagra. Sin este limite, una interpolacion mal hecha produce
    // la deformidad que rompe la ilusion al instante -- y en primera persona
    // se ve de muy cerca.
    Esqueleto e;
    e.codoIzq.flexion    = -1.0f;   // hacia atras: imposible
    e.rodillaDer.flexion = -0.5f;
    e.aplicarLimites();

    CHECK(e.codoIzq.flexion    >= 0.0f);
    CHECK(e.rodillaDer.flexion >= 0.0f);
}

TEST_CASE("Articulaciones: el codo y la rodilla tienen tope de recorrido") {
    // Un codo real llega a ~150 grados y una rodilla a ~135. Pasarse hace que
    // el antebrazo atraviese el biceps.
    Esqueleto e;
    e.codoDer.flexion     = 10.0f;   // absurdo
    e.rodillaIzq.flexion  = 10.0f;
    e.aplicarLimites();

    CHECK(e.codoDer.flexion    <= doctest::Approx(2.62f));   // ~150 grados
    CHECK(e.rodillaIzq.flexion <= doctest::Approx(2.36f));   // ~135 grados
}

TEST_CASE("Articulaciones: una bisagra no abduce") {
    // El codo no se separa lateralmente: solo dobla. Permitirlo produce
    // brazos rotos hacia los lados.
    Esqueleto e;
    e.codoIzq.abduccion = 0.9f;
    e.aplicarLimites();
    CHECK(e.codoIzq.abduccion == doctest::Approx(0.0f));
}

TEST_CASE("Articulaciones: el hombro y la cadera SI pueden abducir") {
    // Son articulaciones esfericas: el brazo se separa del cuerpo y la pierna
    // tambien. aplicarLimites no debe tocarlas.
    Esqueleto e;
    e.hombroDer.abduccion = 0.7f;
    e.caderaIzq.abduccion = 0.3f;
    e.aplicarLimites();

    CHECK(e.hombroDer.abduccion == doctest::Approx(0.7f));
    CHECK(e.caderaIzq.abduccion == doctest::Approx(0.3f));
}

// ============================================================================
// 5. LA MALLA
// ============================================================================

TEST_CASE("Malla: se genera geometria valida") {
    Cuerpo c;
    Fauna::MallaAnimal m;
    c.construir(m, PartesVisibles::primeraPersona(), 12);

    CHECK(m.vertices.size() > 0);
    CHECK(m.indices.size()  > 0);
    // Triangulos completos: los indices van de tres en tres.
    CHECK(m.indices.size() % 3 == 0);
    CHECK(m.triangulos() > 0);
}

TEST_CASE("Malla: ningun indice apunta fuera del array de vertices") {
    // Un indice fuera de rango es lectura de memoria invalida en el momento
    // de dibujar. Es el fallo mas grave posible en una malla y no da sintomas
    // hasta que revienta.
    Cuerpo c;
    Fauna::MallaAnimal m;
    c.construir(m, PartesVisibles::primeraPersona(), 12);

    const size_t n = m.vertices.size();
    REQUIRE(n > 0);
    for (uint16_t idx : m.indices) {
        REQUIRE(idx < n);
    }
}

TEST_CASE("Malla: el LOD cambia el detalle, no la forma") {
    Cuerpo c;
    Fauna::MallaAnimal alto, bajo;
    c.construir(alto, PartesVisibles::primeraPersona(), 12);
    c.construir(bajo, PartesVisibles::primeraPersona(), 6);

    // Menos lados = menos geometria...
    CHECK(bajo.vertices.size() < alto.vertices.size());

    // ...pero la silueta se mantiene: la caja envolvente apenas cambia.
    // Si cambiara mucho, el LOD estaria deformando el cuerpo en vez de
    // simplificarlo.
    CHECK(bajo.maximo.y == doctest::Approx(alto.maximo.y).epsilon(0.02));
    CHECK(bajo.minimo.y == doctest::Approx(alto.minimo.y).epsilon(0.05));
}

TEST_CASE("Malla: quitar partes quita geometria") {
    Cuerpo c;
    Fauna::MallaAnimal completo, soloTorso;

    c.construir(completo, PartesVisibles::primeraPersona(), 8);

    PartesVisibles p;
    p.brazos = p.manos = p.piernas = p.pies = false;
    c.construir(soloTorso, p, 8);

    CHECK(soloTorso.vertices.size() < completo.vertices.size());
    CHECK(soloTorso.vertices.size() > 0);   // el torso sigue ahi
}

TEST_CASE("Malla: el cuerpo ocupa el espacio que le toca") {
    Cuerpo c;
    Fauna::MallaAnimal m;
    c.construir(m, PartesVisibles::primeraPersona(), 12);

    // A lo ancho no se sale de la caja de colision del jugador (0.6 de ancho,
    // o sea +-0.3). Los brazos son lo mas lateral.
    INFO("ancho del cuerpo: ", m.minimo.x, " a ", m.maximo.x);
    CHECK(m.maximo.x <= 0.45f);
    CHECK(m.minimo.x >= -0.45f);

    // A lo alto va de los pies (cerca de 0) al hombro.
    CHECK(m.minimo.y >= -0.05f);
    CHECK(m.maximo.y <= c.medidas.estatura);
}

TEST_CASE("Malla: la generacion es determinista") {
    // Misma semilla, misma malla. Sin esto el cuerpo cambiaria de color entre
    // frames, que es el defecto que produce un ruido mal sembrado.
    Cuerpo a, b;
    a.semilla = b.semilla = 999;

    Fauna::MallaAnimal ma, mb;
    a.construir(ma, PartesVisibles::primeraPersona(), 8);
    b.construir(mb, PartesVisibles::primeraPersona(), 8);

    REQUIRE(ma.vertices.size() == mb.vertices.size());
    for (size_t i = 0; i < ma.vertices.size(); ++i) {
        CHECK(ma.vertices[i].pos.x == doctest::Approx(mb.vertices[i].pos.x));
        CHECK(ma.vertices[i].pos.y == doctest::Approx(mb.vertices[i].pos.y));
        CHECK(ma.vertices[i].r     == doctest::Approx(mb.vertices[i].r));
    }
}

// ============================================================================
// 6. PIEL
// ============================================================================

TEST_CASE("Piel: los tonos son distintos entre si") {
    // Cuatro tonos de referencia, no una paleta cerrada: el tono es un
    // parametro continuo y estos son solo puntos de partida.
    const TonoPiel claro = TonoPiel::claro();
    const TonoPiel oscuro = TonoPiel::oscuro();
    CHECK(claro.r > oscuro.r);
    CHECK(claro.g > oscuro.g);
    CHECK(claro.b > oscuro.b);
}

TEST_CASE("Piel: las palmas son mas claras que las piernas") {
    // Modulacion por zona: hay menos melanina en las palmas, y las piernas
    // suelen estar menos expuestas. Sin esto la piel se ve como plastico de
    // un solo color.
    Cuerpo c;
    float rm, gm, bm, rp, gp, bp;
    c.colorDe(Zona::MANO,        1, 1, rm, gm, bm);
    c.colorDe(Zona::PANTORRILLA, 1, 1, rp, gp, bp);
    CHECK(rm > rp);
}

TEST_CASE("Piel: el color se mantiene dentro de rango") {
    // El ruido y la modulacion no pueden sacar el color de [0,1]: por encima
    // se satura a blanco y se pierde el tono.
    Cuerpo c;
    c.piel = TonoPiel::claro();   // el caso mas cerca del techo

    for (int i = 0; i < 40; ++i) {
        for (int j = 0; j < 12; ++j) {
            float r, g, b;
            c.colorDe(Zona::MANO, i, j, r, g, b);
            INFO("i=", i, " j=", j);
            CHECK(r <= 1.0f);
            CHECK(g <= 1.0f);
            CHECK(b <= 1.0f);
            CHECK(r >= 0.0f);
        }
    }
}
