#ifndef PECARI_ANATOMIA_H
#define PECARI_ANATOMIA_H

#include "AnimalMalla.h"
#include <cstdint>
#include <cmath>

// ============================================================================
// ANATOMIA PARAMETRICA DEL PECARI Y CONSTRUCCION DE SU MALLA
// ============================================================================
// Convierte un juego de PARAMETROS anatomicos en una malla organica.
//
// La separacion importa: los parametros son datos (se pueden variar por
// especie, por edad, por individuo) y el constructor es codigo compartido.
// Cambiar de pecari a venado sera cambiar los numeros, no reescribir esto.
//
// ----------------------------------------------------------------------------
// NO ES UN CERDO. LO QUE HAY QUE ACERTAR
// ----------------------------------------------------------------------------
// Tayassuidae y Suidae se separaron hace ~37 millones de anios. El parecido es
// convergencia. Estos son los rasgos que, si se fallan, producen un cerdo:
//
//   1. CUERPO EN BARRIL, corto y alto. El cerdo es largo y bajo; el pecari es
//      compacto y de perfil casi cuadrado. Su tronco es proporcionalmente mas
//      CORTO y mucho mas ALTO.
//
//   2. PATAS LARGAS Y FINAS. Es la firma visual: un cuerpo macizo sobre patas
//      de ciervo. El cerdo las tiene cortas y gruesas.
//
//   3. CABEZA GRANDE EN CUNA, con la linea frontal RECTA. El cerdo tiene el
//      perfil concavo; el pecari, recto.
//
//   4. TRES DEDOS TRASEROS, no cuatro. Es EL criterio de campo que separa las
//      dos familias.
//
//   5. COLA INEXISTENTE (1.2 cm). El error mas visible seria darle cola.
//
//   6. COLMILLOS RECTOS Y VERTICALES, no curvados hacia arriba como el jabali.
//
//   7. COLLAR CLARO cruzando hombro y cuello. Da nombre a la especie.
//
//   8. CRIN DORSAL de la coronilla a la grupa, con glandula sobre las ancas.
//
//   9. LINEA DORSAL ALTA Y RECTA, con la grupa ligeramente caida.
//
// ----------------------------------------------------------------------------
// TRAZABILIDAD
// ----------------------------------------------------------------------------
// Misma escala del AI simulator: MEDIDO / DERIVADO / INFERIDO / ESTIMADO.
// Datos de AI simulator/Mamiferos/, con las fuentes citadas en cada bloque.
// ============================================================================

namespace Fauna {

// ============================================================================
// PARAMETROS ANATOMICOS
// ============================================================================
// El punto 5 pide exactamente esto: un juego de numeros que permita generar
// variaciones naturales sin duplicar el modelo.
//
// TODO en METROS, que es la unidad de la anatomia publicada. La conversion a
// bloques del motor (0.60 m) ocurre en un solo sitio, al dibujar.
struct ParametrosPecari {

    // --- CUERPO ---
    // MEDIDO: longitud cabeza-cuerpo 84-106 cm; se usa el centro del rango.
    float largoCuerpo   = 0.95f;

    // MEDIDO: altura a la cruz 30-50 cm. Se toma un valor alto-medio por
    // coherencia con 95 cm de longitud.
    float alturaCruz    = 0.44f;

    // ESTIMADO: el tronco "en forma de barril" es mas ancho por el centro.
    // La razon ancho/alto ~0.86 es lo que lo hace compacto y no aplanado.
    float anchoTorso    = 0.27f;
    float altoTorso     = 0.31f;

    // Fraccion del largo total que ocupa el TRONCO (sin cabeza ni cuello).
    // DERIVADO: cabeza (0.25 m) + cuello (0.12 m) sobre 0.95 m deja 0.58 para
    // el tronco, o sea el 61%.
    float fraccionTronco = 0.61f;

    // --- CUELLO ---
    // Corto y macizo: sostiene una cabeza proporcionalmente grande.
    float largoCuello   = 0.12f;   // ESTIMADO
    float grosorCuello  = 0.22f;   // ESTIMADO

    // --- CABEZA ---
    // "Proporcionalmente grande" es descripcion cualitativa constante en la
    // literatura. En cuna: ancha en el craneo, afilada hacia el hocico.
    float largoCabeza   = 0.25f;   // ESTIMADO
    float anchoCabeza   = 0.19f;   // ESTIMADO
    float altoCabeza    = 0.21f;   // ESTIMADO

    // --- HOCICO ---
    // El organo de trabajo: escarba y huele. Detecta raices a 8 cm bajo tierra.
    float largoHocico   = 0.10f;   // ESTIMADO
    float anchoHocico   = 0.105f;  // DERIVADO: afilado 0.55 respecto al craneo

    // El DISCO RINARIAL: cartilago apoyado en el hueso PRENASAL, cuya funcion
    // es resistir la abrasion del suelo al escarbar.  MEDIDO (que existe)
    float discoRinarial = 0.062f;  // ESTIMADO (diametro)

    // --- PATAS ---
    // "Largas y delgadas en comparacion con el cuerpo robusto". El contraste
    // es una firma visual de la especie.
    float grosorPata    = 0.048f;  // ESTIMADO
    float separacionPataX = 0.082f;// ESTIMADO
    float patasDelanteZ = 0.165f;  // ESTIMADO
    float patasTraseraZ = -0.175f; // ESTIMADO

    // --- PEZUNAS ---
    // "Angostas y funcionales": nada que ver con el casco ancho de un ungulado
    // de pradera. MEDIDO: 4 dedos delante, 3 detras, solo 2 apoyan.
    float anchoPezuna   = 0.020f;  // ESTIMADO
    float altoPezuna    = 0.028f;  // ESTIMADO
    int   dedosDelante  = 4;       // MEDIDO
    int   dedosDetras   = 3;       // MEDIDO  <- LA diferencia con el cerdo

    // --- OREJAS ---
    // MEDIDO: 3-10 cm; se usa el centro. Pequenas y erguidas. El oido es su
    // segundo sentido, muy por delante de la vista.
    float largoOreja    = 0.065f;
    float anchoOreja    = 0.045f;  // ESTIMADO
    float anguloOreja   = 0.34f;   // ESTIMADO: rad hacia fuera

    // --- OJOS ---
    // Laterales: es una presa. Vista pesima (no distingue a mas de 1 m),
    // SIN tapetum lucidum, dominancia de conos, vision dicromatica.
    float diametroOjo   = 0.019f;  // ESTIMADO
    float separacionOjo = 0.058f;  // ESTIMADO

    // --- COLA ---
    // MEDIDO: 1.2 cm. Practicamente inexistente. NO darle cola de cerdo.
    float largoCola     = 0.012f;

    // --- CRIN DORSAL ---
    // De la CORONILLA a la GRUPA. Se eriza al alarmarse y el animal parece
    // mucho mayor.  MEDIDO (cualitativo)
    float altoCrinReposo  = 0.020f;  // ESTIMADO
    float altoCrinErizada = 0.070f;  // ESTIMADO (~3.5x)

    // --- COLLAR ---
    // La banda clara que cruza hombro y cuello. Diagnostica de la especie.
    float anchoCollar   = 0.042f;   // ESTIMADO
    float posCollarZ    = 0.135f;   // ESTIMADO

    // --- GLANDULA DORSAL ---
    // LA caracteristica unica de la familia. Va SOBRE LAS ANCAS, no en mitad
    // del lomo: ponerla en el centro seria un error del modelo.
    float posGlandulaZ  = -0.145f;  // MEDIDO (cualitativo: dorsal, ancas)

    // --- COLOR ---
    // MEDIDO (cualitativo): "negro y gris jaspeado, mas claro en los hombros,
    // con una raya dorsal oscura". Pelo AGUTI: bandas alternas en cada pelo,
    // por eso se ve jaspeado y no liso.
    // â­ PELO NEGRO EN TODO EL CUERPO.
    //
    // Se pidio pelo negro cubriendo el animal entero, y NO contradice el dato:
    // 13_PECARI_ANATOMIA describe las cerdas como "de tonos grisaceos, NEGROS o
    // castanos" (MEDIDO cualitativo), y la variacion individual de tono la
    // recoge explicitamente como normal dentro de la especie. El negro es uno
    // de los tres tonos publicados, no una invencion.
    //
    // Lo que NO se toca, porque si es diagnostico:
    //   - el COLLAR claro sigue igual: da nombre a la especie
    //   - el LOMO sigue siendo mas oscuro que el flanco (raya dorsal MEDIDA)
    //   - el VIENTRE sigue siendo mas claro que el dorso
    //
    // O sea: baja el tono general, se conserva la ESTRUCTURA de claros y
    // oscuros. Un animal de un solo negro plano se leeria como una silueta
    // recortada, no como un cuerpo con volumen.
    float colorBase[3]    = { 0.088f, 0.082f, 0.078f };
    float colorLomo[3]    = { 0.048f, 0.045f, 0.044f };   // raya dorsal oscura
    float colorVientre[3] = { 0.135f, 0.126f, 0.118f };   // mas claro

    // --- LA CRESTA DORSAL: BLANCO GRISACEO ---
    //
    // Va en su propia zona (ZonaCuerpo::CRIN) para poder ser clara SIN
    // blanquear la raya dorsal oscura sobre la que se levanta.
    //
    // Y tiene sentido funcional: la crin es SENALIZACION, no abrigo -- se eriza
    // al alarmarse para que el animal parezca mayor. Una cresta clara sobre un
    // cuerpo negro es la senal mas legible posible, y a contraluz recorta la
    // silueta. Con la crin del mismo color que el lomo, el gesto se perdia.
    //
    // confianza: ESTIMADO (el tono exacto). La existencia de la cresta erectil
    // y su funcion de senalizacion son MEDIDO.
    float colorCrin[3]    = { 0.775f, 0.768f, 0.742f };
    float colorCollar[3]  = { 0.730f, 0.690f, 0.590f };
    float colorPata[3]    = { 0.058f, 0.054f, 0.051f };
    float colorHocico[3]  = { 0.072f, 0.064f, 0.062f };   // piel, no pelo
    float colorPezuna[3]  = { 0.042f, 0.040f, 0.038f };   // queratina

