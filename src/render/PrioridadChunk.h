#pragma once

#include <cmath>
#include <cstdint>

// ============================================================================
// QUE CHUNK VA PRIMERO
// ============================================================================
// RESPONSABILIDAD UNICA: convertir (donde esta el jugador, hacia donde va,
// hacia donde mira, cuanto lleva esperando un chunk) en UN numero comparable.
//
// Sin OpenGL, sin World, sin estado global: funciones puras sobre datos
// planos. Se puede probar sin arrancar el juego, y de hecho hay un test que
// fija las relaciones de orden que importan (delante gana a detras, lo de
// debajo gana a todo, etc).
//
// ----------------------------------------------------------------------------
// LO QUE HACIA EL SISTEMA ANTERIOR, Y POR QUE NO BASTABA
// ----------------------------------------------------------------------------
// La formula era:
//
//     priority = distance * (1.0f - dotProduct * 0.5f)
//
// Tres problemas medidos:
//
//   1. EL SESGO ERA DEMASIADO DEBIL. Delante multiplica por 0.5, detras por
//      1.5. Un chunk a distancia 4 delante (prioridad 2.0) PIERDE contra uno a
//      distancia 1 detras (prioridad 1.5). O sea que nunca adelanta mas de un
//      anillo: en la practica seguia siendo carga por distancia.
//
//   2. LA VELOCIDAD SE TIRABA. moveDir se normalizaba, asi que volar a 50 m/s
//      y caminar a 4 m/s producian exactamente la misma prioridad. Justo al
//      reves de lo que hace falta: cuanto mas rapido va el jugador, mas lejos
//      hay que mirar, porque llegara antes.
//
//   3. NO HABIA ENVEJECIMIENTO. Un chunk lateral podia quedarse esperando
//      indefinidamente mientras el jugador daba vueltas, porque su prioridad
//      no mejoraba nunca.
//
// ----------------------------------------------------------------------------
// EL MODELO NUEVO: ANILLOS DUROS + PUNTUACION DENTRO DEL ANILLO
// ----------------------------------------------------------------------------
// La leccion del problema 1 es que un multiplicador continuo no separa
// urgencias distintas: siempre existe una distancia que lo compensa.
//
// Por eso el anillo es una BARRERA, no un factor. Un chunk de R0 gana a
// CUALQUIER chunk de R1, pase lo que pase con el resto de terminos. Asi
// "el suelo bajo mis pies" nunca pierde contra "una vista bonita a lo lejos",
// que es la unica forma de garantizar el objetivo (que el jugador no caiga en
// un agujero) en vez de dejarlo al azar de una suma de pesos.
//
// Dentro del anillo si se usa una puntuacion continua, porque ahi todas las
// opciones son igual de legitimas y solo queremos la mejor.
// ============================================================================

namespace Streaming {

// ----------------------------------------------------------------------------
// LOS ANILLOS
// ----------------------------------------------------------------------------
enum class Anillo : uint8_t {
    R0_EMERGENCIA = 0,  // el chunk del jugador y sus 8 vecinos: sin esto se cae
    R1_INMEDIATO,       // lo que se toca al andar un segundo
    R2_CERCANO,         // claramente visible
    R3_PREDICHO,        // hacia donde va o mira
    R4_PRECARGA,        // el resto del radio
    _COUNT
};

inline const char* nombreAnillo(Anillo a) {
    switch (a) {
        case Anillo::R0_EMERGENCIA: return "R0";
        case Anillo::R1_INMEDIATO:  return "R1";
        case Anillo::R2_CERCANO:    return "R2";
        case Anillo::R3_PREDICHO:   return "R3";
        case Anillo::R4_PRECARGA:   return "R4";
        default:                    return "R?";
    }
}

// ----------------------------------------------------------------------------
// LO QUE HACE FALTA SABER DEL JUGADOR
// ----------------------------------------------------------------------------
// Deliberadamente en coordenadas de CHUNK (cx, cz) mas un residuo, y con la
// velocidad en bloques/segundo SIN normalizar -- que es el dato que el sistema
// anterior tiraba.
struct ContextoJugador {
    int   chunkX = 0, chunkZ = 0;      // chunk que ocupa el jugador
    float velX = 0.0f, velZ = 0.0f;    // bloques/segundo, MAGNITUD CONSERVADA
    float miradaX = 0.0f, miradaZ = 1.0f;  // direccion de camara, normalizada
    int   distanciaRender = 4;

    float rapidez() const { return std::sqrt(velX * velX + velZ * velZ); }

