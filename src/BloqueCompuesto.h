#pragma once

#include "BlockType.h"
#include <cstdint>

// ============================================================================
// BLOQUES COMPUESTOS: VARIAS COSAS Y UN ESTADO DENTRO DE UN SOLO VOXEL
// ============================================================================
// Un voxel del motor guarda UN BlockType y nada mas: no hay metadatos, ni
// tile entities, ni una tabla lateral. Eso hacia imposible un maguey que
// "tiene 7 de 15 de aguamiel, esta en la etapa 4 y le quedan tres puntas".
//
// Este archivo resuelve eso SIN tocar el formato de guardado, apoyandose en un
// hecho del motor: BlockType se serializa como 4 BYTES (32 bits) y solo se
// usan los primeros ~168 valores. Sobran miles de millones.
//
// Asi que el estado se mete DENTRO del ID:
//
//     ID = COMPUESTO_BASE + familia * ESTADOS_POR_FAMILIA + estado
//
// Es la misma tecnica que el motor YA usa para las celdas mixtas
// (BLOCK_MIXTO_BASE + base*7*FAMILIAS + relleno), aqui generalizada.
//
// ----------------------------------------------------------------------------
// LO QUE SE GANA
// ----------------------------------------------------------------------------
//   - PERSISTENCIA GRATIS. El estado viaja en el ID, asi que el save, la
//     paleta del chunk y la carga funcionan sin cambiar una linea. No hay
//     formato nuevo que versionar ni migracion que escribir.
//   - CERO MEMORIA EXTRA. No hay mapa lateral que llenar ni purgar al
//     descargar un chunk (una fuga que el motor ya sufre con waterLevels).
//   - CERO ENTIDADES. Las puntas de un maguey no son objetos: son un campo
//     de bits que el mesher lee al construir la malla.
//
// ----------------------------------------------------------------------------
// LO QUE CUESTA
// ----------------------------------------------------------------------------
//   - Los campos son ACOTADOS: lo que quepa en sus bits. Para "cuanta
//     aguamiel" o "que etapa de crecimiento" sobra; para un float con
//     decimales, no. Cuando haga falta un campo continuo, se cuantiza.
//
// ----------------------------------------------------------------------------
// GENERICO, NO DEL MAGUEY
// ----------------------------------------------------------------------------
// El maguey es la primera familia, no el motivo del diseño. La misma
// estructura vale para arbol+frutos, piedra+musgo, tronco+hongos o
// planta+flores: cada una declara sus componentes y el reparto de bits de su
// estado, y hereda gratis el raycast por componente, el guardado y el render.
// ============================================================================

namespace Compuesto {

// ----------------------------------------------------------------------------
// EL ESPACIO DE IDs
// ----------------------------------------------------------------------------
// Va MUY por encima del enum y del rango de las celdas mixtas, para que no
// pueda pisar un bloque real ni ahora ni cuando el enum crezca.
//
// Las mixtas ocupan [1000, 1000 + 22*154) = [1000, 4388). Se arranca en
// 100.000 con holgura de sobra.
constexpr int COMPUESTO_BASE = 100000;

// BlockType.h necesita saber donde empieza este rango (para clasificar los
// compuestos como planta) pero no puede preguntarlo: la dependencia va al
// reves. Alli hay una copia del numero, y esto garantiza que las dos no se
// separen jamas -- si alguien mueve una, el build falla aqui en vez de
// corromper mundos en silencio.
static_assert(COMPUESTO_BASE == BLOQUE_COMPUESTO_BASE_ID,
              "COMPUESTO_BASE y BLOQUE_COMPUESTO_BASE_ID tienen que coincidir");

// Cuantos estados distintos puede tener una familia. 16 bits = 65.536
// combinaciones, que es lo que dan los campos de abajo con margen para crecer.
constexpr int BITS_ESTADO        = 16;
constexpr int ESTADOS_POR_FAMILIA = 1 << BITS_ESTADO;

// Familias registradas. Añadir una es añadir una linea AL FINAL: insertar en
// medio correria los IDs y los mundos guardados leerian otra planta.
enum Familia : int {
    FAM_MAGUEY = 0,     // cuerpo + puntas + aguamiel

    // --- Preparadas, aun sin implementar ---
    // Se declaran ya para reservarles su hueco de IDs: hacerlo despues
    // obligaria a insertar en medio, que es justo lo que corrompe saves.
    FAM_ARBOL_FRUTO,    // tronco/rama + frutos
    FAM_PIEDRA_MUSGO,   // roca + musgo
    FAM_PLANTA_FLOR,    // mata + flores

    // La BIZNAGA: cactus de barril del desierto mexicano. Crece por etapas,
    // como el maguey, pero es una bola y no una roseta.
    FAM_BIZNAGA,

    // El AGUA con volumen. No es una planta: es la primera familia que no lo
    // es, y por eso BlockType.h tiene que mirar la familia antes de dar por
    // hecho que todo compuesto es vegetacion (ver esOrganicoParaHacha).
    FAM_AGUA,

    // El AGAVE TEQUILANA AZUL. Es una familia APARTE y no una etapa mas del
    // maguey pulquero, por dos razones:
    //
    //   1. NO CABE. Los 16 bits del maguey estan repartidos hasta el bit 14
    //      (SEGMENTO = {13,2}); solo queda el 15 libre, y esta especie
    //      necesita al menos dos campos nuevos (fase del ciclo y altura del
    //      quiote). Meterlo con calzador obligaria a subir BITS_ESTADO, lo
    //      que corre los IDs de TODAS las familias siguientes -- justo lo
    //      que este archivo prohibe porque corrompe mundos guardados.
    //
    //   2. ES OTRA PLANTA. El tequilana tiene silueta, color y ciclo de vida
    //      distintos del pulquero: hojas rigidas y erectas en vez de
    //      arqueadas, tono azul plateado por la cera, y termina en jima o en
    //      quiote. Compartir estado con el pulquero obligaria a preguntar
    //      "de que especie eres" en cada calculo del mesher.
    //
    // Al ser familia propia estrena 16 bits limpios y no toca un solo bit de
    // lo que ya esta guardado en disco.
    FAM_AGAVE_AZUL,

    // El AGAVE ABISAII. Roseta masiva de hojas anchas y arqueadas, sembrada
    // en los afloramientos de roca de las faldas de montana.
    //
    // Familia propia por las MISMAS DOS RAZONES que el tequilana, y conviene
    // comprobarlas en vez de heredarlas:
    //
    //   1. NO CABE en el tequilana. Alli los 16 bits llegan hasta el bit 14
    //      (FLORECIDO) y solo queda el 15 libre; esta especie necesita al
    //      menos un campo de etapa propio. Y subir BITS_ESTADO correria los
    //      IDs de TODAS las familias, que es lo que corrompe saves.
    //
    //   2. ES OTRA PLANTA. El abisaii tiene la hoja ANCHA y ARQUEADA --se
    //      vence por su propio peso-- frente a la hoja rigida y erecta del
    //      tequilana. Compartir estado obligaria a preguntar "de que especie
    //      eres" en cada calculo del mesher, que es justo lo que este
    //      archivo evita.
    //
    // ⚠️ VA AL FINAL, despues de FAM_AGAVE_AZUL. Insertarla en medio correria
    // los IDs de las familias siguientes y los mundos guardados leerian otra
    // planta en su sitio.
    FAM_AGAVE_ABISAII,

    // ========================================================================
    // EL NOPAL DE CASTILLA: UNA MATA DE PENCAS EN UNA SOLA CELDA
    // ========================================================================
    // PEDIDO: "que el nopal permita tener 3 o 4 pencas mas en su misma casilla
    // en cualquier direccion o posicion -- encima, abajo, al lado, este, sur,
    // norte, oeste, mil formas; y que el tallo se conecte con las pencas y haga
    // una especie de rollo, y alrededor crezcan las pencas sueltas".
    //
    // ------------------------------------------------------------------------
    // POR QUE NO SERVIA EL SISTEMA VIEJO
    // ------------------------------------------------------------------------
    // El apilado de pencas vivia en IDs DEL ENUM: CLADODIO_X2, X3, Z2, Z3. Eso
    // da un tope de 3 pencas y solo en 2 ejes, porque cada combinacion necesita
    // su propia entrada. Para "cualquier direccion" harian falta cientos de
    // entradas nuevas, y cada una insertada en medio corre los IDs de todo lo
    // que va detras -- que es justo lo que este archivo existe para evitar.
    //
    // ------------------------------------------------------------------------
    // EL REPARTO DE LOS 16 BITS, Y POR QUE ES ASI
    // ------------------------------------------------------------------------
    // La clave es NO guardar la geometria. Se guardan las DECISIONES (cuantas
    // pencas, hacia donde, que semilla) y la FORMA se deriva por hash de la
    // posicion, exactamente como hace CristalMineral.h con los cristales.
    //
    //     EXTRAS   3b   0..7    cuantas pencas extra cuelgan de esta celda
    //     DIR_A    3b   0..5    direccion de la 1a extra (las 6 caras)
    //     DIR_B    3b   0..5    direccion de la 2a
    //     DIR_C    3b   0..5    direccion de la 3a
    //     SEMILLA  3b   0..7    variacion de angulos y desplazamientos
    //     ROLLO    1b   0/1     el tallo se enrolla con las pencas
    //                 ----
    //                  16 bits
    //
    // Tres direcciones explicitas y el resto por hash: con 4 o mas extras, las
    // que pasan de la tercera toman su rumbo del hash de (posicion, indice).
    // Es lo que permite "mil formas" sin gastar 3 bits por cada una.
    //
    // Cuentas: 8 semillas x 6^3 direcciones x 2 rollos = 3.456 combinaciones
    // guardadas, y cada una se despliega en angulos distintos segun DONDE este
    // la planta. Dos nopales con el mismo estado en casillas distintas no se
    // ven iguales.
    //
    // ⚠️ VA AL FINAL, despues de FAM_AGAVE_ABISAII. Misma regla de siempre.
    FAM_NOPAL,

    FAM_COUNT
};

constexpr int COMPUESTO_FIN =
    COMPUESTO_BASE + FAM_COUNT * ESTADOS_POR_FAMILIA;

// Las otras dos copias que BlockType.h necesita para reconocer el agua sin
// poder incluir este archivo. Misma red que el static_assert de arriba: si
// alguien mueve FAM_AGUA de sitio o cambia el ancho del estado, el build para
// aqui en vez de dejar que el motor confunda agua con vegetacion.
static_assert(ESTADOS_POR_FAMILIA == BLOQUE_COMPUESTO_ESTADOS,
              "ESTADOS_POR_FAMILIA y BLOQUE_COMPUESTO_ESTADOS tienen que coincidir");
static_assert((int)FAM_AGUA == BLOQUE_COMPUESTO_FAM_AGUA,
              "FAM_AGUA cambio de indice: actualiza BLOQUE_COMPUESTO_FAM_AGUA "
              "en BlockType.h o el motor tratara el agua como una planta");

// ¿Este ID es un bloque compuesto?
inline bool esCompuesto(BlockType t) {
    return (int)t >= COMPUESTO_BASE && (int)t < COMPUESTO_FIN;
}

// Componer y descomponer.
inline BlockType hacer(Familia f, uint16_t estado) {
    return (BlockType)(COMPUESTO_BASE + (int)f * ESTADOS_POR_FAMILIA + estado);
}
inline Familia familiaDe(BlockType t) {
    if (!esCompuesto(t)) return FAM_COUNT;
    return (Familia)(((int)t - COMPUESTO_BASE) / ESTADOS_POR_FAMILIA);
}
inline uint16_t estadoDe(BlockType t) {
    if (!esCompuesto(t)) return 0;
    return (uint16_t)(((int)t - COMPUESTO_BASE) % ESTADOS_POR_FAMILIA);
}

// ----------------------------------------------------------------------------
// EMPAQUETADO DE CAMPOS
// ----------------------------------------------------------------------------
// Un campo es (desplazamiento, ancho). Leer y escribir pasa SIEMPRE por estas
// dos funciones para que ningun sitio del motor se invente su propia
// aritmetica de bits -- que es como se acaba con dos versiones que no
// coinciden.
struct Campo {
    uint8_t desplaz;   // desde que bit
    uint8_t ancho;     // cuantos bits

