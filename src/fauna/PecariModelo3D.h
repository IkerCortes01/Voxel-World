#pragma once

#include <cstdint>

// ============================================================================
// PECARI DE COLLAR (Dicotyles tajacu) -- MODELO 3D ANATOMICO
// ============================================================================
// La traduccion a geometria dibujable de 13_PECARI_ANATOMIA.json.
//
// Cuatro capas de la MISMA criatura: la piel por fuera, el esqueleto que la
// sostiene, los organos que van dentro y el encefalo que los manda. No son
// cuatro modelos sueltos -- estan encajados unos dentro de otros y hay
// static_assert al final que lo verifican.
//
// ----------------------------------------------------------------------------
// DE DONDE SALEN LOS NUMEROS
// ----------------------------------------------------------------------------
// De 13_PECARI_ANATOMIA.json, que sigue el protocolo del AI simulator: cada
// valor lleva confianza (MEDIDO / DERIVADO / INFERIDO / ESTIMADO / AUSENTE) y
// fuente cuando la hay.
//
// Aqui se marca cada constante con su confianza en comentario. Y conviene ser
// claro sobre el reparto, porque es la regla del sistema:
//
//   LO QUE ESTA MEDIDO es lo que de verdad define a la especie: que hay, que
//   falta, que esta fusionado y hacia donde crecen los colmillos. Eso es
//   solido y esta protegido por static_assert.
//
//   LOS NUMEROS INTERNOS son en su mayoria ESTIMADO. La literatura de
//   Tayassuidae describe la anatomia cualitativamente con mucho detalle, pero
//   no publica las coordenadas de un higado. Fingir lo contrario romperia la
//   regla del sistema, asi que van marcados sin disimulo.
//
// ----------------------------------------------------------------------------
// POR QUE HAY static_assert AL FINAL
// ----------------------------------------------------------------------------
// El error natural al modelar un animal "parecido a un cerdo" es deslizarse
// hacia el cerdo: ponerle cuatro dedos traseros, curvarle los colmillos,
// darle una vesicula biliar. Los static_assert convierten ese deslizamiento
// en un error de compilacion.
//
// Es la misma tecnica que BloqueCompuesto.h usa con COMPUESTO_BASE: si dos
// datos que tienen que coincidir dejan de hacerlo, el build para en vez de
// producir algo malo en silencio.
// ============================================================================

namespace Pecari {

// ----------------------------------------------------------------------------
// ESCALA
// ----------------------------------------------------------------------------
// La anatomia se declara en METROS para poder compararla con un libro de
// zoologia sin traducir nada. El motor mide en BLOQUES de 0.60 m
// (Fisica::LADO_M). La conversion ocurre solo aqui.

constexpr float LADO_BLOQUE_M = 0.60f;

constexpr float aBloques(float metros) { return metros / LADO_BLOQUE_M; }
constexpr float aMetros(float bloques) { return bloques * LADO_BLOQUE_M; }

// ----------------------------------------------------------------------------
// UNA CAJA ANATOMICA
// ----------------------------------------------------------------------------
// Toda pieza -- un hueso, una viscera, un trozo de lomo -- es una caja con
// CENTRO y medias-dimensiones.
//
// Se guarda el centro y no la esquina porque el cuerpo es simetrico respecto
// a su eje: con el centro, reflejar izquierda/derecha es negar cx, en vez de
// reajustar seis numeros a mano.
//
// Ejes:  +X derecha del animal   +Y arriba   +Z hacia el HOCICO
// Origen: punto medio entre las cuatro pezunas, a nivel del suelo.
struct Caja {
    float cx, cy, cz;      // centro, en metros desde el origen
    float hx, hy, hz;      // medias-dimensiones

    constexpr float ancho() const { return hx * 2.0f; }
    constexpr float alto()  const { return hy * 2.0f; }
    constexpr float largo() const { return hz * 2.0f; }

    constexpr float volumen() const { return ancho() * alto() * largo(); }

    constexpr float minX() const { return cx - hx; }
    constexpr float maxX() const { return cx + hx; }
    constexpr float minY() const { return cy - hy; }
    constexpr float maxY() const { return cy + hy; }
    constexpr float minZ() const { return cz - hz; }
    constexpr float maxZ() const { return cz + hz; }

    // ¿Cabe esta caja entera dentro de la otra? Es lo que comprueban los
    // static_assert de contencion: que ninguna viscera se salga de su
    // cavidad.
    constexpr bool dentroDe(const Caja& o) const {
        return minX() >= o.minX() && maxX() <= o.maxX() &&
               minY() >= o.minY() && maxY() <= o.maxY() &&
               minZ() >= o.minZ() && maxZ() <= o.maxZ();
    }

