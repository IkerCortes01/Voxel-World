#ifndef PLAYER_TYPES_H
#define PLAYER_TYPES_H

#include <cmath>
#include <cstdint>

// ============================================================================
// PLAYER TYPES - Tipos y configuracion compartidos del sistema de movimiento
// ============================================================================
// Este header no depende de NADA del motor (ni OpenGL, ni World, ni GLFW).
// Contiene:
//   - MoveVec3: vector matematico independiente
//   - MovementConfig: TODOS los parametros ajustables en un solo sitio
//   - MovementState: estado del jugador (datos puros)
//   - IWorldQuery: interfaz de consulta al mundo (inversion de dependencias)
//
// PRINCIPIO SOLID aplicado: Dependency Inversion.
// Los modulos de fisica dependen de la ABSTRACCION IWorldQuery, no de la
// clase World concreta. Esto permite:
//   - Probar la fisica sin cargar un mundo real (tests deterministas)
//   - Cambiar el backend del mundo sin tocar la fisica
// ============================================================================

namespace PlayerSys {

// ----------------------------------------------------------------------------
// VECTOR 3D
// ----------------------------------------------------------------------------
// Propio, para que el modulo no dependa del Vec3 del motor.
struct MoveVec3 {
    float x, y, z;

    MoveVec3() : x(0), y(0), z(0) {}
    MoveVec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    MoveVec3 operator+(const MoveVec3& o) const { return MoveVec3(x+o.x, y+o.y, z+o.z); }
    MoveVec3 operator-(const MoveVec3& o) const { return MoveVec3(x-o.x, y-o.y, z-o.z); }
    MoveVec3 operator*(float s)           const { return MoveVec3(x*s, y*s, z*s); }

    MoveVec3& operator+=(const MoveVec3& o) { x+=o.x; y+=o.y; z+=o.z; return *this; }

    float lengthSq()   const { return x*x + y*y + z*z; }
    float length()     const { return sqrtf(lengthSq()); }
    float lengthSqXZ() const { return x*x + z*z; }
    float lengthXZ()   const { return sqrtf(lengthSqXZ()); }

    MoveVec3 normalized() const {
        const float len = length();
        if (len < 1e-6f) return MoveVec3(0,0,0);
        const float inv = 1.0f / len;
        return MoveVec3(x*inv, y*inv, z*inv);
    }

    // Normaliza solo el plano horizontal, conservando Y.
    MoveVec3 normalizedXZ() const {
        const float len = lengthXZ();
        if (len < 1e-6f) return MoveVec3(0, y, 0);
        const float inv = 1.0f / len;
        return MoveVec3(x*inv, y, z*inv);
    }

    bool isFinite() const {
        return std::isfinite(x) && std::isfinite(y) && std::isfinite(z);
    }
};

// ----------------------------------------------------------------------------
// CONFIGURACION DE MOVIMIENTO
// ----------------------------------------------------------------------------
// TODOS los numeros magicos del sistema viven aqui. Ajustar la sensacion del
// personaje = tocar esta struct, sin buscar constantes por el codigo.
//
// Los valores por defecto reproducen la sensacion de un sandbox voxel
// moderno: respuesta inmediata, inercia corta pero perceptible, salto de
// ~1.25 bloques que permite subir un bloque con margen.
struct MovementConfig {

    // ---- DIMENSIONES DE LA CAPSULA ----
    float width        = 0.6f;   // Anchura (diametro de la capsula)
    float height       = 1.8f;   // Altura de pie
    // Agachado: 0.95, no 1.5.
    //
    // El caso de uso principal de agacharse en un sandbox voxel es PASAR POR
    // UN HUECO DE 1 BLOQUE (excavar un tunel de una altura). Con 1.5 la caja
    // seguia sin caber en un hueco de 1.0 y agacharse no servia para nada
    // salvo caminar despacio. Con 0.95 cabe con margen.
    float crouchHeight = 0.95f;  // Altura agachado
    float eyeHeight    = 1.62f;  // Altura de camara de pie
    float crouchEyeHeight = 0.80f; // Altura de camara agachado

    // ---- VELOCIDADES OBJETIVO (bloques/segundo) ----
    float walkSpeed    = 4.317f; // Caminar
    float sprintSpeed  = 5.612f; // Correr (~1.3x)
    float crouchSpeed  = 1.31f;  // Agachado
    float swimSpeed    = 2.20f;  // Nadando
    float flySpeed     = 10.0f;  // Vuelo creativo
    float flySprintSpeed = 21.0f;// Vuelo rapido
    float ladderSpeed  = 2.35f;  // Subir/bajar escalera