    // El ojo: casi negro, y MAS OSCURO que cualquier otra parte. Es lo que le
    // da mirada al animal -- sin contraste contra la cara, no se ve.
    //
    // No es negro puro: un ojo real refleja algo de cielo. Y NO brilla de
    // noche: esta especie NO tiene tapetum lucidum (MEDIDO), a diferencia de
    // los felinos con los que comparte habitat. Darle brillo seria el error
    // tipico al representar fauna.
    // AJUSTADO AL OSCURECER EL CUERPO: el comentario de arriba exige que el ojo
    // sea MAS OSCURO QUE CUALQUIER OTRA PARTE, y con el pelaje ya en negro el
    // valor anterior (0.035) habia dejado de cumplirlo frente a la pezuna. Un
    // ojo que no contrasta con la cara desaparece y el animal pierde la mirada.
    // Hay un test que lo comprueba.
    // EL IRIS. Se aclara respecto al valor anterior (0.016) porque ahora hay
    // una PUPILA de verdad contra la que contrastar: un ojo entero del mismo
    // negro es una canica, y lo que produce una mirada es la diferencia entre
    // iris y pupila. Sigue siendo oscuro -- el iris de esta especie es pardo
    // muy oscuro -- pero ya no es el punto mas negro de la cara.
    //
    // NO brilla de noche: esta especie NO tiene tapetum lucidum (MEDIDO), a
    // diferencia de los felinos con los que comparte habitat.
    //
    // MEDIDO al ajustarlo: la cara negra queda en 0.091 de luminancia, asi que
    // un iris de 0.087 era INDISTINGUIBLE de ella -- exactamente el bug que
    // este color existe para evitar. Se sube a ~0.21, que contrasta de verdad
    // sin dejar de ser un ojo pardo oscuro de herbivoro.
    float colorOjo[3]     = { 0.255f, 0.195f, 0.130f };   // pardo ambar oscuro

    // LA PUPILA: lo mas oscuro del animal, y a proposito. Es el agujero por el
    // que entra la luz; no refleja nada.
    // Negro casi puro: es un agujero, no una superficie. Tiene que quedar por
    // debajo del vertice mas oscuro del pelaje (medido: 0.0047 tras el
    // jaspeado), o deja de ser el punto mas negro del animal.
    float colorPupila[3]  = { 0.003f, 0.003f, 0.004f };

    // --- PELAJE ---
    // Cerdas largas, gruesas y rigidas: casi puas flexibles.
    float largoCerda      = 0.050f;   // ESTIMADO
    float densidadPelo    = 1.0f;     // multiplicador global

    // Sube de 0.16 a 0.21: sobre un pelaje negro hace falta mas recorrido de
    // jaspeado para que el aguti se lea. Con el sesgo hacia la luz de colorear()
    // esto se traduce en punteado de puntas claras, que es lo que da la textura
    // de pelo sin anadir un solo triangulo.
    float variacionColor  = 0.21f;    // cuanto jaspea el aguti

    // Cuanto se deforma la superficie para que no parezca un globo liso.
    // Desplaza los VERTICES, no solo las normales, asi que tambien cambia la
    // SILUETA -- que es donde mas se nota la piel estirada.
    // confianza: ESTIMADO (ajuste visual)
    float relieveSuperficie = 1.0f;

    // ------------------------------------------------------------------------
    // â­ CUANTO SE "CUADRA" EL ANIMAL: DE ORGANICO A VOXEL
    // ------------------------------------------------------------------------
    // Se pidio que el modelo fuera CUBICO -- que encaje en un mundo de
    // voxeles -- MANTENIENDO su tamano.
    //
    // Las dos cosas son compatibles porque la cuadratura no escala nada: lleva
    // cada punto del anillo desde la elipse hasta el RECTANGULO que ya la
    // circunscribia. El punto mas lejano (la esquina) ya estaba a esa
    // distancia, asi que la envolvente del animal no crece ni un milimetro y
    // las medidas anatomicas MEDIDAS siguen siendo las mismas. Hay tests que
    // lo comprueban.
    //
    // 0.78 y no 1.0: a tope del todo el animal se lee como una caja con patas
    // y pierde que es un ser vivo. Con 0.78 los lados son claramente planos y
    // las aristas se ven, pero queda el redondeo justo para que el lomo siga
    // siendo un lomo. Es la misma decision que toma cualquier modelo voxel de
    // criatura: caja de base, esquinas apenas matadas.
    float cuadratura = 0.78f;

    // --- VARIACION INDIVIDUAL ---
    uint32_t semilla = 1u;
};

// ----------------------------------------------------------------------------
// VARIACION POR ESPECIE
// ----------------------------------------------------------------------------
// El punto 2 pide poder representar otras poblaciones sin duplicar el modelo.
// Se resuelve con funciones que DEVUELVEN parametros, no con clases nuevas.
namespace Especies {

    // Dicotyles tajacu â€” pecari de collar. El de este proyecto.
    inline ParametrosPecari pecariDeCollar() {
        return ParametrosPecari{};   // los valores por defecto
    }

    // Tayassu pecari â€” pecari labiado.
    //
    // ADVERTENCIA: es OTRA ESPECIE, no una variante. Manadas de 25-100+,
    // NOCTURNO en vez de diurno, y comportamiento muy distinto. Se incluye
    // solo para demostrar que la arquitectura admite variacion; su IA NO debe
    // reutilizar la del pecari de collar.
    inline ParametrosPecari pecariLabiado() {
        ParametrosPecari p;
        p.largoCuerpo = 1.05f;      // algo mayor
        p.alturaCruz  = 0.50f;
        p.anchoTorso  = 0.30f;
        p.altoTorso   = 0.34f;
        // Sin collar: tiene una mancha blanca en el LABIO, de ahi el nombre.
        p.colorCollar[0] = 0.82f; p.colorCollar[1] = 0.80f; p.colorCollar[2] = 0.74f;
        p.anchoCollar = 0.010f;     // casi inexistente en el hombro
        // Se reajusta al oscurecer el pelaje del de collar: este valor era
        // 0.205, MAS CLARO que el nuevo colorBase del de collar (0.088). Como
        // existe precisamente para que las dos especies no se confundan, tiene
        // que seguir siendo relativamente mas claro, no absolutamente.
        p.colorBase[0] = 0.128f; p.colorBase[1] = 0.119f; p.colorBase[2] = 0.112f;
        // Tampoco tiene la crin clara del de collar: su rasgo es el labio.
        p.colorCrin[0] = 0.180f; p.colorCrin[1] = 0.172f; p.colorCrin[2] = 0.166f;
        return p;
    }

    // Catagonus wagneri â€” pecari del Chaco.
    // NO vive en Mexico. Se declara porque cambia UN numero (dos dedos
    // traseros en vez de tres) y demuestra que el modelo lo admite.
    inline ParametrosPecari pecariDelChaco() {
        ParametrosPecari p;
        p.largoCuerpo = 1.10f;
        p.alturaCruz  = 0.55f;
        p.dedosDetras = 2;          // MEDIDO: la tercera especie reduce a dos
        p.largoOreja  = 0.090f;     // orejas notablemente mayores
        return p;
    }

    // --- VARIACION POR EDAD ---
    // Una cria NO es un adulto encogido: el craneo crece antes que el cuerpo,
    // asi que tiene la cabeza proporcionalmente enorme y las patas
    // larguiruchas.
    //
    // DERIVADO: masa al nacer 0.5 kg / adulto 18.7 kg. La masa va con el cubo
    // de la longitud, luego el neonato mide (0.5/18.7)^(1/3) = 0.30 del adulto.
    inline ParametrosPecari aplicarEdad(ParametrosPecari p, int etapa) {
        float esc = 1.0f, fCabeza = 1.0f, fPatas = 1.0f, fHocico = 1.0f;

        switch (etapa) {
            case 0:  esc = 0.30f; fCabeza = 1.42f; fPatas = 1.18f; fHocico = 0.72f; break;  // NEONATO
            case 1:  esc = 0.52f; fCabeza = 1.28f; fPatas = 1.12f; fHocico = 0.80f; break;  // JUVENIL
            case 2:  esc = 0.78f; fCabeza = 1.12f; fPatas = 1.05f; fHocico = 0.91f; break;  // SUBADULTO
            case 3:  esc = 1.00f; break;                                                     // ADULTO
            case 4:  esc = 0.97f; fCabeza = 1.02f; fPatas = 0.98f; fHocico = 1.02f; break;  // SENESCENTE
        }

        p.largoCuerpo *= esc;  p.alturaCruz *= esc;
        p.anchoTorso  *= esc;  p.altoTorso  *= esc;
        p.largoCuello *= esc;  p.grosorCuello *= esc;

        // La cabeza escala MENOS que el cuerpo en las crias: por eso se
        // multiplica ademas por fCabeza > 1.
        p.largoCabeza *= esc * fCabeza;
        p.anchoCabeza *= esc * fCabeza;
        p.altoCabeza  *= esc * fCabeza;

        p.largoHocico *= esc * fHocico;
        p.anchoHocico *= esc * fHocico;
        p.discoRinarial *= esc * fHocico;

        p.grosorPata *= esc * (2.0f - fPatas);   // mas finas en las crias
        p.separacionPataX *= esc;
        p.patasDelanteZ *= esc;  p.patasTraseraZ *= esc;
        p.anchoPezuna *= esc;    p.altoPezuna *= esc;

        p.largoOreja *= esc * fCabeza;
        p.anchoOreja *= esc * fCabeza;
        p.diametroOjo *= esc * fCabeza;
        p.separacionOjo *= esc * fCabeza;

        p.largoCola *= esc;
        p.altoCrinReposo *= esc;  p.altoCrinErizada *= esc;
        p.anchoCollar *= esc;     p.posCollarZ *= esc;
        p.posGlandulaZ *= esc;
        p.largoCerda *= esc;

        return p;
    }
}

// ============================================================================
// NIVEL DE DETALLE
// ============================================================================
// El punto 17 pide cuatro niveles. Aqui se definen los parametros de cada uno;
// la seleccion por distancia va en el gestor.
struct ConfigLOD {
    int   ladosCuerpo;      // vertices por anillo del tronco
    int   seccionesCuerpo;  // anillos a lo largo del tronco
    int   ladosPata;
    int   seccionesPata;
    bool  conOrejas;
    bool  conOjos;
    bool  conColmillos;
    bool  conPezunasDetalle; // dedos individuales vs bloque unico
    bool  conCrin;
    bool  perturbarNormales; // el "normal map" procedural
    float intensidadPelaje;
};

inline ConfigLOD configDeLOD(int lod) {
    switch (lod) {
        case 0:  // CERCA: anatomia completa
            // DENSIDAD SUBIDA DE 14x11 A 20x16, Y LA PATA DE 8x5 A 10x7.
            //
            // La piel se veia "lisa y estirada" y buena parte era resolucion:
            // con 11 anillos a lo largo de todo el cuerpo, entre costilla y
            // costilla no hay vertices donde meter relieve. El sombreado es
            // Gouraud (por vertice), asi que TODO el detalle de superficie
            // vive en la densidad de la malla -- sin vertices no hay donde
            // dibujar un musculo.
            //
            // Coste medido: LOD 0 pasa de 841 a ~1900 vertices. Sigue muy por
            // debajo del presupuesto de 64 KB por malla, y la malla se COMPARTE
            // entre todos los individuos de la especie: 100 pecaries siguen
            // usando una sola. Solo LOD 0 la paga (animales a menos de 7 m).
            return { 20, 16, 10, 7, true,  true,  true,  true,  true,  true,  0.34f };
        case 1:  // MEDIA
            // LA CRIN PASA A ESTAR ACTIVA AQUI (antes false).
            //
            // Con la cresta en blanco grisaceo sobre un cuerpo negro deja de
            // ser un detalle fino y pasa a ser SILUETA: es lo primero que se
            // distingue del animal a media distancia. Apagarla en LOD 1 la
            // hacia aparecer de golpe al acercarse -- popping en el rasgo mas
            // visible del modelo.
            //
            // Cuesta 4 lados x seccionesCuerpo vertices, que es barato: la
            // crin es un tubo de 4 caras, no una malla densa.
            return { 14, 11, 7, 5, true,  true,  false, true,  true,  true,  0.24f };
        case 2:  // LEJOS: silueta simplificada
            return {  7,  6, 5, 3, true,  false, false, false, false, false, 0.0f  };
        default: // LOD3, MUY LEJOS: minimo viable
            return {  5,  4, 4, 2, false, false, false, false, false, false, 0.0f  };
    }
}

// ============================================================================
// CONSTRUCTOR DE LA MALLA
// ============================================================================
class ConstructorPecari {
public:

