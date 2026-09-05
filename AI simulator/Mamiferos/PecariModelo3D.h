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

// POR QUE NO HAY SIMULACION DE PELO AQUI
//
// Tres razones que apuntan al mismo sitio:
//
//   1. BIOLOGICA. Estas cerdas no son pelo, son puas de 1.6 mm de grosor.
//      No ondean, no se aplastan, no fluyen. Un sistema de fur dinamico
//      (shells, strands, fisica de mechones) simularia un movimiento que el
//      animal real NO TIENE. Seria caro y ademas falso.
//
//   2. DE ESCALA. El motor mide en bloques de 0.60 m. Una cerda de 5 cm es
//      sub-texel: no hay resolucion donde dibujarla.
//
//   3. FUNCIONAL. Todo lo que este pelaje COMUNICA esta medido y es de baja
//      dimensionalidad: se eriza, se aclara en verano, se ensucia, y lleva
//      un collar diagnostico. Cuatro senales, no un campo continuo.
//
// Asi que el pelaje se modela como ESTADO DE SUPERFICIE: cuatro bytes que
// modulan el color al sombrear. Ver PelajeEmpaquetado, mas abajo.

// El collar claro que cruza el hombro y da nombre a la especie.
// Existencia MEDIDA y diagnostica; dimensiones ESTIMADO.
//
// NO ES DECORACION: es el rasgo que da NOMBRE a la especie y el que la
// distingue de un jabali de un vistazo. Un pecari de collar sin collar
// dibujado no es un detalle omitido, es una identificacion equivocada.
// Se resuelve en la textura o con un desplazamiento de paleta: coste CERO.
constexpr float COLLAR_ANCHO_M = 0.042f;
constexpr float COLLAR_Z       = 0.135f;

// --- LA MUDA ESTACIONAL ---
//
// MEDIDO (Zervanos y Hadley 1973): en verano el pelaje SE ACLARA y baja la
// densidad de cerdas.
//
// Importa mas de lo que parece. Este animal NO PUEDE JADEAR: no tiene forma
// de evaporar humedad por la boca. Su termorregulacion es casi toda
// conductual --sombra, cueva, revolcadero-- y esta muda es el UNICO ajuste
// fisiologico que le queda. Es la mitad pasiva de un problema que por lo
// demas resuelve moviendose.
//
// La DIRECCION del cambio esta MEDIDA. Las MAGNITUDES de abajo son ESTIMADO:
// la fuente no publica cuanto se aclara.
constexpr float MUDA_ACLARADO_VERANO    = 0.22f;  // ESTIMADO
constexpr float MUDA_REDUCCION_DENSIDAD = 0.30f;  // ESTIMADO

// Una muda tarda SEMANAS. Tiene que ir muy por detras de la temperatura:
// si el pelaje siguiera al clima diario se veria como un parpadeo.  ESTIMADO
constexpr float MUDA_CONSTANTE_TIEMPO_DIAS = 21.0f;

// --- EL LODO DEL REVOLCADERO ---
//
// El revolcadero esta MEDIDO como conducta de termorregulacion. Lo que NO
// esta medido es cuanto calor disipa: eso es AUSENTE en la literatura.
//
// Se modela porque cierra un lazo que si no queda abierto: sin barro visible,
// el animal se revuelca y sale identico, y no hay forma de VER que la
// necesidad se satisfizo. El estado interno tiene que asomar a la superficie.
constexpr float LODO_OSCURECIMIENTO_MAX = 0.35f;   // ESTIMADO
constexpr float LODO_TIEMPO_SECADO_S    = 900.0f;  // ESTIMADO -- 15 min

// OJO CON NO CONFUNDIR DOS COSAS:
//   - El lodo SOBRE EL ANIMAL se seca en minutos (esto).
//   - El REVOLCADERO como estructura del mundo persiste >= 6 anos (MEDIDO) y
//     atrae a otras especies. Eso es un bloque del mundo, no estado del
//     animal, y no vive en este archivo.

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

// --- COHERENCIA DEL PELAJE ---

// El collar cruza el HOMBRO: va por delante del centro del tronco. Si se
// desliza al centro del lomo deja de ser el collar de la especie.
static_assert(Externo::COLLAR_Z > 0.0f,
              "El collar se ha ido al lomo o a las ancas: cruza el HOMBRO");
