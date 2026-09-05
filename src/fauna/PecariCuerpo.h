#ifndef PECARI_CUERPO_H
#define PECARI_CUERPO_H

#include <cstdint>
#include <cmath>
#include <vector>
#include "PecariModelo3D.h"

// ============================================================================
// EL CUERPO DEL PECARI: ESQUELETO JERARQUICO, OJOS, ARTICULACIONES Y PELO
// ============================================================================
// Sustituye al ConstruirCuerpo() plano de PecariEntidad.h, que apilaba diez
// cajas sueltas. Aqui el cuerpo es un ARBOL de huesos: cada pieza cuelga de su
// padre y hereda su transformacion.
//
// ----------------------------------------------------------------------------
// POR QUE UNA JERARQUIA Y NO UNA LISTA
// ----------------------------------------------------------------------------
// Es lo que hace posible lo que se pidio: "si mueve la cabeza no mueve el
// cuerpo".
//
// Con una lista plana, girar la cabeza obliga a recalcular a mano la posicion
// de hocico, ojos, orejas y colmillos, y cualquier olvido produce una cara
// descuadrada. Con un arbol, se gira el hueso CUELLO y todo lo que cuelga de
// el le sigue solo, mientras el tronco no se entera.
//
// La jerarquia sale de la anatomia real, no de conveniencia:
//
//   RAIZ (el animal en el mundo)
//    +- TRONCO ............... el barril; centro de masa
//    |   +- CUELLO ........... corto y musculoso
//    |   |   +- CABEZA ....... gira independiente del tronco
//    |   |       +- HOCICO ... el organo de trabajo
//    |   |       +- OJO izq/der
//    |   |       +- OREJA izq/der
//    |   |       +- COLMILLOS
//    |   +- CRESTA DORSAL .... de la coronilla a la grupa
//    |   +- GLANDULA ......... sobre las ancas
//    |   +- COLA ............. 1.2 cm, casi inexistente
//    +- PATA x4
//        +- SEGMENTO SUPERIOR (humero / femur)
//            +- SEGMENTO MEDIO (radioulna / tibia)  <- articulacion
//                +- CANA (metapodios fusionados)    <- articulacion
//                    +- PEZUNA
//
// ----------------------------------------------------------------------------
// LAS ARTICULACIONES SALEN DEL ESQUELETO MEDIDO
// ----------------------------------------------------------------------------
// No se inventa donde dobla la pata. El esqueleto de esta especie DICTA que
// articulaciones existen y cuales NO:
//
//   RADIO Y ULNA FUSIONADOS (MEDIDO)  -> el antebrazo NO rota. Es un solo
//                                        hueso. Un pecari no puede girar la
//                                        muneca como un perro.
//   METAPODIOS 3-4 FUSIONADOS (MEDIDO)-> la cana es UNA pieza rigida.
//   CARPO Y TARSO SOLDADOS (MEDIDO)   -> el tobillo dobla en un solo eje.
//
// El resultado es una pata que dobla en TRES puntos y solo en el plano de
// avance. Eso es exactamente lo que hace a un ungulado corredor: se sacrifica
// movilidad por rigidez y eficiencia. Modelarla con rotulas libres seria
// contradecir el hueso.
//
// ----------------------------------------------------------------------------
// TRAZABILIDAD
// ----------------------------------------------------------------------------
// Misma escala del AI simulator: MEDIDO / DERIVADO / INFERIDO / ESTIMADO.
// Fuentes nuevas de esta ronda citadas en cada bloque.
// ============================================================================

namespace Fauna {

// ============================================================================
// ESCALA DE TEXTURA
// ============================================================================
// El cuerpo se define en PIXELES, como una textura de vóxel, y se convierte a
// metros en un solo sitio.
//
// ----------------------------------------------------------------------------
// UN CONFLICTO REAL QUE HUBO QUE RESOLVER, Y COMO
// ----------------------------------------------------------------------------
// Las tres medidas pedidas (50 largo x 40 ancho x 46 alto) son coherentes
// ENTRE SI, pero chocaban con la anatomia MEDIDA si se anclaban al largo:
//
//   Anclando 50 px = 0.490 m (el tronco MEDIDO) sale 1 px = 0.0098 m.
//   Entonces el torso mide 46 * 0.0098 = 0.4508 m DE ALTO.
//   Pero la altura a la cruz MEDIDA es 0.44 m.
//
//   Es decir: el torso solo seria MAS ALTO que el animal entero. No quedaria
//   ni un milimetro para las patas. Geometricamente imposible.
//
// SOLUCION ADOPTADA: se conservan las tres medidas pedidas EXACTAMENTE, y se
// cambia el ANCLA. En vez de fijar el largo del tronco, se fija la ALTURA A
// LA CRUZ, que es el dato mas solido de los dos:
//
//   cruz (0.44 m) = alto del torso (46 px) + largo de pata libre (18 px)
//   -> 64 px de cruz = 0.44 m  ->  1 px = 0.006875 m
//
// El reparto 46/18 no es arbitrario: da un torso que ocupa el 72% de la
// altura, coherente con "patas largas y delgadas SOBRE UN CUERPO MACIZO",
// que es la firma visual descrita de la especie.
//
// COSTE HONESTO DE LA DECISION: con esta escala el tronco mide 0.344 m de
// largo en vez de los 0.490 m MEDIDOS. Es un 30% mas corto. El animal
// completo (hocico a grupa) sigue rondando los 0.95 m MEDIDOS porque cabeza y
// cuello aportan el resto, pero el TRONCO por si solo se aparta del dato.
//
// Se declara sin disimulo: son las proporciones que se pidieron, y respetarlas
// obliga a ceder en algo. Se cedio en el largo del tronco y no en la altura a
// la cruz porque la cruz es lo que determina si el animal se ve del tamano
// correcto junto al jugador.
namespace Px {
    // Torso, tal como se pidio. Estos tres numeros NO se tocan.
    //
    // ACTUALIZACION: el largo subio de 50 a 70 px por peticion. Las
    // proporciones se recalcularon MANTENIENDO la relacion ancho/alto que ya
    // se habia fijado, para que el animal no cambie de forma al alargarse:
    //   antes  50 x 40 x 46
    //   factor 70/50 = 1.4
    //   ahora  70 x 56 x 64.4  ->  se redondea a 70 x 56 x 64
    constexpr float TORSO_LARGO_PX = 70.0f;   // "grueso" = profundidad Z
    constexpr float TORSO_ANCHO_PX = 56.0f;   // X
    constexpr float TORSO_ALTO_PX  = 64.0f;   // Y

    // Largo de pata visible bajo la panza, en px.
    // DERIVADO: lo que queda de la cruz al descontar el torso.
    // Se escala igual que el resto: 18 * 1.4 = 25.2 -> 25
    constexpr float PATA_LIBRE_PX = 25.0f;

    // La cruz en px: torso + pata.
    constexpr float CRUZ_PX = TORSO_ALTO_PX + PATA_LIBRE_PX;   // 64

    // EL ANCLA: la altura a la cruz MEDIDA.
    // Rango publicado 30-50 cm; se usa 0.44 m, el mismo valor que ya usaba
    // 13_PECARI_ANATOMIA para el ejemplar tipo.
    constexpr float CRUZ_M = 0.44f;

    constexpr float METROS_POR_PX = CRUZ_M / CRUZ_PX;   // 0.006875

    constexpr float aM(float px) { return px * METROS_POR_PX; }

    // Cuantos px vale un bloque del motor. Sale del cociente, no se elige.
    constexpr float PX_POR_BLOQUE = 0.60f / METROS_POR_PX;   // ~87.3
}

// Comprobaciones que impiden que la escala se vaya de madre en silencio.
// Si alguien cambia un numero de arriba sin pensar, el build para.
static_assert(Px::aM(Px::CRUZ_PX) == Px::CRUZ_M,
              "La cruz debe seguir siendo el ancla de la escala");
static_assert(Px::aM(Px::TORSO_ALTO_PX) < Px::CRUZ_M,
              "El torso no puede ser mas alto que la cruz: no quedaria sitio "
              "para las patas");
static_assert(Px::aM(Px::TORSO_LARGO_PX) > 0.20f &&
              Px::aM(Px::TORSO_LARGO_PX) < 0.60f,
              "El tronco se ha ido del orden de magnitud de un pecari real");
static_assert(Px::TORSO_LARGO_PX > Px::TORSO_ANCHO_PX,
              "Es un barril: tiene que ser mas largo que ancho");

// ============================================================================
// DATOS ANATOMICOS NUEVOS DE ESTA RONDA DE INVESTIGACION
// ============================================================================

// ----------------------------------------------------------------------------
// EL OJO
// ----------------------------------------------------------------------------
// Hay DOS estudios publicados sobre la retina de esta especie, y entre los dos
// dan un ojo con caracter propio en vez de dos puntos negros.
namespace Ojo {

