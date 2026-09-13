#ifndef PECARI_ENTIDAD_H
#define PECARI_ENTIDAD_H

#include <cstdint>
#include <cmath>
#include <vector>
#include "PecariModelo3D.h"

// ============================================================================
// EL PECARI COMO ENTIDAD VIVA
// ============================================================================
// RESPONSABILIDAD UNICA: el estado de un pecari concreto y como cambia con el
// tiempo. NO decide donde nacen (eso es PecariSpawn), NO dibuja (eso es el
// motor, con la geometria que este archivo le da).
//
// ----------------------------------------------------------------------------
// LO QUE ESTE ARCHIVO ES Y LO QUE NO ES
// ----------------------------------------------------------------------------
// NO es el sistema de IA de 01_ARQUITECTURA. Ese sistema tiene siete
// necesidades continuas, cinco hormonas, utility AI con rank y weight, y
// memoria espacial. Nada de eso esta aqui.
//
// Esto es la CAPA MINIMA que hace falta para que un pecari exista en el mundo
// y se mueva de forma creible: posicion, orientacion, un modo de andar, y
// cohesion de manada. Es la fase 1 de la ruta incremental que el propio
// archivo de arquitectura prescribe, y se para justo antes de las necesidades.
//
// Se ha construido de forma que el utility AI ENCAJE ENCIMA sin reescribir
// nada: el campo `objetivo` es el unico punto por donde una IA futura tendria
// que empujar. Hoy lo escribe el deambular; manana lo escribiria el
// comportamiento que gane la utilidad.
//
// ----------------------------------------------------------------------------
// COHESION DE MANADA: POR QUE VECINDAD TOPOLOGICA
// ----------------------------------------------------------------------------
// 01_ARQUITECTURA lo fija, y no por gusto: MEDIDO (Ballerini et al. 2008,
// PNAS) que los animales interactuan con sus ~6-7 vecinos mas proximos
// INDEPENDIENTEMENTE de la distancia, no con "todos los que esten a menos de
// X metros".
//
// La diferencia importa: con radio metrico fijo, una manada dispersa se
// desintegra en cuanto se separa; con k-vecinos, no. Y hay un bono: limitar a
// ~7 vecinos es TAMBIEN la optimizacion que hace la cohesion barata. La
// biologia y el rendimiento coinciden, que es lo que el archivo de
// arquitectura llama "la optimizacion y la biologia coinciden".
//
// Aqui las manadas son pequenas (5-15), asi que casi siempre TODOS los
// miembros son vecinos. La estructura queda preparada igualmente porque el
// limite es lo que evita que una manada grande cueste O(n^2).
//
// ----------------------------------------------------------------------------
// TRAZABILIDAD
// ----------------------------------------------------------------------------
// Misma escala del AI simulator. Las velocidades y ritmos de este archivo son
// mayoritariamente ESTIMADO: son ajuste de juego. Lo que SI sale de dato
// medido va marcado y citado.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// ETAPA VITAL
// ----------------------------------------------------------------------------
// Las cinco de 01_ARQUITECTURA. Hoy solo afectan a la ESCALA visual; el
// crecimiento de Gompertz y la demografia son fase 3 y no estan aqui.
enum class EtapaPecari : uint8_t {
    NEONATO = 0,
    JUVENIL,
    SUBADULTO,
    ADULTO,
    SENESCENTE
};

// ----------------------------------------------------------------------------
// MODO DE LOCOMOCION
// ----------------------------------------------------------------------------
// Lo que el animal esta haciendo con las patas. Es lo minimo para que la
// animacion tenga sentido y para que la velocidad no sea una constante.
enum class ModoPecari : uint8_t {
    QUIETO = 0,     // parado, quizas hozando
    ANDANDO,        // desplazamiento normal de forrajeo
    TROTANDO        // desplazamiento rapido: alcanzar a la manada
};

// ----------------------------------------------------------------------------
// QUE ESTA HACIENDO EL ANIMAL
// ----------------------------------------------------------------------------
// Distinto de ModoPecari, que es solo LOCOMOCION (que hacen las patas). Esto
// es la INTENCION, y es lo que decide a donde va.
//
// Son cuatro y no catorce a proposito. El catalogo completo de
// 11_PECARI_IA.json tiene quince comportamientos con utility AI, rank y peso;
// esto es solo la rama de AMENAZA, que es lo que se pidio. Los otros once
// (forrajear, beber, termorregular, cortejar...) necesitan el sistema de
// necesidades y hormonas, que no existe todavia.
enum class ConductaPecari : uint8_t {
    CALMA = 0,      // lo de siempre: deambular con la manada
    ALERTA,         // ha notado algo; se para, mira y eriza la cresta
    HUIDA,          // se aleja del peligro
    DEFENSA         // encara al agresor. Ver la nota de ATACA_SI en el .cpp
};

// ----------------------------------------------------------------------------
// DATOS DE MOVIMIENTO
// ----------------------------------------------------------------------------
namespace PecariMovimiento {

    // --- Velocidades, en bloques/segundo ---
    //
    // ANCLAJE MEDIDO: la distancia diaria recorrida es 2.2 km (Taber et al.
    // 1993), sobre un area diaria de 18.2 ha.
    //
    // 2200 m / 0.60 m por bloque = 3667 bloques al dia.
    // Si el animal esta activo ~10 h y en movimiento efectivo ~35% de ese
    // tiempo (INFERIDO del presupuesto de T. pecari: 33% desplazamiento),
    // son 3667 / (10*3600*0.35) = 0.29 bloques/s de media.
    //
    // Esa es la media INCLUYENDO paradas para comer. La velocidad instantanea
    // de marcha es mayor. Se toma 1.1 bloques/s (0.66 m/s), que es un paso
    // tranquilo de ungulado pequeno y produce esa media al alternar con
    // paradas.
    // confianza: DERIVADO (de la distancia diaria MEDIDA)
    constexpr float VEL_ANDANDO = 1.1f;

    // El trote para reincorporarse al grupo. No hay medicion publicada de la
    // velocidad de carrera de esta especie.
    // confianza: ESTIMADO
    constexpr float VEL_TROTANDO = 3.2f;

    // --- Giro ---
    // Radianes por segundo. Un animal de patas cortas gira rapido.
    // confianza: ESTIMADO
    constexpr float VEL_GIRO = 3.0f;

    // --- ACELERACION ---
    //
    // ⭐ ESTO FALTABA, Y ERA LA CAUSA DE QUE EL MOVIMIENTO NO SE VIERA REAL.
    //
    // La velocidad se asignaba de golpe: p.vx = dirX * vel. O sea que un
    // animal pasaba de quieto a 6 bloques/s EN UN FRAME, y de 6 a 0 igual de
    // seco. No hay arranque ni frenada, solo dos estados, y el ojo lo lee como
    // teletransporte por mucho que las patas se muevan.
    //
    // Un cuerpo de 18.7 kg no cambia de velocidad instantaneamente: tiene
    // inercia. Con estos valores, arrancar a trote (3.2) cuesta ~0.36 s y
    // lanzarse a huir (6.0) ~0.67 s, que es el orden de lo que tarda un
    // ungulado pequeno en ponerse en marcha.
    //
    // La frenada es mas rapida que el arranque: parar cuesta menos que
    // acelerar, porque se puede clavar las pezunas.
    // confianza: ESTIMADO
    constexpr float ACELERACION = 9.0f;    // bloques/s^2
    constexpr float FRENADA     = 14.0f;   // bloques/s^2