    // ⭐ CUANTOS CHUNKS POR DELANTE MIRAR
    //
    // La idea: si el jugador va a 20 bloques/s y un chunk tarda ~0.5 s en
    // estar listo, para cuando llegue habra recorrido 10 bloques. Hay que
    // empezar a cargarlo cuando aun esta a esa distancia, no cuando el jugador
    // ya lo pisa.
    //
    // SEGUNDOS_ANTICIPACION es el presupuesto de latencia del pipeline
    // completo (generar + mallar + subir). 0.75 s es conservador para la
    // maquina mas lenta del perfil; en una rapida sobra, y sobrar no hace daño
    // (solo precarga un poco antes).
    //
    // Se acota a 3 chunks para que un jugador en modo creativo volando muy
    // rapido no pida un corredor infinito y se coma el presupuesto entero.
    int chunksAnticipacion() const {
        // ⭐ 1.5 s, NO 0.75.
        //
        // El 0.75 era "el presupuesto de latencia del pipeline completo
        // (generar + mallar + subir)". Pero medido en la maquina de referencia,
        // un chunk tarda bastante mas que eso en recorrer el pipeline cuando
        // hay cola: pasa por la cola de generacion (tope 24), dos workers, la
        // cola de mallado (tope 16), tres workers mas, y la cola de subida.
        //
        // Con 0.75 s y vuelo rapido (21 bloques/s) la anticipacion sale a 0.98
        // chunks -- o sea UNO. El jugador cruza un chunk cada 0.76 s, asi que
        // pedia el terreno justo cuando ya casi lo estaba pisando: llegaba
        // tarde y el mundo aparecia de golpe delante. Ese es el sintoma de
        // "carga brusca al volar".
        //
        // Con 1.5 s se pide con dos chunks de margen a velocidad de vuelo, que
        // es el tiempo que el pipeline necesita de verdad. Pasarse tampoco es
        // gratis (se precarga terreno que quiza no se pise), por eso no se sube
        // mas: es el punto donde el corredor deja de llegar tarde sin empezar a
        // desperdiciar.
        constexpr float SEGUNDOS_ANTICIPACION = 1.5f;
        constexpr float BLOQUES_POR_CHUNK     = 16.0f;
        const float bloques = rapidez() * SEGUNDOS_ANTICIPACION;
        int n = (int)(bloques / BLOQUES_POR_CHUNK);
        if (n < 0) n = 0;
        // ⭐ Tope 5, no 3. A 21 bloques/s (vuelo rapido) la formula pide 1.97
        // chunks; el tope de 3 no llegaba a estorbar ahi, pero SI recortaba en
        // cuanto se volaba mas rapido o el pipeline iba cargado -- que es justo
        // cuando mas falta hace mirar lejos. 5 chunks a 16 bloques son 80
        // bloques de corredor, que sigue siendo una fraccion del radio cargado.
        if (n > 5) n = 5;
        return n;
    }

    // ¿Va lo bastante rapido como para cambiar de modo? (§54)
    //
    // 8 bloques/s esta justo por encima de correr (que en este motor ronda
    // 5.6) y por debajo de volar. Es el punto donde la carga por distancia
    // deja de alcanzar al jugador.
    bool movimientoRapido() const { return rapidez() > 8.0f; }
};

// ----------------------------------------------------------------------------
// EN QUE ANILLO CAE UN CHUNK
// ----------------------------------------------------------------------------
inline Anillo anilloDe(int cx, int cz, const ContextoJugador& j) {
    const int dx = cx - j.chunkX;
    const int dz = cz - j.chunkZ;
    const int d2 = dx * dx + dz * dz;

    // R0: el propio chunk y los 8 que lo rodean (distancia Chebyshev 1).
    //
    // Se usa Chebyshev y no euclidea a proposito: las diagonales tienen que
    // entrar. Un jugador en la esquina de su chunk tiene el chunk diagonal a
    // menos de un bloque de sus pies, y con distancia euclidea (d2 <= 1) se
    // quedaba fuera. Ese era un agujero real: caminar en diagonal por una
    // frontera te dejaba mirando al vacio.
    const int chebyshev = (dx < 0 ? -dx : dx) > (dz < 0 ? -dz : dz)
                        ? (dx < 0 ? -dx : dx) : (dz < 0 ? -dz : dz);
    if (chebyshev <= 1) return Anillo::R0_EMERGENCIA;

    if (d2 <= 4)  return Anillo::R1_INMEDIATO;   // radio 2
    if (d2 <= 9)  return Anillo::R2_CERCANO;     // radio 3

    // R3: fuera del nucleo, pero en la direccion en la que el jugador se mueve
    // o mira. Es lo que evita que el terreno "aparezca de golpe" al avanzar.
    //
    // El corredor de movimiento solo cuenta si de verdad se esta moviendo:
    // con el jugador quieto, rapidez() ~ 0 y el producto escalar no significa
    // nada.
    const float dist = std::sqrt((float)d2);
    if (dist > 0.001f) {
        const float nx = dx / dist;
        const float nz = dz / dist;

        if (j.rapidez() > 0.5f) {
            const float r = j.rapidez();
            const float dotMov = (nx * j.velX + nz * j.velZ) / r;
            // cos(40 grados) ~ 0.766: un cono estrecho por delante.
            if (dotMov > 0.766f && dist <= (float)(3 + j.chunksAnticipacion()))
                return Anillo::R3_PREDICHO;
        }

        // Cono de camara, mas ancho (cos 50 ~ 0.643) porque el campo de vision
        // es ancho y girar la cabeza es instantaneo.
        const float dotMirada = nx * j.miradaX + nz * j.miradaZ;
        if (dotMirada > 0.643f) return Anillo::R3_PREDICHO;
    }

    return Anillo::R4_PRECARGA;
}

// ----------------------------------------------------------------------------
// LA PRIORIDAD FINAL
// ----------------------------------------------------------------------------
// MENOR = MAS URGENTE (misma convencion que tenia el motor, para que el
// std::sort existente siga leyendose igual).
//
// Estructura: anillo * SEPARACION_ANILLO + puntuacion dentro del anillo.
//
// SEPARACION_ANILLO (1000) es mayor que cualquier puntuacion interna posible,
// que es lo que convierte el anillo en una barrera dura en vez de en un peso
// mas. Un chunk de R0 con la peor puntuacion interna imaginable sigue ganando
// al mejor de R1.
struct EntradaPrioridad {
    int    cx = 0, cz = 0;
    Anillo anillo = Anillo::R4_PRECARGA;
    float  valor  = 0.0f;      // menor = antes
    float  distancia = 0.0f;   // en chunks, para diagnostico