static_assert(Externo::COLLAR_Z < Externo::TRONCO.maxZ(),
              "El collar se sale del tronco por delante");

// El pelaje de verano ACLARA. La direccion la fija Zervanos y Hadley 1973;
// invertirla seria contradecir el dato medido.
static_assert(Externo::MUDA_ACLARADO_VERANO > 0.0f,
              "La muda de verano oscurece: MEDIDO que el pelaje SE ACLARA");

// Y el barro OSCURECE. Si no, revolcarse no se notaria.
static_assert(Externo::LODO_OSCURECIMIENTO_MAX > 0.0f,
              "El lodo no oscurece: revolcarse seria invisible");

// El lodo no puede tapar al animal del todo: sigue siendo un pecari
// embarrado, no una silueta negra.
static_assert(Externo::LODO_OSCURECIMIENTO_MAX < 1.0f,
              "El lodo deja al animal completamente negro");

// La muda va MUY por detras del clima. Si bajara de una semana, el pelaje
// parpadearia con cada racha de calor en vez de mudar.
static_assert(Externo::MUDA_CONSTANTE_TIEMPO_DIAS > 7.0f,
              "La muda es demasiado rapida: el pelaje parpadearia con el clima");

// El barro se seca en minutos; el revolcadero del mundo dura anos. Son cosas
// distintas y no deben confundirse.
static_assert(Externo::LODO_TIEMPO_SECADO_S < 86400.0f,
              "El lodo tarda mas de un dia en secarse: se confundio el barro del "
              "animal con el revolcadero del mundo");

// La cresta baja EXPONENCIALMENTE con tau = tiempo_subida / factor_bajada.
// Esa aproximacion solo vale si tau es mucho mayor que un frame. Con el factor
// por debajo de 1 y un tiempo de subida de 0.25 s, tau ronda los 0.7 s frente
// a los 0.017 s de un frame a 60 fps: sobra margen. El assert vigila que nadie
// suba el factor por encima de 1 y invierta la asimetria.
static_assert(Externo::CRESTA_FACTOR_BAJADA > 0.0f &&
              Externo::CRESTA_FACTOR_BAJADA < 1.0f,
              "El factor de bajada de la cresta debe estar entre 0 y 1: por encima "
              "de 1 la cresta bajaria mas rapido de lo que sube, al reves de lo que "
              "hace un animal de presa");

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

// ----------------------------------------------------------------------------
// EL PELAJE, EN CUATRO BYTES
// ----------------------------------------------------------------------------
// Todo el aspecto de la superficie de un pecari cabe aqui. Cada campo es un
// uint8_t que representa un 0..1 en 256 pasos.
//
// POR QUE BYTES Y NO FLOATS: la diferencia es 4 bytes frente a 16. Con 1000
// animales son 4 KB contra 16 KB, y mas importante, cuatro veces mas
// individuos por linea de cache al recorrer la piara para dibujarla. La
// precision de un float aqui no compra nada: nadie distingue el tono 0.501
// del 0.502 en un animal de 95 cm visto a diez metros.
//
// PARA COMPARAR: un solo vertice con posicion, normal y UV ocupa 32 bytes.
// El pelaje entero de un pecari cuesta menos que un octavo de vertice.
//
// COSTE POR FRAME: ninguno. El tono no cambia nunca, la muda se actualiza una
// vez por dia simulado y la suciedad va en el tick de necesidades. Solo el
// erizado se anima, y ya se animaba antes.
struct PelajeEmpaquetado {
    // Tono base del individuo. 0 = grisaceo, 255 = castano.
    // La VARIACION esta MEDIDA (la especie presenta esos tonos); su
    // distribucion es ESTIMADO.
    //
    // Sin esto una piara entera se ve como copias del mismo objeto -- y esta
    // especie forma manadas cohesivas MEDIDAS, asi que el jugador siempre los
    // vera juntos. Es justo el caso donde la uniformidad canta.
    uint8_t tonoBase = 128;

    // Cuanto ha mudado hacia el pelaje de verano. 0 = invierno, 255 = verano.
    // Aclara el tono y baja la densidad aparente de cerdas.  MEDIDO (la
    // direccion), ESTIMADO (la magnitud).
    uint8_t muda = 0;

