#pragma once

#include "EstadoChunk.h"
#include <cstdint>

// ============================================================================
// EL VIGILANTE: NINGUN CHUNK SE QUEDA ATASCADO
// ============================================================================
// RESPONSABILIDAD UNICA: dado (estado, cuanto lleva en el, cuantas veces se ha
// reintentado), decir que hay que hacer.
//
// Funcion pura sobre datos planos. Sin OpenGL, sin World, sin reloj propio --
// el tiempo entra por parametro, que es lo que permite probar en un test un
// timeout de 30 segundos sin esperar 30 segundos.
//
// ----------------------------------------------------------------------------
// POR QUE NO VALE EL VIGILANTE ANTERIOR
// ----------------------------------------------------------------------------
// El motor ya tenia uno, en World::render:
//
//     if (chunk->batches.empty() && ++chunk->framesConCandado > 60) { soltar; }
//
// Tres defectos de fondo:
//
//   1. CUENTA FRAMES, NO TIEMPO. A 200 FPS son 0.3 s; a 20 FPS son 3 s. O sea
//      que el umbral significa cosas distintas segun la carga -- y justo
//      cuando el sistema va mal (pocos FPS) es cuando mas tarda en reaccionar.
//
//   2. SOLO VIGILA UN ESTADO. Mira el candado de mallado y nada mas. Un chunk
//      perdido en la cola de generacion, o esperando un vecino que no va a
//      llegar, no lo ve nadie.
//
//   3. NO DISTINGUE "PERDIDO" DE "OCUPADO". Su unica señal era "lleva 60
//      frames", asi que con los workers cargados soltaba el candado de chunks
//      que un worker estaba usando -- autorizando a reciclar memoria viva.
//      El vigilante podia CAUSAR el fallo que existia para evitar.
//
// Aqui el tiempo es real, se vigilan todos los estados transitorios, y la
// accion depende del estado concreto en vez de ser siempre "suelta el candado".
// ============================================================================

namespace Streaming {

// ----------------------------------------------------------------------------
// LOS PLAZOS
// ----------------------------------------------------------------------------
// En SEGUNDOS reales. Son generosos a proposito: el vigilante es una red de
// seguridad para trabajo PERDIDO, no un planificador. Si los plazos fueran
// ajustados, cancelaria trabajo legitimo que solo iba lento, y eso genera mas
// trabajo (recargar lo cancelado) justo cuando el sistema ya va justo -- una
// espiral.
//
// La referencia: con el presupuesto mas restrictivo (1 chunk/frame a 30 FPS) y
// las colas llenas (24 gen + 16 malla), el peor caso legitimo para atravesar
// una cola entera ronda 1.5 s. Los plazos son ~4x eso.
struct Plazos {
    float solicitado   = 8.0f;   // en cola de generacion sin que nadie lo coja
    float generando    = 10.0f;  // un worker lo tiene: generar un chunk son ms
    float mallaEnCola  = 8.0f;
    float mallando     = 10.0f;
    float mallaLista   = 5.0f;   // esperando subida: el hilo principal manda
    float subiendo     = 3.0f;   // subir son ms; 3 s es que se perdio
    float esperaVecino = 6.0f;   // ⭐ el que faltaba en el sistema anterior
    float descargaCola = 15.0f;  // marcado para salir y nadie lo saca

