#!/usr/bin/env python3
# Script para añadir la función breakBlock

# Leer el archivo
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Función breakBlock para añadir
breakblock_function = '''void breakBlock(GameState* state) {
    // Iniciar el proceso de minado al presionar click izquierdo
    state->mouseLeftPressed = true;
}

'''

# Buscar la línea donde está la función placeBlock
insert_line = None
for i, line in enumerate(lines):
    if line.strip().startswith('void placeBlock(GameState* state)'):
        insert_line = i
        break

if insert_line is not None:
    # Insertar la función breakBlock antes de placeBlock
    lines.insert(insert_line, breakblock_function)

    # Guardar el archivo
    with open('src/main.cpp', 'w', encoding='utf-8') as f:
        f.writelines(lines)

    print("[OK] Funcion breakBlock anadida exitosamente!")
    print(f"   Insertada en la linea {insert_line + 1}")
else:
    print("[ERROR] No se encontro la funcion placeBlock")