    // Reflejada al otro costado. Para los pares (pulmones, patas).
    constexpr Caja espejo() const {
        return Caja{ -cx, cy, cz, hx, hy, hz };
    }
};

// ============================================================================
// EL EJEMPLAR MODELADO
// ============================================================================
// Un adulto de tamano medio, no un maximo excepcional.

namespace Escala {
    // MEDIDO -- PMID 3803989 (Growth, 1986), n=16
    constexpr float MASA_KG = 18.7f;

    // DERIVADO -- centro del rango MEDIDO 84-106 cm (Animal Diversity Web)
    constexpr float LARGO_CABEZA_CUERPO_M = 0.95f;

    // DERIVADO -- del rango MEDIDO 30-50 cm. Se toma un valor alto-medio por
    // coherencia con una longitud de 95 cm; el rango publicado es muy ancho y
    // probablemente mezcla subadultos.
    constexpr float ALTO_CRUZ_M = 0.44f;

    // MEDIDO -- Animal Diversity Web. 1.2 cm: practicamente inexistente.
    // Es un rasgo visible y el modelo NO debe darle cola de cerdo.
    constexpr float LARGO_COLA_M = 0.012f;

    // DERIVADO -- centro del rango MEDIDO 3-10 cm
    constexpr float LARGO_OREJA_M = 0.065f;
}

// ============================================================================
// CAPA 1 -- FORMA EXTERNA
// ============================================================================

namespace Externo {

// --- EL TRONCO: el barril ---
//
// La pieza que define al animal. "En forma de barril" no es adorno: es mas
// ancho por el centro que por los extremos porque ahi dentro va el
// proestomago, que es voluminoso porque tiene que fermentar celulosa.
//
// ESTIMADO (el largo es DERIVADO de la longitud cabeza-cuerpo MEDIDA)
constexpr Caja TRONCO = { 0.000f, 0.315f, 0.000f, 0.125f, 0.125f, 0.245f };

// Cuanto encoge la seccion en los extremos respecto al centro. Es lo que
// hace que sea un barril y no un ladrillo.  ESTIMADO
constexpr float ESTRECHAMIENTO_EXTREMOS = 0.78f;

// --- LA CABEZA: grande y en cuna ---
// Proporcionalmente grande (cualitativo MEDIDO), afilada hacia el hocico.
// ESTIMADO
constexpr Caja CABEZA = { 0.000f, 0.345f, 0.315f, 0.095f, 0.105f, 0.135f };

// La punta: mucho mas estrecha que el craneo. Es el organo de trabajo --
// escarba y huele.  ESTIMADO
constexpr Caja HOCICO = { 0.000f, 0.295f, 0.485f, 0.052f, 0.050f, 0.050f };

// El afilado no es un numero elegido: es el cociente entre los dos anchos.
constexpr float AFILADO_CABEZA = HOCICO.hx / CABEZA.hx;   // ~0.55

// --- LAS PATAS: largas y delgadas sobre un cuerpo macizo ---
//
// El contraste es una firma visual de la especie. Y explica la radioulna
// fusionada: una pata fina que corre y cava necesita rigidez.
constexpr float PATA_GROSOR_M     = 0.048f;   // ESTIMADO
constexpr float PATA_LARGO_M      = 0.200f;   // DERIVADO: cruz - media panza
constexpr float PATA_SEPARACION_X = 0.082f;   // ESTIMADO
constexpr float PATA_DELANTERA_Z  = 0.165f;   // ESTIMADO
constexpr float PATA_TRASERA_Z    = -0.175f;  // ESTIMADO

// --- LOS DEDOS: la diferencia que se ve desde fuera ---
//
// UNA DE LAS SENAS DE IDENTIDAD. El cerdo verdadero tiene CUATRO dedos
// traseros. El pecari tiene TRES. (El del Chaco, Catagonus wagneri, dos.)
//
// Van como datos y no como comentario porque el modelo DIBUJA los dedos que
// diga este numero: anadir el pecari del Chaco es cambiar una constante.
constexpr int DEDOS_DELANTEROS      = 4;   // MEDIDO: 2 apoyan, 2 reducidos
constexpr int DEDOS_TRASEROS        = 3;   // MEDIDO <-- LA DIFERENCIA
constexpr int DEDOS_TRASEROS_CHACO  = 2;   // MEDIDO (Catagonus wagneri)
constexpr int DEDOS_QUE_APOYAN      = 2;   // MEDIDO: solo los centrales

// Pezunas "angostas y funcionales": nada que ver con el casco ancho de un
// ungulado de pradera.  ESTIMADO
constexpr float PEZUNA_ANCHO_M = 0.020f;
constexpr float PEZUNA_ALTO_M  = 0.028f;

// --- LA CRESTA DORSAL ERECTIL ---
//
// Una linea de cerdas a lo largo del dorso que el animal LEVANTA al
// alarmarse. Erizada, parece mucho mas grande. Es senalizacion, no abrigo.
//
// Por eso no es una pieza fija: es un parametro animado (ver Estado).
constexpr float CRESTA_ALTURA_REPOSO_M  = 0.020f;  // ESTIMADO
constexpr float CRESTA_ALTURA_ERIZADA_M = 0.070f;  // ESTIMADO (~3.5x)
constexpr float CRESTA_LARGO_M          = 0.440f;  // ESTIMADO
constexpr float CRESTA_ANCHO_M          = 0.032f;  // ESTIMADO

// Rapido: es una reaccion de alarma, no un gesto lento.  ESTIMADO
constexpr float CRESTA_TIEMPO_ERIZADO_S = 0.25f;

// Se eriza deprisa y baja despacio. Alarmarse es inmediato, calmarse cuesta:
// es el patron de la respuesta de sobresalto en presas.  ESTIMADO
constexpr float CRESTA_FACTOR_BAJADA = 0.35f;

// --- LA GLANDULA DE ALMIZCLE DORSAL ---
//
// LA CARACTERISTICA UNICA de la familia, y la infraestructura de su vida
// social. Segrega un aceite de olor fortisimo; el animal se frota contra
// arboles y piedras para marcar territorio, y los miembros de la piara se
// frotan entre si cabeza-contra-grupa para compartir olor. Asi se reconocen.
//
// SU POSICION IMPORTA: va sobre las ANCAS, no en mitad del lomo. Ponerla en
// el centro del dorso seria un error del modelo.
// Posicion MEDIDA (cualitativa); dimensiones ESTIMADO.
constexpr Caja GLANDULA_ALMIZCLE = { 0.000f, 0.435f, -0.145f, 0.030f, 0.011f, 0.042f };

// ESTIMADO -- el olor es notoriamente detectable a distancia, pero no hay
// medicion publicada de un radio.
constexpr float ALMIZCLE_RADIO_PERCEPCION_M = 45.0f;

// --- LOS COLMILLOS: rectos, no curvos ---
//
// LA OTRA SENA DE IDENTIDAD, y la que mas se confunde. El jabali los tiene
// CURVADOS HACIA ARRIBA, saliendo de la boca. El pecari los tiene RECTOS,
// creciendo VERTICALMENTE HACIA ABAJO, pegados a la mandibula y casi
// invisibles con la boca cerrada.
//
// Y hacen algo que el jabali no puede: al cerrar la boca el superior FROTA
// contra el inferior, asi que se afilan solos cada vez que el animal mastica.
//
// 0 grados = perfectamente vertical. Se declara explicito porque es
// exactamente lo que lo distingue.  MEDIDO
constexpr float COLMILLO_ANGULO_GRADOS = 0.0f;    // el jabali seria ~+55
constexpr float COLMILLO_LARGO_M       = 0.038f;  // ESTIMADO
constexpr float COLMILLO_GROSOR_M      = 0.010f;  // ESTIMADO

// MEDIDO -- depende del surco oseo del maxilar (ver Esqueleto).
constexpr bool COLMILLOS_SE_AUTOAFILAN = true;

// --- PELAJE ---
// Cerdas largas, gruesas y rigidas: casi puas flexibles. De ahi que el animal
// se vea erizado incluso en calma.  ESTIMADO
constexpr float CERDA_LARGO_M  = 0.050f;
constexpr float CERDA_GROSOR_M = 0.0016f;

// El collar claro que cruza el hombro y da nombre a la especie.
// Existencia MEDIDA y diagnostica; dimensiones ESTIMADO.
constexpr float COLLAR_ANCHO_M = 0.042f;
constexpr float COLLAR_Z       = 0.135f;

// MEDIDO -- 38 dientes, formula 2133/3133
constexpr int TOTAL_DIENTES = 38;

} // namespace Externo

// ============================================================================
// CAPA 2 -- ESQUELETO
// ============================================================================
// Similar al del cerdo salvo en cuatro adaptaciones, y esas cuatro son justo
// lo que lo convierte en un corredor de terreno duro.

namespace Esqueleto {

constexpr Caja CRANEO = { 0.000f, 0.350f, 0.305f, 0.075f, 0.080f, 0.115f };  // ESTIMADO

// EL SURCO OSEO DEL CANINO.
//
// En el maxilar superior hay una acanaladura labrada en el hueso donde ENCAJA
// el canino inferior al cerrar la boca. Es la pieza mecanica que hace posible
// el autoafilado: sin el surco, los colmillos chocarian en vez de deslizar.
//
// No es cosmetico -- es la razon de que COLMILLOS_SE_AUTOAFILAN pueda ser true.
constexpr bool  SURCO_CANINO_EXISTE      = true;    // MEDIDO
constexpr float SURCO_CANINO_PROFUND_M   = 0.005f;  // ESTIMADO
constexpr float SURCO_CANINO_LARGO_M     = 0.026f;  // ESTIMADO

// EL CANAL AUDITIVO OCULTO.
//
// El conducto del oido no sale al exterior por donde cabria esperar: pasa por
// un canal OSEO metido entre los huesos del arco cigomatico (el pomulo).
// Queda protegido de espinas y ramas -- lo que conviene a un animal que vive
// metiendo la cabeza en matorral cerrado.
constexpr bool  CANAL_AUDITIVO_EN_CIGOMATICO = true;   // MEDIDO
constexpr float ARCO_CIGOMATICO_LARGO_M      = 0.058f; // ESTIMADO

// --- COLUMNA ---
// INFERIDO desde suiformes: la formula vertebral concreta de Dicotyles tajacu
// no se localizo. Se marca como inferencia en vez de presentarla como dato de
// la especie.
constexpr int VERTEBRAS_CERVICALES = 7;    // INFERIDO (constante en mamiferos)
constexpr int VERTEBRAS_TORACICAS  = 14;   // INFERIDO
constexpr int VERTEBRAS_LUMBARES   = 5;    // INFERIDO
constexpr int PARES_COSTILLAS      = 14;   // DERIVADO: una por toracica

// La caja toracica: lo que da al barril su forma de barril.  ESTIMADO
constexpr Caja CAJA_TORACICA = { 0.000f, 0.320f, 0.075f, 0.108f, 0.108f, 0.155f };

// --- LA RADIOULNA: el hueso fusionado ---
//
// LA ADAPTACION OSEA MAS CARACTERISTICA del miembro anterior. En casi todos
// los mamiferos el antebrazo tiene DOS huesos -- radio y ulna (cubito) -- que
// giran uno sobre otro: es lo que te deja poner la palma arriba y abajo.
//
// El pecari los tiene COMPLETAMENTE FUSIONADOS en una pieza solida. Es un
// intercambio deliberado: pierde la rotacion, gana rigidez. Un hueso entero
// no se tuerce al golpear el suelo corriendo ni al escarbar raices duras.
//
// Se declara como UNA caja precisamente porque es UN hueso, no dos.
constexpr bool RADIO_Y_ULNA_FUSIONADOS = true;   // MEDIDO
constexpr int  HUESOS_EN_ANTEBRAZO     = 1;      // MEDIDO (serian 2 en un cerdo)

constexpr Caja RADIOULNA = { 0.082f, 0.205f, 0.165f, 0.017f, 0.068f, 0.017f };  // ESTIMADO

// --- LOS METAPODIOS FUSIONADOS ---
//
// Los metapodios son los huesos largos de mano y pie. Los del tercer y cuarto
// dedo -- los dos centrales, los que apoyan -- estan fusionados entre si.
//
// Misma logica que la radioulna: dos huesos sueltos se desalinean bajo carga,
// uno solo no. Los ungulados corredores llegan a esta solucion una y otra vez
// de forma independiente (en los ciervos el hueso equivalente es la "cana").
constexpr bool METAPODIOS_3_4_FUSIONADOS = true;   // MEDIDO
constexpr bool CARPO_TARSO_SOLDADOS      = true;   // MEDIDO

constexpr Caja METAPODIO_FUSIONADO = { 0.082f, 0.098f, 0.165f, 0.014f, 0.050f, 0.014f };  // ESTIMADO

constexpr Caja PELVIS = { 0.000f, 0.325f, -0.175f, 0.085f, 0.055f, 0.068f };  // ESTIMADO

} // namespace Esqueleto

// ============================================================================
// CAPA 3 -- ORGANOS INTERNOS
// ============================================================================
// Aqui el pecari deja de parecerse al cerdo del todo. Por fuera es
// discutible; por dentro no hay discusion posible.

namespace Organos {

// --- EL ESTOMAGO COMPLEJO ---
//
// LA GRAN DIFERENCIA. Un cerdo tiene un estomago SIMPLE: una bolsa. El pecari
// lo tiene dividido en TRES O CUATRO CAMARAS -- dos sacos ciegos no
// glandulares, una bolsa gastrica y el estomago posterior glandular.
//
// No es un rumiante: no regurgita ni rumia. Pero hace algo parecido antes del
// estomago glandular -- fermentacion microbiana PRE-GASTRICA. Hay bacterias
// en los sacos ciegos que rompen la CELULOSA antes de que la comida llegue al
// acido.
//
// Y eso explica su dieta: raices lenosas, frutos duros y NOPAL. Un estomago
// simple no saca energia de una penca de nopal; este si. Es la razon
// fisiologica de que prospere en zona semiarida -- justo donde este motor ya
// tiene nopales, biznagas y magueyes.
constexpr int  CAMARAS_ESTOMACALES_MIN   = 3;      // MEDIDO
constexpr int  CAMARAS_ESTOMACALES_MAX   = 4;      // MEDIDO
constexpr bool FERMENTACION_PREGASTRICA  = true;   // MEDIDO
constexpr bool ES_RUMIANTE_VERDADERO     = false;  // MEDIDO: fermenta, no rumia

// La envolvente tiene que cubrir las CUATRO camaras, incluida la glandular,
// que es la ultima del recorrido y por eso la que mas atras queda. Ajustada
// para contenerla: el static_assert de mas abajo lo verifica.
constexpr Caja ESTOMAGO = { -0.014f, 0.285f, 0.012f, 0.080f, 0.062f, 0.105f };  // ESTIMADO

// Las camaras. Los dos sacos ciegos NO son simetricos: la asimetria es real
// en la anatomia descrita, aunque estas dimensiones sean reconstruccion.
constexpr Caja SACO_CIEGO_IZQUIERDO = { -0.055f, 0.298f,  0.058f, 0.032f, 0.040f, 0.044f };  // ESTIMADO
constexpr Caja SACO_CIEGO_DERECHO   = {  0.034f, 0.298f,  0.058f, 0.029f, 0.038f, 0.041f };  // ESTIMADO
constexpr Caja BOLSA_GASTRICA       = { -0.018f, 0.272f, -0.004f, 0.050f, 0.045f, 0.050f };  // ESTIMADO
constexpr Caja ESTOMAGO_GLANDULAR   = { -0.009f, 0.264f, -0.062f, 0.043f, 0.036f, 0.029f };  // ESTIMADO

// --- EL HIGADO, Y LO QUE LE FALTA ---
//
// EL PECARI NO TIENE VESICULA BILIAR. Es una ausencia limpia y uno de los
// criterios que separan Tayassuidae de Suidae sin ambiguedad.
//
// El higado sigue produciendo bilis; lo que no hay es donde almacenarla, asi
// que se vierte de forma CONTINUA al intestino delgado en vez de soltarse a
// golpes cuando llega una comida grasa. Encaja con su dieta: un herbivoro que
// come poco y a menudo no necesita un deposito para un banquete ocasional.
//
// EL MODELO TIENE QUE REPRESENTAR ESTA AUSENCIA ACTIVAMENTE, porque el error
// natural al modelar "algo parecido a un cerdo" es incluirla.
constexpr bool TIENE_VESICULA_BILIAR      = false;  // MEDIDO. Un cerdo si tiene.
constexpr bool BILIS_CONTINUA_AL_DUODENO  = true;   // MEDIDO

constexpr Caja HIGADO = { 0.040f, 0.318f, 0.105f, 0.056f, 0.043f, 0.050f };  // ESTIMADO

constexpr Caja CORAZON          = { -0.010f, 0.330f, 0.132f, 0.032f, 0.036f, 0.029f };  // ESTIMADO
constexpr Caja PULMON_IZQUIERDO = { -0.062f, 0.348f, 0.115f, 0.038f, 0.048f, 0.070f };  // ESTIMADO
constexpr Caja PULMON_DERECHO   = {  0.062f, 0.348f, 0.115f, 0.038f, 0.048f, 0.070f };  // ESTIMADO
constexpr Caja INTESTINO        = {  0.013f, 0.252f, -0.068f, 0.095f, 0.048f, 0.105f }; // ESTIMADO

// --- APARATO UROGENITAL ---
// Tiene rasgos exclusivos de la familia: seno urogenital diferenciado y
// glandulas vestibulares propias. Se declara la envolvente sin detallar mas:
// el modelo no lo dibuja en detalle, pero ocupa sitio y entra en el reparto
// del abdomen.
constexpr bool SENO_UROGENITAL_DIFERENCIADO   = true;  // MEDIDO
constexpr bool GLANDULAS_VESTIBULARES_PROPIAS = true;  // MEDIDO

constexpr Caja UROGENITAL = { 0.000f, 0.248f, -0.158f, 0.040f, 0.030f, 0.042f };  // ESTIMADO

} // namespace Organos

// ============================================================================
// CAPA 4 -- ENCEFALO
// ============================================================================
// Un cerebro construido alrededor de dos capacidades: OLER y RECORDAR DONDE.

namespace Encefalo {

constexpr Caja ENCEFALO = { 0.000f, 0.362f, 0.288f, 0.038f, 0.032f, 0.050f };  // ESTIMADO

// --- LOS BULBOS OLFATORIOS: desproporcionados ---
//
// La region que procesa olores es PROPORCIONALMENTE ENORME. En un primate los
// bulbos olfatorios son una esquirla junto al resto del cerebro; aqui son una
// parte principal de la pieza.
//
// Tiene sentido, porque el olfato le resuelve tres problemas a la vez:
//   - reconocer a los suyos por la marca de almizcle de la piara
//   - encontrar raices y tuberculos ENTERRADOS, que no se ven
//   - oler al depredador antes de tenerlo encima
//
// Que sean desproporcionadamente grandes es MEDIDO. La fraccion concreta no
// se localizo publicada: el 0.15 es reconstruccion para dar escala.
constexpr Caja BULBOS_OLFATORIOS = { 0.000f, 0.356f, 0.325f, 0.024f, 0.017f, 0.027f };  // ESTIMADO

constexpr float FRACCION_OLFATORIA = 0.15f;   // ESTIMADO

// ESTIMADO -- sin medicion publicada de umbral olfativo en la especie.
constexpr float ALCANCE_OLFATO_M            = 120.0f;
constexpr int   OLFATO_PROFUNDIDAD_BLOQUES  = 3;

// --- CORTEZA GIRENCEFALICA ---
//
// El cerebro tiene GIROS y SURCOS: pliegues. Un cerebro liso (lisencefalico,
// como el de una rata) tiene poca superficie; plegarlo mete mucha mas corteza
// en el mismo craneo. Es lo que sostiene el comportamiento complejo.
constexpr bool ES_GIRENCEFALICO   = true;  // MEDIDO
constexpr int  SURCOS_PRINCIPALES = 6;     // ESTIMADO (para el modelo visual)

// --- MEMORIA ESPACIAL ---
//
// Las regiones de memoria a largo plazo y navegacion estan destacadas. Una
// piara recuerda MAPAS MENTALES exactos del territorio: donde hay agua
// estacional, que arbol da fruto y cuando, que rutas son seguras. Y lo
// recuerda durante ANOS.
//
// La capacidad es MEDIDA (cualitativa); estos numeros son la traduccion a
// simulacion, y son de diseno.
constexpr int   PUNTOS_MEMORIZADOS_MAX = 24;      // ESTIMADO
constexpr float MEMORIA_DURACION_DIAS  = 365.0f;  // ESTIMADO

constexpr Caja HIPOCAMPO = { 0.000f, 0.360f, 0.275f, 0.018f, 0.013f, 0.022f };  // ESTIMADO

// --- VIDA SOCIAL ---
//
// Estrictamente gregario: no existe el pecari solitario.
//
// OJO CON LA JERARQUIA. El encargo mencionaba "jerarquias de dominancia",
// pero 10_PECARI_BIOLOGIA y 00_LEEME establecen que NO hay macho alfa ni
// jerarquia lineal documentada en Dicotyles tajacu, y el monomorfismo sexual
// MEDIDO lo corrobora por otra via. Ante conflicto entre el texto de encargo
// y un dato MEDIDO del sistema, manda el dato: aqui se conservan las
// vocalizaciones y la respuesta coordinada, y NO se implementa jerarquia.
constexpr bool HAY_JERARQUIA_LINEAL = false;   // MEDIDO (la ausencia)

enum class Vocalizacion : uint8_t {
    GRUNIDO_CONTACTO = 0,   // "sigo aqui": cohesion de la piara
    CASTANETEO,             // colmillos: amenaza, precede a la carga
    LADRIDO_ALARMA,         // depredador visto
    CHILLIDO                // dolor o captura
};

constexpr int VOCALIZACIONES_DISTINTAS = 4;   // INFERIDO (simplificacion)

// ESTIMADO -- alcances de diseno, sin medicion publicada.
constexpr float ALCANCE_GRUNIDO_M    = 25.0f;
constexpr float ALCANCE_CASTANETEO_M = 40.0f;
constexpr float ALCANCE_LADRIDO_M    = 150.0f;   // la alarma tiene que llegar lejos
constexpr float ALCANCE_CHILLIDO_M   = 200.0f;

} // namespace Encefalo

// ============================================================================
// LAS AMARRAS
// ============================================================================
// Cada viscera tiene que caber en la cavidad que la aloja, y cada hueso en el
// miembro que lo contiene. Si alguien retoca una medida sin mirar las demas,
// el build se para AQUI en vez de producir un animal con el higado fuera del
// cuerpo.

// --- Los organos, dentro del tronco ---
static_assert(Organos::ESTOMAGO.dentroDe(Externo::TRONCO),
              "El estomago se sale del cuerpo: agranda el TRONCO o encoge el ESTOMAGO");
static_assert(Organos::HIGADO.dentroDe(Externo::TRONCO),
              "El higado se sale del cuerpo");
static_assert(Organos::CORAZON.dentroDe(Externo::TRONCO),
              "El corazon se sale del cuerpo");
static_assert(Organos::PULMON_IZQUIERDO.dentroDe(Externo::TRONCO),
              "El pulmon izquierdo se sale del cuerpo");
static_assert(Organos::PULMON_DERECHO.dentroDe(Externo::TRONCO),
              "El pulmon derecho se sale del cuerpo");
static_assert(Organos::INTESTINO.dentroDe(Externo::TRONCO),
              "El intestino se sale del cuerpo");
static_assert(Organos::UROGENITAL.dentroDe(Externo::TRONCO),
              "El aparato urogenital se sale del cuerpo");

// --- Las camaras, dentro del estomago ---
static_assert(Organos::SACO_CIEGO_IZQUIERDO.dentroDe(Organos::ESTOMAGO),
              "El saco ciego izquierdo se sale del estomago");
static_assert(Organos::SACO_CIEGO_DERECHO.dentroDe(Organos::ESTOMAGO),
              "El saco ciego derecho se sale del estomago");
static_assert(Organos::BOLSA_GASTRICA.dentroDe(Organos::ESTOMAGO),
              "La bolsa gastrica se sale del estomago");
static_assert(Organos::ESTOMAGO_GLANDULAR.dentroDe(Organos::ESTOMAGO),
              "El estomago glandular se sale del estomago");

// --- El esqueleto, dentro de su envoltura ---
static_assert(Esqueleto::CAJA_TORACICA.dentroDe(Externo::TRONCO),
              "La caja toracica se sale del cuerpo");
static_assert(Esqueleto::PELVIS.dentroDe(Externo::TRONCO),
              "La pelvis se sale del cuerpo");
static_assert(Esqueleto::CRANEO.dentroDe(Externo::CABEZA),
              "El craneo se sale de la cabeza");

// --- El encefalo, dentro del craneo ---
static_assert(Encefalo::ENCEFALO.dentroDe(Esqueleto::CRANEO),
              "El encefalo no cabe en el craneo");
static_assert(Encefalo::BULBOS_OLFATORIOS.dentroDe(Esqueleto::CRANEO),
              "Los bulbos olfatorios no caben en el craneo");
static_assert(Encefalo::HIPOCAMPO.dentroDe(Esqueleto::CRANEO),
              "El hipocampo no cabe en el craneo");

// ----------------------------------------------------------------------------
// LAS SENAS DE IDENTIDAD
// ----------------------------------------------------------------------------
// Las cinco que separan al pecari del cerdo. Se comprueban en compilacion
// para que nadie las "corrija" por descuido hacia los valores del cerdo, que
// es el error natural al escribir un animal parecido.

static_assert(Externo::DEDOS_TRASEROS == 3,
              "El pecari tiene TRES dedos traseros; cuatro seria un cerdo");
static_assert(Esqueleto::HUESOS_EN_ANTEBRAZO == 1,
              "Radio y ulna van FUSIONADOS: un solo hueso en el antebrazo");
static_assert(!Organos::TIENE_VESICULA_BILIAR,
              "El pecari NO tiene vesicula biliar: es una ausencia diagnostica");
static_assert(Organos::CAMARAS_ESTOMACALES_MIN >= 3,
              "El estomago del pecari es complejo, no una bolsa simple de cerdo");
static_assert(Externo::COLMILLO_ANGULO_GRADOS == 0.0f,
              "Los colmillos del pecari son RECTOS; curvados serian de jabali");

// El autoafilado depende del surco oseo: sin el, los colmillos chocarian.
static_assert(!Externo::COLMILLOS_SE_AUTOAFILAN || Esqueleto::SURCO_CANINO_EXISTE,
              "No puede haber autoafilado sin el surco oseo del maxilar");

// --- Coherencia visual ---
static_assert(Externo::CRESTA_ALTURA_ERIZADA_M > Externo::CRESTA_ALTURA_REPOSO_M * 2.0f,
              "La cresta erizada apenas se nota: la senal de alarma no se veria");
static_assert(Externo::HOCICO.hx < Externo::CABEZA.hx,
              "El hocico no es mas estrecho que la cabeza: no se afila");
static_assert(Escala::LARGO_COLA_M < 0.05f,
              "La cola del pecari es practicamente inexistente, no una cola de cerdo");

// La cresta no puede ser mas larga que el lomo que la sostiene.
static_assert(Externo::CRESTA_LARGO_M <= Externo::TRONCO.largo(),
              "La cresta es mas larga que el tronco");

// ============================================================================
// ESTADO VIVO
// ============================================================================
// Lo que cambia mientras el animal existe. Se separa de la anatomia (que es
// constante) para que el modelo se pueda dibujar sin instanciar un animal.

// En que esta el animal ahora mismo.
enum class Actividad : uint8_t {
    PASTANDO = 0,   // cabeza baja, comiendo: el estado por defecto
    CAMINANDO,      // sigue a la piara
    ESCARBANDO,     // hocico bajo tierra buscando raices
    ALERTA,         // ha olido u oido algo: la cresta sube
    HUYENDO,        // la piara se dispersa
    AMENAZANDO,     // castanetea los colmillos y encara
    MARCANDO,       // se frota la glandula contra un arbol
    DESCANSANDO
};

// Que capa se esta mostrando. Es lo que convierte el modelo en una lamina
// anatomica: la misma criatura, vista por dentro.
enum class Capa : uint8_t {
    EXTERNO = 0,    // piel, cerdas, cresta
    ESQUELETO,      // los huesos y sus fusiones
    ORGANOS,        // visceras, con el estomago de camaras
    NERVIOSO        // encefalo, bulbos, hipocampo
};

struct Estado {
    Actividad actividad = Actividad::PASTANDO;

