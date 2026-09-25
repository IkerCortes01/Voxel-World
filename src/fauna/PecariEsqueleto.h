#ifndef PECARI_ESQUELETO_H
#define PECARI_ESQUELETO_H

#include <cmath>
#include <vector>
#include "AnimalSkinning.h"
#include "PecariAnatomia.h"

// ============================================================================
// EL ESQUELETO DEL PECARI, Y COMO SE PEGA A SU PIEL
// ============================================================================
// Dos cosas:
//
//   1. DONDE ESTA CADA ARTICULACION, derivado de los mismos parametros
//      anatomicos con los que se construye la malla. No son numeros nuevos:
//      si manana se alarga la pata, la rodilla se mueve sola.
//
//   2. QUE HUESO MUEVE CADA VERTICE, calculado por posicion. No hay una lista
//      pintada a mano: cada vertice mira donde esta y decide.
//
// ----------------------------------------------------------------------------
// POR QUE LOS PESOS SE CALCULAN Y NO SE GUARDAN
// ----------------------------------------------------------------------------
// La malla es procedural: cambia con la especie, la etapa vital y el LOD. Una
// tabla de pesos escrita a mano se quedaria desfasada en cuanto se tocara
// cualquiera de las tres, y el fallo seria silencioso -- vertices siguiendo al
// hueso equivocado.
//
// Calcularlos de la posicion garantiza que siempre casan con la malla que hay.
// Y se hace UNA VEZ, al generar la malla, no por frame.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// AMPLITUDES DE LA MARCHA
// ----------------------------------------------------------------------------
// Se toman de PecariCuerpo.h (namespace PecariMarcha), donde ya estaban
// justificadas para el camino de cajas. Aqui se replican como constantes
// propias para que este header no dependa de aquel: son el mismo animal, asi
// que los mismos numeros.
//
// El codo y la rodilla doblan en SENTIDOS OPUESTOS -- la pata delantera hacia
// atras, la trasera hacia delante. Es lo que distingue a un cuadrupedo de una
// mesa andando.
namespace Marcha {
    // Amplitud de cada articulacion al andar, en radianes.  ESTIMADO
    constexpr float AMP_HOMBRO   = 0.38f;
    constexpr float AMP_CODO     = 0.30f;
    constexpr float AMP_CANA     = 0.22f;

    constexpr float AMP_CADERA   = 0.34f;
    constexpr float AMP_RODILLA  = 0.42f;   // la trasera flexiona mas
    constexpr float AMP_CORVEJON = 0.26f;

    // MEDIDO (anatomia general de los cuadrupedos)
    constexpr float SENTIDO_DELANTERA = -1.0f;
    constexpr float SENTIDO_TRASERA   = +1.0f;

    // Proporciones de los tres segmentos, en fraccion del largo de la pata.
    // La cana es larga: es lo que hace al pecari digitigrado y buen corredor.
    // DERIVADO de las cajas de Esqueleto en PecariModelo3D.h
    constexpr float FRAC_SUPERIOR = 0.38f;   // humero / femur
    constexpr float FRAC_MEDIA    = 0.34f;   // radioulna / tibia
}

