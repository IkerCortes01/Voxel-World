#ifndef GRAVITY_SYSTEM_H
#define GRAVITY_SYSTEM_H

#include "PlayerTypes.h"

// ============================================================================
// GRAVITY SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: modificar velocity.y por efecto de la gravedad.
// No mueve al jugador, no resuelve colisiones, no lee input.
//
// ----------------------------------------------------------------------------
// GRAVEDAD ASIMETRICA
// ----------------------------------------------------------------------------
// La gravedad de caida es mayor que la de subida (fallGravityMult = 1.28).
// Esto no es fisica newtoniana, es diseño de juego deliberado:
//
//   - Con gravedad simetrica, la parabola del salto pasa el mismo tiempo
//     subiendo que bajando. El tramo descendente se siente "flotante" y el
//     jugador pierde la sensacion de control al final del salto.
//   - Acelerando la caida, el salto tiene un apice nitido y el aterrizaje
//     llega antes: la respuesta se percibe mas precisa sin reducir la altura
//     alcanzable.
//
// Es la misma tecnica que usan los plataformas desde Super Mario Bros.
// Ajustable a 1.0 si se prefiere una parabola simetrica pura.
// ============================================================================

namespace PlayerSys {

class GravitySystem {
private:
    const MovementConfig* cfg;

public:
    explicit GravitySystem(const MovementConfig* c) : cfg(c) {}

    // Aplica gravedad para un paso de tiempo fijo.
    void apply(MovementState& state, float dt) const {
        // Vuelo: sin gravedad. El control vertical lo lleva MovementSystem.
        if (state.isFlying) return;

        // ---- EN AGUA ----
        // Gravedad reducida + empuje de flotabilidad. El resultado es un
        // hundimiento lento con posibilidad de mantenerse a flote nadando.
        if (state.isInWater) {
            const float net = cfg->waterGravity - cfg->buoyancy;
            state.velocity.y -= net * dt;

            // Amortiguacion vertical: el agua frena tanto la subida como la
            // bajada, evitando oscilaciones de flotabilidad.
            state.velocity.y = MathUtil::damp(state.velocity.y, 0.0f, 2.2f, dt);

            // Limite de hundimiento
            const float maxSink = -6.0f;
            if (state.velocity.y < maxSink) state.velocity.y = maxSink;
            return;
        }

        // ---- EN ESCALERA ----
        // La escalera anula la gravedad: el control vertical es directo.
        if (state.isOnLadder) return;

        // ---- EN SUELO ----
        // No acumular velocidad negativa mientras se esta apoyado: si no, tras
        // unos segundos parado la velocidad seria enorme y el primer paso a un
        // hueco produciria una caida instantanea y brusca.
        if (state.onGround && state.velocity.y <= 0.0f) {
            state.velocity.y = 0.0f;
            return;
        }

        // ---- EN EL AIRE ----
        const float g = (state.velocity.y < 0.0f)
                      ? cfg->gravity * cfg->fallGravityMult  // cayendo
                      : cfg->gravity;                        // subiendo

        state.velocity.y -= g * dt;

        // Velocidad terminal
        if (state.velocity.y < -cfg->maxFallSpeed) {
            state.velocity.y = -cfg->maxFallSpeed;
        }
    }
};

} // namespace PlayerSys

#endif // GRAVITY_SYSTEM_H
