#ifndef PLAYER_CONTROLLER_H
#define PLAYER_CONTROLLER_H

#include "PlayerTypes.h"
#include "InputManager.h"
#include "PhysicsSystem.h"
#include "CameraSystem.h"

// ============================================================================
// PLAYER CONTROLLER - Fachada del sistema de movimiento
// ============================================================================
// RESPONSABILIDAD UNICA: coordinar Input -> Camara -> Fisica y exponer una
// API sencilla al motor. No implementa fisica: delega.
//
// El motor solo necesita:
//     controller.update(frameDt);
//     Vec3 ojo = controller.getEyePosition();
//
// ARQUITECTURA (dependencias en una sola direccion):
//
//     InputManager ──► InputCommand ──┐
//                                     ├──► PlayerController
//     IWorldQuery ──► PhysicsSystem ──┘         │
//                        │                      │
//         ┌──────────────┼──────────────┐       ▼
//         ▼              ▼              ▼   CameraSystem
//    Collision      Gravity/Jump    Movement
//
// Ningun subsistema conoce a su padre ni a sus hermanos: todos operan sobre
// MovementState y MovementConfig, que son datos puros.
// ============================================================================

namespace PlayerSys {

class PlayerController {
private:
    MovementConfig config;
    MovementState  state;

    InputManager   input;
    PhysicsSystem  physics;
    CameraSystem   camera;

    // Ultimo comando (depuracion / HUD)
    InputCommand   lastCommand;

public:
    PlayerController(const IWorldQuery* world)
        : input(&config),
          physics(world, &config),
          camera(&config) {}

    // ---- Acceso ----
    MovementState&       getState()        { return state; }
    const MovementState& getState()  const { return state; }
    MovementConfig&      getConfig()       { return config; }
    const MovementConfig& getConfig() const { return config; }
    InputManager&        getInput()        { return input; }
    const PhysicsSystem& getPhysics() const { return physics; }
    const CameraSystem&  getCamera()  const { return camera; }
    const InputCommand&  getLastCommand() const { return lastCommand; }

    void setPosition(float x, float y, float z) {
        state.position = MoveVec3(x, y, z);
        state.previousPosition = state.position;
        state.velocity = MoveVec3(0, 0, 0);
        state.stepSmoothOffset = 0.0f;
    }

    void setRotation(float yaw, float pitch) {
        state.yaw = yaw;
        state.pitch = MathUtil::clamp(pitch, -config.maxPitch, config.maxPitch);
    }

    void setFlying(bool f) {
        state.isFlying = f;
        if (f) {
            state.onGround = false;
        } else {
            // Al desactivar el vuelo se anula la velocidad vertical para que
            // no herede un impulso residual y caiga de golpe.
            state.velocity.y = 0.0f;
        }
    }

    bool isFlying() const { return state.isFlying; }

    // ========================================================================
    // ACTUALIZAR (una vez por frame)
    // ========================================================================
    void update(float frameDt) {
        // 1. Construir comando del frame (consume deltas de raton)
        lastCommand = input.buildCommand();

        // 2. CAMARA PRIMERO.
        //    La rotacion debe aplicarse ANTES de la fisica para que el
        //    movimiento de este frame use la orientacion que el jugador
        //    acaba de fijar. Al reves habria un frame de latencia entre
        //    girar y avanzar en la nueva direccion — perceptible como
        //    "el personaje va donde yo miraba, no donde miro".
        camera.rotate(state, lastCommand.lookDeltaYaw, lastCommand.lookDeltaPitch);

        // 3. Fisica con pasos fijos
        const int steps = physics.update(state, lastCommand, frameDt);

        // 4. Notificar escalon a la camara (efecto visual)
        if (steps > 0) {
            const float stepAmount = physics.getLastStepUp();
            if (stepAmount > 0.0f) {
                camera.notifyStepUp(state, stepAmount);
            }
        }

        // 5. Suavizado visual del escalon (por frame, no por paso)
        camera.updateStepSmoothing(state, frameDt);
    }

    // ========================================================================
    // CONSULTAS PARA EL RENDER
    // ========================================================================
    MoveVec3 getEyePosition() const {
        return camera.getEyePosition(state, physics.getInterpolationAlpha());
    }

    MoveVec3 getInterpolatedPosition() const {
        const float a = physics.getInterpolationAlpha();
        return MoveVec3(
            MathUtil::lerp(state.previousPosition.x, state.position.x, a),
            MathUtil::lerp(state.previousPosition.y, state.position.y, a),
            MathUtil::lerp(state.previousPosition.z, state.position.z, a)
        );
    }

    MoveVec3 getLookDirection() const { return camera.getLookDirection(state); }

    float getYaw()   const { return state.yaw; }
    float getPitch() const { return state.pitch; }
    float getSpeed() const { return state.velocity.lengthXZ(); }

    // Nombre del estado actual (HUD de depuracion).
    const char* getStateName() const {
        if (state.isFlying)     return "VOLANDO";
        if (state.isOnLadder)   return "ESCALERA";
        if (state.isSubmerged)  return "SUMERGIDO";
        if (state.isInWater)    return "EN AGUA";
        if (!state.onGround)    return state.velocity.y > 0.0f ? "SALTANDO" : "CAYENDO";
        if (state.isCrouching)  return "AGACHADO";
        if (state.isSprinting)  return "CORRIENDO";
        if (state.velocity.lengthSqXZ() > 0.01f) return "CAMINANDO";
        return "QUIETO";
    }
};

} // namespace PlayerSys

#endif // PLAYER_CONTROLLER_H
