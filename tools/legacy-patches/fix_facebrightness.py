#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Add faceBrightness declaration
old_code = """                    // COLORED LIGHTING - Obtener color de luz
                    float lightColorR, lightColorG, lightColorB;
                    chunk->getLightColor(x, y, z, lightColorR, lightColorG, lightColorB);"""

new_code = """                    // COLORED LIGHTING - Obtener color de luz
                    float lightColorR, lightColorG, lightColorB;
                    chunk->getLightColor(x, y, z, lightColorR, lightColorG, lightColorB);

                    // Variable para multiplicador de brillo por cara
                    float faceBrightness;"""

content = content.replace(old_code, new_code)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Added faceBrightness variable declaration!")
