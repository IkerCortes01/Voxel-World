#include <doctest/doctest.h>
#include "render/EstadoChunk.h"
#include "render/PrioridadChunk.h"
#include "render/VigilanteChunk.h"

#include <vector>
#include <algorithm>

// ============================================================================
// TESTS DEL STREAMING DE CHUNKS
// ============================================================================
// Lo que se fija aqui es el COMPORTAMIENTO que el jugador nota:
//
//   - ningun chunk puede quedarse atascado (la Regla Absoluta)
//   - el suelo bajo los pies siempre gana
//   - el terreno se pide ANTES de llegar a el
//   - un chunk que espera mucho acaba subiendo
//   - moverse por una frontera no produce carga/descarga en bucle
//
// Todo sin OpenGL: son funciones puras sobre datos planos, asi que el test
// corre en milisegundos y no necesita ventana ni mundo.
// ============================================================================

using namespace Streaming;

// ----------------------------------------------------------------------------
TEST_CASE("Estados: el camino feliz completo es legal") {
    // El recorrido normal de un chunk, de no existir a dibujable.
    const Estado camino[] = {
        Estado::DESCARGADO, Estado::SOLICITADO, Estado::GENERANDO,
        Estado::GENERADO, Estado::MALLA_EN_COLA, Estado::MALLANDO,
        Estado::MALLA_LISTA, Estado::SUBIENDO, Estado::LISTO
    };
    for (size_t i = 0; i + 1 < sizeof(camino) / sizeof(camino[0]); ++i) {
        INFO("de ", nombreEstado(camino[i]), " a ", nombreEstado(camino[i + 1]));
        CHECK(transicionValida(camino[i], camino[i + 1]));
    }
}

TEST_CASE("Estados: solo LISTO se dibuja") {
    // Es la separacion que evita dibujar geometria a medias.
    for (int i = 0; i < (int)Estado::_COUNT; ++i) {
        const Estado e = (Estado)i;
        CHECK(esDibujable(e) == (e == Estado::LISTO));
    }
}

TEST_CASE("Estados: hay bloques antes de que haya geometria") {
    // La fisica y el guardado necesitan los bloques mucho antes de que el
    // chunk sea visible. Confundir las dos preguntas hace que el jugador
    // atraviese el suelo mientras carga.
    CHECK(tieneBloques(Estado::GENERADO));
    CHECK(tieneBloques(Estado::MALLANDO));
    CHECK(tieneBloques(Estado::ESPERA_VECINO));
    CHECK(tieneBloques(Estado::LISTO));

    CHECK_FALSE(tieneBloques(Estado::DESCARGADO));
    CHECK_FALSE(tieneBloques(Estado::SOLICITADO));
    CHECK_FALSE(tieneBloques(Estado::GENERANDO));  // a medio construir
    CHECK_FALSE(tieneBloques(Estado::FALLIDO));
}

TEST_CASE("Estados: desde cualquier sitio se puede salir") {
    // ⭐ LA REGLA ABSOLUTA, a nivel de la tabla de transiciones.
    //
    // Si existiera un estado sin salida, ninguna cantidad de watchdog lo
    // salvaria. Esto comprueba que la topologia no tiene callejones.
    for (int i = 0; i < (int)Estado::_COUNT; ++i) {
        const Estado e = (Estado)i;
        INFO("estado ", nombreEstado(e));
        // DESCARGADO siempre es alcanzable = el chunk se puede liberar.
        CHECK(transicionValida(e, Estado::DESCARGADO));
        // FALLIDO siempre es alcanzable = siempre se puede rendir con
        // diagnostico en vez de colgarse.
        CHECK(transicionValida(e, Estado::FALLIDO));
    }
    // Y FALLIDO no es terminal de verdad: se reintenta.
    CHECK(transicionValida(Estado::FALLIDO, Estado::SOLICITADO));
}

