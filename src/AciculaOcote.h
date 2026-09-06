#pragma once

#include "BlockType.h"
#include <cstdint>
#include <cmath>

// ============================================================================
// LAS ACICULAS DEL OCOTE: MECHONES, NO UN CUBO DE HOJAS
// ============================================================================
// La hoja del pino no es una lamina que tape una cara: es una AGUJA. Y no
// crece suelta, crece en manojos. Dibujarla como un cubo con textura de hoja
// era lo mas lejos posible de lo que se ve en el campo.
//
// ----------------------------------------------------------------------------
// LO QUE DICE LA BIOLOGIA, Y QUE ES LO QUE SE MODELA
// ----------------------------------------------------------------------------
// Pinus montezumae (ocote blanco):
//
//   ACICULAS de 20 a 35 cm de largo y apenas 1.3 mm de ancho, muy flexibles,
//   de borde finamente aserrado y punta aguda. En corte transversal son
//   TRIANGULARES, porque asi encajan unas con otras dentro del brote.
//
//   Se agrupan en FASCICULOS de 5 (a veces 3 o 6). Todas las de un mismo
//   paquete nacen juntas, abrazadas en la base por una vaina castaña -- la
//   "colita" del ocoxal.
//
//   Los fasciculos se reparten de forma RADIAL alrededor de la ramilla y se
//   concentran en las PUNTAS de las ramas. De ahi el aspecto de escobeton:
//   mechones tupidos, no una masa uniforme.
//
// Pinus leiophylla (ocote chino) sigue el mismo patron con aciculas mas finas
// y en manojos mas apretados, normalmente de 3 a 5.
//
// ----------------------------------------------------------------------------
// COMO SE TRADUCE A UN MOTOR DE VOXELES
// ----------------------------------------------------------------------------
// A escala del juego un bloque son 60 cm, asi que una acicula de 30 cm mide
// MEDIO BLOQUE. No caben agujas de una en una: lo que se dibuja es el
// FASCICULO -- el mechon entero -- como un sprite plano.
//
// Cada celda de hoja emite varios mechones repartidos en las tres direcciones
// del espacio, con inclinaciones distintas. Eso da:
//
//   - Volumen desde cualquier angulo, sin ser un cubo. Es el 2.5D clasico:
//     planos con textura que, cruzados, se leen como masa.
//   - Que se vea a traves. Un pino real deja pasar la luz entre los mechones;
//     un cubo opaco no.
//   - Que la copa no tenga "caras". Al mirar hacia arriba desde el tronco se
//     ve el ramaje entre las agujas, no un techo.
//
// LA VARIACION ES POR POSICION, no aleatoria: sale de un hash de la celda, asi
// que el mismo arbol se ve igual siempre y no hace falta guardar nada.
// ============================================================================

namespace Acicula {

// ¿Este bloque son aciculas de ocote?
//
// Cubre las cuatro variantes: las hojas sueltas de las dos especies y las dos
// celdas que ademas llevan la rama dentro.
// constexpr para que el static_assert de mas abajo pueda evaluarla en tiempo
// de compilacion; sigue siendo utilizable en runtime igual que antes.
constexpr bool esAciculaOcote(BlockType t) {
    return t == BLOCK_LEAVES_OCOTE      || t == BLOCK_LEAVES_OCOTE_RAMA ||
           t == BLOCK_LEAVES_OCOTE_CHINO|| t == BLOCK_LEAVES_OCOTE_CHINO_RAMA;
}

// ⚠️ LA COPIA DE BlockType.h NO PUEDE SEPARARSE DE ESTA.
//
// Alli vive `Acicula_esFollajeOcote`, identica a esta, porque BlockType.h la
// necesita para decidir con quien comparte celda una acicula (ver `combinar`)
// y no puede incluir este archivo: la dependencia va al reves y habria ciclo.
//
// Esto ata las dos: si alguien añade una variante de follaje en un sitio y no
// en el otro, el build PARA aqui en vez de dejar que la acicula nueva se funda
// en silencio con cualquier planta.
static_assert(esAciculaOcote(BLOCK_LEAVES_OCOTE) ==
              Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE) &&
              esAciculaOcote(BLOCK_LEAVES_OCOTE_RAMA) ==
              Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_RAMA) &&
              esAciculaOcote(BLOCK_LEAVES_OCOTE_CHINO) ==
              Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_CHINO) &&
              esAciculaOcote(BLOCK_LEAVES_OCOTE_CHINO_RAMA) ==
              Acicula_esFollajeOcote(BLOCK_LEAVES_OCOTE_CHINO_RAMA) &&
              // Y que ninguna de las dos se trague una hoja de otra especie.
              !Acicula_esFollajeOcote(BLOCK_LEAVES) &&
              !Acicula_esFollajeOcote(BLOCK_LEAVES_OYAMEL),
              "Acicula::esAciculaOcote y Acicula_esFollajeOcote (BlockType.h) "
              "se han separado: las dos tienen que reconocer las mismas "
              "variantes de follaje de ocote");