    // Lodo encima, del revolcadero. 0 = seco, 255 = recien salido.
    uint8_t suciedad = 0;

    // Cresta: 0 = pegada al lomo, 255 = completamente erizada.
    // Continuo porque se anima; no es un si/no.
    uint8_t erizado = 0;

    // --- Lectura como 0..1, para el sombreado ---
    float tonoBase01() const { return tonoBase * (1.0f / 255.0f); }
    float muda01()     const { return muda     * (1.0f / 255.0f); }
    float suciedad01() const { return suciedad * (1.0f / 255.0f); }
    float erizado01()  const { return erizado  * (1.0f / 255.0f); }

    // El color final es una sola cuenta: se parte del tono del individuo, la
    // muda lo ACLARA y el lodo lo OSCURECE. Devuelve un multiplicador de
    // luminancia para el sombreador; no hay textura por individuo.
    float factorLuminancia() const {
        const float base   = 0.72f + 0.28f * tonoBase01();
        const float verano = 1.0f + Externo::MUDA_ACLARADO_VERANO * muda01();
        const float barro  = 1.0f - Externo::LODO_OSCURECIMIENTO_MAX * suciedad01();
        return base * verano * barro;
    }

    // En verano hay menos cerdas. Sirve para atenuar el relieve del pelaje
    // en el sombreado, no para quitar geometria: no la hay.
    float densidadCerdas() const {
        return 1.0f - Externo::MUDA_REDUCCION_DENSIDAD * muda01();
    }

    // Altura actual de la cresta, en metros.
    float alturaCresta() const {
        return Externo::CRESTA_ALTURA_REPOSO_M +
               erizado01() * (Externo::CRESTA_ALTURA_ERIZADA_M -
                              Externo::CRESTA_ALTURA_REPOSO_M);
    }
};

// Cuatro bytes. Si alguien mete un float aqui, el build para: es exactamente
// el descuido que este diseno existe para evitar.
static_assert(sizeof(PelajeEmpaquetado) == 4,
              "El pelaje se fue de 4 bytes: alguien metio un float donde iba un byte");

// ----------------------------------------------------------------------------
// DE QUE COLOR SALE UN PECARI
// ----------------------------------------------------------------------------
// El tono NO se guarda en una tabla ni se sortea al azar puro: se DERIVA del
// id del individuo y del de su matrilinea. Dos consecuencias practicas:
//
//   - No cuesta memoria. No hay lista de colores que mantener.
//   - Es reproducible. El mismo animal sale del mismo color siempre, aunque
//     el chunk se descargue y se vuelva a cargar.
//
// Y los parientes se parecen. Eso no es un capricho: la filopatria femenina
// esta MEDIDA en esta especie, las manadas se construyen sobre linajes
// maternos (el campo linaje_materno ya existe en 11_PECARI_IA.json), asi que
// una piara ES una familia. Que se parezcan entre si es lo coherente.
//
// La heredabilidad del tono NO esta medida: la CORRELACION es ESTIMADO. Lo
// que respeta es una estructura social que si esta medida.

// Cuanto puede alejarse un hermano del tono de su linaje, en pasos de 0..255.
// +-38 sobre 255 es una diferencia visible entre hermanos sin que dejen de
// parecer de la misma familia.  ESTIMADO
constexpr int TONO_DISPERSION_FAMILIAR = 38;

// El linaje se centra en un rango util: ni blanco ni negro puros, que no
// existen en la especie. 40..215 sobre 255.  ESTIMADO
constexpr int TONO_LINAJE_MIN   = 40;
constexpr int TONO_LINAJE_RANGO = 175;

// Mezclador entero. No usa float ni rand(): mismo id, mismo color, siempre.
constexpr uint32_t mezclar(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU;
    x ^= x >> 15; x *= 0x846ca68bU;
    x ^= x >> 16;
    return x;
}

