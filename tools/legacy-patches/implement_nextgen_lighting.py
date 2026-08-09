#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the Chunk structure
old_chunk_light = """struct Chunk {
    Vec3i position;
    BlockType blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    unsigned char lightLevels[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]; // 0-18 niveles de luz
    unsigned int displayList;
    bool needsRebuild;
    bool isGenerated;
    bool needsLightUpdate;

    Chunk(Vec3i pos) : position(pos), displayList(0), needsRebuild(true), isGenerated(false), needsLightUpdate(true) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    blocks[x][y][z] = BLOCK_AIR;
                    lightLevels[x][y][z] = 0;
                }
            }
        }
    }"""

new_chunk_light = """// ============================================================================
// NEXT-GEN LIGHTING SYSTEM - RGB COLORED LIGHTING + SKYLIGHT + TORCHLIGHT
// ============================================================================

// Estructura de luz optimizada con bitfields (5 bits por canal)
// Total: 16 bits (2 bytes) por voxel
struct LightVoxel {
    uint16_t sunlight   : 5;  // 0-31 (usamos 0-18)
    uint16_t torchlight : 5;  // 0-31 (usamos 0-18)
    uint16_t red        : 2;  // 0-3 (RGB reducido para ahorrar espacio)
    uint16_t green      : 2;  // 0-3
    uint16_t blue       : 2;  // 0-3

    LightVoxel() : sunlight(0), torchlight(0), red(0), green(0), blue(0) {}

    // Obtener luz total (max de sunlight y torchlight)
    uint8_t getTotalLight() const {
        return (sunlight > torchlight) ? sunlight : torchlight;
    }

    // Obtener color de luz (0.0 - 1.0)
    void getLightColor(float& r, float& g, float& b) const {
        if (torchlight > 0) {
            // Luz de antorcha tiene color
            r = red / 3.0f;
            g = green / 3.0f;
            b = blue / 3.0f;
        } else {
            // Luz solar es blanca
            r = g = b = 1.0f;
        }
    }
};

struct Chunk {
    Vec3i position;
    BlockType blocks[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE];
    LightVoxel lightData[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]; // NEXT-GEN LIGHTING
    unsigned int displayList;
    bool needsRebuild;
    bool isGenerated;
    bool needsLightUpdate;

    Chunk(Vec3i pos) : position(pos), displayList(0), needsRebuild(true), isGenerated(false), needsLightUpdate(true) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    blocks[x][y][z] = BLOCK_AIR;
                    lightData[x][y][z] = LightVoxel(); // Inicializar luz en 0
                }
            }
        }
    }"""

content = content.replace(old_chunk_light, new_chunk_light)

# Update getLightLevel method
old_get_light = """    unsigned char getLightLevel(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return 0;
        return lightLevels[x][y][z];
    }"""

new_get_light = """    // Obtener luz total (max de sun y torch)
    uint8_t getLightLevel(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return 0;
        return lightData[x][y][z].getTotalLight();
    }

    // Obtener sunlight
    uint8_t getSunlight(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return 0;
        return lightData[x][y][z].sunlight;
    }

    // Obtener torchlight
    uint8_t getTorchlight(int x, int y, int z) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return 0;
        return lightData[x][y][z].torchlight;
    }

    // Obtener color de luz
    void getLightColor(int x, int y, int z, float& r, float& g, float& b) const {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE) {
            r = g = b = 1.0f;
            return;
        }
        lightData[x][y][z].getLightColor(r, g, b);
    }"""

content = content.replace(old_get_light, new_get_light)

# Update setLightLevel method
old_set_light = """    void setLightLevel(int x, int y, int z, unsigned char level) {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return;
        if (level > 18) level = 18;
        lightLevels[x][y][z] = level;
        needsRebuild = true;
    }"""

new_set_light = """    // Establecer sunlight
    void setSunlight(int x, int y, int z, uint8_t level) {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return;
        if (level > 18) level = 18;
        lightData[x][y][z].sunlight = level;
        needsRebuild = true;
    }

    // Establecer torchlight con color RGB
    void setTorchlight(int x, int y, int z, uint8_t level, uint8_t r, uint8_t g, uint8_t b) {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return;
        if (level > 18) level = 18;
        lightData[x][y][z].torchlight = level;
        lightData[x][y][z].red = (r > 3) ? 3 : r;
        lightData[x][y][z].green = (g > 3) ? 3 : g;
        lightData[x][y][z].blue = (b > 3) ? 3 : b;
        needsRebuild = true;
    }

    // Legacy: setLightLevel (ahora usa sunlight)
    void setLightLevel(int x, int y, int z, uint8_t level) {
        setSunlight(x, y, z, level);
    }"""

content = content.replace(old_set_light, new_set_light)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("NEXT-GEN LIGHTING STRUCTURE implementada!")
print("\nCaracteristicas:")
print("  - LightVoxel con bitfields (16 bits total)")
print("  - 5 bits sunlight (0-18)")
print("  - 5 bits torchlight (0-18)")
print("  - 2 bits RGB cada color")
print("  - Metodos getSunlight() y getTorchlight()")
print("  - Metodos getLightColor() para colored lighting")
print("  - Backward compatible con codigo existente")
