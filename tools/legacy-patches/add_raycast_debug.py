#!/usr/bin/env python3
# Script para añadir debug visual del raycast

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# AÑADIR INFORMACIÓN DE DEBUG EN PANTALLA
# ============================================================================
print("[INFO] Anadiendo informacion de debug del raycast...")

# Buscar donde se renderiza el hotbar y añadir info de debug antes
debug_info = '''
            // ============ DEBUG: Información del raycast ============
            Vec3 debugRayOrigin = g_gameState->player.getEyePosition();
            Vec3 debugRayDirection = g_gameState->player.getForward();
            RaycastResult debugRayResult = raycastBlock(g_gameState->world, debugRayOrigin, debugRayDirection, 5.0f);

            // Mostrar información de debug en esquina superior izquierda
            glColor4f(1.0f, 1.0f, 0.0f, 1.0f);  // Amarillo
            char debugText[256];

            // Posición del jugador
            sprintf(debugText, "Pos: (%.1f, %.1f, %.1f)",
                g_gameState->player.position.x,
                g_gameState->player.position.y,
                g_gameState->player.position.z);
            renderText(debugText, 10, 10, 12);

            // Dirección del raycast (normalizada)
            sprintf(debugText, "Dir: (%.2f, %.2f, %.2f)",
                debugRayDirection.x,
                debugRayDirection.y,
                debugRayDirection.z);
            renderText(debugText, 10, 30, 12);

            // Ángulos de cámara
            sprintf(debugText, "Yaw: %.1f  Pitch: %.1f",
                g_gameState->player.yaw,
                g_gameState->player.pitch);
            renderText(debugText, 10, 50, 12);

            // Bloque detectado
            if (debugRayResult.hit) {
                BlockType blockType = g_gameState->world.getBlock(
                    debugRayResult.blockPos.x,
                    debugRayResult.blockPos.y,
                    debugRayResult.blockPos.z
                );

                const char* blockName = "DESCONOCIDO";
                switch(blockType) {
                    case BLOCK_DIRT: blockName = "TIERRA"; break;
                    case BLOCK_GRASS: blockName = "CESPED"; break;
                    case BLOCK_STONE: blockName = "PIEDRA"; break;
                    case BLOCK_WOOD: blockName = "MADERA"; break;
                    case BLOCK_LEAVES: blockName = "HOJAS"; break;
                    case BLOCK_SAND: blockName = "ARENA"; break;
                    case BLOCK_BEDROCK: blockName = "BEDROCK"; break;
                    case BLOCK_WATER: blockName = "AGUA"; break;
                    default: break;
                }

                glColor4f(0.0f, 1.0f, 0.0f, 1.0f);  // Verde = detectado
                sprintf(debugText, "BLOQUE: %s (%d, %d, %d)",
                    blockName,
                    debugRayResult.blockPos.x,
                    debugRayResult.blockPos.y,
                    debugRayResult.blockPos.z);
                renderText(debugText, 10, 70, 12);

                sprintf(debugText, "Dist: %.2f bloques", debugRayResult.distance);
                renderText(debugText, 10, 90, 12);
            } else {
                glColor4f(1.0f, 0.0f, 0.0f, 1.0f);  // Rojo = no detectado
                sprintf(debugText, "BLOQUE: NINGUNO (aire/lejos)");
                renderText(debugText, 10, 70, 12);
            }

            // Indicador de dirección cardinal
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
            float yaw = g_gameState->player.yaw;
            const char* direction = "";

            // Normalizar yaw a 0-360
            while (yaw < 0) yaw += 360;
            while (yaw >= 360) yaw -= 360;

            if (yaw >= 337.5f || yaw < 22.5f) direction = "SUR";
            else if (yaw >= 22.5f && yaw < 67.5f) direction = "SUR-OESTE";
            else if (yaw >= 67.5f && yaw < 112.5f) direction = "OESTE";
            else if (yaw >= 112.5f && yaw < 157.5f) direction = "NOR-OESTE";
            else if (yaw >= 157.5f && yaw < 202.5f) direction = "NORTE";
            else if (yaw >= 202.5f && yaw < 247.5f) direction = "NOR-ESTE";
            else if (yaw >= 247.5f && yaw < 292.5f) direction = "ESTE";
            else if (yaw >= 292.5f && yaw < 337.5f) direction = "SUR-ESTE";

            sprintf(debugText, "Mirando: %s", direction);
            renderText(debugText, 10, 110, 12);

            // Indicador vertical
            float pitch = g_gameState->player.pitch;
            const char* vertical = "";
            if (pitch < -45) vertical = "ARRIBA";
            else if (pitch > 45) vertical = "ABAJO";
            else vertical = "HORIZONTAL";

            sprintf(debugText, "Vertical: %s", vertical);
            renderText(debugText, 10, 130, 12);
            // ============ FIN DEBUG ============

'''

# Buscar el lugar donde se renderiza el hotbar
hotbar_pattern = r'(            // Renderizar hotbar\n            renderHotbar\(&g_gameState->inventory, width, height\);)'

if re.search(hotbar_pattern, content):
    content = re.sub(hotbar_pattern, debug_info + r'\1', content)
    print("   [OK] Debug info anadida antes del hotbar")
else:
    print("   [WARNING] No se encontro el patron del hotbar, buscando alternativa...")

    # Buscar patrón alternativo
    alt_pattern = r'(        if \(!g_gameState->isPaused\) \{[^}]*?renderHotbar)'

    if re.search(alt_pattern, content, re.DOTALL):
        # Insertar antes del if
        insert_pos = content.find('            // Renderizar hotbar')
        if insert_pos != -1:
            content = content[:insert_pos] + debug_info + content[insert_pos:]
            print("   [OK] Debug info anadida (patron alternativo)")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Debug visual del raycast anadido!")
print("\nAhora veras en pantalla:")
print("  - Posicion del jugador (X, Y, Z)")
print("  - Direccion del raycast (X, Y, Z)")
print("  - Angulos de camara (Yaw, Pitch)")
print("  - Bloque detectado y su posicion")
print("  - Distancia al bloque")
print("  - Direccion cardinal (Norte, Sur, Este, Oeste, etc.)")
print("  - Orientacion vertical (Arriba, Abajo, Horizontal)")
print("\nEsto te permite verificar que el raycast funciona en TODAS direcciones")
print("\nCompila: cmake --build build --config Release")