    // --- Lo que se sabe de la vision, MEDIDO ---
    //
    // AGUDEZA MUY BAJA. La literatura de campo es unanime: "vista
    // extremadamente pobre, no distinguen objetos a mas de un metro". Compensan
    // con olfato (detectan raices a 8 cm bajo tierra) y oido.
    constexpr float ALCANCE_UTIL_M = 1.0f;              // MEDIDO (cualitativo)

    // SIN TAPETUM LUCIDUM. El fondo de ojo es ATAPETAL.
    // CONSECUENCIA DE JUEGO: los ojos del pecari NO deben brillar de noche.
    // Es el error tipico al dibujar fauna, y aqui seria falso.
    // Fuente: "The retina of the collared peccary: structure and function",
    // Veterinary Ophthalmology (2018), PMID 29336116.
    constexpr bool TIENE_TAPETUM = false;               // MEDIDO

    // DOMINAN LOS CONOS, no los bastones. Las respuestas de bastones en ERG
    // fueron muy bajas y sin adaptacion a la oscuridad evidente.
    // Es confirmacion FISIOLOGICA independiente de que el animal es DIURNO:
    // no solo se le ve de dia, es que su ojo esta construido para el dia.
    constexpr bool DOMINAN_CONOS = true;                // MEDIDO
    constexpr float CONOS_S_PORCENTAJE = 10.0f;         // MEDIDO: conos S = 10%

    // VISION DICROMATICA: conos sensibles a onda corta y media.
    // Ve el mundo aproximadamente como un humano con daltonismo rojo-verde.
    constexpr int TIPOS_DE_CONO = 2;                    // MEDIDO

    // --- La retina, con numeros ---
    // Costa et al. (2020), PLoS One. n = 6 retinas de 3 machos de 15-22 kg.
    constexpr float CELULAS_GANGLIONARES_TOTAL = 1029498.0f;  // MEDIDO
    constexpr float DENSIDAD_PICO_CG_POR_MM2   = 6767.0f;     // MEDIDO
    constexpr float AREA_RETINA_MM2            = 837.8f;      // MEDIDO

    // FRANJA VISUAL HORIZONTAL (visual streak): banda alargada de alta
    // densidad en la retina centro-dorsal.
    //
    // POR QUE IMPORTA PARA EL MODELO: una franja HORIZONTAL significa que el
    // animal ve mejor a lo ancho del horizonte que arriba y abajo. Es la
    // firma de una presa de terreno abierto, y va con pupila HORIZONTAL y
    // ojos LATERALES.
    constexpr bool TIENE_FRANJA_VISUAL = true;          // MEDIDO

    // --- Geometria del ojo en el modelo ---
    // Ojos LATERALES, no frontales: es un herbivoro presa. La posicion lateral
    // da vision casi panoramica (320-340 grados en ungulados) a costa de poca
    // vision binocular.
    // confianza: MEDIDO (que son laterales) / ESTIMADO (los px concretos)
    constexpr float DIAMETRO_PX      = 3.0f;
    constexpr float SEPARACION_X_PX  = 8.5f;    // desde el eje, hacia fuera
    constexpr float ALTURA_PX        = 4.0f;    // sobre el centro de la cabeza
    constexpr float ADELANTO_Z_PX    = 4.0f;    // hacia el hocico

    // Pupila horizontal, como el resto de ungulados presa.
    // confianza: INFERIDO (de ungulados con franja visual horizontal)
    constexpr float PUPILA_ANCHO_PX = 2.2f;
    constexpr float PUPILA_ALTO_PX  = 1.0f;

    // --- Parpadeo ---
    // Un ojo que nunca parpadea se lee como muerto. No hay medicion de la tasa
    // en esta especie.
    // confianza: ESTIMADO
    constexpr float PARPADEO_CADA_S_MIN = 3.0f;
    constexpr float PARPADEO_CADA_S_MAX = 8.0f;
    constexpr float PARPADEO_DURACION_S = 0.12f;
}

// ----------------------------------------------------------------------------
// EL PELO
// ----------------------------------------------------------------------------
// Se pidio que tengan pelo. El pelaje de esta especie tiene tres rasgos
// documentados que lo hacen reconocible, y los tres se modelan.
namespace Pelo {

    // 1. CERDAS, no pelo suave. "Largas, gruesas y rigidas": casi puas
    //    flexibles. Por eso el animal se ve erizado incluso en calma.
    constexpr float CERDA_LARGO_M  = 0.050f;   // ESTIMADO (de 13_ANATOMIA)
    constexpr float CERDA_GROSOR_M = 0.0016f;  // ESTIMADO

    // 2. PATRON AGUTI: cada pelo lleva BANDAS alternas oscuras y claras a lo
    //    largo del tallo. No es un color plano.
    //
    //    CONSECUENCIA VISUAL: de lejos el animal se ve gris jaspeado
    //    ("grizzled black and gray"), no gris liso. Se reproduce variando el
    //    tono por mechon en vez de pintar el cuerpo de un color unico.
    constexpr bool PATRON_AGUTI = true;        // MEDIDO
    constexpr int  BANDAS_POR_PELO = 3;        // MEDIDO (cualitativo)

    // 3. MELENA DORSAL CONTINUA: de la CORONILLA a la GRUPA, donde esta la
    //    glandula. No es solo una cresta en el lomo: arranca en la cabeza.
    //    Es un detalle que casi todas las representaciones se saltan.
    constexpr bool MELENA_LLEGA_A_LA_CORONILLA = true;   // MEDIDO

    // Cuantos mechones se dibujan. Es un compromiso: suficientes para que se
    // lea como pelaje, pocos para no hundir el frame rate.
    //
    // El usuario pidio explicitamente "sin abrumar" en el encargo original.
    // confianza: ESTIMADO (decision de rendimiento)
    constexpr int MECHONES_LOMO    = 14;
    constexpr int MECHONES_FLANCO  = 5;   // por costado
    constexpr int MECHONES_CUELLO  = 4;

    // Cuanto varia el tono entre mechones, para el efecto jaspeado del aguti.
    constexpr float VARIACION_TONO = 0.22f;   // ESTIMADO
}

// ----------------------------------------------------------------------------
// LAS ARTICULACIONES
// ----------------------------------------------------------------------------
// Que la pata doble en tres puntos NO es una decision de animacion: es lo que
// permite el esqueleto MEDIDO de esta especie.
namespace Articulacion {

    // Los tres puntos de flexion de cada pata.
    //
    // Delantera: HOMBRO -> CODO -> CARPO (muneca)
    // Trasera:   CADERA -> RODILLA -> CORVEJON (tarso)
    //
    // Y NO hay rotacion de antebrazo, porque radio y ulna estan FUSIONADOS
    // (MEDIDO). Todas las articulaciones son de BISAGRA en el plano de avance.
    constexpr bool SOLO_BISAGRA = true;   // MEDIDO (derivado de la fusion osea)

    // Amplitud de cada articulacion al andar, en radianes.
    // No hay cinematica publicada de esta especie: son valores de animacion
    // ajustados para que el paso se lea bien.
    // confianza: ESTIMADO
    constexpr float AMPLITUD_HOMBRO   = 0.38f;
    constexpr float AMPLITUD_CODO     = 0.30f;
    constexpr float AMPLITUD_CARPO    = 0.22f;

    constexpr float AMPLITUD_CADERA   = 0.34f;
    constexpr float AMPLITUD_RODILLA  = 0.42f;   // la trasera flexiona mas
    constexpr float AMPLITUD_CORVEJON = 0.26f;