// El tono de un individuo: el CENTRO de su linaje, mas una desviacion propia.
//
// OJO CON LA TRAMPA: la version obvia --promediar el hash del linaje con el
// del individuo-- NO funciona. Promediar dos valores centrados en 128 colapsa
// la varianza hacia el centro, y la piara entera vuelve a verse uniforme, que
// es justo el problema que esto venia a resolver.
//
// Por eso el linaje fija el CENTRO y el individuo aporta una desviacion CON
// SIGNO alrededor de el. Asi cada familia ocupa su propia franja del rango en
// vez de amontonarse todas en el medio.
constexpr uint8_t tonoDe(uint32_t idIndividuo, uint32_t idLinaje) {
    // Centro de la familia, repartido por el rango util.
    const int centro = TONO_LINAJE_MIN +
                       static_cast<int>((mezclar(idLinaje) & 0xFFu) * TONO_LINAJE_RANGO / 255);

    // Desviacion propia del individuo: -DISPERSION .. +DISPERSION
    //
    // Se mezcla con el LINAJE, no solo con el id del individuo. Si dependiera
    // solo del id, el hermano numero 2 seria el mas claro en TODAS las piaras
    // del mundo: el mismo patron repetido familia tras familia, que se nota
    // enseguida cuando el jugador ve varias manadas.
    const int bruto = static_cast<int>(mezclar(idIndividuo * 2654435761u ^ (idLinaje * 40503u)) & 0xFFu);
    const int desvio = (bruto * (2 * TONO_DISPERSION_FAMILIAR) / 255) - TONO_DISPERSION_FAMILIAR;

    const int t = centro + desvio;
    return static_cast<uint8_t>(t < 0 ? 0 : (t > 255 ? 255 : t));
}

// Un pecari recien creado, listo para dibujar.
constexpr PelajeEmpaquetado nacer(uint32_t idIndividuo, uint32_t idLinaje) {
    return PelajeEmpaquetado{ tonoDe(idIndividuo, idLinaje), 0, 0, 0 };
}

constexpr int distTono(uint8_t a, uint8_t b) { return a > b ? a - b : b - a; }

// 1. DOS HERMANOS SE PARECEN. Nunca pueden separarse mas que la dispersion
//    familiar completa, porque ambos cuelgan del mismo centro.
static_assert(distTono(tonoDe(1u, 7u), tonoDe(2u, 7u)) <= 2 * TONO_DISPERSION_FAMILIAR,
              "Dos hermanos salen de tonos demasiado distintos: la correlacion de linaje no funciona");
static_assert(distTono(tonoDe(3u, 7u), tonoDe(9u, 7u)) <= 2 * TONO_DISPERSION_FAMILIAR,
              "Dos hermanos salen de tonos demasiado distintos: la correlacion de linaje no funciona");

// 2. PERO NO SON CLONES. Si dos hermanos salieran identicos, la piara
//    volveria a verse como copias del mismo objeto.
static_assert(tonoDe(1u, 7u) != tonoDe(2u, 7u),
              "Dos hermanos salen identicos: no hay variacion individual");

// 3. Y DOS FAMILIAS SE DISTINGUEN. Este es el que caza el error de colapso de
//    varianza: si el generador amontona todos los tonos en el centro, dos
//    linajes lejanos acaban indistinguibles y esto salta.
static_assert(distTono(tonoDe(1u, 7u), tonoDe(1u, 42u)) > 0,
              "El linaje no influye en el tono: todas las piaras se veran iguales");

// 4. NI BLANCOS NI NEGROS PUROS: la especie no los tiene.
static_assert(tonoDe(1u, 7u) > 0 && tonoDe(1u, 7u) < 255,
              "Un tono se fue al extremo puro del rango");

struct Estado {
    Actividad actividad = Actividad::PASTANDO;

    // Todo el aspecto de la superficie, en 4 bytes.
    PelajeEmpaquetado pelaje;

    // Fase del paso, para animar las patas. Radianes.
    float fasePaso = 0.0f;

    // Cuanto olor de su piara lleva encima. Baja con el tiempo y sube al
    // frotarse con otro miembro. Si llega a cero, los demas lo tratan como
    // extrano -- que es como funciona de verdad.
    float marcaAlmizcle = 1.0f;

    float alturaCresta() const { return pelaje.alturaCresta(); }

