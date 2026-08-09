#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find and replace the spawn system
old_spawn = """    // Encontrar altura del terreno en la posición de spawn
    int terrainHeight = 0;
    for (int y = 127; y >= 0; y--) {
        BlockType block = g_gameState->world.getBlock(0, y, 0);
        if (block != BLOCK_AIR) {
            terrainHeight = y + 1;
            break;
        }
    }

    // Colocar al jugador sobre el terreno con un pequeño offset
    if (terrainHeight > 0) {
        g_gameState->player.position.y = terrainHeight + 2.0f;
    }"""

new_spawn = """    // SISTEMA DE SPAWN SEGURO - El jugador SIEMPRE aparece en superficie
    std::cout << "Buscando posicion de spawn segura..." << std::endl;

    bool foundSafeSpawn = false;
    int spawnX = 0;
    int spawnZ = 0;
    int spawnY = 0;

    // Buscar en espiral desde el centro
    for (int radius = 0; radius <= 32 && !foundSafeSpawn; radius++) {
        for (int dx = -radius; dx <= radius && !foundSafeSpawn; dx++) {
            for (int dz = -radius; dz <= radius && !foundSafeSpawn; dz++) {
                // Solo buscar en el borde del radio actual (optimización)
                if (abs(dx) != radius && abs(dz) != radius) continue;

                int testX = dx;
                int testZ = dz;

                // Buscar la superficie desde arriba
                for (int y = CHUNK_HEIGHT - 1; y >= 1; y--) {
                    BlockType currentBlock = g_gameState->world.getBlock(testX, y, testZ);
                    BlockType blockBelow = g_gameState->world.getBlock(testX, y - 1, testZ);
                    BlockType blockAbove = g_gameState->world.getBlock(testX, y + 1, testZ);
                    BlockType blockAbove2 = g_gameState->world.getBlock(testX, y + 2, testZ);

                    // Condiciones para un spawn seguro:
                    // 1. Bloque actual es AIRE (no spawneamos dentro de bloques)
                    // 2. Bloque debajo es SÓLIDO (no AIRE, no AGUA)
                    // 3. Bloque encima es AIRE (espacio para la cabeza)
                    // 4. 2 bloques encima es AIRE (espacio completo)

                    bool currentIsAir = (currentBlock == BLOCK_AIR);
                    bool belowIsSolid = (blockBelow != BLOCK_AIR && blockBelow != BLOCK_WATER);
                    bool aboveIsAir = (blockAbove == BLOCK_AIR);
                    bool above2IsAir = (blockAbove2 == BLOCK_AIR);

                    // NO spawneamos en agua
                    bool notInWater = (currentBlock != BLOCK_WATER &&
                                      blockAbove != BLOCK_WATER &&
                                      blockAbove2 != BLOCK_WATER);

                    if (currentIsAir && belowIsSolid && aboveIsAir && above2IsAir && notInWater) {
                        // Verificar que NO sea una cueva (debe tener cielo encima)
                        bool hasSky = true;
                        for (int checkY = y + 3; checkY < CHUNK_HEIGHT; checkY++) {
                            BlockType skyBlock = g_gameState->world.getBlock(testX, checkY, testZ);
                            if (skyBlock != BLOCK_AIR && skyBlock != BLOCK_WATER) {
                                hasSky = false;
                                break;
                            }
                        }

                        if (hasSky) {
                            // ENCONTRADO! Posición segura
                            spawnX = testX;
                            spawnY = y;
                            spawnZ = testZ;
                            foundSafeSpawn = true;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (foundSafeSpawn) {
        g_gameState->player.position.x = spawnX + 0.5f;
        g_gameState->player.position.y = spawnY + 0.1f;  // Pequeño offset para estar sobre el bloque
        g_gameState->player.position.z = spawnZ + 0.5f;
        std::cout << "Spawn seguro encontrado en: X=" << spawnX
                  << ", Y=" << spawnY << ", Z=" << spawnZ << std::endl;
    } else {
        // Fallback: buscar cualquier superficie (sin verificación de cielo)
        std::cout << "ADVERTENCIA: No se encontro spawn ideal, usando fallback..." << std::endl;
        for (int y = CHUNK_HEIGHT - 1; y >= 10; y--) {
            BlockType current = g_gameState->world.getBlock(0, y, 0);
            BlockType below = g_gameState->world.getBlock(0, y - 1, 0);

            if (current == BLOCK_AIR && below != BLOCK_AIR && below != BLOCK_WATER) {
                g_gameState->player.position.y = y + 0.1f;
                std::cout << "Spawn fallback en Y=" << y << std::endl;
                break;
            }
        }
    }"""

content = content.replace(old_spawn, new_spawn)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Sistema de spawn seguro implementado!")
print("\nCaracteristicas:")
print("  - Busca en espiral desde el centro (radio 0 a 32)")
print("  - Verifica bloque actual = AIRE")
print("  - Verifica bloque debajo = SOLIDO (no agua)")
print("  - Verifica 2 bloques encima = AIRE (espacio completo)")
print("  - Verifica NO estar en agua")
print("  - Verifica tener cielo encima (no spawn en cuevas)")
print("  - Fallback si no encuentra posicion ideal")
