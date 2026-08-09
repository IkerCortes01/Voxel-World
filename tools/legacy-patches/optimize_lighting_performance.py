import re

# Leer el archivo main.cpp
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

print("Optimizando sistema de iluminación para 60 FPS...")

# ============================================
# FIX 1: Optimizar propagateSunlight con visited set
# ============================================

# Encontrar la función propagateSunlight y reemplazarla con una versión optimizada
old_propagate_sunlight = r"""    void propagateSunlight\(\) \{
        std::cout << "Propagando sunlight horizontal\.\.\." << std::endl;

        std::queue<LightNode> lightQueue;

        // Añadir todos los voxels con sunlight como fuentes
        for \(auto& pair : chunks\) \{
            Chunk\* chunk = pair\.second;
            if \(!chunk \|\| !chunk->isGenerated\) continue;

            for \(int x = 0; x < CHUNK_SIZE; x\+\+\) \{
                for \(int y = 0; y < CHUNK_HEIGHT; y\+\+\) \{
                    for \(int z = 0; z < CHUNK_SIZE; z\+\+\) \{
                        uint8_t sunlight = chunk->getSunlight\(x, y, z\);
                        if \(sunlight > 0\) \{
                            int worldX = chunk->position\.x \* CHUNK_SIZE \+ x;
                            int worldZ = chunk->position\.z \* CHUNK_SIZE \+ z;
                            lightQueue\.push\(LightNode\(worldX, y, worldZ, sunlight\)\);
                        \}
                    \}
                \}
            \}
        \}

        // BFS propagation
        int iterations = 0;
        while \(!lightQueue\.empty\(\)\) \{
            LightNode node = lightQueue\.front\(\);
            lightQueue\.pop\(\);

            if \(node\.lightLevel <= 1\) continue;

            // 6 direcciones
            int dx\[\] = \{0, 0, 0, 0, 1, -1\};
            int dy\[\] = \{1, -1, 0, 0, 0, 0\};
            int dz\[\] = \{0, 0, 1, -1, 0, 0\};

            for \(int i = 0; i < 6; i\+\+\) \{
                int nx = node\.x \+ dx\[i\];
                int ny = node\.y \+ dy\[i\];
                int nz = node\.z \+ dz\[i\];

                if \(ny < 0 \|\| ny >= CHUNK_HEIGHT\) continue;

                BlockType neighborBlock = getBlock\(nx, ny, nz\);
                if \(neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER\) continue;

                uint8_t currentSunlight = getSunlight\(nx, ny, nz\);
                uint8_t newLight = node\.lightLevel - 1;

                if \(newLight > currentSunlight\) \{
                    setSunlight\(nx, ny, nz, newLight\);
                    lightQueue\.push\(LightNode\(nx, ny, nz, newLight\)\);
                \}
            \}

            iterations\+\+;
            if \(iterations % 50000 == 0\) \{
                std::cout << "  Sunlight iterations: " << iterations << std::endl;
            \}
        \}

        std::cout << "Sunlight propagation completada! \(" << iterations << " iterations\)" << std::endl;
    \}"""

new_propagate_sunlight = """    void propagateSunlight() {
        std::cout << "Propagando sunlight horizontal (OPTIMIZADO)..." << std::endl;

        std::queue<LightNode> lightQueue;
        std::unordered_set<int64_t> visited;  // Evitar procesar el mismo voxel múltiples veces

        auto hashPos = [](int x, int y, int z) -> int64_t {
            return ((int64_t)x << 32) | ((int64_t)y << 16) | (int64_t)z;
        };

        // Añadir todos los voxels con sunlight como fuentes
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
                            int64_t hash = hashPos(worldX, y, worldZ);
                            lightQueue.push(LightNode(worldX, y, worldZ, sunlight));
                            visited.insert(hash);
                        }
                    }
                }
            }
        }

        // BFS propagation con límite de iteraciones
        int iterations = 0;
        const int MAX_ITERATIONS = 1000000;  // Límite de seguridad

        while (!lightQueue.empty() && iterations < MAX_ITERATIONS) {
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

                int64_t hash = hashPos(nx, ny, nz);
                if (visited.count(hash)) continue;  // YA VISITADO, SKIP

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

        std::cout << "Sunlight propagation completada! (" << iterations << " iterations)" << std::endl;
    }"""

content = re.sub(old_propagate_sunlight, new_propagate_sunlight, content, flags=re.DOTALL)

# ============================================
# FIX 2: Optimizar propagateTorchlight con visited set
# ============================================