    float de(Estado e) const {
        switch (e) {
            case Estado::SOLICITADO:       return solicitado;
            case Estado::GENERANDO:        return generando;
            case Estado::MALLA_EN_COLA:    return mallaEnCola;
            case Estado::MALLANDO:         return mallando;
            case Estado::MALLA_LISTA:      return mallaLista;
            case Estado::SUBIENDO:         return subiendo;
            case Estado::ESPERA_VECINO:    return esperaVecino;
            case Estado::DESCARGA_EN_COLA: return descargaCola;
            default:                       return 0.0f;   // no se vigila
        }
    }
};

// ----------------------------------------------------------------------------
// QUE HACER CON UN CHUNK ATASCADO
// ----------------------------------------------------------------------------
enum class Accion : uint8_t {
    NINGUNA = 0,      // todo en orden
    REENCOLAR,        // cancelar el trabajo perdido y volver a pedirlo
    FORZAR_MALLADO,   // dejar de esperar al vecino y mallar con lo que hay
    MARCAR_FALLIDO    // se agotaron los reintentos: estado terminal diagnosticado
};

inline const char* nombreAccion(Accion a) {
    switch (a) {
        case Accion::NINGUNA:        return "NINGUNA";
        case Accion::REENCOLAR:      return "REENCOLAR";
        case Accion::FORZAR_MALLADO: return "FORZAR_MALLADO";
        case Accion::MARCAR_FALLIDO: return "MARCAR_FALLIDO";
        default:                     return "?";
    }
}

// Cuantas veces se reintenta antes de rendirse.
//
// No es "infinite retry loop" (que el diseño prohibe explicitamente) ni
// abandono silencioso: al agotarse se pasa a FALLIDO, que es un estado
// diagnosticado, visible en el overlay y contado en las metricas. Y FALLIDO
// puede volver a SOLICITADO si el jugador se aleja y regresa, asi que un fallo
// transitorio (disco ocupado, memoria justa) se acaba resolviendo solo.
constexpr int MAX_REINTENTOS_VIGILANTE = 3;

// ----------------------------------------------------------------------------
// LA DECISION
// ----------------------------------------------------------------------------
inline Accion decidir(Estado estado, float segundosEnEstado, int reintentos,
                      const Plazos& plazos = Plazos{}) {
    if (!esTransitorio(estado)) return Accion::NINGUNA;

    const float plazo = plazos.de(estado);
    if (plazo <= 0.0f) return Accion::NINGUNA;
    if (segundosEnEstado < plazo) return Accion::NINGUNA;

    // ⭐ ESPERA_VECINO TIENE SALIDA PROPIA, Y NO ES REENCOLAR.
    //
    // Reencolarlo seria volver a esperar al mismo vecino que no ha llegado en
    // 6 segundos -- un ciclo. La salida correcta es DEJAR DE ESPERAR: mallar
    // con el borde incompleto.
    //
    // El coste es una costura visible en esa frontera. El beneficio es que hay
    // terreno donde antes habia un agujero. Es la misma decision que ya tomaba
    // el mesher ("mejor visible que invisible") y la razon de que el motor no
    // se quede en blanco en el borde del mundo cargado. Ademas se corrige sola:
    // cuando el vecino aparezca, la invalidacion normal remalla los dos.
    if (estado == Estado::ESPERA_VECINO) {
        return (reintentos >= MAX_REINTENTOS_VIGILANTE) ? Accion::MARCAR_FALLIDO
                                                        : Accion::FORZAR_MALLADO;
    }

    if (reintentos >= MAX_REINTENTOS_VIGILANTE) return Accion::MARCAR_FALLIDO;
    return Accion::REENCOLAR;
}

// ============================================================================
// ⭐ EL VIGILANTE DE LOS WORKERS: NADIE VIGILABA AL VIGILADO
// ============================================================================
// RESPONSABILIDAD UNICA: dado cuanto lleva un hilo de mallado sin dar señales
// de vida, decir si esta colgado.
//
// ----------------------------------------------------------------------------
// EL HUECO QUE ESTO TAPA, Y COMO APARECIO
// ----------------------------------------------------------------------------
// `decidir()` (arriba) vigila CHUNKS, y funciona. Pero tiene una excepcion
// deliberada y necesaria en World::pasarVigilante:
//
//     if (mallaInFlight.count(c->position)) continue;   // lo tiene un worker
//
// Esa linea es correcta: soltarle el candado a un chunk que un worker esta
// leyendo autoriza a la descarga por distancia a devolverlo al pool y
// reciclarlo como otro chunk -- el use-after-recycle que el vigilante existe
// para evitar. No se puede quitar.
//
// Pero deja un agujero: si el WORKER es el que se cuelga, su chunk esta en
// mallaInFlight, asi que el vigilante de chunks lo salta para siempre. Nadie
// mira al worker. Y con los tres hilos colgados la cola de entrada se llena
// (tope 16) y el mundo deja de cargar entero.
//
// No es teorico: ocurrio. `cargarTunaSinHuecos` hacia I/O de disco y creaba
// texturas de OpenGL sin comprobar `puedeCargar()`, asi que un worker que
// topara con una tuna tocaba GL sin contexto y se quedaba muerto dentro del
// driver. Sintoma: 4 chunks dibujables de 47, y el juego sin cerrarse nunca.
// La causa esta arreglada; esto es la red para la SIGUIENTE de su clase.
//
// ----------------------------------------------------------------------------
// POR QUE EL PLAZO ES TAN LARGO
// ----------------------------------------------------------------------------
// Mallar un chunk son milisegundos; los peores medidos, decenas. Un worker que
// lleva SEIS SEGUNDOS sin avanzar una sola columna de 16 no va lento: esta
// parado. El plazo es enorme a proposito para que no pueda haber falsos
// positivos por carga -- y porque la accion que se toma no es gratis.
//
// ----------------------------------------------------------------------------
// ⚠️ LO QUE ESTE VIGILANTE *NO* HACE, Y ES LO MAS IMPORTANTE
// ----------------------------------------------------------------------------
// NO suelta el candado. NO mata el hilo. NO toca el chunk.
//
// Soltar el candado seria reintroducir exactamente el fallo que la excepcion
// de arriba evita, y encima con un hilo que sigue vivo leyendo ese chunk --
// peor que el problema original. Y matar un hilo bloqueado dentro de un driver
// de GL deja el driver en un estado indefinido.
//
// Lo que hace es DIAGNOSTICAR: dejarlo escrito en el log con el chunk y el
// bloque concretos, y contarlo en las metricas para que el overlay lo pinte.
// Un cuelgue de worker es un fallo de programacion (alguien toco GL o un mutex
// del hilo principal desde un worker), no una condicion transitoria que se
// pueda reintentar. Lo unico util que puede hacer el motor es decir DONDE, con
// el dato exacto, en vez de dejar al jugador mirando un mundo a medio dibujar
// sin ninguna pista.
// ============================================================================

// Plazo, en segundos, sin un solo latido antes de dar un worker por colgado.
// Un latido es una columna de 16x128 celdas, o un cambio de fase.
constexpr float SEGUNDOS_WORKER_COLGADO = 6.0f;

// ¿Este worker esta colgado?
//
// `activo` = esta dentro del mesher ahora mismo. Un worker inactivo esta
// esperando en la cola, que es su estado normal de reposo: no se vigila.
inline bool workerColgado(bool activo, float segundosSinLatir,
                          float plazo = SEGUNDOS_WORKER_COLGADO) {
    if (!activo) return false;
    return segundosSinLatir >= plazo;
}

// ----------------------------------------------------------------------------
// EL PRESUPUESTO ADAPTATIVO POR FRAME (§10, §11)
// ----------------------------------------------------------------------------
// El sistema anterior presupuestaba por CONTEO (1-2 chunks, 1-6 mallas). El
// problema del conteo es que la unidad no es constante: un chunk de cueva y
// otro de bosque con doscientos arboles no cuestan lo mismo ni de lejos, asi
// que "2 chunks" puede ser 1 ms o 40 ms. Presupuestar por conteo es
// presupuestar a ciegas.
//
// Aqui el presupuesto es TIEMPO, que es la magnitud que de verdad importa para
// el frame pacing, y ademas se reparte por fase para que una no se coma a las
// otras.
struct Presupuesto {
    float generacionMs = 2.0f;   // carga de disco + integracion
    float malladoMs    = 2.5f;   // encolado + trabajo sincrono de respaldo
    float subidaMs     = 2.0f;   // glBufferData: hilo principal por narices
    float descargaMs   = 1.0f;   // liberar y guardar

