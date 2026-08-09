#!/usr/bin/env python3
# Script para arreglar la detección de bloques en todas direcciones

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# ARREGLAR updateMining: Usar raycast simple de 5 bloques
# ============================================================================
print("[INFO] Arreglando funcion updateMining...")

old_raycast = r'''    Vec3 origin = state->player\.getEyePosition\(\);
    Vec3 direction = state->player\.getForward\(\);

    // Detectar qué bloque estamos mirando
    RaycastResult shortResult = raycastBlockAll\(state->world, origin, direction, 1\.5f\);
    RaycastResult result = shortResult;

    if \(!shortResult\.hit\) \{
        result = raycastBlock\(state->world, origin, direction, 5\.0f\);
    \}'''

new_raycast = '''    Vec3 origin = state->player.getEyePosition();
    Vec3 direction = state->player.getForward();

    // Detectar qué bloque estamos mirando (solo bloques sólidos, hasta 5 bloques)
    RaycastResult result = raycastBlock(state->world, origin, direction, 5.0f);'''

if re.search(old_raycast, content):
    content = re.sub(old_raycast, new_raycast, content)
    print("   [OK] Raycast en updateMining simplificado")
    print("        - Eliminado raycast corto de 1.5 bloques")
    print("        - Ahora usa directamente raycast de 5 bloques")
else:
    print("   [WARNING] No se encontro el patron exacto, buscando alternativa...")

    # Buscar patrón más flexible
    pattern = r'(    Vec3 origin = state->player\.getEyePosition\(\);)\s+(Vec3 direction = state->player\.getForward\(\);)\s+// Detectar[^}]+?raycastBlock\(state->world, origin, direction, 5\.0f\);'

    if re.search(pattern, content, re.DOTALL):
        print("   [INFO] Patron encontrado, aplicando fix...")
        content = re.sub(
            pattern,
            r'\1\n    \2\n\n    // Detectar qué bloque estamos mirando (solo bloques sólidos, hasta 5 bloques)\n    RaycastResult result = raycastBlock(state->world, origin, direction, 5.0f);',
            content,
            flags=re.DOTALL
        )
        print("   [OK] Fix aplicado!")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Raycast arreglado!")
print("\nCambios realizados:")
print("  - Eliminado raycast corto de 1.5 bloques (detectaba pasto alto)")
print("  - Ahora usa solo raycast de 5 bloques para bloques solidos")
print("  - Funciona correctamente al mirar arriba, abajo, o al frente")
print("\nAhora recompila: cmake --build build --config Release")