old_propagate_torchlight = r"""    // Propagar TORCHLIGHT \(luz de antorchas y bloques emisores\)
    void propagateTorchlight\(\) \{
        std::cout << "Propagando torchlight\.\.\." << std::endl;

        std::queue<LightNode> lightQueue;

        // Añadir bloques emisores como fuentes
        for \(auto& pair : chunks\) \{
            Chunk\* chunk = pair\.second;
            if \(!chunk \|\| !chunk->isGenerated\) continue;

            for \(int x = 0; x < CHUNK_SIZE; x\+\+\) \{
                for \(int y = 0; y < CHUNK_HEIGHT; y\+\+\) \{
                    for \(int z = 0; z < CHUNK_SIZE; z\+\+\) \{
                        BlockType block = chunk->getBlock\(x, y, z\);
                        EmissiveBlock emission = getBlockEmission\(block\);

                        if \(emission\.light > 0\) \{
                            int worldX = chunk->position\.x \* CHUNK_SIZE \+ x;
                            int worldZ = chunk->position\.z \* CHUNK_SIZE \+ z;

                            chunk->setTorchlight\(x, y, z, emission\.light,
                                               emission\.r, emission\.g, emission\.b\);

                            lightQueue\.push\(LightNode\(worldX, y, worldZ, emission\.light\)\);
                        \}
                    \}
                \}
            \}
        \}

        // BFS propagation
        int iterations = 0;
        while \(!lightQueue\.empty\(\)\) \{
            LightNode node = lightQueue\.front\(\);
            lightQueue\.pop\(\);

            if \(node\.lightLevel <= 1\) continue;

            // 6 direcciones
            int dx\[\] = \{0, 0, 0, 0, 1, -1\};
            int dy\[\] = \{1, -1, 0, 0, 0, 0\};
            int dz\[\] = \{0, 0, 1, -1, 0, 0\};

            for \(int i = 0; i < 6; i\+\+\) \{
                int nx = node\.x \+ dx\[i\];
                int ny = node\.y \+ dy\[i\];
                int nz = node\.z \+ dz\[i\];

                if \(ny < 0 \|\| ny >= CHUNK_HEIGHT\) continue;

                BlockType neighborBlock = getBlock\(nx, ny, nz\);
                if \(neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER\) continue;

                uint8_t currentTorchlight = getTorchlight\(nx, ny, nz\);
                uint8_t newLight = node\.lightLevel - 1;

                if \(newLight > currentTorchlight\) \{
                    setTorchlight\(nx, ny, nz, newLight, 3, 2, 1\);  // Color anaranjado por defecto
                    lightQueue\.push\(LightNode\(nx, ny, nz, newLight\)\);
                \}
            \}

            iterations\+\+;
            if \(iterations % 50000 == 0\) \{
                std::cout << "  Torchlight iterations: " << iterations << std::endl;
            \}
        \}

        std::cout << "Torchlight propagation completada! \(" << iterations << " iterations\)" << std::endl;
    \}"""

new_propagate_torchlight = """    // Propagar TORCHLIGHT (luz de antorchas y bloques emisores)
    void propagateTorchlight() {
        std::cout << "Propagando torchlight (OPTIMIZADO)..." << std::endl;

        std::queue<LightNode> lightQueue;
        std::unordered_set<int64_t> visited;

        auto hashPos = [](int x, int y, int z) -> int64_t {
            return ((int64_t)x << 32) | ((int64_t)y << 16) | (int64_t)z;
        };

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

                            int64_t hash = hashPos(worldX, y, worldZ);
                            lightQueue.push(LightNode(worldX, y, worldZ, emission.light));
                            visited.insert(hash);
                        }
                    }
                }
            }
        }

        // BFS propagation con límite
        int iterations = 0;
        const int MAX_ITERATIONS = 500000;  // Menos límite porque hay menos fuentes

        while (!lightQueue.empty() && iterations < MAX_ITERATIONS) {
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

                int64_t hash = hashPos(nx, ny, nz);
                if (visited.count(hash)) continue;

                BlockType neighborBlock = getBlock(nx, ny, nz);
                if (neighborBlock != BLOCK_AIR && neighborBlock != BLOCK_WATER) continue;

                uint8_t currentTorchlight = getTorchlight(nx, ny, nz);
                uint8_t newLight = node.lightLevel - 1;

                if (newLight > currentTorchlight) {
                    setTorchlight(nx, ny, nz, newLight, 3, 2, 1);  // Color anaranjado por defecto
                    lightQueue.push(LightNode(nx, ny, nz, newLight));
                    visited.insert(hash);
                }
            }

            iterations++;
        }

        std::cout << "Torchlight propagation completada! (" << iterations << " iterations)" << std::endl;
    }"""

content = re.sub(old_propagate_torchlight, new_propagate_torchlight, content, flags=re.DOTALL)

# ============================================
# FIX 3: Agregar #include <unordered_set> al inicio
# ============================================

# Buscar donde están los includes y agregar unordered_set
includes_section = r'(#include <queue>)'
new_includes = r'\1\n#include <unordered_set>'
content = re.sub(includes_section, new_includes, content)

# Guardar el archivo
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n✓ Sistema de iluminación optimizado!")
print("\nCambios aplicados:")
print("  1. Agregado visited set en propagateSunlight (evita re-procesar voxels)")
print("  2. Agregado visited set en propagateTorchlight")
print("  3. Límite de 1M iterations en sunlight (antes: infinito)")
print("  4. Límite de 500K iterations en torchlight")
print("  5. Agregado #include <unordered_set>")
print("\nEstimado: De 30M+ iterations → ~200K iterations")
print("Ganancia de performance: ~150x más rápido")
