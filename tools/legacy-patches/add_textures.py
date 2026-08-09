#!/usr/bin/env python3
import re

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find and replace the buildChunkMesh function
old_function = r'''    void buildChunkMesh\(Chunk\* chunk\) \{
        if \(!chunk->needsRebuild\) return;

        if \(chunk->displayList\) \{
            glDeleteLists\(chunk->displayList, 1\);
        \}

        chunk->displayList = glGenLists\(1\);
        glNewList\(chunk->displayList, GL_COMPILE\);

        glBegin\(GL_QUADS\);

        int facesRendered = 0;  // Contador para debug

        for \(int x = 0; x < CHUNK_SIZE; x\+\+\) \{
            for \(int z = 0; z < CHUNK_SIZE; z\+\+\) \{
                for \(int y = 0; y < CHUNK_HEIGHT; y\+\+\) \{
                    BlockType block = chunk->getBlock\(x, y, z\);
                    if \(block == BLOCK_AIR\) continue;

                    int worldX = chunk->position\.x \* CHUNK_SIZE \+ x;
                    int worldY = y;
                    int worldZ = chunk->position\.z \* CHUNK_SIZE \+ z;

                    float wx = \(float\)worldX;
                    float wy = \(float\)worldY;
                    float wz = \(float\)worldZ;

                    float r, g, b;
                    getBlockColor\(block, r, g, b\);

                    // Obtener nivel de luz del bloque \(0-18\)
                    unsigned char lightLevel = chunk->getLightLevel\(x, y, z\);
                    float lightFactor = \(float\)lightLevel / 18\.0f; // 0\.0 a 1\.0
                    // Asegurar luz mínima para que no sea completamente negro
                    if \(lightFactor < 0\.05f\) lightFactor = 0\.05f;

                    // Top face \(\+Y\)
                    BlockType topNeighbor = getBlockInChunk\(worldX, worldY \+ 1, worldZ\);
                    if \(shouldRenderFace\(block, topNeighbor\)\) \{
                        glColor3f\(r \* lightFactor, g \* lightFactor, b \* lightFactor\);
                        glVertex3f\(wx, wy \+ 1, wz\);
                        glVertex3f\(wx, wy \+ 1, wz \+ 1\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz \+ 1\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz\);
                        facesRendered\+\+;
                    \}

                    // Bottom face \(-Y\)
                    BlockType bottomNeighbor = getBlockInChunk\(worldX, worldY - 1, worldZ\);
                    if \(shouldRenderFace\(block, bottomNeighbor\)\) \{
                        glColor3f\(r \* 0\.5f \* lightFactor, g \* 0\.5f \* lightFactor, b \* 0\.5f \* lightFactor\);
                        // Orden anti-horario visto desde abajo \(para back-face culling correcto\)
                        glVertex3f\(wx, wy, wz\);
                        glVertex3f\(wx \+ 1, wy, wz\);
                        glVertex3f\(wx \+ 1, wy, wz \+ 1\);
                        glVertex3f\(wx, wy, wz \+ 1\);
                        facesRendered\+\+;
                    \}

                    // North face \(\+Z\)
                    BlockType northNeighbor = getBlockInChunk\(worldX, worldY, worldZ \+ 1\);
                    if \(shouldRenderFace\(block, northNeighbor\)\) \{
                        glColor3f\(r \* 0\.8f \* lightFactor, g \* 0\.8f \* lightFactor, b \* 0\.8f \* lightFactor\);
                        glVertex3f\(wx, wy, wz \+ 1\);
                        glVertex3f\(wx \+ 1, wy, wz \+ 1\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz \+ 1\);
                        glVertex3f\(wx, wy \+ 1, wz \+ 1\);
                        facesRendered\+\+;
                    \}

                    // South face \(-Z\)
                    BlockType southNeighbor = getBlockInChunk\(worldX, worldY, worldZ - 1\);
                    if \(shouldRenderFace\(block, southNeighbor\)\) \{
                        glColor3f\(r \* 0\.8f \* lightFactor, g \* 0\.8f \* lightFactor, b \* 0\.8f \* lightFactor\);
                        glVertex3f\(wx \+ 1, wy, wz\);
                        glVertex3f\(wx, wy, wz\);
                        glVertex3f\(wx, wy \+ 1, wz\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz\);
                        facesRendered\+\+;
                    \}

                    // East face \(\+X\)
                    BlockType eastNeighbor = getBlockInChunk\(worldX \+ 1, worldY, worldZ\);
                    if \(shouldRenderFace\(block, eastNeighbor\)\) \{
                        glColor3f\(r \* 0\.6f \* lightFactor, g \* 0\.6f \* lightFactor, b \* 0\.6f \* lightFactor\);
                        glVertex3f\(wx \+ 1, wy, wz \+ 1\);
                        glVertex3f\(wx \+ 1, wy, wz\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz\);
                        glVertex3f\(wx \+ 1, wy \+ 1, wz \+ 1\);
                        facesRendered\+\+;
                    \}

                    // West face \(-X\)
                    BlockType westNeighbor = getBlockInChunk\(worldX - 1, worldY, worldZ\);
                    if \(shouldRenderFace\(block, westNeighbor\)\) \{
                        glColor3f\(r \* 0\.6f \* lightFactor, g \* 0\.6f \* lightFactor, b \* 0\.6f \* lightFactor\);
                        glVertex3f\(wx, wy, wz\);
                        glVertex3f\(wx, wy, wz \+ 1\);
                        glVertex3f\(wx, wy \+ 1, wz \+ 1\);
                        glVertex3f\(wx, wy \+ 1, wz\);
                        facesRendered\+\+;
                    \}
                \}
            \}
        \}

        glEnd\(\);
        glEndList\(\);

        chunk->needsRebuild = false;
    \}'''