    // El codo y la rodilla doblan en SENTIDOS OPUESTOS. Es la diferencia
    // anatomica que hace que un cuadrupedo se vea como un cuadrupedo y no
    // como una mesa andando: la pata delantera dobla hacia atras, la trasera
    // hacia delante.
    constexpr float SENTIDO_DELANTERA = -1.0f;   // MEDIDO (anatomia general)
    constexpr float SENTIDO_TRASERA   = +1.0f;

    // Proporciones de los tres segmentos de la pata, en fraccion del largo
    // total. La cana (metapodio fusionado) es larga: es lo que hace al animal
    // digitigrado y buen corredor.
    // confianza: DERIVADO de las cajas de Esqueleto en PecariModelo3D.h
    constexpr float FRACCION_SUPERIOR = 0.38f;   // humero / femur
    constexpr float FRACCION_MEDIA    = 0.34f;   // radioulna / tibia
    constexpr float FRACCION_CANA     = 0.28f;   // metapodios fusionados
}

// ============================================================================
// GEOMETRIA DE SALIDA
// ============================================================================

// Una caja ya situada y orientada en el mundo, lista para dibujar.
struct PiezaCuerpo {
    float cx, cy, cz;        // centro en mundo, bloques
    float hx, hy, hz;        // medias dimensiones, bloques
    float r, g, b;           // color base

    // ------------------------------------------------------------------------
    // ROTACION PROPIA DE LA PIEZA — EL ARREGLO DE UN BUG REAL
    // ------------------------------------------------------------------------
    // Sin estos dos angulos, cada pieza era una AABB: una caja SIEMPRE alineada
    // a los ejes del mundo.
    //
    // EL BUG QUE ESO PRODUCIA, medido antes de arreglarlo:
    //   El torso mide 50 px de largo y 40 de ancho. Al girar el animal 90
    //   grados, el largo deberia pasar al eje X. Se midio hx/hy/hz a 0, 30, 60
    //   y 90 grados y salieron IDENTICOS en las cuatro.
    //
    //   O sea: el animal cambiaba de sitio pero su cuerpo seguia mirando al
    //   norte. Al caminar en diagonal se veia atravesado, que es exactamente
    //   lo que se reporto.
    //
    // La causa era que EmitirCaja rotaba la POSICION del centro pero nunca la
    // CAJA. Ahora cada pieza lleva su propia orientacion y el motor la aplica
    // al dibujar.
    float giroY;             // guinada de la pieza, radianes
    float giroX;             // cabeceo de la pieza, radianes

    // Normal dominante para la iluminacion. Las piezas del cuerpo son cajas,
    // asi que el motor ilumina cara a cara; esto sirve para piezas finas
    // (pelo) que se dibujan como laminas.
    float nx, ny, nz;

    // Emision propia. SIEMPRE 0 en este animal: no tiene tapetum lucidum, asi
    // que sus ojos NO brillan en la oscuridad. Se deja el campo para que otra
    // especie que si lo tenga (un felino) pueda usarlo.
    float emision;

    // Si es true, la pieza es un mechon de pelo: fina, y el motor puede
    // decidir dibujarla con menos detalle a distancia.
    bool esPelo;
};

// ----------------------------------------------------------------------------
// ESTADO DE POSE
// ----------------------------------------------------------------------------
// Lo que el animal le dice a su cuerpo. Separado del agente para que el cuerpo
// se pueda construir en tests sin simular nada.
struct PosePecari {
    // Situacion en el mundo
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float orientacionCuerpo = 0.0f;    // radianes; 0 = mirando a +Z

    // LA CABEZA GIRA APARTE DEL CUERPO.
    // Es lo que se pidio: "si mueve la cabeza no mueve el cuerpo".
    float giroCabezaY = 0.0f;          // guinada: mirar a los lados
    float giroCabezaX = 0.0f;          // cabeceo: bajar el hocico al suelo

    // Animacion
    float fasePaso = 0.0f;             // ciclo de marcha, radianes
    float rapidez  = 0.0f;             // bloques/s, modula la amplitud
    float erizado  = 0.0f;             // 0..1, cresta dorsal
    float parpadeo = 0.0f;             // 0 = ojo abierto, 1 = cerrado

    float escala = 1.0f;               // por etapa vital
    uint32_t semilla = 1u;             // variacion individual del pelaje

    // Etapa vital, 0..4 (NEONATO, JUVENIL, SUBADULTO, ADULTO, SENESCENTE).
    // Decide las PROPORCIONES, no solo el tamano: una cria tiene la cabeza
    // proporcionalmente mucho mayor.
    int etapa = 3;                     // ADULTO por defecto

    // ------------------------------------------------------------------------
    // TIEMPO ACUMULADO
    // ------------------------------------------------------------------------
    // Para los movimientos que NO dependen del paso: respirar, y el balanceo
    // lento de un animal parado. Sin esto, un pecari quieto seria una estatua.
    float tiempoVivo = 0.0f;
};

// ----------------------------------------------------------------------------
// MOVIMIENTO CORPORAL REALISTA
// ----------------------------------------------------------------------------
// Un cuadrupedo andando no se desplaza rigido: el cuerpo sube y baja, se
// balancea de lado y cabecea. Sin eso el animal se ve deslizando sobre el
// suelo aunque las patas se muevan bien.
//
// Los tres movimientos salen del MISMO ciclo de paso, que es lo que los hace
// coherentes entre si en vez de tres oscilaciones sueltas.
namespace Balanceo {
    // SUBIDA Y BAJADA. Dos veces por ciclo de paso: el cuerpo sube cada vez
    // que un par diagonal empuja. Por eso va al DOBLE de frecuencia.
    // confianza: ESTIMADO (animacion, sin cinematica publicada de la especie)
    constexpr float VERTICAL_PX = 1.6f;

    // BALANCEO LATERAL (roll). El peso pasa de un par diagonal al otro.
    constexpr float LATERAL_RAD = 0.055f;

    // CABECEO (pitch). El tren delantero baja al recibir el peso.
    constexpr float CABECEO_RAD = 0.045f;

    // GUINADA DEL TRONCO. Un cuadrupedo corto de cuerpo culebrea un poco al
    // andar: el tren trasero no sigue exactamente la linea del delantero.
    constexpr float SERPENTEO_RAD = 0.030f;

    // RESPIRACION. Independiente del paso: sigue cuando el animal esta quieto.
    // Es lo que impide que un pecari parado parezca muerto.
    // El pecari en reposo respira del orden de 20-30 veces por minuto
    // (INFERIDO de mamiferos de talla similar; no hay dato publicado para la
    // especie, declarado AUSENTE en 10_PECARI_BIOLOGIA).
    constexpr float RESPIRACION_HZ    = 0.42f;   // ~25 por minuto
    constexpr float RESPIRACION_ESCALA = 0.012f; // cuanto se hincha el torso
}

// ============================================================================
// PROPORCIONES POR EDAD
// ============================================================================
// Una cria NO es un adulto encogido. Si solo se multiplica todo por 0.30 sale
// un adulto en miniatura, y se nota: los animales jovenes tienen la cabeza
// proporcionalmente ENORME y las patas comparativamente largas y finas.
//
// Es alometria real, no estilo: el craneo crece antes que el resto del cuerpo
// en practicamente todos los mamiferos, asi que al nacer la cabeza ya esta
// cerca de su tamano final mientras el tronco aun tiene mucho que crecer.
//
// ANCLAJES MEDIDOS de la especie (10_PECARI_BIOLOGIA):
//   peso al nacer      0.5 kg
//   peso adulto       18.7 kg
//   destete            6 semanas
//   independencia      9 meses
//   madurez macho    ~12 meses
//   primera reproduccion real de la hembra  16-21 meses
//
// La razon de masas 18.7/0.5 = 37.4 da una razon de LONGITUDES de 37.4^(1/3)
// = 3.34, o sea que el neonato mide ~0.30 del adulto. Eso es DERIVADO.
// El reparto de esa diferencia entre cabeza, tronco y patas es ESTIMADO.
struct ProporcionEdad {
    float escalaGeneral;    // tamano global respecto al adulto
    float factorCabeza;     // >1 = cabeza proporcionalmente mayor
    float factorPatas;      // >1 = patas proporcionalmente mas largas
    float factorTorso;      // <1 = tronco proporcionalmente menor
    float factorHocico;     // el hocico se alarga con la edad
};

// Las cinco etapas, con las tres que se pidieron bien diferenciadas.
inline ProporcionEdad ProporcionesDe(int etapa) {
    switch (etapa) {
        case 0:  // NEONATO — el "bebe"
            // Cabeza muy grande, patas larguiruchas, cuerpo poco desarrollado.
            // Es el aspecto tipico de una cria de ungulado: casi todo cabeza
            // y patas.
            return { 0.30f, 1.42f, 1.18f, 0.86f, 0.72f };

        case 1:  // JUVENIL — ya destetado (>6 semanas MEDIDO)
            return { 0.52f, 1.28f, 1.12f, 0.91f, 0.80f };

        case 2:  // SUBADULTO — el "adolescente"
            // Ya casi adulto de forma, pero aun no de tamano. Entre la
            // independencia (9 meses) y la primera reproduccion (16-21).
            return { 0.78f, 1.12f, 1.05f, 0.96f, 0.91f };

        case 3:  // ADULTO — la referencia
            return { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f };

        case 4:  // SENESCENTE
            // Encoge algo y pierde masa muscular: el tronco se afina.
            return { 0.97f, 1.02f, 0.98f, 0.95f, 1.02f };
    }
    return { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f };
}

// ----------------------------------------------------------------------------
// LIMITES DEL CUELLO
// ----------------------------------------------------------------------------
// La cabeza no gira 360 grados. Un cuello corto y musculoso como el de esta
// especie tiene recorrido limitado.
// confianza: ESTIMADO (no hay medicion publicada de amplitud cervical)
namespace Cuello {
    constexpr float GIRO_MAX_Y = 1.13f;    // ~65 grados a cada lado
    constexpr float GIRO_MAX_X = 0.70f;    // ~40 grados arriba/abajo

