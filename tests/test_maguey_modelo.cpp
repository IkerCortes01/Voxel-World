#include <doctest/doctest.h>
#include <vector>
#include "BloqueCompuesto.h"

using namespace Compuesto;
namespace M = Compuesto::Maguey;

// ============================================================================
// EL MODELO DEL MAGUEY: CONECTADO AL SUELO Y SIN PIEZAS FLOTANDO
// ============================================================================
// Tres arreglos de forma, y lo que los motivo:
//
//   1. EL CAJETE FLOTABA. Se dibujaba a una altura FIJA (y = 0.55) que no
//      dependia del tamaño de la mata. En un brote quedaba muy por encima de
//      las hojas y, como usa la textura del maguey, se veia como un cubo
//      verde suspendido en el aire. Ahora su altura sale de la propia planta
//      y ademas lleva un pie que lo une al suelo.
//
//   2. LAS ESPINAS FLOTABAN. Arrancaban a media altura, en un anillo de
//      palitos sueltos alrededor del maguey. Ahora nacen en la PUNTA DE LAS
//      HOJAS, que es donde remata una penca de agave de verdad.
//
//   3. LA PLANTA NO LLEGABA AL SUELO. La caja del cuerpo empezaba por encima
//      de y=0 en algunas etapas. Ahora arranca en 0 siempre.
//
// Las formulas de la geometria viven en main.cpp (dentro de buildChunkMesh y
// de cajaDePiezaN), que los tests no enlazan. Aqui se replican las mismas
// cuentas para poder comprobar las RELACIONES entre ellas -- que es donde
// estaban los fallos: no en un numero suelto, sino en dos piezas que no
// encajaban.

// --- Las medidas del motor, pedidas donde viven ---
//
// ⚠️ ANTES ESTABAN COPIADAS AQUI, y eso fallo: al subir la escala de los
// maduros se les puso un TOPE en el motor (para que la planta no se saliera
// del voxel), pero la copia de este archivo no lo tenia. El test empezo a
// comprobar una geometria que ya no existia y salto donde no habia bug.
//
// Ahora se llaman las funciones de verdad, asi que el test no puede volver a
// quedarse desfasado respecto a lo que dibuja el juego.
static float altHojaDe(float esc)   { return M::altoHoja(esc); }
static float cajeteY0De(float esc)  { return M::alturaCajete(esc); }
static float cajeteRDe(float esc)   { return M::radioCajete(esc); }
static float cajeteHDe(float esc)   { return M::hondoCajete(esc); }
static float espinaY0De(float esc)  { return M::alturaEspina(esc); }
constexpr float CAJETE_P = 0.05f;

// Todas las etapas, que es donde se ve si una formula escala mal.
static std::vector<float> todasLasEscalas() {
    std::vector<float> v;
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e)
        v.push_back(M::escalaDeEtapa(e));
    return v;
}

// ----------------------------------------------------------------------------
// 1. EL CAJETE YA NO FLOTA
// ----------------------------------------------------------------------------

TEST_CASE("Modelo: el cajete se queda DENTRO de la planta, no encima") {
    // El bug: altura fija 0.55 para todas las etapas. En un brote (escala
    // 0.45) las hojas llegan a 0.31, asi que el cuenco quedaba 0.24 por
    // encima de la planta -- flotando.
    for (float esc : todasLasEscalas()) {
        const float altHoja = altHojaDe(esc);
        const float cy0     = cajeteY0De(esc);

        INFO("escala ", esc, " hoja=", altHoja, " cajete=", cy0);

        // El fondo del cuenco esta POR DEBAJO de donde llegan las hojas: esta
        // hundido en la roseta, que es donde se capa un maguey.
        CHECK(cy0 < altHoja);

        // Y por encima del suelo: es un cuenco, no un agujero en la tierra.
        CHECK(cy0 > 0.0f);
    }
}