// ¿Y de cual de las dos especies?
inline bool esChino(BlockType t) {
    return t == BLOCK_LEAVES_OCOTE_CHINO || t == BLOCK_LEAVES_OCOTE_CHINO_RAMA;
}

// ----------------------------------------------------------------------------
// DATOS DE LA ESPECIE
// ----------------------------------------------------------------------------
struct Especie {
    int   mechones;    // cuantos fasciculos por celda
    float largo;       // largo del mechon, en fraccion de bloque
    float ancho;       // ancho del sprite del mechon
    float caida;       // cuanto cuelga la punta: las aciculas son FLEXIBLES
    float ladeo;       // cuanto se tuerce el sprite respecto a su eje (radianes)
};

inline Especie EspecieDe(BlockType t) {
    if (esChino(t)) {
        // Pinus leiophylla: acicula mas fina y manojo mas apretado, asi que
        // mas mechones y algo mas cortos. De ahi que su copa se lea mas tupida.
        return { 16, 0.62f, 0.46f, 0.20f, 0.42f };
    }
    // Pinus montezumae: la acicula larga (20-35 cm = 0.33 a 0.58 bloques) y
    // muy flexible, de ahi la caida marcada. Es el pino de los mechones
    // colgantes.
    //
    // ⭐ EL NUMERO SUBIO DE 6 A 14.
    //
    // Un fasciculo de esta especie son 5 aciculas, y de una ramilla salen
    // fasciculos cada pocos milimetros. Con 6 mechones por celda la copa se
    // leia como sprites CONTABLES: se distinguia uno a uno donde deberia
    // haber masa. Con 14 el ojo deja de contarlos y empieza a ver follaje,
    // que es lo que hace un pino visto de cerca.
    //
    // El coste esta acotado y es previsible: son quads sin estado, en el
    // mismo batch de la textura de hoja, y solo los emiten las celdas de
    // acicula. Ver la nota de MAX_MECHONES sobre por que el tope importa.
    return { 14, 0.78f, 0.52f, 0.30f, 0.55f };
}

// ----------------------------------------------------------------------------
// EL TOPE DE MECHONES POR CELDA
// ----------------------------------------------------------------------------
// El mesher reserva su array con este numero, asi que vive AQUI y no alli:
// tenerlo en los dos sitios es como se acaba con un buffer de 12 al que se le
// piden 16 y se escribe fuera.
//
// El margen sobre el maximo de la tabla (16 del chino) cubre la variacion por
// celda de MechonesDe, que suma hasta +2.
constexpr int MAX_MECHONES = 20;