    constexpr uint16_t mascara() const {
        return (uint16_t)(((1u << ancho) - 1u) << desplaz);
    }
    constexpr uint16_t maximo() const {
        return (uint16_t)((1u << ancho) - 1u);
    }
};

inline uint16_t leer(uint16_t estado, Campo c) {
    return (uint16_t)((estado & c.mascara()) >> c.desplaz);
}

inline uint16_t escribir(uint16_t estado, Campo c, uint16_t valor) {
    if (valor > c.maximo()) valor = c.maximo();   // se acota, no se desborda
    return (uint16_t)((estado & ~c.mascara()) |
                      ((uint16_t)(valor << c.desplaz) & c.mascara()));
}

// ============================================================================
// FAMILIA: MAGUEY
// ============================================================================
// Reparto de los 16 bits del estado. El total no puede pasar de 16; si algun
// dia hace falta mas, se sube BITS_ESTADO (y con el ESTADOS_POR_FAMILIA), lo
// que corre los IDs de las familias siguientes -- asi que hacerlo exige
// migrar, igual que insertar una familia en medio.
namespace Maguey {

    // Etapa de crecimiento. Cinco etapas reales + margen.
    //   0 brote | 1 joven | 2 adulto | 3 maduro | 4 productor
    constexpr Campo ETAPA     = { 0, 3 };   // 0..7

    // Giro de la planta: cuatro orientaciones. Es lo que evita que un campo
    // de magueyes se vea como copias calcadas del mismo modelo.
    constexpr Campo GIRO      = { 3, 2 };   // 0..3

    // Puntas que le quedan. NO es un bloque por combinacion: es un contador,
    // y el mesher decide donde va cada una a partir del hash de la posicion.
    constexpr Campo PUNTAS    = { 5, 3 };   // 0..7

    // Aguamiel acumulada, en dieciseisavos de la capacidad. Sube GRADUALMENTE
    // con el tiempo; no aparece de golpe.
    constexpr Campo AGUAMIEL  = { 8, 4 };   // 0..15

    // ¿Esta capado? Un maguey sin capar no produce aunque sea maduro: hay que
    // abrirlo primero, que es como se hace de verdad.
    constexpr Campo CAPADO    = { 12, 1 };  // 0..1

    // ========================================================================
    // ⭐ EN QUE ALTURA DE LA PLANTA ESTA ESTA CELDA
    // ========================================================================
    // Un maguey de verdad no mide lo mismo de retoño que de viejo: los
    // maduros levantan varios metros. Para eso la planta ocupa VARIAS celdas
    // en vertical, y cada una tiene que saber que trozo le toca dibujar.
    //
    // 0 = la de abajo (donde esta la piña y donde se capa)
    // 1 = la siguiente, y asi hasta 3.
    //
    // ⚠️ NO SON PLANTAS DISTINTAS. Es UNA planta repartida en celdas: la de
    // abajo lleva el estado bueno (etapa, jugo, capado) y manda; las de
    // arriba solo prolongan la roseta hacia el cielo.
    //
    // Se guarda gratis, como todo lo demas: va dentro del ID.
    constexpr Campo SEGMENTO  = { 13, 2 };  // 0..3