    // ---- ACELERACION Y FRICCION ----
    // Modelo exponencial: v += (objetivo - v) * (1 - exp(-tasa*dt))
    // Es INDEPENDIENTE DEL FPS por construccion (ver MovementSystem).
    float groundAccel    = 45.0f; // Respuesta en suelo (alto = inmediato)
    float groundFriction = 38.0f; // Frenado en suelo
    float airAccel       = 9.0f;  // Control aereo parcial
    float airFriction    = 1.2f;  // Rozamiento del aire (bajo = conserva impulso)
    float waterAccel     = 12.0f;
    float waterFriction  = 6.0f;

    // ---- SALTO ----
    // La altura es el parametro AUTORITATIVO; la velocidad inicial se deriva
    // de ella y de la gravedad (ver JumpSystem). Asi "quiero saltar 1.25
    // bloques" se expresa directamente.
    float jumpHeight       = 1.252f; // Bloques de altura del salto
    float sprintJumpBoost  = 0.20f;  // Impulso horizontal extra al saltar corriendo
    float coyoteTime       = 0.10f;  // Margen para saltar tras dejar el borde (s)
    float jumpBufferTime   = 0.12f;  // Margen para registrar salto antes de aterrizar (s)
    float waterJumpSpeed   = 3.2f;   // Impulso de nado hacia arriba

    // ---- GRAVEDAD ----
    float gravity          = 28.0f;  // Bloques/s^2 (suave)
    float fallGravityMult  = 1.28f;  // Gravedad extra al caer -> respuesta mas nitida
    float maxFallSpeed     = 78.4f;  // Velocidad terminal
    float waterGravity     = 4.5f;   // Gravedad reducida en agua
    float buoyancy         = 3.6f;   // Empuje hacia arriba en agua

    // ---- ESCALONES ----
    // 1.05, no 0.6.
    //
    // En un mundo de voxeles un bloque ocupa el intervalo [y, y+1), asi que
    // su SUPERFICIE esta exactamente 1.0 unidades por encima del suelo
    // anterior. El requisito "subir bloques de 1 bloque de altura" exige por
    // tanto stepHeight > 1.0; con 0.6 (valor tipico de motores de terreno
    // continuo tipo Unity/Unreal) el jugador choca contra cada bloque suelto
    // y hay que saltar para todo.
    //
    // El margen de 0.05 absorbe el error de coma flotante al comparar contra
    // el limite exacto del voxel.
    float stepHeight       = 1.05f;  // Altura maxima a subir automaticamente
    float maxStepUpSpeed   = 0.0f;   // 0 = instantaneo (sin animacion vertical)

    // ---- COLISIONES ----
    float skinWidth        = 0.001f; // Margen anti-penetracion
    int   maxSlideIterations = 3;    // Iteraciones de deslizamiento por eje

    // ---- CAMARA ----
    float mouseSensitivity = 0.15f;
    bool  invertY          = false;
    float maxPitch         = 89.9f;
    float stepSmoothTime   = 0.10f;  // Suavizado visual al subir escalon (s)
    bool  enableCameraSmoothing = true;

    // ---- FIXED TIMESTEP ----
    // La fisica se integra en pasos fijos, lo que garantiza resultados
    // identicos a cualquier FPS (ver PhysicsSystem).
    float fixedTimeStep    = 1.0f / 120.0f; // 120 Hz
    // 16 pasos = 0.133 s de tiempo simulado por frame, suficiente para
    // absorber el clamp de dt del motor (0.1 s = 12 pasos) sin perder tiempo.
    //
    // Con el valor anterior (8) el tope era 0.0667 s: por debajo de ~15 FPS la
    // fisica no podia consumir todo el tiempo real y el juego entraba en
    // CAMARA LENTA permanente (a 10 FPS el mundo avanzaba al 66% de
    // velocidad). Se midio: ratio tiempo simulado/real = 0.667 a 10 FPS.
    // Justo el escenario "equipo lento" que hay que proteger.
    int   maxSubSteps      = 16;            // Tope anti espiral de la muerte
};

// ----------------------------------------------------------------------------
// ESTADO DEL JUGADOR
// ----------------------------------------------------------------------------
// Datos puros. Ningun metodo con logica: los sistemas operan sobre este
// estado (Single Responsibility).
struct MovementState {
    MoveVec3 position;
    MoveVec3 velocity;

    // Orientacion
    float yaw   = 0.0f;   // grados
    float pitch = 0.0f;   // grados