    bool operator<(const EntradaPrioridad& o) const { return valor < o.valor; }
};

// `segundosEsperando` implementa el antienvejecimiento (§42): un chunk que
// lleva mucho tiempo en la cola sube solo, para que los laterales no se mueran
// de hambre mientras el jugador da vueltas.
//
// El factor es 40 por segundo, o sea que hacen falta 25 s de espera para ganar
// un anillo entero (1000). Es deliberadamente lento: el antienvejecimiento
// existe para romper el bloqueo indefinido, NO para competir con la urgencia
// real. Si fuera rapido, un chunk viejo a la espalda podria adelantar al suelo
// bajo los pies del jugador, que es exactamente lo que no queremos.
inline EntradaPrioridad calcularPrioridad(int cx, int cz,
                                          const ContextoJugador& j,
                                          float segundosEsperando = 0.0f) {
    constexpr float SEPARACION_ANILLO = 1000.0f;
    constexpr float PUNTOS_POR_SEGUNDO_ESPERA = 40.0f;

    EntradaPrioridad e;
    e.cx = cx;
    e.cz = cz;

    const int dx = cx - j.chunkX;
    const int dz = cz - j.chunkZ;
    e.distancia = std::sqrt((float)(dx * dx + dz * dz));
    e.anillo    = anilloDe(cx, cz, j);

    // Dentro del anillo: la distancia manda, con un descuento por alineacion
    // con el movimiento. Aqui el multiplicador continuo SI vale, porque solo
    // ordena entre iguales y no puede saltarse una barrera.
    float dentro = e.distancia * 10.0f;

    if (e.distancia > 0.001f && j.rapidez() > 0.5f) {
        const float nx = dx / e.distancia;
        const float nz = dz / e.distancia;
        const float dotMov = (nx * j.velX + nz * j.velZ) / j.rapidez();
        if (dotMov > 0.0f) dentro *= (1.0f - dotMov * 0.4f);
    }

    e.valor = (float)e.anillo * SEPARACION_ANILLO
            + dentro
            - segundosEsperando * PUNTOS_POR_SEGUNDO_ESPERA;

    return e;
}

// ----------------------------------------------------------------------------
// RADIOS DE CARGA Y DESCARGA: LA HISTERESIS (§34)
// ----------------------------------------------------------------------------
// El sistema anterior cargaba a d <= RENDER_DISTANCE y descargaba a
// d > RENDER_DISTANCE + 1. Ese "+1" es un buffer fijo, no una histeresis: un
// jugador oscilando sobre una frontera de chunk cruza los dos umbrales a la
// vez y produce carga/descarga repetida.
//
// Con margen 2 hay una banda de verdad: para que un chunk recien cargado se
// descargue, el jugador tiene que alejarse DOS chunks completos (32 bloques),
// no volver sobre sus pasos medio metro.
//
// El coste es memoria: el area crece con el cuadrado del radio. Con
// RENDER_DISTANCE=4 son 81 chunks cargados frente a 113 que se mantienen. A
// ~200 KB por chunk son ~6 MB extra, que es barato comparado con el tiron de
// regenerar un chunk que se acaba de tirar.
inline int radioCarga(const ContextoJugador& j) {
    return j.distanciaRender;
}

inline int radioDescarga(const ContextoJugador& j) {
    constexpr int MARGEN_HISTERESIS = 2;
    return j.distanciaRender + MARGEN_HISTERESIS;
}

// El radio que hay que BARRER para decidir que cargar. Incluye el corredor de
// anticipacion, porque si no el corredor no se llegaria a pedir nunca: un
// chunk que esta fuera del radio de carga no entra en la lista de candidatos
// por muy prioritario que fuese su anillo.
inline int radioBarrido(const ContextoJugador& j) {
    return radioCarga(j) + j.chunksAnticipacion();
}

} // namespace Streaming