    // Cuantas celdas de alto ocupa un maguey de cada etapa.
    //
    // De retoño a viejo, como en el campo: un brote no levanta del suelo y un
    // productor se ve desde lejos.
    inline int celdasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case 0:  return 1;   // BROTE:  el bebe, un bloque
            case 1:  return 2;   // JOVEN:  el mediano, dos bloques
            case 2:  return 3;   // ADULTO: el grande, tres bloques
            case 3:  return 4;   // MADURO: cuatro, ya se puede capar
            default: return 4;   // PRODUCTOR: cuatro, el que da aguamiel
        }
    }

    // --- Etapas con nombre, para no repartir numeros magicos por el motor ---
    enum Etapa : uint16_t {
        BROTE = 0, JOVEN = 1, ADULTO = 2, MADURO = 3, PRODUCTOR = 4
    };

    // Capacidad maxima de aguamiel segun la etapa. Un maguey joven no da
    // nada; solo el productor llena del todo.
    inline uint16_t capacidad(uint16_t etapa) {
        switch (etapa) {
            case PRODUCTOR: return 15;   // el tope del campo
            case MADURO:    return 8;
            default:        return 0;    // aun no produce
        }
    }

    // ¿Puede producir aguamiel ahora mismo?
    // Hacen falta las dos cosas: estar en edad Y estar capado.
    inline bool produce(uint16_t estado) {
        return leer(estado, CAPADO) != 0 &&
               capacidad(leer(estado, ETAPA)) > 0;
    }

    // ¿Hay bastante para llenar un tazon? Se pide la mitad de la capacidad de
    // un productor: menos que eso es un fondo que no merece la pena.
    constexpr uint16_t AGUAMIEL_PARA_TAZON = 8;

    inline bool hayParaTazon(uint16_t estado) {
        return leer(estado, AGUAMIEL) >= AGUAMIEL_PARA_TAZON;
    }

    // Cuantas puntas le tocan a cada etapa al generarse.
    //
    // ⭐ SOLO LOS GRANDES TIENEN PUNTA.
    //
    // El brote y el joven llevaban 2 y 3 espinas, y no debe ser asi: la
    // punta gruesa es lo que distingue a un maguey HECHO. Un maguey pequeno
    // con espinas se veia igual que uno adulto en miniatura, y ademas
    // rompia la regla de reconocerlos de un vistazo por la punta.
    //
    // Ahora la punta aparece a partir del ADULTO y va a mas con la edad.
    inline uint16_t puntasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case BROTE:  return 0;    // sin punta: es un brote
            case JOVEN:  return 0;    // sin punta todavia
            case ADULTO: return 5;
            default:     return 6;    // maduro y productor
        }
    }

    // Escala del modelo por etapa: lo que mide respecto a un maguey mediano.
    //
    // ⭐ LOS QUE DAN AGUAMIEL SON MUCHO MAS GRANDES.
    //
    // El salto entre ADULTO y MADURO es deliberadamente brusco (1.30 -> 2.60,
    // el doble) porque ahi esta la frontera que le importa al jugador: el
    // maduro es el primero que se puede capar. Antes la progresion era suave
    // (1.30 -> 1.80 -> 2.20) y desde lejos no se distinguia cual servia; habia
    // que acercarse a cada mata a probar.
    //
    // Ahora un maguey productivo se ve de lejos como lo que es: una planta
    // enorme entre matas pequeñas. Es la misma idea que hace reconocible un
    // agave adulto en el campo -- llega a los dos metros mientras los
    // hijuelos que lo rodean no levantan medio.
    // ⚠️ EL TOPE ES 0.86, Y NO ES ARBITRARIO.
    //
    // La roseta se dibuja con hojas de 9 px de largo por escala. La del
    // anillo exterior va casi tumbada y se abre 0,6375 del largo, asi que a
    // partir de escala 1.15 esas hojas SE SALEN DEL VOXEL e invaden la celda
    // de al lado -- se ven atravesando el terreno.
    //
    // Se descubrio persiguiendo un "bloque raro encima del maguey": no era un
    // bloque, era la punta de la roseta asomando hasta cinco voxels por
    // encima de su celda con las escalas anteriores (que llegaban a 3.20).
    //
    // Si algun dia hace falta un maguey mas grande, no basta con subir este
    // numero: hay que repartir la planta en VARIOS bloques, como hace el
    // nopal con su tallo y sus pencas.
    inline float escalaDeEtapa(uint16_t etapa) {
        // ⭐ DECISION FIRME: EL MAGUEY OCUPA UNA SOLA CELDA.
        //
        // No 3 niveles, no multi-bloque. La celda da la posicion; el modelo
        // 3D vive dentro de ella.
        //
        // Estas escalas NO son medidas en bloques: son un parametro que
        // alimenta altoHoja() y radioRoseta(), y las dos van ACOTADAS (0.96 y
        // 0.44). El acotado es la garantia dura de que nada se sale; estos
        // numeros solo reparten el tamaño entre etapas.
        //
        // El reparto llega hasta 1.90 a proposito: asi el PRODUCTOR satura
        // los topes y se ve claramente como la mata mas grande, mientras el
        // brote se queda pequeño. Se distinguen de un vistazo sin que ninguno
        // pise la celda vecina.
        switch (etapa) {
            case BROTE:     return 0.45f;   // recien nacido, ras de suelo
            case JOVEN:     return 0.70f;
            case ADULTO:    return 1.00f;   // la mata mediana de referencia
            case MADURO:    return 1.40f;   // ya se puede capar: se nota
            default:        return 1.90f;   // productor: llena su celda
        }
    }

    // ========================================================================
    // MEDIDAS DEL MODELO 3D
    // ========================================================================
    // Las usan TRES sistemas que tienen que coincidir al pixel: el mesher (lo
    // que se dibuja), cajaDePiezaN (lo que se selecciona) y nopalHitboxCon (lo
    // que frena al jugador). Estaban repetidas en los tres sitios, y esa es la
    // forma clasica de que una se cambie y las otras no: entonces apuntas a un
    // sitio y seleccionas otro.
    //
    // ⚠️ TODO SE ACOTA AL VOXEL. Un bloque no puede dibujar fuera de su celda:
    // se cruzaria con el vecino y se veria atravesando el terreno.
    //
    // El modelo se reescribio de cero: cada penca es una tira de cajas que
    // sube y se abre, y CADA CAJA se recorta al voxel antes de emitirse. Asi
    // la garantia es estructural y no depende de acertar los topes a mano --
    // que es lo que fallaba en la version anterior.
    //
    // Estas dos son las medidas maestras de las que sale todo lo demas.
    // Tienen que coincidir con las del mesher (buscar "ALTO" y "RADIO" en la
    // rama del maguey de buildChunkMesh).

    // ========================================================================
    // ⭐ LAS MEDIDAS REALES DE LA PLANTA
    // ========================================================================
    // Tomadas de la descripcion botanica del genero Agave, con la escala del
    // juego de 1 bloque = 1 metro:
    //
    //   Alto de la roseta ....... 1.5 - 3.0 m  -> 1.5 - 3.0 bloques
    //   Diametro (ancho) ........ 1.5 - 3.0 m  -> 1.5 - 3.0 bloques
    //   Penca ................... 1-2 m largo x 15-25 cm ancho
    //   Quiote (al florecer) .... 5 - 12 m
    //
    // ⚠️ EL DATO QUE MANDA: ALTO == DIAMETRO.
    //
    // La roseta es tan ANCHA como ALTA -- una media esfera, no una columna.
    // Es lo que el modelo tenia mal de raiz: crecia a lo alto (hasta 4
    // bloques) mientras el ancho seguia topado en media celda, asi que salia
    // un mastil de hojas en vez de una roseta abierta.
    //
    // Para que las proporciones sean las de la descripcion, la planta TIENE
    // que asomar de su celda a lo ancho. Es legitimo: el recorte del
    // dibujado es por CHUNK (16 bloques), no por celda, asi que una hoja que
    // sobresale medio bloque se dibuja sin problema. Lo que no puede salirse
    // es la COLISION, que si se consulta celda a celda -- y no hace falta:
    // el jugador choca con la piña, no con las hojas, igual que atraviesa la
    // hierba alta.

    // Radio maximo al que puede llegar una hoja, en bloques desde el centro.
    //
    // 1.40 da un diametro de 2.8 bloques, que es el ejemplar hecho de la
    // descripcion (1.5-3.0 m). Es el tope de seguridad, no la medida de uso:
    // quien manda es radioDeCeldas(), que lo saca de la altura.
    //
    // ⚠️ NO SUBIR DE 1.5. Media hoja mas alla y la planta empezaria a cruzar
    // el borde del CHUNK (16 bloques), donde el recorte del dibujado si
    // corta: se veria aparecer y desaparecer al girar la camara.
    //
    // Esta en 1.50 -- el limite documentado, no un numero al azar -- porque
    // este techo se reparte entre el arranque de la penca y su recorrido (ver
    // radioDeCeldas). Con 1.40 el radio util se quedaba en 1.045 y el MADURO
    // topaba en el mismo tamaño que el PRODUCTOR: dos etapas que deben
    // distinguirse de un vistazo se veian iguales.
    inline float radioMaximoReal() { return 1.50f; }

    // Hasta donde sube la planta.
    inline float altoHoja(float esc) {
        const float a = 0.14f + 0.62f * esc;
        return (a > 0.96f) ? 0.96f : a;
    }

    // Hasta donde se abre la roseta, medido desde el centro.
    //
    // ⚠️ EL TOPE ES 0.44 Y NO PUEDE SUBIR, y conviene saber por que antes de
    // tocarlo: el motor consulta la colision y el dibujado CELDA A CELDA, asi
    // que lo que se dibuje mas alla de 0.5 desde el centro invade la celda de
    // al lado y se ve atravesando el terreno del vecino.
    //
    // O sea: a lo ANCHO la planta no puede crecer mas que su columna. Lo que
    // si crece es lo demas -- la altura (varias celdas), el numero de pencas
    // y el GROSOR de cada una -- y eso es lo que hace que un ejemplar viejo
    // se vea macizo al lado de un brote.
    // ⭐ AHORA SIGUE A LA ALTURA, que es lo que dice la descripcion.
    //
    // Antes topaba en 0.44 (diametro 0.88, menos de UN bloque) mientras la
    // planta subia hasta 4. Proporcion real 4:1 en vez del 1:1 que toca.
    //
    // El radio se saca de las CELDAS que ocupa la planta, para que el
    // diametro y el alto vayan de la mano como en la mata de verdad.
    // ⚠️ ALTO Y DIAMETRO SALEN DEL MISMO NUMERO.
    //
    // Es la unica forma de garantizar el 1:1 que pide la descripcion. Si se
    // calculan por separado, cualquier tope que salte en uno de los dos
    // rompe la proporcion -- que es justo lo que pasaba: el alto subia con
    // las celdas mientras el radio se quedaba clavado en su maximo.
    //
    // `alturaReal` es la altura VISIBLE de la roseta, en bloques. De ahi
    // salen las dos medidas, asi que la planta siempre es tan ancha como
    // alta por construccion.
    inline float alturaReal(int celdas) {
        // El retoño no llega al bloque; el ejemplar viejo se acerca a tres,
        // que es el tope real del genero (1.5-3.0 m).
        switch (celdas) {
            case 1:  return 0.55f;   // retoño
            case 2:  return 1.30f;
            case 3:  return 2.10f;
            default: return 2.80f;   // ejemplar hecho
        }
    }

    // El radio es la MITAD de la altura: diametro == alto.
    //
    // ⚠️ LO QUE SE ACOTA ES EL ALCANCE, NO EL RADIO SUELTO.
    //
    // La penca no nace en el centro: arranca dentro de la piña, a 0.34*RADIO,
    // y desde ahi recorre otro RADIO. Su punta llega, pues, a 1.34*RADIO.
    //
    // Topando solo `r` se dejaba pasar un RADIO de 1.40 cuyo alcance real era
    // 1.88 -- por encima del techo. El mesher lo recortaba despues penca a
    // penca, y como el recorte era el mismo para todas, las de fuera se
    // quedaban clavadas en el mismo largo: la roseta perdia su abanico y
    // FALTABAN PENCAS justo en las matas grandes (16 de 26 en un PRODUCTOR),
    // mientras que en las pequeñas no faltaba ninguna.
    //
    // Acotando aqui el ALCANCE, el radio que sale ya cabe entero, asi que el
    // mesher no tiene que recortar nada y la planta se dibuja COMPLETA en
    // todos los tamaños. Es el mismo criterio que el resto del archivo: se
    // acota en el origen para que la garantia sea por construccion.
    constexpr float ARRANQUE = 0.34f;   // donde nace la penca, en fraccion de RADIO

    inline float radioDeCeldas(int celdas) {
        const float r = alturaReal(celdas) * 0.5f;
        // El techo se reparte entre el arranque y el recorrido de la hoja.
        const float tope = radioMaximoReal() / (1.0f + ARRANQUE);
        return (r > tope) ? tope : r;
    }

    inline float radioRoseta(float esc) {
        const float r = 0.10f + 0.34f * esc;
        return (r > 0.44f) ? 0.44f : r;
    }

    // ========================================================================
    // ⭐ LO QUE ENGORDA CON EL TAMAÑO
    // ========================================================================
    // Se pidio que al crecer se multiplique tambien el ancho y el grosor, no
    // solo la altura. A lo ancho la roseta topa en su celda (ver arriba), asi
    // que el volumen se gana donde SI hay sitio:
    //
    //   - cada penca es mas GRUESA (mas carnosa)
    //   - cada penca es mas ANCHA dentro de su propio limite
    //   - hay MAS pencas
    //
    // El resultado es el mismo que se buscaba: la planta se ve mas maciza
    // segun crece, sin salirse de su columna.

    // Multiplicador de grosor/ancho de penca. Un brote tiene hojas finas; un
    // ejemplar viejo las tiene gordas y carnosas.
    inline float engrosado(uint16_t etapa) {
        switch (etapa) {
            case 0:  return 0.70f;   // BROTE:  hojas tiernas y finas
            case 1:  return 1.00f;   // JOVEN
            case 2:  return 1.30f;   // ADULTO
            case 3:  return 1.55f;   // MADURO
            default: return 1.75f;   // PRODUCTOR: bien carnosa
        }
    }

    // Cuantas pencas le salen. Crece con la edad: "las pencas sean aun mas
    // entre mas grande".
    inline int pencasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case 0:  return 7;    // BROTE:  el cogollo apenas abierto
            case 1:  return 11;   // JOVEN
            case 2:  return 16;   // ADULTO
            case 3:  return 21;   // MADURO
            default: return 26;   // PRODUCTOR: roseta densa
        }
    }

    // Medio ancho del cajete (el cuenco donde se junta el jugo).
    inline float radioCajete(float esc) {
        const float r = 0.10f + 0.11f * esc;
        return (r > 0.38f) ? 0.38f : r;
    }

    // Altura del fondo del cajete: hundido entre el arranque de las pencas.
    inline float alturaCajete(float esc) { return altoHoja(esc) * 0.30f; }

    // Hondura del cuenco.
    inline float hondoCajete(float esc) {
        const float h = 0.08f + 0.10f * esc;
        return (h > 0.30f) ? 0.30f : h;
    }

    // Donde nacen las espinas: en la punta de las pencas de fuera.
    inline float alturaEspina(float esc) { return altoHoja(esc) * 0.72f; }

    // Medio ancho de la caja de colision del cuerpo. Sigue al radio de la
    // roseta para que el jugador choque donde ve la planta, pero se queda
    // dentro del voxel para poder pasar al lado.
    inline float medioAnchoCuerpo(float esc) {
        const float m = radioRoseta(esc) * 0.85f;
        return (m > 0.45f) ? 0.45f : m;
    }

    // --- Construir un maguey ---
    inline BlockType crear(uint16_t etapa, uint16_t giro,
                           uint16_t puntas, uint16_t aguamiel,
                           bool capado, uint16_t segmento = 0) {
        uint16_t e = 0;
        e = escribir(e, ETAPA,    etapa);
        e = escribir(e, GIRO,     giro);
        e = escribir(e, PUNTAS,   puntas);
        e = escribir(e, AGUAMIEL, aguamiel);
        e = escribir(e, CAPADO,   capado ? 1 : 0);
        e = escribir(e, SEGMENTO, segmento);
        return hacer(FAM_MAGUEY, e);
    }

    // La misma planta, pero el trozo que va en la altura `s`.
    inline BlockType conSegmento(BlockType t, uint16_t s) {
        return hacer(FAM_MAGUEY, escribir(estadoDe(t), SEGMENTO, s));
    }

    // Un maguey recien generado: sin capar y sin jugo, con las puntas que le
    // tocan por edad.
    inline BlockType nuevo(uint16_t etapa, uint16_t giro) {
        return crear(etapa, giro, puntasDeEtapa(etapa), 0, false);
    }

    // --- Leer campos de un bloque ya hecho ---
    inline uint16_t etapaDe(BlockType t)    { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)     { return leer(estadoDe(t), GIRO); }
    inline uint16_t puntasDe(BlockType t)   { return leer(estadoDe(t), PUNTAS); }
    inline uint16_t aguamielDe(BlockType t) { return leer(estadoDe(t), AGUAMIEL); }
    inline bool     capadoDe(BlockType t)   { return leer(estadoDe(t), CAPADO) != 0; }

    // En que altura de la planta esta esta celda. 0 es la de abajo.
    inline uint16_t segmentoDe(BlockType t) { return leer(estadoDe(t), SEGMENTO); }

    // ¿Es la celda de abajo? Es la que MANDA: la que lleva el estado bueno,
    // la que se capa, la que da el jugo y la que sostiene a las de arriba.
    inline bool esBase(BlockType t) { return segmentoDe(t) == 0; }

    // Cuantas celdas ocupa esta planta en total.
    inline int celdasDe(BlockType t) { return celdasDeEtapa(etapaDe(t)); }

    // --- Devolver un bloque nuevo con un campo cambiado ---
    // Se devuelve otro ID en vez de mutar: los bloques son valores, y asi el
    // llamador decide cuando escribirlo en el mundo.
    inline BlockType conCampo(BlockType t, Campo c, uint16_t v) {
        return hacer(FAM_MAGUEY, escribir(estadoDe(t), c, v));
    }

    inline BlockType conAguamiel(BlockType t, uint16_t v) {
        return conCampo(t, AGUAMIEL, v);
    }
    inline BlockType capar(BlockType t) {
        // Capar tambien le quita una punta: es la que se corta para abrirlo.
        const uint16_t p = puntasDe(t);
        BlockType r = conCampo(t, CAPADO, 1);
        if (p > 0) r = conCampo(r, PUNTAS, (uint16_t)(p - 1));
        return r;
    }
    inline BlockType vaciado(BlockType t) {
        return conAguamiel(t, 0);
    }
    inline BlockType conEtapa(BlockType t, uint16_t etapa) {
        BlockType r = conCampo(t, ETAPA, etapa);
        // Al crecer le salen las puntas que le tocan, si no le quitaron mas.
        const uint16_t p = puntasDeEtapa(etapa);
        if (puntasDe(r) < p && !capadoDe(r)) r = conCampo(r, PUNTAS, p);
        return r;
    }
    inline BlockType conPuntas(BlockType t, uint16_t p) {
        return conCampo(t, PUNTAS, p);
    }

} // namespace Maguey

