#!/usr/bin/env python3
# Script para añadir declaración forward de renderText

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Buscar la función renderHotbar y añadir declaración forward antes
hotbar_pos = content.find('void renderHotbar(Inventory* inventory')

if hotbar_pos != -1:
    # Retroceder hasta encontrar el comentario
    while hotbar_pos > 0 and content[hotbar_pos-80:hotbar_pos].find('RENDERIZADO DE HOTBAR') == -1:
        hotbar_pos -= 1

    # Retroceder al inicio de línea
    while hotbar_pos > 0 and content[hotbar_pos-1] != '\n':
        hotbar_pos -= 1

    # Añadir declaraciones forward
    forward_decl = '''// Declaraciones forward
void renderText(const char* text, float x, float y, float size);
void renderChar(char c, float x, float y, float size);

'''

    content = content[:hotbar_pos] + forward_decl + content[hotbar_pos:]
    print("   [OK] Declaraciones forward anadidas")

    # Guardar
    with open('src/main.cpp', 'w', encoding='utf-8') as f:
        f.write(content)

    print("[OK] Archivo corregido!")
else:
    print("[ERROR] No se encontro la funcion renderHotbar")