    // --- Flags de contacto ---
    bool onGround      = false;
    bool wasOnGround   = false;
    bool hitCeiling    = false;
    bool hitWall       = false;

    // --- Estados de movimiento ---
    bool isSprinting   = false;
    bool isCrouching   = false;
    bool isFlying      = false;
    bool isInWater     = false;  // cuerpo en agua
    bool isSubmerged   = false;  // ojos bajo el agua
    bool isOnLadder    = false;

    // --- Temporizadores (para coyote time y buffer de salto) ---
    float timeSinceGrounded = 999.0f;
    float timeSinceJumpPressed = 999.0f;
    bool  jumpConsumed = false;

    // --- Datos de colision (depuracion) ---
    MoveVec3 lastCollisionNormal;
    MoveVec3 lastMoveDirection;
    int      collisionChecksThisFrame = 0;

    // --- Suavizado visual del escalon ---
    // Al subir un escalon la posicion salta instantaneamente, pero la CAMARA
    // se interpola para que no haya un tiron vertical.
    float stepSmoothOffset = 0.0f;

    // --- Interpolacion de render ---
    // Guarda la posicion del paso fisico anterior para interpolar el render
    // entre pasos fijos (elimina el micro-stutter).
    MoveVec3 previousPosition;
};

// ----------------------------------------------------------------------------
// COMANDO DE ENTRADA
// ----------------------------------------------------------------------------
// El input se traduce a esta estructura NEUTRA antes de llegar a la fisica.
// Ventaja: la fisica no sabe nada de GLFW ni de teclas concretas, y se puede
// alimentar desde un test, una repeticion o la red.
struct InputCommand {
    float moveForward = 0.0f; // [-1, 1]
    float moveRight   = 0.0f; // [-1, 1]
    bool  jump        = false;
    bool  sprint      = false;
    bool  crouch      = false;
    bool  flyUp       = false;
    bool  flyDown     = false;

    // Delta del raton acumulado este frame (grados ya escalados).
    float lookDeltaYaw   = 0.0f;
    float lookDeltaPitch = 0.0f;

    void clear() {
        moveForward = moveRight = 0.0f;
        jump = sprint = crouch = flyUp = flyDown = false;
        lookDeltaYaw = lookDeltaPitch = 0.0f;
    }
};

// ----------------------------------------------------------------------------
// INTERFAZ DE CONSULTA AL MUNDO (Dependency Inversion)
// ----------------------------------------------------------------------------
// Lo MINIMO que la fisica necesita saber del mundo. El motor implementa esto
// sobre su clase World; los tests lo implementan sobre un mundo sintetico.
class IWorldQuery {
public:
    virtual ~IWorldQuery() = default;

    // Un bloque solido bloquea el movimiento.
    virtual bool isSolid(int x, int y, int z) const = 0;
    // Agua: afecta a flotabilidad y velocidad.
    virtual bool isLiquid(int x, int y, int z) const = 0;
    // Escalera: permite ascenso vertical controlado.
    virtual bool isClimbable(int x, int y, int z) const = 0;
};

// ----------------------------------------------------------------------------
// UTILIDADES MATEMATICAS
// ----------------------------------------------------------------------------
namespace MathUtil {

    inline float clamp(float v, float lo, float hi) {
        return v < lo ? lo : (v > hi ? hi : v);
    }

    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    // ------------------------------------------------------------------------
    // SUAVIZADO INDEPENDIENTE DEL FRAMERATE
    // ------------------------------------------------------------------------
    // El lerp ingenuo `v += (objetivo - v) * tasa * dt` DEPENDE del framerate:
    // a 30 FPS converge distinto que a 240 FPS, porque aplicar N veces un
    // factor no equivale a aplicarlo una vez con N*dt.
    //
    // La forma correcta es la solucion exacta de la EDO dv/dt = k(objetivo-v):
    //     v(t+dt) = objetivo + (v - objetivo) * exp(-k*dt)
    //
    // Esto SI es exacto a cualquier dt, y es lo que hace que el movimiento se
    // sienta identico a 30 y a 500 FPS.
    inline float damp(float current, float target, float rate, float dt) {
        if (rate <= 0.0f) return current;
        const float t = 1.0f - expf(-rate * dt);
        return current + (target - current) * t;
    }

    inline float sign(float v) {
        return (v > 0.0f) ? 1.0f : ((v < 0.0f) ? -1.0f : 0.0f);
    }

    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI / 180.0f;

} // namespace MathUtil

} // namespace PlayerSys

#endif // PLAYER_TYPES_H