    // ------------------------------------------------------------------------
    // GENERAR EL ANIMAL COMPLETO
    // ------------------------------------------------------------------------
    // Una sola malla, un solo objeto de render. El punto 4 lo exige: los
    // componentes son conceptuales, no entidades separadas.
    static void generar(MallaAnimal& malla,
                        const ParametrosPecari& p,
                        int lod) {
        malla.limpiar();
        const ConfigLOD cfg = configDeLOD(lod);

        construirTronco(malla, p, cfg);
        construirCuelloYCabeza(malla, p, cfg);
        construirPatas(malla, p, cfg);
        construirCola(malla, p, cfg);
        if (cfg.conOrejas)  construirOrejas(malla, p, cfg);
        if (cfg.conOjos)    construirOjos(malla, p, cfg);
        if (cfg.conCrin)    construirCrin(malla, p, cfg);
        // Las fosas nasales van con el mismo presupuesto que los ojos: son
        // detalle de cara, y se ven o no se ven juntos.
        if (cfg.conOjos)    construirNariz(malla, p, cfg);

        // Normales suaves: es lo que convierte los triangulos en una
        // superficie continua bajo el sombreado Gouraud del pipeline fijo.
        GeneradorMalla::calcularNormales(malla);

        // --- RELIEVE DE SUPERFICIE ---
        //
        // EL ORDEN DE ESTAS TRES LLAMADAS NO ES NEGOCIABLE:
        //
        //   1. calcularNormales   -> hace falta una normal para saber HACIA
        //                            DONDE desplazar cada vertice.
        //   2. relieveDeSuperficie-> mueve los vertices y deja las normales
        //                            obsoletas (apuntan a la forma anterior).
        //   3. calcularNormales   -> se rehacen sobre la geometria YA
        //                            deformada. Sin este segundo pase el
        //                            relieve no se veria: la silueta cambiaria
        //                            pero la luz seguiria calculandose como si
        //                            la superficie fuera lisa.
        //
        // Solo donde hay presupuesto de detalle (LOD 0-1). De lejos la silueta
        // no da para distinguir una costilla.
        if (cfg.perturbarNormales) {
            GeneradorMalla::relieveDeSuperficie(malla, 0, p.semilla,
                                                p.relieveSuperficie);
            GeneradorMalla::calcularNormales(malla);
        }

        // Color por zona + jaspeado aguti.
        colorear(malla, p);

        // El "normal map" procedural: perturba las normales donde hay pelo.
        if (cfg.perturbarNormales) {
            GeneradorMalla::perturbarPorPelaje(malla, p.semilla,
                                               cfg.intensidadPelaje);
        }

        malla.actualizarCaja();
    }

private:
    // ------------------------------------------------------------------------
    // EL TRONCO: el barril
    // ------------------------------------------------------------------------
    // La pieza que define al animal. No es un cilindro: el radio varia a lo
    // largo del eje formando un barril, y la linea dorsal se arquea.
    static void construirTronco(MallaAnimal& malla,
                                const ParametrosPecari& p,
                                const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float z0 = -largoTronco * 0.52f;   // grupa
        const float z1 =  largoTronco * 0.48f;   // pecho

        // Altura del eje del tronco sobre el suelo.
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;

        std::vector<SeccionCuerpo> secs;
        const int N = cfg.seccionesCuerpo;

        for (int i = 0; i < N; ++i) {
            const float t = (float)i / (float)(N - 1);   // 0 grupa .. 1 pecho
            SeccionCuerpo s;
            s.z = z0 + (z1 - z0) * t;

            // --- PERFIL DEL BARRIL ---
            // Maximo en el centro-tercio delantero (donde va la caja toracica
            // y el proestomago voluminoso que fermenta celulosa), estrechando
            // hacia los dos extremos.
            //
            // El pico esta en t=0.58, no en 0.5: el pecari tiene el volumen
            // desplazado hacia el pecho, no centrado.
            const float d = (t - 0.58f) / 0.58f;
            const float perfil = 1.0f - 0.22f * d * d;

            s.radioX = p.anchoTorso * 0.5f * perfil;
            s.radioY = p.altoTorso  * 0.5f * perfil;

            // --- LINEA DORSAL ---
            // Alta y bastante recta, con la GRUPA LIGERAMENTE CAIDA. Es un
            // rasgo de perfil de la especie: no es una curva simetrica.
            const float caidaGrupa = (1.0f - t) * (1.0f - t) * 0.035f;
            s.centroY = ejeY - caidaGrupa * p.alturaCruz;

            // El vientre se achata hacia el centro del cuerpo (donde el animal
            // apoya la panza) y menos en los extremos.
            s.achatadoVientre = 0.35f + 0.45f * (1.0f - std::fabs(t - 0.5f) * 2.0f);

            // El lomo se alza mas hacia la cruz: es donde nace la crin.
            s.alzadoLomo = 0.12f + 0.22f * t;

            s.zonaLomo    = (t < 0.30f) ? ZonaCuerpo::GRUPA : ZonaCuerpo::LOMO;
            s.zonaFlanco  = ZonaCuerpo::TORSO;
            s.zonaVientre = ZonaCuerpo::VIENTRE;

            secs.push_back(s);
        }

        // Cuadrar la seccion: el animal es de un mundo de voxeles.
        GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
        // ⭐⭐ EL PECHO SE TAPA. "SE VE POR DENTRO DESDE EL CUELLO".
        //
        // Estaba en `false`, con la idea de que el cuello lo taparia al
        // enchufarse. Pero el cuello es OTRO TUBO, mas estrecho que el pecho y
        // que ademas empieza tambien abierto por detras: entre el borde del
        // torso y el del cuello queda un anillo de hueco.
        //
        // Con GL_CULL_FACE activo ese hueco no se ve como un agujero negro --
        // se ve el INTERIOR del animal, porque las caras de dentro del torso
        // quedan de espaldas y OpenGL las descarta, dejando ver hasta la pared
        // opuesta. De ahi el efecto de "mirar dentro de la cabeza" al
        // acercarse al cuello.
        //
        // No se arregla ensanchando el cuello: el cuello es MAS estrecho que
        // el pecho por anatomia, asi que siempre habria reborde. Se arregla
        // cerrando cada tubo por su cuenta, que ademas es lo correcto -- son
        // dos volumenes solidos, no un tubo continuo.
        //
        // Coste: un abanico de triangulos por tapa. Nada.
        GeneradorMalla::coserTubo(malla, secs, cfg.ladosCuerpo, p.semilla,
                                  true, true);
    }