TEST_CASE("Estados: la invalidacion no repite la generacion") {
    // Romper un bloque tiene que volver a mallar, NO a generar terreno: el
    // terreno ya es correcto y regenerarlo borraria justo la modificacion del
    // jugador.
    CHECK(transicionValida(Estado::LISTO, Estado::GENERADO));
    CHECK(transicionValida(Estado::LISTO, Estado::MALLA_EN_COLA));
    // Pero no se puede retroceder a generar.
    CHECK_FALSE(transicionValida(Estado::LISTO, Estado::GENERANDO));
    CHECK_FALSE(transicionValida(Estado::GENERADO, Estado::SOLICITADO));
}

TEST_CASE("Estados: un chunk marcado para descarga puede rescatarse") {
    // El jugador dio media vuelta antes de que se liberara: sigue entero en
    // memoria, asi que volver es gratis. Es la mitad de la defensa contra el
    // ciclo carga/descarga en las fronteras (la otra mitad es la histeresis).
    CHECK(transicionValida(Estado::DESCARGA_EN_COLA, Estado::LISTO));
    CHECK(transicionValida(Estado::DESCARGA_EN_COLA, Estado::GENERADO));
}

// ----------------------------------------------------------------------------
TEST_CASE("Prioridad: el suelo bajo los pies gana a todo") {
    // ⭐ ESTE ES EL TEST QUE PROTEGE EL OBJETIVO PRINCIPAL.
    //
    // El sistema anterior fallaba aqui: con priority = d*(1-dot*0.5), un chunk
    // a distancia 4 DELANTE (2.0) ganaba a uno a distancia 1 DETRAS (1.5). O
    // sea que el jugador podia quedarse sin el suelo que estaba pisando por
    // cargar una vista bonita.
    ContextoJugador j;
    j.chunkX = 0; j.chunkZ = 0;
    j.velX = 20.0f; j.velZ = 0.0f;      // corriendo fuerte hacia +X
    j.miradaX = 1.0f; j.miradaZ = 0.0f;

    const EntradaPrioridad detras   = calcularPrioridad(-1, 0, j);  // R0, a la espalda
    const EntradaPrioridad delante4 = calcularPrioridad(4, 0, j);   // lejos, delante

    CHECK(detras.anillo == Anillo::R0_EMERGENCIA);
    CHECK(delante4.anillo != Anillo::R0_EMERGENCIA);
    // Menor = mas urgente. El de detras DEBE ir primero pese a ir a la espalda.
    CHECK(detras.valor < delante4.valor);
}

TEST_CASE("Prioridad: las diagonales entran en R0") {
    // Bug real que esto previene: con distancia euclidea (d2 <= 1) el chunk
    // diagonal quedaba fuera de la emergencia. Caminar en diagonal por una
    // esquina te dejaba mirando al vacio, porque ese chunk esta literalmente a
    // un paso de los pies del jugador.
    ContextoJugador j;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dz = -1; dz <= 1; ++dz) {
            INFO("vecino ", dx, ",", dz);
            CHECK(anilloDe(dx, dz, j) == Anillo::R0_EMERGENCIA);
        }
}

TEST_CASE("Prioridad: la velocidad SI cuenta") {
    // El sistema anterior normalizaba el vector de movimiento y tiraba la
    // magnitud, asi que volar a 50 m/s priorizaba igual que caminar. Al reves
    // de lo que hace falta.
    ContextoJugador lento;
    lento.velX = 2.0f;
    ContextoJugador rapido;
    rapido.velX = 40.0f;

    CHECK(lento.chunksAnticipacion() == 0);
    CHECK(rapido.chunksAnticipacion() > 0);
    CHECK(radioBarrido(rapido) > radioBarrido(lento));

    // Y no se dispara sin control aunque el jugador vuele absurdamente rapido.
    ContextoJugador absurdo;
    absurdo.velX = 5000.0f;
    CHECK(absurdo.chunksAnticipacion() <= 3);
}