new_function = '''    void buildChunkMesh(Chunk* chunk) {
        if (!chunk->needsRebuild) return;

        if (chunk->displayList) {
            glDeleteLists(chunk->displayList, 1);
        }

        chunk->displayList = glGenLists(1);
        glNewList(chunk->displayList, GL_COMPILE);

        // Habilitar texturas
        glEnable(GL_TEXTURE_2D);

        int facesRendered = 0;  // Contador para debug

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    BlockType block = chunk->getBlock(x, y, z);
                    if (block == BLOCK_AIR) continue;

                    int worldX = chunk->position.x * CHUNK_SIZE + x;
                    int worldY = y;
                    int worldZ = chunk->position.z * CHUNK_SIZE + z;

                    float wx = (float)worldX;
                    float wy = (float)worldY;
                    float wz = (float)worldZ;

                    // Obtener nivel de luz del bloque (0-18)
                    unsigned char lightLevel = chunk->getLightLevel(x, y, z);
                    float lightFactor = (float)lightLevel / 18.0f; // 0.0 a 1.0
                    // Asegurar luz mínima para que no sea completamente negro
                    if (lightFactor < 0.05f) lightFactor = 0.05f;

                    // Top face (+Y) - face index 0
                    BlockType topNeighbor = getBlockInChunk(worldX, worldY + 1, worldZ);
                    if (shouldRenderFace(block, topNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 0);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(lightFactor, lightFactor, lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(0, 0); glVertex3f(wx, wy + 1, wz);
                        glTexCoord2f(0, 1); glVertex3f(wx, wy + 1, wz + 1);
                        glTexCoord2f(1, 1); glVertex3f(wx + 1, wy + 1, wz + 1);
                        glTexCoord2f(1, 0); glVertex3f(wx + 1, wy + 1, wz);
                        glEnd();
                        facesRendered++;
                    }

                    // Bottom face (-Y) - face index 1
                    BlockType bottomNeighbor = getBlockInChunk(worldX, worldY - 1, worldZ);
                    if (shouldRenderFace(block, bottomNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 1);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(0.5f * lightFactor, 0.5f * lightFactor, 0.5f * lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(0, 0); glVertex3f(wx, wy, wz);
                        glTexCoord2f(1, 0); glVertex3f(wx + 1, wy, wz);
                        glTexCoord2f(1, 1); glVertex3f(wx + 1, wy, wz + 1);
                        glTexCoord2f(0, 1); glVertex3f(wx, wy, wz + 1);
                        glEnd();
                        facesRendered++;
                    }

                    // North face (+Z) - face index 2
                    BlockType northNeighbor = getBlockInChunk(worldX, worldY, worldZ + 1);
                    if (shouldRenderFace(block, northNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 2);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(0.8f * lightFactor, 0.8f * lightFactor, 0.8f * lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(0, 0); glVertex3f(wx, wy, wz + 1);
                        glTexCoord2f(1, 0); glVertex3f(wx + 1, wy, wz + 1);
                        glTexCoord2f(1, 1); glVertex3f(wx + 1, wy + 1, wz + 1);
                        glTexCoord2f(0, 1); glVertex3f(wx, wy + 1, wz + 1);
                        glEnd();
                        facesRendered++;
                    }

                    // South face (-Z) - face index 3
                    BlockType southNeighbor = getBlockInChunk(worldX, worldY, worldZ - 1);
                    if (shouldRenderFace(block, southNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 3);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(0.8f * lightFactor, 0.8f * lightFactor, 0.8f * lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(1, 0); glVertex3f(wx + 1, wy, wz);
                        glTexCoord2f(0, 0); glVertex3f(wx, wy, wz);
                        glTexCoord2f(0, 1); glVertex3f(wx, wy + 1, wz);
                        glTexCoord2f(1, 1); glVertex3f(wx + 1, wy + 1, wz);
                        glEnd();
                        facesRendered++;
                    }

                    // East face (+X) - face index 4
                    BlockType eastNeighbor = getBlockInChunk(worldX + 1, worldY, worldZ);
                    if (shouldRenderFace(block, eastNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 4);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(0.6f * lightFactor, 0.6f * lightFactor, 0.6f * lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(0, 0); glVertex3f(wx + 1, wy, wz + 1);
                        glTexCoord2f(1, 0); glVertex3f(wx + 1, wy, wz);
                        glTexCoord2f(1, 1); glVertex3f(wx + 1, wy + 1, wz);
                        glTexCoord2f(0, 1); glVertex3f(wx + 1, wy + 1, wz + 1);
                        glEnd();
                        facesRendered++;
                    }

                    // West face (-X) - face index 5
                    BlockType westNeighbor = getBlockInChunk(worldX - 1, worldY, worldZ);
                    if (shouldRenderFace(block, westNeighbor)) {
                        GLuint texture = g_textureManager->getBlockTexture(block, 5);
                        glBindTexture(GL_TEXTURE_2D, texture);

                        glColor3f(0.6f * lightFactor, 0.6f * lightFactor, 0.6f * lightFactor);
                        glBegin(GL_QUADS);
                        glTexCoord2f(1, 0); glVertex3f(wx, wy, wz);
                        glTexCoord2f(0, 0); glVertex3f(wx, wy, wz + 1);
                        glTexCoord2f(0, 1); glVertex3f(wx, wy + 1, wz + 1);
                        glTexCoord2f(1, 1); glVertex3f(wx, wy + 1, wz);
                        glEnd();
                        facesRendered++;
                    }
                }
            }
        }

        glDisable(GL_TEXTURE_2D);
        glEndList();

        chunk->needsRebuild = false;
    }'''