    // Velocidad de giro de la cabeza. Mas rapida que la del cuerpo: mirar es
    // barato, girarse entero no.
    constexpr float VEL_GIRO = 4.5f;       // rad/s   ESTIMADO
}

inline float LimitarGiroCuelloY(float a) {
    if (a >  Cuello::GIRO_MAX_Y) return  Cuello::GIRO_MAX_Y;
    if (a < -Cuello::GIRO_MAX_Y) return -Cuello::GIRO_MAX_Y;
    return a;
}

inline float LimitarGiroCuelloX(float a) {
    if (a >  Cuello::GIRO_MAX_X) return  Cuello::GIRO_MAX_X;
    if (a < -Cuello::GIRO_MAX_X) return -Cuello::GIRO_MAX_X;
    return a;
}

// ============================================================================
// COLORES
// ============================================================================
// MEDIDO (cualitativo): "negro y gris jaspeado, mas claro en los hombros, con
// una raya dorsal oscura" y el collar blanquecino cruzando el hombro.
namespace Tono {
    constexpr float CUERPO_R = 0.285f, CUERPO_G = 0.258f, CUERPO_B = 0.232f;
    constexpr float LOMO_R   = 0.185f, LOMO_G   = 0.170f, LOMO_B   = 0.158f;  // raya dorsal
    constexpr float COLLAR_R = 0.730f, COLLAR_G = 0.690f, COLLAR_B = 0.590f;
    constexpr float PATA_R   = 0.195f, PATA_G   = 0.178f, PATA_B   = 0.162f;
    constexpr float HOCICO_R = 0.150f, HOCICO_G = 0.132f, HOCICO_B = 0.125f;
    constexpr float PEZUNA_R = 0.105f, PEZUNA_G = 0.098f, PEZUNA_B = 0.092f;

    // Ojo: esclerotica muy oscura y pupila negra. Sin brillo: es ATAPETAL.
    constexpr float OJO_R = 0.070f, OJO_G = 0.058f, OJO_B = 0.052f;
    constexpr float PUPILA_R = 0.020f, PUPILA_G = 0.018f, PUPILA_B = 0.016f;

    // Un punto de luz especular MUY tenue, para que el ojo no parezca pintado.
    // No es tapetum: es reflejo de la cornea humeda, que si existe.
    constexpr float BRILLO_OJO_R = 0.42f, BRILLO_OJO_G = 0.40f, BRILLO_OJO_B = 0.38f;