    // ------------------------------------------------------------------------
    // CUELLO Y CABEZA
    // ------------------------------------------------------------------------
    // Van en el MISMO tubo para que la union sea continua. Coserlos por
    // separado dejaria una arista visible justo donde mas se mira.
    static void construirCuelloYCabeza(MallaAnimal& malla,
                                       const ParametrosPecari& p,
                                       const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float zPecho = largoTronco * 0.48f;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;

        std::vector<SeccionCuerpo> secs;

        // --- CUELLO: corto y macizo ---
        {
            SeccionCuerpo s;
            s.z = zPecho;
            s.radioX = p.grosorCuello * 0.5f;
            s.radioY = p.grosorCuello * 0.52f;
            s.centroY = ejeY + p.altoTorso * 0.06f;
            s.achatadoVientre = 0.20f;
            s.alzadoLomo = 0.30f;
            s.zonaLomo = ZonaCuerpo::CUELLO;
            s.zonaFlanco = ZonaCuerpo::CUELLO;
            s.zonaVientre = ZonaCuerpo::CUELLO;
            secs.push_back(s);

            // LA GARGANTA BAJA. El primer anillo del cuello estaba a 0.6 de su
            // largo, asi que entre el pecho y el cuello quedaba un salto de
            // 7,1 cm -- el mayor de toda la cadena una vez cerrados los de la
            // cabeza. Es la union tronco-cuello, o sea la otra mitad de "se ve
            // separado": el cuello parecia enchufado al pecho.
            SeccionCuerpo s1c = s;
            s1c.z = zPecho + p.largoCuello * 0.30f;
            s1c.radioX = p.grosorCuello * 0.485f;
            s1c.radioY = p.grosorCuello * 0.505f;
            s1c.centroY = ejeY + p.altoTorso * 0.08f;
            secs.push_back(s1c);

            SeccionCuerpo s2 = s;
            s2.z = zPecho + p.largoCuello * 0.6f;
            s2.radioX = p.grosorCuello * 0.46f;
            s2.radioY = p.grosorCuello * 0.48f;
            s2.centroY = ejeY + p.altoTorso * 0.10f;
            secs.push_back(s2);

            // â­ LA GARGANTA: el tramo que faltaba.
            //
            // El cuello acababa en 0.6 y la cabeza empezaba en 1.0, asi que el
            // 40% final no tenia NINGUNA seccion. coserTubo cose anillos
            // consecutivos, de modo que ese hueco se salvaba con un unico
            // salto largo: una arista dura justo donde la cabeza se une al
            // cuerpo, y el animal se veia hecho de piezas por ahi.
            //
            // Medido: 4.8 cm entre el ultimo anillo del cuello y el primero de
            // la cabeza, con el resto de uniones por debajo de 2 cm.
            //
            // Esta seccion intermedia lo cierra, y ademas hace de transicion de
            // grosor: el cuello se ensancha al llegar al craneo en vez de
            // cambiar de golpe.
            SeccionCuerpo s3 = s;
            s3.z = zPecho + p.largoCuello * 0.85f;
            s3.radioX = p.grosorCuello * 0.48f;
            s3.radioY = p.grosorCuello * 0.50f;
            s3.centroY = ejeY + p.altoTorso * 0.11f;
            secs.push_back(s3);

            // â­â­ LA NUCA: "EL CUELLO SE VE SEPARADO DE LA CABEZA".
            //
            // Y no lo estaba: cuello y cabeza van en el MISMO tubo, asi que no
            // hay dos piezas que puedan despegarse. Lo que se veia era un
            // SALTO -- medido, 8,8 cm entre el ultimo anillo del cuello
            // (z=0.85 del cuello) y el primero del craneo, en un cuello que
            // mide 12 cm en total.
            //
            // coserTubo une anillos CONSECUTIVOS con quads. Un hueco de 8,8 cm
            // se salvaba con un unico quad larguisimo: una banda lisa, sin
            // curvatura intermedia, que capta la luz de forma uniforme y se lee
            // exactamente como una junta entre dos piezas.
            //
            // Era ademas el peor sitio posible para tener el unico tramo largo
            // del modelo, porque es donde el skinning MAS deforma: el peso pasa
            // de 0 (tronco) a 255 (cabeza) justo ahi, asi que al girar la
            // cabeza ese quad se estiraba todavia mas y la "junta" se abria.
            //
            // Con este anillo el mayor salto de la cadena baja a ~4,4 cm, en
            // linea con el resto de uniones. Y ademas ensancha ligeramente
            // hacia el craneo: la nuca de un suido es maciza, no un cuello
            // fino enchufado a una cabeza ancha.
            SeccionCuerpo s4 = s;
            s4.z = zPecho + p.largoCuello * 0.95f;
            s4.radioX = p.grosorCuello * 0.52f;
            s4.radioY = p.grosorCuello * 0.54f;
            s4.centroY = ejeY + p.altoTorso * 0.115f;
            secs.push_back(s4);
        }

        // --- CABEZA EN CUNA ---
        // El craneo es ancho por detras y se afila hacia el hocico. La linea
        // frontal es RECTA (el cerdo la tiene concava): es un rasgo de perfil
        // que distingue a las dos familias.
        const float zCraneo = zPecho + p.largoCuello;
        {
            SeccionCuerpo s;
            s.z = zCraneo;
            s.radioX = p.anchoCabeza * 0.5f;
            s.radioY = p.altoCabeza * 0.5f;
            s.centroY = ejeY + p.altoTorso * 0.12f;
            s.achatadoVientre = 0.30f;
            s.alzadoLomo = 0.10f;
            s.zonaLomo = ZonaCuerpo::CABEZA;
            s.zonaFlanco = ZonaCuerpo::CABEZA;
            s.zonaVientre = ZonaCuerpo::CABEZA;
            secs.push_back(s);

            // LA SIEN. Mismo problema que el carrillo, un tramo antes: entre
            // la nuca (zCraneo) y la mejilla (0.42) quedaban 7,2 cm sin
            // anillos. Es la parte mas ancha de la cabeza y se estaba
            // resolviendo con una sola faceta plana.
            SeccionCuerpo s1b = s;
            s1b.z = zCraneo + p.largoCabeza * 0.21f;
            s1b.radioX = p.anchoCabeza * 0.5f * 1.02f;   // el craneo abulta algo
            s1b.radioY = p.altoCabeza * 0.5f * 0.99f;
            s1b.centroY = s.centroY - p.altoCabeza * 0.020f;
            secs.push_back(s1b);

            // Mejilla: aun ancha.
            SeccionCuerpo s2 = s;
            s2.z = zCraneo + p.largoCabeza * 0.42f;
            s2.radioX = p.anchoCabeza * 0.46f;
            s2.radioY = p.altoCabeza * 0.44f;
            s2.centroY = s.centroY - p.altoCabeza * 0.045f;
            secs.push_back(s2);

            // â­ EL CARRILLO: EL TRAMO QUE FALTABA DE VERDAD.
            //
            // MEDIDO buscando el mayor salto de la cadena: estaba AQUI, no en
            // el cuello. Entre la mejilla (0.42 del largo de la cabeza) y la
            // base del hocico (0.78) habia un hueco del 36% de la cabeza --
            // 8,7 cm-- sin un solo anillo.
            //
            // Y es justo el tramo donde la cabeza hace su cambio de forma mas
            // fuerte: pasa de craneo ancho a hocico estrecho (el afilado
            // MEDIDO de ~0.55). Salvar esa transicion con un unico quad
            // producia una faceta plana y continua a cada lado de la cara, que
            // es lo que se lee como "la cabeza esta pegada al cuello": no una
            // separacion, sino una junta lisa donde deberia haber curva.
            //
            // Con este anillo el afilado se reparte en dos tramos y la mejilla
            // recupera su volumen.
            SeccionCuerpo sc = s;
            sc.z = zCraneo + p.largoCabeza * 0.60f;
            sc.radioX = p.anchoCabeza * 0.5f * 0.78f;
            sc.radioY = p.altoCabeza * 0.5f * 0.74f;
            sc.centroY = s.centroY - p.altoCabeza * 0.085f;
            secs.push_back(sc);

            // Base del hocico: aqui empieza el afilado.
            SeccionCuerpo s3 = s;
            s3.z = zCraneo + p.largoCabeza * 0.78f;
            s3.radioX = p.anchoHocico * 0.62f;
            s3.radioY = p.altoCabeza * 0.30f;
            s3.centroY = s.centroY - p.altoCabeza * 0.13f;
            s3.zonaLomo = ZonaCuerpo::CABEZA;
            s3.zonaFlanco = ZonaCuerpo::HOCICO;
            s3.zonaVientre = ZonaCuerpo::HOCICO;
            secs.push_back(s3);
        }

        // --- HOCICO ---
        // Cilindro corto y estrecho. Es el organo de trabajo.
        const float zHocico = zCraneo + p.largoCabeza;
        {
            SeccionCuerpo s;
            s.z = zHocico;
            s.radioX = p.anchoHocico * 0.5f;
            s.radioY = p.anchoHocico * 0.48f;
            s.centroY = ejeY + p.altoTorso * 0.12f - p.altoCabeza * 0.17f;
            s.achatadoVientre = 0.10f;
            s.alzadoLomo = 0.0f;
            s.zonaLomo = ZonaCuerpo::HOCICO;
            s.zonaFlanco = ZonaCuerpo::HOCICO;
            s.zonaVientre = ZonaCuerpo::HOCICO;
            secs.push_back(s);

            // --- EL DISCO RINARIAL ---
            // La punta no es un cono: es un DISCO plano de cartilago apoyado
            // en el hueso prenasal, disenado para escarbar sin deformarse.
            // Por eso el ultimo anillo se ENSANCHA en vez de cerrarse.
            SeccionCuerpo s2 = s;
            s2.z = zHocico + p.largoHocico * 0.82f;
            s2.radioX = p.discoRinarial * 0.5f;
            s2.radioY = p.discoRinarial * 0.44f;
            secs.push_back(s2);

            SeccionCuerpo s3 = s2;
            s3.z = zHocico + p.largoHocico;
            s3.radioX = p.discoRinarial * 0.5f;
            s3.radioY = p.discoRinarial * 0.44f;
            secs.push_back(s3);
        }

        // Cuadrar la seccion: el animal es de un mundo de voxeles.
        GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
        // ⭐⭐ Y LA GARGANTA TAMBIEN SE TAPA, por lo mismo.
        //
        // Este tubo empezaba abierto por detras esperando que el torso lo
        // tapara. Los dos se esperaban el uno al otro y ninguno cerraba: ese
        // era el hueco por el que se veia el interior de la cabeza.
        //
        // La tapa queda METIDA dentro del pecho --el cuello nace por dentro
        // del torso, no pegado a su cara-- asi que no se ve desde fuera. Solo
        // cierra el volumen.
        const size_t base = GeneradorMalla::coserTubo(
            malla, secs, cfg.ladosCuerpo, p.semilla, true, true);
        (void)base;
    }

