#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# FIX 1: Aumentar luz ambiental mínima
old_ambient = """                    // Luz mínima para visibilidad en cuevas
                    if (lightFactor < 0.05f) lightFactor = 0.05f;"""

new_ambient = """                    // LUZ AMBIENTAL MÍNIMA (nunca negro absoluto)
                    if (lightFactor < 0.15f) lightFactor = 0.15f;  // 15% ambient light"""

content = content.replace(old_ambient, new_ambient)

# FIX 2: Reducir gamma (era demasiado oscuro)
old_gamma = """                    // GAMMA CURVE para oscuridad realista (no lineal)
                    float rawLight = (float)lightLevel / 18.0f;
                    float lightFactor = pow(rawLight, 1.4f); // Gamma 1.4"""

new_gamma = """                    // GAMMA CURVE para oscuridad realista (no lineal)
                    float rawLight = (float)lightLevel / 18.0f;

                    // Si NO hay luz calculada, usar luz ambiental temporal
                    if (rawLight == 0.0f) {
                        rawLight = 0.8f;  // 80% luz temporal mientras se calcula
                    }

                    float lightFactor = pow(rawLight, 1.2f); // Gamma 1.2 (menos agresivo)"""

content = content.replace(old_gamma, new_gamma)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("LIGHTING RENDERING FIXED!")
print("\nCambios:")
print("  1. Luz ambiental: 0.05 -> 0.15 (15%)")
print("  2. Gamma curve: 1.4 -> 1.2 (menos oscuro)")
print("  3. Luz temporal: 80% mientras se calcula iluminacion")
print("\nResultado:")
print("  - NUNCA habra negro absoluto")
print("  - Mundo visible mientras se calcula luz")
print("  - Gamma mas suave")
