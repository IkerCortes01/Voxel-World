#ifndef PLAYER_DEBUG_H
#define PLAYER_DEBUG_H

#include "PlayerController.h"

// ============================================================================
// PLAYER DEBUG - Herramientas de visualizacion
// ============================================================================
// RESPONSABILIDAD UNICA: exponer el estado interno del controlador para
// depuracion. Aislado en su propio header para poder excluirlo de release.
//
// Este modulo NO dibuja: produce DATOS de dibujo (lineas, cajas, texto) que
// el motor renderiza con su propia API. Asi el sistema de movimiento sigue
// sin depender de OpenGL.
// ============================================================================

namespace PlayerSys {

// Un segmento de linea a dibujar (para vectores y cajas).
struct DebugLine {
    MoveVec3 from;
    MoveVec3 to;
    float r, g, b;
};

class PlayerDebug {
public:
    // Interruptores individuales.
    bool showHitbox      = false;
    bool showVelocity    = false;
    bool showNormals     = false;
    bool showMoveDir     = false;
    bool showStateText   = false;

    bool anyEnabled() const {
        return showHitbox || showVelocity || showNormals || showMoveDir || showStateText;
    }

    void toggleAll() {
        const bool on = !anyEnabled();
        showHitbox = showVelocity = showNormals = showMoveDir = showStateText = on;
    }

    // ========================================================================
    // GENERAR GEOMETRIA DE DEPURACION
    // ========================================================================
    // Rellena `out` con hasta `maxLines` segmentos. Devuelve cuantos escribio.
    // Sin asignaciones dinamicas: el llamador provee el buffer.
    int buildLines(const PlayerController& pc, DebugLine* out, int maxLines) const {
        if (out == nullptr || maxLines <= 0) return 0;

        const MovementState& s = pc.getState();
        const MovementConfig& cfg = pc.getConfig();
        int n = 0;

        // ---- HITBOX ----
        if (showHitbox && n + 12 <= maxLines) {
            const float h = cfg.width * 0.5f;
            const float height = s.isCrouching ? cfg.crouchHeight : cfg.height;
            const MoveVec3 p = s.position;

            const float x0 = p.x - h, x1 = p.x + h;
            const float y0 = p.y,     y1 = p.y + height;
            const float z0 = p.z - h, z1 = p.z + h;

            // Verde si esta en el suelo, rojo si esta en el aire.
            const float cr = s.onGround ? 0.1f : 1.0f;
            const float cg = s.onGround ? 1.0f : 0.3f;
            const float cb = 0.1f;

            // 4 aristas inferiores
            out[n++] = { MoveVec3(x0,y0,z0), MoveVec3(x1,y0,z0), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y0,z0), MoveVec3(x1,y0,z1), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y0,z1), MoveVec3(x0,y0,z1), cr,cg,cb };
            out[n++] = { MoveVec3(x0,y0,z1), MoveVec3(x0,y0,z0), cr,cg,cb };
            // 4 aristas superiores
            out[n++] = { MoveVec3(x0,y1,z0), MoveVec3(x1,y1,z0), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y1,z0), MoveVec3(x1,y1,z1), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y1,z1), MoveVec3(x0,y1,z1), cr,cg,cb };
            out[n++] = { MoveVec3(x0,y1,z1), MoveVec3(x0,y1,z0), cr,cg,cb };
            // 4 verticales
            out[n++] = { MoveVec3(x0,y0,z0), MoveVec3(x0,y1,z0), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y0,z0), MoveVec3(x1,y1,z0), cr,cg,cb };
            out[n++] = { MoveVec3(x1,y0,z1), MoveVec3(x1,y1,z1), cr,cg,cb };
            out[n++] = { MoveVec3(x0,y0,z1), MoveVec3(x0,y1,z1), cr,cg,cb };
        }

        // ---- VECTOR DE VELOCIDAD (cian) ----
        if (showVelocity && n + 2 <= maxLines) {
            MoveVec3 c = s.position;
            c.y += 1.0f;
            // Escala 0.2 para que sea legible sin salirse de pantalla.
            const MoveVec3 tip(c.x + s.velocity.x * 0.2f,
                               c.y + s.velocity.y * 0.2f,
                               c.z + s.velocity.z * 0.2f);
            out[n++] = { c, tip, 0.0f, 1.0f, 1.0f };

            // Componente horizontal en azul (util para ver la inercia)
            const MoveVec3 tipXZ(c.x + s.velocity.x * 0.2f, c.y, c.z + s.velocity.z * 0.2f);
            out[n++] = { c, tipXZ, 0.2f, 0.4f, 1.0f };
        }

        // ---- DIRECCION DE MOVIMIENTO DESEADA (amarillo) ----
        if (showMoveDir && n + 1 <= maxLines) {
            MoveVec3 c = s.position;
            c.y += 0.1f;
            const MoveVec3 tip(c.x + s.lastMoveDirection.x * 1.5f, c.y,
                               c.z + s.lastMoveDirection.z * 1.5f);
            out[n++] = { c, tip, 1.0f, 1.0f, 0.0f };
        }

        // ---- NORMAL DE COLISION (magenta) ----
        if (showNormals && n + 1 <= maxLines) {
            if (s.lastCollisionNormal.lengthSq() > 0.01f) {
                MoveVec3 c = s.position;
                c.y += 0.9f;
                const MoveVec3 tip(c.x + s.lastCollisionNormal.x * 1.0f,
                                   c.y + s.lastCollisionNormal.y * 1.0f,
                                   c.z + s.lastCollisionNormal.z * 1.0f);
                out[n++] = { c, tip, 1.0f, 0.0f, 1.0f };
            }
        }

        return n;
    }

    // ========================================================================
    // LINEAS DE TEXTO DE ESTADO
    // ========================================================================
    // Escribe hasta `maxLines` cadenas en el buffer provisto.
    // bufferSize es el tamaño de CADA cadena.
    int buildStatusText(const PlayerController& pc,
                        char lines[][96], int maxLines) const {
        if (!showStateText || maxLines <= 0) return 0;

        const MovementState& s = pc.getState();
        int n = 0;

        auto add = [&](const char* fmt, auto... args) {
            if (n < maxLines) {
                snprintf(lines[n], 96, fmt, args...);
                ++n;
            }
        };

        add("ESTADO: %s", pc.getStateName());
        add("POS  X:%.2f Y:%.2f Z:%.2f", s.position.x, s.position.y, s.position.z);
        add("VEL  X:%.2f Y:%.2f Z:%.2f", s.velocity.x, s.velocity.y, s.velocity.z);
        add("VEL horizontal: %.3f b/s", s.velocity.lengthXZ());
        add("YAW:%.1f PITCH:%.1f", s.yaw, s.pitch);
        add("suelo:%d techo:%d pared:%d", (int)s.onGround, (int)s.hitCeiling, (int)s.hitWall);
        add("agua:%d sumergido:%d escalera:%d", (int)s.isInWater, (int)s.isSubmerged, (int)s.isOnLadder);
        add("agachado:%d corriendo:%d volando:%d", (int)s.isCrouching, (int)s.isSprinting, (int)s.isFlying);
        add("pasos fisica/frame: %d", pc.getPhysics().getLastStepCount());
        add("consultas colision: %d", s.collisionChecksThisFrame);
        add("coyote: %.3fs  buffer: %.3fs", s.timeSinceGrounded, s.timeSinceJumpPressed);

        return n;
    }
};

} // namespace PlayerSys

#endif // PLAYER_DEBUG_H