TEST_CASE("Prioridad: se pide el terreno ANTES de llegar") {
    // Corriendo hacia +X, un chunk por delante fuera del nucleo tiene que
    // entrar en el corredor predicho y ganar al mismo chunk a la espalda.
    ContextoJugador j;
    j.velX = 20.0f; j.velZ = 0.0f;
    j.miradaX = 1.0f; j.miradaZ = 0.0f;

    const EntradaPrioridad delante = calcularPrioridad(4, 0, j);
    const EntradaPrioridad detras  = calcularPrioridad(-4, 0, j);

    CHECK(delante.anillo == Anillo::R3_PREDICHO);
    CHECK(detras.anillo  == Anillo::R4_PRECARGA);
    CHECK(delante.valor < detras.valor);
}

TEST_CASE("Prioridad: el que espera mucho acaba subiendo") {
    // Antienvejecimiento (§42): sin esto, un chunk lateral puede no cargarse
    // nunca mientras el jugador da vueltas.
    ContextoJugador j;
    const EntradaPrioridad recien   = calcularPrioridad(4, 4, j, 0.0f);
    const EntradaPrioridad esperando = calcularPrioridad(4, 4, j, 20.0f);
    CHECK(esperando.valor < recien.valor);
}

TEST_CASE("Prioridad: el antienvejecimiento NO adelanta a la emergencia") {
    // El contrapeso del test anterior. Si el envejecimiento fuera rapido, un
    // chunk viejo y lejano podria adelantar al suelo bajo los pies -- que es
    // exactamente el fallo que el sistema de anillos existe para impedir.
    ContextoJugador j;
    const EntradaPrioridad viejoLejano = calcularPrioridad(10, 10, j, 20.0f);
    const EntradaPrioridad nuevoR0     = calcularPrioridad(1, 0, j, 0.0f);
    CHECK(nuevoR0.valor < viejoLejano.valor);
}

TEST_CASE("Prioridad: la histeresis abre una banda de verdad") {
    // Cargar a 4 y descargar a 5 (el "+1" del sistema anterior) no es
    // histeresis: es un buffer fijo, y oscilar sobre la frontera produce
    // carga/descarga repetida. Con margen 2 hay banda real.
    ContextoJugador j;
    j.distanciaRender = 4;
    CHECK(radioCarga(j) == 4);
    CHECK(radioDescarga(j) >= radioCarga(j) + 2);
}

TEST_CASE("Prioridad: el orden global respeta los anillos") {
    // Se mezclan chunks de todo el radio, se ordenan como haria el
    // planificador, y se comprueba que ningun anillo peor se cuela por delante
    // de uno mejor. Es la propiedad de "barrera dura" del diseño.
    ContextoJugador j;
    j.velX = 15.0f;
    j.miradaX = 1.0f; j.miradaZ = 0.0f;

    std::vector<EntradaPrioridad> v;
    for (int x = -6; x <= 6; ++x)
        for (int z = -6; z <= 6; ++z)
            v.push_back(calcularPrioridad(x, z, j));

    std::sort(v.begin(), v.end());

    for (size_t i = 1; i < v.size(); ++i) {
        INFO("posicion ", i, ": ", nombreAnillo(v[i - 1].anillo),
             " seguido de ", nombreAnillo(v[i].anillo));
        CHECK((int)v[i - 1].anillo <= (int)v[i].anillo);
    }
}

// ----------------------------------------------------------------------------
TEST_CASE("Vigilante: dentro de plazo no toca nada") {
    CHECK(decidir(Estado::GENERANDO, 0.5f, 0) == Accion::NINGUNA);
    CHECK(decidir(Estado::MALLANDO,  1.0f, 0) == Accion::NINGUNA);
}

TEST_CASE("Vigilante: los estados estables no se vigilan") {
    // LISTO no tiene que avanzar a ningun sitio: vigilarlo daria falsos
    // positivos permanentes.
    CHECK(decidir(Estado::LISTO,      9999.0f, 0) == Accion::NINGUNA);
    CHECK(decidir(Estado::DESCARGADO, 9999.0f, 0) == Accion::NINGUNA);
    CHECK(decidir(Estado::FALLIDO,    9999.0f, 0) == Accion::NINGUNA);
}

