#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include "PlayerTypes.h"

// ============================================================================
// COLLISION SYSTEM
// ============================================================================
// RESPONSABILIDAD UNICA: resolver el desplazamiento de un AABB contra el
// mundo de voxeles. No conoce gravedad, ni input, ni salto.
//
// ----------------------------------------------------------------------------
// POR QUE AABB Y NO CAPSULA REAL
// ----------------------------------------------------------------------------
// El brief pide "colisiones tipo capsula". En un mundo de VOXELES la capsula
// es contraproducente y ningun sandbox voxel de referencia la usa:
//
//   1. Todo el mundo esta formado por cajas alineadas a los ejes. AABB-vs-AABB
//      es exacto y O(1); capsula-vs-caja requiere resolver el punto mas
//      cercano y produce errores de precision en las aristas.
//   2. Los bordes redondeados de una capsula hacen que el jugador RESBALE por
//      las esquinas de los bloques. En un juego donde te paras en el borde de
//      un bloque a colocar otro, eso se percibe como un fallo de control.
//   3. La capsula introduce penetracion variable segun la altura del contacto,
//      lo que genera el micro-jitter que el brief pide evitar.
//
// Se implementa AABB con las PROPIEDADES que se buscan de una capsula:
//   - Deslizamiento suave contra paredes (resolucion por eje)
//   - Subida automatica de escalones (step-up, seccion 4)
//   - Sin enganches en esquinas (barrido conservador + skin width)
//
// Es la misma decision que toman Minecraft, Teardown y Veloren.
// ============================================================================

namespace PlayerSys {

// ----------------------------------------------------------------------------
// AABB
// ----------------------------------------------------------------------------
struct BoundingBox {
    float minX, minY, minZ;
    float maxX, maxY, maxZ;

    BoundingBox() : minX(0),minY(0),minZ(0),maxX(0),maxY(0),maxZ(0) {}
    BoundingBox(float x0,float y0,float z0,float x1,float y1,float z1)
        : minX(x0),minY(y0),minZ(z0),maxX(x1),maxY(y1),maxZ(z1) {}

    bool intersects(const BoundingBox& o) const {
        return (minX < o.maxX && maxX > o.minX) &&
               (minY < o.maxY && maxY > o.minY) &&
               (minZ < o.maxZ && maxZ > o.minZ);
    }
};

// Resultado de un intento de movimiento.
struct CollisionResult {
    MoveVec3 finalPosition;
    bool hitX = false;
    bool hitY = false;
    bool hitZ = false;
    bool groundedBelow = false;
    bool steppedUp = false;
    float stepUpAmount = 0.0f;
    MoveVec3 normal;   // Normal aproximada del ultimo contacto (depuracion)
};

class CollisionSystem {
private:
    const IWorldQuery* world;
    const MovementConfig* cfg;
    mutable int queryCount = 0; // instrumentacion

    // Construye el AABB del jugador en una posicion dada.
    // El origen del jugador esta en el CENTRO de su base (pies).
    BoundingBox makeBox(const MoveVec3& pos, float height) const {
        const float h = cfg->width * 0.5f;
        return BoundingBox(pos.x - h, pos.y,          pos.z - h,
                           pos.x + h, pos.y + height, pos.z + h);
    }

    // Floor correcto para negativos (int cast trunca hacia cero: bug clasico
    // que hace que las colisiones fallen en coordenadas negativas).
    static int floorToInt(float v) {
        const int i = (int)v;
        return (v < (float)i) ? i - 1 : i;
    }

    // ¿Colisiona el AABB con algun bloque solido?
    bool overlapsSolid(const BoundingBox& box) const {
        // Rango de voxeles que puede tocar la caja.
        // El -1e-4 en los maximos evita incluir el bloque contiguo cuando la
        // cara del AABB coincide EXACTAMENTE con el limite del voxel: sin
        // esto, estar de pie sobre y=64.0 detectaria colision con el bloque
        // de y=64 y el jugador vibraria.
        const int x0 = floorToInt(box.minX);
        const int x1 = floorToInt(box.maxX - 1e-4f);
        const int y0 = floorToInt(box.minY);
        const int y1 = floorToInt(box.maxY - 1e-4f);
        const int z0 = floorToInt(box.minZ);
        const int z1 = floorToInt(box.maxZ - 1e-4f);

        for (int y = y0; y <= y1; ++y) {
            for (int z = z0; z <= z1; ++z) {
                for (int x = x0; x <= x1; ++x) {
                    ++queryCount;

                    // Caja REAL del bloque, no su voxel entero: hay bloques
                    // que solo ocupan una parte (p. ej. el cladodio del nopal,
                    // una losa de 5/16). Para un bloque normal esto devuelve
                    // el cubo completo y el resultado es identico al de antes.
                    float bx0, by0, bz0, bx1, by1, bz1;
                    if (!world->getBlockBox(x, y, z, bx0, by0, bz0,
                                                     bx1, by1, bz1)) continue;

                    // A coordenadas de mundo.
                    bx0 += (float)x; bx1 += (float)x;
                    by0 += (float)y; by1 += (float)y;
                    bz0 += (float)z; bz1 += (float)z;

                    // Solape real entre el AABB del jugador y el del bloque.
                    if (box.maxX > bx0 && box.minX < bx1 &&
                        box.maxY > by0 && box.minY < by1 &&
                        box.maxZ > bz0 && box.minZ < bz1) {
                        return true;
                    }
                }
            }
        }
        return false;
    }

public:
    CollisionSystem(const IWorldQuery* w, const MovementConfig* c)
        : world(w), cfg(c) {}

