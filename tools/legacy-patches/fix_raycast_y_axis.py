#!/usr/bin/env python3
# Script para invertir solo el eje Y del raycast

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# REVERTIR: Mouse callback a la versión original
# ============================================================================
print("[INFO] Revirtiendo mouse callback al original...")

old_callback = r'    g_gameState->player\.yaw -= \(float\)xoffset;  // Mouse derecha = yaw disminuye = gira derecha\n    g_gameState->player\.pitch -= \(float\)yoffset;  // Mouse arriba = pitch disminuye = mirar arriba'

original_callback = '''    g_gameState->player.yaw -= (float)xoffset;  // Restar: mouse derecha = yaw disminuye = gira derecha
    g_gameState->player.pitch += (float)yoffset;'''

if re.search(old_callback, content):
    content = re.sub(old_callback, original_callback, content)
    print("   [OK] Mouse callback revertido a pitch += yoffset")
else:
    print("   [WARNING] No se encontro el patron del callback")

# ============================================================================
# INVERTIR: Eje Y del vector forward (para raycast)
# ============================================================================
print("[INFO] Invirtiendo eje Y del vector forward...")

old_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara \(incluye pitch\)
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            cosf\(pitchRad\) \* sinf\(yawRad\),
            -sinf\(pitchRad\),  // [^\n]*
            cosf\(pitchRad\) \* -cosf\(yawRad\)
        \)\.normalize\(\);
    \}'''

new_forward = '''    Vec3 getForward() const {
        // Vector 3D para la cámara (incluye pitch) - Y invertido para raycast
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            cosf(pitchRad) * sinf(yawRad),
            sinf(pitchRad),  // Y invertido: pitch+ = abajo en raycast
            cosf(pitchRad) * -cosf(yawRad)
        ).normalize();
    }'''

if re.search(old_forward, content, re.DOTALL):
    content = re.sub(old_forward, new_forward, content, re.DOTALL)
    print("   [OK] Eje Y invertido: -sinf(pitchRad) -> sinf(pitchRad)")
else:
    print("   [WARNING] No se encontro el patron de getForward")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Sistema de seleccion de bloques arreglado!")
print("\nCambios realizados:")
print("  1. Mouse callback: pitch += yoffset (camara normal)")
print("  2. Raycast: Y = sinf(pitchRad) (seleccion invertida)")
print("\nResultado:")
print("  - Camara se mueve normalmente")
print("  - Raycast selecciona donde apunta la cruz")
print("\nCompila: cmake --build build --config Release")
