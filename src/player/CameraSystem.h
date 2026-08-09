#ifndef CAMERA_SYSTEM_H
#define CAMERA_SYSTEM_H

#include "PlayerTypes.h"

// ============================================================================
// CAMERA SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: orientacion de la vista y posicion del ojo.
// No mueve al jugador ni toca la velocidad.
//
// ----------------------------------------------------------------------------
// POR QUE LA ROTACION NO USA DELTA TIME
// ----------------------------------------------------------------------------
// El delta del raton YA es un desplazamiento acumulado: si el frame dura el
// doble, el raton habra generado el doble de cuentas. Multiplicar ademas por
// dt aplicaria la escala dos veces y haria que la sensibilidad cambiara con
// el framerate — el error mas comun en camaras FPS.
//
// Por eso rotate() NO recibe dt: se aplica el delta crudo escalado solo por
// la sensibilidad. Asi el giro por centimetro de raton es identico a 30 y a
// 500 FPS.
//
// ----------------------------------------------------------------------------
// ESTABILIDAD (sin jitter)
// ----------------------------------------------------------------------------
// Tres medidas concretas:
//
//   1. La rotacion se aplica SIEMPRE en el hilo de render, con el delta del
//      frame actual: nunca se interpola ni se retrasa. Latencia cero.
//   2. La POSICION del ojo se interpola entre pasos de fisica fijos, porque
//      la fisica corre a 120 Hz y el render puede ir a 144 o 240: sin
//      interpolar se veria el micro-stutter de mostrar la misma posicion en
//      dos frames y saltar en el tercero.
//   3. El escalon (step-up) mueve la posicion de golpe hasta 0.6 bloques.
//      Ese salto se compensa con un offset visual que decae suavemente, de
//      modo que el mundo no da un tiron vertical al subir un bloque.
// ============================================================================

namespace PlayerSys {

class CameraSystem {
private:
    const MovementConfig* cfg;

public:
    explicit CameraSystem(const MovementConfig* c) : cfg(c) {}

    // ========================================================================
    // ROTACION (sin delta time, ver nota de cabecera)
    // ========================================================================
    void rotate(MovementState& state, float deltaYaw, float deltaPitch) const {
        // Guarda anti-NaN: un delta corrupto podria envenenar el estado de
        // camara de forma permanente.
        if (!std::isfinite(deltaYaw) || !std::isfinite(deltaPitch)) return;

        state.yaw += deltaYaw;

        const float pitchDir = cfg->invertY ? -1.0f : 1.0f;
        state.pitch += deltaPitch * pitchDir;

        // Clamp del pitch: nunca llegar a +-90 exactos, porque ahi el vector
        // de vista se vuelve degenerado (forward horizontal = 0) y el
        // movimiento perderia su direccion de referencia.
        state.pitch = MathUtil::clamp(state.pitch, -cfg->maxPitch, cfg->maxPitch);

        // Yaw en [0, 360): evita perdida de precision de float tras un rato
        // girando en la misma direccion.
        if (state.yaw >= 360.0f)     state.yaw -= 360.0f;
        else if (state.yaw < 0.0f)   state.yaw += 360.0f;
    }

    // ========================================================================
    // ALTURA DE OJO
    // ========================================================================
    float getEyeHeight(const MovementState& state) const {
        return state.isCrouching ? cfg->crouchEyeHeight : cfg->eyeHeight;
    }

    // ========================================================================
    // POSICION DEL OJO INTERPOLADA
    // ========================================================================
    // alpha: fraccion [0,1] entre el paso de fisica anterior y el actual.
    // Es lo que convierte una fisica a 120 Hz en una imagen fluida a
    // cualquier framerate de render.
    MoveVec3 getEyePosition(const MovementState& state, float alpha) const {
        MoveVec3 p;

        if (cfg->enableCameraSmoothing) {
            p.x = MathUtil::lerp(state.previousPosition.x, state.position.x, alpha);
            p.y = MathUtil::lerp(state.previousPosition.y, state.position.y, alpha);
            p.z = MathUtil::lerp(state.previousPosition.z, state.position.z, alpha);
        } else {
            p = state.position;
        }

        p.y += getEyeHeight(state);

        // Compensacion del escalon: se resta el offset pendiente, que decae
        // a cero en stepSmoothTime segundos.
        p.y -= state.stepSmoothOffset;

        return p;
    }

    // ========================================================================
    // ACTUALIZAR SUAVIZADO DEL ESCALON
    // ========================================================================
    // Se llama una vez por FRAME DE RENDER (no por paso de fisica), porque es
    // un efecto puramente visual.
    void updateStepSmoothing(MovementState& state, float frameDt) const {
        if (state.stepSmoothOffset == 0.0f) return;

        if (cfg->stepSmoothTime <= 0.0f) {
            state.stepSmoothOffset = 0.0f;
            return;
        }

        // Decaimiento exponencial (independiente del FPS).
        const float rate = 1.0f / cfg->stepSmoothTime;
        state.stepSmoothOffset = MathUtil::damp(state.stepSmoothOffset, 0.0f,
                                                rate * 3.0f, frameDt);

        if (fabsf(state.stepSmoothOffset) < 0.002f) {
            state.stepSmoothOffset = 0.0f;
        }
    }

    // Registrar un escalon recien subido: la posicion ya subio, asi que se
    // introduce un offset negativo que la camara ira reabsorbiendo.
    void notifyStepUp(MovementState& state, float amount) const {
        state.stepSmoothOffset += amount;
        // Tope de seguridad
        if (state.stepSmoothOffset > cfg->stepHeight) {
            state.stepSmoothOffset = cfg->stepHeight;
        }
    }

    // Direccion de vista completa.
    MoveVec3 getLookDirection(const MovementState& state) const {
        const float y = state.yaw   * MathUtil::DEG2RAD;
        const float p = state.pitch * MathUtil::DEG2RAD;
        const float cp = cosf(p);
        return MoveVec3(sinf(y) * cp, sinf(p), -cosf(y) * cp);
    }
};

} // namespace PlayerSys

#endif // CAMERA_SYSTEM_H