    // --- Cohesion de manada ---
    //
    // MEDIDO (Byers y Bekoff 1981): "la unidad social es una manada cohesiva
    // en la que se mantienen distancias interindividuales PEQUENAS".
    //
    // Distancia a la que un miembro se siente comodo respecto al centro del
    // grupo. Mas alla, tiende a volver.
    // confianza: ESTIMADO (el valor) / MEDIDO (que debe ser pequeno)
    constexpr float RADIO_COMODO_BLOQUES = 7.0f;

    // A partir de aqui trota para reincorporarse: se ha quedado atras.
    // confianza: ESTIMADO
    constexpr float RADIO_ALARMA_BLOQUES = 14.0f;

    // Separacion minima entre dos pecaries: no se atraviesan.
    // El tronco mide 0.25 m de ancho (Externo::TRONCO.ancho()), asi que dos
    // animales lado a lado ocupan ~0.5 m = 0.83 bloques. Se redondea a 0.9.
    // confianza: DERIVADO (de la anatomia)
    constexpr float SEPARACION_MINIMA = 0.9f;

    // --- Vecindad topologica (Ballerini et al. 2008, PNAS) ---
    // MEDIDO: ~6-7 vecinos, independientemente de la distancia.
    constexpr int K_VECINOS = 7;

    // --- Ritmo de deambular ---
    // Cada cuanto el animal elige un destino nuevo, en segundos.
    // Las interacciones sociales medidas son "breves" (Byers y Bekoff), asi
    // que el animal cambia de idea a menudo.
    // confianza: ESTIMADO
    constexpr float TIEMPO_DECISION_MIN = 3.0f;
    constexpr float TIEMPO_DECISION_MAX = 9.0f;

    // Cuanto se aleja del centro de la manada al elegir destino.
    // confianza: ESTIMADO
    constexpr float RADIO_DEAMBULAR = 6.0f;
}

// ----------------------------------------------------------------------------
// UN PECARI
// ----------------------------------------------------------------------------
// Deliberadamente PLANO y sin punteros: es un POD que cabe en un array
// contiguo. 01_ARQUITECTURA senala que la actualizacion de estado afecta a
// todos los agentes de todos los niveles de LOD y que ahi es donde conviene
// invertir en layout de datos.
struct PecariAgente {
    // --- Identidad ---
    int   id = 0;
    int   idManada = -1;       // -1 = solitario (disperso)

    // --- Posicion y orientacion, en bloques ---
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float vx = 0.0f, vz = 0.0f;       // velocidad horizontal
    float vy = 0.0f;                  // velocidad vertical (caida)
    float orientacion = 0.0f;         // radianes; 0 = mirando hacia +Z
    bool  enSuelo = true;

    // --- Estado vital ---
    EtapaPecari etapa = EtapaPecari::ADULTO;
    ModoPecari  modo  = ModoPecari::QUIETO;

    // ⭐ RITMO PERSONAL, DE 0 A 50.
    //
    // Es el "cuanto corre ESTE animal" dentro de lo que le permite su modo de
    // locomocion. No sustituye a andar/trotar/huir: los modula.
    //
    //      0  -> el mas lento de su especie
    //     25  -> el ritmo tipico (lo que hacia el motor antes, sin variacion)
    //     50  -> el mas rapido
    //
    // POR QUE 0-50 Y NO 0-1: la escala se pidio asi, y ademas tiene una
    // ventaja practica -- es un entero, asi que se puede comparar, mostrar en
    // depuracion y guardar sin preocuparse de la precision del float.
    //
    // De donde sale: se DERIVA de la semilla del animal (ver RitmoDeSemilla),
    // asi que no cuesta memoria persistente y el mismo pecari corre siempre
    // igual, aunque su chunk se descargue y vuelva.
    uint8_t ritmo = 25;

    // ------------------------------------------------------------------------
    // SALUD, en medios puntos
    // ------------------------------------------------------------------------
    // En MEDIOS y no en float por lo mismo que la durabilidad de las
    // herramientas (ver Inventory.h): con enteros no hay error de redondeo
    // acumulado tras muchos golpes, que es justo lo que arruinaria una barra
    // de vida.
    //
    // 0 = sin estrenar: quien lo crea le pone la que le toca por su etapa. Se
    // hace asi para que un agente construido por defecto no nazca muerto.
    int vidaMedios = 0;

    // ------------------------------------------------------------------------
    // AUDACIA (boldness), 0..1
    // ------------------------------------------------------------------------
    // ⭐ ESTE RASGO ES REAL Y ESTA MEDIDO EN ESTA ESPECIE. No es una invencion
    // de diseño para dar variedad.
    //
    // MEDIDO: variabilidad pronunciada en boldness con plasticidad segun el
    // nivel de riesgo (PMID 34740780, Behavioural Processes 2021, n=26). Y
    // tiene consecuencias FISIOLOGICAS demostradas: los individuos mas
    // calmados digieren la fibra significativamente mejor (0.41 a 0.79 de
    // digestibilidad entre individuos, DOI 10.1017/s1751731120001354).
    //
    // Aqui se usa para lo que 11_PECARI_IA.json especifica en su formula:
    //
    //     distancia_huida *= (1.4 - 0.8 * audacia)
    //
    // O sea: "los audaces dejan acercarse mas". Un audaz aguanta al jugador
    // cerca y es mas propenso a plantar cara; un timido sale corriendo antes.
    //
    // Sale de la SEMILLA del animal, asi que es determinista y no cuesta
    // guardarla: el mismo pecari tiene siempre el mismo caracter.
    float audacia = 0.5f;

    // ------------------------------------------------------------------------
    // LA RAMA DE AMENAZA
    // ------------------------------------------------------------------------
    ConductaPecari conducta = ConductaPecari::CALMA;

    // De donde vino el golpe. Es lo que permite huir EN DIRECCION CONTRARIA en
    // vez de en una al azar, y encarar al agresor si toca defenderse.
    float amenazaX = 0.0f, amenazaZ = 0.0f;
    bool  hayAmenaza = false;

    // Cuanto le queda de sobresalto, en segundos. Al llegar a cero vuelve a
    // CALMA. Sin esto el animal huiria para siempre.
    float tiempoConducta = 0.0f;

    // Escala visual respecto al adulto. Sale de la etapa.
    float escala = 1.0f;

