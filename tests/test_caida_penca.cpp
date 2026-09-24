#include <doctest/doctest.h>
#include "render/CaidaPenca.h"

#include <cmath>

using namespace Render;

// ============================================================================
// LA PENCA SE ESTIRABA AL COLOCARLA
// ============================================================================
// BUG REPORTADO: "cuando las coloco se estiran mucho y luego vuelven a su
// forma".
//
// Era literal. Los dos numeros estaban en el codigo y nadie los habia cruzado:
//
//     la animacion de caida terminaba en   X = 15 px   (el LARGO)
//     el reposo la dibuja en               X =  8 px   (el ANCHO)
//
// Durante los 0,40 s de la caida la penca crecia hasta casi el doble de ancha
// y al acabar pegaba un tiron de vuelta. Lo mismo en Y: la animacion la dejaba
// apoyada en el suelo del voxel y el reposo la pone centrada.
//
// La causa es que las dos cajas se calculaban POR SEPARADO: la animacion
// interpolaba hacia una penca tumbada imaginaria en vez de hacia LA penca
// tumbada que el mesher dibuja un instante despues.

namespace {
// Las medidas reales de la penca en el motor, en fraccion de voxel.
constexpr float LARGO  = 15.0f / 16.0f;
constexpr float ANCHO  =  8.0f / 16.0f;
constexpr float GRUESO = 13.0f / 16.0f;

float anchoDe(const CajaPenca& c) { return c.maxX - c.minX; }
float altoDe (const CajaPenca& c) { return c.maxY - c.minY; }
float fondoDe(const CajaPenca& c) { return c.maxZ - c.minZ; }
} // namespace

// ----------------------------------------------------------------------------
// EL TEST DEL BUG
// ----------------------------------------------------------------------------

TEST_CASE("Penca: la caida ACABA exactamente en la caja de reposo") {
    // ⭐⭐ ESTE ES EL BUG, Y LA UNICA PROPIEDAD QUE DE VERDAD IMPORTA.
    //
    // Si el ultimo fotograma de la animacion no coincide con lo que el mesher
    // dibuja despues, el jugador ve un SALTO. Da igual lo bonita que sea la
    // curva: lo que se nota es el tiron del final.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    const CajaPenca fin = cajaCayendo(dePie, reposo, 1.0f);

    CHECK(fin.minX == doctest::Approx(reposo.minX));
    CHECK(fin.maxX == doctest::Approx(reposo.maxX));
    CHECK(fin.minY == doctest::Approx(reposo.minY));
    CHECK(fin.maxY == doctest::Approx(reposo.maxY));
    CHECK(fin.minZ == doctest::Approx(reposo.minZ));
    CHECK(fin.maxZ == doctest::Approx(reposo.maxZ));
}

TEST_CASE("Penca: NO se estira en ningun momento de la caida") {
    // El sintoma que se reporto, medido directamente: en ningun instante la
    // penca puede ser mas ancha que su medida mas larga.
    //
    // Con el bug, a mitad de caida el eje X llegaba a 15 px -- el LARGO -- que
    // es casi el doble de sus 8 px de ancho.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    for (int i = 0; i <= 20; ++i) {
        const float t = (float)i / 20.0f;
        const CajaPenca c = cajaCayendo(dePie, reposo, t);

        INFO("t=", t, "  ancho=", anchoDe(c), "  alto=", altoDe(c),
             "  fondo=", fondoDe(c));

        // Ninguna dimension puede superar la mayor de la penca (el largo).
        CHECK(anchoDe(c) <= LARGO + 1e-4f);
        CHECK(altoDe(c)  <= LARGO + 1e-4f);
        CHECK(fondoDe(c) <= LARGO + 1e-4f);

        // Y ninguna puede hacerse negativa (caja del reves).
        CHECK(anchoDe(c) > 0.0f);
        CHECK(altoDe(c)  > 0.0f);
        CHECK(fondoDe(c) > 0.0f);
    }
}

TEST_CASE("Penca: el ancho NO crece durante la caida") {
    // ⭐ LA FORMA PRECISA DEL SINTOMA.
    //
    // De pie y tumbada la penca mide lo MISMO de ancho (8 px): la caida solo
    // la gira, no la deforma. Asi que el ancho tiene que ser constante de
    // principio a fin.
    //
    // Con el bug crecia hasta 15 y volvia. Este test lo habria cazado.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    const float anchoInicial = anchoDe(dePie);
    for (int i = 0; i <= 20; ++i) {
        const float t = (float)i / 20.0f;
        const CajaPenca c = cajaCayendo(dePie, reposo, t);
        INFO("t=", t, "  ancho=", anchoDe(c), "  inicial=", anchoInicial);
        CHECK(anchoDe(c) == doctest::Approx(anchoInicial).epsilon(0.02));
    }
}

