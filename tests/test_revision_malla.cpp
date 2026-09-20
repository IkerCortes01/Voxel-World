#include <doctest/doctest.h>
#include "render/RevisionMalla.h"

#include <cstdint>

using namespace Render;

// ============================================================================
// UNA MALLA VIEJA NO PUEDE BORRAR UNA EDICION NUEVA
// ============================================================================
// EL BUG REPORTADO, TAL CUAL: "cuando pongo un nivel de un bloque en creativo y
// rapidamente rompo el nivel encima de un bloque completo, el bloque se
// renderiza sin actualizarse".
//
// No era un parpadeo: el bloque se quedaba mal PARA SIEMPRE, hasta que otra
// cosa invalidara el chunk por casualidad.
//
// La causa es una carrera entre el jugador y el worker que malla:
//
//   1. Colocar el nivel  -> needsRebuild = true, el chunk se encola.
//   2. El worker saca su FOTO de los bloques. Incluye el nivel.
//   3. Romper el nivel   -> needsRebuild = true otra vez.
//   4. El worker entrega -> `needsRebuild = false` SIN MIRAR NADA.
//
// El paso 4 borraba la peticion del paso 3.
//
// Estos tests fijan el protocolo que lo cierra. No prueban OpenGL ni hilos:
// prueban la DECISION, que es donde estaba el fallo.

// ----------------------------------------------------------------------------
// EL CAMINO NORMAL
// ----------------------------------------------------------------------------

TEST_CASE("Revision: sin escrituras entremedias, el trabajo se da por hecho") {
    // El caso que ocurre el 99% de las veces: el chunk se encola, nadie lo
    // toca, la malla llega al dia. Tiene que quedar limpio -- si aqui se
    // dejara `seguirPendiente`, TODOS los chunks se remallarian en bucle.
    const DecisionIntegracion d = decidirIntegracion(7u, 7u);

    CHECK(d.integrarMalla     == true);
    CHECK(d.seguirPendiente   == false);
    CHECK(d.conservarUrgencia == false);
}

TEST_CASE("Revision: al dia es exactamente igualdad") {
    CHECK(mallaAlDia(0u, 0u)         == true);
    CHECK(mallaAlDia(1u, 1u)         == true);
    CHECK(mallaAlDia(123456u, 123456u) == true);

    CHECK(mallaAlDia(0u, 1u)   == false);
    CHECK(mallaAlDia(5u, 6u)   == false);
}

// ----------------------------------------------------------------------------
// LA CARRERA
// ----------------------------------------------------------------------------

TEST_CASE("Revision: EL BUG -- colocar y romper rapido no pierde la rotura") {
    // ⭐ LA REPRODUCCION EXACTA DEL FALLO REPORTADO.
    //
    // Se simula la secuencia completa con un contador de revision.
    uint32_t revision = 0;

    // 1. El jugador coloca el nivel.
    ++revision;                            // revision = 1
    bool needsRebuild = true;

    // 2. El chunk se encola: el encargo anota la revision que va a ver.
    const uint32_t revisionEnVuelo = revision;   // 1

    // 3. El jugador rompe el nivel MIENTRAS el worker trabaja.
    ++revision;                            // revision = 2
    needsRebuild = true;

    // 4. El worker entrega la malla del paso 2 (la que aun tiene el nivel).
    const DecisionIntegracion d = decidirIntegracion(revisionEnVuelo, revision);

    // La malla se integra: es mas nueva que la que el chunk tiene dibujada.
    CHECK(d.integrarMalla == true);

    // ⭐ PERO EL CHUNK SIGUE PENDIENTE. Esto es lo que antes se perdia.
    CHECK(d.seguirPendiente == true);
    if (d.seguirPendiente) needsRebuild = true; else needsRebuild = false;
    CHECK(needsRebuild == true);
}