// ----------------------------------------------------------------------------
// CONSTRUIR EL ESQUELETO EN REPOSO
// ----------------------------------------------------------------------------
// Los pivotes salen de los MISMOS parametros que la malla, asi que no pueden
// descuadrarse.
inline EsqueletoReposo EsqueletoDePecari(const ParametrosPecari& p) {
    EsqueletoReposo e;

    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float ejeY        = p.alturaCruz - p.altoTorso * 0.5f;
    const float zPecho      = largoTronco * 0.48f;
    const float zCraneo     = zPecho + p.largoCuello;
    const float yHombro     = ejeY - p.altoTorso * 0.28f;
    const float largoPata   = yHombro - p.altoPezuna;

    // --- Raiz ---
    e.huesos[(int)HuesoAnimal::TRONCO] = { V3(0, ejeY, 0), -1 };

    // --- Cuello y cabeza: la cadena que permite mirar sin girarse ---
    e.huesos[(int)HuesoAnimal::CUELLO] = {
        V3(0, ejeY + p.altoTorso * 0.06f, zPecho),
        (int8_t)HuesoAnimal::TRONCO
    };
    e.huesos[(int)HuesoAnimal::CABEZA] = {
        V3(0, ejeY + p.altoTorso * 0.12f, zCraneo),
        (int8_t)HuesoAnimal::CUELLO
    };

    // --- Cola ---
    e.huesos[(int)HuesoAnimal::COLA] = {
        V3(0, ejeY + p.altoTorso * 0.30f, -largoTronco * 0.50f),
        (int8_t)HuesoAnimal::TRONCO
    };

    // --- Las cuatro patas ---
    // Tres segmentos cada una, en las proporciones medidas. El pivote de cada
    // uno es donde EMPIEZA: hombro arriba, codo a 38% de bajada, cana a 72%.
    const float yCodo = yHombro - largoPata * Marcha::FRAC_SUPERIOR;
    const float yCana = yHombro - largoPata * (Marcha::FRAC_SUPERIOR +
                                               Marcha::FRAC_MEDIA);

    struct Def { HuesoAnimal sup, med, inf; float sx, sz; };
    const Def defs[4] = {
        { HuesoAnimal::HOMBRO_DI, HuesoAnimal::CODO_DI,     HuesoAnimal::CANA_DI,
          -1.0f, p.patasDelanteZ },
        { HuesoAnimal::HOMBRO_DD, HuesoAnimal::CODO_DD,     HuesoAnimal::CANA_DD,
          +1.0f, p.patasDelanteZ },
        { HuesoAnimal::CADERA_TI, HuesoAnimal::RODILLA_TI,  HuesoAnimal::CORVEJON_TI,
          -1.0f, p.patasTraseraZ },
        { HuesoAnimal::CADERA_TD, HuesoAnimal::RODILLA_TD,  HuesoAnimal::CORVEJON_TD,
          +1.0f, p.patasTraseraZ }
    };

    for (const Def& d : defs) {
        const float px = d.sx * p.separacionPataX;
        e.huesos[(int)d.sup] = { V3(px, yHombro, d.sz), (int8_t)HuesoAnimal::TRONCO };
        e.huesos[(int)d.med] = { V3(px, yCodo,   d.sz), (int8_t)d.sup };
        e.huesos[(int)d.inf] = { V3(px, yCana,   d.sz), (int8_t)d.med };
    }

    return e;
}

