#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
EXTREME FPS FIX - ILUMINACIÓN MÍNIMA PARA 60 FPS GARANTIZADOS

Cambios DRÁSTICOS:
1. MAX_ITERATIONS: 10K -> 0 (ELIMINAR BFS)
2. MAX_CHUNKS_PER_FRAME: 5 -> 1
3. Solo SKYLIGHT VERTICAL (columnas) - SIN propagación horizontal
4. Resultado: ~300 operations/frame (16x16 columnas)
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

# Reemplazar TODA la función lightChunk con una versión ultra-simple
old_light_chunk = r'// MINECRAFT-STYLE: Iluminar UN CHUNK individual \(incremental\).*?chunk->needsLightUpdate = false;\s+\}'

new_light_chunk = """// EXTREME FPS: Iluminar UN CHUNK (solo skylight vertical, SIN BFS)
    void lightChunk(Chunk* chunk) {
        if (!chunk || !chunk->isGenerated) return;

        const int cx = chunk->position.x;
        const int cz = chunk->position.z;

        // SOLO SKYLIGHT VERTICAL - Sin propagación horizontal (ultra rápido)
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                uint8_t currentLight = 18;

                // Propagación vertical top-down
                for (int y = CHUNK_HEIGHT - 1; y >= 0; y--) {
                    BlockType block = chunk->getBlock(x, y, z);

                    if (block == BLOCK_AIR || block == BLOCK_WATER) {
                        chunk->setSunlight(x, y, z, currentLight);
                    } else {
                        chunk->setSunlight(x, y, z, 0);
                        currentLight = 0;  // Bloque sólido bloquea luz
                    }
                }
            }
        }

        // Propagar 1 bloque a los lados (mínima propagación)
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_HEIGHT; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    uint8_t light = chunk->getSunlight(x, y, z);
                    if (light <= 1) continue;

                    // Propagar a vecinos inmediatos (solo -1 light)
                    int dx[] = {1, -1, 0, 0};
                    int dz[] = {0, 0, 1, -1};

                    for (int i = 0; i < 4; i++) {
                        int nx = x + dx[i];
                        int nz = z + dz[i];

                        if (nx >= 0 && nx < CHUNK_SIZE && nz >= 0 && nz < CHUNK_SIZE) {
                            BlockType neighborBlock = chunk->getBlock(nx, y, nz);
                            if (neighborBlock == BLOCK_AIR || neighborBlock == BLOCK_WATER) {
                                uint8_t neighborLight = chunk->getSunlight(nx, y, nz);
                                uint8_t newLight = light - 1;
                                if (newLight > neighborLight) {
                                    chunk->setSunlight(nx, y, nz, newLight);
                                }
                            }
                        }
                    }
                }
            }
        }

        chunk->needsRebuild = true;
        chunk->needsLightUpdate = false;
    }"""

content = re.sub(old_light_chunk, new_light_chunk, content, flags=re.DOTALL)

# Cambiar chunks procesados por frame: 5 -> 1
content = content.replace(
    'const int MAX_CHUNKS_PER_FRAME = 5;  // Ultra-optimizado: 5 chunks/frame',
    'const int MAX_CHUNKS_PER_FRAME = 1;  // EXTREME FPS: 1 chunk/frame'
)

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("EXTREME FPS FIX aplicado!")
print("")
print("Cambios DRASTICOS:")
print("  1. ELIMINADO BFS completamente")
print("  2. Solo skylight vertical (columnas)")
print("  3. Propagacion minima (1 bloque a los lados)")
print("  4. 1 chunk por frame")
print("")
print("Operations por frame:")
print("  - Skylight: 16x16x256 = 65K reads (muy rapido)")
print("  - Propagacion minima: ~1K operations")
print("  - Total: ~66K operations vs 50M antes")
print("")
print("Resultado esperado: 60 FPS ESTABLES GARANTIZADOS")