TEST_CASE("Vigilante: TODOS los estados transitorios tienen rescate") {
    // ⭐ LA REGLA ABSOLUTA (§65), verificada estado por estado.
    //
    // El vigilante anterior solo miraba el candado de mallado. Un chunk
    // perdido en la cola de generacion, o esperando un vecino que no llegaba,
    // no lo rescataba nadie.
    for (int i = 0; i < (int)Estado::_COUNT; ++i) {
        const Estado e = (Estado)i;
        if (!esTransitorio(e)) continue;
        INFO("estado transitorio ", nombreEstado(e));
        // Con tiempo de sobra, el vigilante SIEMPRE actua.
        CHECK(decidir(e, 100.0f, 0) != Accion::NINGUNA);
    }
}

TEST_CASE("Vigilante: esperar un vecino no es un estado de reposo") {
    // La trampa historica: parece legitimo ("estoy esperando, es normal"), y
    // por eso el codigo viejo lo excluia del rescate. Pero si el vecino no
    // llega nunca, el chunk se queda ahi para siempre.
    CHECK(esTransitorio(Estado::ESPERA_VECINO));
    // Y su salida NO es reencolar (seria volver a esperar lo mismo): es dejar
    // de esperar y mallar con lo que hay.
    CHECK(decidir(Estado::ESPERA_VECINO, 100.0f, 0) == Accion::FORZAR_MALLADO);
}

TEST_CASE("Vigilante: se rinde con diagnostico, no en silencio ni en bucle") {
    // Ni "infinite retry loop" (prohibido por el diseño) ni abandono callado.
    CHECK(decidir(Estado::GENERANDO, 100.0f, 0) == Accion::REENCOLAR);
    CHECK(decidir(Estado::GENERANDO, 100.0f, MAX_REINTENTOS_VIGILANTE)
          == Accion::MARCAR_FALLIDO);
    CHECK(decidir(Estado::ESPERA_VECINO, 100.0f, MAX_REINTENTOS_VIGILANTE)
          == Accion::MARCAR_FALLIDO);
}

// ----------------------------------------------------------------------------
TEST_CASE("Presupuesto: se recorta rapido y se recupera despacio") {
    // Un tiron se ve al instante; recuperar presupuesto medio segundo mas
    // tarde no lo nota nadie. Por eso la asimetria.
    Presupuesto base;
    const float objetivo = 8.3f;   // 120 FPS

    const Presupuesto apretado = ajustarPresupuesto(base, 20.0f, objetivo);
    CHECK(apretado.malladoMs < base.malladoMs);

    const Presupuesto holgado = ajustarPresupuesto(base, 3.0f, objetivo);
    CHECK(holgado.malladoMs > base.malladoMs);

    // La bajada tiene que ser mas brusca que la subida.
    const float caida  = base.malladoMs - apretado.malladoMs;
    const float subida = holgado.malladoMs - base.malladoMs;
    CHECK(caida > subida);
}

TEST_CASE("Presupuesto: nunca llega a cero") {
    // Si el presupuesto pudiera anularse, el streaming se pararia del todo y
    // el jugador se quedaria sin mundo -- peor que un frame irregular. Se
    // simula un frame catastrofico sostenido.
    Presupuesto p;
    for (int i = 0; i < 200; ++i) p = ajustarPresupuesto(p, 500.0f, 8.3f);

    CHECK(p.generacionMs > 0.0f);
    CHECK(p.malladoMs    > 0.0f);
    CHECK(p.subidaMs     > 0.0f);
    CHECK(p.descargaMs   > 0.0f);
}