    constexpr float COLMILLO_R = 0.82f, COLMILLO_G = 0.80f, COLMILLO_B = 0.72f;
}

// ============================================================================
// CONSTRUCCION DEL CUERPO
// ============================================================================

// Rota (dx,dz) alrededor del eje Y.
inline void RotarY(float dx, float dz, float ang, float& oX, float& oZ) {
    const float c = std::cos(ang), s = std::sin(ang);
    oX = dx * c + dz * s;
    oZ = -dx * s + dz * c;
}

// Hash determinista para la variacion del pelaje.
inline float RuidoPelo(uint32_t semilla, int indice) {
    uint32_t h = semilla ^ ((uint32_t)indice * 2654435761u);
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return (float)(h % 1000u) / 1000.0f;
}

// ----------------------------------------------------------------------------
// UN NODO DEL ESQUELETO
// ----------------------------------------------------------------------------
// Posicion y orientacion acumuladas de un hueso. Es lo que permite que la
// cabeza gire sin arrastrar el tronco: cada hueso solo conoce a su padre.
struct NodoHueso {
    float x, y, z;        // origen del hueso en espacio local del animal
    float giroY;          // guinada acumulada
    float giroX;          // cabeceo acumulado
};

// Aplica un desplazamiento local a un nodo, respetando su rotacion.
// Los desplazamientos dx,dy,dz llegan en PIXELES (que es como se declara todo
// el cuerpo) y los nodos viven en METROS. La conversion ocurre AQUI, en un
// solo sitio.
//
// Es el error que este codigo tuvo y que un test cazo: sumar pixeles a un
// nodo en metros mandaba la cabeza a 17 metros de altura. Mezclar unidades es
// el fallo mas facil de cometer y el mas dificil de ver leyendo, asi que la
// frontera esta marcada de forma explicita.
inline NodoHueso HijoDe(const NodoHueso& padre,
                        float dxPx, float dyPx, float dzPx,
                        float giroYExtra = 0.0f, float giroXExtra = 0.0f) {
    NodoHueso n;

    // px -> m, antes de cualquier otra cosa.
    const float dx = Px::aM(dxPx);
    const float dy = Px::aM(dyPx);
    const float dz = Px::aM(dzPx);

    // El desplazamiento se rota por la orientacion del PADRE.
    float rx, rz;
    RotarY(dx, dz, padre.giroY, rx, rz);

    // Cabeceo del padre: inclina el eje Z/Y del hijo.
    const float cx = std::cos(padre.giroX), sx = std::sin(padre.giroX);
    const float dyRot = dy * cx - rz * sx;
    const float dzRot = dy * sx + rz * cx;

    n.x = padre.x + rx;
    n.y = padre.y + dyRot;
    n.z = padre.z + dzRot;
    n.giroY = padre.giroY + giroYExtra;
    n.giroX = padre.giroX + giroXExtra;
    return n;
}

// Emite una caja colgada de un hueso.
inline void EmitirCaja(std::vector<PiezaCuerpo>& salida,
                       const NodoHueso& hueso,
                       const PosePecari& pose,
                       float dxPx, float dyPx, float dzPx,
                       float hxPx, float hyPx, float hzPx,
                       float r, float g, float b,
                       bool esPelo = false,
                       float emision = 0.0f) {
    // Local del hueso -> local del animal
    float rx, rz;
    RotarY(Px::aM(dxPx), Px::aM(dzPx), hueso.giroY, rx, rz);

    const float cx = std::cos(hueso.giroX), sx = std::sin(hueso.giroX);
    const float dyM = Px::aM(dyPx);
    const float yRot = dyM * cx - rz * sx;
    const float zRot = dyM * sx + rz * cx;

    const float lx = hueso.x + rx;
    const float ly = hueso.y + yRot;
    const float lz = hueso.z + zRot;

    // Local del animal -> mundo
    float wx, wz;
    RotarY(lx, lz, pose.orientacionCuerpo, wx, wz);

    // Los tamanos van en metros; a bloques se dividen por 0.60.
    constexpr float M_A_BLOQUES = 1.0f / 0.60f;

    PiezaCuerpo p;
    p.cx = pose.x + wx * pose.escala * M_A_BLOQUES;
    p.cy = pose.y + ly * pose.escala * M_A_BLOQUES;
    p.cz = pose.z + wz * pose.escala * M_A_BLOQUES;
    p.hx = Px::aM(hxPx) * pose.escala * M_A_BLOQUES;
    p.hy = Px::aM(hyPx) * pose.escala * M_A_BLOQUES;
    p.hz = Px::aM(hzPx) * pose.escala * M_A_BLOQUES;
    p.r = r; p.g = g; p.b = b;

    // LA ROTACION DE LA PIEZA. Es lo que arregla el bug del torso: la caja no
    // solo se coloca en su sitio, tambien queda ORIENTADA como el hueso del
    // que cuelga, mas la orientacion del animal entero.
    p.giroY = hueso.giroY + pose.orientacionCuerpo;
    p.giroX = hueso.giroX;

    p.nx = 0.0f; p.ny = 1.0f; p.nz = 0.0f;
    p.emision = emision;
    p.esPelo = esPelo;
    salida.push_back(p);
}

// ----------------------------------------------------------------------------
// UNA PATA CON TRES ARTICULACIONES
// ----------------------------------------------------------------------------
// Construye los cuatro segmentos encadenados. Cada uno cuelga del anterior,
// asi que doblar el codo mueve tambien la cana y la pezuna, como en una pata
// de verdad.
inline void ConstruirPata(std::vector<PiezaCuerpo>& salida,
                          const NodoHueso& tronco,
                          const PosePecari& pose,
                          float ladoX,        // -1 izquierda, +1 derecha
                          float posZPx,       // adelante/atras respecto al tronco
                          bool esDelantera,
                          float faseOffset,
                          float factorPatas = 1.0f) {
    using namespace Articulacion;

    // La amplitud del paso crece con la velocidad: parado, la pata no oscila.
    // Es lo que impide el patinaje.
    const float amp = std::fmin(pose.rapidez / 1.1f, 1.4f);
    const float ciclo = std::sin(pose.fasePaso + faseOffset);
    // El segundo armonico da el "recoger la pata" de la fase de vuelo.
    const float cicloFlex = std::fmax(0.0f, -std::cos(pose.fasePaso + faseOffset));

    const float sentido = esDelantera ? SENTIDO_DELANTERA : SENTIDO_TRASERA;

    const float ampSup = (esDelantera ? AMPLITUD_HOMBRO : AMPLITUD_CADERA)   * amp;
    const float ampMed = (esDelantera ? AMPLITUD_CODO   : AMPLITUD_RODILLA)  * amp;
    const float ampInf = (esDelantera ? AMPLITUD_CARPO  : AMPLITUD_CORVEJON) * amp;

    // Largo total de la pata: del hombro (dentro del torso) al suelo.
    //
    // La articulacion superior nace algo por dentro del torso, no en su borde,
    // asi que la pata es un poco mas larga que el hueco visible bajo la panza.
    //
    // SIN HUECO CON EL SUELO: el largo se calcula para que la punta de la
    // pezuna quede EXACTAMENTE a nivel del suelo. Antes las patas se quedaban
    // cortas y el animal parecia flotar, que es lo que se reporto.
    //
    // La cadena es: hombro (a 0.45 del alto del torso, hacia dentro) ->
    // 3 segmentos -> pezuna. Todo eso tiene que sumar la distancia del hombro
    // al suelo.
    // OJO CON ESTE CALCULO: fue un bug real.
    //
    // El hombro NO esta a "PATA_LIBRE + 0.45*TORSO" del suelo. Se coloca a
    // -0.45*TORSO desde el CENTRO del tronco, y el centro esta a
    // PATA_LIBRE + 0.5*TORSO. Asi que la altura real del hombro es:
    //
    //   (PATA_LIBRE + 0.5*TORSO) - 0.45*TORSO  =  PATA_LIBRE + 0.05*TORSO
    //
    // Con la formula equivocada la cadena de la pata salia 25.6 px mas larga
    // de la cuenta y las pezunas se hundian 12.7 cm en el suelo. Es
    // exactamente el hueco entre pies y terreno que se reporto, pero al reves.
    const float alturaHombroPx = Px::PATA_LIBRE_PX + Px::TORSO_ALTO_PX * 0.05f;
    const float pezunaAltoPx   = 2.85f * factorPatas;

    // Lo que deben medir los tres segmentos juntos: del hombro al suelo,
    // descontando la pezuna que va debajo.
    //
    // SIN factorPatas aqui: el largo tiene que llegar al suelo EXACTAMENTE,
    // sea cual sea la edad. Multiplicarlo dejaria a las crias flotando (o
    // hundidas), que es justo lo que se pidio evitar.
    //
    // El factorPatas de las crias se nota en el GROSOR (patas mas finas) y en
    // que el animal entero es mas pequeno por la escala general, no en que la
    // pata deje de tocar el suelo.
    const float largoTotalPx = alturaHombroPx - pezunaAltoPx;

    const float largoSup  = largoTotalPx * FRACCION_SUPERIOR;
    const float largoMed  = largoTotalPx * FRACCION_MEDIA;
    const float largoCana = largoTotalPx * FRACCION_CANA;

    // Grosores: la pata se AFINA hacia abajo. Es la firma visual de la
    // especie: "patas largas y delgadas sobre un cuerpo macizo".
    const float grosorSup  = 7.3f * factorPatas;
    const float grosorMed  = 5.6f * factorPatas;
    const float grosorCana = 4.2f * factorPatas;

    // --- ARTICULACION 1: hombro / cadera ---
    // Nace un poco DENTRO del torso (0.45 de su media altura), no colgando de
    // su borde: es donde esta la cabeza del humero o del femur en un animal
    // real, y evita que la pata parezca pegada por fuera.
    const float anguloSup = ciclo * ampSup;
    NodoHueso artSuperior = HijoDe(tronco,
        ladoX * (Px::TORSO_ANCHO_PX * 0.38f),
        -Px::TORSO_ALTO_PX * 0.45f,
        posZPx,
        0.0f, anguloSup);

    EmitirCaja(salida, artSuperior, pose,
               0.0f, -largoSup * 0.5f, 0.0f,
               grosorSup * 0.5f, largoSup * 0.5f, grosorSup * 0.5f,
               Tono::PATA_R, Tono::PATA_G, Tono::PATA_B);

    // --- ARTICULACION 2: codo / rodilla ---
    // Dobla en el sentido que dicta la anatomia: la delantera hacia atras,
    // la trasera hacia delante.
    const float anguloMed = sentido * cicloFlex * ampMed;
    NodoHueso artMedia = HijoDe(artSuperior, 0.0f, -largoSup, 0.0f, 0.0f, anguloMed);

    EmitirCaja(salida, artMedia, pose,
               0.0f, -largoMed * 0.5f, 0.0f,
               grosorMed * 0.5f, largoMed * 0.5f, grosorMed * 0.5f,
               Tono::PATA_R, Tono::PATA_G, Tono::PATA_B);

    // --- ARTICULACION 3: carpo / corvejon ---
    // La cana es UNA pieza: metapodios 3 y 4 FUSIONADOS (MEDIDO).
    const float anguloInf = -sentido * cicloFlex * ampInf;
    NodoHueso artCana = HijoDe(artMedia, 0.0f, -largoMed, 0.0f, 0.0f, anguloInf);

    EmitirCaja(salida, artCana, pose,
               0.0f, -largoCana * 0.5f, 0.0f,
               grosorCana * 0.5f, largoCana * 0.5f, grosorCana * 0.5f,
               Tono::PATA_R, Tono::PATA_G, Tono::PATA_B);

    // --- PEZUNAS ---
    // MEDIDO: solo DOS dedos apoyan (los centrales). Los laterales cuelgan
    // sin cargar peso. Delante hay 4 dedos, detras 3: es LA diferencia con el
    // cerdo verdadero, que tiene 4 detras.
    NodoHueso pie = HijoDe(artCana, 0.0f, -largoCana, 0.0f);

    const float pezunaAncho = 2.05f * factorPatas;
    const float pezunaAlto  = pezunaAltoPx;

    // Los dos que apoyan.
    for (int d = 0; d < 2; ++d) {
        const float sep = (d == 0 ? -1.0f : 1.0f) * pezunaAncho * 0.62f;
        EmitirCaja(salida, pie, pose,
                   sep, -pezunaAlto * 0.5f, 0.6f,
                   pezunaAncho * 0.5f, pezunaAlto * 0.5f, pezunaAncho * 0.7f,
                   Tono::PEZUNA_R, Tono::PEZUNA_G, Tono::PEZUNA_B);
    }

    // Los laterales reducidos: cuelgan detras y ARRIBA, sin tocar el suelo.
    // Delantera 4 dedos -> 2 laterales. Trasera 3 -> 1 lateral.
    const int laterales = esDelantera ? 2 : 1;
    for (int d = 0; d < laterales; ++d) {
        const float sep = (laterales == 1) ? 0.0f
                        : ((d == 0 ? -1.0f : 1.0f) * pezunaAncho * 0.72f);
        EmitirCaja(salida, pie, pose,
                   sep, pezunaAlto * 0.35f, -1.5f,
                   pezunaAncho * 0.30f, pezunaAlto * 0.32f, pezunaAncho * 0.34f,
                   Tono::PEZUNA_R, Tono::PEZUNA_G, Tono::PEZUNA_B);
    }
}

// ----------------------------------------------------------------------------
// EL PELO
// ----------------------------------------------------------------------------
// Mechones de cerda como laminas finas. No es un sistema de pelo real: son
// piezas planas orientadas, que es lo que cabe en el presupuesto de un motor
// de vóxeles y basta para que se lea como pelaje.
inline void ConstruirPelo(std::vector<PiezaCuerpo>& salida,
                          const NodoHueso& tronco,
                          const NodoHueso& cabeza,
                          const PosePecari& pose) {
    using namespace Pelo;

    const float largoCerdaPx = CERDA_LARGO_M / Px::METROS_POR_PX;   // ~5.1 px

    // --- MELENA DORSAL ---
    // De la grupa a la coronilla. Se eriza con `erizado`, y erizada el animal
    // parece mucho mas grande: es senalizacion de alarma, no abrigo.
    const float alzado = 1.0f + pose.erizado * 2.5f;

    for (int i = 0; i < MECHONES_LOMO; ++i) {
        const float t = (float)i / (float)(MECHONES_LOMO - 1);
        const float zPx = (0.5f - t) * Px::TORSO_LARGO_PX * 0.98f;

        // Patron AGUTI: cada mechon con su tono, para el jaspeado.
        const float ruido = RuidoPelo(pose.semilla, i);
        const float tono = 1.0f - VARIACION_TONO * 0.5f + ruido * VARIACION_TONO;

        // La raya dorsal es mas oscura que el resto del cuerpo (MEDIDO).
        const float largo = largoCerdaPx * alzado * (0.75f + ruido * 0.5f);

        EmitirCaja(salida, tronco, pose,
                   0.0f,
                   Px::TORSO_ALTO_PX * 0.5f + largo * 0.4f,
                   zPx,
                   1.15f, largo * 0.5f, 1.7f,
                   Tono::LOMO_R * tono, Tono::LOMO_G * tono, Tono::LOMO_B * tono,
                   true);
    }

    // --- LA MELENA LLEGA A LA CORONILLA ---
    // MEDIDO y casi siempre olvidado en las representaciones: la crin no
    // empieza en el lomo, arranca en la cabeza.
    if (MELENA_LLEGA_A_LA_CORONILLA) {
        for (int i = 0; i < MECHONES_CUELLO; ++i) {
            const float t = (float)i / (float)MECHONES_CUELLO;
            const float ruido = RuidoPelo(pose.semilla, 100 + i);
            const float tono = 1.0f - VARIACION_TONO * 0.5f + ruido * VARIACION_TONO;
            const float largo = largoCerdaPx * alzado * (0.6f + ruido * 0.4f);

            EmitirCaja(salida, cabeza, pose,
                       0.0f,
                       9.0f + largo * 0.35f,
                       -3.0f - t * 5.0f,
                       1.0f, largo * 0.45f, 1.5f,
                       Tono::LOMO_R * tono, Tono::LOMO_G * tono, Tono::LOMO_B * tono,
                       true);
        }
    }

    // --- FLANCOS ---
    // Cerdas mas cortas y pegadas, para que la silueta no sea un cubo liso.
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = (lado == 0) ? -1.0f : 1.0f;
        for (int i = 0; i < MECHONES_FLANCO; ++i) {
            const float t = (float)i / (float)(MECHONES_FLANCO - 1);
            const float zPx = (0.42f - t * 0.84f) * Px::TORSO_LARGO_PX;
            const int idx = 200 + lado * 50 + i;
            const float ruido = RuidoPelo(pose.semilla, idx);
            const float tono = 1.0f - VARIACION_TONO * 0.5f + ruido * VARIACION_TONO;
            const float largo = largoCerdaPx * (0.45f + ruido * 0.35f);

            EmitirCaja(salida, tronco, pose,
                       sx * (Px::TORSO_ANCHO_PX * 0.5f + largo * 0.30f),
                       Px::TORSO_ALTO_PX * 0.16f,
                       zPx,
                       largo * 0.34f, 2.4f, 2.0f,
                       Tono::CUERPO_R * tono, Tono::CUERPO_G * tono, Tono::CUERPO_B * tono,
                       true);
        }
    }
}

