#ifndef JUMP_SYSTEM_H
#define JUMP_SYSTEM_H

#include "PlayerTypes.h"

// ============================================================================
// JUMP SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: decidir cuando se salta y con que velocidad inicial.
//
// ----------------------------------------------------------------------------
// ALTURA COMO PARAMETRO AUTORITATIVO
// ----------------------------------------------------------------------------
// La velocidad de salto NO es una constante ajustada a ojo: se DERIVA de la
// altura deseada y de la gravedad, con la formula de conservacion de energia
//
//     v0 = sqrt(2 * g * h)
//
// Consecuencia practica: si se cambia la gravedad, el salto sigue midiendo
// exactamente jumpHeight bloques. Con una velocidad fija, cualquier ajuste de
// gravedad romperia silenciosamente la altura y habria que reajustar a mano
// (que es de donde salen los "no llego a subir el bloque" tras un retoque).
//
// Los tiempos de subida y caida tambien se derivan (ver getRiseTime), de modo
// que el brief "permitir modificar facilmente altura / tiempo de subida /
// tiempo de caida" se cumple ajustando jumpHeight y gravity.
//
// ----------------------------------------------------------------------------
// COYOTE TIME Y JUMP BUFFER
// ----------------------------------------------------------------------------
// Dos margenes de tolerancia que son la diferencia entre unos controles que
// se sienten "precisos" y otros que se sienten "injustos":
//
//   COYOTE TIME: permite saltar hasta 100 ms DESPUES de haber dejado el
//     borde. Sin esto, el jugador que pulsa salto justo al salir de la
//     plataforma no salta, y lo percibe como input perdido (aunque
//     tecnicamente ya estaba en el aire).
//
//   JUMP BUFFER: si se pulsa salto hasta 120 ms ANTES de tocar el suelo, el
//     salto se ejecuta al aterrizar. Sin esto, encadenar saltos exige un
//     timing perfecto al frame.
//
// Ambos margenes son en TIEMPO, no en frames, asi que se comportan igual a
// cualquier FPS.
// ============================================================================

namespace PlayerSys {

class JumpSystem {
private:
    const MovementConfig* cfg;

public:
    explicit JumpSystem(const MovementConfig* c) : cfg(c) {}

    // Velocidad inicial para alcanzar exactamente cfg->jumpHeight.
    float getJumpVelocity() const {
        return sqrtf(2.0f * cfg->gravity * cfg->jumpHeight);
    }

    // Tiempo de subida hasta el apice (informativo / depuracion).
    float getRiseTime() const {
        return getJumpVelocity() / cfg->gravity;
    }

    // Tiempo de caida desde el apice (menor por la gravedad asimetrica).
    float getFallTime() const {
        const float g = cfg->gravity * cfg->fallGravityMult;
        return sqrtf(2.0f * cfg->jumpHeight / g);
    }

    // ------------------------------------------------------------------------
    // ACTUALIZAR TEMPORIZADORES
    // ------------------------------------------------------------------------
    // Se llama una vez por paso fijo, antes de tryJump.
    void updateTimers(MovementState& state, const InputCommand& input, float dt) const {
        (void)input; // el buffer de salto se lleva a nivel de frame

        // Tiempo desde que se piso suelo por ultima vez (coyote time).
        if (state.onGround) {
            state.timeSinceGrounded = 0.0f;
            state.jumpConsumed = false;
        } else {
            state.timeSinceGrounded += dt;
        }

        // NOTA: timeSinceJumpPressed NO se actualiza aqui.
        // Se lleva en PhysicsSystem::update, a nivel de FRAME, porque a
        // framerates altos un frame es mas corto que un paso fijo y una
        // pulsacion breve podria caer entre dos pasos y perderse.
    }

    // ------------------------------------------------------------------------
    // INTENTAR SALTAR
    // ------------------------------------------------------------------------
    // Devuelve true si se ejecuto un salto.
    bool tryJump(MovementState& state, const InputCommand& input) const {

        // ---- NADAR HACIA ARRIBA ----
        // En agua, saltar equivale a nadar hacia la superficie de forma
        // continua (no es un impulso puntual).
        if (state.isInWater && !state.isFlying) {
            if (input.jump) {
                state.velocity.y = cfg->waterJumpSpeed;
                return true;
            }
            return false;
        }

        // ---- SUBIR ESCALERA ----
        if (state.isOnLadder && !state.isFlying) {
            if (input.jump) {
                state.velocity.y = cfg->ladderSpeed;
                return true;
            }
            return false;
        }

        if (state.isFlying) return false;

        // ---- SALTO NORMAL ----
        // Condiciones: hay una pulsacion reciente Y hubo suelo reciente,
        // y no se ha consumido ya este salto.
        const bool bufferedPress = state.timeSinceJumpPressed <= cfg->jumpBufferTime;
        const bool coyoteOk      = state.timeSinceGrounded    <= cfg->coyoteTime;

        if (bufferedPress && coyoteOk && !state.jumpConsumed) {
            state.velocity.y = getJumpVelocity();
            state.onGround = false;
            state.jumpConsumed = true;

            // Consumir el buffer para que una sola pulsacion no encadene
            // dos saltos.
            state.timeSinceJumpPressed = 999.0f;
            state.timeSinceGrounded    = 999.0f;

            // ---- IMPULSO DE SPRINT ----
            // Saltar corriendo conserva (y amplifica levemente) el impulso
            // horizontal. Es lo que permite los saltos largos y hace que
            // correr y saltar se sienta fluido en vez de "frenar al despegar".
            if (state.isSprinting) {
                const float speed = state.velocity.lengthXZ();
                if (speed > 0.1f) {
                    const float boost = cfg->sprintJumpBoost;
                    state.velocity.x += (state.velocity.x / speed) * boost;
                    state.velocity.z += (state.velocity.z / speed) * boost;
                }
            }
            return true;
        }

        return false;
    }
};

} // namespace PlayerSys

#endif // JUMP_SYSTEM_H