    // --- Deambular ---
    float objetivoX = 0.0f, objetivoZ = 0.0f;
    float tiempoHastaDecision = 0.0f;

    // --- Animacion ---
    // Fase del ciclo de paso, en radianes. Avanza con la velocidad real, de
    // modo que las patas NUNCA patinan: si el animal va despacio, el paso es
    // lento. Es lo mas barato que se puede hacer para que el movimiento se
    // lea bien.
    float fasePaso = 0.0f;

    // --- LA CABEZA, QUE GIRA APARTE DEL CUERPO ---
    //
    // Es lo que permite "si mueve la cabeza no mueve el cuerpo": el cuerpo
    // apunta a donde CAMINA, la cabeza a donde MIRA, y son cosas distintas.
    //
    // Biologicamente encaja: este animal tiene vista pesima (no distingue
    // objetos a mas de un metro, MEDIDO) y se orienta con el hocico. Mueve
    // mucho la cabeza precisamente porque no puede fiarse de los ojos.
    float giroCabezaY = 0.0f;      // guinada, radianes
    float giroCabezaX = 0.0f;      // cabeceo: bajar el hocico a olfatear
    float giroCabezaObjetivoY = 0.0f;
    float giroCabezaObjetivoX = 0.0f;
    float tiempoHastaMirar = 0.0f;

    // --- PARPADEO ---
    // Un ojo que nunca parpadea se lee como muerto.
    float parpadeo = 0.0f;         // 0 = abierto, 1 = cerrado
    float tiempoHastaParpadeo = 0.0f;

    // Cresta dorsal erizada, 0..1.
    //
    // MEDIDO (cualitativo): el animal levanta las cerdas del lomo al
    // alarmarse. Hoy nada la activa porque no hay sistema de miedo; el campo
    // existe porque el modelo 3D ya lo dibuja y porque es el enganche natural
    // para cuando exista ALERTA.
    float erizado = 0.0f;

    // LOD en el que se dibujo la ultima vez. Lo necesita la HISTERESIS: sin
    // recordar el nivel anterior, un animal en el umbral oscilaria entre dos
    // niveles cada frame y costaria mas que estando siempre en el alto.
    int lodActual = 0;

    // Tiempo que lleva vivo, en segundos. Lo usa la respiracion del cuerpo:
    // es lo que hace que un animal PARADO no parezca una estatua.
    float tiempoVivo = 0.0f;

    // Semilla propia, para que cada individuo deambule distinto sin compartir
    // estado global ni depender del orden de actualizacion.
    uint32_t semilla = 0;
};

// ----------------------------------------------------------------------------
// ESCALA POR ETAPA
// ----------------------------------------------------------------------------
// El adulto es 1.0. Las crias son notablemente mas pequenas.
//
// ANCLAJE MEDIDO: peso al nacer 0.5 kg, adulto 18.7 kg. La razon de masas es
// 37x, y como la masa va con el CUBO de la longitud, la razon de longitudes
// es 37^(1/3) = 3.33. Una cria mide por tanto ~0.30 del adulto.
//
// El resto de etapas se interpolan entre ese anclaje y el adulto.
// confianza: DERIVADO (neonato y adulto) / ESTIMADO (etapas intermedias)
inline float EscalaDeEtapa(EtapaPecari e) {
    switch (e) {
        case EtapaPecari::NEONATO:    return 0.30f;   // DERIVADO de 0.5/18.7 kg
        case EtapaPecari::JUVENIL:    return 0.55f;   // ESTIMADO
        case EtapaPecari::SUBADULTO:  return 0.80f;   // ESTIMADO
        case EtapaPecari::ADULTO:     return 1.00f;
        case EtapaPecari::SENESCENTE: return 0.97f;   // ESTIMADO: encoge un poco
    }
    return 1.0f;
}

// ============================================================================
// SALUD Y CARACTER
// ============================================================================

// Vida maxima por etapa, en MEDIOS puntos.
//
// Sale de la MASA MEDIDA de cada etapa (la misma tabla que ya usa el empuje):
// neonato 0.5 kg, juvenil 2.6, subadulto 8.9, adulto 18.7, senescente 17.1.
//
// No es la masa en bruto -- eso daria un adulto con 37 veces la vida de una
// cria y volveria ridiculo cazarlas. Se comprime: raiz de la proporcion
// respecto al adulto, redondeado a algo jugable.
//
//   ADULTO      6 puntos (12 medios)   varios golpes de hacha
//   SENESCENTE  5                      algo mas fragil, ya viejo
//   SUBADULTO   4
//   JUVENIL     3
//   NEONATO     2                      minimo digno; matar una cria de un
//                                      golpe seria feo y ademas quitaria el
//                                      dilema de que la manada la defienda
inline int VidaMaximaMedios(EtapaPecari e) {
    switch (e) {
        case EtapaPecari::NEONATO:    return 4;
        case EtapaPecari::JUVENIL:    return 6;
        case EtapaPecari::SUBADULTO:  return 8;
        case EtapaPecari::ADULTO:     return 12;
        case EtapaPecari::SENESCENTE: return 10;
    }
    return 12;
}

// La audacia del individuo, sacada de su semilla.
//
// El diseño (11_PECARI_IA.json) especifica Normal(0.5, 0.18) truncada a [0,1].
// Aqui se aproxima sumando tres uniformes: por el teorema central del limite
// la suma tiende a una normal, y con tres terminos ya queda una campana
// razonable. Es mucho mas barato que una gaussiana de verdad y no necesita
// tablas ni logaritmos.
//
// La media sale 0.5 y la desviacion ~0.167, muy cerca del 0.18 pedido.
inline float AudaciaDeSemilla(uint32_t semilla) {
    uint32_t s = semilla ^ 0x9E3779B9u;
    float suma = 0.0f;
    for (int i = 0; i < 3; ++i) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        suma += (float)(s % 100000u) / 100000.0f;
    }
    float a = suma / 3.0f;
    if (a < 0.0f) a = 0.0f;
    if (a > 1.0f) a = 1.0f;
    return a;
}