TEST_CASE("Penca: la caida EMPIEZA de pie") {
    // El otro extremo. Si t=0 no diera la penca de pie, la animacion
    // arrancaria con un salto en vez de acabar con el.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    const CajaPenca ini = cajaCayendo(dePie, reposo, 0.0f);
    CHECK(ini.minY == doctest::Approx(dePie.minY));
    CHECK(ini.maxY == doctest::Approx(dePie.maxY));
    CHECK(altoDe(ini) == doctest::Approx(LARGO));   // de pie: larga en Y
}

TEST_CASE("Penca: de pie es alta y tumbada es baja") {
    // La comprobacion de cordura de todo el sistema: caerse tiene que cambiar
    // la silueta. Si las dos cajas fueran parecidas, la animacion no se veria.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    INFO("de pie: alto=", altoDe(dePie), "  tumbada: alto=", altoDe(reposo));
    CHECK(altoDe(dePie) > altoDe(reposo));

    // Y tumbada ocupa mas planta: el largo pasa al plano horizontal... o no.
    // OJO: en este modelo la penca tumbada NO se alarga en el plano, conserva
    // su ancho. Lo que cambia es que su eje LARGO deja de estar en vertical.
    CHECK(altoDe(reposo) == doctest::Approx(GRUESO));
}

// ----------------------------------------------------------------------------
// APOYARSE EN UN NIVEL DE BLOQUE
// ----------------------------------------------------------------------------

TEST_CASE("Penca: sobre medio bloque BAJA, no flota") {
    // Un nivel parcial no llena su voxel: media losa llega a 0.5, no a 1.0.
    // La penca tumbada se dibuja centrada en SU voxel, asi que sobre medio
    // bloque quedaria flotando con el hueco a la vista.
    //
    // Es el mismo ajuste que ya hacian los guijarros.
    const CajaPenca sobreEntero = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);
    const CajaPenca sobreMedio  = cajaReposoTumbada(ANCHO, GRUESO, 0.5f);

    INFO("sobre bloque entero minY=", sobreEntero.minY,
         "   sobre medio bloque minY=", sobreMedio.minY);

    // Baja exactamente lo que le falta al nivel para llegar arriba.
    CHECK(sobreMedio.minY == doctest::Approx(sobreEntero.minY - 0.5f));
    CHECK(sobreMedio.maxY == doctest::Approx(sobreEntero.maxY - 0.5f));

    // Y no cambia de tamano al bajar: solo se mueve.
    CHECK(altoDe(sobreMedio) == doctest::Approx(altoDe(sobreEntero)));
    CHECK(anchoDe(sobreMedio) == doctest::Approx(anchoDe(sobreEntero)));
}

TEST_CASE("Penca: sobre un nivel, la caida tampoco da tiron") {
    // ⭐ EL CASO QUE SE PIDIO: "en cualquier bloque, nivel de bloque".
    //
    // Si la animacion acabara centrada y el reposo bajado, el salto reaparece
    // -- solo que en vertical en vez de en anchura. Hay que aplicar el mismo
    // descenso a los dos.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.5f);

    const CajaPenca fin = cajaCayendo(dePie, reposo, 1.0f);
    CHECK(fin.minY == doctest::Approx(reposo.minY));
    CHECK(fin.maxY == doctest::Approx(reposo.maxY));
}

// ----------------------------------------------------------------------------
// LA CURVA
// ----------------------------------------------------------------------------

TEST_CASE("Penca: la caida acelera, no es lineal") {
    // Una interpolacion recta se ve mecanica. Con t*t la penca arranca
    // despacio y acelera, que es como cae algo por su peso.
    //
    // Se comprueba en la mitad del recorrido: con t*t, a t=0.5 solo se ha
    // recorrido el 25%, no el 50%.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    const CajaPenca mitad = cajaCayendo(dePie, reposo, 0.5f);

    const float recorrido = (dePie.maxY - mitad.maxY) /
                            (dePie.maxY - reposo.maxY);
    INFO("a mitad de tiempo se ha recorrido el ", recorrido * 100.0f, "%");
    CHECK(recorrido < 0.40f);   // claramente por debajo del 50% lineal
    CHECK(recorrido > 0.10f);   // pero se ha movido
}

TEST_CASE("Penca: la caida es monotona, no rebota") {
    // La penca baja y se queda. Si en algun tramo subiera, se veria como un
    // rebote raro a mitad del gesto.
    const CajaPenca dePie  = cajaDePie(LARGO, ANCHO, GRUESO);
    const CajaPenca reposo = cajaReposoTumbada(ANCHO, GRUESO, 0.0f);

    float anterior = 1e9f;
    for (int i = 0; i <= 20; ++i) {
        const float t = (float)i / 20.0f;
        const CajaPenca c = cajaCayendo(dePie, reposo, t);
        CHECK(c.maxY <= anterior + 1e-5f);   // nunca sube
        anterior = c.maxY;
    }
}
