#ifndef PHYSICS_SYSTEM_H
#define PHYSICS_SYSTEM_H

#include "PlayerTypes.h"
#include "CollisionSystem.h"
#include "GravitySystem.h"
#include "MovementSystem.h"
#include "JumpSystem.h"

// ============================================================================
// PHYSICS SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: orquestar los subsistemas en el orden correcto y
// avanzar la simulacion con PASOS DE TIEMPO FIJOS.
//
// ----------------------------------------------------------------------------
// FIXED TIMESTEP: la pieza que garantiza independencia del FPS
// ----------------------------------------------------------------------------
// PROBLEMA con integrar usando el dt variable del frame:
//   Aunque cada formula sea "correcta", la integracion de la gravedad es
//   cuadratica en el tiempo. Con Euler explicito, la altura alcanzada por un
//   salto depende del tamaño del paso:
//
//     dt = 1/30  -> altura ~1.29 bloques
//     dt = 1/240 -> altura ~1.25 bloques
//
//   Es decir: en un PC lento saltarias MAS ALTO. El brief pide explicitamente
//   "no perder altura en equipos lentos"; con dt variable el problema existe
//   en ambos sentidos y ademas hace la fisica no reproducible.
//
// SOLUCION: acumulador de tiempo. La fisica SIEMPRE avanza en pasos de
// exactamente fixedTimeStep (1/120 s). Si un frame dura 33 ms, se ejecutan 4
// pasos; si dura 4 ms, a veces 0 pasos. El sobrante se guarda para el
// siguiente frame y se usa como alpha de interpolacion del render.
//
// Resultado: la trayectoria es IDENTICA bit a bit a 30, 60, 144 o 500 FPS.
//
// ----------------------------------------------------------------------------
// ESPIRAL DE LA MUERTE
// ----------------------------------------------------------------------------
// Si simular un paso costara mas que el propio paso, el acumulador creceria
// sin fin y el juego se congelaria. maxSubSteps corta el bucle: por encima de
// ese numero se descarta el tiempo sobrante (el juego va "a camara lenta" un
// instante en vez de bloquearse).
// ============================================================================

namespace PlayerSys {

class PhysicsSystem {
private:
    const MovementConfig* cfg;
    CollisionSystem  collision;
    GravitySystem    gravity;
    MovementSystem   movement;
    JumpSystem       jump;

    float accumulator = 0.0f;

    // Estadisticas de depuracion
    int   lastStepCount = 0;
    int   lastQueryCount = 0;
    float lastStepUpAmount = 0.0f;

public:
    PhysicsSystem(const IWorldQuery* world, const MovementConfig* config)
        : cfg(config),
          collision(world, config),
          gravity(config),
          movement(config),
          jump(config) {}

    const CollisionSystem& getCollision() const { return collision; }
    const JumpSystem&      getJump()      const { return jump; }

    int   getLastStepCount()   const { return lastStepCount; }
    int   getLastQueryCount()  const { return lastQueryCount; }
    float getLastStepUp()      const { return lastStepUpAmount; }

    // Fraccion sobrante para interpolar el render entre pasos fisicos.
    float getInterpolationAlpha() const {
        return MathUtil::clamp(accumulator / cfg->fixedTimeStep, 0.0f, 1.0f);
    }

    // ========================================================================
    // ACTUALIZAR (llamar una vez por frame con el dt real)
    // ========================================================================
    // Devuelve el numero de pasos de fisica ejecutados.
    int update(MovementState& state, const InputCommand& input, float frameDt) {
        // Sanidad del dt: un valor corrupto (pausa, breakpoint, ventana
        // arrastrada) no debe teletransportar al jugador.
        if (!std::isfinite(frameDt) || frameDt < 0.0f) frameDt = 0.0f;
        if (frameDt > 0.25f) frameDt = 0.25f;

        // ====================================================================
        // REGISTRO DEL SALTO A NIVEL DE FRAME (no de paso fijo)
        // ====================================================================
        // BUG QUE ESTO CORRIGE: a framerates altos, un frame (2 ms a 500 FPS)
        // es MAS CORTO que un paso de fisica (8.3 ms a 120 Hz). Una pulsacion
        // breve de salto puede caer enteramente entre dos pasos fijos, de modo
        // que ningun fixedUpdate llega a ver input.jump == true y el salto se
        // pierde. Se medio: a 500 FPS el jugador no saltaba en absoluto.
        //
        // La solucion es registrar la pulsacion aqui, donde se ve TODO frame,
        // y dejar que el paso fijo consuma el buffer. Asi ninguna pulsacion se
        // pierde por muy alto que sea el framerate.
        if (input.jump) {
            state.timeSinceJumpPressed = 0.0f;
        } else {
            state.timeSinceJumpPressed += frameDt;
        }

        accumulator += frameDt;

        int steps = 0;
        while (accumulator >= cfg->fixedTimeStep && steps < cfg->maxSubSteps) {
            fixedUpdate(state, input, cfg->fixedTimeStep);
            accumulator -= cfg->fixedTimeStep;
            ++steps;
        }

        // Anti espiral: descartar el exceso si no pudimos alcanzarlo.
        if (steps >= cfg->maxSubSteps && accumulator > cfg->fixedTimeStep) {
            accumulator = 0.0f;
        }

        lastStepCount = steps;
        return steps;
    }

