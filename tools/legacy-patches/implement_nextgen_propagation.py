#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the lighting system section and replace it
old_lighting_section_marker = "    // Obtener emisión de luz de un bloque\n    int getBlockEmission(BlockType block) {"

if old_lighting_section_marker not in content:
    print("ERROR: Could not find lighting section")
    exit(1)

# Find the start of getBlockEmission
start_idx = content.find(old_lighting_section_marker)

# Find the end of calculateWorldLightingThreaded (before startLightingCalculation)
end_marker = "    // Calcular iluminación en un hilo separado\n    void startLightingCalculation() {"
end_idx = content.find(end_marker)

if start_idx == -1 or end_idx == -1:
    print(f"ERROR: start_idx={start_idx}, end_idx={end_idx}")
    exit(1)

# New next-gen lighting system
new_lighting_code = """    // ========================================================================
    // NEXT-GEN LIGHTING - EMISSIVE BLOCKS WITH RGB COLOR
    // ========================================================================

    struct EmissiveBlock {
        uint8_t light;  // 0-18
        uint8_t r, g, b; // 0-3
    };

    EmissiveBlock getBlockEmission(BlockType block) {
        EmissiveBlock emission = {0, 0, 0, 0};

        switch (block) {
            // Futuro: bloques emisores con color
            case BLOCK_STONE: // Placeholder para GLOWSTONE
                emission = {15, 3, 3, 2}; // Luz blanca-amarilla
                break;

            // case BLOCK_TORCH:
            //     emission = {14, 3, 2, 1}; // Luz naranja
            //     break;

            // case BLOCK_LAVA:
            //     emission = {15, 3, 1, 0}; // Luz roja-naranja
            //     break;

            // case BLOCK_BLUE_CRYSTAL:
            //     emission = {12, 0, 2, 3}; // Luz azul
            //     break;

            default:
                break;
        }

        return emission;
    }

    // ========================================================================
    // SKYLIGHT SYSTEM - Propagación vertical desde el cielo
    // ========================================================================

    void calculateSkylight() {
        std::cout << "Calculando skylight (luz solar vertical)..." << std::endl;

        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            // Para cada columna (x, z)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int worldX = chunk->position.x * CHUNK_SIZE + x;
                    int worldZ = chunk->position.z * CHUNK_SIZE + z;

                    // Empezar con luz máxima del sol
                    uint8_t currentLight = 18;

                    // Propagación vertical desde arriba
                    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
                        BlockType block = chunk->getBlock(x, y, z);

                        if (block == BLOCK_AIR || block == BLOCK_WATER) {
                            // Bloque transparente: mantener luz
                            chunk->setSunlight(x, y, z, currentLight);
                        } else {
                            // Bloque sólido: detener luz solar
                            chunk->setSunlight(x, y, z, 0);
                            currentLight = 0; // Debajo de bloques sólidos = oscuro
                        }
                    }
                }
            }
        }

        std::cout << "Skylight completado!" << std::endl;
    }

    // ========================================================================
    // BFS FLOOD-FILL PROPAGATION - Para sunlight Y torchlight
    // ========================================================================

    struct LightNode {
        int x, y, z;
        uint8_t lightLevel;

        LightNode(int _x, int _y, int _z, uint8_t _light)
            : x(_x), y(_y), z(_z), lightLevel(_light) {}
    };

    // Propagar SUNLIGHT horizontalmente (después de skylight vertical)
    void propagateSunlight() {
        std::cout << "Propagando sunlight horizontal..." << std::endl;

        std::queue<LightNode> lightQueue;

        // Añadir todas las fuentes de luz solar
        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        uint8_t sunlight = chunk->getSunlight(x, y, z);
                        if (sunlight > 0) {
                            int worldX = chunk->position.x * CHUNK_SIZE + x;
                            int worldZ = chunk->position.z * CHUNK_SIZE + z;
                            lightQueue.push(LightNode(worldX, y, worldZ, sunlight));
                        }
                    }
                }
            }
        }

        // BFS propagation
        int iterations = 0;
        while (!lightQueue.empty()) {
            LightNode node = lightQueue.front();
            lightQueue.pop();

            if (node.lightLevel <= 1) continue;

            // 6 direcciones
            int dx[] = {0, 0, 0, 0, 1, -1};
            int dy[] = {1, -1, 0, 0, 0, 0};
            int dz[] = {0, 0, 1, -1, 0, 0};

            for (int i = 0; i < 6; i++) {
                int nx = node.x + dx[i];
                int ny = node.y + dy[i];
                int nz = node.z + dz[i];

                if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

                BlockType neighborBlock = getBlock(nx, ny, nz);
                if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

                uint8_t currentSunlight = getSunlight(nx, ny, nz);
                uint8_t newLight = node.lightLevel - 1;

                if (newLight > currentSunlight) {
                    setSunlight(nx, ny, nz, newLight);
                    lightQueue.push(LightNode(nx, ny, nz, newLight));
                }
            }

            iterations++;
            if (iterations % 50000 == 0) {
                std::cout << "  Sunlight iterations: " << iterations << std::endl;
            }
        }

        std::cout << "Sunlight propagation completada! (" << iterations << " iterations)" << std::endl;
    }

    // Propagar TORCHLIGHT (luz de antorchas y bloques emisores)
    void propagateTorchlight() {
        std::cout << "Propagando torchlight..." << std::endl;

        std::queue<LightNode> lightQueue;

        // Añadir bloques emisores como fuentes
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

                            lightQueue.push(LightNode(worldX, y, worldZ, emission.light));
                        }
                    }
                }
            }
        }

        // BFS propagation
        int iterations = 0;
        while (!lightQueue.empty()) {
            LightNode node = lightQueue.front();
            lightQueue.pop();

            if (node.lightLevel <= 1) continue;

            // 6 direcciones
            int dx[] = {0, 0, 0, 0, 1, -1};
            int dy[] = {1, -1, 0, 0, 0, 0};
            int dz[] = {0, 0, 1, -1, 0, 0};

            for (int i = 0; i < 6; i++) {
                int nx = node.x + dx[i];
                int ny = node.y + dy[i];
                int nz = node.z + dz[i];

                if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

                BlockType neighborBlock = getBlock(nx, ny, nz);
                if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

                uint8_t currentTorchlight = getTorchlight(nx, ny, nz);
                uint8_t newLight = node.lightLevel - 1;

                if (newLight > currentTorchlight) {
                    // Obtener color de la fuente
                    float r, g, b;
                    getLightColor(node.x, node.y, node.z, r, g, b);

                    setTorchlight(nx, ny, nz, newLight,
                                (uint8_t)(r * 3), (uint8_t)(g * 3), (uint8_t)(b * 3));

                    lightQueue.push(LightNode(nx, ny, nz, newLight));
                }
            }

            iterations++;
        }

        std::cout << "Torchlight propagation completada! (" << iterations << " iterations)" << std::endl;
    }

    // ========================================================================
    // SISTEMA COMPLETO - Skylight + Sunlight + Torchlight
    // ========================================================================

    void calculateWorldLightingThreaded() {
        std::cout << "\\n=== NEXT-GEN LIGHTING CALCULATION ===" << std::endl;

        // PASO 1: Inicializar todo en 0
        std::cout << "[1/4] Inicializando luz..." << std::endl;
        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        chunk->lightData[x][y][z] = LightVoxel();
                    }
                }
            }
        }

        // PASO 2: Skylight (propagación vertical)
        std::cout << "[2/4] Calculando skylight..." << std::endl;
        calculateSkylight();

        // PASO 3: Sunlight (propagación horizontal)
        std::cout << "[3/4] Propagando sunlight..." << std::endl;
        propagateSunlight();

        // PASO 4: Torchlight (bloques emisores)
        std::cout << "[4/4] Propagando torchlight..." << std::endl;
        propagateTorchlight();

        std::cout << "=== LIGHTING COMPLETE! ===" << std::endl;

        // Marcar chunks para rebuild
        for (auto& pair : chunks) {
            if (pair.second && pair.second->isGenerated) {
                pair.second->needsRebuild = true;
                pair.second->needsLightUpdate = false;
            }
        }
    }

    // Helpers para World class
    uint8_t getSunlight(int x, int y, int z) {
        if (y < 0 || y >= CHUNK_HEIGHT) return 0;

        Vec3i chunkPos(
            (int)floor((float)x / CHUNK_SIZE),
            0,
            (int)floor((float)z / CHUNK_SIZE)
        );

        Chunk* chunk = getChunk(chunkPos);
        if (!chunk) return 0;

        int localX = x - chunkPos.x * CHUNK_SIZE;
        int localZ = z - chunkPos.z * CHUNK_SIZE;

        if (localX < 0) localX += CHUNK_SIZE;
        if (localZ < 0) localZ += CHUNK_SIZE;

        return chunk->getSunlight(localX, y, localZ);
    }

    void setSunlight(int x, int y, int z, uint8_t level) {
        if (y < 0 || y >= CHUNK_HEIGHT) return;

        Vec3i chunkPos(
            (int)floor((float)x / CHUNK_SIZE),
            0,
            (int)floor((float)z / CHUNK_SIZE)
        );

        Chunk* chunk = getChunk(chunkPos);
        if (!chunk) return;

        int localX = x - chunkPos.x * CHUNK_SIZE;
        int localZ = z - chunkPos.z * CHUNK_SIZE;

        if (localX < 0) localX += CHUNK_SIZE;
        if (localZ < 0) localZ += CHUNK_SIZE;

        chunk->setSunlight(localX, y, localZ, level);
    }

    uint8_t getTorchlight(int x, int y, int z) {
        if (y < 0 || y >= CHUNK_HEIGHT) return 0;

        Vec3i chunkPos(
            (int)floor((float)x / CHUNK_SIZE),
            0,
            (int)floor((float)z / CHUNK_SIZE)
        );

        Chunk* chunk = getChunk(chunkPos);
        if (!chunk) return 0;

        int localX = x - chunkPos.x * CHUNK_SIZE;
        int localZ = z - chunkPos.z * CHUNK_SIZE;

        if (localX < 0) localX += CHUNK_SIZE;
        if (localZ < 0) localZ += CHUNK_SIZE;

        return chunk->getTorchlight(localX, y, localZ);
    }

    void setTorchlight(int x, int y, int z, uint8_t level, uint8_t r, uint8_t g, uint8_t b) {
        if (y < 0 || y >= CHUNK_HEIGHT) return;

        Vec3i chunkPos(
            (int)floor((float)x / CHUNK_SIZE),
            0,
            (int)floor((float)z / CHUNK_SIZE)
        );

        Chunk* chunk = getChunk(chunkPos);
        if (!chunk) return;

        int localX = x - chunkPos.x * CHUNK_SIZE;
        int localZ = z - chunkPos.z * CHUNK_SIZE;

        if (localX < 0) localX += CHUNK_SIZE;
        if (localZ < 0) localZ += CHUNK_SIZE;

        chunk->setTorchlight(localX, y, localZ, level, r, g, b);
    }

    void getLightColor(int x, int y, int z, float& r, float& g, float& b) {
        if (y < 0 || y >= CHUNK_HEIGHT) {
            r = g = b = 1.0f;
            return;
        }

        Vec3i chunkPos(
            (int)floor((float)x / CHUNK_SIZE),
            0,
            (int)floor((float)z / CHUNK_SIZE)
        );

        Chunk* chunk = getChunk(chunkPos);
        if (!chunk) {
            r = g = b = 1.0f;
            return;
        }

        int localX = x - chunkPos.x * CHUNK_SIZE;
        int localZ = z - chunkPos.z * CHUNK_SIZE;

        if (localX < 0) localX += CHUNK_SIZE;
        if (localZ < 0) localZ += CHUNK_SIZE;

        chunk->getLightColor(localX, y, localZ, r, g, b);
    }

    """

# Replace the old lighting code
content = content[:start_idx] + new_lighting_code + content[end_idx:]

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("NEXT-GEN LIGHTING PROPAGATION implementado!")
print("\nSistemas:")
print("  [1] EmissiveBlock con RGB color")
print("  [2] Skylight vertical (luz solar desde arriba)")
print("  [3] Sunlight horizontal BFS propagation")
print("  [4] Torchlight BFS propagation con color")
print("  [5] Sistema completo en 4 pasos")
print("  [6] Helpers: getSunlight, getTorchlight, getLightColor")
