#ifndef PECARI_CARA_H
#define PECARI_CARA_H

#include <cmath>
#include <cstdint>
#include <vector>
#include "AnimalSkinning.h"

// ============================================================================
// LA CARA VIVA: BOCA, OJOS Y PARPADOS
// ============================================================================
// PEDIDO: "boca, ojos, parpados [...] animaciones de todo".
//
//   BOCA      la mandibula es un hueso: masca al comer, jadea al huir y castañea
//             los colmillos al encarar (el castañeteo de alarma de la especie).
//   OJOS      un hueso por ojo: el globo y la pupila giran para mirar alrededor.
//   PARPADOS  un casquete sobre cada ojo. En reposo de la malla esta CERRADO;
//             abrirlo es encogerlo hacia arriba hasta quedar como un pliegue.
//
// Todo de cerca (LOD 0 y 1): es donde existen los ojos.
// ============================================================================

namespace Fauna {

struct GestoCara {
    float boca = 0.0f;          // 0 cerrada .. 1 abierta del todo
    float parpadoI = 0.0f;      // 0 abierto .. 1 cerrado
    float parpadoD = 0.0f;
    float mirada = 0.0f;        // radianes: los ojos hacia delante (+) o atras
};

constexpr float BOCA_MAXIMA = 0.42f;   // radianes de apertura de la mandibula

inline float azar01(uint32_t v) {
    v ^= v >> 16; v *= 0x7feb352du; v ^= v >> 15; v *= 0x846ca68bu; v ^= v >> 16;
    return (v & 0xFFFFu) / 65535.0f;
}

// El parpadeo: cada pocos segundos, distinto en cada animal, 0.16 s cerrado.
inline float parpadeo(float t, uint32_t semilla) {
    const float periodo = 2.6f + 3.0f * azar01(semilla * 7u + 3u);
    const float fase = std::fmod(t + periodo * azar01(semilla), periodo);
    constexpr float DURA = 0.16f;
    if (fase > DURA) return 0.0f;
    return 1.0f - std::fabs(fase / DURA * 2.0f - 1.0f);    // baja y sube
}

// conducta: 0 calma, 1 alerta, 2 huida, 3 defensa (ConductaPecari).
inline GestoCara GestoDe(int conducta, float t, uint32_t semilla, float rapidez) {
    GestoCara g;
    const float p = parpadeo(t, semilla);
    g.parpadoI = g.parpadoD = p;
    const float s = azar01(semilla + 11u) * 6.2831853f;

    switch (conducta) {
        case 1:   // alerta: boca cerrada, ojos fijos y bien abiertos
            g.parpadoI = g.parpadoD = p * 0.6f;
            break;
        case 2:   // huida: jadea
            g.boca = 0.25f + 0.12f * std::sin(t * 9.0f + s);
            break;
        case 3: { // defensa: castañetea los colmillos
            const float c = std::sin(t * 15.0f + s);
            g.boca = c > 0.0f ? 0.75f * c : 0.0f;
            break;
        }
        default:  // calma: quieto, a ratos masca; los ojos curiosean
            if (rapidez < 0.3f) {
                const float rato = std::sin(t * 0.45f + s);
                if (rato > 0.3f) g.boca = 0.10f + 0.10f * (0.5f + 0.5f * std::sin(t * 8.0f));
            }
            g.mirada = 0.30f * std::sin(t * 0.6f + s) * std::sin(t * 0.23f + s * 0.5f);
            break;
    }
    return g;
}

// El muerto: boca entreabierta y ojos entornados.
inline GestoCara GestoMuerto() {
    GestoCara g;
    g.boca = 0.35f;
    g.parpadoI = g.parpadoD = 0.6f;
    return g;
}

inline void AplicarGesto(const GestoCara& g, PoseEsqueleto& pose) {
    pose.giroX[(int)HuesoAnimal::MANDIBULA] = g.boca * BOCA_MAXIMA;
    pose.giroY[(int)HuesoAnimal::OJO_I] = g.mirada;
    pose.giroY[(int)HuesoAnimal::OJO_D] = g.mirada;
}

// ----------------------------------------------------------------------------
// LOS PARPADOS, DESPUES DEL SKINNING
// ----------------------------------------------------------------------------
// Se rehacen desde la malla en reposo: se pliegan hacia el borde de arriba
// del ojo segun lo abiertos que esten, y van con el hueso de la cabeza.
inline void AnimarParpados(const MallaAnimal& reposo, const GestoCara& g,
                           const TransHueso trans[NUM_HUESOS],
                           std::vector<VerticeAnimal>& salida) {
    if (salida.size() != reposo.vertices.size()) return;

    // El borde superior de cada parpado, una vez por malla.
    struct Borde { const MallaAnimal* m; float y[2]; };
    static Borde cache[8] = {};
    static int siguiente = 0;
    const Borde* b = nullptr;
    for (const Borde& c : cache) if (c.m == &reposo) { b = &c; break; }
    if (!b) {
        Borde nuevo{ &reposo, { -1e9f, -1e9f } };
        bool hay = false;
        for (const VerticeAnimal& v : reposo.vertices) {
            if (v.zona != ZonaCuerpo::PARPADO) continue;
            const int lado = v.pos.x < 0 ? 0 : 1;
            if (v.pos.y > nuevo.y[lado]) nuevo.y[lado] = v.pos.y;
            hay = true;
        }
        if (!hay) nuevo.y[0] = nuevo.y[1] = 0.0f;
        cache[siguiente] = nuevo;
        b = &cache[siguiente];
        siguiente = (siguiente + 1) % 8;
    }

    const TransHueso& cab = trans[(int)HuesoAnimal::CABEZA];
    for (size_t i = 0; i < reposo.vertices.size(); ++i) {
        const VerticeAnimal& v = reposo.vertices[i];
        if (v.zona != ZonaCuerpo::PARPADO) continue;
        const int lado = v.pos.x < 0 ? 0 : 1;
        const float cerrado = lado == 0 ? g.parpadoI : g.parpadoD;
        // Abierto del todo: el casquete queda en un 12 % de su alto, pegado
        // arriba. Cerrado: cubre el ojo entero.
        const float alto = 0.12f + 0.88f * cerrado;
        const float top = b->y[lado];
        V3 p = v.pos;
        p.y = top - (top - p.y) * alto;
        salida[i].pos = cab.aplicarPunto(p);
        salida[i].normal = cab.aplicarDireccion(v.normal);
    }
}

} // namespace Fauna

#endif // PECARI_CARA_H