// ----------------------------------------------------------------------------
// UN MECHON COLOCADO
// ----------------------------------------------------------------------------
// Lo que el mesher necesita para dibujarlo: de donde nace, hacia donde apunta
// y cuanto mide. El mesher lo convierte en dos quads cruzados.
struct Mechon {
    float ox, oy, oz;   // origen dentro del voxel (0..1)
    float dx, dy, dz;   // direccion en la que sale (unitaria)
    float largo;
    float ancho;

    // ⭐ EL LADEO: CUANTO SE TUERCE EL SPRITE SOBRE SU PROPIO EJE.
    //
    // Sin esto los dos quads de un mechon se cruzan siempre en la misma
    // posicion relativa a su direccion, asi que todos los mechones de la copa
    // presentan el plano al observador con el MISMO angulo. El ojo lo lee como
    // repeticion: una rejilla de cruces en vez de agujas sueltas.
    //
    // Un fasciculo real no esta alineado con nada -- la vaina lo suelta con el
    // giro que le toque. Este angulo (en radianes) es ese giro, y sale del hash
    // de la celda y del indice, asi que cada mechon del arbol esta ladeado de
    // forma distinta y el conjunto deja de verse ordenado.
    float ladeo;

    // La fase de su oscilacion, en radianes. Es lo que impide que la copa
    // entera lata al unisono como si fuera un solo objeto: cada mechon entra
    // en el ciclo de la brisa en un momento distinto. Ver DesplazarHoja.
    float fase;

    // ⭐ CUANTO SE AGARRA ESTE MECHON A LA MADERA. 0 = al aire, 1 = clavado.
    //
    // Sale de mirar si el mechon apunta hacia una rama o un tronco vecino, y
    // gobierna las dos mitades del comportamiento:
    //
    //   la FORMA  -> el que se agarra sale recto; el suelto se vence.
    //   el MOVIMIENTO -> el que se agarra apenas se mueve; el suelto ondea.
    //
    // Que las dos cosas salgan del MISMO numero es lo que hace que la copa se
    // vea coherente: la parte tiesa es exactamente la parte quieta, que es lo
    // que pasa en un arbol de verdad.
    float sujecion;
};

// ============================================================================
// COMO SE APOYA ESTA CELDA EN LA MADERA
// ============================================================================
// Un mechon no cuelga igual si tiene la rama pegada o si esta al aire. Donde la
// acicula NACE de la madera, arranca RECTA y tiesa -- la vaina la sujeta. Donde
// no hay nada que la sujete, se vence.
//
// Eso es lo que hace que una copa se lea como una estructura y no como una
// nube: el follaje esta TENSO junto al ramaje y BLANDO en los bordes, y la
// frontera entre las dos cosas dibuja por si sola donde estan las ramas.
//
// ----------------------------------------------------------------------------
// DE DONDE SALEN LAS MILES DE FORMAS
// ----------------------------------------------------------------------------
// La celda mira sus SEIS vecinas y se queda con un mapa de bits: que lados
// tienen madera. Son 2^6 = 64 vecindarios distintos, y cada uno reparte de otra
// manera lo rigido y lo blando dentro de la celda.
//
// Multiplicado por lo que ya variaba antes -- el numero de mechones (5 valores),
// el giro de la espiral y el ladeo de cada sprite, todo por hash de posicion --
// el numero de siluetas distintas que puede tomar una celda de follaje se va a
// las decenas de miles, sin un solo ID nuevo y sin tocar el guardado.
//
// El bit por lado, en el orden de VECINOS_MADERA (ver abajo).
enum LadoMadera : uint8_t {
    MADERA_ARRIBA  = 1 << 0,
    MADERA_ABAJO   = 1 << 1,
    MADERA_ESTE    = 1 << 2,
    MADERA_OESTE   = 1 << 3,
    MADERA_SUR     = 1 << 4,
    MADERA_NORTE   = 1 << 5
};