// ============================================================================
// ⭐ EL RITMO: DE 0 A 50
// ============================================================================
// Cuanto corre un animal CONCRETO dentro de lo que le permite su modo de
// locomocion. Es lo que hace que una piara no se mueva como un bloque unico:
// unos van delante y otros se rezagan, y siempre los mismos.
//
// ----------------------------------------------------------------------------
// POR QUE SE DERIVA DE LA SEMILLA Y NO SE GUARDA
// ----------------------------------------------------------------------------
// Misma razon que el tono del pelaje (ver 00_LEEME.txt del AI simulator): un
// valor derivado no cuesta memoria persistente y es REPRODUCIBLE. El mismo
// pecari corre siempre igual aunque su chunk se descargue y vuelva a cargarse,
// sin escribir un byte en el save.
//
// ----------------------------------------------------------------------------
// EL REPARTO NO ES UNIFORME, Y ES DELIBERADO
// ----------------------------------------------------------------------------
// Se usa la misma tecnica que la audacia --sumar tres muestras-- porque un
// reparto uniforme daria tantos animales extremos como medios, y una manada
// donde la mitad va al minimo y la otra al maximo se ve rara. Con la suma, la
// mayoria queda cerca de 25 y los extremos son escasos, que es como se reparte
// cualquier rasgo en una poblacion real.
inline uint8_t RitmoDeSemilla(uint32_t semilla) {
    // Otro mezclador que el de la audacia: si compartieran constante, el
    // animal mas audaz seria SIEMPRE el mas rapido, y eso es una correlacion
    // que nadie ha medido.
    uint32_t s = semilla ^ 0x85EBCA6Bu;
    float suma = 0.0f;
    for (int i = 0; i < 3; ++i) {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        suma += (float)(s % 100000u) / 100000.0f;
    }
    const float u = suma / 3.0f;                 // 0..1, con moda en 0.5
    int r = (int)(u * 50.0f + 0.5f);
    if (r < 0)  r = 0;
    if (r > 50) r = 50;
    return (uint8_t)r;
}

// El multiplicador de velocidad que corresponde a un ritmo.
//
// El rango va de 0.70 a 1.30, centrado en 1.0 para ritmo 25. O sea: el animal
// tipico se mueve EXACTAMENTE como antes de que existiera este sistema, y los
// extremos se apartan un 30%.
//
// Ese 30% no es un numero libre: por encima, el mas lento de la manada se
// queda tan atras que la cohesion no lo alcanza y se ve como un animal roto;
// por debajo, la diferencia no se aprecia y el sistema no sirve de nada.
inline float FactorDeRitmo(uint8_t ritmo) {
    if (ritmo > 50) ritmo = 50;
    const float t = (float)ritmo / 50.0f;        // 0..1
    return 0.70f + t * 0.60f;                    // 0.70 .. 1.30
}

// ----------------------------------------------------------------------------
// LA RAMA DE AMENAZA: CONSTANTES
// ----------------------------------------------------------------------------
namespace PecariAmenaza {

    // Distancia BASE a la que el animal se aleja del peligro, en bloques.
    // La real se modula con la audacia (ver DistanciaHuida).
    constexpr float DISTANCIA_HUIDA_BASE = 18.0f;

    // Lo que corre huyendo. Mas que el trote normal: es una carrera, no un
    // desplazamiento.
    //
    // ESTIMADO. La velocidad punta de la especie no la encontre medida en la
    // literatura revisada; se toma el doble del trote, que ya era ESTIMADO.
    constexpr float VEL_HUIDA = 6.0f;

    // Cuanto dura el sobresalto antes de calmarse, en segundos.
    //
    // ANCLAJE: el cortisol de esta especie tiene una semivida MEDIDA de 66
    // minutos, asi que "calmarse del todo" es cosa de horas. Eso es
    // impracticable en un juego, y ademas dejaria manadas huyendo para
    // siempre. Se comprime a una escala jugable y se declara como tal.
    constexpr float DURACION_HUIDA  = 8.0f;
    constexpr float DURACION_ALERTA = 5.0f;
    constexpr float DURACION_DEFENSA = 10.0f;

    // Radio en el que un grito de alarma llega a los companeros de manada.
    // Por debajo del radio de alarma de cohesion (14): la manada se entera de
    // lo que le pasa a un miembro que tiene cerca, no de lo que pasa al otro
    // lado del mapa.
    constexpr float RADIO_ALARMA = 12.0f;

    // Radio en el que se busca una CRIA para decidir si toca defender.
    constexpr float RADIO_CRIA = 8.0f;

    // A que distancia del agresor deja de acercarse el que defiende. Es su
    // alcance de embestida: se le echa encima y se para ahi.
    constexpr float ALCANCE_EMBESTIDA = 1.6f;

    // Audacia por encima de la cual un adulto encara en vez de huir aunque no
    // haya crias de por medio.
    //
    // 0.78 sobre una Normal(0.5, 0.18) deja aproximadamente al 6% de los
    // individuos: los muy audaces. Es lo bastante raro para que sorprenda y lo
    // bastante frecuente para que ocurra.
    constexpr float AUDACIA_ENCARA = 0.78f;

    // Distancia de huida del individuo, con la formula del diseño:
    //     distancia_huida *= (1.4 - 0.8 * audacia)
    // MEDIDO (que la audacia modula la tolerancia) / ESTIMADO (los coeficientes).
    inline float DistanciaHuida(float audacia) {
        return DISTANCIA_HUIDA_BASE * (1.4f - 0.8f * audacia);
    }

} // namespace PecariAmenaza

// ----------------------------------------------------------------------------
// HITBOX
// ----------------------------------------------------------------------------
// Sale de la anatomia MEDIDA, no de un numero inventado.
//   ancho  = tronco (0.25 m) con margen para las patas
//   alto   = altura a la cruz MEDIDA (0.44 m)
//   largo  = longitud cabeza-cuerpo MEDIDA (0.95 m)
struct HitboxPecari {
    float semiAncho;   // en bloques
    float alto;
    float semiLargo;
};

inline HitboxPecari HitboxDe(const PecariAgente& p) {
    using namespace ::Pecari;
    HitboxPecari h;
    h.semiAncho = aBloques(Externo::TRONCO.hx * 1.4f) * p.escala;
    h.alto      = aBloques(Escala::ALTO_CRUZ_M)       * p.escala;
    h.semiLargo = aBloques(Escala::LARGO_CABEZA_CUERPO_M * 0.5f) * p.escala;
    return h;
}

// ============================================================================
// SIMULACION DE MOVIMIENTO
// ============================================================================
// Funciones puras o casi puras, para poder testearlas sin motor.

// Hash rapido para decisiones individuales. Avanza la semilla del animal.
inline uint32_t SiguienteAleatorio(uint32_t& s) {
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}

inline float AleatorioUnidad(uint32_t& s) {
    return (float)(SiguienteAleatorio(s) % 100000u) / 100000.0f;
}

// ----------------------------------------------------------------------------
// CENTRO DE LA MANADA
// ----------------------------------------------------------------------------
// El centroide de los miembros. 01_ARQUITECTURA lo define asi explicitamente:
// "la manada NO es un agente, es una relacion entre agentes".
struct CentroManada {
    float x = 0.0f, z = 0.0f;
    int   miembros = 0;
};

inline CentroManada CalcularCentro(const std::vector<PecariAgente>& todos, int idManada) {
    CentroManada c;
    for (const PecariAgente& p : todos) {
        if (p.idManada != idManada) continue;
        c.x += p.x; c.z += p.z; ++c.miembros;
    }
    if (c.miembros > 0) {
        c.x /= (float)c.miembros;
        c.z /= (float)c.miembros;
    }
    return c;
}

