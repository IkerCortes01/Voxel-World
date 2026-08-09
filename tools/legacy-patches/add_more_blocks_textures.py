#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AGREGAR MÁS BLOQUES Y TEXTURAS
Expandir el sistema de bloques
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

print("Agregando nuevos bloques y texturas...")

# 1. Agregar nuevos tipos de bloques
old_enum = r'(BLOCK_BEDROCK)\s*\};'
new_blocks = r'''\1,
    BLOCK_COBBLESTONE,
    BLOCK_PLANKS,
    BLOCK_BRICKS,
    BLOCK_GLASS,
    BLOCK_COAL_ORE,
    BLOCK_IRON_ORE,
    BLOCK_GOLD_ORE,
    BLOCK_DIAMOND_ORE
};'''

content = re.sub(old_enum, new_blocks, content)
print("  1. Agregados 8 nuevos tipos de bloques")

# 2. Agregar colores para los nuevos bloques
old_bedrock_color = r'(case BLOCK_BEDROCK:.*?r = 0\.1f; g = 0\.1f; b = 0\.1f; break;)'

new_colors = r'''\1
        case BLOCK_COBBLESTONE: r = 0.5f; g = 0.5f; b = 0.5f; break;
        case BLOCK_PLANKS: r = 0.6f; g = 0.4f; b = 0.2f; break;
        case BLOCK_BRICKS: r = 0.7f; g = 0.3f; b = 0.2f; break;
        case BLOCK_GLASS: r = 0.8f; g = 0.9f; b = 1.0f; break;
        case BLOCK_COAL_ORE: r = 0.3f; g = 0.3f; b = 0.3f; break;
        case BLOCK_IRON_ORE: r = 0.7f; g = 0.6f; b = 0.5f; break;
        case BLOCK_GOLD_ORE: r = 1.0f; g = 0.8f; b = 0.2f; break;
        case BLOCK_DIAMOND_ORE: r = 0.4f; g = 0.8f; b = 1.0f; break;'''

content = re.sub(old_bedrock_color, new_colors, content)
print("  2. Agregados colores para nuevos bloques")

# 3. Hacer bloques transparentes (GLASS)
old_is_transparent = r'(return type == BLOCK_AIR \|\| type == BLOCK_WATER);'
new_is_transparent = r'return type == BLOCK_AIR || type == BLOCK_WATER || type == BLOCK_GLASS;'

content = re.sub(old_is_transparent, new_is_transparent, content)
print("  3. GLASS marcado como transparente")

# 4. Agregar nombres para el inventario
old_block_names = r'(case BLOCK_BEDROCK: return "Bedrock";)'

new_names = r'''\1
        case BLOCK_COBBLESTONE: return "Cobblestone";
        case BLOCK_PLANKS: return "Planks";
        case BLOCK_BRICKS: return "Bricks";
        case BLOCK_GLASS: return "Glass";
        case BLOCK_COAL_ORE: return "Coal Ore";
        case BLOCK_IRON_ORE: return "Iron Ore";
        case BLOCK_GOLD_ORE: return "Gold Ore";
        case BLOCK_DIAMOND_ORE: return "Diamond Ore";'''

content = re.sub(old_block_names, new_names, content)
print("  4. Agregados nombres para inventario")

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\nBloques agregados:")
print("  - COBBLESTONE (gris)")
print("  - PLANKS (marron)")
print("  - BRICKS (rojo ladrillo)")
print("  - GLASS (transparente azul)")
print("  - COAL ORE (negro)")
print("  - IRON ORE (cafe)")
print("  - GOLD ORE (dorado)")
print("  - DIAMOND ORE (cyan)")
print("\nAhora hay mas variedad de bloques!")