    // Cresta: 0 = pegada al lomo, 1 = completamente erizada. Continuo porque
    // se anima; no es un si/no.
    float erizado = 0.0f;

    // Fase del paso, para animar las patas. Radianes.
    float fasePaso = 0.0f;

    // Cuanto olor de su piara lleva encima. Baja con el tiempo y sube al
    // frotarse con otro miembro. Si llega a cero, los demas lo tratan como
    // extrano -- que es como funciona de verdad.
    float marcaAlmizcle = 1.0f;

    // Altura actual de la cresta, en metros. Derivada de erizado.
    float alturaCresta() const {
        const float a = erizado < 0.0f ? 0.0f : (erizado > 1.0f ? 1.0f : erizado);
        return Externo::CRESTA_ALTURA_REPOSO_M +
               a * (Externo::CRESTA_ALTURA_ERIZADA_M - Externo::CRESTA_ALTURA_REPOSO_M);
    }

    // Se eriza deprisa y baja despacio: alarmarse es inmediato, calmarse
    // cuesta. Asi se comporta un animal de presa.
    void actualizarCresta(float dt, bool alarmado) {
        const float subida = dt / Externo::CRESTA_TIEMPO_ERIZADO_S;
        if (alarmado) {
            erizado += subida;
            if (erizado > 1.0f) erizado = 1.0f;
        } else {
            erizado -= subida * Externo::CRESTA_FACTOR_BAJADA;
            if (erizado < 0.0f) erizado = 0.0f;
        }
    }
};

// ============================================================================
// DERIVADOS PARA EL MOTOR
// ============================================================================
// Lo que hace falta para colisionar y colocar al animal. Sale de la anatomia.
//
// OJO: si 11_PECARI_IA define velocidades propias, MANDAN LAS DE ALLI. Estas
// existen para que el modelo se pueda mover solo, no para competir con la
// capa de IA como fuente de verdad.

namespace Motor {

// DERIVADO de las cajas anatomicas, con holgura.
constexpr float HITBOX_ANCHO_BLOQUES = 0.55f;
constexpr float HITBOX_ALTO_BLOQUES  = 0.85f;
constexpr float HITBOX_LARGO_BLOQUES = 1.58f;

// ESTIMADO -- no se localizaron velocidades medidas para la especie.
constexpr float VELOCIDAD_PASTANDO = 0.35f;
constexpr float VELOCIDAD_TROTE    = 5.1f;    // ~11 km/h
constexpr float VELOCIDAD_HUIDA    = 16.2f;   // ~35 km/h

} // namespace Motor

// La hitbox tiene que envolver al animal, no recortarlo.
static_assert(aMetros(Motor::HITBOX_LARGO_BLOQUES) >= Escala::LARGO_CABEZA_CUERPO_M - 0.01f,
              "La hitbox es mas corta que el animal");
static_assert(aMetros(Motor::HITBOX_ALTO_BLOQUES) >= Escala::ALTO_CRUZ_M,
              "La hitbox es mas baja que la cruz del animal");

} // namespace Pecari