// ----------------------------------------------------------------------------
// ELEGIR UN DESTINO NUEVO
// ----------------------------------------------------------------------------
// El animal deambula ALREDEDOR DEL CENTRO DE SU MANADA, no del mapa entero.
// Eso es lo que produce que el grupo se mueva junto sin que nadie lo dirija:
// no hay lider, hay una querencia compartida.
//
// (El liderazgo emergente de Couzin 2005, donde un individuo informado arrastra
// al grupo, es fase posterior: necesita el sistema de necesidades para que
// alguien tenga informacion que le importe.)
inline void ElegirDestino(PecariAgente& p, const CentroManada& centro) {
    using namespace PecariMovimiento;

    const float ang = AleatorioUnidad(p.semilla) * 6.28318531f;
    const float rad = AleatorioUnidad(p.semilla) * RADIO_DEAMBULAR;

    if (centro.miembros > 0) {
        p.objetivoX = centro.x + std::cos(ang) * rad;
        p.objetivoZ = centro.z + std::sin(ang) * rad;
    } else {
        // Solitario: deambula alrededor de si mismo.
        p.objetivoX = p.x + std::cos(ang) * rad;
        p.objetivoZ = p.z + std::sin(ang) * rad;
    }

    p.tiempoHastaDecision = TIEMPO_DECISION_MIN +
        AleatorioUnidad(p.semilla) * (TIEMPO_DECISION_MAX - TIEMPO_DECISION_MIN);
}

