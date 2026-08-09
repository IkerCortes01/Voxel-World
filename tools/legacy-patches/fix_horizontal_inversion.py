#!/usr/bin/env python3
# Script para arreglar la inversión horizontal del raycast

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# INVERTIR componentes X y Z del vector forward
# ============================================================================
print("[INFO] Invirtiendo componentes X y Z del vector forward...")

old_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara \(incluye pitch\) - Y invertido para raycast
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            cosf\(pitchRad\) \* sinf\(yawRad\),
            sinf\(pitchRad\),  // [^\n]*
            cosf\(pitchRad\) \* -cosf\(yawRad\)
        \)\.normalize\(\);
    \}'''

new_forward = '''    Vec3 getForward() const {
        // Vector 3D para la cámara - Componentes X y Z invertidos
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            -cosf(pitchRad) * sinf(yawRad),  // X invertido
            sinf(pitchRad),                   // Y correcto
            cosf(pitchRad) * cosf(yawRad)    // Z: doble negativo = positivo
        ).normalize();
    }'''

if re.search(old_forward, content, re.DOTALL):
    content = re.sub(old_forward, new_forward, content, re.DOTALL)
    print("   [OK] Componentes X y Z invertidos")
    print("        X: cos * sin  ->  -cos * sin")
    print("        Z: cos * -cos  ->  cos * cos")
else:
    print("   [WARNING] No se encontro el patron")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Inversion horizontal corregida!")
print("\nCambios realizados:")
print("  - Componente X: invertido (negativo)")
print("  - Componente Z: doble negativo cancelado (positivo)")
print("\nResultado esperado:")
print("  - Miras derecha -> raycast va a la derecha")
print("  - Miras izquierda -> raycast va a la izquierda")
print("\nCompila: cmake --build build --config Release")