// ----------------------------------------------------------------------------
// ASIGNAR CADA VERTICE A SU HUESO
// ----------------------------------------------------------------------------
// Se decide por ZONA anatomica --que el generador ya marca-- y se gradua el
// peso por posicion. La zona dice QUE hueso; la posicion, CUANTO.
//
// EL PESO GRADUADO ES LO QUE EVITA EL PELLIZCO. Un vertice del hombro con peso
// 1 se iria entero con la pata y dejaria un cono en el costado. Bajando el peso
// segun sube hacia el cuerpo, la union se estira suave.
inline void CalcularPesosPecari(const MallaAnimal& malla,
                                const ParametrosPecari& p,
                                std::vector<PesoVertice>& pesos) {
    pesos.resize(malla.vertices.size());

    const float largoTronco = p.largoCuerpo * p.fraccionTronco;
    const float ejeY        = p.alturaCruz - p.altoTorso * 0.5f;
    const float zPecho      = largoTronco * 0.48f;
    const float zCraneo     = zPecho + p.largoCuello;
    const float yHombro     = ejeY - p.altoTorso * 0.28f;
    const float largoPata   = yHombro - p.altoPezuna;

    const float yCodo = yHombro - largoPata * Marcha::FRAC_SUPERIOR;
    const float yCana = yHombro - largoPata * (Marcha::FRAC_SUPERIOR +
                                               Marcha::FRAC_MEDIA);

    // Punto medio entre las patas delanteras y traseras: separa a que tren
    // pertenece un vertice de pata.
    const float zMedioPatas = (p.patasDelanteZ + p.patasTraseraZ) * 0.5f;

    for (size_t i = 0; i < malla.vertices.size(); ++i) {
        const VerticeAnimal& v = malla.vertices[i];
        PesoVertice& w = pesos[i];

        // Por defecto: pegado al tronco, que no se mueve. Es la opcion segura.
        w.hueso = (uint8_t)HuesoAnimal::TRONCO;
        w.peso  = 0;

        switch (v.zona) {

        // --- CABEZA Y HOCICO: van enteros con el hueso de la cabeza ---
        case ZonaCuerpo::CABEZA:
        case ZonaCuerpo::HOCICO:
        case ZonaCuerpo::OREJA:
        case ZonaCuerpo::OJO:      // los ojos van con la cabeza, faltaria mas
        case ZonaCuerpo::PUPILA:   // y la pupila con el ojo, obviamente
            w.hueso = (uint8_t)HuesoAnimal::CABEZA;
            w.peso  = 255;
            break;

        // --- CUELLO: transicion. Pegado al pecho no se mueve; junto al
        //     craneo se mueve entero. Sin esto, girar la cabeza abriria un
        //     pliegue en la garganta. ---
        case ZonaCuerpo::CUELLO: {
            const float t = (v.pos.z - zPecho) / (zCraneo - zPecho + 1e-5f);
            const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
            w.hueso = (uint8_t)HuesoAnimal::CUELLO;
            w.peso  = (uint8_t)(k * 255.0f);
            break;
        }

        case ZonaCuerpo::COLLAR:
            w.hueso = (uint8_t)HuesoAnimal::CUELLO;
            w.peso  = 128;
            break;

        // --- LA CRIN: EL PELO DEL CUELLO SIGUE A LA CABEZA ---
        //
        // Se pidio que "el pelo del cuello reaccione al movimiento de la
        // cabeza". No lo hacia: CRIN no tenia caso aqui, asi que caia en el
        // `default` de arriba --TRONCO con peso 0-- y se quedaba CLAVADA
        // mientras la cabeza giraba por debajo. El animal movia la cabeza y la
        // cresta no se enteraba.
        //
        // La crin recorre el lomo de la GRUPA a la CORONILLA, asi que su
        // reaccion tiene que ser GRADUAL: la parte de atras va con el cuerpo y
        // la de delante con la cabeza. Un peso plano la arrancaria del lomo
        // por un lado o la dejaria rigida por el otro.
        //
        // Se usa el MISMO reparto que el cuello --de zPecho a zCraneo-- para
        // que el pelo y la piel de debajo se muevan juntos. Si cada uno
        // siguiera su propia curva, la crin se despegaria del lomo al girar.
        //
        // ⚠️ Y CON EL HUESO DEL CUELLO, NO EL DE LA CABEZA. El pelo que hay
        // sobre el craneo es poco; el grueso de la crin esta sobre el cuello,
        // que es lo que acompana el gesto sin exagerarlo. Con el hueso de la
        // cabeza, la cresta entera daria el bandazo completo del giro.
        case ZonaCuerpo::CRIN: {
            const float t = (v.pos.z - zPecho) / (zCraneo - zPecho + 1e-5f);
            const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
            w.hueso = (uint8_t)HuesoAnimal::CUELLO;
            w.peso  = (uint8_t)(k * 255.0f);
            break;
        }

        case ZonaCuerpo::COLA:
            w.hueso = (uint8_t)HuesoAnimal::COLA;
            w.peso  = 255;
            break;

        // --- PATAS Y PEZUNAS: el segmento sale de la ALTURA ---
        case ZonaCuerpo::PATA:
        case ZonaCuerpo::PEZUNA: {
            const bool delantera = (v.pos.z > zMedioPatas);
            const bool izquierda = (v.pos.x < 0.0f);

            HuesoAnimal sup, med, inf;
            if (delantera) {
                sup = izquierda ? HuesoAnimal::HOMBRO_DI : HuesoAnimal::HOMBRO_DD;
                med = izquierda ? HuesoAnimal::CODO_DI   : HuesoAnimal::CODO_DD;
                inf = izquierda ? HuesoAnimal::CANA_DI   : HuesoAnimal::CANA_DD;
            } else {
                sup = izquierda ? HuesoAnimal::CADERA_TI  : HuesoAnimal::CADERA_TD;
                med = izquierda ? HuesoAnimal::RODILLA_TI : HuesoAnimal::RODILLA_TD;
                inf = izquierda ? HuesoAnimal::CORVEJON_TI: HuesoAnimal::CORVEJON_TD;
            }

            if (v.pos.y <= yCana) {
                // Cana y pezuna: la punta de la pata, se mueve entera.
                w.hueso = (uint8_t)inf;
                w.peso  = 255;
            } else if (v.pos.y <= yCodo) {
                w.hueso = (uint8_t)med;
                w.peso  = 255;
            } else {
                // Tramo alto: se desvanece hacia el cuerpo. En el hombro el
                // peso baja a 0 para que la pata no arranque el costado.
                const float t = (v.pos.y - yHombro) / (yCodo - yHombro + 1e-5f);
                const float k = (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t);
                w.hueso = (uint8_t)sup;
                w.peso  = (uint8_t)(k * 255.0f);
            }
            break;
        }

        // --- TRONCO Y COMPANIA: quietos ---
        // El cuerpo no se dobla al andar. El balanceo y el sube-y-baja del
        // conjunto los aplica el render sobre el animal entero, que es mas
        // barato que moverlo vertice a vertice.
        default:
            break;
        }
    }
}