// ----------------------------------------------------------------------------
// PASO DE SIMULACION DE UN INDIVIDUO
// ----------------------------------------------------------------------------
// dt en segundos. `vecinos` son los k mas proximos de su manada.
//
// El orden de las fuerzas importa y es el de boids clasico, con la extension
// de relacion social que 01_ARQUITECTURA pide:
//   1. SEPARACION  - no atravesarse (la mas fuerte, es fisica)
//   2. COHESION    - volver al grupo si se ha alejado
//   3. OBJETIVO    - ir a donde queria ir
inline void ActualizarPecari(PecariAgente& p,
                             const CentroManada& centro,
                             const std::vector<const PecariAgente*>& vecinos,
                             float dt) {
    using namespace PecariMovimiento;

    // --- 0. Reloj propio ---
    // Alimenta la respiracion, que corre aunque el animal no se mueva.
    p.tiempoVivo += dt;

    // ------------------------------------------------------------------------
    // ⭐ 0-bis. LA AMENAZA MANDA SOBRE TODO LO DEMAS
    // ------------------------------------------------------------------------
    // Un animal asustado no forrajea ni se preocupa de si la manada le queda
    // lejos: se va. Por eso esto va ANTES del deambular y le pisa el objetivo.
    //
    // Es la traduccion del rank 10 que 11_PECARI_IA.json da a HUIR y DEFENDER:
    // "puede saltarse la seleccion normal si el estimulo supera un umbral".
    if (p.conducta != ConductaPecari::CALMA) {
        p.tiempoConducta -= dt;

        if (p.tiempoConducta <= 0.0f) {
            // Se le pasa el susto.
            p.conducta = ConductaPecari::CALMA;
            p.hayAmenaza = false;
            // Y se le fuerza a elegir destino nuevo: si se quedara con el que
            // tenia antes del sobresalto, volveria caminando al sitio donde
            // le acaban de pegar.
            p.tiempoHastaDecision = 0.0f;
        } else if (p.hayAmenaza) {
            const float ax = p.x - p.amenazaX;
            const float az = p.z - p.amenazaZ;
            const float d  = std::sqrt(ax*ax + az*az);

            if (p.conducta == ConductaPecari::HUIDA) {
                // ALEJARSE: el objetivo se pone al otro lado, en la direccion
                // opuesta al agresor.
                if (d > 1e-3f) {
                    const float dist = PecariAmenaza::DistanciaHuida(p.audacia);
                    p.objetivoX = p.x + (ax / d) * dist;
                    p.objetivoZ = p.z + (az / d) * dist;
                }
            } else if (p.conducta == ConductaPecari::DEFENSA) {
                // ENCARAR: el objetivo es el agresor. Se para a un cuerpo de
                // distancia para no meterse dentro de el.
                if (d > PecariAmenaza::ALCANCE_EMBESTIDA) {
                    p.objetivoX = p.amenazaX;
                    p.objetivoZ = p.amenazaZ;
                } else {
                    p.objetivoX = p.x;   // ya lo tiene encima: se planta
                    p.objetivoZ = p.z;
                }
            } else {
                // ALERTA: no se mueve, solo mira y eriza. Se queda donde esta.
                p.objetivoX = p.x;
                p.objetivoZ = p.z;
            }
        }

        // La cresta erizada acompaña a todo lo que no sea calma. Es la unica
        // señal externa que el animal ya sabia dar (el modelo 3D la dibuja).
        p.erizado = 1.0f;
    }

    // --- 1. Decidir destino si toca ---
    p.tiempoHastaDecision -= dt;
    if (p.tiempoHastaDecision <= 0.0f && p.conducta == ConductaPecari::CALMA) {
        ElegirDestino(p, centro);
    }

    // --- 2. Direccion deseada: empieza por el objetivo ---
    float dirX = p.objetivoX - p.x;
    float dirZ = p.objetivoZ - p.z;

    // --- 3. Cohesion: si se ha alejado del grupo, tira hacia el centro ---
    //
    // ⚠️ La cohesion NO se aplica al que huye o al que defiende.
    //
    // Si se aplicara, un animal que sale corriendo se veria arrastrado de
    // vuelta hacia el centro de la manada en cuanto pasara del radio de
    // alarma -- o sea, huiria en circulos alrededor de sus companeros y
    // acabaria volviendo justo al sitio del que escapa. Cuando hay un
    // depredador, alejarse gana a no perder al grupo.
    bool debeTrotar = false;
    if (centro.miembros > 1 && p.conducta == ConductaPecari::CALMA) {
        const float haciaX = centro.x - p.x;
        const float haciaZ = centro.z - p.z;
        const float dist = std::sqrt(haciaX*haciaX + haciaZ*haciaZ);

        if (dist > RADIO_ALARMA_BLOQUES) {
            // Muy lejos: la cohesion DOMINA y ademas trota.
            // Es lo que impide que un individuo se pierda para siempre.
            dirX = haciaX; dirZ = haciaZ;
            debeTrotar = true;
        } else if (dist > RADIO_COMODO_BLOQUES) {
            // Algo lejos: se suma una componente de regreso, proporcional a
            // lo pasado que esta del radio comodo.
            const float exceso = (dist - RADIO_COMODO_BLOQUES) /
                                 (RADIO_ALARMA_BLOQUES - RADIO_COMODO_BLOQUES);
            dirX += haciaX * exceso;
            dirZ += haciaZ * exceso;
        }
    }

    // --- 4. Separacion: no atravesar a los vecinos ---
    // Es la fuerza mas fuerte porque es fisica, no preferencia.
    for (const PecariAgente* v : vecinos) {
        const float dx = p.x - v->x;
        const float dz = p.z - v->z;
        const float d2 = dx*dx + dz*dz;
        if (d2 < SEPARACION_MINIMA * SEPARACION_MINIMA && d2 > 1e-6f) {
            const float d = std::sqrt(d2);
            const float empuje = (SEPARACION_MINIMA - d) / SEPARACION_MINIMA;
            dirX += (dx / d) * empuje * 4.0f;
            dirZ += (dz / d) * empuje * 4.0f;
        }
    }

    // --- 5. Normalizar y decidir el modo ---
    const float len = std::sqrt(dirX*dirX + dirZ*dirZ);
    if (len < 0.15f) {
        // Ya esta donde queria: se para. Parar es importante: un animal que
        // nunca se detiene se lee como un automata.
        //
        // ⭐ PERO SE PARA FRENANDO, no de golpe.
        //
        // Esto era p.vx = p.vz = 0 en seco, asi que el animal pasaba de su
        // velocidad plena a cero en un frame. Ademas la amplitud del paso sale
        // de la rapidez (PecariCuerpo), asi que las patas se congelaban a
        // media zancada en vez de terminar el paso.
        const float velActual = std::sqrt(p.vx*p.vx + p.vz*p.vz);
        const float paso = FRENADA * dt;
        if (velActual <= paso || velActual < 1e-4f) {
            p.modo = ModoPecari::QUIETO;
            p.vx = 0.0f; p.vz = 0.0f;
        } else {
            // Todavia rodando: sigue en su modo y frena.
            const float k = (velActual - paso) / velActual;
            p.vx *= k;
            p.vz *= k;
        }
    } else {
        dirX /= len; dirZ /= len;

        // ⭐ HUIR Y DEFENDER SON CARRERA, NO PASEO.
        //
        // El que huye corre de verdad; el que va a embestir tambien. Un
        // animal que "huye" a velocidad de paseo no se lee como asustado, y
        // uno que carga andando no da ningun miedo.
        const bool corriendo = (p.conducta == ConductaPecari::HUIDA ||
                                p.conducta == ConductaPecari::DEFENSA);

        p.modo = (corriendo || debeTrotar) ? ModoPecari::TROTANDO
                                           : ModoPecari::ANDANDO;
        // ⭐ Y CADA ANIMAL LLEVA SU PROPIO RITMO ENCIMA.
        //
        // El modo decide el rango (pasear, trotar, huir) y el ritmo decide
        // donde cae ESTE animal dentro de el. Multiplicar en vez de sumar es
        // lo correcto: un pecari rapido lo es tanto paseando como huyendo,
        // que es como funciona la condicion fisica de verdad.
        const float velBase = corriendo  ? PecariAmenaza::VEL_HUIDA
                            : debeTrotar ? VEL_TROTANDO
                                         : VEL_ANDANDO;
        const float vel = velBase * FactorDeRitmo(p.ritmo);

        // ⭐ ACELERAR HACIA LA VELOCIDAD QUE TOCA, no saltar a ella.
        //
        // Antes esto era p.vx = dirX * vel directamente. Ver el comentario de
        // ACELERACION arriba: el salto instantaneo es lo que hacia que el
        // movimiento no se leyera como el de un animal.
        //
        // Se acelera el VECTOR entero, no cada eje por separado, para que al
        // cambiar de rumbo la velocidad describa una curva en vez de girar en
        // escuadra.
        // ⭐ EN CURVA CERRADA SE VA MAS DESPACIO.
        //
        // Sin esto el animal derrapa: se traslada a toda velocidad hacia un
        // lado mientras el cuerpo todavia esta girando hacia otro, y un
        // cuadrupedo no puede moverse perpendicular a su propio eje.
        //
        // El factor sale del angulo entre donde MIRA y donde QUIERE IR: de
        // frente va a tope, y cuanto mas cerrado el giro, mas frena. No baja
        // de 0.35 para que no se quede clavado al darse la vuelta.
        const float angDeseado = std::atan2(dirX, dirZ);
        float desvio = angDeseado - p.orientacion;
        while (desvio >  3.14159265f) desvio -= 6.28318531f;
        while (desvio < -3.14159265f) desvio += 6.28318531f;

        const float cosDesvio = std::cos(desvio);
        const float frenoCurva = (cosDesvio > 0.35f) ? cosDesvio : 0.35f;
        const float velCurva = vel * frenoCurva;

        const float objVx = dirX * velCurva;
        const float objVz = dirZ * velCurva;

        const float difVx = objVx - p.vx;
        const float difVz = objVz - p.vz;
        const float difLen = std::sqrt(difVx*difVx + difVz*difVz);

        if (difLen > 1e-5f) {
            // Frenar cuesta menos que arrancar: si el objetivo es mas lento
            // que la velocidad actual, se usa la frenada.
            const float velActual = std::sqrt(p.vx*p.vx + p.vz*p.vz);
            // Se compara con velCurva, que es el objetivo REAL de este frame:
            // usar 'vel' haria que un animal frenando en curva se creyera
            // acelerando.
            const float tasa = (velCurva < velActual) ? FRENADA : ACELERACION;

            const float paso = tasa * dt;
            if (paso >= difLen) {
                // Llega en este frame: no hay que pasarse.
                p.vx = objVx;
                p.vz = objVz;
            } else {
                p.vx += difVx / difLen * paso;
                p.vz += difVz / difLen * paso;
            }
        }

        // --- 6. Girar hacia donde va, sin teletransportar la orientacion ---
        // 'desvio' ya es el angulo que falta, normalizado a [-pi, pi] arriba
        // para el freno en curva. No hace falta recalcularlo.
        float delta = desvio;
        const float giroMax = VEL_GIRO * dt;
        if (delta >  giroMax) delta =  giroMax;
        if (delta < -giroMax) delta = -giroMax;
        p.orientacion += delta;
    }

    // --- 7. Integrar posicion ---
    //
    // ⚠️ AQUI YA NO SE MUEVE AL ANIMAL.
    //
    // Antes esto era `p.x += p.vx * dt; p.z += p.vz * dt;` -- una suma directa
    // sin preguntarle nada al mundo. Ese era el bug de "el pecari atraviesa
    // los bloques": no habia colision porque no habia NADA que comprobar, y
    // esta funcion ni siquiera recibe el mundo para poder hacerlo.
    //
    // Ahora deja la INTENCION de movimiento (vx, vz) y de moverlo se encarga
    // MundoPecaries::moverConColision(), que si tiene el mundo delante.
    //
    // Se hace asi y no cambiando la firma de esta funcion a proposito: es una
    // funcion PURA sobre el agente y sus vecinos, y eso es lo que permite
    // testear el comportamiento de manada sin motor (ver
    // tests/test_pecari_entidad.cpp). Meterle el mundo la ataria al juego.

    // --- 8. Fase del paso, ligada a la velocidad REAL ---
    // Asi las patas no patinan nunca: si no se mueve, no hay ciclo de paso.
    const float rapidez = std::sqrt(p.vx*p.vx + p.vz*p.vz);
    p.fasePaso += rapidez * dt * 2.6f;
    if (p.fasePaso > 6.28318531f) p.fasePaso -= 6.28318531f;

    // --- 9. LA CABEZA: mira aparte del cuerpo ---
    //
    // El cuerpo apunta a donde CAMINA; la cabeza va por libre. Cada cierto
    // tiempo el animal elige otra direccion a la que mirar, y la cabeza gira
    // hacia ella suavemente.
    //
    // Cuando esta QUIETO baja mas el hocico: es un animal que se orienta
    // olfateando el suelo, no mirando. Su vista es pesima (MEDIDO: no
    // distingue objetos a mas de un metro) y detecta raices a 8 cm bajo
    // tierra, asi que hozar es su forma normal de explorar.
    p.tiempoHastaMirar -= dt;
    if (p.tiempoHastaMirar <= 0.0f) {
        // Nueva direccion de mirada, dentro de lo que da el cuello.
        p.giroCabezaObjetivoY = (AleatorioUnidad(p.semilla) * 2.0f - 1.0f) * 0.9f;

        if (p.modo == ModoPecari::QUIETO) {
            // Parado: hocico abajo, olfateando.
            p.giroCabezaObjetivoX = 0.25f + AleatorioUnidad(p.semilla) * 0.40f;
        } else {
            // En marcha: la cabeza mas nivelada.
            p.giroCabezaObjetivoX = (AleatorioUnidad(p.semilla) * 2.0f - 1.0f) * 0.20f;
        }

        p.tiempoHastaMirar = 1.2f + AleatorioUnidad(p.semilla) * 3.0f;
    }

    // Giro suave hacia el objetivo. La cabeza gira MAS RAPIDO que el cuerpo:
    // mirar es barato, girarse entero no.
    {
        constexpr float VEL_CUELLO = 4.5f;   // rad/s
        const float pasoMax = VEL_CUELLO * dt;

        float dY = p.giroCabezaObjetivoY - p.giroCabezaY;
        if (dY >  pasoMax) dY =  pasoMax;
        if (dY < -pasoMax) dY = -pasoMax;
        p.giroCabezaY += dY;

        float dX = p.giroCabezaObjetivoX - p.giroCabezaX;
        if (dX >  pasoMax) dX =  pasoMax;
        if (dX < -pasoMax) dX = -pasoMax;
        p.giroCabezaX += dX;
    }

    // --- 10. PARPADEO ---
    if (p.parpadeo > 0.0f) {
        // Cerrando y abriendo: el parpadeo dura ~0.12 s.
        p.parpadeo -= dt / 0.12f;
        if (p.parpadeo < 0.0f) p.parpadeo = 0.0f;
    } else {
        p.tiempoHastaParpadeo -= dt;
        if (p.tiempoHastaParpadeo <= 0.0f) {
            p.parpadeo = 1.0f;
            p.tiempoHastaParpadeo = 3.0f + AleatorioUnidad(p.semilla) * 5.0f;
        }
    }

    // --- 11. Cresta: relajarse ---
    // Se eriza deprisa y baja despacio (MEDIDO cualitativamente como patron
    // de sobresalto en presas; el factor es ESTIMADO). Hoy nada la eriza,
    // asi que solo baja.
    if (p.erizado > 0.0f) {
        // Ojo al ::Pecari con dos puntos delante: dentro de namespace Fauna,
        // "Pecari" a secas es el STRUCT de este archivo, no el namespace del
        // modelo 3D. Sin el ambito global esto no compila.
        p.erizado -= dt * ::Pecari::Externo::CRESTA_FACTOR_BAJADA;
        if (p.erizado < 0.0f) p.erizado = 0.0f;
    }
}