// Los desplazamientos, en el MISMO orden que los bits de arriba. Quien rellena
// el mapa (el mesher, que es el unico que sabe leer el mundo) recorre esta
// tabla, de modo que el orden solo esta escrito una vez.
static const int VECINOS_MADERA[6][3] = {
    { 0,  1,  0},   // arriba
    { 0, -1,  0},   // abajo
    { 1,  0,  0},   // este
    {-1,  0,  0},   // oeste
    { 0,  0,  1},   // sur
    { 0,  0, -1}    // norte
};

// ----------------------------------------------------------------------------
// LOS MECHONES DE ESTA CELDA
// ----------------------------------------------------------------------------
// Devuelve cuantos se han escrito en `salida`.
//
// ⭐ LA DISPOSICION ES RADIAL, que es la palabra que usa la descripcion
// botanica: los fasciculos salen en todas las direcciones alrededor de la
// ramilla, no solo hacia arriba ni solo hacia los lados.
//
// Se reparten sobre una esfera con la espiral de Fibonacci, que es la forma
// mas barata de repartir N puntos por una esfera sin que se amontonen en los
// polos. Encaja ademas con que la filotaxis del pino es helicoidal.
//
// `maderaAlrededor` es el mapa de bits de arriba: que lados de la celda tienen
// rama o tronco. Con 0 (follaje suelto en el aire) el resultado es el de antes.
inline int MechonesDe(BlockType tipo, int wx, int wy, int wz,
                      Mechon* salida, int maxSalida,
                      uint8_t maderaAlrededor = 0) {
    const Especie e = EspecieDe(tipo);

    unsigned h = (unsigned)(wx * 73856093) ^ (unsigned)(wy * 19349663) ^
                 (unsigned)(wz * 83492791);
    h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;

    // El numero varia por celda: una copa real no tiene la misma densidad en
    // todas partes. El rango es +-2 (antes +-1), que a estas cantidades se
    // nota como zonas mas y menos tupidas dentro de la misma copa.
    int n = e.mechones + (int)(h % 5u) - 2;
    if (n < 3) n = 3;
    if (n > maxSalida) n = maxSalida;

    // Angulo de oro: es lo que reparte los puntos sin patron visible.
    constexpr float ORO = 2.39996323f;

    for (int i = 0; i < n; ++i) {
        // --- Direccion: espiral de Fibonacci sobre la esfera ---
        // y va de casi +1 a casi -1, y el radio del anillo sale del seno.
        const float t = ((float)i + 0.5f) / (float)n;
        float dy = 1.0f - 2.0f * t;
        const float r = std::sqrt(1.0f - dy * dy);
        // El giro de partida depende de la celda: dos celdas vecinas no
        // reparten sus mechones igual.
        const float ang = ORO * (float)i + (float)(h % 628u) * 0.01f;
        float dx = std::cos(ang) * r;
        float dz = std::sin(ang) * r;

        // ====================================================================
        // ⭐ ¿ESTE MECHON NACE DE LA MADERA O ESTA AL AIRE?
        // ====================================================================
        // Se compara la direccion en la que sale con los lados que tienen rama
        // o tronco. Un mechon que apunta HACIA la madera nace de ella: esta
        // sujeto. Uno que apunta al aire, no.
        //
        // El producto escalar da directamente cuanto se parecen las dos
        // direcciones: 1 si apunta justo a ese lado, 0 si va perpendicular.
        // Quedarse con el MAYOR es preguntar "¿cual es la madera a la que mas
        // se arrima este mechon?".
        float apoyo = 0.0f;
        if (maderaAlrededor) {
            for (int L = 0; L < 6; ++L) {
                if (!(maderaAlrededor & (1u << L))) continue;
                const float px = (float)VECINOS_MADERA[L][0];
                const float py = (float)VECINOS_MADERA[L][1];
                const float pz = (float)VECINOS_MADERA[L][2];
                const float d = dx * px + dy * py + dz * pz;
                if (d > apoyo) apoyo = d;
            }
        }

        // ⭐ SESGO HACIA ABAJO: LA ACICULA CUELGA... SALVO DONDE SE AGARRA.
        //
        // Miden hasta 35 cm y son "muy flexibles": no se sostienen rectas, se
        // vencen. Sin esto los mechones salen como un erizo rigido, que es
        // justo el aspecto que NO tiene un pino.
        //
        // Pero la caida se ANULA en el mechon que nace de la madera. Es lo
        // pedido, y es lo que se ve en el arbol: la aguja pegada a la ramilla
        // sale tiesa, y solo se dobla segun se aleja del punto de agarre. Por
        // eso una copa tiene el follaje TENSO donde hay rama y BLANDO en el
        // aire, y no una caida uniforme por todas partes.
        //
        // `apoyo` va de 0 (al aire, cae entera) a 1 (pegado a la madera, no
        // cae nada).
        dy -= e.caida * (1.0f - apoyo);
        const float len = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (len > 1e-4f) { dx /= len; dy /= len; dz /= len; }

        // --- Origen: cerca del centro, no todos en el mismo punto ---
        // Un fasciculo nace de la ramilla, y la ramilla ocupa el eje de la
        // celda. Se dispersan un poco para que no salgan todos del mismo sitio.
        unsigned ho = h ^ (unsigned)((i + 1) * 2654435761u);
        ho ^= ho >> 13; ho *= 1274126177u; ho ^= ho >> 16;
        const float disp = 0.16f;
        Mechon& m = salida[i];
        m.ox = 0.5f + ((float)(ho        % 100u) / 100.0f - 0.5f) * disp;
        m.oy = 0.5f + ((float)((ho >> 7) % 100u) / 100.0f - 0.5f) * disp;
        m.oz = 0.5f + ((float)((ho >> 14)% 100u) / 100.0f - 0.5f) * disp;

        m.dx = dx; m.dy = dy; m.dz = dz;

        // Largo y ancho con su variacion: los mechones no son clones.
        const float f = 0.75f + (float)((ho >> 21) % 50u) / 100.0f;
        m.largo = e.largo * f;
        m.ancho = e.ancho * (0.85f + (float)((ho >> 26) % 30u) / 100.0f);

        // ⭐ EL LADEO, CON SIGNO Y CENTRADO EN CERO.
        //
        // Va de -ladeo a +ladeo. Que sea SIMETRICO importa: un rango que solo
        // fuera positivo torceria todos los mechones hacia el mismo lado y la
        // copa entera quedaria peinada, que es otra forma del mismo patron
        // visible que esto viene a romper.
        //
        // Se toma un tramo del hash que no use ningun otro campo (los de
        // arriba gastan hasta el bit 31 en trozos de 5 a 7 bits), asi que el
        // ladeo no queda correlacionado con el largo ni con el ancho.
        const float giro01 = (float)((ho >> 3) % 200u) / 200.0f;   // 0..1
        m.ladeo = (giro01 - 0.5f) * 2.0f * e.ladeo;

        // La fase propia del mechon para la brisa: dos mechones vecinos no
        // pueden oscilar a la vez o la copa entera latiria como un solo objeto.
        m.fase = (float)((ho >> 11) % 628u) * 0.01f;   // 0..2*pi

        // Lo que ya se calculo para enderezarlo sirve tambien para frenarlo:
        // el mechon que nace de la madera ni se dobla ni se mueve.
        m.sujecion = apoyo;
    }
    return n;
}

