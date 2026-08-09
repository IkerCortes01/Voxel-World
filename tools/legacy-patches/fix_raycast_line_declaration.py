#!/usr/bin/env python3
# Script para añadir declaración forward de renderRaycastLine

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Buscar la función renderRaycastLine y añadir declaración forward antes
function_pos = content.find('void renderRaycastLine(Vec3 origin')

if function_pos != -1:
    # Retroceder hasta encontrar el comentario o inicio de línea
    while function_pos > 0 and content[function_pos-1] != '\n':
        function_pos -= 1

    # Añadir declaración forward
    forward_decl = '// Declaración forward\nvoid renderRaycastLine(Vec3 origin, Vec3 direction, float distance, bool hit);\n\n'

    content = content[:function_pos] + forward_decl + content[function_pos:]
    print("   [OK] Declaracion forward de renderRaycastLine anadida")

    # Guardar
    with open('src/main.cpp', 'w', encoding='utf-8') as f:
        f.write(content)

    print("[OK] Archivo corregido!")
else:
    print("[ERROR] No se encontro la funcion renderRaycastLine")