// ----------------------------------------------------------------------------
// LA POSE DE LA MARCHA
// ----------------------------------------------------------------------------
// Traduce la fase del paso a angulos de articulacion.
//
// PATRON DIAGONAL: el pecari es un ungulado y anda al paso, moviendo en
// diagonal --delantera izquierda con trasera derecha--. De ahi los desfases de
// media vuelta entre pares.
inline void PoseDeMarcha(float fasePaso, float rapidez, PoseEsqueleto& pose) {
    pose.limpiar();

    // La amplitud crece con la velocidad y satura: por encima de trote las
    // patas ya dan la zancada entera.
    float amp = rapidez / 1.6f;
    if (amp > 1.0f) amp = 1.0f;
    if (amp < 0.0f) amp = 0.0f;

    const float PI = 3.14159265f;

    // Las cuatro patas, con su desfase. Diagonal: DI va con TD.
    struct Pata {
        HuesoAnimal sup, med, inf;
        float desfase;
        float sentido;
        float ampSup, ampMed, ampInf;
    };
    const Pata patas[4] = {
        { HuesoAnimal::HOMBRO_DI, HuesoAnimal::CODO_DI, HuesoAnimal::CANA_DI,
          0.0f, Marcha::SENTIDO_DELANTERA,
          Marcha::AMP_HOMBRO, Marcha::AMP_CODO, Marcha::AMP_CANA },
        { HuesoAnimal::HOMBRO_DD, HuesoAnimal::CODO_DD, HuesoAnimal::CANA_DD,
          PI, Marcha::SENTIDO_DELANTERA,
          Marcha::AMP_HOMBRO, Marcha::AMP_CODO, Marcha::AMP_CANA },
        { HuesoAnimal::CADERA_TI, HuesoAnimal::RODILLA_TI, HuesoAnimal::CORVEJON_TI,
          PI, Marcha::SENTIDO_TRASERA,
          Marcha::AMP_CADERA, Marcha::AMP_RODILLA, Marcha::AMP_CORVEJON },
        { HuesoAnimal::CADERA_TD, HuesoAnimal::RODILLA_TD, HuesoAnimal::CORVEJON_TD,
          0.0f, Marcha::SENTIDO_TRASERA,
          Marcha::AMP_CADERA, Marcha::AMP_RODILLA, Marcha::AMP_CORVEJON }
    };

    for (const Pata& pa : patas) {
        const float f = fasePaso + pa.desfase;

        // El segmento superior oscila adelante y atras: es el que da la
        // zancada.
        pose.giroX[(int)pa.sup] = std::sin(f) * pa.ampSup * amp;

        // ====================================================================
        // ⭐ LA PATA SE ESTIRA EN APOYO Y SOLO SE DOBLA AL RECOGERLA
        // ====================================================================
        // BUG REPORTADO: las patas se veian dobladas; se pidieron "rectas, con
        // articulaciones realistas".
        //
        // La causa estaba en esta curva. Era `max(0, sin(f + pi/2))`, que vale
        // mas de cero durante MEDIO CICLO ENTERO y llega a 1 en su centro. O
        // sea: la rodilla se quedaba flexionada la mitad del tiempo, incluida
        // buena parte de la fase en que la pata sostiene el peso.
        //
        // Un ungulado hace lo contrario, y es lo que le permite estar de pie
        // sin cansarse: en APOYO la pata esta casi recta --la columna osea
        // aguanta el peso, no el musculo-- y solo se pliega en la fase de
        // VUELO, para que la pezuna despegue del suelo y no vaya arrastrando.
        //
        // La curva nueva concentra la flexion en el cuarto de ciclo del vuelo:
        // `max(0, sin)` elevado al cuadrado cae mucho mas rapido a los lados,
        // asi que la pata pasa la mayor parte del paso estirada.
        //
        //     ANTES  ▁▂▄▆█▆▄▂▁▁▁▁▁▁▁▁   flexionada medio ciclo
        //     AHORA  ▁▁▁▂▅█▅▂▁▁▁▁▁▁▁▁   flexionada solo al recoger
        const float flex = std::sin(f + PI * 0.5f);
        const float soloFlex = (flex > 0.0f) ? (flex * flex) : 0.0f;

        pose.giroX[(int)pa.med] = soloFlex * pa.ampMed * amp * pa.sentido;
        pose.giroX[(int)pa.inf] = soloFlex * pa.ampInf * amp * pa.sentido * 0.6f;
    }

    // La cabeza acompana el paso con un cabeceo leve. Un cuadrupedo andando
    // mueve la cabeza al ritmo de las patas; sin esto se ve rigido.
    pose.giroX[(int)HuesoAnimal::CUELLO] = std::sin(fasePaso * 2.0f) * 0.035f * amp;
}