    float total() const { return generacionMs + malladoMs + subidaMs + descargaMs; }
};

// El objetivo del ajuste NO es cargar rapido: es no romper el frame.
//
// Se ajusta contra el tiempo de frame suavizado. La asimetria es deliberada:
// se recorta DEPRISA (multiplicando) y se recupera DESPACIO (sumando). Un
// tiron es inmediatamente visible para el jugador; recuperar medio milisegundo
// de presupuesto medio segundo mas tarde no lo nota nadie.
inline Presupuesto ajustarPresupuesto(const Presupuesto& actual,
                                      float msFrameSuavizado,
                                      float msObjetivo) {
    Presupuesto p = actual;

    // Suelo: por debajo de esto el streaming no progresa y el jugador se queda
    // sin mundo, que es peor que un frame irregular. Nunca se baja de aqui.
    //
    // ⭐ EL SUELO DE LA SUBIDA ES EL DOBLE QUE EL RESTO, Y NO ES CAPRICHO.
    //
    // Las tres fases no son intercambiables desde el punto de vista del
    // jugador. Generar y mallar producen datos que NO SE VEN; subir es la
    // unica que convierte trabajo ya hecho en pixeles.
    //
    // Con el suelo a 0.5 ms se midio la patologia: explorando, la cola de
    // mallas terminadas crecia sin parar (130 -> 815) mientras los tres
    // workers aparecian `libre` en el log. Habia 815 chunks mallados, en
    // memoria, invisibles -- y el regulador, al ver FPS bajos, recortaba justo
    // la fase que habria arreglado el problema. Un lazo de realimentacion:
    // menos subidas -> mas cola -> peor percepcion -> menos presupuesto.
    //
    // 1.0 ms de suelo son ~10 subidas por frame (glBufferData ronda 0.1 ms),
    // suficiente para drenar cualquier acumulacion en un par de segundos sin
    // que el frame lo note. Recortar aqui es lo ultimo que conviene hacer.
    constexpr float MIN_GEN = 0.5f, MIN_MALLA = 0.5f, MIN_SUBIDA = 1.0f, MIN_DESC = 0.25f;
    constexpr float MAX_GEN = 4.0f, MAX_MALLA = 5.0f, MAX_SUBIDA = 4.0f, MAX_DESC = 2.0f;

    const float holgura = msObjetivo - msFrameSuavizado;

    if (holgura < -1.0f) {
        // Vamos claramente por encima del objetivo: recortar de golpe.
        p.generacionMs *= 0.6f;
        p.malladoMs    *= 0.6f;
        p.subidaMs     *= 0.7f;   // menos agresivo: sin subida no hay nada visible
        p.descargaMs   *= 0.6f;
    } else if (holgura > 2.0f) {
        // Sobra tiempo de sobra: subir poco a poco.
        p.generacionMs += 0.25f;
        p.malladoMs    += 0.25f;
        p.subidaMs     += 0.20f;
        p.descargaMs   += 0.10f;
    }
    // Entre -1 y +2 ms: banda muerta. Sin ella el presupuesto oscilaria cada
    // frame y el ritmo de carga seria visiblemente irregular.

    if (p.generacionMs < MIN_GEN)    p.generacionMs = MIN_GEN;
    if (p.generacionMs > MAX_GEN)    p.generacionMs = MAX_GEN;
    if (p.malladoMs    < MIN_MALLA)  p.malladoMs    = MIN_MALLA;
    if (p.malladoMs    > MAX_MALLA)  p.malladoMs    = MAX_MALLA;
    if (p.subidaMs     < MIN_SUBIDA) p.subidaMs     = MIN_SUBIDA;
    if (p.subidaMs     > MAX_SUBIDA) p.subidaMs     = MAX_SUBIDA;
    if (p.descargaMs   < MIN_DESC)   p.descargaMs   = MIN_DESC;
    if (p.descargaMs   > MAX_DESC)   p.descargaMs   = MAX_DESC;

    return p;
}

} // namespace Streaming
