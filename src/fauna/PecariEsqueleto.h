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

        // El codo y la rodilla solo FLEXIONAN, nunca al reves: una pata no se
        // dobla hacia el otro lado. De ahi el max(0, ...): la articulacion se
        // pliega en la fase de recogida y se estira del todo en la de apoyo.
        const float flex = std::sin(f + PI * 0.5f);
        const float soloFlex = (flex > 0.0f) ? flex : 0.0f;

        pose.giroX[(int)pa.med] = soloFlex * pa.ampMed * amp * pa.sentido;
        pose.giroX[(int)pa.inf] = soloFlex * pa.ampInf * amp * pa.sentido * 0.6f;
    }

    // La cabeza acompana el paso con un cabeceo leve. Un cuadrupedo andando
    // mueve la cabeza al ritmo de las patas; sin esto se ve rigido.
    pose.giroX[(int)HuesoAnimal::CUELLO] = std::sin(fasePaso * 2.0f) * 0.035f * amp;
}

} // namespace Fauna

#endif // PECARI_ESQUELETO_H