// ----------------------------------------------------------------------------
// EL CUERPO COMPLETO
// ----------------------------------------------------------------------------
inline void ConstruirCuerpoPecari(const PosePecari& poseEntrada,
                                  std::vector<PiezaCuerpo>& salida) {
    salida.clear();

    PosePecari pose = poseEntrada;
    // Los limites del cuello se aplican aqui, no en quien llama: asi es
    // imposible construir un pecari con la cabeza del reves.
    pose.giroCabezaY = LimitarGiroCuelloY(pose.giroCabezaY);
    pose.giroCabezaX = LimitarGiroCuelloX(pose.giroCabezaX);

    // Proporciones segun la edad. Una cria no es un adulto encogido: tiene la
    // cabeza proporcionalmente enorme y las patas larguiruchas.
    const ProporcionEdad prop = ProporcionesDe(pose.etapa);

    // La escala general de la etapa se combina con la que traiga la pose, para
    // que quien llame pueda seguir escalando por su cuenta si lo necesita.
    pose.escala *= prop.escalaGeneral;

    // === RAIZ ===
    // Altura del CENTRO del torso sobre el suelo, en metros.
    //
    // El centro esta a: largo de pata libre + media altura de torso.
    // Con 18 px de pata y 46 de torso: 18 + 23 = 41 px = 0.282 m.
    //
    // Comprobacion: 41 + 23 = 64 px = la cruz. Cuadra.
    // El nodo se construye directamente en METROS (los nodos siempre viven en
    // metros; solo los offsets que se le pasan a HijoDe van en px).
    const float alturaTroncoM = Px::aM(Px::PATA_LIBRE_PX +
                                       Px::TORSO_ALTO_PX * 0.5f);

    // === MOVIMIENTO CORPORAL ===
    //
    // La amplitud crece con la velocidad: parado, el cuerpo solo respira.
    const float intensidad = std::fmin(pose.rapidez / 1.1f, 1.5f);

    // El rebote vertical va al DOBLE de frecuencia que el paso: el cuerpo sube
    // una vez por cada par diagonal que empuja, y hay dos pares por ciclo.
    //
    // SOLO HACIA ARRIBA, nunca hacia abajo. Es lo que impide que las pezunas
    // se hundan en el suelo: las patas son rigidas y cuelgan del tronco, asi
    // que si el tronco baja, los pies atraviesan el terreno.
    //
    // Biologicamente tambien es lo correcto: la postura de reposo ES la
    // posicion baja del ciclo. Un cuadrupedo se IMPULSA hacia arriba al andar,
    // no se hunde por debajo de donde esta parado.
    const float reboteY = (0.5f + 0.5f * std::sin(pose.fasePaso * 2.0f)) *
                          Balanceo::VERTICAL_PX * intensidad;

    // Balanceo lateral y cabeceo, a la frecuencia del paso.
    const float roll  = std::sin(pose.fasePaso) * Balanceo::LATERAL_RAD * intensidad;
    const float pitch = std::cos(pose.fasePaso) * Balanceo::CABECEO_RAD * intensidad;

    // Serpenteo: el tronco no sigue una linea perfectamente recta.
    const float serpenteo = std::sin(pose.fasePaso * 0.5f) *
                            Balanceo::SERPENTEO_RAD * intensidad;

    // RESPIRACION. No depende del paso, asi que sigue con el animal quieto.
    const float respiracion = std::sin(pose.tiempoVivo *
                                       Balanceo::RESPIRACION_HZ * 6.28318531f);
    const float hincha = 1.0f + respiracion * Balanceo::RESPIRACION_ESCALA;

    // La RAIZ lleva el rebote y el serpenteo, pero NO el cabeceo.
    //
    // Motivo: las patas cuelgan de la raiz, y un cabeceo del tronco las
    // inclinaria tambien, metiendo las pezunas delanteras en el suelo. El
    // cabeceo se aplica solo al TRONCO visible y a lo que va encima, que es
    // donde de verdad se ve.
    NodoHueso raiz{ 0.0f,
                    alturaTroncoM + Px::aM(reboteY),
                    0.0f,
                    serpenteo,
                    0.0f };

    // Nodo aparte para lo que SI cabecea: tronco, cuello, cabeza y pelo.
    NodoHueso ejeCuerpo = raiz;
    ejeCuerpo.giroX = pitch;

    // === TRONCO: el barril ===
    // Ahora con las proporciones pedidas: 70 x 56 x 64 px.
    NodoHueso tronco = ejeCuerpo;

    // El torso se hincha al respirar. Solo en ancho y alto: los pulmones
    // expanden la caja toracica a lo ancho, no alargan el animal.
    EmitirCaja(salida, tronco, pose,
               0.0f, 0.0f, 0.0f,
               Px::TORSO_ANCHO_PX * 0.5f * hincha * prop.factorTorso,
               Px::TORSO_ALTO_PX * 0.5f * hincha * prop.factorTorso,
               Px::TORSO_LARGO_PX * 0.5f * prop.factorTorso,
               Tono::CUERPO_R, Tono::CUERPO_G, Tono::CUERPO_B);

    // El balanceo lateral se guarda para aplicarlo a las piezas: se usa mas
    // abajo como inclinacion extra del cuerpo.
    (void)roll;

    // --- EL COLLAR ---
    // MEDIDO y diagnostico: la banda clara que cruza el hombro y da nombre a
    // la especie.
    EmitirCaja(salida, tronco, pose,
               0.0f, 0.0f, Px::TORSO_LARGO_PX * 0.27f,
               Px::TORSO_ANCHO_PX * 0.51f,
               Px::TORSO_ALTO_PX * 0.51f,
               2.2f,
               Tono::COLLAR_R, Tono::COLLAR_G, Tono::COLLAR_B);

    // --- LA GLANDULA DE ALMIZCLE ---
    // MEDIDO: va sobre las ANCAS, no en mitad del lomo. Es la caracteristica
    // unica de la familia y la infraestructura de su vida social.
    EmitirCaja(salida, tronco, pose,
               0.0f,
               Px::TORSO_ALTO_PX * 0.46f,
               -Px::TORSO_LARGO_PX * 0.30f,
               3.1f, 1.1f, 4.3f,
               Tono::LOMO_R * 0.85f, Tono::LOMO_G * 0.85f, Tono::LOMO_B * 0.85f);

    // --- LA COLA ---
    // MEDIDO: 1.2 cm. Practicamente inexistente. Deliberadamente minuscula:
    // darle cola de cerdo seria el error mas visible del modelo.
    const float colaPx = 0.012f / Px::METROS_POR_PX;   // ~1.2 px
    EmitirCaja(salida, tronco, pose,
               0.0f,
               Px::TORSO_ALTO_PX * 0.38f,
               -Px::TORSO_LARGO_PX * 0.5f - colaPx * 0.5f,
               1.1f, 1.1f, colaPx * 0.5f,
               Tono::CUERPO_R, Tono::CUERPO_G, Tono::CUERPO_B);

    // === CUELLO ===
    // Corto y macizo. Es el que sostiene una cabeza proporcionalmente grande.
    //
    // El factor de cabeza se declara aqui porque el CUELLO tambien lo usa:
    // una cria con cabezon necesita un cuello a juego, o la union canta.
    const float fc = prop.factorCabeza;

    NodoHueso cuello = HijoDe(tronco,
        0.0f,
        Px::TORSO_ALTO_PX * 0.18f,
        Px::TORSO_LARGO_PX * 0.46f,
        pose.giroCabezaY * 0.35f,     // el cuello acompana un tercio del giro
        pose.giroCabezaX * 0.30f);

    EmitirCaja(salida, cuello, pose,
               0.0f, 0.0f, 3.0f,
               11.0f * fc, 11.5f * fc, 6.0f * fc,
               Tono::CUERPO_R * 0.95f, Tono::CUERPO_G * 0.95f, Tono::CUERPO_B * 0.95f);

    // --- EL COLLAR TAMBIEN CRUZA EL CUELLO ---
    //
    // Correccion de esta ronda: la banda clara no se queda en el hombro, sube
    // y rodea el CUELLO. Es literalmente lo que le da nombre a la especie, y
    // por eso las descripciones hablan de "un collar que cruza hombros y
    // cuello".
    //
    // Antes solo se pintaba la parte del tronco, con lo que el collar quedaba
    // cortado por abajo y no se leia como un collar.
    // confianza: MEDIDO (que rodea el cuello) / ESTIMADO (el ancho exacto)
    EmitirCaja(salida, cuello, pose,
               0.0f, 0.0f, -1.5f,
               11.3f * fc, 11.8f * fc, 2.6f * fc,
               Tono::COLLAR_R, Tono::COLLAR_G, Tono::COLLAR_B);

    // === CABEZA ===
    // AQUI ESTA LO QUE SE PIDIO: la cabeza cuelga del cuello y gira con su
    // propio angulo. El tronco NO se entera de que la cabeza se ha girado.
    NodoHueso cabeza = HijoDe(cuello,
        0.0f, 2.0f, 8.0f,
        pose.giroCabezaY * 0.65f,     // los otros dos tercios del giro
        pose.giroCabezaX * 0.70f);

    // Cabeza en cuna: ancha en el craneo.
    // El factorCabeza la agranda en las crias: es alometria real, el craneo
    // crece antes que el resto del cuerpo.
    EmitirCaja(salida, cabeza, pose,
               0.0f, 0.0f, 0.0f,
               13.6f * fc, 15.0f * fc, 19.3f * fc,
               Tono::CUERPO_R, Tono::CUERPO_G, Tono::CUERPO_B);

    // --- HOCICO ---
    // Mucho mas estrecho que el craneo: afilado ~0.55 (es el cociente de
    // anchos, no un numero elegido). Es el organo de trabajo: escarba y huele.
    //
    // En las crias el hocico es CORTO y se alarga con la edad: por eso lleva
    // su propio factor. Es lo que da a un lechon esa cara chata.
    const float fh = prop.factorHocico;
    EmitirCaja(salida, cabeza, pose,
               0.0f, -7.1f * fc, (19.3f * fc) + (7.1f * fh),
               7.4f * fh, 7.1f * fh, 7.1f * fh,
               Tono::HOCICO_R, Tono::HOCICO_G, Tono::HOCICO_B);

    // --- EL DISCO RINARIAL: la punta del hocico ---
    //
    // No es "la punta del morro" sin mas: es un DISCO CARTILAGINOSO
    // especializado, apoyado sobre un huesecillo propio (el PRENASAL), que
    // existe precisamente para escarbar sin deformarse.
    //
    // Es la herramienta con la que el animal remueve el suelo — lo que lo
    // convierte en ingeniero de ecosistemas — y la que detecta raices a 8 cm
    // bajo tierra. En un animal de vista pesima, este disco hace casi todo el
    // trabajo de percepcion.
    //
    // Detalle que lo separa del cerdo: los musculos que mueven el disco nacen
    // en puntos DISTINTOS del craneo, aunque la estructura sea convergente.
    //
    // Se dibuja como una losa achatada y mas clara, pegada al frente del
    // hocico: es plano y humedo, no una continuacion del morro.
    // confianza: MEDIDO (que existe y para que sirve) / ESTIMADO (dimensiones)
    EmitirCaja(salida, cabeza, pose,
               0.0f, -7.1f * fc, (19.3f * fc) + (13.4f * fh),
               6.2f * fh, 5.6f * fh, 1.5f * fh,
               Tono::HOCICO_R * 1.28f, Tono::HOCICO_G * 1.22f, Tono::HOCICO_B * 1.20f);

    // --- LOS OJOS ---
    // Laterales, no frontales: es una presa. Da vision casi panoramica a
    // costa de poca binocular. Y el parpadeo los cierra.
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = (lado == 0) ? -1.0f : 1.0f;

        // El parpado baja sobre el ojo. Con parpadeo=1 el ojo queda cerrado.
        const float aperturaY = Ojo::DIAMETRO_PX * 0.5f * (1.0f - pose.parpadeo);

        if (aperturaY > 0.05f) {
            // Globo ocular
            EmitirCaja(salida, cabeza, pose,
                       sx * Ojo::SEPARACION_X_PX * fc,
                       Ojo::ALTURA_PX * fc,
                       Ojo::ADELANTO_Z_PX * fc,
                       Ojo::DIAMETRO_PX * 0.5f * fc, aperturaY * fc, Ojo::DIAMETRO_PX * 0.5f * fc,
                       Tono::OJO_R, Tono::OJO_G, Tono::OJO_B);

            // Pupila HORIZONTAL, como el resto de ungulados presa. Va con la
            // franja visual horizontal MEDIDA en su retina.
            const float pupilaAlto = std::fmin(Ojo::PUPILA_ALTO_PX * 0.5f, aperturaY * 0.8f);
            EmitirCaja(salida, cabeza, pose,
                       sx * (Ojo::SEPARACION_X_PX + 0.9f) * fc,
                       Ojo::ALTURA_PX * fc,
                       Ojo::ADELANTO_Z_PX * fc,
                       Ojo::PUPILA_ANCHO_PX * 0.35f, pupilaAlto, Ojo::PUPILA_ANCHO_PX * 0.5f,
                       Tono::PUPILA_R, Tono::PUPILA_G, Tono::PUPILA_B);

            // Reflejo de la cornea. SIN emision propia: este animal NO tiene
            // tapetum lucidum (MEDIDO), asi que sus ojos no brillan de noche.
            // El brillo es reflejo, y solo se ve si hay luz que reflejar.
            EmitirCaja(salida, cabeza, pose,
                       sx * (Ojo::SEPARACION_X_PX + 1.15f) * fc,
                       (Ojo::ALTURA_PX + 0.5f) * fc,
                       (Ojo::ADELANTO_Z_PX + 0.7f) * fc,
                       0.35f, std::fmin(0.35f, aperturaY * 0.6f), 0.35f,
                       Tono::BRILLO_OJO_R, Tono::BRILLO_OJO_G, Tono::BRILLO_OJO_B,
                       false,
                       0.0f);   // <- emision CERO, a proposito
        } else {
            // Ojo cerrado: solo la linea del parpado.
            EmitirCaja(salida, cabeza, pose,
                       sx * Ojo::SEPARACION_X_PX * fc,
                       Ojo::ALTURA_PX * fc,
                       Ojo::ADELANTO_Z_PX * fc,
                       Ojo::DIAMETRO_PX * 0.5f * fc, 0.30f * fc, Ojo::DIAMETRO_PX * 0.5f * fc,
                       Tono::CUERPO_R * 0.8f, Tono::CUERPO_G * 0.8f, Tono::CUERPO_B * 0.8f);
        }
    }

    // --- OREJAS ---
    // MEDIDO: 6.5 cm. Pequenas y erguidas. El oido es su segundo sentido tras
    // el olfato, muy por delante de la vista.
    const float orejaPx = 0.065f / Px::METROS_POR_PX;   // ~6.6 px
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = (lado == 0) ? -1.0f : 1.0f;
        EmitirCaja(salida, cabeza, pose,
                   sx * 9.2f * fc, 14.0f * fc, -4.9f * fc,
                   3.6f * fc, orejaPx * 0.5f * fc, 2.1f * fc,
                   Tono::CUERPO_R * 0.88f, Tono::CUERPO_G * 0.88f, Tono::CUERPO_B * 0.88f);
    }

    // --- COLMILLOS ---
    // LA OTRA SENA DE IDENTIDAD, y la que mas se confunde. El jabali los tiene
    // CURVADOS HACIA ARRIBA; el pecari los tiene RECTOS y VERTICALES HACIA
    // ABAJO, pegados a la mandibula. Angulo MEDIDO: 0 grados.
    // AMPLIACION DE ESTA RONDA: en el pecari se usan al morder LOS CUATRO
    // colmillos, superiores E inferiores, y los cuatro estan implantados
    // VERTICALMENTE. En el cerdo no es asi.
    //
    // Y el autoafilado tiene un mecanismo concreto: la cara posterior del
    // colmillo INFERIOR carece casi por completo de esmalte, asi que se
    // desgasta contra la cara anterior esmaltada del SUPERIOR. El roce ocurre
    // en el movimiento vertical de masticacion (ortal), o sea tanto al abrir
    // como al cerrar.
    //
    // Efecto secundario notable: los colmillos encajan tan justos que el
    // animal NO PUEDE mover la mandibula de lado a lado con la boca cerrada.
    // Su masticacion es una bisagra pura, arriba y abajo.
    const float colmilloLargoPx = 0.038f / Px::METROS_POR_PX;
    for (int lado = 0; lado < 2; ++lado) {
        const float sx = (lado == 0) ? -1.0f : 1.0f;

        // SUPERIOR: baja desde el maxilar. Es el que se ve asomar.
        EmitirCaja(salida, cabeza, pose,
                   sx * 4.6f * fc,
                   (-12.0f * fc) - colmilloLargoPx * 0.5f,
                   14.7f * fc,
                   0.77f, colmilloLargoPx * 0.5f, 0.77f,
                   Tono::COLMILLO_R, Tono::COLMILLO_G, Tono::COLMILLO_B);

        // INFERIOR: sube desde la mandibula, justo por detras del superior,
        // que es donde ambos se rozan. Mas corto y casi oculto con la boca
        // cerrada.
        EmitirCaja(salida, cabeza, pose,
                   sx * 4.6f * fc,
                   (-13.2f * fc) - colmilloLargoPx * 0.30f,
                   13.1f * fc,
                   0.68f, colmilloLargoPx * 0.34f, 0.68f,
                   Tono::COLMILLO_R * 0.94f, Tono::COLMILLO_G * 0.94f,
                   Tono::COLMILLO_B * 0.94f);
    }

    // === LAS CUATRO PATAS ===
    // Marcha diagonal: la delantera izquierda va con la trasera derecha. Es el
    // patron real de un cuadrupedo al andar.
    constexpr float PI_F = 3.14159265f;
    const float zDelantera =  Px::TORSO_LARGO_PX * 0.33f;
    const float zTrasera   = -Px::TORSO_LARGO_PX * 0.35f;

    // Las patas cuelgan de la RAIZ, no del tronco cabeceante: si siguieran
    // al cabeceo, las pezunas delanteras se hundirian en el suelo.
    ConstruirPata(salida, raiz, pose, -1.0f, zDelantera, true,  0.0f, prop.factorPatas);
    ConstruirPata(salida, raiz, pose, +1.0f, zDelantera, true,  PI_F, prop.factorPatas);
    ConstruirPata(salida, raiz, pose, -1.0f, zTrasera,   false, PI_F, prop.factorPatas);
    ConstruirPata(salida, raiz, pose, +1.0f, zTrasera,   false, 0.0f, prop.factorPatas);

    // === EL PELO ===
    // Va al final para que quede por encima del cuerpo al dibujar.
    ConstruirPelo(salida, tronco, cabeza, pose);
}

} // namespace Fauna

#endif // PECARI_CUERPO_H
