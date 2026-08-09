#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find "const int CHUNK_SIZE = 16;" and insert LightVoxel struct before it
for i, line in enumerate(lines):
    if 'const int CHUNK_SIZE = 16;' in line:
        # Insert the LightVoxel struct definition before this line
        light_voxel_def = """// ============================================================================
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

"""
        lines.insert(i, light_voxel_def)
        print(f"Inserted LightVoxel struct before line {i+1}")
        break

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)

print("LightVoxel struct added successfully!")
