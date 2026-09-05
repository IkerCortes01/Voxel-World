#ifndef ANIMAL_SKINNING_H
#define ANIMAL_SKINNING_H

#include <cstdint>
#include <cmath>
#include <vector>
#include "AnimalMalla.h"

// ============================================================================
// SKINNING: LA MALLA SE DOBLA POR SUS ARTICULACIONES
// ============================================================================
// RESPONSABILIDAD UNICA: dada una malla en reposo y una pose de huesos,
// producir la malla deformada. No sabe que animal es, no dibuja, no conoce
// OpenGL.
//
// ----------------------------------------------------------------------------
// EL PROBLEMA QUE RESUELVE
// ----------------------------------------------------------------------------
// La malla organica se generaba UNA VEZ por (especie, etapa, LOD) y se
// compartia entre todos los animales. Eso ahorra memoria --100 pecaries usan 1
// malla-- pero tiene una consecuencia que se ve enseguida:
//
//     LA MALLA ERA UNA ESTATUA.
//
// No habia forma de que las patas se movieran. Un pecari caminando era una
// figura rigida deslizandose por el suelo, con las patas clavadas. Habia un
// arbol de huesos completo en PecariCuerpo.h --con codo y rodilla doblando en
// sentidos opuestos, y los metapodios fusionados que MIDE la anatomia-- pero
// solo lo usaba el camino de CAJAS, que el render ya no dibuja.
//
// Aqui se unen las dos mitades: la malla organica pasa a doblarse por el mismo
// esqueleto que ya estaba descrito.
//
// ----------------------------------------------------------------------------
// COMO FUNCIONA, Y POR QUE ASI
// ----------------------------------------------------------------------------
// Cada vertice lleva UN hueso y UN peso (0..1):
//
//   peso = 1  el vertice sigue al hueso entero (mitad de la cana)
//   peso = 0  el vertice no se entera (mitad del lomo)
//   0 < peso < 1  se mezcla, y es lo que evita el pellizco en las uniones
//
// UN solo hueso por vertice, no cuatro como en un motor moderno. Es una
// decision deliberada:
//
//   - El pecari tiene articulaciones de BISAGRA (MEDIDO: radio y ulna
//     fusionados, metapodios 3-4 fusionados). No hay torsion que repartir.
//   - Con el peso graduado en la zona de union, una sola influencia da un
//     resultado indistinguible a la distancia a la que se ve el animal.
//   - Cuesta 4 bytes por vertice en vez de 16, y una multiplicacion en vez de
//     cuatro. Con 30 animales cerca eso se nota.
//
// ----------------------------------------------------------------------------
// COSTE
// ----------------------------------------------------------------------------
// El skinning se hace en CPU, por animal y por frame, SOLO para los animales
// cercanos (LOD 0 y 1). Los lejanos se dibujan con la malla en reposo, que es
// lo que ya se hacia: a 30 bloques nadie ve doblarse un codo.
//
// LOD 0 son 715 vertices; LOD 1, 453. Un animal en LOD 0 cuesta ~715
// transformaciones de punto y normal por frame. La malla deformada se escribe
// en un buffer REUTILIZADO entre animales, asi que no hay reserva de memoria
// por frame.
// ============================================================================

namespace Fauna {

// ----------------------------------------------------------------------------
// LOS HUESOS
// ----------------------------------------------------------------------------
// Trece: el tronco, la cadena cuello-cabeza, y tres segmentos por pata.
//
// Es el mismo esqueleto que describe PecariCuerpo.h, con las mismas
// articulaciones y las mismas que NO existen:
//
//   - No hay rotacion de antebrazo: radio y ulna estan FUSIONADOS (MEDIDO).
//   - La cana es UNA pieza: los metapodios 3-4 estan fusionados (MEDIDO).
//   - Todas las articulaciones son BISAGRAS en el plano de avance.
//
// Modelar rotulas libres seria contradecir el hueso.
enum class HuesoAnimal : uint8_t {
    TRONCO = 0,     // la raiz: no se mueve respecto al animal
    CUELLO,
    CABEZA,         // gira aparte del cuerpo: mirar sin girarse entero
    COLA,

    HOMBRO_DI,      // delantera izquierda
    CODO_DI,
    CANA_DI,

    HOMBRO_DD,      // delantera derecha
    CODO_DD,
    CANA_DD,