TEST_CASE("Revision: la malla desfasada SE INTEGRA, no se descarta") {
    // ⚠️ LA TENTACION EQUIVOCADA, Y POR QUE ESTE TEST EXISTE.
    //
    // Lo "obvio" al ver una malla desfasada es tirarla y esperar a la buena.
    // Ya se midio el efecto de descartar mallas en este motor: el 61% de las
    // de chunks recien tocados se tiraban, y el jugador veia el chunk
    // parpadear en cada golpe.
    //
    // La malla desfasada describe un estado MAS NUEVO que el dibujado.
    // Descartarla deja al jugador mirando geometria aun mas vieja durante otro
    // ciclo entero.
    for (uint32_t enVuelo = 0; enVuelo < 5; ++enVuelo) {
        for (uint32_t actual = enVuelo; actual < enVuelo + 5; ++actual) {
            const DecisionIntegracion d = decidirIntegracion(enVuelo, actual);
            CHECK(d.integrarMalla == true);   // SIEMPRE, al dia o no
        }
    }
}

TEST_CASE("Revision: el remallado hereda la prioridad del jugador") {
    // Una malla solo se desfasa porque alguien escribio mientras el worker
    // trabajaba, y quien escribe a mano es el jugador. Si el remallado entrara
    // por la cola normal, competiria con el terreno nuevo y el bloque tardaria
    // en corregirse justo en el caso donde mas se mira.
    const DecisionIntegracion d = decidirIntegracion(3u, 4u);
    CHECK(d.conservarUrgencia == true);

    // Y al dia NO conserva urgencia: si no, un chunk adelantaria en la cola
    // para siempre y la prioridad dejaria de significar nada.
    const DecisionIntegracion limpia = decidirIntegracion(4u, 4u);
    CHECK(limpia.conservarUrgencia == false);
}

TEST_CASE("Revision: varias escrituras seguidas tambien se recogen") {
    // Construir rapido son muchas escrituras dentro de la misma ventana. Da
    // igual cuantas sean: mientras la revision no coincida, sigue pendiente.
    const uint32_t enVuelo = 10u;
    CHECK(decidirIntegracion(enVuelo, 11u).seguirPendiente == true);
    CHECK(decidirIntegracion(enVuelo, 12u).seguirPendiente == true);
    CHECK(decidirIntegracion(enVuelo, 99u).seguirPendiente == true);
}

TEST_CASE("Revision: el ciclo converge -- el segundo intento cierra") {
    // Que el chunk se quede pendiente solo sirve si el REINTENTO acaba
    // limpiando. Si no, seria un bucle infinito de remallados.
    //
    // Se simula: tras integrar la malla desfasada, el chunk se vuelve a
    // encolar anotando la revision ACTUAL. Si el jugador ya no escribe, la
    // siguiente integracion coincide y el ciclo termina.
    uint32_t revision = 5u;
    uint32_t enVuelo  = 4u;      // encargo viejo

    DecisionIntegracion d = decidirIntegracion(enVuelo, revision);
    REQUIRE(d.seguirPendiente == true);

    // Reencolado: el nuevo encargo ve el contenido de ahora.
    enVuelo = revision;          // 5

    d = decidirIntegracion(enVuelo, revision);
    CHECK(d.seguirPendiente == false);   // cerrado en UN reintento
}

// ----------------------------------------------------------------------------
// DEFENSAS
// ----------------------------------------------------------------------------

TEST_CASE("Revision: el desbordamiento del contador no rompe nada") {
    // `revision` es uint32_t y sube en cada escritura de bloque. Al dar la
    // vuelta, lo unico que importa es que la comparacion sigue siendo una
    // IGUALDAD: para un falso "al dia" harian falta exactamente 2^32
    // escrituras en el mismo chunk mientras una sola malla esta en vuelo.
    const uint32_t max = 0xFFFFFFFFu;

    CHECK(mallaAlDia(max, max) == true);

    uint32_t r = max;
    ++r;                          // da la vuelta a 0
    CHECK(r == 0u);
    CHECK(mallaAlDia(max, r) == false);   // se detecta el cambio
}

TEST_CASE("Revision: un chunk recien creado esta al dia") {
    // Ambos contadores nacen en 0, asi que la primera malla de un chunk nuevo
    // no puede salir marcada como desfasada -- eso remallaria todo el terreno
    // recien generado una segunda vez sin motivo.
    CHECK(mallaAlDia(0u, 0u) == true);
    CHECK(decidirIntegracion(0u, 0u).seguirPendiente == false);
}