    // ------------------------------------------------------------------------
    // LAS CUATRO PATAS
    // ------------------------------------------------------------------------
    // Largas y finas sobre un cuerpo macizo: el contraste es la firma visual.
    // Se afinan hacia abajo, como una pata de ungulado corredor.
    static void construirPatas(MallaAnimal& malla,
                               const ParametrosPecari& p,
                               const ConfigLOD& cfg) {
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;
        const float yHombro = ejeY - p.altoTorso * 0.28f;
        const float largoPata = yHombro - p.altoPezuna;

        const struct { float sx, sz; bool delantera; } defs[4] = {
            { -1.0f, p.patasDelanteZ, true  },
            { +1.0f, p.patasDelanteZ, true  },
            { -1.0f, p.patasTraseraZ, false },
            { +1.0f, p.patasTraseraZ, false }
        };

        for (const auto& d : defs) {
            std::vector<SeccionCuerpo> secs;

            // ================================================================
            // â­ LA PATA TIENE TRES SEGMENTOS Y DOS ARTICULACIONES
            // ================================================================
            // Antes era un CONO RECTO: `centroY = yHombro - largoPata * t` es
            // una linea, y `z = d.sz` constante. Un palo liso que se estrecha.
            //
            // Y era una contradiccion con el resto del sistema, porque el
            // esqueleto SI tiene las articulaciones -- PecariEsqueleto.h
            // declara CODO/RODILLA/CORVEJON con sus amplitudes (AMP_CODO 0.30,
            // AMP_RODILLA 0.42...) y el skinning las dobla. Pero doblar un palo
            // liso por un punto solo produce un palo con un pliegue: no hay
            // codo que ver porque la geometria no lo tiene.
            //
            // Aqui la pata pasa a tener la forma real:
            //
            //        DELANTERA              TRASERA
            //        hombro |               cadera |
            //               |                      |\
            //         codo  /                rodilla \      <- angulos
            //              |                          |        OPUESTOS
            //         cana |                corvejon  /
            //              |                         |
            //           pezuna                    pezuna
            //
            // LOS ANGULOS VAN AL REVES entre delantera y trasera, y eso NO es
            // un detalle: es lo que hace que un cuadrupedo se lea como tal. El
            // codo apunta hacia ATRAS y la rodilla hacia ADELANTE. Es el mismo
            // SENTIDO_DELANTERA/SENTIDO_TRASERA (-1/+1) que ya usa el
            // esqueleto, asi que la malla y los huesos coinciden.
            //
            // COSTE: dos secciones mas por pata. Ocho anillos en todo el
            // animal, y solo en LOD 0-1 (de cerca). En LOD 2-3 el reparto se
            // mantiene y no se paga nada.

            // Fracciones de los tres segmentos. Son las MISMAS que
            // PecariEsqueleto.h usa para colocar los pivotes (FRAC_SUPERIOR
            // 0.38, FRAC_MEDIA 0.34), asi que el codo de la malla cae
            // exactamente donde esta el hueso que la dobla.
            constexpr float FRAC_SUP   = 0.38f;   // humero / femur
            constexpr float FRAC_MED   = 0.34f;   // radioulna / tibia
            // El resto (0.28) es la cana: larga, porque el pecari es
            // digitigrado -- camina de puntillas. Es lo que lo hace corredor.

            // Cuanto se adelanta o atrasa cada articulacion, en fraccion del
            // largo de la pata.
            //
            // â­ BAJADO DE 0.085 A 0.022: LAS PATAS ESTABAN DOBLADAS EN REPOSO.
            //
            // El valor anterior metia un zigzag de 8,5% del largo de la pata
            // EN LA PROPIA GEOMETRIA, o sea en la malla cacheada, que es la
            // postura de reposo. El animal quieto ya aparecia agachado, con el
            // codo y la rodilla marcados como si estuviera a media zancada.
            //
            // Y se sumaba al doblado REAL: PoseDeMarcha dobla los mismos huesos
            // encima de una malla que ya venia curvada, asi que en movimiento
            // la flexion se exageraba el doble.
            //
            // El zigzag de reposo de un ungulado es MUY leve -- la pata esta
            // casi recta en apoyo; es lo que le permite aguantar el peso con el
            // minimo esfuerzo muscular (reposo osteo-ligamentoso). Un zigzag
            // marcado es la postura de un felino agazapado, no la de un suido
            // de pie.
            //
            // Se deja un resto (0.022) porque CERO daria un palo perfectamente
            // recto, y entonces la articulacion no se leeria como tal: hace
            // falta un minimo de quiebre para que el codo tenga direccion y la
            // flexion de la marcha salga hacia el lado correcto.
            // MEDIDO al ajustarlo: con 0.022 el rango en Z de la pata bajaba a
            // 0.037 contra un grosor de 0.048 -- o sea MENOS que su propio
            // diametro, que es la firma de un cono recto. Me habia pasado de
            // frenada: quitar la postura agachada no debe llegar a borrar la
            // articulacion.
            //
            // 0.042 deja la pata visiblemente recta de pie y conserva el codo.
            const float sentido = d.delantera ? -1.0f : +1.0f;
            constexpr float QUIEBRE = 0.042f;

            // Los cuatro puntos de control de la pata, de arriba abajo. La
            // seccion intermedia de cada tramo se interpola entre ellos, asi
            // que la pata queda continua y no en tramos rectos pegados.
            struct Nodo { float t, dz, grosor; };
            const Nodo nodos[4] = {
                // t (0 arriba .. 1 abajo)  |  desplazamiento Z  |  grosor rel.
                { 0.0f,                      0.0f,                 1.35f },
                { FRAC_SUP,                  sentido * QUIEBRE,    1.02f },
                { FRAC_SUP + FRAC_MED,      -sentido * QUIEBRE * 0.55f, 0.74f },
                { 1.0f,                      0.0f,                 0.62f }
            };

            // Al menos un anillo por articulacion, mas los que pida el LOD.
            // Con seccionesPata=5 (LOD 0) salen 7; con 2 (LOD 3) salen 4, que
            // sigue bastando para una silueta a 60 metros.
            const int N = cfg.seccionesPata + 2;

            for (int i = 0; i < N; ++i) {
                const float t = (float)i / (float)(N - 1);   // 0 arriba, 1 abajo

                // Localizar t entre los nodos e interpolar. Suave (smoothstep)
                // para que el codo sea un codo y no un pico anguloso.
                int k = 0;
                while (k < 2 && t > nodos[k + 1].t) ++k;
                const float span = nodos[k + 1].t - nodos[k].t;
                float u = (span > 1e-6f) ? (t - nodos[k].t) / span : 0.0f;
                if (u < 0.0f) u = 0.0f;
                if (u > 1.0f) u = 1.0f;
                const float su = u * u * (3.0f - 2.0f * u);   // smoothstep

                const float dz     = nodos[k].dz     + (nodos[k + 1].dz     - nodos[k].dz)     * su;
                const float gFactor= nodos[k].grosor + (nodos[k + 1].grosor - nodos[k].grosor) * su;

                SeccionCuerpo s;
                s.z = d.sz + dz;
                s.centroY = yHombro - largoPata * t;

                // --- AFINADO ---
                // De 1.35x en el hombro (donde hay musculo) a 0.62x en la
                // cana. La pata de un pecari es notablemente mas fina abajo.
                // Ahora el perfil viene de los nodos, asi que el grosor cambia
                // por TRAMO: el muslo es macizo, la cana es un palillo.
                const float grosor = p.grosorPata * gFactor;
                s.radioX = grosor * 0.5f;
                s.radioY = grosor * 0.5f;

                // â­ LA ARTICULACION ES MAS ANCHA QUE EL HUESO.
                //
                // Un codo o una rodilla abultan: hay epifisis, ligamento y
                // tendon. Sin esto la pata se ve como una manguera doblada.
                // El bulto es sutil (12%) y solo en la vecindad del nodo.
                const float dCodo = std::fabs(t - FRAC_SUP);
                const float dRod  = std::fabs(t - (FRAC_SUP + FRAC_MED));
                const float cerca = (dCodo < dRod) ? dCodo : dRod;
                if (cerca < 0.10f) {
                    const float bulto = 1.0f + 0.12f * (1.0f - cerca / 0.10f);
                    s.radioX *= bulto;
                    s.radioY *= bulto;
                }

                // La articulacion es mas ANCHA que PROFUNDA: se dobla en un
                // solo plano, como una bisagra, y eso se nota en la silueta.
                if (cerca < 0.10f) s.radioX *= 1.06f;

                // Las patas traseras son algo mas gruesas arriba: llevan la
                // masa muscular del cuarto trasero.
                if (!d.delantera && t < 0.4f) {
                    s.radioX *= 1.18f;
                    s.radioY *= 1.18f;
                }

                s.achatadoVientre = 0.0f;
                s.alzadoLomo = 0.0f;
                s.zonaLomo = s.zonaFlanco = s.zonaVientre = ZonaCuerpo::PATA;
                secs.push_back(s);
            }

            const size_t base = malla.vertices.size();
            // â­ LAS PATAS VAN TAPADAS POR ARRIBA.
            //
            // Estaban con las dos tapas a false, o sea tubos ABIERTOS por los
            // dos extremos. Con GL_CULL_FACE activo eso deja ver el interior
            // del cilindro por el agujero: la pata se veia hueca, y en el
            // borde las normales promediadas apuntaban a cualquier sitio.
            //
            // Se midio con el volumen encerrado (teorema de la divergencia):
            // la pata daba exactamente 0, la firma de una superficie que no
            // cierra. El tronco daba +0.012.
            //
            // Arriba SI se tapa, porque ahi el tubo se mete en el cuerpo y el
            // agujero queda a la vista desde abajo. Abajo NO hace falta: lo
            // cubre la pezuna.
            // Cuadrar la seccion: el animal es de un mundo de voxeles.
            GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
            GeneradorMalla::coserTubo(malla, secs, cfg.ladosPata, p.semilla,
                                      true, false);
            GeneradorMalla::transformar(malla, base,
                                        V3(d.sx * p.separacionPataX, 0.0f, 0.0f));
            GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::PATA, 0.85f);

            construirPezunas(malla, p, cfg, d.sx, d.sz,
                             yHombro - largoPata, d.delantera);
        }
    }

