#ifndef PECARI_MUSCULOS_H
#define PECARI_MUSCULOS_H

#include <cmath>
#include <vector>
#include "AnimalSkinning.h"

// ============================================================================
// MUSCULOS Y MOVIMIENTO SECUNDARIO (fase 4)
// ============================================================================
// PEDIDO: "musculos moviles". Tres efectos, todos como una pasada sobre la
// malla YA deformada, a lo largo de la normal de cada vertice:
//
//   RESPIRAR   el torso se hincha y se deshincha. Mas rapido al correr: jadea.
//   PANZA      el vientre tiene inercia: un resorte que se queda atras al
//              arrancar y rebota al frenar, mas el bote de cada paso.
//   MUSCULOS   hombro y muslo abultan cuando la pata se dobla, y el antebrazo
//              y la pierna cuando se dobla la articulacion de en medio.
//
// Coste: una pasada sobre los vertices, solo de cerca (LOD 0 y 1).
// ============================================================================

namespace Fauna {

// El resorte de la panza. Vive en cada animal y se actualiza con su reloj.
struct ResortePanza {
    float x = 0.0f;           // desplazamiento, -1..1
    float v = 0.0f;
    float rapidezPrev = 0.0f;

    void actualizar(float dt, float rapidez) {
        if (dt <= 0.0f) return;
        const float acel = (rapidez - rapidezPrev) / dt;
        rapidezPrev = rapidez;
        constexpr float K = 60.0f, C = 7.0f, EMPUJE = 0.06f;
        // Euler semi-implicito: estable aunque la fauna vaya a 20 Hz.
        v += (-K * x - C * v - acel * EMPUJE) * dt;
        x += v * dt;
        if (x > 1.0f) { x = 1.0f; v = 0.0f; }
        if (x < -1.0f) { x = -1.0f; v = 0.0f; }
    }
};

struct DinamicaCuerpo {
    float panza = 0.0f;       // el resorte, -1..1
    float fasePaso = 0.0f;
    float rapidez = 0.0f;     // bloques/s
    float tiempo = 0.0f;      // reloj del animal, para respirar
    float faseRespira = 0.0f; // cada uno a su ritmo
    bool  respira = true;     // los muertos no
};

inline bool esTroncoBlando(ZonaCuerpo z) {
    return z == ZonaCuerpo::TORSO || z == ZonaCuerpo::LOMO ||
           z == ZonaCuerpo::VIENTRE || z == ZonaCuerpo::GRUPA ||
           z == ZonaCuerpo::COLLAR;
}

inline void AnimarMusculos(const MallaAnimal& reposo,
                           const std::vector<PesoVertice>& pesos,
                           const PoseEsqueleto& pose,
                           const TransHueso trans[NUM_HUESOS],
                           const DinamicaCuerpo& d,
                           std::vector<VerticeAnimal>& salida) {
    const size_t n = reposo.vertices.size();
    if (salida.size() != n || pesos.size() != n) return;

    // Respiracion: ~0.45 Hz en reposo, hasta 1.6 Hz corriendo.
    float ritmo = d.rapidez / 3.0f;
    if (ritmo > 1.0f) ritmo = 1.0f;
    const float frec = 0.45f + 1.15f * ritmo;
    const float aire = d.respira
        ? std::sin(d.tiempo * 6.2831853f * frec + d.faseRespira) * (0.0045f + 0.003f * ritmo)
        : 0.0f;
    // El bote del paso: dos por ciclo (cada par de patas apoya una vez).
    float bote = std::sin(d.fasePaso * 2.0f) * 0.004f * (ritmo > 0 ? ritmo : 0);
    const float panza = d.panza * 0.018f + bote;

    // La altura del vientre: lo mas bajo del tronco se mueve mas.
    const float yEje = trans[(int)HuesoAnimal::TRONCO].pivote.y;

    for (size_t i = 0; i < n; ++i) {
        const VerticeAnimal& v = reposo.vertices[i];
        VerticeAnimal& o = salida[i];
        const int h = pesos[i].hueso;
        float desp = 0.0f;   // a lo largo de la normal
        float baja = 0.0f;   // en vertical

        if (esTroncoBlando(v.zona)) {
            desp += aire * (v.zona == ZonaCuerpo::VIENTRE ? 1.4f : 1.0f);
            if (v.zona == ZonaCuerpo::VIENTRE || v.pos.y < yEje) {
                float k = (yEje - v.pos.y) / 0.20f;
                k = k < 0 ? 0 : (k > 1 ? 1 : k);
                baja += panza * k;
            }
        }

        // Los musculos de las patas: abultan cerca de la articulacion que se
        // dobla, y mas cuanto mas se dobla.
        if (h >= (int)HuesoAnimal::HOMBRO_DI && h <= (int)HuesoAnimal::CORVEJON_TD) {
            const float giro = std::fabs(pose.giroX[h]);
            const V3& piv = trans[h].pivote;
            const float dx = v.pos.x - piv.x, dy = v.pos.y - piv.y, dz = v.pos.z - piv.z;
            const float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            float cerca = 1.0f - dist / 0.14f;
            if (cerca > 0.0f) desp += 0.010f * (giro > 0.8f ? 0.8f : giro) * cerca;
        }

        if (desp != 0.0f) {
            o.pos.x += o.normal.x * desp;
            o.pos.y += o.normal.y * desp;
            o.pos.z += o.normal.z * desp;
        }
        o.pos.y -= baja;
    }
}

} // namespace Fauna

#endif // PECARI_MUSCULOS_H