# Find the function
start_marker = "    void buildChunkMesh(Chunk* chunk) {"
start_idx = content.find(start_marker)
if start_idx == -1:
    print("ERROR: Could not find buildChunkMesh function")
    exit(1)

# Find the next occurrence of "void " after the function to know where it ends
next_function_idx = content.find("\n    void ", start_idx + 1)
if next_function_idx == -1:
    next_function_idx = len(content)

# Now find "chunk->needsRebuild = false;" within this range
end_marker = "chunk->needsRebuild = false;"
end_idx = content.find(end_marker, start_idx, next_function_idx)
if end_idx == -1:
    print("ERROR: Could not find 'chunk->needsRebuild = false;' in buildChunkMesh")
    exit(1)

# Find the closing brace after that line
end_idx = content.find("\n    }", end_idx)
if end_idx == -1:
    print("ERROR: Could not find closing brace")
    exit(1)
end_idx += len("\n    }")

# Replace the function
new_content = content[:start_idx] + new_function + content[end_idx:]

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(new_content)

print("Successfully modified buildChunkMesh function to include texture support!")
print("Changes:")
print("  - Added glEnable(GL_TEXTURE_2D) at start")
print("  - Added glDisable(GL_TEXTURE_2D) at end")
print("  - Bound appropriate textures for each face")
print("  - Added UV coordinates (glTexCoord2f) for all vertices")
print("  - Removed color multipliers (now using white color with lighting)")