// ----------------------------------------------------------------------------
// VECINOS TOPOLOGICOS
// ----------------------------------------------------------------------------
// Los K_VECINOS mas proximos de la misma manada (Ballerini et al. 2008).
//
// Con manadas de 5-15 esto es casi siempre "todos", pero la cota es lo que
// impide que una manada grande cueste O(n^2) y ademas es el limite biologico
// real.
inline void BuscarVecinos(const std::vector<PecariAgente>& todos,
                          size_t indicePropio,
                          std::vector<const PecariAgente*>& salida) {
    salida.clear();
    const PecariAgente& yo = todos[indicePropio];

    // Insercion ordenada por distancia, con tope. Para k=7 es mas rapido que
    // ordenar el vector entero.
    float mejoresD2[PecariMovimiento::K_VECINOS];
    const PecariAgente* mejores[PecariMovimiento::K_VECINOS];
    int n = 0;

    for (size_t i = 0; i < todos.size(); ++i) {
        if (i == indicePropio) continue;
        const PecariAgente& o = todos[i];
        if (o.idManada != yo.idManada) continue;

        const float dx = o.x - yo.x;
        const float dz = o.z - yo.z;
        const float d2 = dx*dx + dz*dz;

        if (n < PecariMovimiento::K_VECINOS) {
            int j = n++;
            while (j > 0 && mejoresD2[j-1] > d2) {
                mejoresD2[j] = mejoresD2[j-1]; mejores[j] = mejores[j-1]; --j;
            }
            mejoresD2[j] = d2; mejores[j] = &o;
        } else if (d2 < mejoresD2[n-1]) {
            int j = n-1;
            while (j > 0 && mejoresD2[j-1] > d2) {
                mejoresD2[j] = mejoresD2[j-1]; mejores[j] = mejores[j-1]; --j;
            }
            mejoresD2[j] = d2; mejores[j] = &o;
        }
    }

    for (int i = 0; i < n; ++i) salida.push_back(mejores[i]);
}

// ============================================================================
// GEOMETRIA DE DIBUJO
// ============================================================================
// El motor necesita una lista de cajas ya colocadas y orientadas. Se le da
// eso y nada mas: este archivo no llama a OpenGL.
//
// Cada pieza sale de PecariModelo3D.h, que a su vez sale de
// 13_PECARI_ANATOMIA.json. Los rasgos que un modelo NO debe equivocarse
// (tronco en barril, hocico estrecho, patas finas, cola inexistente, collar,
// cresta) estan todos aqui.

struct CajaDibujo {
    // Centro en coordenadas de MUNDO, en bloques.
    float cx, cy, cz;
    // Medias dimensiones en bloques, ya escaladas por la etapa.
    float hx, hy, hz;
    // Color RGB 0..1
    float r, g, b;
};