// ============================================================================
// LA FISICA DE LA HOJA
// ============================================================================
// Una acicula de 30 cm y 1.3 mm de grosor es, mecanicamente, una VIGA EN
// VOLADIZO: empotrada por la base en la vaina y libre por la punta. Eso tiene
// una consecuencia que se ve a simple vista y es lo que se pidio:
//
//     LA BASE NO SE MUEVE. LA PUNTA SE MUEVE MUCHO.
//
// No es una convencion artistica, es como flexiona una viga empotrada: el
// desplazamiento crece con la distancia al empotramiento, y lo hace mas deprisa
// que la propia distancia. Para una carga en el extremo la curva elastica va
// como una cubica; aqui se usa una CUADRATICA (t^2), que tiene la misma forma
// --plana en la base, cada vez mas abierta hacia la punta-- y cuesta una
// multiplicacion.
//
// ----------------------------------------------------------------------------
// POR QUE NO ES UNA SIMULACION CON ESTADO
// ----------------------------------------------------------------------------
// Guardar velocidad y posicion por hoja significaria estado por vertice: cientos
// de miles de floats que ademas habria que integrar cada frame y que no cabrian
// en el presupuesto. Y no hace falta.
//
// Lo que el ojo lee como "la rama reacciona" son dos cosas:
//   1. que se aparte cuando algo la toca, y
//   2. que vuelva sola cuando se va.
//
// Las dos salen de una funcion PURA de (posicion, tiempo, donde esta el
// jugador). Sin estado, sin integracion, sin memoria: el mismo instante da
// siempre el mismo resultado, asi que no puede desincronizarse ni acumular
// error, y cuesta lo mismo con una hoja que con un millon.
// ============================================================================

