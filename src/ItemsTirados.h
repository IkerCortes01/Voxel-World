#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================================
// ⭐ LOS ITEMS DEJADOS EN EL SUELO: CUERPOS CON FISICA
// ============================================================================
// PEDIDO (1): "que solo de el item tirado en 3D [...] y que si colocas mas y
// mas en el mismo lugar se acumulan y tienen fisica y gravedad".
// PEDIDO (2): "que el maguey y nopal de Castilla sueltos tengan colision,
// reaccionen a paredes, techo, piso y bloques y al jugador y entidades, y que
// sea mas realista".
//
// El nopal y el maguey no se convierten en un bloque al colocarlos: se deja el
// propio item, acostado. Cada uno es una CAJA baja (0,6 x 0,28 x 0,6) con
// posicion y velocidad en los tres ejes:
//
//   SUELO     cae con gravedad y se posa sobre lo que tenga debajo, a la
//             altura de verdad del bloque (uno entero o una capa de 3 px).
//             Si cae fuerte, rebota un poco. Si le quitan el apoyo, cae.
//   PAREDES   no atraviesa bloques: al llegar a uno se para y rebota algo.
//   TECHO     si sube y hay un bloque encima, se queda debajo.
//   ROCE      en el suelo se frena enseguida; en el aire, casi nada.
//   MONTON    el que cae encima de otro descansa sobre el. Dos al MISMO nivel
//             que se pisan se apartan uno de otro.
//   JUGADOR   al caminar contra uno lo empuja por delante (no lo atraviesa);
//   Y ANIMALES  los animales, igual.
//
// A diferencia de lo que suelta un bloque al romperse, estos no van hacia el
// jugador ni se recogen al pasar: se recogen mirandolos y pulsando F.
//
// Aqui esta solo la fisica, sin motor: el mundo lo da quien llama. La prueba
// tests/test_items_tirados.cpp.
// ============================================================================

