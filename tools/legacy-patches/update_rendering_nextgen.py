#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find buildChunkMesh and update lighting calculations
# Find the lighting calculation part
old_lighting_calc = """                    // Obtener nivel de luz del bloque (0-18)
                    unsigned char lightLevel = chunk->getLightLevel(x, y, z);
                    float lightFactor = (float)lightLevel / 18.0f; // 0.0 a 1.0
                    // Asegurar luz mínima para que no sea completamente negro
                    if (lightFactor < 0.05f) lightFactor = 0.05f;"""

new_lighting_calc = """                    // NEXT-GEN LIGHTING CALCULATION
                    uint8_t lightLevel = chunk->getLightLevel(x, y, z);

                    // GAMMA CURVE para oscuridad realista (no lineal)
                    float rawLight = (float)lightLevel / 18.0f;
                    float lightFactor = pow(rawLight, 1.4f); // Gamma 1.4

                    // Luz mínima para visibilidad en cuevas
                    if (lightFactor < 0.05f) lightFactor = 0.05f;

                    // COLORED LIGHTING - Obtener color de luz
                    float lightColorR, lightColorG, lightColorB;
                    chunk->getLightColor(x, y, z, lightColorR, lightColorG, lightColorB);"""

content = content.replace(old_lighting_calc, new_lighting_calc)

# Update each face rendering to use colored lighting
# Top face
old_top_face = """                        glColor3f(lightFactor, lightFactor, lightFactor);"""

new_top_face = """                        // Aplicar color de luz (con multiplicador de cara)
                        float faceBrightness = 1.0f; // Top = más brillante
                        glColor3f(lightFactor * lightColorR * faceBrightness,
                                 lightFactor * lightColorG * faceBrightness,
                                 lightFactor * lightColorB * faceBrightness);"""

# Find and replace in top face context
top_face_context = """                    if (shouldRenderFace(block, topNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 0);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(lightFactor, lightFactor, lightFactor);"""

new_top_face_context = """                    if (shouldRenderFace(block, topNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 0);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        // Aplicar color de luz (con multiplicador de cara)
                        float faceBrightness = 1.0f; // Top = más brillante
                        glColor3f(lightFactor * lightColorR * faceBrightness,
                                 lightFactor * lightColorG * faceBrightness,
                                 lightFactor * lightColorB * faceBrightness);"""

content = content.replace(top_face_context, new_top_face_context)

# Bottom face
bottom_face_context = """                        glColor3f(0.5f * lightFactor, 0.5f * lightFactor, 0.5f * lightFactor);"""

new_bottom_face_context = """                        faceBrightness = 0.5f; // Bottom = más oscuro
                        glColor3f(lightFactor * lightColorR * faceBrightness,
                                 lightFactor * lightColorG * faceBrightness,
                                 lightFactor * lightColorB * faceBrightness);"""

content = content.replace(bottom_face_context, new_bottom_face_context)

# North/South faces (0.8f)
north_face_context = """                        glColor3f(0.8f * lightFactor, 0.8f * lightFactor, 0.8f * lightFactor);"""

new_north_south_context = """                        faceBrightness = 0.8f; // N/S faces
                        glColor3f(lightFactor * lightColorR * faceBrightness,
                                 lightFactor * lightColorG * faceBrightness,
                                 lightFactor * lightColorB * faceBrightness);"""

content = content.replace(north_face_context, new_north_south_context)

# East/West faces (0.6f)
east_face_context = """                        glColor3f(0.6f * lightFactor, 0.6f * lightFactor, 0.6f * lightFactor);"""

new_east_west_context = """                        faceBrightness = 0.6f; // E/W faces = más oscuro
                        glColor3f(lightFactor * lightColorR * faceBrightness,
                                 lightFactor * lightColorG * faceBrightness,
                                 lightFactor * lightColorB * faceBrightness);"""

content = content.replace(east_face_context, new_east_west_context)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("NEXT-GEN RENDERING implementado!")
print("\nMejoras:")
print("  - Gamma curve (pow 1.4) para oscuridad realista")
print("  - Colored lighting RGB aplicado a cada cara")
print("  - Multiplicadores de brillo por cara:")
print("    * Top: 1.0 (mas brillante)")
print("    * North/South: 0.8")
print("    * East/West: 0.6 (mas oscuro)")
print("    * Bottom: 0.5 (el mas oscuro)")