// ============================================================================
// FAMILIA: BIZNAGA
// ============================================================================
// El cactus de barril del desierto mexicano (Ferocactus, Echinocactus). Una
// bola achatada y acanalada, cubierta de espinas, que crece MUY despacio: los
// ejemplares grandes tienen decadas.
//
// Se modela con el mismo sistema que el maguey, pero es mucho mas simple: no
// produce nada, no se capa. Solo crece.
namespace Biznaga {

    // Etapa de crecimiento. Tres etapas reales, como se pidio.
    //   0 pequena | 1 mediana | 2 adulta
    constexpr Campo ETAPA = { 0, 2 };   // 0..3 (sobra una)

    // Giro: cuatro orientaciones para que un grupo no parezca calcado.
    constexpr Campo GIRO  = { 2, 2 };   // 0..3

    // Cuantas costillas tiene. Una biznaga real tiene entre 13 y 21, y el
    // numero es constante durante toda su vida: se fija al brotar.
    constexpr Campo COSTILLAS = { 4, 3 };   // 0..7 -> se mapea a 13..20

    // Quedan libres los bits 7..15.

    enum Etapa : uint16_t {
        PEQUENA = 0, MEDIANA = 1, ADULTA = 2
    };

    // Cuanto mide cada etapa, en fraccion de bloque. Una biznaga adulta de
    // verdad pasa del metro de diametro, asi que llena el voxel entero.
    //
    // ⭐ ESCALADA, NO REDIBUJADA.
    //
    // Se veia demasiado pequeña: un boton en el suelo del desierto en vez del
    // barril que es. Lo unico que cambia aqui es el TAMAÑO -- la forma sale
    // toda de este numero, porque el mesher deriva de el las dos medidas:
    //
    //     r    = esc * 0.5     (radio)
    //     alto = esc * 0.75    (altura, 3/4 del ancho)
    //
    // Al salir las dos del mismo dato, subir la escala agranda el cactus
    // conservando exactamente su proporcion de Ferocactus (mas ancho que alto)
    // y su numero de costillas. No se toca ni un angulo ni un perfil.
    //
    // ⚠️ EL TOPE DE LA ADULTA ES 1.0, Y NO PUEDE PASAR DE AHI.
    //
    // Con esc = 1.0 el radio es 0.5, o sea el bloque EXACTO: la biznaga llena
    // su celda y la toca por los cuatro lados. Un valor mayor la sacaria del
    // voxel, y entonces se colaria en la celda vecina -- donde ni la colision
    // ni la seleccion la esperan, porque las dos trabajan celda a celda.
    //
    // Las otras dos etapas suben en la misma proporcion para que el cactus siga
    // creciendo de forma pareja y se sigan distinguiendo de un vistazo.
    inline float escalaDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case PEQUENA: return 0.45f;
            case MEDIANA: return 0.72f;
            default:      return 1.00f;   // adulta: el bloque entero
        }
    }

    // Costillas de verdad, a partir del campo de 3 bits.
    inline int costillasDe(uint16_t v) { return 13 + (int)(v % 8u); }

    // --- Construir ---
    inline BlockType crear(uint16_t etapa, uint16_t giro, uint16_t costillas) {
        uint16_t e = 0;
        e = escribir(e, ETAPA,     etapa);
        e = escribir(e, GIRO,      giro);
        e = escribir(e, COSTILLAS, costillas);
        return hacer(FAM_BIZNAGA, e);
    }

    inline BlockType nuevo(uint16_t etapa, uint16_t giro, uint16_t costillas) {
        return crear(etapa, giro, costillas);
    }

    // --- Leer ---
    inline uint16_t etapaDe(BlockType t) { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)  { return leer(estadoDe(t), GIRO); }
    inline uint16_t costillasCampoDe(BlockType t) {
        return leer(estadoDe(t), COSTILLAS);
    }
    inline int costillasReales(BlockType t) {
        return costillasDe(costillasCampoDe(t));
    }

    inline bool esBiznaga(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_BIZNAGA;
    }

    // --- Crecer ---
    // Devuelve la biznaga una etapa mas vieja, o la misma si ya es adulta.
    inline BlockType crecida(BlockType t) {
        const uint16_t e = etapaDe(t);
        if (e >= ADULTA) return t;
        uint16_t est = estadoDe(t);
        est = escribir(est, ETAPA, (uint16_t)(e + 1));
        return hacer(FAM_BIZNAGA, est);
    }

    // ¿Ya no puede crecer mas?
    inline bool estaHecha(BlockType t) { return etapaDe(t) >= ADULTA; }

} // namespace Biznaga

// ============================================================================
// FAMILIA: AGAVE TEQUILANA AZUL
// ============================================================================
// El agave del tequila. A diferencia del pulquero -- que se capa y vive
// decadas dando aguamiel -- esta planta tiene un ciclo de vida con FINAL, y
// ese final se bifurca:
//
//     ROSETA (7-12 anos)  --- jima ---->  PIÑA          (cosecha)
//            |
//            '------------ vejez ------>  QUIOTE        (floracion y muerte)
//
// ----------------------------------------------------------------------------
// LAS TRES SILUETAS
// ----------------------------------------------------------------------------
//
//   1. ROSETA. Hojas largas, carnosas, RIGIDAS Y ERECTAS -- no arqueadas como
//      las del pulquero: esa es la diferencia de silueta que se ve de lejos.
//      Cubiertas de cera, lo que da el azul plateado mate. Bordes fuertemente
//      aserrados con espinas oscuras y una espina terminal muy dura.
//      Hasta 2 m de alto y 3 m de ancho: es MAS ANCHA QUE ALTA.
//
//   2. PIÑA. Al jimar (cortar todas las pencas) queda el tallo central: una
//      bola fibrosa blanca y verde claro, identica a una piña gigante.
//      40-100 kg, o sea del orden de un bloque entero de diametro.
//
//   3. QUIOTE. Si no se cosecha, el centro levanta un eje floral vertical de
//      hasta 5 m que se abre arriba como un candelabro con racimos de flores
//      amarillas. Es la fase final: la planta muere despues de florecer.
//
// ----------------------------------------------------------------------------
// ESCALA DEL JUEGO: 1 bloque = 1 metro
// ----------------------------------------------------------------------------
//   Roseta ..... 2 m alto x 3 m ancho  -> 2 celdas de alto, radio 1.5
//   Piña ....... ~1 m de diametro      -> cabe en una celda
//   Quiote ..... 5 m                   -> 5 celdas por encima de la roseta
namespace AgaveAzul {

    // ------------------------------------------------------------------------
    // REPARTO DE LOS 16 BITS
    // ------------------------------------------------------------------------
    // Estrena estado limpio, asi que hay sitio de sobra. Se deja margen en
    // cada campo para no repetir el apuro del maguey.

    // Fase del ciclo de vida. Es lo que decide QUE SILUETA se dibuja, asi que
    // va primero: es el campo que mas mira el mesher.
    constexpr Campo FASE      = { 0, 2 };   // 0..3

    // Edad dentro de la fase roseta. La planta tarda 7-12 anos en hacerse.
    constexpr Campo ETAPA     = { 2, 3 };   // 0..7

    // Giro: ocho orientaciones (el maguey solo tenia cuatro). Con hojas
    // rigidas y radiales, cuatro giros se notan repetidos.
    constexpr Campo GIRO      = { 5, 3 };   // 0..7

    // En que altura de la planta esta esta celda. El quiote necesita llegar a
    // 5 celdas por encima de la roseta, asi que hacen falta 3 bits.
    constexpr Campo SEGMENTO  = { 8, 3 };   // 0..7

    // Cuanto ha crecido el quiote, en celdas ya levantadas. Sube poco a poco
    // para que se vea ESTIRARSE en vez de aparecer entero de golpe.
    constexpr Campo QUIOTE    = { 11, 3 };  // 0..7

    // ¿El quiote ya abrio sus flores? Solo cuando llega arriba del todo.
    constexpr Campo FLORECIDO = { 14, 1 };  // 0..1

    // Queda libre el bit 15.

    // --- Las tres siluetas ---
    enum Fase : uint16_t {
        ROSETA = 0,   // fase de crecimiento: la corona de pencas
        PINA   = 1,   // jimada: solo el tallo central, como una piña gigante
        QUIOTE_F = 2  // floracion: el eje vertical con el candelabro
    };

    // --- Etapas de la roseta ---
    // Cinco, como el pulquero, para que el ritmo de crecimiento se sienta
    // igual aunque la planta sea otra.
    enum Etapa : uint16_t {
        HIJUELO = 0,   // el retoño que echa por rizoma
        JOVEN   = 1,
        MEDIA   = 2,
        HECHA   = 3,   // ya se puede jimar
        MADURA  = 4    // en su punto: la que da mas piña
    };

    // ------------------------------------------------------------------------
    // MEDIDAS DE LA ROSETA
    // ------------------------------------------------------------------------
    // ⚠️ AQUI ESTA LA DIFERENCIA DE PROPORCION CON EL PULQUERO.
    //
    // El maguey pulquero es tan ancho como alto (1:1). El tequilana es
    // claramente MAS ANCHO QUE ALTO: 2 m de alto por 3 m de diametro, o sea
    // 1:1.5. Por eso alto y radio NO salen del mismo numero como alli: se
    // calculan por separado y con esa relacion.

