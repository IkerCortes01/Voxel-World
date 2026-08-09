#!/usr/bin/env python3
# Script para actualizar la generación de terreno y sistema de minado

import re

# Leer el archivo
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# 1. Cambiar SEA_LEVEL de 62 a 45 (para tener ríos en lugar de océanos)
content = re.sub(
    r'const int SEA_LEVEL = 62;',
    'const int SEA_LEVEL = 45;',
    content
)

# 2. Modificar la condición de minado para permitir romper todo excepto agua
old_mining_check = r'''    // No minar aire ni agua
    if \(blockType == BLOCK_AIR \|\| blockType == BLOCK_WATER\) \{
        state->isMining = false;
        state->miningProgress = 0\.0f;
        return;
    \}'''

new_mining_check = '''    // Solo no minar aire (el agua no se puede romper según getBlockBreakTime)
    if (blockType == BLOCK_AIR) {
        state->isMining = false;
        state->miningProgress = 0.0f;
        return;
    }

    // No minar bloques irrompibles (bedrock y agua según getBlockBreakTime)
    float checkBreakTime = getBlockBreakTime(blockType);
    if (checkBreakTime >= 999.0f) {
        state->isMining = false;
        state->miningProgress = 0.0f;
        return;
    }'''

content = re.sub(old_mining_check, new_mining_check, content, flags=re.MULTILINE)

# 3. Modificar la generación de terreno para más montañas y cuevas
old_terrain_gen = r'''                // Combinar noises para terreno más natural
                float height = continentalness \* 0\.6f \+ erosion \* 0\.3f \+ peaks \* 0\.1f;
                height = \(height \+ 1\.0f\) \* 0\.5f; // Normalizar a 0-1

                int terrainHeight = \(int\)\(height \* 80\) \+ 20; // Rango 20-100'''

new_terrain_gen = '''                // Combinar noises para terreno más montañoso
                float height = continentalness * 0.4f + erosion * 0.3f + peaks * 0.3f;
                height = (height + 1.0f) * 0.5f; // Normalizar a 0-1

                int terrainHeight = (int)(height * 120) + 15; // Rango 15-135 (más montañas)'''

content = re.sub(old_terrain_gen, new_terrain_gen, content, flags=re.MULTILINE)

# 4. Añadir generación de cuevas
old_stone_block = r'''                    // Piedra profunda
                    else if \(y < terrainHeight - 5\) \{
                        blockType = BLOCK_STONE;
                    \}'''

new_stone_block = '''                    // Piedra profunda con cuevas
                    else if (y < terrainHeight - 5) {
                        // Generar cuevas usando noise 3D
                        float caveNoise = noise.octaveNoise(worldX * 0.05f, y * 0.05f, worldZ * 0.05f, 3);

                        // Cuevas más frecuentes y grandes
                        if (y > 10 && y < terrainHeight - 10 && caveNoise > 0.55f) {
                            blockType = BLOCK_AIR; // Cueva
                        } else {
                            blockType = BLOCK_STONE;
                        }
                    }'''

content = re.sub(old_stone_block, new_stone_block, content, flags=re.MULTILINE)

# Guardar el archivo modificado
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("[OK] Archivo actualizado exitosamente!")
print("   - Nivel del mar cambiado de 62 a 45 (rios)")
print("   - Sistema de minado: ahora se puede romper todo excepto agua")
print("   - Generacion de terreno: mas montanas (rango 15-135)")
print("   - Anadidas cuevas con noise 3D")