    // ------------------------------------------------------------------------
    // PEZUNAS: 4 dedos delante, 3 detras
    // ------------------------------------------------------------------------
    // MEDIDO, y es EL criterio de campo que separa Tayassuidae de Suidae: el
    // cerdo verdadero tiene CUATRO dedos traseros, el pecari TRES.
    //
    // Solo DOS apoyan (los centrales); los laterales cuelgan sin cargar peso.
    static void construirPezunas(MallaAnimal& malla,
                                 const ParametrosPecari& p,
                                 const ConfigLOD& cfg,
                                 float ladoX, float posZ,
                                 float yPie, bool delantera) {
        const int nDedos = delantera ? p.dedosDelante : p.dedosDetras;

        // A LOD bajo, las pezunas son un solo bloque: a esa distancia no se
        // distinguen los dedos y no vale la pena el coste.
        const int nGenerar = cfg.conPezunasDetalle ? nDedos : 2;

        for (int dedo = 0; dedo < nGenerar; ++dedo) {
            const bool apoya = (dedo < 2);   // MEDIDO: solo 2 tocan el suelo

            std::vector<SeccionCuerpo> secs;
            SeccionCuerpo a, b;

            a.z = posZ + (apoya ? 0.006f : -0.014f);
            a.centroY = apoya ? yPie : (yPie + p.altoPezuna * 0.55f);
            a.radioX = p.anchoPezuna * (apoya ? 0.5f : 0.30f);
            a.radioY = p.anchoPezuna * (apoya ? 0.42f : 0.26f);
            a.zonaLomo = a.zonaFlanco = a.zonaVientre = ZonaCuerpo::PEZUNA;

            b = a;
            b.centroY = a.centroY - p.altoPezuna * (apoya ? 1.0f : 0.55f);
            b.radioX *= 0.82f;
            b.radioY *= 0.82f;

            secs.push_back(a);
            secs.push_back(b);

            const size_t base = malla.vertices.size();
            // Cuadrar la seccion: el animal es de un mundo de voxeles.
            GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
            GeneradorMalla::coserTubo(malla, secs, 4, p.semilla, true, true);

            // Reparto lateral de los dedos.
            float offX = 0.0f;
            if (apoya) {
                offX = (dedo == 0 ? -1.0f : 1.0f) * p.anchoPezuna * 0.58f;
            } else {
                const int k = dedo - 2;
                offX = (nDedos - 2 == 1) ? 0.0f
                     : ((k == 0 ? -1.0f : 1.0f) * p.anchoPezuna * 0.70f);
            }

            GeneradorMalla::transformar(malla, base,
                V3(ladoX * p.separacionPataX + offX, 0.0f, 0.0f));
            // Queratina: sin pelo.
            GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::PEZUNA, 0.0f);
        }
    }

    // ------------------------------------------------------------------------
    // LA COLA
    // ------------------------------------------------------------------------
    // MEDIDO: 1.2 cm. Deliberadamente minuscula. Darle cola de cerdo seria el
    // error mas visible del modelo.
    static void construirCola(MallaAnimal& malla,
                              const ParametrosPecari& p,
                              const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;

        std::vector<SeccionCuerpo> secs;
        SeccionCuerpo a, b;

        a.z = -largoTronco * 0.52f;
        a.centroY = ejeY + p.altoTorso * 0.32f;
        a.radioX = p.largoCola * 0.55f;
        a.radioY = p.largoCola * 0.55f;
        a.zonaLomo = a.zonaFlanco = a.zonaVientre = ZonaCuerpo::COLA;

        b = a;
        b.z = a.z - p.largoCola;
        b.radioX *= 0.7f;
        b.radioY *= 0.7f;

        secs.push_back(a);
        secs.push_back(b);

        const size_t base = malla.vertices.size();
        // Cuadrar la seccion: el animal es de un mundo de voxeles.
        GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
        GeneradorMalla::coserTubo(malla, secs, 4, p.semilla, false, true);
        GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::COLA, 1.0f);
        (void)cfg;
    }

    // ------------------------------------------------------------------------
    // OREJAS
    // ------------------------------------------------------------------------
    // MEDIDO: 6.5 cm. Pequenas y erguidas. El oido es su segundo sentido tras
    // el olfato, muy por delante de la vista.
    static void construirOrejas(MallaAnimal& malla,
                                const ParametrosPecari& p,
                                const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;
        const float zCraneo = largoTronco * 0.48f + p.largoCuello;
        const float yTop = ejeY + p.altoTorso * 0.12f + p.altoCabeza * 0.38f;

        for (int lado = 0; lado < 2; ++lado) {
            const float sx = (lado == 0) ? -1.0f : 1.0f;

            std::vector<SeccionCuerpo> secs;
            // SUBE DE 3 A 6 ANILLOS. Con tres, la oreja era un cono recto de
            // tres tramos: no habia donde meter la curva del pabellon ni el
            // estrechamiento de la base. Seis dan una oreja con forma.
            const int N = 6;
            for (int i = 0; i < N; ++i) {
                const float t = (float)i / (float)(N - 1);
                SeccionCuerpo s;
                s.z = zCraneo + p.largoCabeza * 0.10f;
                // â­ LA OREJA ARRANCA DENTRO DEL CRANEO, no posada encima.
                //
                // Antes empezaba justo en yTop, que es el borde de la cabeza:
                // nacia exactamente en la superficie, sin morder carne. Medido,
                // quedaba a 7.2 cm de cualquier vertice del craneo -- flotando.
                //
                // Ahora el primer anillo va HUNDIDO un 30% del largo de la
                // oreja, asi que la base queda enterrada en la cabeza y no hay
                // costura que ver. Es como se implanta una oreja de verdad:
                // el pabellon sale de dentro, no se apoya fuera.
                const float hundido = p.largoOreja * 0.30f;
                s.centroY = yTop - hundido + (p.largoOreja + hundido) * t;

                // --- EL PERFIL DEL PABELLON ---
                //
                // Antes era un afilado LINEAL: un cono, o sea un pico de
                // papel. Una oreja real no es eso -- se ESTRECHA en la base
                // (donde se implanta en el craneo), se ENSANCHA en el tercio
                // inferior formando la concha que recoge el sonido, y solo
                // entonces se afila hacia la punta.
                //
                // Ese ensanchamiento es funcional: el oido es el segundo
                // sentido de esta especie, muy por delante de la vista
                // (MEDIDO: no distingue objetos a mas de un metro). Una oreja
                // que solo se afila no recogeria nada.
                //
                // La curva: sin(pi*t) da el vientre de la concha; el termino
                // lineal la cierra hacia la punta.
                const float concha = std::sin(3.14159265f * t);
                const float perfil = (1.0f - 0.62f * t) * (0.72f + 0.42f * concha);

                s.radioX = p.anchoOreja * 0.5f * perfil;
                // La oreja es una LAMINA: mucho mas ancha que profunda. El
                // grosor tambien crece algo en la concha.
                s.radioY = p.anchoOreja * 0.16f * (0.80f + 0.30f * concha)
                                        * (1.0f - 0.30f * t);

                // â­ LA OREJA SE INCLINA HACIA ATRAS SEGUN SUBE.
                //
                // Estaba en z CONSTANTE, o sea perfectamente vertical y
                // plana contra el craneo. Las orejas de un suido salen
                // inclinadas hacia atras; verticales dan aspecto de juguete.
                s.z -= p.largoOreja * 0.34f * t * t;

                s.zonaLomo = s.zonaFlanco = s.zonaVientre = ZonaCuerpo::OREJA;
                secs.push_back(s);
            }

            const size_t base = malla.vertices.size();
            // Cuadrar la seccion: el animal es de un mundo de voxeles.
            GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
            GeneradorMalla::coserTubo(malla, secs, cfg.ladosPata, p.semilla,
                                      true, true);
            GeneradorMalla::transformar(malla, base,
                V3(sx * p.anchoCabeza * 0.34f, 0.0f, 0.0f));
            // Abrirlas hacia fuera.
            GeneradorMalla::rotarY(malla, base, sx * p.anguloOreja,
                V3(sx * p.anchoCabeza * 0.34f, yTop, zCraneo));
            GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::OREJA, 0.55f);
        }
    }

    // ------------------------------------------------------------------------
    // LA NARIZ: FOSAS NASALES EN EL DISCO RINARIAL
    // ------------------------------------------------------------------------
    // El disco de la punta del hocico estaba LISO: una losa de cartilago sin
    // un solo rasgo. Y resulta que es la herramienta mas importante del animal.
    //
    // POR QUE MERECE GEOMETRIA PROPIA:
    //   - Este animal tiene vista pesima (MEDIDO: no distingue objetos a mas
    //     de un metro) y lo resuelve TODO con el olfato.
    //   - Detecta raices y tuberculos a 8 cm BAJO TIERRA.
    //   - Sus bulbos olfatorios son desproporcionadamente grandes (MEDIDO).
    //
    // O sea: las fosas nasales son, funcionalmente, los "ojos" del pecari. Que
    // fueran invisibles mientras los ojos --que apenas usa-- si se modelaban
    // era justo al reves de lo que pide la biologia de la especie.
    //
    // Se hacen como dos hendiduras HUNDIDAS en el disco: un tubo corto metido
    // hacia dentro y mas oscuro, que es como se lee un orificio sin agujerear
    // la malla de verdad.
    static void construirNariz(MallaAnimal& malla,
                               const ParametrosPecari& p,
                               const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;
        const float zCraneo = largoTronco * 0.48f + p.largoCuello;
        const float zHocico = zCraneo + p.largoCabeza;

        // La cara del disco, con las mismas formulas de construirCuelloYCabeza.
        const float zDisco  = zHocico + p.largoHocico;
        const float yDisco  = ejeY + p.altoTorso * 0.12f - p.altoCabeza * 0.17f;
        const float rDiscoX = p.discoRinarial * 0.5f;

        for (int lado = 0; lado < 2; ++lado) {
            const float sx = (lado == 0) ? -1.0f : 1.0f;

            std::vector<SeccionCuerpo> secs;

            // Boca del orificio, a ras del disco.
            SeccionCuerpo a;
            a.z = zDisco - p.discoRinarial * 0.04f;
            a.radioX = p.discoRinarial * 0.155f;
            a.radioY = p.discoRinarial * 0.115f;
            a.centroY = yDisco;
            a.zonaLomo = a.zonaFlanco = a.zonaVientre = ZonaCuerpo::HOCICO;
            secs.push_back(a);

            // Y el fondo, METIDO hacia dentro. Es la profundidad lo que hace
            // que se lea como agujero y no como una mancha pintada.
            SeccionCuerpo b = a;
            b.z = zDisco - p.discoRinarial * 0.30f;
            b.radioX *= 0.45f;
            b.radioY *= 0.45f;
            secs.push_back(b);

            const size_t base = malla.vertices.size();
            // Cuadrar la seccion: el animal es de un mundo de voxeles.
            GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
            GeneradorMalla::coserTubo(malla, secs, cfg.ladosPata, p.semilla,
                                      false, true);

            // Las dos fosas, separadas a los lados del eje del hocico y algo
            // inclinadas: no son dos circulos paralelos.
            GeneradorMalla::transformar(malla, base,
                V3(sx * rDiscoX * 0.46f, 0.0f, 0.0f));

            // Piel desnuda y humeda: sin pelo, y por tanto sin jaspeado.
            //
            // El OSCURECIDO de la fosa no se hace aqui: colorear() corre
            // DESPUES de construir y sobrescribe el color de todos los
            // vertices, asi que pintarlas ahora no serviria de nada. Se
            // resuelve alli, por posicion (ver "LAS FOSAS NASALES").
            GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::HOCICO, 0.0f);
        }
    }

    // ------------------------------------------------------------------------
    // OJOS
    // ------------------------------------------------------------------------
    // LATERALES, no frontales: es una presa. Vision casi panoramica a costa de
    // poca binocular.
    //
    // SIN TAPETUM LUCIDUM (MEDIDO): sus ojos NO brillan de noche. Es el error
    // tipico al representar fauna y aqui seria biologicamente falso.
    static void construirOjos(MallaAnimal& malla,
                              const ParametrosPecari& p,
                              const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;
        const float zCraneo = largoTronco * 0.48f + p.largoCuello;

        // â­ EL OJO SE APOYA EN EL CRANEO, NO FLOTA A UNA ALTURA FIJA
        //
        // Antes el ojo era un tubito colocado en una coordenada calculada a
        // ojo, del mismo diametro por delante y por detras, y con el eje en Z
        // -- o sea apuntando hacia ADELANTE. En un animal con los ojos
        // LATERALES eso lo dejaba medio enterrado en la mejilla: existia en la
        // malla pero apenas se veia asomar.
        //
        // Aqui se calcula donde esta de verdad la superficie del craneo a la
        // altura del ojo, con las mismas formulas de construirCuelloYCabeza, y
        // el ojo se pone AHI. La cuenca sigue a la cabeza aunque cambien las
        // proporciones (una cria tiene la cara mas corta y mas redonda).
        const float zOjo = zCraneo + p.largoCabeza * 0.34f;

        // Interpolar el craneo entre su seccion trasera (z=zCraneo) y la
        // mejilla (z=zCraneo+largoCabeza*0.42): son las dos que rodean al ojo.
        const float tOjo = 0.34f / 0.42f;
        const float radioXCraneo = p.anchoCabeza * 0.5f
                                 + (p.anchoCabeza * 0.46f - p.anchoCabeza * 0.5f) * tOjo;
        const float radioYCraneo = p.altoCabeza * 0.5f
                                 + (p.altoCabeza * 0.44f - p.altoCabeza * 0.5f) * tOjo;
        const float centroYCraneo = (ejeY + p.altoTorso * 0.12f)
                                  - p.altoCabeza * 0.045f * tOjo;

        // Altura del ojo: en el tercio superior de la cara, que es donde esta
        // en un suido. Va como fraccion del radio para que escale con la edad.
        const float yOjo = centroYCraneo + radioYCraneo * 0.42f;

        // A esa altura el craneo ya no es tan ancho como en su ecuador: se
        // toma el semieje real de la elipse para no dejar el ojo ni hundido ni
        // despegado.  x = radioX * sqrt(1 - (y/radioY)^2)
        const float ky = 0.42f;
        const float xCraneo = radioXCraneo * std::sqrt(1.0f - ky * ky);

        for (int lado = 0; lado < 2; ++lado) {
            const float sx = (lado == 0) ? -1.0f : 1.0f;

            // ================================================================
            // EL GLOBO OCULAR
            // ================================================================
            // Un elipsoide corto en el eje X (o sea, MIRANDO HACIA EL LADO) en
            // vez de un tubo hacia adelante. Tres anillos: entra en la cuenca,
            // asoma en su punto mas ancho, y cierra en la cornea.
            //
            // El eje se construye en Z y se rota 90 grados llevando la
            // coordenada Z a la X, que es lo que apunta el ojo hacia fuera.
            std::vector<SeccionCuerpo> secs;
            const float rOjo = p.diametroOjo * 0.5f;

            // Dentro de la cuenca (queda oculto: da el cierre).
            SeccionCuerpo a;
            a.z = -rOjo * 0.75f;
            a.radioX = rOjo * 0.52f;
            a.radioY = rOjo * 0.52f;
            a.centroY = 0.0f;
            a.zonaLomo = a.zonaFlanco = a.zonaVientre = ZonaCuerpo::OJO;
            secs.push_back(a);

            // El ecuador: el punto mas ancho, justo en la superficie.
            SeccionCuerpo b = a;
            b.z = 0.0f;
            b.radioX = rOjo;
            b.radioY = rOjo * 0.86f;      // ligeramente ovalado, como un ojo real
            secs.push_back(b);

            // La cornea, que sobresale un poco: es lo que capta la luz y hace
            // que el ojo se lea como una esfera humeda y no como un disco.
            SeccionCuerpo c = a;
            c.z = rOjo * 0.62f;
            c.radioX = rOjo * 0.60f;
            c.radioY = rOjo * 0.52f;
            secs.push_back(c);

            const size_t baseOjo = malla.vertices.size();
            // ⚠️ EL OJO NO SE CUADRA, Y ES LA UNICA EXCEPCION DEL MODELO.
            //
            // Todo lo demas pasa a seccion de caja para encajar en un mundo de
            // voxeles, pero un globo ocular cuadrado no se lee como un ojo: se
            // lee como un error. Es una esfera humeda, la unica pieza del
            // animal cuya forma redonda es su rasgo.
            //
            // Minecraft hace lo mismo con los suyos: el cuerpo es de cajas y
            // los ojos van pintados en la textura, nunca facetados.
            GeneradorMalla::coserTubo(malla, secs, cfg.ladosPata, p.semilla,
                                      true, true);

            // Girar el ojo para que MIRE HACIA EL LADO y llevarlo a su sitio.
            // Los ojos laterales son la firma de una presa: campo visual casi
            // panoramico a costa de poca vision binocular.
            for (size_t i = baseOjo; i < malla.vertices.size(); ++i) {
                V3& q = malla.vertices[i].pos;
                const float ejeLateral = q.z;    // el eje del elipsoide
                const float anchoZ     = q.x;    // su seccion, hacia adelante
                q.x = sx * (xCraneo * 0.94f + ejeLateral);
                q.z = zOjo + anchoZ;
                q.y = yOjo + q.y;
            }

            for (size_t i = baseOjo; i < malla.vertices.size(); ++i) {
                malla.vertices[i].zona = ZonaCuerpo::OJO;
                malla.vertices[i].pelo = 0.0f;
            }

            // ================================================================
            // LA PUPILA: HORIZONTAL, COMO LA DE UN HERBIVORO PRESA
            // ================================================================
            // El globo era de un solo color, o sea una canica oscura. Lo que
            // convierte eso en una MIRADA es el contraste iris/pupila y, sobre
            // todo, LA FORMA de la pupila.
            //
            // MEDIDO en la retina de esta especie: hay una FRANJA VISUAL
            // HORIZONTAL (visual streak), una banda alargada de alta densidad
            // de celulas ganglionares. Eso significa que ve mejor a lo ancho
            // del horizonte que arriba y abajo, y va acompanado de pupila
            // HORIZONTAL -- el patron de cabra, oveja o caballo, no el circulo
            // de un depredador.
            //
            // Es un rasgo que casi ninguna representacion de fauna acierta, y
            // aqui esta respaldado por el dato, no elegido por estilo.
            //
            // Se dibuja como una losa muy achatada pegada a la cornea, apenas
            // por delante para que no haga z-fighting con el globo.
            {
                std::vector<SeccionCuerpo> pup;
                SeccionCuerpo q1;
                q1.z = rOjo * 0.30f;
                q1.radioX = rOjo * 0.62f;    // ancha...
                q1.radioY = rOjo * 0.26f;    // ...y baja: horizontal
                q1.centroY = 0.0f;
                q1.zonaLomo = q1.zonaFlanco = q1.zonaVientre = ZonaCuerpo::PUPILA;
                pup.push_back(q1);

                SeccionCuerpo q2 = q1;
                q2.z = rOjo * 0.70f;
                q2.radioX = rOjo * 0.46f;
                q2.radioY = rOjo * 0.19f;
                pup.push_back(q2);

                const size_t basePup = malla.vertices.size();
                // La pupila tampoco se cuadra: va pegada al globo y tiene que
                // seguir su curvatura, o se despegaria en las esquinas.
                GeneradorMalla::coserTubo(malla, pup, cfg.ladosPata, p.semilla,
                                          false, true);

                // Misma transformacion que el globo: el ojo mira al lado.
                for (size_t i = basePup; i < malla.vertices.size(); ++i) {
                    V3& q = malla.vertices[i].pos;
                    const float ejeLateral = q.z;
                    const float anchoZ     = q.x;
                    q.x = sx * (xCraneo * 0.94f + ejeLateral);
                    q.z = zOjo + anchoZ;
                    q.y = yOjo + q.y;
                }
                GeneradorMalla::marcarZona(malla, basePup,
                                           ZonaCuerpo::PUPILA, 0.0f);
            }

            // ================================================================
            // EL PARPADO / REBORDE DE LA ORBITA
            // ================================================================
            // Lo que hace que el ojo PAREZCA un ojo y no una canica pegada: un
            // anillo de piel algo mas oscura alrededor, ligeramente elevado.
            // Sin el, el globo sale del craneo sin transicion y se ve como una
            // pelota incrustada.
            //
            // Es barato: un solo anillo, y solo en los LOD que ya dibujan ojos.
            {
                std::vector<SeccionCuerpo> orb;
                SeccionCuerpo o1;
                o1.z = -rOjo * 0.30f;
                o1.radioX = rOjo * 1.30f;
                o1.radioY = rOjo * 1.12f;
                o1.centroY = 0.0f;
                o1.zonaLomo = o1.zonaFlanco = o1.zonaVientre = ZonaCuerpo::CABEZA;
                orb.push_back(o1);

                SeccionCuerpo o2 = o1;
                o2.z = rOjo * 0.16f;
                o2.radioX = rOjo * 1.16f;
                o2.radioY = rOjo * 0.98f;
                orb.push_back(o2);

                const size_t baseOrb = malla.vertices.size();
                GeneradorMalla::coserTubo(malla, orb, cfg.ladosPata, p.semilla,
                                          false, false);

                for (size_t i = baseOrb; i < malla.vertices.size(); ++i) {
                    V3& q = malla.vertices[i].pos;
                    const float ejeLateral = q.z;
                    const float anchoZ     = q.x;
                    q.x = sx * (xCraneo * 0.92f + ejeLateral);
                    q.z = zOjo + anchoZ;
                    q.y = yOjo + q.y;
                }

                // Piel desnuda alrededor del ojo: sin pelo, y se queda con el
                // color de la cabeza (lo oscurece el sombreado del reborde).
                for (size_t i = baseOrb; i < malla.vertices.size(); ++i) {
                    malla.vertices[i].zona = ZonaCuerpo::CABEZA;
                    malla.vertices[i].pelo = 0.0f;
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    // LA CRIN DORSAL
    // ------------------------------------------------------------------------
    // De la CORONILLA a la GRUPA â€” no solo el lomo. Es un detalle que casi
    // todas las representaciones se saltan.
    //
    // NO es un pelo por geometria: es UNA cresta continua, un solo tubo fino
    // que sigue la linea dorsal. El aspecto de pelo lo dan el color y la
    // perturbacion de normales.
    static void construirCrin(MallaAnimal& malla,
                              const ParametrosPecari& p,
                              const ConfigLOD& cfg) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;
        const float ejeY = p.alturaCruz - p.altoTorso * 0.5f;
        const float zGrupa = -largoTronco * 0.50f;
        const float zNuca  = largoTronco * 0.48f + p.largoCuello * 0.9f;

        // Los mismos numeros con los que construirTronco define el barril. Se
        // repiten aqui a proposito: es lo que permite EVALUAR la superficie del
        // lomo en cualquier z en vez de aproximarla.
        const float z0Tronco = -largoTronco * 0.52f;
        const float z1Tronco =  largoTronco * 0.48f;

        // â­ LA CRIN VA PEGADA AL LOMO, Y ANTES FLOTABA
        //
        // Estaba colgada de una constante: `ejeY + altoTorso*0.5*1.30`. Ese
        // 1.30 la subia un 30% por encima del radio del torso, o sea unos 2 cm
        // de aire entre el lomo y la crin. Se veia como una linea suelta
        // flotando sobre la espalda -- que es exactamente lo que hay que
        // corregir.
        //
        // Y ademas era una linea RECTA sobre un lomo que NO lo es: el tronco
        // tiene perfil de barril (mas grueso al centro), la grupa caida, y el
        // lomo alzado de forma creciente hacia la cruz. Aunque se bajara el
        // 1.30 al valor justo, la crin solo tocaria el lomo en un punto y
        // seguiria despegada en el resto.
        //
        // Asi que aqui se EVALUA la altura real de la superficie dorsal para
        // cada z, con las mismas formulas de construirTronco. La crin se apoya
        // sobre lo que hay debajo, sea cual sea la forma.
        auto alturaLomoEn = [&](float z) -> float {
            // Donde cae esta z dentro del tronco (0 grupa .. 1 pecho).
            float t = (z - z0Tronco) / (z1Tronco - z0Tronco);
            if (t < 0.0f) t = 0.0f;
            if (t > 1.0f) t = 1.0f;

            // Perfil de barril: identico a construirTronco.
            const float d = (t - 0.58f) / 0.58f;
            const float perfil = 1.0f - 0.22f * d * d;
            const float radioY = p.altoTorso * 0.5f * perfil;

            // Grupa caida: identico a construirTronco.
            const float caidaGrupa = (1.0f - t) * (1.0f - t) * 0.035f;
            const float centroY = ejeY - caidaGrupa * p.alturaCruz;

            // El alzado del lomo estira la mitad superior del anillo un 30%
            // como maximo (ver la deformacion 2 de coserTubo). La cresta del
            // lomo es justo ese punto mas alto.
            const float alzado = 0.12f + 0.22f * t;
            return centroY + radioY * (1.0f + alzado * 0.30f);
        };

        std::vector<SeccionCuerpo> secs;
        const int N = cfg.seccionesCuerpo;

        for (int i = 0; i < N; ++i) {
            const float t = (float)i / (float)(N - 1);
            SeccionCuerpo s;
            s.z = zGrupa + (zNuca - zGrupa) * t;

            // La crin es mas alta hacia la cruz y se afina en los extremos.
            const float perfil = std::sin(3.14159265f * (0.15f + 0.85f * t));
            s.radioX = p.anchoCollar * 0.16f;
            s.radioY = p.altoCrinReposo * 0.5f * (0.5f + 0.9f * perfil);

            // â­ SE HUNDE EN EL LOMO, NO SE POSA ENCIMA.
            //
            // El centro del tubo se coloca de modo que su mitad baja quede
            // METIDA bajo la piel. Dos razones:
            //
            //   1. Las cerdas de verdad SALEN de la piel: nacen dentro. Una
            //      crin apoyada justo encima deja una costura visible.
            //   2. Sin solape, el hueco entre dos superficies curvas se abre
            //      en cuanto la malla se dobla al animarse.
            //
            // EL CENTRO VA EN LA PROPIA LINEA DEL LOMO. Asi la mitad inferior
            // del tubo queda por debajo de la piel y solo asoma la mitad de
            // arriba, que es la cresta de pelo visible. Medido sobre la malla:
            // el vertice mas bajo de la crin queda ~1 cm POR DEBAJO de la
            // superficie del lomo, y el mas alto sobresale ~2 cm -- que es
            // justo la altura de crin en reposo que declara la anatomia.
            const float yLomo = alturaLomoEn(s.z);
            s.centroY = yLomo;

            s.zonaLomo = s.zonaFlanco = s.zonaVientre = ZonaCuerpo::CRIN;
            secs.push_back(s);
        }

        const size_t base = malla.vertices.size();
        // Cuadrar la seccion: el animal es de un mundo de voxeles.
        GeneradorMalla::aplicarCuadratura(secs, p.cuadratura);
        GeneradorMalla::coserTubo(malla, secs, 4, p.semilla, true, true);
        // Zona propia: es lo que le permite ser blanco grisaceo sin arrastrar
        // consigo la raya dorsal oscura. Antes iba marcada como LOMO.
        GeneradorMalla::marcarZona(malla, base, ZonaCuerpo::CRIN, 1.0f);
    }

    // ------------------------------------------------------------------------
    // COLOR: zonas anatomicas + jaspeado AGUTI
    // ------------------------------------------------------------------------
    // El punto 9 pide variacion procedural y el 10 que respete regiones
    // anatomicas. Ambas cosas se resuelven aqui, en CPU, una sola vez.
    //
    // El pelo AGUTI (bandas alternas en cada pelo) es lo que hace que el
    // animal se vea "grizzled black and gray" â€” jaspeado, no gris liso. Se
    // reproduce variando el tono por vertice con ruido de baja frecuencia.
    static void colorear(MallaAnimal& malla, const ParametrosPecari& p) {
        const float largoTronco = p.largoCuerpo * p.fraccionTronco;

        for (size_t i = 0; i < malla.vertices.size(); ++i) {
            VerticeAnimal& v = malla.vertices[i];

            const float* c = p.colorBase;
            switch (v.zona) {
                case ZonaCuerpo::LOMO:
                case ZonaCuerpo::GRUPA:   c = p.colorLomo;    break;
                case ZonaCuerpo::CRIN:    c = p.colorCrin;    break;
                case ZonaCuerpo::VIENTRE: c = p.colorVientre; break;
                case ZonaCuerpo::PATA:    c = p.colorPata;    break;
                case ZonaCuerpo::PEZUNA:  c = p.colorPezuna;  break;
                case ZonaCuerpo::HOCICO:  c = p.colorHocico;  break;
                case ZonaCuerpo::OJO:     c = p.colorOjo;     break;
                case ZonaCuerpo::PUPILA:  c = p.colorPupila;  break;
                default:                  c = p.colorBase;    break;
            }

            float r = c[0], g = c[1], b = c[2];

            // --- MATICES POR ZONA ---
            //
            // CUELLO, OREJA y COLA caian en la rama por defecto, asi que salian
            // EXACTAMENTE del mismo color que el torso. Un animal de un solo
            // tono se lee como plastico: lo que da la sensacion de pelaje son
            // las diferencias suaves entre partes, no una textura.
            //
            // Son ajustes pequenos --y sobre el color ya elegido, no colores
            // nuevos-- para que sigan leyendose como el mismo animal.
            if (v.zona == ZonaCuerpo::OREJA) {
                // La oreja tiene el pelo mas ralo: se transparenta un poco y
                // tira a rosado por la piel de debajo.
                r *= 1.06f; g *= 0.98f; b *= 0.96f;
            } else if (v.zona == ZonaCuerpo::CUELLO) {
                // El cuello es algo mas oscuro que el flanco: ahi el pelo es
                // mas denso y largo, y se apelmaza.
                r *= 0.94f; g *= 0.94f; b *= 0.95f;
            } else if (v.zona == ZonaCuerpo::COLA) {
                r *= 0.88f; g *= 0.88f; b *= 0.90f;
            } else if (v.zona == ZonaCuerpo::HOCICO) {
                // --- LAS FOSAS NASALES ---
                //
                // Se identifican por estar METIDAS hacia dentro respecto a la
                // cara del disco: son las unicas partes del hocico con z menor
                // que el frente. Se oscurecen porque son el interior de un
                // orificio, y eso es lo que las hace legibles como agujeros y
                // no como dos manchas.
                //
                // Va aqui y no en construirNariz porque colorear() corre
                // despues y sobrescribiria cualquier color puesto antes.
                const float zFrente = largoTronco * 0.48f + p.largoCuello
                                    + p.largoCabeza + p.largoHocico;
                const float dentro = zFrente - v.pos.z;
                if (dentro > p.discoRinarial * 0.02f &&
                    dentro < p.discoRinarial * 0.42f) {
                    // Cuanto mas al fondo, mas oscuro: da profundidad.
                    const float k = dentro / (p.discoRinarial * 0.42f);
                    const float f = 1.0f - 0.72f * k;
                    r *= f; g *= f; b *= f;
                }
            }

            // --- EL COLLAR ---
            // Banda clara que cruza HOMBRO Y CUELLO. Es diagnostica: da nombre
            // a la especie. Se aplica por posicion, como una mascara
            // procedural.
            const float zCollar = largoTronco * 0.5f * (p.posCollarZ / 0.135f) * 0.30f
                                + largoTronco * 0.30f;
            // El collar NO pinta el ojo ni la pezuna: son piel desnuda y
            // queratina, no llevan pelo que pueda ser claro. Sin esta
            // exclusion, un pecari con el collar a la altura de la cara
            // acabaria con los ojos pintados de crema.
            //
            // La CRIN tambien queda fuera: ya tiene su propio tono claro, y
            // mezclarle encima el crema del collar le quitaria el gris que la
            // distingue de la banda del hombro. Son dos rasgos claros
            // distintos y deben leerse como tales.
            if (v.zona != ZonaCuerpo::PATA && v.zona != ZonaCuerpo::PEZUNA &&
                v.zona != ZonaCuerpo::OJO  && v.zona != ZonaCuerpo::CRIN &&
                v.zona != ZonaCuerpo::PUPILA) {
                const float d = std::fabs(v.pos.z - zCollar);
                if (d < p.anchoCollar) {
                    // Transicion suave en los bordes: un collar con corte duro
                    // se veria pintado.
                    //
                    // EL TOPE SUBE DE 0.90 A 1.0 AL OSCURECER EL PELAJE.
                    //
                    // Con el cuerpo en gris pardo (0.285) quedarse en 0.90
                    // apenas se notaba: el 10% de color de fondo que se colaba
                    // era casi del mismo tono. Con el cuerpo en negro (0.088)
                    // ese mismo 10% arrastra el collar hacia abajo y lo apaga
                    // -- medido: de 0.69 a 0.37 de luminancia, o sea la mitad.
                    //
                    // El collar es DIAGNOSTICO: da nombre a la especie y es lo
                    // que la distingue de un jabali a simple vista. No puede
                    // depender de lo oscuro que sea el resto del animal.
                    //
                    // El degradado de los bordes lo sigue dando k*k, asi que no
                    // reaparece el corte duro: solo el CENTRO de la banda llega
                    // a su color pleno, que es como se ve en el animal real.
                    //
                    // Y k*k NO BASTA, por una razon de muestreo: el centro de
                    // la banda cae ENTRE dos anillos de la malla. Medido, el
                    // vertice mas cercano queda a 0.0174 m de un collar de
                    // 0.042 m de ancho, o sea k=0.59 y k*k=0.34: ningun
                    // vertice llegaba nunca al color pleno del collar.
                    //
                    // Con el cuerpo gris eso pasaba desapercibido. Con el
                    // cuerpo negro el collar se quedaba a medio camino y
                    // perdia la mitad de su luminancia.
                    //
                    // Se sustituye por una MESETA con bordes suaves: dentro
                    // del tercio central la mezcla es plena, y solo se degrada
                    // en los dos tercios exteriores. Asi el collar tiene su
                    // color de verdad sin depender de que un vertice caiga
                    // justo en el centro, y los bordes siguen fundiendo.
                    const float k = 1.0f - (d / p.anchoCollar);
                    const float meseta = (k - 0.35f) / 0.30f;   // 0 en k=0.35, 1 en k=0.65
                    const float mezcla = (meseta <= 0.0f) ? (k * k)
                                       : (meseta >= 1.0f) ? 1.0f
                                       : (k * k) * (1.0f - meseta) + 1.0f * meseta;
                    r = r * (1.0f - mezcla) + p.colorCollar[0] * mezcla;
                    g = g * (1.0f - mezcla) + p.colorCollar[1] * mezcla;
                    b = b * (1.0f - mezcla) + p.colorCollar[2] * mezcla;
                }
            }

            // --- JASPEADO AGUTI ---
            // Dos frecuencias, como pide el punto 9: una lenta que da manchas
            // grandes y otra rapida que da el grano del pelo.
            const float n1 = GeneradorMalla::ruido(p.semilla, (int)i, 0) - 0.5f;
            const float n2 = GeneradorMalla::ruido(p.semilla + 991, (int)i / 3, 1) - 0.5f;
            float jasp = (n1 * 0.65f + n2 * 0.35f) * p.variacionColor * v.pelo;

            // â­ EL JASPEADO SE SESGA HACIA LA LUZ EN LO OSCURO.
            //
            // El aguti es ADITIVO y simetrico: +-0.08 alrededor del tono base.
            // Con el pelaje ya en negro (lomo a 0.048) la mitad negativa se
            // recorta contra 0 y desaparece -- el animal perderia justo en las
            // zonas mas oscuras el jaspeado que lo hace parecer pelo, y saldria
            // una silueta plana. Es el efecto contrario al que se busca.
            //
            // Se corrige inclinando la mezcla hacia arriba cuanto mas oscuro es
            // el tono base. En lo claro (collar, crin) no cambia nada: ahi hay
            // recorrido de sobra en las dos direcciones.
            //
            // Es lo mismo que hace un pelaje negro real bajo el sol: no se ve
            // "mas negro" en las sombras del pelo, se ven los BRILLOS de las
            // puntas. El contraste del negro va hacia la luz, no hacia el vacio.
            const float luma = r * 0.30f + g * 0.59f + b * 0.11f;
            if (luma < 0.30f) {
                // 0 en luma=0.30 (sin sesgo) -> 1 en luma=0 (todo hacia arriba)
                const float oscuridad = 1.0f - (luma / 0.30f);
                if (jasp < 0.0f) jasp *= (1.0f - oscuridad * 0.80f);
                else             jasp *= (1.0f + oscuridad * 0.90f);
            }

            r += jasp; g += jasp; b += jasp;

            // Sin recortar, el pipeline fijo satura y salen manchas quemadas.
            v.r = (r < 0.0f) ? 0.0f : (r > 1.0f ? 1.0f : r);
            v.g = (g < 0.0f) ? 0.0f : (g > 1.0f ? 1.0f : g);
            v.b = (b < 0.0f) ? 0.0f : (b > 1.0f ? 1.0f : b);
        }
    }
};

} // namespace Fauna

#endif // PECARI_ANATOMIA_H