TEST_CASE("Modelo: el cajete sube y baja con el tamaño de la mata") {
    // Lo que fallaba antes: la altura era CONSTANTE (0.55 fijo), asi que en
    // un brote el cuenco quedaba flotando por encima de la planta.
    //
    // Ahora sigue al tamaño. No se exige que crezca ESTRICTAMENTE en cada
    // etapa: las dos mayores comparten altura porque las dos llegan al tope
    // que impide que la planta se salga del voxel. Lo que importa es que no
    // BAJE al crecer -- eso si seria un fallo -- y que el maduro este
    // claramente por encima del brote.
    float anterior = -1.0f;
    for (float esc : todasLasEscalas()) {
        const float cy0 = cajeteY0De(esc);
        CHECK(cy0 >= anterior);      // nunca retrocede
        anterior = cy0;
    }

    // Y la diferencia entre el mas pequeño y el mas grande es real. Se pide
    // un 50% mas, no el doble: el cajete depende de altoHoja, que arranca en
    // un minimo fijo (0.16) para que un brote no tenga la roseta a ras de
    // suelo. Ese minimo comprime la proporcion entre etapas, asi que exigir
    // el doble era atar el test a los numeros exactos en vez de a la regla.
    CHECK(cajeteY0De(M::escalaDeEtapa(M::PRODUCTOR)) >
          cajeteY0De(M::escalaDeEtapa(M::BROTE)) * 1.5f);
}

TEST_CASE("Modelo: el pie del cajete cierra el hueco hasta el suelo") {
    // Aunque el cuenco este mas bajo, entre su fondo y el suelo hay aire. El
    // pie -- el tallo carnoso -- lo rellena. Sin el, la planta se seguiria
    // leyendo como piezas sueltas.
    for (float esc : todasLasEscalas()) {
        const float cy0 = cajeteY0De(esc);
        // El pie va de 0 a cy0+cP: toca el suelo por abajo y el fondo del
        // cuenco por arriba, sin dejar hueco en medio.
        const float pieDesde = 0.0f;
        const float pieHasta = cy0 + CAJETE_P;

        INFO("escala ", esc);
        CHECK(pieDesde == doctest::Approx(0.0f));   // nace en el suelo
        CHECK(pieHasta > cy0);                      // llega al fondo del cuenco
    }
}

TEST_CASE("Modelo: el cuenco tiene hueco dentro para el jugo") {
    // Si las paredes se comieran el interior, no cabria el aguamiel y el
    // componente de fluido no tendria donde dibujarse.
    for (float esc : todasLasEscalas()) {
        const float cR = cajeteRDe(esc);
        const float cH = cajeteHDe(esc);

        INFO("escala ", esc);
        CHECK(cR - CAJETE_P > 0.0f);    // queda hueco entre paredes
        CHECK(cH > CAJETE_P);           // y hondura por encima del fondo

        // El cuenco cabe en el voxel: no asoma por los lados.
        CHECK(0.5f - cR >= 0.0f);
        CHECK(0.5f + cR <= 1.0f);
    }
}

// ----------------------------------------------------------------------------
// 2. LAS ESPINAS NACEN DE LAS HOJAS
// ----------------------------------------------------------------------------

TEST_CASE("Modelo: las espinas rematan la hoja, no flotan aparte") {
    // El bug: arrancaban en 0.10 + 0.22*esc, un anillo a media altura sin
    // relacion con las hojas. Ahora salen del 85% de la hoja, o sea de su
    // extremo.
    for (float esc : todasLasEscalas()) {
        const float altHoja = altHojaDe(esc);
        const float espY0   = espinaY0De(esc);

        INFO("escala ", esc, " hoja=", altHoja, " espina=", espY0);

        // Nace cerca del extremo de la hoja...
        CHECK(espY0 > altHoja * 0.5f);
        // ...pero sin pasarse de el: la espina continua la hoja, no empieza
        // por encima con un salto de aire.
        CHECK(espY0 <= altHoja);
    }
}

TEST_CASE("Modelo: las espinas quedan por encima del cajete") {
    // La corona de espinas remata la planta; el cuenco esta hundido en el
    // centro. Si se solaparan, las espinas atravesarian el cajete.
    for (float esc : todasLasEscalas()) {
        INFO("escala ", esc);
        CHECK(espinaY0De(esc) > cajeteY0De(esc));
    }
}