// ============================================================================
// ADAPTACION AL TERRENO
// ============================================================================
// EL PROBLEMA: la altura del animal se resolvia con UNA sonda bajo su centro,
// asi que las cuatro patas se colocaban a la misma altura siempre. En una
// pendiente o sobre un escalon eso da dos resultados igual de malos: las patas
// de abajo quedan colgando en el aire, o las de arriba se hunden en la roca.
// El animal se ve flotando en diagonal, que es lo que se reporto.
//
// LA SOLUCION: se sondea el suelo BAJO CADA PATA y se corrige cada una por su
// cuenta. Es la version simple de lo que en un motor grande hace un IK de dos
// huesos -- y para una pata de tres segmentos con articulaciones de bisagra,
// que es lo que el esqueleto MEDIDO permite, la version simple basta.
//
// Se reparte en dos efectos, y el reparto importa:
//
//   1. EL TRONCO SE INCLINA (cabeceo y alabeo). Un animal en cuesta no queda
//      horizontal: se orienta con la pendiente. Esto resuelve la mayor parte y
//      es lo que mas se ve de lejos.
//
//   2. CADA PATA AJUSTA LO QUE FALTE. Lo que la inclinacion del cuerpo no
//      cubre, lo absorbe la pata: se extiende si el suelo esta mas bajo, se
//      recoge si esta mas alto.
//
// Hacerlo SOLO con las patas dejaria el cuerpo plano sobre una cuesta, que se
// ve antinatural. Hacerlo SOLO con el tronco haria que en un escalon las dos
// patas de un lado quedaran mal igualmente. Hacen falta los dos.

// Alturas del suelo bajo cada pata, en METROS y relativas a la altura del
// suelo bajo el CENTRO del animal. Positivo = ese pie pisa mas alto.
struct SueloBajoPatas {
    float delanteraIzq = 0.0f;
    float delanteraDer = 0.0f;
    float traseraIzq   = 0.0f;
    float traseraDer   = 0.0f;
};

namespace Terreno {
    // Cuanto se permite corregir, en metros. Mas alla de esto el desnivel deja
    // de ser "terreno irregular" y pasa a ser un escalon que el animal SUBE, no
    // algo a lo que se adapta estando quieto.
    //
    // 0.18 m sobre una cruz de 0.44 m es un 40%: cubre holgadamente el terreno
    // accidentado sin llegar a posturas imposibles.
    constexpr float CORRECCION_MAX_M = 0.18f;

    // Cuanto del desnivel absorbe el CUERPO inclinandose, y cuanto las patas.
    // Con 0.55 el cuerpo hace algo mas de la mitad: es lo que da la lectura de
    // "animal orientado con la cuesta" sin llegar a posturas forzadas.
    constexpr float REPARTO_TRONCO = 0.55f;

    // Limites de inclinacion del tronco, en radianes. ~17 grados. Por encima
    // el animal se veria escalando en vez de caminando.
    constexpr float INCLINACION_MAX = 0.30f;

    // Cuanto baja el hombro por radian de correccion de pata. Sale de la
    // geometria: la pata mide ~0.28 m, asi que 1 radian de giro del hueso
    // superior desplaza el pie del orden de ese largo.
    constexpr float METROS_POR_RADIAN = 0.28f;
}

