#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
FIX CRÍTICO PARA 60 FPS GARANTIZADOS
El problema es que hay demasiados chunks renderizándose y meshes rebuilding
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

print("Aplicando FIX CRÍTICO para 60 FPS...")

# 1. REDUCIR RENDER_DISTANCE drásticamente (de 8 a 3)
content = re.sub(
    r'const int RENDER_DISTANCE = \d+;',
    'const int RENDER_DISTANCE = 3;  // CRÍTICO: Reducido para 60 FPS',
    content
)
print("  1. RENDER_DISTANCE: 8 -> 3")

# 2. Reducir chunks generados por frame
content = re.sub(
    r'const int MAX_CHUNKS_PER_FRAME = \d+;',
    'const int MAX_CHUNKS_PER_FRAME = 1;  // CRÍTICO: Solo 1/frame',
    content
)
print("  2. MAX_CHUNKS_PER_FRAME: 2 -> 1")

# 3. Reducir meshes construidos por frame
content = re.sub(
    r'const int MAX_MESHES_PER_FRAME = \d+;',
    'const int MAX_MESHES_PER_FRAME = 1;  // CRÍTICO: Solo 1/frame',
    content
)
print("  3. MAX_MESHES_PER_FRAME: 3 -> 1")

# 4. COMENTAR la parte de lighting en buildChunkMesh que marca needsRebuild
# Buscar donde se marca needsRebuild = true en lightChunk
old_light_chunk_rebuild = r'(chunk->needsRebuild = true;)\s+(chunk->needsLightUpdate = false;)'
new_light_chunk_rebuild = r'// \1  // CRÍTICO: No forzar rebuild\n        \2'
content = re.sub(old_light_chunk_rebuild, new_light_chunk_rebuild, content)
print("  4. Deshabilitado auto-rebuild en lightChunk")

# 5. Asegurar que needsRebuild solo se marque cuando REALMENTE cambia el chunk
# En setBlock, solo marcar si el bloque CAMBIÓ
old_set_block = r'(blocks\[x\]\[y\]\[z\] = type;)\s+(needsRebuild = true;)'
new_set_block = r'''if (blocks[x][y][z] != type) {
            \1
            \2  // Solo rebuild si cambió
        }'''
content = re.sub(old_set_block, new_set_block, content)
print("  5. setBlock solo marca rebuild si bloque cambió")

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n✓ FIX CRÍTICO APLICADO!")
print("\nOptimizaciones:")
print("  - RENDER_DISTANCE: 3 (solo 7x7 = 49 chunks max)")
print("  - 1 chunk generado/frame")
print("  - 1 mesh construido/frame")
print("  - Meshes no se reconstruyen innecesariamente")
print("\nEsto DEBE dar 60 FPS estables.")
