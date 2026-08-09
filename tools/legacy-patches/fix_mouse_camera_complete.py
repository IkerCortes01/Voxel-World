#!/usr/bin/env python3
# Script para arreglar completamente el mouse y la cámara

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# FIX 1: Arreglar callback del mouse - invertir pitch
# ============================================================================
print("[INFO] Arreglando callback del mouse (pitch)...")

old_callback = r'    g_gameState->player\.yaw -= \(float\)xoffset;  // [^\n]*\n    g_gameState->player\.pitch \+= \(float\)yoffset;'

new_callback = '''    g_gameState->player.yaw -= (float)xoffset;  // Mouse derecha = yaw disminuye = gira derecha
    g_gameState->player.pitch -= (float)yoffset;  // Mouse arriba = pitch disminuye = mirar arriba'''

if re.search(old_callback, content):
    content = re.sub(old_callback, new_callback, content)
    print("   [OK] Pitch invertido: ahora pitch -= yoffset")
else:
    print("   [WARNING] No se encontro el patron exacto")

# ============================================================================
# FIX 2: Restaurar getForward() al original (con -sinf)
# ============================================================================
print("[INFO] Restaurando getForward() al original...")

# Buscar la versión que ya fue modificada
modified_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara \(incluye pitch\)
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            cosf\(pitchRad\) \* sinf\(yawRad\),
            sinf\(pitchRad\),  // [^\n]*
            cosf\(pitchRad\) \* -cosf\(yawRad\)
        \)\.normalize\(\);
    \}'''

original_forward = '''    Vec3 getForward() const {
        // Vector 3D para la cámara (incluye pitch)
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            cosf(pitchRad) * sinf(yawRad),
            -sinf(pitchRad),  // Pitch negativo = arriba (Y positivo)
            cosf(pitchRad) * -cosf(yawRad)
        ).normalize();
    }'''

if re.search(modified_forward, content, re.DOTALL):
    content = re.sub(modified_forward, original_forward, content, re.DOTALL)
    print("   [OK] getForward() restaurado con -sinf(pitchRad)")
else:
    print("   [INFO] getForward() ya tiene el formato correcto o no se encontro")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Mouse y camara arreglados completamente!")
print("\nCambios realizados:")
print("  1. Callback del mouse:")
print("     - pitch -= yoffset (antes era +=)")
print("     - Ahora: Mouse arriba = pitch disminuye = mirar arriba")
print("\n  2. Funcion getForward():")
print("     - Y = -sinf(pitchRad)")
print("     - pitch negativo → sin negativo → Y positivo → raycast arriba")
print("\nCompila: cmake --build build --config Release")