// Colores del pelaje.
//
// MEDIDO (cualitativo): "cerdas largas, gruesas y rigidas, de tonos
// grisaceos, negros o castanos". El collar es una banda CLARA que cruza el
// hombro y da nombre a la especie.
namespace ColorPecari {
    constexpr float CUERPO_R = 0.29f, CUERPO_G = 0.26f, CUERPO_B = 0.23f;
    constexpr float COLLAR_R = 0.72f, COLLAR_G = 0.68f, COLLAR_B = 0.58f;
    constexpr float PATA_R   = 0.20f, PATA_G   = 0.18f, PATA_B   = 0.16f;
    constexpr float HOCICO_R = 0.16f, HOCICO_G = 0.14f, HOCICO_B = 0.13f;
}

// Rota un punto (dx,dz) alrededor del origen segun la orientacion del animal.
inline void RotarXZ(float dx, float dz, float ang, float& outX, float& outZ) {
    const float c = std::cos(ang), s = std::sin(ang);
    // orientacion 0 = mirando a +Z
    outX = dx * c + dz * s;
    outZ = -dx * s + dz * c;
}

// Convierte una caja anatomica (en metros, espacio local del animal) en una
// caja de dibujo en coordenadas de mundo.
inline CajaDibujo CajaAMundo(const ::Pecari::Caja& caja, const PecariAgente& p,
                             float r, float g, float b,
                             float desplazY = 0.0f) {
    using ::Pecari::aBloques;

    const float lx = aBloques(caja.cx) * p.escala;
    const float ly = aBloques(caja.cy) * p.escala + desplazY;
    const float lz = aBloques(caja.cz) * p.escala;

    float wx, wz;
    RotarXZ(lx, lz, p.orientacion, wx, wz);

    CajaDibujo d;
    d.cx = p.x + wx;
    d.cy = p.y + ly;
    d.cz = p.z + wz;
    d.hx = aBloques(caja.hx) * p.escala;
    d.hy = aBloques(caja.hy) * p.escala;
    d.hz = aBloques(caja.hz) * p.escala;
    d.r = r; d.g = g; d.b = b;
    return d;
}

// ----------------------------------------------------------------------------
// CONSTRUIR EL CUERPO COMPLETO
// ----------------------------------------------------------------------------
// Devuelve las cajas que hay que dibujar, ya orientadas y en mundo.
//
// El orden no importa para el dibujado, pero se agrupa por partes para que
// sea legible.
inline void ConstruirCuerpo(const PecariAgente& p, std::vector<CajaDibujo>& salida) {
    using namespace ::Pecari;
    using namespace ColorPecari;
    salida.clear();

    // --- TRONCO: el barril ---
    salida.push_back(CajaAMundo(Externo::TRONCO, p, CUERPO_R, CUERPO_G, CUERPO_B));

    // --- CABEZA ---
    salida.push_back(CajaAMundo(Externo::CABEZA, p, CUERPO_R, CUERPO_G, CUERPO_B));

    // --- HOCICO: mucho mas estrecho que el craneo ---
    // Es el rasgo que impide que la cabeza parezca un ladrillo.
    salida.push_back(CajaAMundo(Externo::HOCICO, p, HOCICO_R, HOCICO_G, HOCICO_B));

    // --- COLLAR: la banda clara del hombro ---
    // MEDIDO y diagnostico: da nombre a la especie.
    {
        Caja collar;
        collar.cx = 0.0f;
        collar.cy = Externo::TRONCO.cy;
        collar.cz = Externo::COLLAR_Z;
        collar.hx = Externo::TRONCO.hx * 1.02f;   // sobresale un pelo
        collar.hy = Externo::TRONCO.hy * 1.02f;
        collar.hz = Externo::COLLAR_ANCHO_M * 0.5f;
        salida.push_back(CajaAMundo(collar, p, COLLAR_R, COLLAR_G, COLLAR_B));
    }

    // --- CRESTA DORSAL ERECTIL ---
    // Se levanta con `erizado`. En reposo es una linea baja; alarmada, el
    // animal parece mucho mas grande (factor ~3.5 MEDIDO cualitativamente).
    {
        const float alturaCresta =
            Externo::CRESTA_ALTURA_REPOSO_M +
            (Externo::CRESTA_ALTURA_ERIZADA_M - Externo::CRESTA_ALTURA_REPOSO_M) * p.erizado;

        Caja cresta;
        cresta.cx = 0.0f;
        cresta.cy = Externo::TRONCO.maxY() + alturaCresta * 0.5f;
        cresta.cz = Externo::TRONCO.cz;
        cresta.hx = Externo::CRESTA_ANCHO_M * 0.5f;
        cresta.hy = alturaCresta * 0.5f;
        cresta.hz = Externo::CRESTA_LARGO_M * 0.5f;
        salida.push_back(CajaAMundo(cresta, p, PATA_R, PATA_G, PATA_B));
    }

    // --- LAS CUATRO PATAS, con ciclo de paso ---
    //
    // Marcha diagonal: la delantera izquierda va con la trasera derecha.
    // Es el patron de un cuadrupedo real al andar, y sale gratis desfasando
    // pi radianes los pares diagonales.
    {
        const float largoPata = Externo::PATA_LARGO_M;
        const float grosor    = Externo::PATA_GROSOR_M;

        struct DefPata { float sx, sz; float faseOffset; };
        const DefPata patas[4] = {
            { -1.0f, Externo::PATA_DELANTERA_Z, 0.0f            },  // del. izq
            {  1.0f, Externo::PATA_DELANTERA_Z, 3.14159265f     },  // del. der
            { -1.0f, Externo::PATA_TRASERA_Z,   3.14159265f     },  // tras. izq
            {  1.0f, Externo::PATA_TRASERA_Z,   0.0f            }   // tras. der
        };

        for (const DefPata& dp : patas) {
            // La pata oscila adelante y atras con el ciclo de paso.
            const float swing = std::sin(p.fasePaso + dp.faseOffset);

            Caja pata;
            pata.cx = dp.sx * Externo::PATA_SEPARACION_X;
            pata.cy = largoPata * 0.5f;
            pata.cz = dp.sz + swing * 0.045f;   // amplitud del paso
            pata.hx = grosor * 0.5f;
            pata.hy = largoPata * 0.5f;
            pata.hz = grosor * 0.5f;
            salida.push_back(CajaAMundo(pata, p, PATA_R, PATA_G, PATA_B));
        }
    }

    // --- LA COLA ---
    // MEDIDO: 1.2 cm. Practicamente inexistente.
    // Se dibuja DELIBERADAMENTE minuscula: el error mas facil de cometer con
    // este animal es darle cola de cerdo, y seria visible al instante.
    {
        Caja cola;
        cola.cx = 0.0f;
        cola.cy = Externo::TRONCO.maxY() - 0.02f;
        cola.cz = Externo::TRONCO.minZ() - Escala::LARGO_COLA_M * 0.5f;
        cola.hx = 0.012f;
        cola.hy = 0.012f;
        cola.hz = Escala::LARGO_COLA_M * 0.5f;
        salida.push_back(CajaAMundo(cola, p, CUERPO_R, CUERPO_G, CUERPO_B));
    }
}

} // namespace Fauna

#endif // PECARI_ENTIDAD_H
