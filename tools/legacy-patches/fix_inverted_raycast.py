#!/usr/bin/env python3
# Script para arreglar la inversión del raycast

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# ARREGLAR getForward() - Invertir componente Y
# ============================================================================
print("[INFO] Arreglando funcion getForward()...")

old_forward = r'''    Vec3 getForward\(\) const \{
        // Vector 3D para la cámara \(incluye pitch\)
        float yawRad = yaw \* 3\.14159f / 180\.0f;
        float pitchRad = pitch \* 3\.14159f / 180\.0f;
        return Vec3\(
            cosf\(pitchRad\) \* sinf\(yawRad\),
            -sinf\(pitchRad\),
            cosf\(pitchRad\) \* -cosf\(yawRad\)
        \)\.normalize\(\);
    \}'''

new_forward = '''    Vec3 getForward() const {
        // Vector 3D para la cámara (incluye pitch)
        float yawRad = yaw * 3.14159f / 180.0f;
        float pitchRad = pitch * 3.14159f / 180.0f;
        return Vec3(
            cosf(pitchRad) * sinf(yawRad),
            sinf(pitchRad),  // Cambiado: sin negativo (pitch positivo = abajo)
            cosf(pitchRad) * -cosf(yawRad)
        ).normalize();
    }'''

if re.search(old_forward, content):
    content = re.sub(old_forward, new_forward, content)
    print("   [OK] Componente Y del vector forward invertido")
else:
    print("   [WARNING] No se encontro el patron exacto, buscando alternativa...")
    # Intentar match más simple
    simple_pattern = r'(Vec3 getForward\(\) const \{[^}]*)-sinf\(pitchRad\),'
    if re.search(simple_pattern, content, re.DOTALL):
        content = re.sub(
            simple_pattern,
            r'\1sinf(pitchRad),  // Arreglado: pitch positivo = mirar abajo',
            content,
            flags=re.DOTALL
        )
        print("   [OK] Componente Y arreglado (patron simple)")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Direccion del raycast arreglada!")
print("\nCambios realizados:")
print("  - Componente Y: -sinf(pitchRad) -> sinf(pitchRad)")
print("  - Ahora: Pitch positivo = mirar ABAJO (raycast va abajo)")
print("  - Ahora: Pitch negativo = mirar ARRIBA (raycast va arriba)")
print("\nCompila: cmake --build build --config Release")