    // Se eriza deprisa y baja despacio: alarmarse es inmediato, calmarse
    // cuesta. Asi se comporta un animal de presa.
    //
    // OJO CON EL REDONDEO, que aqui tiene trampa doble:
    //
    //   1. Truncar a byte en cada paso rompe la asimetria. Con dt pequeno el
    //      decremento vale una fraccion de unidad, el redondeo se la come, y
    //      la cresta se queda clavada o cae a saltos. El error EMPEORA cuantos
    //      mas fps tenga el juego, que es justo al reves de lo deseable.
    //
    //   2. Una bajada LINEAL, ademas, no es lo que hace un animal. Una
    //      respuesta de sobresalto no se apaga a ritmo constante: decae, muy
    //      rapido al principio y cada vez mas despacio. Es un decaimiento
    //      exponencial.
    //
    // Asi que la bajada es exponencial y se calcula desde el valor actual, no
    // por acumulacion de pasos. Eso la hace INDEPENDIENTE del framerate: el
    // mismo tiempo real da el mismo resultado a 30 que a 240 fps.
    void actualizarCresta(float dt, bool alarmado) {
        float e = pelaje.erizado * (1.0f / 255.0f);
        if (alarmado) {
            // Sube lineal: alarmarse es inmediato y a ritmo pleno.
            e += dt / Externo::CRESTA_TIEMPO_ERIZADO_S;
            if (e > 1.0f) e = 1.0f;
        } else {
            // Baja exponencial. La constante de tiempo sale del tiempo de
            // subida dividido por el factor de bajada: calmarse cuesta
            // 1/0.35 = ~2.9 veces lo que cuesta alarmarse.
            const float tau = Externo::CRESTA_TIEMPO_ERIZADO_S /
                              Externo::CRESTA_FACTOR_BAJADA;
            // Aproximacion de exp(-dt/tau) valida para dt << tau, que siempre
            // se cumple: tau ronda los 0.7 s y dt es un frame.
            float k = 1.0f - dt / tau;
            if (k < 0.0f) k = 0.0f;
            const float nuevo = e * k;

            // UNA EXPONENCIAL NUNCA LLEGA A CERO, y en bytes eso se convierte
            // en un fallo visible: la cresta se queda CLAVADA en un valor bajo
            // para siempre y el animal pasa el resto de su vida medio erizado.
            //
            // Y no basta con poner un umbral: cerca de cero el decremento vale
            // menos que medio paso de byte, el redondeo lo devuelve al valor
            // de partida, y el umbral no se alcanza NUNCA. Es un punto fijo.
            //
            // Se rompe garantizando que la bajada mueva al menos un paso
            // entero de byte cuando el decaimiento por si solo no lo lograria.
            const float pasoByte = 1.0f / 255.0f;
            e = (e - nuevo < pasoByte) ? (e - pasoByte) : nuevo;
            if (e < pasoByte) e = 0.0f;
        }
        pelaje.erizado = static_cast<uint8_t>(e * 255.0f + 0.5f);
    }

    // El lodo se seca solo. Va en el tick de necesidades, NO por frame.
    // Mismo cuidado con el redondeo que en la cresta.
    void actualizarSuciedad(float dt) {
        float s = pelaje.suciedad * (1.0f / 255.0f) - dt / Externo::LODO_TIEMPO_SECADO_S;
        // Igual que en la cresta: por debajo de un paso de byte se corta, o el
        // animal se queda con una mota de barro encima indefinidamente.
        if (s < (1.0f / 255.0f)) s = 0.0f;
        pelaje.suciedad = static_cast<uint8_t>(s * 255.0f + 0.5f);
    }

    // Sale del revolcadero cubierto de barro.
    void revolcarse() { pelaje.suciedad = 255; }

    // La muda persigue al verano con MUCHO retardo. Se llama una vez por dia
    // simulado, no por frame: es un proceso de semanas.
    //
    // 'verano01' es 0 en pleno invierno y 1 en pleno verano.
    void actualizarMuda(float diasTranscurridos, float verano01) {
        const float objetivo = verano01 * 255.0f;
        const float k = diasTranscurridos / Externo::MUDA_CONSTANTE_TIEMPO_DIAS;
        const float alfa = k > 1.0f ? 1.0f : k;
        const float m = pelaje.muda + (objetivo - pelaje.muda) * alfa;
        pelaje.muda = static_cast<uint8_t>(m < 0.0f ? 0.0f : (m > 255.0f ? 255.0f : m));
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
