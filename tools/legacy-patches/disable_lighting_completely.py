#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
DESHABILITAR LIGHTING COMPLETAMENTE
Para diagnosticar si el problema es la iluminación o algo más
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

# 1. Comentar processLightingQueue() en el game loop
content = content.replace(
    '// MINECRAFT-STYLE: Procesar lighting queue (3 chunks/frame)\n            g_gameState->world.processLightingQueue();',
    '// LIGHTING DESHABILITADO PARA DIAGNOSTICO\n            // g_gameState->world.processLightingQueue();'
)

# 2. Hacer que getLightLevel() SIEMPRE devuelva 18 (luz máxima)
old_get_light = r'uint8_t getLightLevel\(int x, int y, int z\) const \{[^}]+return.*?;'

new_get_light = """uint8_t getLightLevel(int x, int y, int z) const {
        return 18;  // LUZ FIJA MAXIMA (sin iluminacion)"""

content = re.sub(old_get_light, new_get_light, content, flags=re.DOTALL)

# 3. Comentar queueChunkForLighting en generateChunk
content = content.replace(
    'queueChunkForLighting(chunk->position);  // Agregar a queue',
    '// queueChunkForLighting(chunk->position);  // DESHABILITADO'
)

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("LIGHTING COMPLETAMENTE DESHABILITADO!")
print("")
print("Cambios:")
print("  1. processLightingQueue() comentado")
print("  2. getLightLevel() siempre retorna 18 (luz maxima)")
print("  3. queueChunkForLighting() comentado")
print("")
print("AHORA TODO EL MUNDO TENDRA LUZ MAXIMA (18)")
print("")
print("Si los FPS siguen bajos, el problema NO es la iluminacion.")
print("Si los FPS suben a 60, entonces la iluminacion era el problema.")
