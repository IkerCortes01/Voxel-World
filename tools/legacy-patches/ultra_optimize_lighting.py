#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
ULTRA-OPTIMIZACIÓN DEL SISTEMA DE ILUMINACIÓN
Cambios:
1. MAX_ITERATIONS: 50K -> 10K por chunk
2. Propagación limitada SOLO al chunk actual (no vecinos)
3. Chunks procesados por frame: 3 -> 5
4. Procesamiento más simple y rápido
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

# 1. Reducir MAX_ITERATIONS en lightChunk de 50K a 10K
content = content.replace(
    'const int MAX_ITERATIONS = 50000;',
    'const int MAX_ITERATIONS = 10000;  // Ultra-optimizado: solo 10K por chunk'
)

# 2. Aumentar chunks procesados por frame de 3 a 5
content = content.replace(
    'const int MAX_CHUNKS_PER_FRAME = 3;',
    'const int MAX_CHUNKS_PER_FRAME = 5;  // Ultra-optimizado: 5 chunks/frame'
)

# 3. Cambiar límite de propagación: de -1/+CHUNK_SIZE a 0/CHUNK_SIZE (solo dentro del chunk)
old_propagation_limit = r'// Limitar propagación a chunk actual \+ 1 bloque en vecinos\s+int localX = nx - cx \* CHUNK_SIZE;\s+int localZ = nz - cz \* CHUNK_SIZE;\s+if \(localX < -1 \|\| localX > CHUNK_SIZE \|\| localZ < -1 \|\| localZ > CHUNK_SIZE\) continue;'

new_propagation_limit = """// ULTRA-OPTIMIZADO: Propagación SOLO dentro del chunk actual
                int localX = nx - cx * CHUNK_SIZE;
                int localZ = nz - cz * CHUNK_SIZE;
                if (localX < 0 || localX >= CHUNK_SIZE || localZ < 0 || localZ >= CHUNK_SIZE) continue;"""

content = re.sub(old_propagation_limit, new_propagation_limit, content, flags=re.DOTALL)

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("ULTRA-OPTIMIZACION completada!")
print("")
print("Cambios:")
print("  1. MAX_ITERATIONS: 50K -> 10K por chunk (80% reduccion)")
print("  2. Chunks/frame: 3 -> 5 (ilumina mas rapido)")
print("  3. Propagacion: SOLO dentro del chunk (mas rapido)")
print("")
print("Resultado esperado:")
print("  - De 150K iterations/frame -> 50K iterations/frame")
print("  - FPS mucho mas estables")
print("  - Iluminacion mas rapida")
