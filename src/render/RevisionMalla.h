#pragma once

#include <cstdint>

// ============================================================================
// REVISION DE CONTENIDO: QUE EDICION VIO LA MALLA QUE ACABA DE LLEGAR
// ============================================================================
// EL BUG QUE ESTO CIERRA
//
// Colocar un nivel de bloque y romperlo INMEDIATAMENTE dejaba el bloque de
// debajo dibujado con la geometria vieja, y ahi se quedaba -- no era un
// parpadeo de un frame, era permanente.
//
// LA CARRERA, PASO A PASO:
//
//   1. El jugador coloca el nivel -> needsRebuild = true, el chunk se encola.
//   2. El worker coge el encargo y saca su FOTO de los bloques. Incluye el
//      nivel.
//   3. El jugador rompe el nivel  -> needsRebuild = true otra vez. Pero el
//      worker ya tiene su foto: esta construyendo un mundo que ya no existe.
//   4. El worker entrega. Al integrar se hacia `needsRebuild = false` SIN
//      MIRAR NADA -- borrando la peticion del paso 3.
//
// El chunk quedaba en LISTO, sin bandera, con geometria obsoleta y sin que
// nadie volviera a pedir el remallado.
//
// ----------------------------------------------------------------------------
// POR QUE NO SE ARREGLA HACIENDO QUE EL WORKER MIRE `needsRebuild`
// ----------------------------------------------------------------------------
// Ya se intento y fue peor. El worker lo IGNORA a proposito: respetarlo hacia
// que el 61% de las mallas de chunks recien tocados se DESCARTARAN, porque un
// mismo chunk entra varias veces en la cola (setBlock invalida el chunk Y sus
// vecinos de frontera) y el encargo de turno se encuentra la bandera ya bajada
// por el encargo anterior. Cada descarte cuesta un ciclo entero.
//
// ----------------------------------------------------------------------------
// LA SOLUCION: SON DOS PREGUNTAS DISTINTAS
// ----------------------------------------------------------------------------
//   needsRebuild -> "hay trabajo pendiente"       (bandera del hilo principal)
//   revision     -> "QUE version del contenido"   (viaja con el encargo)
//
// `revision` sube en cada escritura de bloque. El encargo se lleva anotada la
// que vio. Al integrar se comparan:
//
//   - IGUALES:   la malla esta al dia -> needsRebuild = false. Trabajo hecho.
//   - DISTINTAS: la malla describe contenido superado -> se INTEGRA IGUAL
//                (es mas nueva que la que hay puesta; descartarla devolveria
//                el parpadeo) pero needsRebuild se queda en true y el chunk
//                vuelve a la cola.
//
// Asi se conservan las dos propiedades a la vez: el worker nunca tira trabajo
// hecho, y ninguna edicion del jugador se pierde.
// ============================================================================

namespace Render {

// Lo que hay que decidir al integrar una malla que vuelve de un worker.
struct DecisionIntegracion {
    bool integrarMalla;      // ¿se sube la geometria a la GPU?
    bool seguirPendiente;    // ¿el chunk se queda con needsRebuild?
    bool conservarUrgencia;  // ¿mantiene la prioridad del jugador?
};

// `revisionEnVuelo` es la que el chunk tenia cuando se encolo el encargo.
// `revisionActual`  es la que tiene AHORA, al integrar.
//
// Si el jugador escribio entremedias, la segunda es mayor.
inline DecisionIntegracion decidirIntegracion(uint32_t revisionEnVuelo,
                                              uint32_t revisionActual) {
    const bool alDia = (revisionEnVuelo == revisionActual);

    DecisionIntegracion d;

    // ⭐ LA MALLA SE INTEGRA SIEMPRE, y es lo importante de este diseño.
    //
    // Aunque le falte la ultima edicion, describe un estado MAS NUEVO que el
    // que el chunk tiene dibujado. Tirarla para "esperar a la buena" deja al
    // jugador mirando geometria aun mas vieja durante otro ciclo completo, que
    // es exactamente el parpadeo que costo arreglar.
    d.integrarMalla = true;

    // Lo unico que cambia es si el trabajo se da por terminado.
    d.seguirPendiente = !alDia;

    // Una malla desfasada solo ocurre porque alguien escribio mientras el
    // worker trabajaba, y quien escribe a mano es el jugador. El remallado
    // hereda esa prioridad: va por delante del terreno nuevo.
    d.conservarUrgencia = !alDia;

    return d;
}

// ¿Esta malla describe el contenido actual del chunk?
inline bool mallaAlDia(uint32_t revisionEnVuelo, uint32_t revisionActual) {
    return revisionEnVuelo == revisionActual;
}

} // namespace Render
