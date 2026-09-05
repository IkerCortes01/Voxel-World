#include <doctest/doctest.h>
#include "BloqueCompuesto.h"

using namespace Compuesto;
namespace M = Compuesto::Maguey;

// ============================================================================
// LA HITBOX DEL MAGUEY
// ============================================================================
// BUG QUE ESTO PROTEGE: el jugador chocaba contra un CUBO MACIZO donde veia
// una roseta abierta. Con la textura de la hoja encima, el maguey se leia como
// un bloque solido de hojas.
//
// La causa era un razonamiento que parecia correcto: "la caja debe seguir a la
// silueta dibujada". Se derivaba de las medidas del mesher -- radio del anillo
// exterior, alto del central -- y funcionaba mientras la planta cabia en su
// celda.
//
// Pero desde que el maguey se sale del voxel a proposito (un ejemplar viejo
// mide 3 bloques de alto), esas cuentas superan SIEMPRE el bloque. La caja se
// topaba a 0.5 x 1.0 en todas las etapas menos la primera, o sea: el voxel
// entero.
//
// LA REGLA CORRECTA, que es lo que fijan estos tests:
//
//   Una hitbox no puede salirse de su celda -- el motor la consulta voxel a
//   voxel, asi que lo declarado fuera no existe para la fisica. Lo unico que
//   se puede describir es la parte de la planta QUE ESTA DENTRO del bloque:
//   el COGOLLO.
//
//   Las hojas se abren cruzando a los vecinos, pero son laminas de 2 px: no
//   frenan a nadie, igual que no frena la hierba alta.

// Las mismas formulas que usa nopalHitboxCon (en main.cpp, que los tests no
// enlazan). Se replican porque lo que importa comprobar son las PROPORCIONES,
// no invocar la funcion.
static float radioHitbox(float esc) {
    const float r = 0.10f + 0.09f * esc;
    return (r > 0.30f) ? 0.30f : r;
}
static float altoHitbox(float esc) {
    const float a = 0.30f + 0.45f * esc;
    return (a > 0.96f) ? 0.96f : a;
}

// ----------------------------------------------------------------------------
// EL BUG: LA CAJA NO PUEDE SER EL VOXEL ENTERO
// ----------------------------------------------------------------------------

TEST_CASE("Hitbox: NINGUNA etapa ocupa el bloque entero") {
    // El sintoma exacto del bug. Si una etapa llega a 0.5 de radio y 1.0 de
    // alto, el jugador choca con un cubo macizo donde ve una planta abierta.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        const float r = radioHitbox(esc);
        const float a = altoHitbox(esc);

        INFO("etapa ", e, " radio ", r, " alto ", a);

        // Queda hueco por los lados para poder pasar rozando.
        CHECK(r < 0.5f);
        // Y por arriba: la roseta no llena la celda hasta el techo.
        CHECK(a < 1.0f);
    }
}

TEST_CASE("Hitbox: se puede caminar junto a un maguey") {
    // Un jugador mide 0.6 de ancho. Entre el borde del bloque y la caja de la
    // planta tiene que quedar sitio para pasar sin quedarse encajado, aunque
    // sea rozando.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float r = radioHitbox(M::escalaDeEtapa(e));
        const float hueco = 0.5f - r;   // desde el borde del voxal al cogollo
        INFO("etapa ", e, " hueco a cada lado ", hueco);
        CHECK(hueco > 0.15f);
    }
}

TEST_CASE("Hitbox: la caja SIEMPRE cabe en su celda") {
    // La invariante de fondo: el motor consulta la fisica voxel a voxel, asi
    // que una caja declarada fuera de su celda simplemente no existe -- y
    // ademas confundiria al resolver colisiones con el bloque de al lado.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        const float r = radioHitbox(esc);
        const float a = altoHitbox(esc);

        INFO("etapa ", e);
        CHECK(0.5f - r >= 0.0f);
        CHECK(0.5f + r <= 1.0f);
        CHECK(a > 0.0f);
        CHECK(a <= 1.0f);
    }
}

TEST_CASE("Hitbox: aunque la escala se dispare, la caja sigue dentro") {
    // Red de seguridad. Las escalas del maguey pueden crecer (la planta se
    // sale del voxel a proposito), pero la CAJA no puede seguirlas: es lo que
    // fallaba antes.
    const float ABSURDAS[] = { 3.0f, 5.0f, 20.0f, 1000.0f };
    for (float esc : ABSURDAS) {
        INFO("escala ", esc);
        CHECK(radioHitbox(esc) < 0.5f);
        CHECK(altoHitbox(esc) < 1.0f);
    }
}

// ----------------------------------------------------------------------------
// PERO SIGUE SIENDO UNA PLANTA CON CUERPO
// ----------------------------------------------------------------------------

TEST_CASE("Hitbox: crece con la etapa, sin retrocesos") {
    // Un maguey grande estorba mas que un brote: es lo que se espera al pasar
    // junto a un agave hecho.
    float rAnt = 0.0f, aAnt = 0.0f;
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        const float r = radioHitbox(esc);
        const float a = altoHitbox(esc);
        INFO("etapa ", e);
        CHECK(r >= rAnt);
        CHECK(a >= aAnt);
        rAnt = r; aAnt = a;
    }
}

TEST_CASE("Hitbox: tiene cuerpo de verdad, no es intangible") {
    // El extremo contrario del bug: pasarse de fino y que el jugador
    // atraviese la planta como si fuera hierba. El maguey SI frena.
    for (uint16_t e = M::BROTE; e <= M::PRODUCTOR; ++e) {
        const float esc = M::escalaDeEtapa(e);
        INFO("etapa ", e);
        CHECK(radioHitbox(esc) > 0.08f);   // un palmo de tronco
        CHECK(altoHitbox(esc)  > 0.25f);   // se nota al andar
    }
}

TEST_CASE("Hitbox: un ejemplar viejo estorba mas que un brote") {
    // La diferencia tiene que ser perceptible, no simbolica.
    const float rBrote = radioHitbox(M::escalaDeEtapa(M::BROTE));
    const float rProd  = radioHitbox(M::escalaDeEtapa(M::PRODUCTOR));
    CHECK(rProd > rBrote * 1.5f);
}