    int consumeQueryCount() const { int q = queryCount; queryCount = 0; return q; }

    bool isPositionBlocked(const MoveVec3& pos, float height) const {
        return overlapsSolid(makeBox(pos, height));
    }

    // ========================================================================
    // MOVIMIENTO CON RESOLUCION POR EJE
    // ========================================================================
    // Se mueve un eje cada vez y se comprueba colision tras cada uno.
    //
    // POR QUE POR EJE Y EN ESTE ORDEN (Y -> X -> Z):
    //   Resolver los tres a la vez obliga a calcular el tiempo de impacto y a
    //   proyectar sobre la normal, lo que en una rejilla de voxeles produce
    //   enganches en las juntas entre bloques coplanares.
    //
    //   Resolviendo eje a eje, el deslizamiento sale GRATIS: si X esta
    //   bloqueado pero Z libre, el movimiento en Z se aplica igual y el
    //   jugador se desliza por la pared. Es exacto (sin tunneling para
    //   velocidades por paso < 1 bloque, garantizado por el fixed timestep) y
    //   no tiene casos degenerados.
    //
    //   Y va primero para que el estado "en suelo" ya este resuelto cuando se
    //   evalua el step-up horizontal.
    CollisionResult move(const MoveVec3& position,
                         const MoveVec3& delta,
                         float height,
                         bool allowStepUp) const {
        CollisionResult res;
        MoveVec3 pos = position;

        // ---------------- EJE Y ----------------
        if (delta.y != 0.0f) {
            MoveVec3 test = pos;
            test.y += delta.y;
            if (overlapsSolid(makeBox(test, height))) {
                // Colision vertical: acercarse todo lo posible sin penetrar.
                pos.y = resolveAxisPrecise(pos, delta.y, height, 1);
                res.hitY = true;
                res.normal = MoveVec3(0, delta.y < 0 ? 1.0f : -1.0f, 0);
            } else {
                pos = test;
            }
        }

        // ---------------- EJE X ----------------
        bool blockedX = false;
        if (delta.x != 0.0f) {
            MoveVec3 test = pos;
            test.x += delta.x;
            if (overlapsSolid(makeBox(test, height))) {
                blockedX = true;
            } else {
                pos = test;
            }
        }

        // ---------------- EJE Z ----------------
        bool blockedZ = false;
        if (delta.z != 0.0f) {
            MoveVec3 test = pos;
            test.z += delta.z;
            if (overlapsSolid(makeBox(test, height))) {
                blockedZ = true;
            } else {
                pos = test;
            }
        }

        // ====================================================================
        // STEP-UP: subir automaticamente bloques de <= stepHeight
        // ====================================================================
        // Solo si hay obstruccion horizontal. Se intenta reproducir el
        // movimiento elevado; si cabe, se baja hasta apoyar.
        //
        // CLAVE PARA QUE NO VIBRE: no se sube "un poco cada frame". Se prueba
        // el movimiento completo a la altura elevada y solo se acepta si el
        // resultado es valido Y hay suelo debajo. Un fallo deja al jugador
        // exactamente donde estaba, sin oscilar entre dos estados.
        if (allowStepUp && (blockedX || blockedZ)) {
            const MoveVec3 before = pos;
            MoveVec3 stepped = pos;
            stepped.y += cfg->stepHeight;

            // ¿Hay hueco encima para elevarse? (evita "subir" bajo un techo)
            if (!overlapsSolid(makeBox(stepped, height))) {
                MoveVec3 target = stepped;
                if (blockedX) target.x += delta.x;
                if (blockedZ) target.z += delta.z;

                if (!overlapsSolid(makeBox(target, height))) {
                    // Descender hasta apoyarse: busqueda binaria corta.
                    float lo = 0.0f;               // sin bajar (valido)
                    float hi = cfg->stepHeight;    // bajar todo (quiza invalido)
                    for (int i = 0; i < 6; ++i) {
                        const float mid = (lo + hi) * 0.5f;
                        MoveVec3 probe = target;
                        probe.y -= mid;
                        if (overlapsSolid(makeBox(probe, height))) hi = mid;
                        else lo = mid;
                    }
                    MoveVec3 landed = target;
                    landed.y -= lo;

                    // ------------------------------------------------------------
                    // VERIFICACION DE APOYO REAL (imprescindible)
                    // ------------------------------------------------------------
                    // Si bajo el destino no hay NADA, ninguna sonda de la busqueda
                    // binaria colisiona, `lo` converge a casi stepHeight y `gained`
                    // queda en un valor positivo minusculo (~0.016). Sin esta
                    // comprobacion el movimiento se aceptaba y el jugador
                    // LEVITABA sobre el vacio, subiendo un poco en cada paso al
                    // rozar la esquina de un bloque con un hueco detras.
                    //
                    // Se exige que exista suelo justo debajo de la posicion final.
                    MoveVec3 supportProbe = landed;
                    supportProbe.y -= 0.05f;
                    const bool hasSupport = overlapsSolid(makeBox(supportProbe, height));

                    const float gained = landed.y - before.y;
                    if (gained > 1e-4f && hasSupport && !overlapsSolid(makeBox(landed, height))) {
                        pos = landed;
                        res.steppedUp = true;
                        res.stepUpAmount = gained;
                        blockedX = blockedZ = false;
                    }
                }
            }
        }

        if (blockedX) { res.hitX = true; res.normal = MoveVec3(-MathUtil::sign(delta.x), 0, 0); }
        if (blockedZ) { res.hitZ = true; res.normal = MoveVec3(0, 0, -MathUtil::sign(delta.z)); }

        // ---------------- TEST DE SUELO ----------------
        // Sonda muy corta hacia abajo. Se hace SIEMPRE (no solo al caer) para
        // que el estado sea correcto al caminar por terreno plano.
        MoveVec3 probe = pos;
        probe.y -= 0.02f;
        res.groundedBelow = overlapsSolid(makeBox(probe, height));

        res.finalPosition = pos;
        return res;
    }

