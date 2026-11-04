#ifndef PECARI_PELO_H
#define PECARI_PELO_H

#include <cmath>
#include <cstdint>
#include <vector>
#include "AnimalMalla.h"

// ============================================================================
// EL PELO QUE REACCIONA (fase 5)
// ============================================================================
// PEDIDO: "pelo que reacciona al entorno". Cerdas: lineas que salen de la piel
// ya deformada. Su direccion mezcla:
//
//   ACOSTADA    hacia atras y un poco abajo: el pelaje en reposo.
//   ERIZADA     con la alarma se levanta hacia la normal, sobre todo la crin.
//   VIENTO      se mecen, cada una con su fase.
//   INERCIA     al correr se tumban hacia atras; la panza las sacude.
//
// Las cerdas se eligen una vez por malla (un vertice de cada dos de las zonas
// con pelo) y se dibujan como lineas: unas 250 por animal, solo de cerca.
// ============================================================================

namespace Fauna {

struct Cerda {
    uint16_t vert;
    float largo;       // metros
    float fase;        // para el viento
    bool  crin;        // la cresta se eriza entera
};

inline float largoCerda(ZonaCuerpo z) {
    switch (z) {
        case ZonaCuerpo::CRIN:   return 0.075f;
        case ZonaCuerpo::COLA:   return 0.045f;
        case ZonaCuerpo::LOMO:
        case ZonaCuerpo::GRUPA:  return 0.045f;
        case ZonaCuerpo::COLLAR:
        case ZonaCuerpo::CUELLO: return 0.038f;
        case ZonaCuerpo::TORSO:  return 0.030f;
        default:                 return 0.0f;     // sin cerdas
    }
}

// Las cerdas de una malla, elegidas una sola vez.
inline const std::vector<Cerda>& CerdasDe(const MallaAnimal& m) {
    struct Entrada { const MallaAnimal* m; std::vector<Cerda> c; };
    static Entrada cache[8];
    static int siguiente = 0;
    for (const Entrada& e : cache) if (e.m == &m) return e.c;

    Entrada& e = cache[siguiente];
    siguiente = (siguiente + 1) % 8;
    e.m = &m;
    e.c.clear();
    constexpr size_t MAX_CERDAS = 260;
    int salto = 0;
    for (size_t i = 0; i < m.vertices.size() && e.c.size() < MAX_CERDAS; ++i) {
        const VerticeAnimal& v = m.vertices[i];
        const float largo = largoCerda(v.zona);
        if (largo <= 0.0f || v.pelo < 0.4f) continue;
        if ((salto++ & 1) && v.zona != ZonaCuerpo::CRIN) continue;
        uint32_t h = (uint32_t)i * 2654435761u;
        h ^= h >> 13;
        e.c.push_back({ (uint16_t)i, largo * (0.8f + 0.4f * ((h & 255) / 255.0f)),
                        (h >> 8 & 1023) / 1023.0f * 6.2831853f,
                        v.zona == ZonaCuerpo::CRIN });
    }
    return e.c;
}

struct EntornoPelo {
    float erizado = 0.0f;     // 0..1, la alarma
    float rapidez = 0.0f;     // bloques/s
    float panza = 0.0f;       // el resorte de la fase 4
    float vientoX = 0.0f;     // en el espacio local del animal, -1..1
    float vientoZ = 0.0f;
    float tiempo = 0.0f;
};

// La punta de una cerda, en el espacio local (metros).
inline V3 PuntaCerda(const Cerda& c, const VerticeAnimal& base, const EntornoPelo& e) {
    const V3& n = base.normal;
    // Erizado: la crin entera; el resto, la mitad.
    const float er = c.crin ? e.erizado : e.erizado * 0.5f;
    float corre = e.rapidez * 0.12f;
    if (corre > 0.45f) corre = 0.45f;
    const float mece = std::sin(e.tiempo * 2.6f + c.fase) * 0.22f;
    V3 d(n.x * (0.35f + 0.65f * er) + (e.vientoX * (0.35f + mece)),
         n.y * (0.35f + 0.65f * er) - 0.15f * (1.0f - er) - e.panza * 0.4f,
         n.z * (0.35f + 0.65f * er) - (0.9f + corre) * (1.0f - er) + e.vientoZ * (0.35f + mece));
    d = d.normalizado();
    return V3(base.pos.x + d.x * c.largo, base.pos.y + d.y * c.largo, base.pos.z + d.z * c.largo);
}

} // namespace Fauna

#endif // PECARI_PELO_H
