#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find line 952 and replace lightLevels with lightData
for i, line in enumerate(lines):
    if i == 951:  # Line 952 (0-indexed 951)
        if 'unsigned char lightLevels' in line:
            lines[i] = '    LightVoxel lightData[CHUNK_SIZE][CHUNK_HEIGHT][CHUNK_SIZE]; // NEXT-GEN LIGHTING\n'
            print(f"Replaced line {i+1}: lightLevels -> lightData")
        break

# Find the initialization loop and replace lightLevels with lightData
for i, line in enumerate(lines):
    if 'lightLevels[x][y][z] = 0; // Sin luz por defecto' in line:
        lines[i] = '                    lightData[x][y][z] = LightVoxel(); // Inicializar luz en 0\n'
        print(f"Replaced line {i+1}: lightLevels initialization")
        break

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)

print("\nChunk structure fixed!")
print("  - lightLevels -> lightData")
print("  - Initialization updated")