    // Cuantas celdas de alto ocupa la ROSETA (sin contar el quiote).
    inline int celdasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 1;
            case JOVEN:   return 1;
            case MEDIA:   return 2;
            default:      return 2;   // hecha y madura: 2 m de alto
        }
    }

    // Altura visible de la roseta, en bloques.
    inline float alturaReal(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 0.45f;
            case JOVEN:   return 0.90f;
            case MEDIA:   return 1.45f;
            case HECHA:   return 1.80f;
            default:      return 2.00f;   // los 2 m de la descripcion
        }
    }

    // Radio de la roseta. Es 0.75 de la altura porque el diametro (3 m) es
    // 1.5 veces el alto (2 m): radio = diametro/2 = alto * 0.75.
    //
    // ⚠️ TOPE 1.45. Igual que en el maguey, pasar de 1.5 haria que la planta
    // cruzara el borde del CHUNK (16 bloques), donde el recorte del dibujado
    // si corta: se veria aparecer y desaparecer al girar la camara.
    inline float radioDeEtapa(uint16_t etapa) {
        const float r = alturaReal(etapa) * 0.75f;
        return (r > 1.45f) ? 1.45f : r;
    }

    // Donde nace la penca, en fraccion del radio: dentro de la piña, para que
    // no quede un anillo de aire entre el cuerpo y las hojas.
    constexpr float ARRANQUE = 0.30f;

    // Cuantas pencas tiene. Un tequilana adulto pasa de las 100 hojas, pero
    // dibujarlas todas cierra la roseta en un bulto macizo y se pierde la luz
    // entre hoja y hoja. Se busca la silueta, no el censo.
    inline int pencasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 9;
            case JOVEN:   return 14;
            case MEDIA:   return 20;
            case HECHA:   return 26;
            default:      return 32;   // madura: roseta densa
        }
    }

    // Grosor de la penca. El tequilana es MUY carnoso: la hoja es gruesa y
    // rigida, no una lamina.
    inline float engrosado(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 0.72f;
            case JOVEN:   return 1.00f;
            case MEDIA:   return 1.30f;
            case HECHA:   return 1.55f;
            default:      return 1.80f;
        }
    }

    // ⭐ LO QUE HACE QUE SE VEA RIGIDO Y ERECTO.
    //
    // La penca del pulquero se arquea: sube y se vence al final (por eso su
    // mesher usa un termino de caida cubico). La del tequilana NO: es rigida,
    // sale casi recta y apunta hacia arriba y afuera como una espada.
    //
    // Este es el factor de caida, y es deliberadamente pequeño: lo justo para
    // que no parezcan varillas perfectas, pero muy lejos del arqueo del
    // pulquero.
    inline float caidaDeEtapa(uint16_t etapa) {
        // Solo las hojas de las plantas viejas se vencen un poco.
        return (etapa >= HECHA) ? 0.10f : 0.05f;
    }

    // Cuantos dientes tiene el borde aserrado de cada penca. La descripcion
    // insiste en que el borde esta "fuertemente aserrado", asi que es una
    // caracteristica que hay que ver, no sugerir.
    inline int dientesDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 3;
            case JOVEN:   return 4;
            case MEDIA:   return 5;
            default:      return 6;
        }
    }

    // ------------------------------------------------------------------------
    // MEDIDAS DE LA PIÑA (fase jimada)
    // ------------------------------------------------------------------------
    // 40-100 kg de tallo fibroso. Es una bola achatada, mas ancha que alta,
    // con las cicatrices de las pencas cortadas por fuera.

    // Radio de la piña segun la etapa que tenia al jimarla: una planta madura
    // da una piña mucho mayor que una recien hecha.
    inline float radioPina(uint16_t etapa) {
        switch (etapa) {
            case HECHA: return 0.34f;
            default:    return 0.42f;   // madura: la piña grande
        }
    }

    // Alto de la piña. Achatada: ~0.8 de su radio en cada mitad.
    inline float altoPina(uint16_t etapa) { return radioPina(etapa) * 1.55f; }

    // ------------------------------------------------------------------------
    // MEDIDAS DEL QUIOTE (fase de floracion)
    // ------------------------------------------------------------------------
    // Un eje vertical de hasta 5 m que remata en candelabro. Es lo mas alto
    // que produce esta planta y se ve desde muy lejos.

    // A cuantas celdas llega el quiote cuando esta del todo crecido.
    constexpr int QUIOTE_CELDAS_MAX = 5;

    // Medio grosor del tallo a una altura dada (0 = base, 1 = punta). Se
    // afila hacia arriba, como un esparrago gigante.
    inline float grosorQuiote(float t) {
        return 0.11f * (1.0f - 0.55f * t);
    }

    // Cuantos brazos tiene el candelabro. Un quiote real abre entre 6 y 12
    // ramas horizontales cargadas de flores.
    constexpr int BRAZOS_CANDELABRO = 8;

    // --- Construir ---
    inline BlockType crear(uint16_t fase, uint16_t etapa, uint16_t giro,
                           uint16_t segmento = 0, uint16_t quiote = 0,
                           bool florecido = false) {
        uint16_t e = 0;
        e = escribir(e, FASE,      fase);
        e = escribir(e, ETAPA,     etapa);
        e = escribir(e, GIRO,      giro);
        e = escribir(e, SEGMENTO,  segmento);
        e = escribir(e, QUIOTE,    quiote);
        e = escribir(e, FLORECIDO, florecido ? 1 : 0);
        return hacer(FAM_AGAVE_AZUL, e);
    }

    // Un agave recien generado: roseta, sin quiote.
    inline BlockType nuevo(uint16_t etapa, uint16_t giro) {
        return crear(ROSETA, etapa, giro);
    }

    // La misma planta, pero el trozo que va en la altura `s`.
    inline BlockType conSegmento(BlockType t, uint16_t s) {
        return hacer(FAM_AGAVE_AZUL, escribir(estadoDe(t), SEGMENTO, s));
    }

    // --- Leer ---
    inline uint16_t faseDe(BlockType t)     { return leer(estadoDe(t), FASE); }
    inline uint16_t etapaDe(BlockType t)    { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)     { return leer(estadoDe(t), GIRO); }
    inline uint16_t segmentoDe(BlockType t) { return leer(estadoDe(t), SEGMENTO); }
    inline uint16_t quioteDe(BlockType t)   { return leer(estadoDe(t), QUIOTE); }
    inline bool     florecidoDe(BlockType t){ return leer(estadoDe(t), FLORECIDO) != 0; }

    inline bool esAgaveAzul(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_AGAVE_AZUL;
    }

    // ¿Es la celda de abajo? Es la que MANDA: la que se jima y la que lleva
    // el estado bueno. Las de arriba solo prolongan la planta.
    inline bool esBase(BlockType t) { return segmentoDe(t) == 0; }

    // Cuantas celdas ocupa la planta ENTERA ahora mismo, contando el quiote.
    inline int celdasDe(BlockType t) {
        const uint16_t f = faseDe(t);
        if (f == PINA) return 1;                    // jimada: solo la bola
        const int base = celdasDeEtapa(etapaDe(t));
        if (f == QUIOTE_F) return base + (int)quioteDe(t);
        return base;
    }

    // --- Cambiar de estado ---
    inline BlockType conCampo(BlockType t, Campo c, uint16_t v) {
        return hacer(FAM_AGAVE_AZUL, escribir(estadoDe(t), c, v));
    }

    // ¿Se puede jimar? Hace falta que sea una roseta ya hecha: jimar un
    // hijuelo no da piña y solo mata la planta.
    inline bool sePuedeJimar(BlockType t) {
        return faseDe(t) == ROSETA && etapaDe(t) >= HECHA;
    }

    // LA JIMA: se cortan todas las pencas y queda el tallo central.
    inline BlockType jimada(BlockType t) {
        BlockType r = conCampo(t, FASE, PINA);
        // La piña ocupa UNA celda: se olvida el segmento.
        return conCampo(r, SEGMENTO, 0);
    }

    // ¿Puede echar quiote? Solo la planta madura sin jimar.
    inline bool puedeEspigar(BlockType t) {
        return faseDe(t) == ROSETA && etapaDe(t) >= MADURA;
    }

    // Empieza a espigar: pasa a fase quiote con el tallo aun sin levantar.
    inline BlockType espigada(BlockType t) {
        BlockType r = conCampo(t, FASE, QUIOTE_F);
        return conCampo(r, QUIOTE, 1);
    }

    // El quiote crece una celda mas. Al llegar arriba, abre las flores.
    inline BlockType quioteCrecido(BlockType t) {
        const uint16_t q = quioteDe(t);
        if (q >= QUIOTE_CELDAS_MAX)
            return conCampo(t, FLORECIDO, 1);
        return conCampo(t, QUIOTE, (uint16_t)(q + 1));
    }

    // Crecer una etapa dentro de la fase roseta.
    inline BlockType crecida(BlockType t) {
        const uint16_t e = etapaDe(t);
        if (faseDe(t) != ROSETA || e >= MADURA) return t;
        return conCampo(t, ETAPA, (uint16_t)(e + 1));
    }

} // namespace AgaveAzul

// ============================================================================
// FAMILIA: AGAVE ABISAII
// ============================================================================
// Roseta MASIVA de hoja ancha y arqueada. Es la tercera especie de agave del
// motor, y se distingue de las otras dos por la silueta:
//
//   PULQUERO    hoja larga y estrecha, se arquea al final   (1:1, ancho=alto)
//   TEQUILANA   hoja rigida y erecta, como una espada       (1:1.5)
//   ABISAII     hoja ANCHA que se vence por su peso         (1:1.6, la mas baja)
//
// LA ESCALA, QUE ES LO QUE MAS SE EQUIVOCA AQUI:
//
//   En este motor 1 bloque = 0.60 m (ver PecariModelo3D.h, LADO_BLOQUE_M).
//   Una roseta de 3x3 celdas son 1.8 m de diametro, NO 3 m. Para las rosetas
//   grandes de esta especie hacen falta ~4 celdas de diametro, que es lo que
//   dan los radios de abajo (RADIO 1.15 -> diametro 2.3 celdas -> 1.4 m) sin
//   pasarse del tope que obliga el borde de chunk.
//
// ⚠️ TOPE DE RADIO 1.45, igual que en las otras dos. Pasar de 1.5 haria que
// la planta cruzara el borde del CHUNK (16 bloques), donde el recorte del
// dibujado SI corta: se veria aparecer y desaparecer al girar la camara.
namespace AgaveAbisaii {

    // ------------------------------------------------------------------------
    // REPARTO DE LOS 16 BITS
    // ------------------------------------------------------------------------
    // Estado limpio. Esta especie no tiene quiote implementado (la fase de
    // floracion se deja para mas adelante), asi que sobra sitio: se dejan los
    // campos anchos a proposito para no repetir el apuro del maguey.

    // Edad de la roseta. Cinco etapas, como las otras dos especies, para que
    // el ritmo de crecimiento se sienta igual aunque la planta sea otra.
    constexpr Campo ETAPA    = { 0, 3 };   // 0..7 (se usan 0..4)

    // Giro: ocho orientaciones. Con hojas anchas y radiales, cuatro giros se
    // notan repetidos de inmediato.
    constexpr Campo GIRO     = { 3, 3 };   // 0..7

    // En que altura de la planta esta esta celda. La de abajo (0) es la que
    // MANDA y la unica que dibuja; las de arriba solo reservan el hueco.
    constexpr Campo SEGMENTO = { 6, 3 };   // 0..7

    // Quedan libres los bits 9..15 para lo que venga (quiote, floracion).

    // --- Etapas ---
    enum Etapa : uint16_t {
        HIJUELO = 0,   // el retoño
        JOVEN   = 1,
        MEDIA   = 2,
        HECHA   = 3,
        MADURA  = 4    // la roseta masiva en su punto
    };

    // --- Construir ---
    inline BlockType crear(uint16_t etapa, uint16_t giro, uint16_t segmento = 0) {
        uint16_t e = 0;
        e = escribir(e, ETAPA,    etapa);
        e = escribir(e, GIRO,     giro);
        e = escribir(e, SEGMENTO, segmento);
        return hacer(FAM_AGAVE_ABISAII, e);
    }

    inline BlockType nuevo(uint16_t etapa, uint16_t giro) {
        return crear(etapa, giro, 0);
    }

    // La misma planta, pero el trozo que va en la altura `s`.
    inline BlockType conSegmento(BlockType t, uint16_t s) {
        return hacer(FAM_AGAVE_ABISAII, escribir(estadoDe(t), SEGMENTO, s));
    }

    // --- Leer ---
    inline uint16_t etapaDe(BlockType t)    { return leer(estadoDe(t), ETAPA); }
    inline uint16_t giroDe(BlockType t)     { return leer(estadoDe(t), GIRO); }
    inline uint16_t segmentoDe(BlockType t) { return leer(estadoDe(t), SEGMENTO); }

    inline bool esAgaveAbisaii(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_AGAVE_ABISAII;
    }

    // ¿Es la celda de abajo? Es la que MANDA: la que lleva el estado bueno y
    // la UNICA que dibuja la planta entera (ver la regla multicelda).
    inline bool esBase(BlockType t) { return segmentoDe(t) == 0; }

    // ------------------------------------------------------------------------
    // MEDIDAS DE LA ROSETA
    // ------------------------------------------------------------------------