TEST_CASE("Presupuesto: acotado por arriba aunque sobre tiempo") {
    // Sin tope, con el jugador quieto el presupuesto crecerian sin limite y el
    // primer chunk pesado que llegara produciria un tiron enorme.
    Presupuesto p;
    for (int i = 0; i < 200; ++i) p = ajustarPresupuesto(p, 0.5f, 8.3f);

    CHECK(p.total() < 16.0f);   // cabe holgadamente en un frame de 60 FPS
}

TEST_CASE("Presupuesto: banda muerta, no oscila") {
    // Dentro de la banda el presupuesto no se mueve. Sin ella cambiaria cada
    // frame y el ritmo de carga seria visiblemente irregular.
    Presupuesto base;
    const Presupuesto igual = ajustarPresupuesto(base, 8.0f, 8.3f);
    CHECK(igual.malladoMs == doctest::Approx(base.malladoMs));
    CHECK(igual.generacionMs == doctest::Approx(base.generacionMs));
}

// ============================================================================
// EL VIGILANTE DE LOS WORKERS (Streaming::workerColgado)
// ============================================================================
// Fija el contrato del hueco que este vigilante tapa. El fallo real que lo
// motivo: `cargarTunaSinHuecos` hacia I/O y creaba texturas de OpenGL sin
// comprobar `puedeCargar()`, asi que un hilo de mallado que topara con una
// tuna tocaba GL sin contexto y se quedaba muerto dentro del driver. Con los
// tres hilos muertos la cola de entrada se llenaba y el mundo dejaba de
// cargar: 4 chunks dibujables de 47.
//
// El vigilante de CHUNKS no podia verlo, y no por descuido: salta a proposito
// todo chunk que un worker tenga en las manos, porque soltarle el candado
// autorizaria a reciclar memoria viva. De ahi que hiciera falta uno que mire
// al worker.

TEST_CASE("Worker: uno inactivo NUNCA cuenta como colgado") {
    // Un worker esperando en la cola es su estado normal de reposo. Si contara
    // como colgado, el log se llenaria de avisos con el juego perfectamente.
    CHECK(workerColgado(/*activo=*/false, 0.0f)      == false);
    CHECK(workerColgado(/*activo=*/false, 999.0f)    == false);
}

TEST_CASE("Worker: trabajando dentro de plazo no se toca") {
    // Mallar un chunk son milisegundos; los peores medidos, decenas. Nada de
    // esto se parece a estar parado.
    CHECK(workerColgado(true, 0.0f)   == false);
    CHECK(workerColgado(true, 0.05f)  == false);
    CHECK(workerColgado(true, 1.0f)   == false);
    CHECK(workerColgado(true, 5.9f)   == false);
}

TEST_CASE("Worker: pasado el plazo, colgado") {
    CHECK(workerColgado(true, SEGUNDOS_WORKER_COLGADO)         == true);
    CHECK(workerColgado(true, SEGUNDOS_WORKER_COLGADO + 10.0f) == true);
}

TEST_CASE("Worker: el plazo es holgado, no ajustado") {
    // A proposito: el vigilante NO rescata, solo diagnostica, asi que un falso
    // positivo es ruido en el log que manda a buscar un bug que no existe. El
    // plazo tiene que ser tan largo que ninguna carga legitima lo alcance.
    //
    // Referencia: el peor mallado medido ronda decenas de ms. Seis segundos son
    // dos ordenes de magnitud por encima.
    CHECK(SEGUNDOS_WORKER_COLGADO >= 5.0f);
    CHECK(workerColgado(true, 0.5f) == false);   // un frame malisimo: no es
}

TEST_CASE("Worker: el plazo se puede inyectar, para poder probarlo") {
    // Misma razon que en `decidir`: el tiempo entra por parametro, asi que un
    // test puede comprobar un plazo de 6 segundos sin esperar 6 segundos.
    CHECK(workerColgado(true, 0.2f, /*plazo=*/0.1f) == true);
    CHECK(workerColgado(true, 0.05f, /*plazo=*/0.1f) == false);
}