inline float acotarT(float v, float lim) {
    if (v >  lim) return  lim;
    if (v < -lim) return -lim;
    return v;
}

// Aplica la adaptacion al terreno SOBRE una pose de marcha ya calculada.
//
// Se suma en vez de sustituir: el animal sigue andando mientras se adapta, que
// es justo el caso interesante -- subir una loma caminando.
inline void AplicarTerreno(const SueloBajoPatas& suelo, PoseEsqueleto& pose) {
    using namespace Terreno;

    const float di = acotarT(suelo.delanteraIzq, CORRECCION_MAX_M);
    const float dd = acotarT(suelo.delanteraDer, CORRECCION_MAX_M);
    const float ti = acotarT(suelo.traseraIzq,   CORRECCION_MAX_M);
    const float td = acotarT(suelo.traseraDer,   CORRECCION_MAX_M);

    // --- 1. INCLINACION DEL TRONCO ---
    //
    // CABECEO: si el suelo esta mas alto DELANTE, el animal apunta hacia
    // arriba. Es la media de las delanteras contra la de las traseras.
    const float frente = (di + dd) * 0.5f;
    const float detras = (ti + td) * 0.5f;

    // El signo: suelo mas alto delante -> el morro sube -> giro negativo en X
    // con el convenio del esqueleto (el mismo que usa la flexion de la marcha).
    const float cabeceo = acotarT(-(frente - detras) * REPARTO_TRONCO /
                                   METROS_POR_RADIAN, INCLINACION_MAX);
    pose.giroX[(int)HuesoAnimal::TRONCO] += cabeceo;

    // ALABEO: si el suelo esta mas alto a la izquierda, el animal se ladea.
    // El esqueleto no tiene eje de alabeo propio, asi que se reparte entre las
    // patas del lado (abajo). Es una aproximacion, y se declara como tal: con
    // huesos de bisagra pura no hay donde meter un roll del tronco.
    const float izq = (di + ti) * 0.5f;
    const float der = (dd + td) * 0.5f;
    const float ladeo = (izq - der) * (1.0f - REPARTO_TRONCO);

    // --- 2. LO QUE FALTE, A CADA PATA ---
    //
    // Al cuerpo ya inclinado le queda un residuo por pata. Se convierte a giro
    // del hueso superior: extender la pata = girar de modo que el pie baje.
    //
    // ⚠️ AQUI HUBO UN FALLO QUE CAZO UN TEST, Y CONVIENE DEJARLO ESCRITO.
    //
    // La primera version usaba `(di - frente)`, o sea la desviacion de cada
    // pata respecto a la MEDIA DE SU EJE. Eso solo recoge la diferencia
    // izquierda/derecha DENTRO del eje, y tira lo demas.
    //
    // Consecuencia: en una cuesta uniforme --las dos patas delanteras
    // igual de altas-- ese termino daba CERO para las cuatro. El tronco se
    // inclinaba y las patas no hacian nada, asi que los pies acababan
    // hundidos o en el aire. Justo el caso mas comun, y el que toda esta
    // funcion existe para resolver.
    //
    // Lo correcto es el residuo contra el suelo REAL de cada pie: la parte del
    // desnivel que la inclinacion del tronco no ha llegado a cubrir.
    const float resto = 1.0f - REPARTO_TRONCO;

    struct Ajuste { HuesoAnimal hueso; float metros; };
    const Ajuste ajustes[4] = {
        { HuesoAnimal::HOMBRO_DI, di * resto + ladeo * 0.5f },
        { HuesoAnimal::HOMBRO_DD, dd * resto - ladeo * 0.5f },
        { HuesoAnimal::CADERA_TI, ti * resto + ladeo * 0.5f },
        { HuesoAnimal::CADERA_TD, td * resto - ladeo * 0.5f },
    };

    for (const Ajuste& a : ajustes) {
        // Suelo mas alto bajo ese pie -> la pata se RECOGE (el pie sube).
        const float giro = acotarT(a.metros / METROS_POR_RADIAN,
                                   INCLINACION_MAX);
        pose.giroX[(int)a.hueso] += giro;
    }
}

} // namespace Fauna

#endif // PECARI_ESQUELETO_H
