#!/usr/bin/env python3
# -*- coding: utf-8 -*-

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Encontrar propagateTorchlight
start_idx = None
for i, line in enumerate(lines):
    if 'void propagateTorchlight()' in line:
        start_idx = i - 1
        break

if start_idx is None:
    print("ERROR: No se encontro propagateTorchlight()")
    exit(1)

# Encontrar donde termina
end_idx = None
for i in range(start_idx + 10, len(lines)):
    if 'Torchlight propagation completada!' in lines[i]:
        for j in range(i, min(i + 5, len(lines))):
            if lines[j].strip() == '}':
                end_idx = j + 1
                break
        break

if end_idx is None:
    print("ERROR: No se encontro el fin de propagateTorchlight()")
    exit(1)

print(f"Reemplazando lineas {start_idx+1} a {end_idx}")

new_function = """    // Propagar TORCHLIGHT (luz de antorchas y bloques emisores)
    void propagateTorchlight() {
        std::cout << "Propagando torchlight (OPTIMIZADO)..." << std::endl;

        std::queue<LightNode> lightQueue;
        std::unordered_set<int64_t> visited;

        auto hashPos = [](int x, int y, int z) -> int64_t {
            return ((int64_t)x << 32) | ((int64_t)y << 16) | (int64_t)z;
        };

        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        BlockType block = chunk->getBlock(x, y, z);
                        EmissiveBlock emission = getBlockEmission(block);

                        if (emission.light > 0) {
                            int worldX = chunk->position.x * CHUNK_SIZE + x;
                            int worldZ = chunk->position.z * CHUNK_SIZE + z;

                            chunk->setTorchlight(x, y, z, emission.light,
                                               emission.r, emission.g, emission.b);

                            int64_t hash = hashPos(worldX, y, worldZ);
                            lightQueue.push(LightNode(worldX, y, worldZ, emission.light));
                            visited.insert(hash);
                        }
                    }
                }
            }
        }

        int iterations = 0;
        const int MAX_ITERATIONS = 500000;

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

                int64_t hash = hashPos(nx, ny, nz);
                if (visited.count(hash)) continue;

                BlockType neighborBlock = getBlock(nx, ny, nz);
                if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

                uint8_t currentTorchlight = getTorchlight(nx, ny, nz);
                uint8_t newLight = node.lightLevel - 1;

                if (newLight > currentTorchlight) {
                    setTorchlight(nx, ny, nz, newLight, 3, 2, 1);
                    lightQueue.push(LightNode(nx, ny, nz, newLight));
                    visited.insert(hash);
                }
            }

            iterations++;
        }

        std::cout << "Torchlight propagation completada! (" << iterations << " iterations)" << std::endl;
    }
"""

new_lines = lines[:start_idx] + [new_function + '\n'] + lines[end_idx:]

with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(new_lines)

print("OK: propagateTorchlight optimizada!")
print("  - Agregado visited set")
print("  - Limite de 500K iterations")