    CADERA_TI,      // trasera izquierda
    RODILLA_TI,
    CORVEJON_TI,

    CADERA_TD,      // trasera derecha
    RODILLA_TD,
    CORVEJON_TD,

    _COUNT
};

constexpr int NUM_HUESOS = (int)HuesoAnimal::_COUNT;

// ----------------------------------------------------------------------------
// UN HUESO EN REPOSO
// ----------------------------------------------------------------------------
// Solo hace falta el PIVOTE (donde esta la articulacion) y el PADRE. El eje de
// giro es siempre X, porque todas son bisagras en el plano de avance.
struct HuesoReposo {
    V3      pivote;                  // en espacio local del animal, metros
    int8_t  padre = -1;              // -1 = raiz
};

// El esqueleto en reposo: donde esta cada articulacion antes de animar.
struct EsqueletoReposo {
    HuesoReposo huesos[NUM_HUESOS];
};

// ----------------------------------------------------------------------------
// LA POSE: cuanto ha girado cada hueso
// ----------------------------------------------------------------------------
// Angulos en radianes respecto al reposo. Se rellena desde la fase del paso.
struct PoseEsqueleto {
    float giroX[NUM_HUESOS] = {0};   // bisagra: el giro que de verdad usa todo
    float giroY[NUM_HUESOS] = {0};   // solo cuello y cabeza: mirar a los lados

    void limpiar() {
        for (int i = 0; i < NUM_HUESOS; ++i) { giroX[i] = 0.0f; giroY[i] = 0.0f; }
    }
};

// ----------------------------------------------------------------------------
// MATRIZ DE HUESO YA RESUELTA
// ----------------------------------------------------------------------------
// Rotacion 3x3 mas traslacion. No se usa una matriz 4x4 completa porque no hay
// escalado por hueso: sobran cuatro floats por hueso y una fila de ceros.
struct TransHueso {
    float m[9];      // rotacion, por filas
    V3    t;         // traslacion

    static TransHueso identidad() {
        TransHueso r;
        r.m[0]=1; r.m[1]=0; r.m[2]=0;
        r.m[3]=0; r.m[4]=1; r.m[5]=0;
        r.m[6]=0; r.m[7]=0; r.m[8]=1;
        r.t = V3(0,0,0);
        return r;
    }

    V3 aplicarPunto(const V3& p) const {
        return V3(m[0]*p.x + m[1]*p.y + m[2]*p.z + t.x,
                  m[3]*p.x + m[4]*p.y + m[5]*p.z + t.y,
                  m[6]*p.x + m[7]*p.y + m[8]*p.z + t.z);
    }

