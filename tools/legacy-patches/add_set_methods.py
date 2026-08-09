#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find setLightLevel and replace it completely
old_set_light = """    void setLightLevel(int x, int y, int z, unsigned char level) {
        if (x < 0 || x >= CHUNK_SIZE || y < 0 || y >= CHUNK_HEIGHT || z < 0 || z >= CHUNK_SIZE)
            return;
        if (level > 18) level = 18; // Máximo 18
        lightLevels[x][y][z] = level;
        needsRebuild = true;
    }
};"""

new_set_methods = """    // Establecer sunlight
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
    }
};"""

content = content.replace(old_set_light, new_set_methods)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Added setSunlight() and setTorchlight() methods to Chunk!")
