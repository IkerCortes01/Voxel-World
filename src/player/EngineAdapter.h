#ifndef ENGINE_ADAPTER_H
#define ENGINE_ADAPTER_H

#include "PlayerController.h"
#include "PlayerDebug.h"

// ============================================================================
// ENGINE ADAPTER - Puente entre el motor y el sistema de movimiento
// ============================================================================
// PROBLEMA QUE RESUELVE:
//   El motor tiene ~190 usos de `player.position`, `player.velocity`,
//   `player.yaw`, etc. repartidos por main.cpp (raycast, HUD, guardado,
//   render de la mano, niebla...). Reescribir todos seria arriesgado y no
//   aportaria nada.
//
// SOLUCION (patron Adapter):
//   El PlayerController es la FUENTE DE VERDAD de la simulacion. Tras cada
//   update se COPIA su estado al struct Player heredado, de modo que todo el
//   codigo existente sigue funcionando sin cambios mientras la fisica real la
//   lleva el sistema nuevo.
//
//   Es una sincronizacion unidireccional (controller -> Player) salvo en los
//   puntos donde el motor teletransporta al jugador (spawn, cargar partida),
//   donde se hace la sincronizacion inversa explicitamente con pullFromEngine.
//
// El adaptador es el UNICO archivo del modulo que conoce los tipos del motor,
// y lo hace por plantilla para no incluir main.cpp aqui.
// ============================================================================

namespace PlayerSys {

// ----------------------------------------------------------------------------
// IMPLEMENTACION DE IWorldQuery SOBRE EL MUNDO DEL MOTOR
// ----------------------------------------------------------------------------
// Template para no depender del tipo World concreto (evita dependencia
// circular con main.cpp).
//
// TWorld debe ofrecer: BlockType getBlock(int,int,int)
template <typename TWorld, typename TBlockEnum>
class EngineWorldQuery : public IWorldQuery {
private:
    TWorld* world;

    // Predicados inyectados: el motor decide que es solido/liquido/escalable.
    bool (*solidFn)(TBlockEnum);
    TBlockEnum waterBlock;
    TBlockEnum lavaBlock;
    TBlockEnum ladderBlock;
    bool hasLadderBlock;

public:
    EngineWorldQuery(TWorld* w,
                     bool (*isSolidFn)(TBlockEnum),
                     TBlockEnum water,
                     TBlockEnum lava,
                     TBlockEnum ladder,
                     bool ladderExists)
        : world(w), solidFn(isSolidFn),
          waterBlock(water), lavaBlock(lava),
          ladderBlock(ladder), hasLadderBlock(ladderExists) {}

    void setWorld(TWorld* w) { world = w; }

    bool isSolid(int x, int y, int z) const override {
        if (!world) return false;
        return solidFn(world->getBlock(x, y, z));
    }

    bool isLiquid(int x, int y, int z) const override {
        if (!world) return false;
        const TBlockEnum b = world->getBlock(x, y, z);
        return b == waterBlock || b == lavaBlock;
    }

    bool isClimbable(int x, int y, int z) const override {
        if (!world || !hasLadderBlock) return false;
        return world->getBlock(x, y, z) == ladderBlock;
    }
};

// ----------------------------------------------------------------------------
// SINCRONIZACION CON EL STRUCT Player DEL MOTOR
// ----------------------------------------------------------------------------
// TPlayer debe tener: position{x,y,z}, velocity{x,y,z}, yaw, pitch,
//                     onGround, isInWater, isUnderwater, isFlying
template <typename TPlayer>
inline void pushToEngine(const PlayerController& pc, TPlayer& player) {
    const MovementState& s = pc.getState();

    // POSICION AUTORITATIVA (no interpolada).
    //
    // Se copia la posicion REAL de la simulacion, no la interpolada de
    // render. Motivo: el motor usa player.position para cosas que exigen el
    // valor exacto — guardar la partida, el raycast de romper/colocar
    // bloques, la carga de chunks y el rescate anti-void. Escribir ahi la
    // posicion interpolada introducia un desfase de hasta
    // sprintSpeed/120 ≈ 0.047 bloques entre lo que el jugador ve y lo que el
    // juego calcula, y guardaba en disco una posicion que nunca fue real.
    //
    // La interpolacion se aplica UNICAMENTE donde debe: en la camara, via
    // PlayerController::getEyePosition(), que es lo que consume el render.
    player.position.x = s.position.x;
    player.position.y = s.position.y;
    player.position.z = s.position.z;

    player.velocity.x = s.velocity.x;
    player.velocity.y = s.velocity.y;
    player.velocity.z = s.velocity.z;

    player.yaw   = s.yaw;
    player.pitch = s.pitch;

    player.onGround    = s.onGround;
    player.isInWater   = s.isInWater;
    player.isUnderwater= s.isSubmerged;
    player.isFlying    = s.isFlying;
}

// Sincronizacion INVERSA: el motor movio al jugador (spawn, carga de mundo,
// teletransporte anti-void). El controlador debe adoptar esa posicion.
template <typename TPlayer>
inline void pullFromEngine(PlayerController& pc, const TPlayer& player) {
    pc.setPosition(player.position.x, player.position.y, player.position.z);
    pc.setRotation(player.yaw, player.pitch);
}

} // namespace PlayerSys

#endif // ENGINE_ADAPTER_H