    // La normal NO lleva traslacion: es una direccion, no un punto. Y como la
    // matriz es una rotacion pura (sin escalado), vale la misma sin transponer
    // ni invertir nada.
    V3 aplicarDireccion(const V3& d) const {
        return V3(m[0]*d.x + m[1]*d.y + m[2]*d.z,
                  m[3]*d.x + m[4]*d.y + m[5]*d.z,
                  m[6]*d.x + m[7]*d.y + m[8]*d.z);
    }
};

// ----------------------------------------------------------------------------
// PESOS DE UN VERTICE
// ----------------------------------------------------------------------------
// 2 bytes por vertice: a que hueso pertenece y cuanto le hace caso.
//
// Va en un array APARTE de VerticeAnimal, no dentro. Dos razones:
//   - VerticeAnimal se copia al buffer deformado cada frame; los pesos no
//     cambian nunca, asi que copiarlos seria trabajo tirado.
//   - Asi VerticeAnimal no crece y las mallas ya cacheadas siguen valiendo.
struct PesoVertice {
    uint8_t hueso = 0;      // indice en HuesoAnimal
    uint8_t peso = 255;     // 0..255 -> 0..1
};

// ============================================================================
// CONSTRUIR LAS MATRICES DE LA POSE
// ============================================================================
// Recorre el arbol de padre a hijo acumulando transformaciones. El orden del
// enum garantiza que el padre siempre va antes que el hijo, asi que basta un
// recorrido lineal: no hace falta ordenar ni recursion.
inline void ResolverPose(const EsqueletoReposo& esq,
                         const PoseEsqueleto& pose,
                         TransHueso salida[NUM_HUESOS]) {
    for (int i = 0; i < NUM_HUESOS; ++i) {
        const HuesoReposo& h = esq.huesos[i];

        // Rotacion propia del hueso: bisagra en X, mas guinada en Y para el
        // cuello y la cabeza.
        const float cx = std::cos(pose.giroX[i]), sx = std::sin(pose.giroX[i]);
        const float cy = std::cos(pose.giroY[i]), sy = std::sin(pose.giroY[i]);

        // R = Ry * Rx
        float r[9];
        r[0] =  cy;      r[1] =  sy*sx;   r[2] =  sy*cx;
        r[3] =  0.0f;    r[4] =  cx;      r[5] = -sx;
        r[6] = -sy;      r[7] =  cy*sx;   r[8] =  cy*cx;

        // El giro ocurre ALREDEDOR DEL PIVOTE, no del origen. De ahi que se
        // traslade al pivote, se rote y se vuelva: es lo que hace que un codo
        // doble por el codo y no arrastre la pata entera desde el suelo.
        TransHueso local;
        for (int k = 0; k < 9; ++k) local.m[k] = r[k];
        local.t = V3(h.pivote.x - (r[0]*h.pivote.x + r[1]*h.pivote.y + r[2]*h.pivote.z),
                     h.pivote.y - (r[3]*h.pivote.x + r[4]*h.pivote.y + r[5]*h.pivote.z),
                     h.pivote.z - (r[6]*h.pivote.x + r[7]*h.pivote.y + r[8]*h.pivote.z));

        if (h.padre < 0) {
            salida[i] = local;
        } else {
            // salida[i] = salida[padre] * local
            const TransHueso& P = salida[h.padre];
            TransHueso& S = salida[i];
            for (int f = 0; f < 3; ++f) {
                for (int c = 0; c < 3; ++c) {
                    S.m[f*3+c] = P.m[f*3+0]*local.m[0*3+c]
                               + P.m[f*3+1]*local.m[1*3+c]
                               + P.m[f*3+2]*local.m[2*3+c];
                }
            }
            S.t = P.aplicarPunto(local.t);
        }
    }
}

// ============================================================================
// DEFORMAR LA MALLA
// ============================================================================
// Escribe en 'salida' la malla ya doblada. 'salida' se REUTILIZA entre
// animales y entre frames: se redimensiona solo la primera vez.
//
// El peso mezcla entre "quieto" y "movido por el hueso", que es lo que suaviza
// las uniones. Con peso 1 el vertice va entero con el hueso; con 0.5 se queda
// a medio camino y la union no se pellizca.
inline void DeformarMalla(const MallaAnimal& reposo,
                          const std::vector<PesoVertice>& pesos,
                          const TransHueso trans[NUM_HUESOS],
                          std::vector<VerticeAnimal>& salida) {
    const size_t n = reposo.vertices.size();
    if (salida.size() != n) salida.resize(n);

    // Sin pesos no hay nada que deformar: se copia tal cual. Es la salvaguarda
    // que permite que una malla sin riggear siga dibujandose.
    if (pesos.size() != n) {
        for (size_t i = 0; i < n; ++i) salida[i] = reposo.vertices[i];
        return;
    }

    for (size_t i = 0; i < n; ++i) {
        const VerticeAnimal& v = reposo.vertices[i];
        const PesoVertice& w = pesos[i];

        VerticeAnimal& o = salida[i];
        o = v;   // color, zona y pelo no cambian al doblarse

        const int h = (w.hueso < NUM_HUESOS) ? w.hueso : 0;
        const float k = w.peso * (1.0f / 255.0f);

        if (k <= 0.0f) continue;   // no le afecta ningun hueso: ya esta copiado

        const V3 pMov = trans[h].aplicarPunto(v.pos);
        const V3 nMov = trans[h].aplicarDireccion(v.normal);

        if (k >= 1.0f) {
            o.pos = pMov;
            o.normal = nMov;
        } else {
            const float j = 1.0f - k;
            o.pos = V3(v.pos.x*j + pMov.x*k,
                       v.pos.y*j + pMov.y*k,
                       v.pos.z*j + pMov.z*k);
            o.normal = V3(v.normal.x*j + nMov.x*k,
                          v.normal.y*j + nMov.y*k,
                          v.normal.z*j + nMov.z*k).normalizado();
        }
    }
}

} // namespace Fauna

#endif // ANIMAL_SKINNING_H