namespace ItemsTirados {

constexpr float GROSOR = 0.28f;       // el alto de la caja (0,9 de lado x 5/16)
constexpr float MEDIO = 0.30f;        // medio lado de la caja, a lo ancho
constexpr float RADIO = 0.45f;        // dos mas juntos que esto, a lo ancho, se pisan
constexpr float GRAVEDAD = 20.0f;
constexpr float CAIDA_MAX = 30.0f;
constexpr float REBOTE_SUELO = 0.28f; // lo que conserva al caer fuerte
constexpr float REBOTE_PARED = 0.30f;
constexpr float ROCE_SUELO = 9.0f;    // por segundo
constexpr float ROCE_AIRE = 0.4f;
constexpr float EMPUJE = 3.2f;        // rapidez con que lo aparta quien lo pisa
constexpr float ALCANCE_F = 4.5f;     // desde donde se recogen con F
constexpr float PUNTERIA_F = 0.45f;   // lo lejos del centro que puede pasar la mirada

struct Cuerpo {
    float x = 0, y = 0, z = 0;        // `y` es la BASE: donde toca lo que lo sostiene
    float vy = 0;
    bool enSuelo = false;
    float vx = 0, vz = 0;
};

// Quien empuja: el jugador o un animal. Un cilindro de pie.
struct Empujador {
    float x = 0, z = 0;
    float yPies = 0, alto = 1.8f;
    float radio = 0.35f;
};

// ¿Esta `abajo` debajo de `arriba` y lo bastante cerca a lo ancho para sostenerlo?
inline bool sostiene(const Cuerpo& abajo, const Cuerpo& arriba) {
    if (abajo.y + GROSOR > arriba.y + 0.5f * GROSOR) return false;      // no esta debajo
    const float dx = abajo.x - arriba.x, dz = abajo.z - arriba.z;
    return dx * dx + dz * dz < RADIO * RADIO;
}

// Un paso de fisica para todos.
//
// `celda(ix, iy, iz)` describe el mundo: 0 si la celda esta vacia, de 0 a 1 lo
// que mide de alto lo macizo que tenga (1 = bloque entero), y un numero
// NEGATIVO si ahi no hay mundo cargado: el cuerpo se queda quieto donde esta.
template <typename TCelda>
inline void paso(std::vector<Cuerpo>& c, float dt, TCelda celda,
                 const Empujador* empujan = nullptr, int nEmpujan = 0) {
    if (!(dt > 0.0f)) return;
    if (dt > 0.05f) dt = 0.05f;

    auto piso = [](float v) { return (int)std::floor(v); };
    // ¿Choca la caja puesta en (x, y, z) con algo macizo? Se le quita un pelo
    // por abajo para que el suelo que pisa no cuente como pared.
    auto choca = [&](float x, float y, float z) {
        const float y0 = y + 0.02f, y1 = y + GROSOR - 0.01f;
        for (int ix = piso(x - MEDIO); ix <= piso(x + MEDIO); ++ix)
            for (int iz = piso(z - MEDIO); iz <= piso(z + MEDIO); ++iz)
                for (int iy = piso(y0); iy <= piso(y1); ++iy) {
                    const float h = celda(ix, iy, iz);
                    if (h > 0.0f && (float)iy + h > y0) return true;
                }
        return false;
    };
    // Lo mas alto del terreno bajo la caja, a su altura o por debajo.
    auto suelo = [&](float x, float y, float z) {
        float mejor = y - 100.0f;
        for (int ix = piso(x - MEDIO * 0.8f); ix <= piso(x + MEDIO * 0.8f); ++ix)
            for (int iz = piso(z - MEDIO * 0.8f); iz <= piso(z + MEDIO * 0.8f); ++iz) {
                int iy = piso(y + 0.25f);
                for (int n = 0; n < 12; ++n, --iy) {
                    const float h = celda(ix, iy, iz);
                    if (!(h > 0.0f)) continue;
                    const float tope = (float)iy + (h > 1.0f ? 1.0f : h);
                    if (tope <= y + 0.25f) { mejor = (std::max)(mejor, tope); break; }
                }
            }
        return mejor;
    };

    // De abajo arriba: cada uno se apoya en los que ya se han colocado.
    std::vector<size_t> orden(c.size());
    for (size_t i = 0; i < orden.size(); ++i) orden[i] = i;
    std::sort(orden.begin(), orden.end(), [&](size_t a, size_t b) {
        return c[a].y != c[b].y ? c[a].y < c[b].y : a < b;
    });

    for (size_t k = 0; k < orden.size(); ++k) {
        Cuerpo& q = c[orden[k]];
        // Sin mundo cargado no hay fisica.
        if (celda(piso(q.x), piso(q.y), piso(q.z)) < 0.0f) { q.vx = q.vy = q.vz = 0.0f; continue; }

        // --- Quien lo empuja: el jugador y los animales ---
        for (int e = 0; e < nEmpujan; ++e) {
            const Empujador& p = empujan[e];
            if (q.y + GROSOR < p.yPies || q.y > p.yPies + p.alto) continue;
            const float dx = q.x - p.x, dz = q.z - p.z;
            const float lejos = p.radio + MEDIO;
            const float d2 = dx * dx + dz * dz;
            if (d2 >= lejos * lejos) continue;
            const float d = std::sqrt(d2);
            const float nx = d > 1e-4f ? dx / d : 1.0f, nz = d > 1e-4f ? dz / d : 0.0f;
            const float hondo = (lejos - d) / lejos;            // 0..1: cuanto lo pisa
            q.vx += nx * EMPUJE * (0.4f + hondo) * dt * 12.0f;
            q.vz += nz * EMPUJE * (0.4f + hondo) * dt * 12.0f;
        }
        // --- Los que se pisan al MISMO nivel se apartan ---
        for (size_t j = 0; j < c.size(); ++j) {
            if (j == orden[k]) continue;
            const Cuerpo& o = c[j];
            if (std::fabs(o.y - q.y) >= GROSOR * 0.85f) continue;      // uno encima del otro: monton
            const float dx = q.x - o.x, dz = q.z - o.z;
            const float d2 = dx * dx + dz * dz;
            const float lejos = 2.0f * MEDIO * 0.85f;
            if (d2 >= lejos * lejos) continue;
            const float d = std::sqrt(d2);
            // Exactamente en el mismo punto: cada uno hacia un lado fijo.
            const float nx = d > 1e-4f ? dx / d : (orden[k] < j ? 1.0f : -1.0f);
            const float nz = d > 1e-4f ? dz / d : 0.0f;
            const float f = (lejos - d) / lejos * 2.2f;
            q.vx += nx * f * dt * 12.0f;
            q.vz += nz * f * dt * 12.0f;
        }

        // --- A lo ancho, un eje cada vez: las paredes ---
        const float tope = 8.0f;
        q.vx = (std::max)(-tope, (std::min)(tope, q.vx));
        q.vz = (std::max)(-tope, (std::min)(tope, q.vz));
        if (q.vx != 0.0f) {
            const float nx = q.x + q.vx * dt;
            if (choca(nx, q.y, q.z)) q.vx = -q.vx * REBOTE_PARED;
            else q.x = nx;
        }
        if (q.vz != 0.0f) {
            const float nz = q.z + q.vz * dt;
            if (choca(q.x, q.y, nz)) q.vz = -q.vz * REBOTE_PARED;
            else q.z = nz;
        }

        // --- En vertical: el suelo, el monton y el techo ---
        float apoyo = suelo(q.x, q.y, q.z);
        for (size_t j = 0; j < k; ++j) {
            const Cuerpo& o = c[orden[j]];
            if (sostiene(o, q)) apoyo = (std::max)(apoyo, o.y + GROSOR);
        }
        if (q.y > apoyo + 1e-4f || q.vy > 0.0f) {
            q.enSuelo = false;
            q.vy = (std::max)(-CAIDA_MAX, q.vy - GRAVEDAD * dt);
            float ny = q.y + q.vy * dt;
            if (q.vy > 0.0f && choca(q.x, ny, q.z)) { q.vy = 0.0f; ny = q.y; }     // el techo
            q.y = ny;
            if (q.y <= apoyo) {
                q.y = apoyo;
                // Cae fuerte: rebota un poco. Flojo: se queda.
                if (q.vy < -4.0f) q.vy = -q.vy * REBOTE_SUELO;
                else { q.vy = 0.0f; q.enSuelo = true; }
            }
        } else {
            q.y = apoyo;              // lo que tenia debajo subio (o ya estaba)
            q.vy = 0.0f;
            q.enSuelo = true;
        }

        // --- El roce ---
        const float roce = std::exp(-(q.enSuelo ? ROCE_SUELO : ROCE_AIRE) * dt);
        q.vx *= roce;
        q.vz *= roce;
        if (q.enSuelo && q.vx * q.vx + q.vz * q.vz < 0.0004f) q.vx = q.vz = 0.0f;
    }
}

// El item que se mira: el mas cercano a lo largo de la mirada cuyo centro
// queda a menos de PUNTERIA_F de ella. `o` el ojo y `d` la direccion
// (unitaria). Devuelve su indice, o -1.
inline int mirado(const std::vector<Cuerpo>& c, float ox, float oy, float oz,
                  float dx, float dy, float dz, float alcance = ALCANCE_F) {
    int mejor = -1;
    float mejorT = alcance;
    for (size_t i = 0; i < c.size(); ++i) {
        const float px = c[i].x - ox, py = c[i].y + 0.5f * GROSOR - oy, pz = c[i].z - oz;
        const float t = px * dx + py * dy + pz * dz;
        if (t < 0.0f || t > mejorT) continue;
        const float qx = px - dx * t, qy = py - dy * t, qz = pz - dz * t;
        if (qx * qx + qy * qy + qz * qz > PUNTERIA_F * PUNTERIA_F) continue;
        mejor = (int)i;
        mejorT = t;
    }
    return mejor;
}

}  // namespace ItemsTirados