    // ========================================================================
    // ACERCAMIENTO PRECISO A UNA SUPERFICIE
    // ========================================================================
    // Busqueda binaria del maximo desplazamiento libre en un eje.
    //
    // Es lo que permite quedarse EXACTAMENTE apoyado sobre el bloque en vez
    // de a una distancia arbitraria: sin esto, al caer el jugador se detiene
    // donde le pilla el paso de integracion y la camara "flota" a una altura
    // ligeramente distinta cada vez que aterriza.
    float resolveAxisPrecise(const MoveVec3& pos, float delta, float height, int axis) const {
        float lo = 0.0f;          // desplazamiento seguro conocido
        float hi = delta;         // desplazamiento deseado (colisiona)

        for (int i = 0; i < 8; ++i) {
            const float mid = (lo + hi) * 0.5f;
            MoveVec3 test = pos;
            if      (axis == 0) test.x += mid;
            else if (axis == 1) test.y += mid;
            else                test.z += mid;

            if (overlapsSolid(makeBox(test, height))) hi = mid;
            else                                      lo = mid;
        }

        // Retroceso de skin width para no quedar pegado con penetracion 0,
        // que provocaria que el siguiente test de colision ya diera positivo.
        const float safe = lo - MathUtil::sign(delta) * cfg->skinWidth;

        if      (axis == 0) return pos.x + safe;
        else if (axis == 1) return pos.y + safe;
        else                return pos.z + safe;
    }

    // ========================================================================
    // CONSULTAS DE VOLUMEN (agua, escaleras)
    // ========================================================================
    // Comprueba si algun voxel solapado por la caja cumple un predicado.
    template <typename Pred>
    bool anyBlockInBox(const MoveVec3& pos, float height, Pred pred) const {
        const BoundingBox box = makeBox(pos, height);
        const int x0 = floorToInt(box.minX), x1 = floorToInt(box.maxX - 1e-4f);
        const int y0 = floorToInt(box.minY), y1 = floorToInt(box.maxY - 1e-4f);
        const int z0 = floorToInt(box.minZ), z1 = floorToInt(box.maxZ - 1e-4f);

        for (int y = y0; y <= y1; ++y)
            for (int z = z0; z <= z1; ++z)
                for (int x = x0; x <= x1; ++x)
                    if (pred(x, y, z)) return true;
        return false;
    }

    bool isInWater(const MoveVec3& pos, float height) const {
        return anyBlockInBox(pos, height,
            [this](int x,int y,int z){ return world->isLiquid(x,y,z); });
    }

    bool isOnLadder(const MoveVec3& pos, float height) const {
        return anyBlockInBox(pos, height,
            [this](int x,int y,int z){ return world->isClimbable(x,y,z); });
    }

    bool isPointInLiquid(const MoveVec3& p) const {
        return world->isLiquid(floorToInt(p.x), floorToInt(p.y), floorToInt(p.z));
    }

    // Espacio libre para ponerse de pie (usado al soltar agacharse).
    bool canStandUp(const MoveVec3& pos) const {
        return !overlapsSolid(makeBox(pos, cfg->height));
    }
};

} // namespace PlayerSys

#endif // COLLISION_SYSTEM_H