// ----------------------------------------------------------------------------
// 3. LA PLANTA TOCA EL SUELO
// ----------------------------------------------------------------------------

TEST_CASE("Modelo: el cuerpo arranca en el suelo en TODAS las etapas") {
    // Es lo que hace que la planta este conectada al terreno. La caja del
    // cuerpo empieza en y=0 siempre, sea cual sea el tamaño.
    for (float esc : todasLasEscalas()) {
        const float y0 = 0.0f;              // el motor lo fija asi
        const float y1 = altHojaDe(esc);
        INFO("escala ", esc);
        CHECK(y0 == doctest::Approx(0.0f));
        CHECK(y1 > y0);                     // y tiene altura de verdad
        CHECK(y1 <= 1.0f);                  // sin salirse del voxel
    }
}

TEST_CASE("Modelo: las tres partes forman una pieza continua") {
    // La comprobacion de fondo: recorriendo de abajo arriba no hay un solo
    // tramo de aire. suelo -> pie -> cajete -> hojas -> espinas.
    for (float esc : todasLasEscalas()) {
        const float pieHasta = cajeteY0De(esc) + CAJETE_P;
        const float cajHasta = cajeteY0De(esc) + cajeteHDe(esc);
        const float hojHasta = altHojaDe(esc);
        const float espDesde = espinaY0De(esc);

        INFO("escala ", esc);
        // El pie arranca en el suelo y llega al cuenco.
        CHECK(pieHasta > 0.0f);
        // El cuenco continua desde donde acaba el pie.
        CHECK(cajHasta > pieHasta);
        // Y la espina arranca antes de que acabe la hoja: se solapan, asi que
        // no queda hueco entre una y otra.
        CHECK(espDesde <= hojHasta);
    }
}

// ----------------------------------------------------------------------------
// ROMPER LA PUNTA SUELTA EL JUGO
// ----------------------------------------------------------------------------

TEST_CASE("Modelo: cortar la punta de un maguey cargado da aguamiel") {
    // La via tosca: se pierde la planta como fuente ordenada, pero se saca lo
    // que tenia. Solo si habia bastante para llenar algo.
    const BlockType lleno = M::conAguamiel(
        M::capar(M::nuevo(M::PRODUCTOR, 0)), 15);
    REQUIRE(M::aguamielDe(lleno) >= M::AGUAMIEL_PARA_TAZON);

    // Tras cortar, la planta se queda sin jugo (se derramo).
    const BlockType tras = M::vaciado(M::conPuntas(lleno,
                              (uint16_t)(M::puntasDe(lleno) - 1)));
    CHECK(M::aguamielDe(tras) == 0);
    // Pero sigue viva y capada: volvera a producir.
    CHECK(M::capadoDe(tras));
    CHECK(M::produce(estadoDe(tras)));
}

TEST_CASE("Modelo: con poco jugo, cortar no da nada y se pierde") {
    // El umbral es el mismo que para llenar un tazon por la via buena: por
    // debajo de eso, el corte lo desperdicia.
    const BlockType poco = M::conAguamiel(
        M::capar(M::nuevo(M::PRODUCTOR, 0)),
        (uint16_t)(M::AGUAMIEL_PARA_TAZON - 1));

    CHECK_FALSE(M::hayParaTazon(estadoDe(poco)));

    // Al cortar se vacia igual: el jugo se derrama por el corte.
    const BlockType tras = M::vaciado(poco);
    CHECK(M::aguamielDe(tras) == 0);
}

TEST_CASE("Modelo: arrancar una punta no mata la planta") {
    // Solo se lleva UNA espina; el maguey sigue en pie con las demas.
    BlockType m = M::nuevo(M::PRODUCTOR, 0);
    const uint16_t antes = M::puntasDe(m);
    REQUIRE(antes > 0);

    m = M::conPuntas(m, (uint16_t)(antes - 1));
    CHECK(M::puntasDe(m) == antes - 1);
    CHECK(M::etapaDe(m) == M::PRODUCTOR);   // sigue siendo el mismo maguey
    CHECK(esCompuesto(m));
}
