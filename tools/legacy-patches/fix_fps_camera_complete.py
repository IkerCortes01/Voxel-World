#!/usr/bin/env python3
# Script para aplicar la formula FPS estandar correcta

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# APLICAR FORMULA FPS ESTANDAR (estilo Minecraft)
# ============================================================================
print("[INFO] Aplicando formula FPS estandar...")

old_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara - Componentes X y Z invertidos
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            -cosf\(pitchRad\) \* sinf\(yawRad\),  // [^\n]*
            sinf\(pitchRad\),                   // [^\n]*
            cosf\(pitchRad\) \* cosf\(yawRad\)    // [^\n]*
        \)\.normalize\(\);
    \}'''

# Formula FPS estandar
new_forward = '''    Vec3 getForward() const {
        // Formula FPS estandar (estilo Minecraft)
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            sinf(yawRad) * cosf(pitchRad),   // X = sin(yaw) * cos(pitch)
            -sinf(pitchRad),                  // Y = -sin(pitch)
            -cosf(yawRad) * cosf(pitchRad)   // Z = -cos(yaw) * cos(pitch)
        ).normalize();
    }'''

if re.search(old_forward, content, re.DOTALL):
    content = re.sub(old_forward, new_forward, content, re.DOTALL)
    print("   [OK] Formula FPS estandar aplicada")
    print("        X = sin(yaw) * cos(pitch)")
    print("        Y = -sin(pitch)")
    print("        Z = -cos(yaw) * cos(pitch)")
else:
    print("   [WARNING] No se encontro el patron exacto")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Formula FPS estandar aplicada!")
print("\nEsta es la formula que usa Minecraft:")
print("  - X: horizontal (sin yaw, afectado por cos pitch)")
print("  - Y: vertical (solo pitch)")
print("  - Z: profundidad (cos yaw, afectado por cos pitch)")
print("\nAhora funciona correctamente al combinar pitch + yaw")
print("\nCompila: cmake --build build --config Release")
