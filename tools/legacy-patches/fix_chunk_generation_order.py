#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the chunk generation section
old_generation = """    // Generar chunks iniciales alrededor del jugador
    std::cout << "Generando mundo inicial..." << std::endl;
    g_gameState->world.updateChunks(g_gameState->player.position);"""

new_generation = """    // Generar chunks iniciales alrededor del origen (0,0,0)
    std::cout << "Generando mundo inicial alrededor del origen..." << std::endl;

    // Generar chunks en un radio de 3 chunks alrededor del origen
    for (int cx = -3; cx <= 3; cx++) {
        for (int cz = -3; cz <= 3; cz++) {
            Vec3i chunkPos(cx, 0, cz);
            g_gameState->world.getOrCreateChunk(chunkPos);
        }
    }

    // Reconstruir meshes de todos los chunks generados
    std::cout << "Construyendo meshes iniciales..." << std::endl;
    for (auto& pair : g_gameState->world.chunks) {
        if (pair.second->needsRebuild && pair.second->isGenerated) {
            g_gameState->world.buildChunkMesh(pair.second);
        }
    }

    std::cout << "Mundo inicial generado! (" << g_gameState->world.chunks.size() << " chunks)" << std::endl;"""

content = content.replace(old_generation, new_generation)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Sistema de generacion de chunks mejorado!")
print("\nCambios:")
print("  - Genera 7x7 = 49 chunks alrededor del origen ANTES de buscar spawn")
print("  - Construye todos los meshes antes de buscar spawn")
print("  - Garantiza que el terreno este listo para la busqueda")
