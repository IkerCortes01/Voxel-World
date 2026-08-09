#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Replace the chunk generation code to use the new public methods
old_code = """    // Generar chunks iniciales alrededor del origen (0,0,0)
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

new_code = """    // Generar chunks iniciales alrededor del origen (0,0,0)
    std::cout << "Generando mundo inicial alrededor del origen..." << std::endl;

    // Generar 7x7 = 49 chunks (radio de 3)
    g_gameState->world.generateInitialChunks(3);

    // Construir todos los meshes
    g_gameState->world.buildAllPendingMeshes();

    std::cout << "Mundo inicial generado! (" << g_gameState->world.getChunkCount() << " chunks)" << std::endl;"""

content = content.replace(old_code, new_code)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Codigo de main() actualizado para usar metodos publicos!")
