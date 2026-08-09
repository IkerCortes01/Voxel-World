#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include "PlayerTypes.h"

// ============================================================================
// MOVEMENT SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: convertir la intencion de movimiento (InputCommand)
// en velocidad horizontal. No aplica gravedad, no resuelve colisiones, no
// mueve la posicion.
//
// ----------------------------------------------------------------------------
// MODELO DE ACELERACION EXPONENCIAL
// ----------------------------------------------------------------------------
// La velocidad tiende exponencialmente a la velocidad objetivo:
//
//     v(t+dt) = objetivo + (v - objetivo) * exp(-tasa * dt)
//
// Tres razones para elegir esto:
//
//   1. INDEPENDENCIA DEL FPS EXACTA. Es la solucion analitica de la ecuacion
//      diferencial dv/dt = k(objetivo - v), asi que aplicar un paso de 2*dt
//      da EXACTAMENTE el mismo resultado que dos pasos de dt. El modelo
//      ingenuo `v += (obj-v)*k*dt` no cumple esto y hace que el personaje
//      acelere distinto a 30 y a 240 FPS.
//
//   2. RESPUESTA INMEDIATA CON INERCIA CORTA. Con groundAccel=45 se alcanza
//      el 95% de la velocidad objetivo en ~66 ms: se siente instantaneo al
//      pulsar, pero conserva el peso justo para no parecer un cursor.
//
//   3. SIN OVERSHOOT. Nunca supera la velocidad objetivo, asi que no hace
//      falta clamp posterior (que es lo que produce el "tope" abrupto y poco
//      natural de muchos controladores).
//
// La version anterior del motor asignaba la velocidad directamente
// (v = dir * WALK_SPEED), lo que da respuesta instantanea pero tambien
// PARADA instantanea: el personaje se comporta como si tuviera masa cero, sin
// ninguna sensacion de inercia.
// ============================================================================

namespace PlayerSys {

class MovementSystem {
private:
    const MovementConfig* cfg;

    // Velocidad objetivo segun el estado actual.
    float getTargetSpeed(const MovementState& s) const {
        if (s.isFlying)   return s.isSprinting ? cfg->flySprintSpeed : cfg->flySpeed;
        if (s.isInWater)  return cfg->swimSpeed;
        if (s.isCrouching && s.onGround) return cfg->crouchSpeed;
        if (s.isSprinting && s.onGround) return cfg->sprintSpeed;
        return cfg->walkSpeed;
    }

    // Tasa de aceleracion segun el medio.
    float getAccelRate(const MovementState& s) const {
        if (s.isFlying)  return cfg->groundAccel;
        if (s.isInWater) return cfg->waterAccel;
        return s.onGround ? cfg->groundAccel : cfg->airAccel;
    }

    // Tasa de frenado cuando no hay input.
    float getFrictionRate(const MovementState& s) const {
        if (s.isFlying)  return cfg->groundFriction;
        if (s.isInWater) return cfg->waterFriction;
        return s.onGround ? cfg->groundFriction : cfg->airFriction;
    }

public:
    explicit MovementSystem(const MovementConfig* c) : cfg(c) {}

    // ========================================================================
    // VECTORES DE ORIENTACION
    // ========================================================================
    // CONVENCION: la impuesta por la MATRIZ DE CAMARA DEL MOTOR, que es
    //     glRotatef(-pitch, 1,0,0); glRotatef(-yaw, 0,1,0);
    //
    // Se verifico numericamente la direccion de vista resultante en mundo:
    //     yaw=  0 -> ( 0, 0,-1)      yaw= 90 -> (-1, 0, 0)
    //     yaw=180 -> ( 0, 0, 1)      yaw=270 -> ( 1, 0, 0)
    // es decir:
    //     forward = (-sin(yaw), 0, -cos(yaw))
    //     right   = (-cos(yaw), 0,  sin(yaw))
    //
    // NOTA IMPORTANTE (error corregido):
    // Una version previa de este archivo usaba +sin(yaw) para forward,
    // "unificando" las convenciones pero eligiendo la CONTRARIA a la que usa
    // el renderer y el raycast. El resultado era que a yaw=90 el jugador se
    // movia hacia +X mientras la camara miraba a -X: caminaba de espaldas a
    // lo que veia. Solo coincidia en yaw=0 y 180, por lo que el fallo no se
    // aprecia en la prueba trivial "arranco y pulso W".
    //
    // Perpendicularidad: dot = (-sin)(-cos) + (-cos)(sin) = 0  ✓
    // right es forward rotado -90 grados sobre Y, de modo que D mueve hacia
    // la derecha real de la vista.
    static MoveVec3 getForwardXZ(float yawDegrees) {
        const float r = yawDegrees * MathUtil::DEG2RAD;
        return MoveVec3(-sinf(r), 0.0f, -cosf(r));
    }

