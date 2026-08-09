#!/usr/bin/env python3
# Script para mover renderRaycastLine fuera de GameState

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Buscar y eliminar la función que está mal ubicada (dentro de GameState)
pattern_remove = r'\n// Dibujar línea de raycast desde el jugador hasta el bloque apuntado\n// Declaración forward\nvoid renderRaycastLine\(Vec3 origin, Vec3 direction, float distance, bool hit\);\n\nvoid renderRaycastLine\(Vec3 origin, Vec3 direction, float distance, bool hit\) \{[^}]*\n\}\n'

function_code = None
match = re.search(pattern_remove, content, re.DOTALL)
if match:
    function_code = match.group(0)
    content = re.sub(pattern_remove, '', content, flags=re.DOTALL)
    print("   [OK] Funcion renderRaycastLine removida de posicion incorrecta")
else:
    print("   [ERROR] No se encontro la funcion para mover")
    exit(1)

# Buscar el cierre de GameState y añadir la función DESPUÉS
gamestate_close = r'(};)\n\n(GameState\* g_gameState = nullptr;)'

if re.search(gamestate_close, content):
    content = re.sub(gamestate_close, r'\1\n' + function_code + r'\n\2', content)
    print("   [OK] Funcion renderRaycastLine movida DESPUES de GameState")
else:
    print("   [ERROR] No se encontro el cierre de GameState")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("[OK] Funcion renderRaycastLine reubicada correctamente!")