    // Cuantas celdas de alto ocupa. Mas baja que el tequilana a igual edad:
    // esta especie crece a lo ANCHO, no a lo alto.
    inline int celdasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 1;
            case JOVEN:   return 1;
            case MEDIA:   return 2;
            default:      return 2;
        }
    }

    // Altura visible de la roseta, en bloques.
    inline float alturaReal(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 0.40f;
            case JOVEN:   return 0.75f;
            case MEDIA:   return 1.15f;
            case HECHA:   return 1.45f;
            default:      return 1.60f;
        }
    }

    // Radio de la roseta. Factor 0.80 (el tequilana usa 0.75): esta especie
    // es aun mas ancha en proporcion a su alto -- 1:1.6 frente a 1:1.5.
    //
    // ⚠️ El tope 1.45 es el borde de chunk, no una decision estetica.
    inline float radioDeEtapa(uint16_t etapa) {
        const float r = alturaReal(etapa) * 0.80f;
        return (r > 1.45f) ? 1.45f : r;
    }

    // Donde nace la hoja, en fraccion del radio: dentro del corazon, para que
    // no quede un anillo de aire entre el cuerpo y las hojas.
    constexpr float ARRANQUE = 0.28f;

    // Cuantas hojas tiene. Menos que el tequilana a igual etapa porque la
    // hoja es MUCHO mas ancha: con las mismas 32 la roseta se cierra en un
    // bulto macizo y se pierde la luz entre hoja y hoja.
    inline int hojasDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 7;
            case JOVEN:   return 11;
            case MEDIA:   return 15;
            case HECHA:   return 19;
            default:      return 24;
        }
    }

    // Grosor de la hoja. Carnosa, como todas las suculentas del motor.
    //
    // ⚠️ ESTE ES EL FACTOR, NO EL GROSOR EN PIXELES. El grosor real de 4/16
    // sale de multiplicar GRUESO_BASE por esto en el mesher; aqui solo se
    // dice cuanto engorda con la edad.
    inline float engrosado(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 0.70f;
            case JOVEN:   return 0.95f;
            case MEDIA:   return 1.25f;
            case HECHA:   return 1.50f;
            default:      return 1.75f;
        }
    }

    // ⭐ LO QUE LA DISTINGUE: LA HOJA SE ARQUEA.
    //
    // El tequilana es rigido (caida 0.05-0.10). Esta especie tiene la hoja
    // ancha y pesada, asi que se VENCE: sube, llega a su punto alto y cae.
    // Es el rasgo que se reconoce de lejos, asi que el factor es grande.
    inline float caidaDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 0.10f;   // el retoño aun no pesa
            case JOVEN:   return 0.18f;
            case MEDIA:   return 0.30f;
            case HECHA:   return 0.40f;
            default:      return 0.48f;   // la madura se vence del todo
        }
    }

    // Dientes del borde. Los tiene, pero menos marcados que el tequilana.
    inline int dientesDeEtapa(uint16_t etapa) {
        switch (etapa) {
            case HIJUELO: return 2;
            case JOVEN:   return 3;
            case MEDIA:   return 4;
            default:      return 5;
        }
    }

    // --- Crecer ---
    inline BlockType crecida(BlockType t) {
        const uint16_t e = etapaDe(t);
        if (e >= MADURA) return t;
        return hacer(FAM_AGAVE_ABISAII, escribir(estadoDe(t), ETAPA,
                                                 (uint16_t)(e + 1)));
    }

} // namespace AgaveAbisaii

// ============================================================================
// FAMILIA: AGUA CON VOLUMEN
// ============================================================================
// El agua deja de ser "hay agua o no hay agua" y pasa a tener CUANTA hay.
//
// ----------------------------------------------------------------------------
// POR QUE VIVE AQUI Y NO EN UN std::map
// ----------------------------------------------------------------------------
// El motor guardaba el nivel en un std::map<tuple<x,y,z>, int> aparte. Eso
// tenia tres fallos, y los tres desaparecen metiendo el nivel en el ID:
//
//   1. NO SE GUARDABA. El mapa vive en RAM y el save solo escribe IDs de
//      bloque: al recargar el mundo, TODA el agua volvia a ser fuente. Un
//      charco a medio secar reaparecia lleno.
//   2. NO SE PURGABA. Nadie borraba las entradas de los chunks descargados,
//      asi que crecia durante toda la sesion; y al volver a una zona,
//      getWaterLevel devolvia el nivel rancio de antes.
//   3. UN OCEANO NO CABE. Un mar son millones de celdas: millones de nodos de
//      std::map, con su asignacion y sus tres punteros cada uno.
//
// Metiendolo en el ID, el nivel se guarda solo, se descarga con su chunk y no
// ocupa ni un byte extra. Es exactamente lo que ya se hizo con el maguey.
//
// ----------------------------------------------------------------------------
// LA ESCALA: 8 NIVELES
// ----------------------------------------------------------------------------
// Una celda llena son 8 octavos. Al repartirse pierde altura de octavo en
// octavo, asi que el agua derramada se ve BAJAR por escalones y no de golpe.
//
//     8 = celda llena (el fondo de un rio o del mar)
//     1 = la lamina mas fina que se sostiene
//     0 = seco (la celda deja de ser agua y vuelve a ser aire)
//
// Ojo con el sentido: aqui MAS NUMERO ES MAS AGUA, al reves que el sistema
// viejo, donde 0 era la fuente y 7 el hilo mas fino. Se cambio a proposito
// porque "nivel 8 = lleno, nivel 0 = vacio" es lo que cualquiera espera, y
// porque asi el nivel ES la cantidad: sumarlos y restarlos conserva volumen.
namespace Agua {

    // Cuanta agua cabe en una celda. Es el numerador de los octavos.
    constexpr uint16_t LLENA = 8;

    // --- Reparto de los 16 bits del estado ---

    // Cuanta agua hay aqui, en octavos de celda. 0 no se usa como estado
    // valido: una celda sin agua no es agua, es aire.
    constexpr Campo NIVEL  = { 0, 4 };   // 0..15, se usan 1..8

    // ¿Esta cayendo? El agua que cae se dibuja como una columna que llena la
    // celda entera aunque su nivel sea bajo -- un chorro fino sigue siendo un
    // chorro de arriba abajo, no un charco flotando en el aire.
    constexpr Campo CAYENDO = { 4, 1 };  // 0..1

    // ⭐ ¿ESTA CELDA ES ORILLA CON OLEAJE?
    //
    // La marca la pone el generador en la franja de agua que toca la playa, y
    // es lo que distingue "agua de orilla" de "agua quieta". Sirve para tres
    // cosas a la vez:
    //
    //   1. El simulador de olas solo mira estas celdas: no recorre el oceano.
    //   2. La arena de debajo NO bebe de ellas (ver absorberDebajo). Sin esto
    //      cada vaiven mojaria un poco mas de playa hasta saturarla entera.
    //   3. Al retirarse la ola, la celda no se borra: baja a nivel 1 y se
    //      queda, asi que la ola puede volver. Si se convirtiera en aire,
    //      habria que "crear" agua para el siguiente ciclo -- y el agua de
    //      este motor no se crea.
    constexpr Campo ORILLA = { 5, 1 };   // 0..1

    // ⭐ CORRIENTE: HACIA DONDE EMPUJA ESTA AGUA
    //
    // 0 = quieta. 1..4 = las cuatro direcciones cardinales (N, E, S, O).
    // La usan los rios: el cauce lleva su direccion grabada, asi que el agua
    // arrastra al jugador y el mesher puede animar la textura en el sentido
    // de la corriente sin que nadie tenga que recalcular por donde va el rio.
    //
    // Cabe en 3 bits con sitio para diagonales mas adelante.
    constexpr Campo CORRIENTE = { 6, 3 };   // 0..7

    // Quedan libres los bits 9..15.

    // --- Construir y leer ---

    // Una celda de agua con este nivel. Si el nivel es 0 no hay agua: el
    // llamador tiene que poner aire, y por eso esto NO se puede usar para
    // "agua vacia".
    inline BlockType nuevo(uint16_t nivel, bool cayendo = false) {
        if (nivel > LLENA) nivel = LLENA;
        uint16_t est = 0;
        est = escribir(est, NIVEL, nivel);
        est = escribir(est, CAYENDO, cayendo ? 1 : 0);
        return hacer(FAM_AGUA, est);
    }

    // ¿Es una celda de agua con volumen?
    inline bool esAgua(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_AGUA;
    }

    // Cuanta agua tiene. Devuelve 0 para lo que no es agua, de modo que
    // sumar el contenido de una columna entera no necesita comprobaciones.
    inline uint16_t nivelDe(BlockType t) {
        if (!esAgua(t)) return 0;
        return leer(estadoDe(t), NIVEL);
    }

    // ¿Esta cayendo por el aire?
    inline bool estaCayendo(BlockType t) {
        if (!esAgua(t)) return false;
        return leer(estadoDe(t), CAYENDO) != 0;
    }

    // ¿Es una celda llena del todo?
    // ⚠️ Se cualifica con Agua:: a proposito. BlockType.h tiene un nivelDe()
    // global (el de las capas parciales de terreno) y, sin el prefijo, el
    // compilador no sabe cual de los dos se pide: la llamada es ambigua y el
    // build falla.
    inline bool estaLlena(BlockType t) { return Agua::nivelDe(t) >= LLENA; }

    // La misma agua con otro nivel, conservando lo demas.
    inline BlockType conNivel(BlockType t, uint16_t nivel) {
        if (nivel > LLENA) nivel = LLENA;
        uint16_t est = esAgua(t) ? estadoDe(t) : 0;
        est = escribir(est, NIVEL, nivel);
        return hacer(FAM_AGUA, est);
    }

    // La misma agua marcada (o no) como cayendo.
    inline BlockType conCaida(BlockType t, bool cayendo) {
        uint16_t est = esAgua(t) ? estadoDe(t) : 0;
        est = escribir(est, CAYENDO, cayendo ? 1 : 0);
        return hacer(FAM_AGUA, est);
    }

    // ----------------------------------------------------------------------
    // ORILLA (oleaje)
    // ----------------------------------------------------------------------

    inline bool esOrilla(BlockType t) {
        if (!esAgua(t)) return false;
        return leer(estadoDe(t), ORILLA) != 0;
    }

    inline BlockType conOrilla(BlockType t, bool orilla) {
        uint16_t est = esAgua(t) ? estadoDe(t) : 0;
        est = escribir(est, ORILLA, orilla ? 1 : 0);
        return hacer(FAM_AGUA, est);
    }

    // ----------------------------------------------------------------------
    // CORRIENTE (rios)
    // ----------------------------------------------------------------------
    // Las cuatro cardinales. El 0 se reserva para "sin corriente", asi que el
    // agua quieta -- que es casi toda -- no necesita marca.
    enum Corriente : uint16_t {
        SIN_CORRIENTE = 0,
        HACIA_NORTE   = 1,   // -Z
        HACIA_ESTE    = 2,   // +X
        HACIA_SUR     = 3,   // +Z
        HACIA_OESTE   = 4
    };

    inline uint16_t corrienteDe(BlockType t) {
        if (!esAgua(t)) return SIN_CORRIENTE;
        return leer(estadoDe(t), CORRIENTE);
    }

    inline bool tieneCorriente(BlockType t) {
        return corrienteDe(t) != SIN_CORRIENTE;
    }

    inline BlockType conCorriente(BlockType t, uint16_t dir) {
        if (dir > HACIA_OESTE) dir = SIN_CORRIENTE;
        uint16_t est = esAgua(t) ? estadoDe(t) : 0;
        est = escribir(est, CORRIENTE, dir);
        return hacer(FAM_AGUA, est);
    }

    // El vector de empuje de una corriente, en X/Z. Lo usan el arrastre del
    // jugador y la animacion de la textura, asi que los dos leen SIEMPRE de
    // aqui: si se separaran, el agua se veria correr hacia un lado y
    // empujaria hacia el otro.
    inline void empujeDe(uint16_t dir, float& dx, float& dz) {
        switch (dir) {
            case HACIA_NORTE: dx =  0.0f; dz = -1.0f; break;
            case HACIA_ESTE:  dx =  1.0f; dz =  0.0f; break;
            case HACIA_SUR:   dx =  0.0f; dz =  1.0f; break;
            case HACIA_OESTE: dx = -1.0f; dz =  0.0f; break;
            default:          dx =  0.0f; dz =  0.0f; break;
        }
    }