    // ========================================================================
    // UN PASO DE FISICA (dt SIEMPRE constante)
    // ========================================================================
    // ORDEN DE OPERACIONES. No es arbitrario:
    //
    //   1. Guardar posicion previa      -> para interpolar el render
    //   2. Detectar medio (agua/escalera)-> condiciona todo lo demas
    //   3. Resolver agachado            -> cambia la altura de la caja
    //   4. Timers de salto              -> antes de decidir el salto
    //   5. Movimiento horizontal        -> fija velocidad XZ objetivo
    //   6. Salto                        -> puede sobrescribir velocity.y
    //   7. Gravedad                     -> modifica velocity.y
    //   8. Integrar + colisiones        -> unico punto que mueve la posicion
    //   9. Actualizar flags de contacto
    //
    // Salto ANTES que gravedad para que el primer paso tras saltar conserve
    // la velocidad inicial completa; al reves, la gravedad recortaria la
    // altura del salto en un paso y esta dejaria de coincidir con jumpHeight.
    void fixedUpdate(MovementState& state, const InputCommand& input, float dt) {

        // ---- 1. Posicion previa (interpolacion de render) ----
        state.previousPosition = state.position;
        state.wasOnGround = state.onGround;

        const float curHeight = state.isCrouching ? cfg->crouchHeight : cfg->height;

        // ---- 2. Deteccion de medio ----
        state.isInWater = collision.isInWater(state.position, curHeight);
        state.isOnLadder = collision.isOnLadder(state.position, curHeight);

        MoveVec3 eye = state.position;
        eye.y += state.isCrouching ? cfg->crouchEyeHeight : cfg->eyeHeight;
        state.isSubmerged = collision.isPointInLiquid(eye);

        // ---- 3. Agachado ----
        // Solo se puede dejar de estar agachado si hay hueco para erguirse:
        // si no, el jugador atravesaria el techo al levantarse.
        if (input.crouch && !state.isFlying) {
            state.isCrouching = true;
        } else if (state.isCrouching) {
            if (collision.canStandUp(state.position)) {
                state.isCrouching = false;
            }
            // si no cabe, sigue agachado
        }

        // Sprint: requiere avanzar y no estar agachado.
        state.isSprinting = input.sprint && input.moveForward > 0.1f && !state.isCrouching;

        // ---- 4. Timers de salto ----
        jump.updateTimers(state, input, dt);

        // ---- 5. Movimiento horizontal ----
        movement.apply(state, input, dt);

        // ---- 6. Salto ----
        jump.tryJump(state, input);

        // ---- 7. Gravedad ----
        gravity.apply(state, dt);

        // ---- 8. Integracion + colisiones ----
        const float height = state.isCrouching ? cfg->crouchHeight : cfg->height;

        MoveVec3 delta(state.velocity.x * dt,
                       state.velocity.y * dt,
                       state.velocity.z * dt);

        // Step-up solo tiene sentido caminando por el suelo: en el aire
        // permitiria "escalar" paredes al rozarlas.
        const bool allowStep = (state.onGround || state.isOnLadder)
                             && !state.isFlying
                             && !state.isInWater;

        CollisionResult res = collision.move(state.position, delta, height, allowStep);

        // ---- CROUCH EDGE GUARD ----
        // Agachado en el borde de un bloque: se impide caer.
        // Se prueba la posicion destino; si deja de haber suelo, se cancela
        // el componente horizontal que llevaria al vacio.
        if (state.isCrouching && state.onGround && !res.groundedBelow && state.velocity.y <= 0.0f) {
            MoveVec3 safe = state.position;

            // Probar solo X
            MoveVec3 tryX = state.position;
            tryX.x = res.finalPosition.x;
            MoveVec3 probeX = tryX; probeX.y -= 0.05f;
            if (collision.isPositionBlocked(probeX, height)) safe.x = tryX.x;

            // Probar solo Z
            MoveVec3 tryZ = state.position;
            tryZ.z = res.finalPosition.z;
            MoveVec3 probeZ = tryZ; probeZ.y -= 0.05f;
            if (collision.isPositionBlocked(probeZ, height)) safe.z = tryZ.z;

            safe.y = res.finalPosition.y;
            res.finalPosition = safe;
        }

        state.position = res.finalPosition;

        // ---- 9. Flags de contacto ----
        if (res.hitY) {
            if (state.velocity.y < 0.0f) {
                state.onGround = true;
                state.velocity.y = 0.0f;
            } else {
                state.hitCeiling = true;
                state.velocity.y = 0.0f;
            }
        } else {
            state.hitCeiling = false;
        }

        // El estado de suelo lo dicta la sonda, no solo el choque de este paso:
        // asi caminar sobre terreno plano mantiene onGround estable.
        state.onGround = res.groundedBelow && state.velocity.y <= 0.001f;

        // Anular velocidad contra paredes (evita acumular energia empujando).
        if (res.hitX) state.velocity.x = 0.0f;
        if (res.hitZ) state.velocity.z = 0.0f;
        state.hitWall = res.hitX || res.hitZ;

        if (res.hitX || res.hitY || res.hitZ) {
            state.lastCollisionNormal = res.normal;
        }

        lastStepUpAmount = res.steppedUp ? res.stepUpAmount : 0.0f;

        // ---- SALVAGUARDA ANTI-NaN ----
        // Si algo produjera un NaN, sin esto el jugador quedaria en un estado
        // irrecuperable (toda comparacion con NaN es falsa) y el juego se
        // volveria injugable hasta reiniciar.
        if (!state.position.isFinite()) {
            state.position = state.previousPosition;
            state.velocity = MoveVec3(0,0,0);
        }
        if (!state.velocity.isFinite()) {
            state.velocity = MoveVec3(0,0,0);
        }

        lastQueryCount = collision.consumeQueryCount();
        state.collisionChecksThisFrame = lastQueryCount;
    }
};

} // namespace PlayerSys

#endif // PHYSICS_SYSTEM_H
