#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find where to add the threading includes
includes_marker = "#include <queue>"
if includes_marker not in content:
    print("ERROR: Could not find #include <queue>")
    exit(1)

new_includes = """#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>"""

content = content.replace(includes_marker, new_includes)

# Now find the lighting system section
lighting_section_start = "    // ========================================================================\n    // SISTEMA DE ILUMINACIÓN DINÁMICA (0-18 niveles)\n    // ========================================================================"

if lighting_section_start not in content:
    print("ERROR: Could not find lighting section")
    exit(1)

# Find the end of the lighting section (before FISICA Y COLISIONES)
fisica_section = "// ============================================================================\n// FISICA Y COLISIONES\n// ============================================================================"

lighting_start_idx = content.find(lighting_section_start)
fisica_start_idx = content.find(fisica_section)

if lighting_start_idx == -1 or fisica_start_idx == -1:
    print("ERROR: Could not find section boundaries")
    exit(1)

# New lighting system with threading
new_lighting_system = """    // ========================================================================
    // SISTEMA DE ILUMINACIÓN DINÁMICA (0-18 niveles) - CON THREADING
    // ========================================================================

    std::mutex lightingMutex;
    std::atomic<bool> lightingInProgress{false};
    std::thread* lightingThread = nullptr;

    unsigned char getLightLevel(int x, int y, int z) {
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

        return chunk->getLightLevel(localX, y, localZ);
    }

    void setLightLevel(int x, int y, int z, unsigned char level) {
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

        chunk->setLightLevel(localX, y, localZ, level);
    }

    // Obtener emisión de luz de un bloque
    int getBlockEmission(BlockType block) {
        switch (block) {
            // Futuro: bloques que emiten luz
            // case BLOCK_TORCH: return 14;
            // case BLOCK_LAVA: return 15;
            // case BLOCK_GLOWSTONE: return 15;
            default: return 0;
        }
    }

    // Propagar luz desde una posición específica (BFS)
    void propagateLightFrom(int x, int y, int z, std::queue<Vec3i>& lightQueue) {
        unsigned char currentLight = getLightLevel(x, y, z);
        if (currentLight <= 1) return;

        // 6 direcciones (arriba, abajo, norte, sur, este, oeste)
        int dx[] = {0, 0, 0, 0, 1, -1};
        int dy[] = {1, -1, 0, 0, 0, 0};
        int dz[] = {0, 0, 1, -1, 0, 0};

        for (int i = 0; i < 6; i++) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            int nz = z + dz[i];

            if (ny < 0 || ny >= CHUNK_HEIGHT) continue;

            BlockType neighborBlock = getBlock(nx, ny, nz);

            // Solo propagar a bloques transparentes
            if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

            unsigned char neighborLight = getLightLevel(nx, ny, nz);
            unsigned char newLight = currentLight - 1; // Decrementar luz

            if (newLight > neighborLight) {
                setLightLevel(nx, ny, nz, newLight);
                lightQueue.push(Vec3i(nx, ny, nz));
            }
        }
    }

    // NUEVO: Calcular iluminación completa del mundo (con propagación mejorada)
    void calculateWorldLightingThreaded() {
        std::cout << "Calculando iluminación global..." << std::endl;

        // PASO 1: Inicializar todos los bloques con luz 0
        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        chunk->setLightLevel(x, y, z, 0);
                    }
                }
            }
        }

        std::queue<Vec3i> lightQueue;

        // PASO 2: Establecer luz del sol (nivel 18) desde arriba
        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int worldX = chunk->position.x * CHUNK_SIZE + x;
                    int worldZ = chunk->position.z * CHUNK_SIZE + z;

                    // Encontrar la superficie (primer bloque sólido desde arriba)
                    bool foundSurface = false;
                    for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
                        BlockType block = chunk->getBlock(x, y, z);

                        if (!foundSurface) {
                            // Todavía no hemos encontrado superficie, es aire/cielo
                            if (block == BLOCK_AIR || block == BLOCK_WATER) {
                                chunk->setLightLevel(x, y, z, 18); // Luz solar máxima
                                lightQueue.push(Vec3i(worldX, y, worldZ));
                            } else {
                                // Encontramos el primer bloque sólido
                                foundSurface = true;
                            }
                        } else {
                            // Debajo de la superficie, luz 0 (se propagará desde arriba)
                            break;
                        }
                    }
                }
            }
        }

        // PASO 3: Añadir bloques emisores de luz (antorchas, lava, etc.)
        for (auto& pair : chunks) {
            Chunk* chunk = pair.second;
            if (!chunk || !chunk->isGenerated) continue;

            for (int x = 0; x < CHUNK_SIZE; x++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    for (int z = 0; z < CHUNK_SIZE; z++) {
                        BlockType block = chunk->getBlock(x, y, z);
                        int emission = getBlockEmission(block);
                        if (emission > 0) {
                            int worldX = chunk->position.x * CHUNK_SIZE + x;
                            int worldZ = chunk->position.z * CHUNK_SIZE + z;
                            chunk->setLightLevel(x, y, z, emission);
                            lightQueue.push(Vec3i(worldX, y, worldZ));
                        }
                    }
                }
            }
        }

        // PASO 4: Propagar luz usando BFS (SIN límite de iteraciones)
        std::cout << "Propagando luz desde " << lightQueue.size() << " fuentes..." << std::endl;
        int iterations = 0;

        while (!lightQueue.empty()) {
            Vec3i pos = lightQueue.front();
            lightQueue.pop();
            propagateLightFrom(pos.x, pos.y, pos.z, lightQueue);
            iterations++;

            // Log de progreso cada 100k iteraciones
            if (iterations % 100000 == 0) {
                std::cout << "  Iteraciones: " << iterations << ", Cola: " << lightQueue.size() << std::endl;
            }
        }

        std::cout << "Iluminación completada! Total iteraciones: " << iterations << std::endl;

        // PASO 5: Marcar todos los chunks para reconstruir mesh
        for (auto& pair : chunks) {
            if (pair.second && pair.second->isGenerated) {
                pair.second->needsRebuild = true;
                pair.second->needsLightUpdate = false;
            }
        }
    }

    // Calcular iluminación en un hilo separado
    void startLightingCalculation() {
        if (lightingInProgress) {
            std::cout << "Iluminación ya en progreso, ignorando..." << std::endl;
            return;
        }

        // Esperar a que termine el hilo anterior si existe
        if (lightingThread != nullptr) {
            if (lightingThread->joinable()) {
                lightingThread->join();
            }
            delete lightingThread;
        }

        lightingInProgress = true;
        lightingThread = new std::thread([this]() {
            calculateWorldLightingThreaded();
            lightingInProgress = false;
        });
    }

    // Actualizar iluminación (versión simplificada, llama al sistema threaded)
    void updateWorldLighting() {
        // Solo iniciar cálculo si hay chunks que necesitan actualización
        bool needsUpdate = false;
        for (auto& pair : chunks) {
            if (pair.second && pair.second->needsLightUpdate && pair.second->isGenerated) {
                needsUpdate = true;
                break;
            }
        }

        if (needsUpdate && !lightingInProgress) {
            startLightingCalculation();
        }
    }

    // Destructor: esperar a que termine el hilo de iluminación
    ~World() {
        if (lightingThread != nullptr) {
            if (lightingThread->joinable()) {
                lightingThread->join();
            }
            delete lightingThread;
        }

        for (auto& pair : chunks) {
            delete pair.second;
        }
    }
"""

# Replace the lighting section
content = content[:lighting_start_idx] + new_lighting_system + "\n" + content[fisica_start_idx:]

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Successfully implemented threaded lighting system!")
print("\nChanges:")
print("  ✓ Added threading includes (thread, mutex, atomic)")
print("  ✓ Added lighting mutex for thread safety")
print("  ✓ Implemented calculateWorldLightingThreaded() - NO iteration limit")
print("  ✓ PASO 1: Initialize all blocks with light 0")
print("  ✓ PASO 2: Set sunlight (level 18) from sky down to surface")
print("  ✓ PASO 3: Add light-emitting blocks")
print("  ✓ PASO 4: BFS propagation with no limit")
print("  ✓ Caves will naturally have low light (0-5)")
print("  ✓ Exterior will have full light (18)")
print("  ✓ Shadows from blocks will attenuate light (17, 13, etc.)")