    // ----------------------------------------------------------------------
    // ALTURA VISUAL
    // ----------------------------------------------------------------------
    // Que fraccion del cubo ocupa esta agua al dibujarse. Es lo que hace que
    // "bajar de nivel" se VEA.
    //
    // El agua que cae se dibuja llena: un chorro atraviesa la celda entera.
    //
    // El minimo no es cero sino una lamina de un octavo: una celda con agua
    // tiene que verse mojada, no invisible.
    inline float alturaVisual(BlockType t) {
        // El agua de los mundos viejos (BLOCK_WATER) es una celda LLENA. Sin
        // este caso el mesher la veria a altura 0 y la dibujaria aplastada
        // contra el suelo: un mar guardado antes de este sistema se veria
        // vacio hasta que el flujo lo tocara celda a celda.
        if (t == BLOCK_WATER) return 1.0f;
        if (!esAgua(t)) return 0.0f;
        if (estaCayendo(t)) return 1.0f;
        const uint16_t n = Agua::nivelDe(t);   // ver la nota en estaLlena()
        if (n == 0) return 0.0f;
        return (float)n / (float)LLENA;
    }

} // namespace Agua

// ============================================================================
// COMPONENTES: LAS PARTES DE UN BLOQUE COMPUESTO
// ============================================================================
// Cada familia declara de que partes se compone. El raycast, la rotura y el
// render preguntan por INDICE y no saben de que planta se trata: eso es lo que
// hace el sistema reutilizable.
//
// Un componente NO es un bloque ni una entidad: es una etiqueta con una caja.
enum Componente : int {
    COMP_CUERPO = 0,   // el modelo principal (la roseta, el tronco, la roca)
    COMP_ADORNO = 1,   // lo que sobresale (puntas, frutos, musgo, flores)
    COMP_FLUIDO = 2,   // lo que se acumula dentro (aguamiel, resina, agua)
    COMP_MAX    = 3
};

// Cuantos componentes tiene ESTE bloque ahora mismo. Depende del estado: un
// maguey sin puntas no tiene componente de adorno, y uno sin jugo no tiene
// fluido, asi que el raycast no prueba cajas que no existen.
inline int componentesDe(BlockType t) {
    if (!esCompuesto(t)) return 1;

    switch (familiaDe(t)) {
        case FAM_MAGUEY: {
            int n = 1;                                   // el cuerpo, siempre
            if (Maguey::puntasDe(t) > 0)   n = 2;        // + puntas
            if (Maguey::aguamielDe(t) > 0) n = 3;        // + jugo
            return n;
        }
        case FAM_AGAVE_AZUL: {
            // La roseta tiene cuerpo + puntas (las espinas terminales, que
            // son lo que hay que cortar para jimar). La piña y el quiote son
            // una sola pieza: no hay nada que se seleccione aparte.
            return (AgaveAzul::faseDe(t) == AgaveAzul::ROSETA) ? 2 : 1;
        }
        case FAM_AGAVE_ABISAII:
            // Cuerpo + puntas, igual que la roseta del tequilana: la espina
            // terminal de cada hoja es lo que se selecciona aparte.
            return 2;
        default:
            return 1;
    }
}

// ¿Que componente es el indice i?
inline Componente componenteN(BlockType t, int i) {
    if (i < 0 || i >= componentesDe(t)) return COMP_CUERPO;
    return (Componente)i;
}

// ¿Se puede romper este componente a golpes?
// El fluido nunca: es liquido, se recoge con un recipiente.
inline bool componenteRompible(BlockType t, int i) {
    if (!esCompuesto(t)) return true;
    return componenteN(t, i) != COMP_FLUIDO;
}

// ============================================================================
// ⭐ FAMILIAS QUE SABEN DIBUJARSE
// ============================================================================
// El rango compuesto reserva hueco para familias que AUN NO EXISTEN
// (FAM_ARBOL_FRUTO, FAM_PIEDRA_MUSGO, FAM_PLANTA_FLOR). Reservarlo es
// correcto -- evita corromper saves al implementarlas -- pero deja un agujero:
//
//   un ID de una familia sin mesher NO entra en ninguna rama del dibujado
//   y CAE AL MESHER CUBICO. Resultado: un cubo con textura equivocada.
//
// Ese es exactamente el "cubo fantasma" que hay que impedir POR CONSTRUCCION,
// no tapando caras. La regla, en un solo sitio:
//
//   si la familia no sabe dibujarse -> NO SE DIBUJA NADA.
//
// Nunca "ID desconocido -> bloque por defecto -> cubo visible".
inline bool familiaTieneModelo(Familia f) {
    switch (f) {
        case FAM_MAGUEY:        return true;
        case FAM_BIZNAGA:       return true;
        case FAM_AGUA:          return true;
        case FAM_AGAVE_AZUL:    return true;
        case FAM_AGAVE_ABISAII: return true;
        case FAM_NOPAL:         return true;
        default:                return false;   // reservada, aun sin geometria
    }
}

// ============================================================================
// ⭐⭐ EL NOPAL: HASTA 7 PENCAS EXTRA EN LA MISMA CASILLA
// ============================================================================
// Ver la nota de FAM_NOPAL arriba para el porque. Aqui va el COMO.
//
// ----------------------------------------------------------------------------
// LA IDEA CENTRAL: SE GUARDAN DECISIONES, NO GEOMETRIA
// ----------------------------------------------------------------------------
// Una penca extra necesita, para dibujarse: direccion, angulo de giro,
// inclinacion, desplazamiento respecto al centro y tamano. Son cinco numeros
// continuos por penca; con 4 pencas no caben en 16 bits ni de lejos.
//
// La salida es la misma que usa CristalMineral.h: el ESTADO guarda lo que no
// se puede deducir (cuantas hay, hacia donde miran las tres primeras, que
// semilla les toco) y la FORMA se deriva con un hash de la posicion del bloque.
// El hash es determinista, asi que la planta se ve igual siempre y no hay nada
// que guardar ni que migrar.
namespace Nopal {

    // --- Reparto de los 16 bits ---
    //
    // ⚠️ Estos cinco campos suman exactamente 16. Anadir uno obliga a robarle
    // bits a otro, no a ensanchar el estado: BITS_ESTADO es comun a todas las
    // familias y subirlo corre los IDs de todas.
    constexpr Campo EXTRAS  = {  0, 3 };   // 0..7 pencas extra
    constexpr Campo DIR_A   = {  3, 3 };   // 0..5 cara por la que sale la 1a
    constexpr Campo DIR_B   = {  6, 3 };   // 0..5 la 2a
    constexpr Campo DIR_C   = {  9, 3 };   // 0..5 la 3a
    constexpr Campo SEMILLA = { 12, 3 };   // 0..7 variacion
    constexpr Campo ROLLO   = { 15, 1 };   // el tallo se enrolla

    constexpr uint16_t MAX_EXTRAS = 7;

    // ========================================================================
    // ⭐⭐ EL SEMIEJE DE UNA PENCA: UN SOLO NUMERO PARA DOS SITIOS
    // ========================================================================
    // Lo usan DOS codigos que tienen que estar de acuerdo o la cadena se rompe:
    //
    //   `colocar()`, aqui abajo, para ESPACIAR los eslabones (alto = 2*SEMIEJE)
    //   el mesher, en main.cpp, para DIBUJAR la caja (ly = sy * SEMIEJE)
    //
    // Si se separan, las pencas dejan de tocarse por el borde: o se solapan por
    // dentro o queda un hueco de luz entre ellas. Y no hay aviso -- compila,
    // dibuja, y se ve mal. Por eso vive aqui y no duplicado como literal.
    //
    // ----------------------------------------------------------------------
    // POR QUE ES TAN PEQUENO
    // ----------------------------------------------------------------------
    // Porque SIETE pencas encadenadas tienen que caber en UNA celda. El alto
    // total de la cadena es la suma de los altos menos los solapes:
    //
    //     7 pencas * 2 * SEMIEJE * escala_media  -  6 * SOLAPE
    //
    // Con SEMIEJE = 0.42 eso daba mas de tres celdas y el acotado aplastaba la
    // cadena contra el techo: medido, el 85% de las pencas saturaba el limite y
    // la mata se veia como un amasijo pegado arriba en vez de una columna.
    //
    // Con 0.075 la cadena entera mide ~0.7 de celda y cabe con margen.
    constexpr float SEMIEJE = 0.075f;

    // Las SEIS direcciones por las que puede brotar una penca. El indice es lo
    // que se guarda en DIR_A/B/C; el vector, lo que usa el mesher.
    //
    //   0 = +X este    1 = -X oeste   2 = +Y arriba
    //   3 = -Y abajo   4 = +Z norte   5 = -Z sur
    constexpr int DIRS[6][3] = {
        { 1, 0, 0}, {-1, 0, 0}, { 0, 1, 0},
        { 0,-1, 0}, { 0, 0, 1}, { 0, 0,-1}
    };

    // --- Construir y leer ---

    inline bool esNopal(BlockType t) {
        return esCompuesto(t) && familiaDe(t) == FAM_NOPAL;
    }

    // Un nopal con N pencas extra. Las direcciones van a 0 y se completan
    // luego con `conDireccion`, o se dejan al hash si no se fijan.
    inline BlockType nuevo(uint16_t extras, uint16_t semilla = 0,
                           bool rollo = true) {
        if (extras > MAX_EXTRAS) extras = MAX_EXTRAS;
        uint16_t est = 0;
        est = escribir(est, EXTRAS,  extras);
        est = escribir(est, SEMILLA, (uint16_t)(semilla & 7u));
        est = escribir(est, ROLLO,   rollo ? 1 : 0);
        return hacer(FAM_NOPAL, est);
    }

    inline uint16_t extrasDe(BlockType t) {
        if (!esNopal(t)) return 0;
        return leer(estadoDe(t), EXTRAS);
    }

    inline uint16_t semillaDe(BlockType t) {
        if (!esNopal(t)) return 0;
        return leer(estadoDe(t), SEMILLA);
    }

    inline bool tieneRollo(BlockType t) {
        if (!esNopal(t)) return false;
        return leer(estadoDe(t), ROLLO) != 0;
    }

    // La direccion de la penca extra `i`. Las tres primeras van guardadas; de
    // la cuarta en adelante se derivan del hash, que es lo que permite "mil
    // formas" sin gastar 3 bits por cada una.
    //
    // `hash` lo aporta el llamador a partir de la posicion del bloque (ver
    // `hashPos`), de modo que dos plantas con el mismo estado en casillas
    // distintas reparten sus pencas de otra manera.
    inline int direccionDe(BlockType t, int i, unsigned hash) {
        if (!esNopal(t)) return 0;
        const uint16_t est = estadoDe(t);
        if (i == 0) return (int)(leer(est, DIR_A) % 6u);
        if (i == 1) return (int)(leer(est, DIR_B) % 6u);
        if (i == 2) return (int)(leer(est, DIR_C) % 6u);
        // De la cuarta en adelante: el hash decide, mezclado con el indice
        // para que no salgan todas iguales.
        unsigned h = hash ^ (unsigned)(i * 2654435761u);
        h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
        return (int)(h % 6u);
    }

    // Fija la direccion de una de las tres primeras pencas. Mas alla de la
    // tercera no se puede: su rumbo es del hash, por diseno.
    inline BlockType conDireccion(BlockType t, int i, int dir) {
        if (!esNopal(t) || i < 0 || i > 2) return t;
        const Campo c = (i == 0) ? DIR_A : (i == 1) ? DIR_B : DIR_C;
        uint16_t est = escribir(estadoDe(t), c, (uint16_t)(dir % 6));
        return hacer(FAM_NOPAL, est);
    }

