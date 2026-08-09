#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
MINECRAFT-STYLE INCREMENTAL LIGHTING SYSTEM
- Ilumina chunks individuales en lugar de todo el mundo
- Queue de chunks pendientes
- Procesa 2-3 chunks por frame (no bloquea)
- Propagación LOCAL (solo chunk + vecinos)
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Buscar donde está la clase World
import re

# 1. Agregar queue de lighting en World class
# Buscar la declaración de lightingThread
old_lighting_vars = r'(std::thread\* lightingThread = nullptr;)'
new_lighting_vars = r'\1\n    std::queue<Vec3i> lightingQueue;  // Queue de chunks pendientes de iluminacion\n    std::mutex lightingQueueMutex;  // Proteger la queue'

content = re.sub(old_lighting_vars, new_lighting_vars, content)

# 2. Agregar #include <mutex> al inicio
if '#include <mutex>' not in content:
    content = content.replace('#include <unordered_set>', '#include <unordered_set>\n#include <mutex>')

# 3. Crear función lightChunk (iluminar UN chunk)
# Buscar donde termina propagateTorchlight y agregar la nueva función
new_light_chunk_function = """
    // MINECRAFT-STYLE: Iluminar UN CHUNK individual (incremental)
    void lightChunk(Chunk* chunk) {
        if (!chunk || !chunk->isGenerated) return;

        const int cx = chunk->position.x;
        const int cz = chunk->position.z;

        // PASO 1: Calcular SKYLIGHT vertical para este chunk
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                uint8_t currentLight = 18;

                for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
                    BlockType block = chunk->getBlock(x, y, z);

                    if (block == BLOCK_AIR || block == BLOCK_WATER) {
                        chunk->setSunlight(x, y, z, currentLight);
                    } else {
                        chunk->setSunlight(x, y, z, 0);
                        currentLight = 0;
                    }
                }
            }
        }

        // PASO 2: Propagar luz HORIZONTAL (solo dentro del chunk + 1 bloque vecino)
        std::queue<LightNode> lightQueue;
        std::unordered_set<int64_t> visited;

        auto hashPos = [](int x, int y, int z) -> int64_t {
            return ((int64_t)x << 32) | ((int64_t)y << 16) | (int64_t)z;
        };

        // Agregar fuentes de luz del chunk
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t sunlight = chunk->getSunlight(x, y, z);
                    if (sunlight > 1) {
                        int worldX = cx * CHUNK_SIZE + x;
                        int worldZ = cz * CHUNK_SIZE + z;
                        int64_t hash = hashPos(worldX, y, worldZ);
                        lightQueue.push(LightNode(worldX, y, worldZ, sunlight));
                        visited.insert(hash);
                    }
                }
            }
        }

        // BFS propagation (limitado a 50K iterations por chunk)
        int iterations = 0;
        const int MAX_ITERATIONS = 50000;

        while (!lightQueue.empty() && iterations < MAX_ITERATIONS) {
            LightNode node = lightQueue.front();
            lightQueue.pop();

            if (node.lightLevel <= 1) continue;

            int dx[] = {0, 0, 0, 0, 1, -1};
            int dy[] = {1, -1, 0, 0, 0, 0};
            int dz[] = {0, 0, 1, -1, 0, 0};

            for (int i = 0; i < 6; i++) {
                int nx = node.x + dx[i];
                int ny = node.y + dy[i];
                int nz = node.z + dz[i];

                if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

                // Limitar propagación a chunk actual + 1 bloque en vecinos
                int localX = nx - cx * CHUNK_SIZE;
                int localZ = nz - cz * CHUNK_SIZE;
                if (localX < -1 || localX > CHUNK_SIZE || localZ < -1 || localZ > CHUNK_SIZE) continue;

                int64_t hash = hashPos(nx, ny, nz);
                if (visited.count(hash)) continue;

                BlockType neighborBlock = getBlock(nx, ny, nz);
                if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

                uint8_t currentSunlight = getSunlight(nx, ny, nz);
                uint8_t newLight = node.lightLevel - 1;

                if (newLight > currentSunlight) {
                    setSunlight(nx, ny, nz, newLight);
                    lightQueue.push(LightNode(nx, ny, nz, newLight));
                    visited.insert(hash);
                }
            }

            iterations++;
        }

        chunk->needsRebuild = true;
        chunk->needsLightUpdate = false;
    }

    // Procesar queue de iluminación (llamar cada frame)
    void processLightingQueue() {
        std::lock_guard<std::mutex> lock(lightingQueueMutex);

        // Procesar hasta 3 chunks por frame (balance entre FPS y velocidad de iluminación)
        int chunksProcessed = 0;
        const int MAX_CHUNKS_PER_FRAME = 3;

        while (!lightingQueue.empty() && chunksProcessed < MAX_CHUNKS_PER_FRAME) {
            Vec3i chunkPos = lightingQueue.front();
            lightingQueue.pop();

            Chunk* chunk = getChunk(chunkPos);
            if (chunk && chunk->isGenerated && chunk->needsLightUpdate) {
                lightChunk(chunk);
                chunksProcessed++;
            }
        }
    }

    // Agregar chunk a la queue de iluminación
    void queueChunkForLighting(Vec3i chunkPos) {
        std::lock_guard<std::mutex> lock(lightingQueueMutex);
        lightingQueue.push(chunkPos);
    }
"""

# Buscar donde insertar (después de propagateTorchlight)
pattern = r'(Torchlight propagation completada.*?\n    \})'
replacement = r'\1\n' + new_light_chunk_function

content = re.sub(pattern, replacement, content, flags=re.DOTALL)

# 4. Modificar generateChunk para agregar chunk a la queue automáticamente
old_generate_chunk_end = r'(chunk->isGenerated = true;)'
new_generate_chunk_end = r'\1\n        chunk->needsLightUpdate = true;  // Marcar para iluminación\n        queueChunkForLighting(chunk->position);  // Agregar a queue'

content = re.sub(old_generate_chunk_end, new_generate_chunk_end, content)

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("MINECRAFT-STYLE LIGHTING implementado!")
print("")
print("Cambios:")
print("  1. lightChunk() - Ilumina UN chunk (max 50K iterations)")
print("  2. lightingQueue - Queue de chunks pendientes")
print("  3. processLightingQueue() - Procesa 3 chunks/frame")
print("  4. Auto-queue cuando se genera un chunk")
print("")
print("Beneficios:")
print("  - De 1.5M iterations -> 50K per chunk")
print("  - No bloquea el game loop")
print("  - FPS estables mientras ilumina")