    // NOTA SOBRE EL SIGNO: este vector va INVERTIDO respecto al "right"
    // matematico de la convencion de camara (que seria (-cos, 0, sin)).
    //
    // Motivo: con el signo matematico, A y D movian en direcciones opuestas
    // a las esperadas en la practica. Se invierte aqui, en el UNICO punto que
    // define la lateralidad, en lugar de negar input.moveRight u otros
    // parches repartidos: asi D siempre significa "derecha" para todo el
    // sistema (movimiento, sprint, deteccion de pared) y hay una sola fuente
    // de verdad que cambiar si hiciera falta revisarlo.
    static MoveVec3 getRightXZ(float yawDegrees) {
        const float r = yawDegrees * MathUtil::DEG2RAD;
        return MoveVec3(cosf(r), 0.0f, -sinf(r));
    }

    // Vector de vista completo (con pitch). Coincide con Player::getForward()
    // del motor, que es el que alimenta el raycast.
    static MoveVec3 getLookDirection(float yawDegrees, float pitchDegrees) {
        const float y = yawDegrees   * MathUtil::DEG2RAD;
        const float p = pitchDegrees * MathUtil::DEG2RAD;
        const float cp = cosf(p);
        return MoveVec3(-sinf(y) * cp, sinf(p), -cosf(y) * cp);
    }

    // ========================================================================
    // APLICAR MOVIMIENTO HORIZONTAL
    // ========================================================================
    void apply(MovementState& state, const InputCommand& input, float dt) const {

        // ---- DIRECCION DESEADA ----
        const MoveVec3 fwd   = getForwardXZ(state.yaw);
        const MoveVec3 right = getRightXZ(state.yaw);

        MoveVec3 wish(
            fwd.x * input.moveForward + right.x * input.moveRight,
            0.0f,
            fwd.z * input.moveForward + right.z * input.moveRight
        );

        // ---- NORMALIZACION DIAGONAL ----
        // Sin esto, W+D daria velocidad sqrt(2) ~ 1.41x mayor que solo W
        // (el clasico "strafe running"). Se normaliza solo si el modulo
        // supera 1, para respetar entradas analogicas parciales de un gamepad.
        const float wishLen = wish.lengthXZ();
        if (wishLen > 1.0f) {
            wish.x /= wishLen;
            wish.z /= wishLen;
        }

        const bool hasInput = (wishLen > 1e-4f);
        const float targetSpeed = getTargetSpeed(state);

        state.lastMoveDirection = wish;

        // ---- ACELERAR O FRENAR ----
        if (hasInput) {
            const float rate = getAccelRate(state);
            const float tx = wish.x * targetSpeed;
            const float tz = wish.z * targetSpeed;

            // ---- CONTROL AEREO ----
            // En el aire la aceleracion es menor (airAccel=9 frente a 45 en
            // suelo): se puede corregir la trayectoria pero no cambiarla por
            // completo, que es lo que pide el brief ("controlar parcialmente
            // la trayectoria, sin detener el movimiento").
            state.velocity.x = MathUtil::damp(state.velocity.x, tx, rate, dt);
            state.velocity.z = MathUtil::damp(state.velocity.z, tz, rate, dt);

        } else {
            // Sin input: frenar hacia cero.
            // En el aire airFriction es muy baja (1.2), asi que el impulso se
            // conserva casi intacto: soltar las teclas a media parabola no
            // detiene al jugador en seco.
            const float rate = getFrictionRate(state);
            state.velocity.x = MathUtil::damp(state.velocity.x, 0.0f, rate, dt);
            state.velocity.z = MathUtil::damp(state.velocity.z, 0.0f, rate, dt);

            // Umbral de reposo: evita velocidades residuales infinitesimales
            // que mantendrian al jugador "moviendose" para siempre y
            // dispararian recalculos de colision cada frame.
            if (fabsf(state.velocity.x) < 0.001f) state.velocity.x = 0.0f;
            if (fabsf(state.velocity.z) < 0.001f) state.velocity.z = 0.0f;
        }

        // ====================================================================
        // MOVIMIENTO VERTICAL POR MEDIO
        // ====================================================================

        // ---- VUELO ----
        if (state.isFlying) {
            float vy = 0.0f;
            if (input.flyUp)   vy += 1.0f;
            if (input.flyDown) vy -= 1.0f;
            const float target = vy * getTargetSpeed(state);
            state.velocity.y = MathUtil::damp(state.velocity.y, target,
                                              cfg->groundAccel, dt);
            return;
        }

        // ---- ESCALERA ----
        // Velocidad vertical CONSTANTE (el brief lo pide explicitamente):
        // se asigna directamente en vez de acelerar, para que subir sea
        // uniforme y predecible.
        if (state.isOnLadder) {
            if (input.moveForward > 0.1f || input.flyUp) {
                state.velocity.y = cfg->ladderSpeed;
            } else if (input.crouch) {
                state.velocity.y = -cfg->ladderSpeed;
            } else {
                // Agarrado sin input: se queda quieto (no resbala).
                state.velocity.y = MathUtil::damp(state.velocity.y, 0.0f, 14.0f, dt);
            }
            return;
        }

        // ---- AGUA ----
        // Nado tridimensional: hundirse con agacharse.
        if (state.isInWater) {
            if (input.crouch) {
                state.velocity.y = MathUtil::damp(state.velocity.y,
                                                  -cfg->swimSpeed, cfg->waterAccel, dt);
            }
        }
    }
};

} // namespace PlayerSys

#endif // MOVEMENT_SYSTEM_H
