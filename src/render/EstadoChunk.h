#pragma once

#include <cstdint>

// ============================================================================
// LA MAQUINA DE ESTADOS DE UN CHUNK
// ============================================================================
// RESPONSABILIDAD UNICA: decir en que punto del camino esta un chunk y que
// transiciones son legales.
//
// No conoce World, no conoce OpenGL, no guarda punteros. Se puede probar sin
// arrancar el juego -- igual que Dormir.h o MallaChunk.h.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA QUE RESUELVE
// ----------------------------------------------------------------------------
// Hasta ahora el estado de un chunk vivia repartido en SIETE banderas sueltas
// dentro de `struct Chunk`:
//
//     needsRebuild, isGenerated, waitingForNeighbors, isUpdatingMesh,
//     isBeingGenerated, buildRetries, esperasVecinos
//
// mas TRES conjuntos externos en World (genInFlight, mallaInFlight,
// mallaDone). Catorce combinaciones son alcanzables y solo seis tienen
// sentido. Los estados atrapados que el motor ha ido sufriendo -- y que estan
// documentados uno por uno en los comentarios de main.cpp -- son todos la
// misma clase de fallo: una combinacion de banderas de la que ninguna ruta
// sabia salir.
//
//   "waitingForNeighbors=true + needsRebuild=false"  -> no se malla nunca
//   "isUpdatingMesh heredado del pool"               -> ni se malla ni se dibuja
//   "buildRetries=7 heredado"                        -> se abandona al primer fallo
//
// Con un enum explicito esas combinaciones dejan de existir: un chunk esta en
// UN estado, y las transiciones legales son una tabla que se puede leer de un
// vistazo y verificar en un test.
//
// ----------------------------------------------------------------------------
// POR QUE UN ENUM Y NO MAS BANDERAS
// ----------------------------------------------------------------------------
// Porque el watchdog necesita preguntar "¿cuanto lleva ESTE chunk sin avanzar?"
// y eso exige un unico punto de verdad con una marca de tiempo. Con siete
// banderas no hay forma de responder sin inventarse una heuristica -- que es
// exactamente lo que hacia el vigilante de 60 frames.
// ============================================================================

namespace Streaming {

// ----------------------------------------------------------------------------
// LOS ESTADOS
// ----------------------------------------------------------------------------
// El orden importa: es el orden del camino feliz, y hay tests que comprueban
// que una transicion nunca retrocede salvo por las puertas explicitas
// (invalidacion y cancelacion).
enum class Estado : uint8_t {
    // --- Camino de carga ---
    DESCARGADO = 0,   // no existe en memoria
    SOLICITADO,       // en la cola de generacion, nadie lo ha cogido
    GENERANDO,        // un worker esta construyendo su terreno
    GENERADO,         // tiene bloques; aun no tiene geometria
    MALLA_EN_COLA,    // encolado a un worker de mallado
    MALLANDO,         // un worker esta construyendo su geometria
    MALLA_LISTA,      // la geometria esta en RAM, falta subirla a la GPU
    SUBIENDO,         // en la cola de subida (hilo principal, presupuestado)
    LISTO,            // tiene VBOs validos: el render puede dibujarlo

    // --- Estados de espera (no son fallos) ---
    ESPERA_VECINO,    // no puede mallarse aun: le faltan vecinos generados

    // --- Camino de descarga ---
    DESCARGA_EN_COLA, // marcado para salir; aun puede rescatarse si el jugador vuelve

    // --- Estado terminal de fallo ---
    // Se llega tras agotar los reintentos. NO es un agujero silencioso: el
    // chunk queda marcado, el overlay lo pinta en rojo y las metricas lo
    // cuentan. Es "fallo conocido y diagnosticado", que es lo contrario de
    // "chunk atascado para siempre".
    FALLIDO,

