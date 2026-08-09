#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find where to add the new public methods (after updateChunks)
marker = """        for (auto& pair : chunks) {
            if (meshesBuiltThisFrame >= MAX_MESHES_PER_FRAME) break;
            if (pair.second->needsRebuild) {
                buildChunkMesh(pair.second);
                meshesBuiltThisFrame++;
            }
        }
    }

    void render() {"""

new_code = """        for (auto& pair : chunks) {
            if (meshesBuiltThisFrame >= MAX_MESHES_PER_FRAME) break;
            if (pair.second->needsRebuild) {
                buildChunkMesh(pair.second);
                meshesBuiltThisFrame++;
            }
        }
    }

    // Generar chunks iniciales alrededor del origen (para spawn)
    void generateInitialChunks(int radius) {
        std::cout << "Generando " << ((radius*2+1) * (radius*2+1)) << " chunks iniciales..." << std::endl;

        for (int cx = -radius; cx <= radius; cx++) {
            for (int cz = -radius; cz <= radius; cz++) {
                Vec3i chunkPos(cx, 0, cz);
                getOrCreateChunk(chunkPos);
            }
        }

        std::cout << "Chunks generados! Total: " << chunks.size() << std::endl;
    }

    // Construir todos los meshes pendientes
    void buildAllPendingMeshes() {
        std::cout << "Construyendo meshes..." << std::endl;
        int meshCount = 0;

        for (auto& pair : chunks) {
            if (pair.second->needsRebuild && pair.second->isGenerated) {
                buildChunkMesh(pair.second);
                meshCount++;
            }
        }

        std::cout << "Meshes construidos: " << meshCount << std::endl;
    }

    // Obtener numero de chunks cargados
    int getChunkCount() const {
        return chunks.size();
    }

    void render() {"""

content = content.replace(marker, new_code)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Metodos publicos anadidos a la clase World!")
print("\nNuevos metodos:")
print("  - generateInitialChunks(radius): Genera chunks en area inicial")
print("  - buildAllPendingMeshes(): Construye todos los meshes pendientes")
print("  - getChunkCount(): Obtiene numero de chunks cargados")
