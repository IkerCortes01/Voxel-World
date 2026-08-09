#!/usr/bin/env python3
# Script para invertir completamente el vector de dirección

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# INVERTIR COMPLETAMENTE EL VECTOR getForward()
# ============================================================================
print("[INFO] Invirtiendo vector completo de getForward()...")

old_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara \(incluye pitch\)
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            cosf\(pitchRad\) \* sinf\(yawRad\),
            sinf\(pitchRad\),  // [^\n]*
            cosf\(pitchRad\) \* -cosf\(yawRad\)
        \)\.normalize\(\);
    \}'''

new_forward = '''    Vec3 getForward() const {
        // Vector 3D para la cámara (incluye pitch) - INVERTIDO para raycast
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            -cosf(pitchRad) * sinf(yawRad),     // Invertido X
            -sinf(pitchRad),                     // Invertido Y
            -cosf(pitchRad) * -cosf(yawRad)      // Z = cosf * cosf (doble negativo)
        ).normalize();
    }'''

if re.search(old_forward, content, re.DOTALL):
    content = re.sub(old_forward, new_forward, content, re.DOTALL)
    print("   [OK] Vector completo invertido (X, Y, Z)")
else:
    print("   [ERROR] No se encontro el patron")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Vector de direccion completamente invertido!")
print("\nCambios:")
print("  - X: Invertido (izquierda/derecha corregido)")
print("  - Y: Invertido (arriba/abajo corregido)")
print("  - Z: Doble negativo = positivo (adelante/atras corregido)")
print("\nCompila: cmake --build build --config Release")