    // Anade una penca mas. Devuelve el mismo bloque si ya esta lleno, para que
    // el llamador pueda distinguir "cabia" de "no cabia" comparando.
    inline BlockType anadirPenca(BlockType t, int dir) {
        if (!esNopal(t)) return t;
        const uint16_t n = extrasDe(t);
        if (n >= MAX_EXTRAS) return t;
        uint16_t est = escribir(estadoDe(t), EXTRAS, (uint16_t)(n + 1));
        BlockType r = hacer(FAM_NOPAL, est);
        return (n < 3) ? conDireccion(r, (int)n, dir) : r;
    }

    // ------------------------------------------------------------------------
    // EL HASH DE POSICION
    // ------------------------------------------------------------------------
    // Mismo esquema que CuantosEn() en CristalMineral.h: tres primos grandes,
    // mezcla y avalancha. Determinista y barato.
    inline unsigned hashPos(int x, int y, int z, uint16_t semilla) {
        unsigned h = (unsigned)(x * 73856093) ^ (unsigned)(y * 19349663) ^
                     (unsigned)(z * 83492791) ^ (unsigned)(semilla * 2971215073u);
        h ^= h >> 13; h *= 1274126177u; h ^= h >> 16;
        return h;
    }

    // Un float 0..1 estable, sacado del hash. `canal` separa magnitudes
    // distintas (angulo, desplazamiento, tamano) para que no esten
    // correlacionadas entre si.
    inline float aleatorio01(unsigned hash, int canal) {
        unsigned h = hash ^ (unsigned)(canal * 0x9E3779B9u);
        h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
        return (float)(h % 100000u) / 100000.0f;
    }

    // ------------------------------------------------------------------------
    // LA COLOCACION DE UNA PENCA EXTRA
    // ------------------------------------------------------------------------
    // Todo lo que el mesher necesita para dibujarla, derivado del hash. Nada de
    // esto se guarda.
    struct Extra {
        float cx, cy, cz;   // centro dentro de la celda, 0..1
        float anguloY;      // giro en el plano horizontal, radianes
        float inclina;      // cuanto se tumba, -0.5..0.5
        float escala;       // 0.55..1.00 del tamano de la penca base
        int   dir;          // de que cara sale (indice en DIRS)
    };

    // ========================================================================
    // ⭐⭐ LA CADENA: CADA PENCA BROTA DEL BORDE DE LA ANTERIOR
    // ========================================================================
    // PEDIDO: "que varias pencas puedan estar en una sola casilla, y que crezcan
    // y se coloquen de tal modo que esten APILADAS, PEGADAS y ENLAZADAS
    // naturalmente".
    //
    // ------------------------------------------------------------------------
    // LO QUE HACIA ANTES, Y POR QUE NO SERVIA
    // ------------------------------------------------------------------------
    // La version anterior colocaba cada penca INDEPENDIENTEMENTE: la empujaba
    // hacia una cara del cubo y le sumaba una deriva lateral del hash. El
    // resultado era un ramillete de pencas flotando alrededor del centro, sin
    // tocarse -- "veinte objetos sueltos", no una planta. Cada una se sorteaba
    // sin mirar donde habia quedado la anterior, asi que no podian enlazarse.
    //
    // ------------------------------------------------------------------------
    // LO QUE HACE AHORA: UNA CADENA
    // ------------------------------------------------------------------------
    // La penca `i` nace del BORDE SUPERIOR de la penca `i-1`. Es la misma regla
    // botanica que ya aplica GeneradorNopal en render/NopalArborescente.h --un
    // cladodio brota del filo de otro, nunca de su centro-- y la que hace que
    // la mata se lea como UNA planta.
    //
    // El encadenado se consigue acumulando: se parte del centro de la celda y
    // se avanza, penca a penca, por el vector de salida de cada una. Como el
    // avance es la MITAD del alto de la penca anterior mas la mitad de la
    // nueva, las dos se tocan exactamente por el borde:
    //
    //     centro_i = centro_{i-1} + dir_i * (alto_{i-1} + alto_i) * 0.5
    //
    // Y para que no quede una junta visible, se resta un SOLAPE: la base de la
    // hija se mete un poco dentro de la madre. Es lo que pediste con "pegadas"
    // y lo que evita el hueco de luz entre dos piezas que deberian ser una.
    //
    // ------------------------------------------------------------------------
    // POR QUE SE SESGA HACIA ARRIBA
    // ------------------------------------------------------------------------
    // Una Opuntia crece contra la gravedad: las pencas nuevas salen del filo
    // SUPERIOR, no del de abajo. Sin ese sesgo la mata crecia hacia el suelo
    // tantas veces como hacia el cielo y parecia derramada. `DIRS[2]` es +Y, y
    // la tabla de abajo lo elige con mas probabilidad que el resto.
    //
    // ⚠️ SIGUE CABIENDO TODO EN LA CELDA. El acotado final no es decorativo:
    // una penca que se saliera invadiria la casilla del vecino y se dibujaria
    // dos veces (cada celda se dibuja desde si misma). Por eso la cadena se
    // comprime si hace falta en vez de desbordar.
    inline Extra colocar(BlockType t, int i, int wx, int wy, int wz) {
        Extra e{};
        const unsigned h0 = hashPos(wx, wy, wz, semillaDe(t));

        // ⭐ SE RECORRE LA CADENA DESDE EL PRINCIPIO.
        //
        // Hay que recalcular las pencas 0..i-1 porque la posicion de la `i`
        // depende de todas ellas. Es O(i) con i <= 7: unas pocas decenas de
        // operaciones, y a cambio no hace falta guardar ni un byte de la
        // cadena. La alternativa --cachear-- costaria memoria por bloque y
        // habria que invalidarla, que es justo lo que este diseno evita.
        float px = 0.5f, py = 0.5f, pz = 0.5f;   // centro de la penca actual
        float altoPrev = 0.0f;                    // alto de la anterior

        for (int k = 0; k <= i; ++k) {
            const unsigned h = h0 ^ (unsigned)((k + 1) * 0x85EBCA6Bu);

            // --- TAMANO DE ESTA PENCA ---
            // Se encoge con la profundidad de la cadena: la planta se afina
            // hacia las puntas, como un nopal real. 1.00 la primera, hasta
            // ~0.55 la septima.
            const float merma = 1.0f - 0.07f * (float)k;
            e.escala = (0.72f + aleatorio01(h, 5) * 0.28f) * merma;
            if (e.escala < 0.40f) e.escala = 0.40f;

            // ⭐ ALTO COMPLETO DE LA PENCA, y el factor 2 NO es decorativo.
            //
            // El mesher construye la caja con `ly = sy * sLargo` donde `sy` vale
            // -1 o +1 (ver `esq` en main.cpp): la penca se extiende de
            // `cy - sLargo` a `cy + sLargo`, asi que su alto REAL es el DOBLE
            // del semieje.
            //
            // Usar aqui el semieje a secas espaciaba la cadena a la mitad de lo
            // que mide la pieza, y las pencas se interpenetraban por dentro en
            // vez de tocarse por el borde. Es la clase de error que no da
            // ningun aviso: compila, dibuja, y se ve mal.
            //
            // SEMIEJE es la constante compartida con el mesher (ver su nota
            // arriba). No se repite el numero aqui a proposito.
            const float alto = 2.0f * SEMIEJE * e.escala;

            // --- HACIA DONDE SALE ---
            // Sesgada hacia arriba: ver la nota de la cabecera.
            e.dir = direccionDe(t, k, h0);
            if (k > 0) {
                // 45% de las veces manda el sesgo vertical, pase lo que pase
                // en los bits guardados. Es lo que endereza la mata.
                if (aleatorio01(h, 6) < 0.45f) e.dir = 2;   // +Y
            } else {
                // La PRIMERA siempre sube: es la que arranca la columna desde
                // la penca base, y arrancarla de lado dejaba la mata tumbada.
                e.dir = 2;
            }
            const int* d = DIRS[e.dir];

            // --- EL AVANCE QUE LAS DEJA PEGADAS ---
            //
            // Media penca anterior + media nueva, menos el solape. Con el
            // solape las dos geometrias se interpenetran un poco y la union
            // no deja ver luz entre ellas.
            // El solape es una FRACCION del alto, no un absoluto: con pencas de
            // 0.15 de celda, un solape fijo de 0.055 se tragaba un tercio de
            // cada eslabon. El 18% mantiene la union cerrada sin que una penca
            // desaparezca dentro de la anterior.
            const float SOLAPE = alto * 0.18f;
            const float avance = (altoPrev + alto) * 0.5f - SOLAPE;

            if (k > 0) {
                px += (float)d[0] * avance;
                py += (float)d[1] * avance;
                pz += (float)d[2] * avance;
            } else {
                // La base no avanza: se queda en el centro de la celda, que es
                // donde esta la penca principal con la que enlaza.
                py += alto * 0.5f - SOLAPE;
            }

            // --- DESVIO LATERAL, PEQUENO ---
            //
            // Lo justo para que la columna no sea una torre perfecta. Antes
            // esto valia +-0.18 y era lo que desperdigaba las pencas; ahora es
            // +-0.05, suficiente para que se note organico sin romper la
            // cadena.
            // ⚠️ PROPORCIONAL AL TAMANO DE LA PENCA, no absoluto. Un desvio
            // fijo mayor que la propia penca la saca de la cadena: dejaria de
            // tocar a su madre, que es justo lo que este sistema evita.
            const float dl1 = (aleatorio01(h, 1) - 0.5f) * alto * 0.35f;
            const float dl2 = (aleatorio01(h, 2) - 0.5f) * alto * 0.35f;
            if (d[0] != 0)      { py += dl1; pz += dl2; }
            else if (d[1] != 0) { px += dl1; pz += dl2; }
            else                { px += dl1; py += dl2; }

            altoPrev = alto;

            // --- GIRO E INCLINACION ---
            //
            // ⭐ EL GIRO NO ES LIBRE: SIGUE AL DE LA MADRE.
            //
            // Antes era `aleatorio01 * 2pi`, un giro completo al azar, y por eso
            // dos pencas encadenadas podian quedar cruzadas en angulo recto --
            // la union se veia como un aspa, no como una planta. Ahora cada una
            // hereda el rumbo de la cadena (`k * 0.6` abre el abanico poco a
            // poco) y solo se le suma un ruido corto.
            e.anguloY = (float)k * 0.6f + (aleatorio01(h, 3) - 0.5f) * 0.9f;

            // La inclinacion tambien se modera: una penca encadenada se vence
            // un poco, no se dobla del todo.
            e.inclina = (aleatorio01(h, 4) - 0.5f) * 0.45f;
        }

        // Que la cadena entera quepa en la celda. Si se sale, invadiria la del
        // vecino y se dibujaria dos veces.
        auto acotar = [](float v) {
            return v < 0.12f ? 0.12f : (v > 0.88f ? 0.88f : v);
        };
        e.cx = acotar(px); e.cy = acotar(py); e.cz = acotar(pz);

        return e;
    }

} // namespace Nopal

// ¿Este bloque compuesto se puede dibujar tal cual esta?
//
// Falla en dos casos, y los dos vienen de mundos guardados con una version
// distinta del juego:
//   1. familia reservada pero sin implementar todavia
//   2. familia fuera de rango (save de una version con mas familias)
inline bool puedeDibujarse(BlockType t) {
    if (!esCompuesto(t)) return true;          // no es asunto de este sistema
    const Familia f = familiaDe(t);
    if ((int)f < 0 || (int)f >= (int)FAM_COUNT) return false;
    return familiaTieneModelo(f);
}

} // namespace Compuesto