// Hasta donde llega el empuje del jugador, en bloques. Es su "radio de
// aplastamiento": lo que su cuerpo aparta al pasar.
//
// El jugador mide 0.6 de ancho, asi que 1.6 cubre su cuerpo mas el brazo de
// hoja que se dobla alrededor. Mas que esto y las hojas se apartan antes de
// que llegue, lo que se lee como si le tuvieran miedo en vez de como contacto.
constexpr float RADIO_EMPUJE = 1.6f;

// Cuanto se aparta como maximo la punta, en bloques. Media acicula: se ve
// claramente que cede, pero no tanto como para dejar el hueco vacio.
constexpr float EMPUJE_MAX = 0.42f;

// Amplitud de la brisa de fondo, en bloques. Muy pequena a proposito: es el
// temblor de una copa en calma, no un vendaval.
constexpr float BRISA = 0.045f;

// ----------------------------------------------------------------------------
// EL DESPLAZAMIENTO DE UN PUNTO DE LA HOJA
// ----------------------------------------------------------------------------
// px,py,pz  el punto, en coordenadas de MUNDO
// t01       0 en la base del mechon (pegada a la rama), 1 en la punta
// fase      la fase propia del mechon, para que no oscilen todos a la vez
// tiempo    reloj del juego, en segundos
// jx,jy,jz  donde esta el jugador
// dx,dy,dz  SALIDA: cuanto se desplaza el punto
//
// Es una funcion pura y barata: sin ramas caras, sin memoria, sin locks. Se
// llama por vertice desde el mesher.
// `sujecion` es cuanto se agarra el mechon a la madera (0 al aire, 1 clavado a
// una rama). Un mechon sujeto se mueve mucho menos: es lo que hace que, en una
// rama con hojas encima, se muevan las que quedan libres y no las del punto de
// union.
inline void DesplazarHoja(float px, float py, float pz,
                          float t01, float fase, float tiempo,
                          float jx, float jy, float jz,
                          float& dx, float& dy, float& dz,
                          float sujecion = 0.0f) {
    dx = dy = dz = 0.0f;

    // ⭐ EL PERFIL DE VIGA EN VOLADIZO. ESTA ES LA LINEA QUE IMPORTA.
    //
    // Todo lo que sigue se multiplica por esto, asi que la base (t01=0) queda
    // CLAVADA pase lo que pase --ni la brisa ni el jugador pueden moverla-- y
    // la punta (t01=1) se lleva el desplazamiento entero.
    //
    // Al ser cuadratica, a media hoja el movimiento es solo la CUARTA parte
    // del de la punta, no la mitad. Por eso se ve doblarse y no trasladarse.
    float rigidez = t01 * t01;

    // ⭐ Y LA SEGUNDA MITAD DE LO MISMO: EL PUNTO DE UNION NO SE MUEVE.
    //
    // La rigidez del voladizo dice que la BASE de cada mechon esta quieta. Esto
    // dice ademas que los mechones ANCLADOS A LA MADERA estan quietos ENTEROS,
    // punta incluida.
    //
    // Es exactamente el comportamiento pedido: en una rama con follaje encima,
    // lo que se mueve son las hojas que cuelgan libres --las esquinas, los
    // bordes, lo que no llego a conectar-- mientras que las del punto de union
    // con la rama se quedan firmes. La frontera entre lo que ondea y lo que no
    // dibuja sola donde esta el ramaje.
    //
    // No se anula del todo (queda un 15% incluso clavado) porque una aguja
    // sujeta por la vaina aun vibra un poco; congelarla del todo se ve rigido.
    if (sujecion > 0.0f) {
        const float s = (sujecion > 1.0f) ? 1.0f : sujecion;
        rigidez *= (1.0f - 0.85f * s);
    }

    if (rigidez < 0.001f) return;     // la base no se mueve: nada que calcular

    // ------------------------------------------------------------------------
    // 1. LA BRISA
    // ------------------------------------------------------------------------
    // Dos senos de periodo distinto y no multiplo uno del otro, asi que la suma
    // no se repite a simple vista. La posicion entra en la fase para que la
    // copa ondule por zonas en vez de moverse en bloque.
    const float w1 = tiempo * 1.7f + fase + px * 0.35f + pz * 0.27f;
    const float w2 = tiempo * 1.1f + fase * 1.6f + pz * 0.31f;
    dx += (std::sin(w1) + 0.6f * std::sin(w2)) * BRISA * rigidez;
    dz += (std::cos(w1 * 0.9f) + 0.6f * std::cos(w2 * 1.3f)) * BRISA * rigidez;
    // El vaiven vertical es la mitad: una hoja que cuelga se balancea mas de
    // lado que arriba y abajo.
    dy += std::sin(w1 * 1.3f) * BRISA * 0.5f * rigidez;

    // ------------------------------------------------------------------------
    // 2. EL JUGADOR APARTA LA HOJA
    // ------------------------------------------------------------------------
    // Se mide contra el EJE del jugador (distancia horizontal), no contra un
    // punto: su cuerpo es una columna, y una hoja a la altura de la rodilla
    // tiene que apartarse igual que una a la altura de la cara.
    const float ex = px - jx;
    const float ez = pz - jz;
    const float d2 = ex * ex + ez * ez;

    if (d2 >= RADIO_EMPUJE * RADIO_EMPUJE) return;   // fuera de alcance

    // Solo dentro del alto del cuerpo, con un margen. Sin esto, pasar por
    // debajo de una copa la agitaria entera desde 10 bloques mas abajo.
    const float ey = py - jy;
    if (ey < -1.2f || ey > 2.6f) return;

    const float d = std::sqrt(d2);

    // ⭐ CAIDA SUAVE HACIA EL BORDE, NO UN ESCALON.
    //
    // `f` vale 1 pegado al jugador y 0 justo en el radio, con la pendiente
    // plana en los dos extremos (es un smoothstep). Un corte duro se veria
    // como un salto: la hoja pasaria de apartada a normal en un frame.
    const float u = 1.0f - d / RADIO_EMPUJE;         // 0..1
    const float f = u * u * (3.0f - 2.0f * u);       // smoothstep

    // Empuje RADIAL: la hoja se aparta alejandose del jugador, que es la
    // direccion en la que la empujaria un cuerpo que avanza contra ella.
    const float inv = (d > 1e-3f) ? (1.0f / d) : 0.0f;
    const float fuerza = f * EMPUJE_MAX * rigidez;

    dx += ex * inv * fuerza;
    dz += ez * inv * fuerza;
    // Y cede un poco hacia abajo: una rama apartada se vence, no flota.
    dy -= fuerza * 0.30f;
}

} // namespace Acicula