    _COUNT
};

inline const char* nombreEstado(Estado e) {
    switch (e) {
        case Estado::DESCARGADO:       return "DESCARGADO";
        case Estado::SOLICITADO:       return "SOLICITADO";
        case Estado::GENERANDO:        return "GENERANDO";
        case Estado::GENERADO:         return "GENERADO";
        case Estado::MALLA_EN_COLA:    return "MALLA_EN_COLA";
        case Estado::MALLANDO:         return "MALLANDO";
        case Estado::MALLA_LISTA:      return "MALLA_LISTA";
        case Estado::SUBIENDO:         return "SUBIENDO";
        case Estado::LISTO:            return "LISTO";
        case Estado::ESPERA_VECINO:    return "ESPERA_VECINO";
        case Estado::DESCARGA_EN_COLA: return "DESCARGA_EN_COLA";
        case Estado::FALLIDO:          return "FALLIDO";
        default:                       return "?";
    }
}

// ----------------------------------------------------------------------------
// ¿ESTE ESTADO PUEDE QUEDARSE QUIETO PARA SIEMPRE?
// ----------------------------------------------------------------------------
// Los estados TRANSITORIOS son aquellos en los que alguien deberia estar
// trabajando. Si uno de estos no avanza en un plazo razonable, es que el
// trabajo se perdio -- y ahi es donde entra el watchdog.
//
// DESCARGADO, LISTO y FALLIDO son estables por definicion: nadie tiene que
// hacer nada, asi que no hay nada que vigilar.
//
// ⚠️ ESPERA_VECINO SI ES TRANSITORIO. Es la trampa historica de este motor:
// parece un estado de reposo legitimo ("estoy esperando, es normal"), y por eso
// el codigo viejo lo excluia del rescate. Pero si el vecino no llega NUNCA
// -- porque esta en el borde del mundo cargado, o porque su generacion fallo --
// el chunk se queda ahi para siempre. Tiene que vigilarse como cualquier otro.
inline bool esTransitorio(Estado e) {
    switch (e) {
        case Estado::SOLICITADO:
        case Estado::GENERANDO:
        case Estado::MALLA_EN_COLA:
        case Estado::MALLANDO:
        case Estado::MALLA_LISTA:
        case Estado::SUBIENDO:
        case Estado::ESPERA_VECINO:
        case Estado::DESCARGA_EN_COLA:
            return true;
        // GENERADO no se vigila: es un estado de paso que el planificador
        // recoge en el mismo frame o en el siguiente. Vigilarlo produciria
        // falsos positivos con el presupuesto de mallado agotado.
        case Estado::GENERADO:
        case Estado::DESCARGADO:
        case Estado::LISTO:
        case Estado::FALLIDO:
        default:
            return false;
    }
}

// ¿El render puede dibujar un chunk en este estado?
//
// Solo LISTO. Es la separacion que pedia el diseño: el renderer no genera, no
// malla y no espera -- solo dibuja lo que ya tiene VBOs.
inline bool esDibujable(Estado e) {
    return e == Estado::LISTO;
}

// ¿Tiene bloques utilizables? (fisica, raycast, guardado, luz)
//
// Distinto de esDibujable: un chunk GENERADO todavia no se ve, pero el jugador
// ya puede chocar con el y el guardado ya puede escribirlo. Confundir las dos
// preguntas es lo que produce "el jugador cae a traves del mundo mientras
// carga".
inline bool tieneBloques(Estado e) {
    switch (e) {
        case Estado::GENERADO:
        case Estado::MALLA_EN_COLA:
        case Estado::MALLANDO:
        case Estado::MALLA_LISTA:
        case Estado::SUBIENDO:
        case Estado::LISTO:
        case Estado::ESPERA_VECINO:
        case Estado::DESCARGA_EN_COLA:
            return true;
        default:
            return false;
    }
}

// ----------------------------------------------------------------------------
// ¿ESTA TRANSICION ES LEGAL?
// ----------------------------------------------------------------------------
// Se centraliza aqui para que sea una TABLA y no una regla repartida por el
// codigo. Un test recorre las 144 combinaciones y fija exactamente cuales
// valen; cualquier cambio futuro que rompa una invariante sale en el test y no
// en forma de chunk invisible tres semanas despues.
inline bool transicionValida(Estado de, Estado a) {
    if (de == a) return true;   // re-afirmar el estado nunca es un error

    // Desde cualquier estado se puede caer a FALLIDO (los reintentos se
    // agotaron) o a DESCARGADO (el chunk se libero de verdad). Son las dos
    // valvulas de escape que garantizan que no hay callejones sin salida.
    if (a == Estado::FALLIDO || a == Estado::DESCARGADO) return true;

    // Y desde cualquier estado con bloques se puede volver a GENERADO: es la
    // INVALIDACION (el jugador rompio un bloque, o el watchdog cancelo un
    // trabajo perdido). Vuelve al punto en que hay que remallar, sin repetir
    // la generacion del terreno, que sigue siendo valida.
    if (a == Estado::GENERADO && tieneBloques(de)) return true;

    switch (de) {
        case Estado::DESCARGADO:
            return a == Estado::SOLICITADO;

        case Estado::SOLICITADO:
            // GENERADO directo: el chunk estaba en disco o en el cache y no
            // hizo falta generarlo.
            return a == Estado::GENERANDO || a == Estado::GENERADO;

        case Estado::GENERANDO:
            return a == Estado::GENERADO;

        case Estado::GENERADO:
            return a == Estado::MALLA_EN_COLA || a == Estado::ESPERA_VECINO ||
                   a == Estado::DESCARGA_EN_COLA;

        case Estado::MALLA_EN_COLA:
            return a == Estado::MALLANDO || a == Estado::ESPERA_VECINO ||
                   a == Estado::DESCARGA_EN_COLA;

        case Estado::MALLANDO:
            return a == Estado::MALLA_LISTA || a == Estado::ESPERA_VECINO;

        case Estado::MALLA_LISTA:
            return a == Estado::SUBIENDO || a == Estado::DESCARGA_EN_COLA;

        case Estado::SUBIENDO:
            return a == Estado::LISTO;

        case Estado::LISTO:
            // Un chunk LISTO vuelve a MALLA_EN_COLA cuando se le invalida la
            // geometria (lo cubre la regla de GENERADO de arriba, pero se
            // permite tambien el salto directo para el remallado por luz).
            return a == Estado::MALLA_EN_COLA || a == Estado::DESCARGA_EN_COLA;

        case Estado::ESPERA_VECINO:
            // El vecino llego (o se agoto la paciencia y se malla igual).
            return a == Estado::MALLA_EN_COLA || a == Estado::DESCARGA_EN_COLA;

        case Estado::DESCARGA_EN_COLA:
            // ⭐ EL RESCATE. El jugador dio media vuelta antes de que se
            // liberara: el chunk sigue entero en memoria, asi que volver a
            // LISTO es gratis. Es lo que evita el ciclo carga/descarga en las
            // fronteras, junto con la histeresis.
            return a == Estado::LISTO || a == Estado::GENERADO;

        case Estado::FALLIDO:
            // Se reintenta desde cero cuando el jugador vuelve a acercarse.
            return a == Estado::SOLICITADO;

        default:
            return false;
    }
}

} // namespace Streaming
