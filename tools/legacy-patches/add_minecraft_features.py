#!/usr/bin/env python3
# Script para añadir características estilo Minecraft

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# 1. AÑADIR SISTEMA DE PARTÍCULAS
# ============================================================================
print("[INFO] Anadiendo sistema de particulas...")

particle_system = '''
// ============================================================================
// SISTEMA DE PARTÍCULAS (estilo Minecraft)
// ============================================================================

struct Particle {
    Vec3 position;
    Vec3 velocity;
    float r, g, b;
    float life;
    float maxLife;
    float size;

    Particle() : life(0), maxLife(0), size(0.05f) {}

    Particle(Vec3 pos, Vec3 vel, float red, float green, float blue, float lifetime = 1.0f)
        : position(pos), velocity(vel), r(red), g(green), b(blue), life(lifetime), maxLife(lifetime), size(0.05f) {}

    void update(float deltaTime) {
        life -= deltaTime;
        velocity.y -= 9.8f * deltaTime; // Gravedad
        position = position + velocity * deltaTime;
    }

    bool isDead() const {
        return life <= 0;
    }
};

class ParticleSystem {
public:
    std::vector<Particle> particles;

    void spawnBlockBreakParticles(Vec3 blockPos, BlockType blockType) {
        float r, g, b;
        getBlockColor(blockType, r, g, b);

        // Generar 15-20 partículas
        int count = 15 + (rand() % 6);
        for (int i = 0; i < count; i++) {
            Vec3 pos = blockPos + Vec3(
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f
            );

            Vec3 vel = Vec3(
                ((rand() % 200) - 100) / 100.0f,
                ((rand() % 150) + 50) / 100.0f,
                ((rand() % 200) - 100) / 100.0f
            );

            // Variación de color
            float colorVar = 0.8f + (rand() % 40) / 100.0f;
            particles.push_back(Particle(pos, vel, r * colorVar, g * colorVar, b * colorVar, 0.6f + (rand() % 40) / 100.0f));
        }
    }

    void update(float deltaTime) {
        for (auto& p : particles) {
            p.update(deltaTime);
        }

        // Eliminar partículas muertas
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return p.isDead(); }),
            particles.end()
        );
    }

    void render() {
        if (particles.empty()) return;

        glPointSize(4.0f);
        glBegin(GL_POINTS);

        for (const auto& p : particles) {
            float alpha = p.life / p.maxLife;
            glColor4f(p.r, p.g, p.b, alpha);
            glVertex3f(p.position.x, p.position.y, p.position.z);
        }

        glEnd();
    }
};

'''

# Buscar después de la definición de InventorySlot
insert_pos = content.find('struct InventorySlot {')
if insert_pos != -1:
    # Retroceder hasta encontrar el comentario anterior
    while insert_pos > 0 and content[insert_pos-1] != '\n':
        insert_pos -= 1
    content = content[:insert_pos] + particle_system + content[insert_pos:]
    print("   [OK] Sistema de particulas anadido")
else:
    print("   [ERROR] No se encontro struct InventorySlot")

# ============================================================================
# 2. AÑADIR PARTÍCULAS AL GAMEST STATE
# ============================================================================
print("[INFO] Anadiendo particulas a GameState...")

# Buscar struct GameState y añadir el sistema de partículas
gamestate_pattern = r'(struct GameState \{[^}]*World world;)'
if re.search(gamestate_pattern, content):
    content = re.sub(
        gamestate_pattern,
        r'\1\n    ParticleSystem particles;',
        content
    )
    print("   [OK] ParticleSystem anadido a GameState")

# ============================================================================
# 3. MODIFICAR updateMining PARA GENERAR PARTÍCULAS
# ============================================================================
print("[INFO] Modificando updateMining para generar particulas...")

old_break = r'        // Romper el bloque\n        state->world\.setBlock\(result\.blockPos\.x, result\.blockPos\.y, result\.blockPos\.z, BLOCK_AIR\);'
new_break = '''        // Romper el bloque
        state->world.setBlock(result.blockPos.x, result.blockPos.y, result.blockPos.z, BLOCK_AIR);

        // Generar partículas de rotura
        state->particles.spawnBlockBreakParticles(
            Vec3(result.blockPos.x, result.blockPos.y, result.blockPos.z),
            blockType
        );'''

if re.search(old_break, content):
    content = re.sub(old_break, new_break, content)
    print("   [OK] Particulas anadidas al romper bloques")

# ============================================================================
# 4. AÑADIR FUNCIÓN PARA RENDERIZAR HOTBAR
# ============================================================================
print("[INFO] Anadiendo funcion renderHotbar...")

hotbar_function = '''
// ============================================================================
// RENDERIZADO DE HOTBAR (estilo Minecraft)
// ============================================================================

void renderHotbar(Inventory* inventory, int width, int height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float slotSize = 50.0f;
    float spacing = 4.0f;
    float totalWidth = 9 * slotSize + 8 * spacing;
    float startX = (width - totalWidth) / 2.0f;
    float startY = height - 80.0f;

    // Renderizar los 9 slots del hotbar
    for (int i = 0; i < 9; i++) {
        float x = startX + i * (slotSize + spacing);
        float y = startY;

        // Fondo del slot (gris oscuro semi-transparente)
        if (i == inventory->selectedSlot) {
            // Slot seleccionado (más claro y con borde blanco)
            glColor4f(0.5f, 0.5f, 0.5f, 0.8f);
        } else {
            glColor4f(0.2f, 0.2f, 0.2f, 0.7f);
        }

        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + slotSize, y);
        glVertex2f(x + slotSize, y + slotSize);
        glVertex2f(x, y + slotSize);
        glEnd();

        // Borde del slot
        if (i == inventory->selectedSlot) {
            glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // Blanco para seleccionado
            glLineWidth(3);
        } else {
            glColor4f(0.5f, 0.5f, 0.5f, 0.9f); // Gris para no seleccionados
            glLineWidth(2);
        }

        glBegin(GL_LINE_LOOP);
        glVertex2f(x, y);
        glVertex2f(x + slotSize, y);
        glVertex2f(x + slotSize, y + slotSize);
        glVertex2f(x, y + slotSize);
        glEnd();

        // Renderizar el bloque (si hay)
        InventorySlot& slot = inventory->slots[i];
        if (!slot.isEmpty()) {
            float r, g, b;
            getBlockColor(slot.blockType, r, g, b);

            // Cubo pequeño representando el bloque
            float blockSize = 25.0f;
            float blockX = x + (slotSize - blockSize) / 2.0f;
            float blockY = y + (slotSize - blockSize) / 2.0f - 5.0f;

            glColor4f(r, g, b, 1.0f);
            glBegin(GL_QUADS);
            // Cara frontal del cubo simplificado
            glVertex2f(blockX, blockY);
            glVertex2f(blockX + blockSize, blockY);
            glVertex2f(blockX + blockSize, blockY + blockSize);
            glVertex2f(blockX, blockY + blockSize);
            glEnd();

            // Borde del bloque
            glColor4f(r * 0.5f, g * 0.5f, b * 0.5f, 1.0f);
            glLineWidth(1);
            glBegin(GL_LINE_LOOP);
            glVertex2f(blockX, blockY);
            glVertex2f(blockX + blockSize, blockY);
            glVertex2f(blockX + blockSize, blockY + blockSize);
            glVertex2f(blockX, blockY + blockSize);
            glEnd();

            // Contador de items (si hay más de 1)
            if (slot.count > 1) {
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
                char countStr[8];
                snprintf(countStr, sizeof(countStr), "%d", slot.count);
                renderText(countStr, x + slotSize - 18, y + slotSize - 18, 12);
            }
        }

        // Número del slot (1-9)
        glColor4f(0.8f, 0.8f, 0.8f, 0.9f);
        char numStr[2];
        snprintf(numStr, sizeof(numStr), "%d", i + 1);
        renderText(numStr, x + 3, y + 3, 10);
    }

    glDisable(GL_BLEND);
}

// Renderizar outline del bloque apuntado (estilo Minecraft)
void renderBlockOutline(Vec3i blockPos, Vec3 cameraPos) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Línea negra gruesa
    glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glLineWidth(3);

    float x = blockPos.x;
    float y = blockPos.y;
    float z = blockPos.z;
    float size = 1.001f; // Ligeramente más grande para evitar z-fighting

    glBegin(GL_LINES);

    // Aristas inferiores
    glVertex3f(x, y, z);         glVertex3f(x + size, y, z);
    glVertex3f(x + size, y, z);  glVertex3f(x + size, y, z + size);
    glVertex3f(x + size, y, z + size); glVertex3f(x, y, z + size);
    glVertex3f(x, y, z + size);  glVertex3f(x, y, z);

    // Aristas superiores
    glVertex3f(x, y + size, z);         glVertex3f(x + size, y + size, z);
    glVertex3f(x + size, y + size, z);  glVertex3f(x + size, y + size, z + size);
    glVertex3f(x + size, y + size, z + size); glVertex3f(x, y + size, z + size);
    glVertex3f(x, y + size, z + size);  glVertex3f(x, y + size, z);

    // Aristas verticales
    glVertex3f(x, y, z);         glVertex3f(x, y + size, z);
    glVertex3f(x + size, y, z);  glVertex3f(x + size, y + size, z);
    glVertex3f(x + size, y, z + size); glVertex3f(x + size, y + size, z + size);
    glVertex3f(x, y, z + size);  glVertex3f(x, y + size, z + size);

    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Renderizar animación de grietas (0-9, como Minecraft)
void renderBlockCrack(Vec3i blockPos, float progress) {
    if (progress <= 0.0f) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);

    // 10 niveles de grietas (0-9)
    int crackLevel = (int)(progress * 10.0f);
    if (crackLevel > 9) crackLevel = 9;

    // Color más oscuro según el nivel de rotura
    float darkness = 1.0f - (crackLevel / 10.0f) * 0.5f;
    glColor4f(darkness, darkness, darkness, 0.8f);

    float x = blockPos.x;
    float y = blockPos.y;
    float z = blockPos.z;
    float size = 1.002f; // Ligeramente más grande

    // Renderizar líneas de grietas
    glLineWidth(2);
    glBegin(GL_LINES);

    // Grietas diagonales (más según el nivel)
    for (int i = 0; i <= crackLevel; i++) {
        float offset = i / 10.0f;
        // Cara frontal
        glVertex3f(x + offset, y, z);
        glVertex3f(x + 1.0f, y + 1.0f - offset, z);

        // Cara superior
        glVertex3f(x + offset, y + size, z);
        glVertex3f(x + 1.0f, y + size, z + 1.0f - offset);
    }

    glEnd();
    glDisable(GL_BLEND);
}

'''

# Buscar donde insertar (antes de la función placeBlock)
insert_pos = content.find('void placeBlock(GameState* state)')
if insert_pos != -1:
    # Retroceder hasta el inicio de línea
    while insert_pos > 0 and content[insert_pos-1] != '\n':
        insert_pos -= 1
    content = content[:insert_pos] + hotbar_function + '\n' + content[insert_pos:]
    print("   [OK] Funciones de renderizado anadidas")

# ============================================================================
# 5. AÑADIR LLAMADAS A UPDATE DE PARTÍCULAS
# ============================================================================
print("[INFO] Anadiendo update de particulas en el loop principal...")

# Buscar el update del placeCooldown y añadir update de partículas
old_cooldown = r'        if \(g_gameState->placeCooldown > 0\) \{\n            g_gameState->placeCooldown -= deltaTime;'
new_cooldown = '''        if (g_gameState->placeCooldown > 0) {
            g_gameState->placeCooldown -= deltaTime;
        }

        // Actualizar partículas
        g_gameState->particles.update(deltaTime);'''

if re.search(old_cooldown, content):
    content = re.sub(old_cooldown, new_cooldown, content)
    print("   [OK] Update de particulas anadido")

# ============================================================================
# 6. AÑADIR RENDERIZADO DE PARTÍCULAS, OUTLINE Y GRIETAS
# ============================================================================
print("[INFO] Anadiendo renderizado de particulas, outline y grietas...")

# Buscar después del renderizado de chunks y antes del HUD
old_hud_setup = r'        glMatrixMode\(GL_PROJECTION\);\n        glLoadIdentity\(\);\n        glOrtho\(0, width, height, 0, -1, 1\);'
new_rendering = '''        // Renderizar partículas (en modo 3D)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        g_gameState->particles.render();
        glDisable(GL_BLEND);

        // Renderizar outline del bloque apuntado
        Vec3 rayOrigin = g_gameState->player.getEyePosition();
        Vec3 rayDirection = g_gameState->player.getForward();
        RaycastResult rayResult = raycastBlock(g_gameState->world, rayOrigin, rayDirection, 5.0f);

        if (rayResult.hit && !g_gameState->isPaused) {
            renderBlockOutline(rayResult.blockPos, g_gameState->player.position);

            // Renderizar grietas si estamos minando este bloque
            if (g_gameState->isMining &&
                rayResult.blockPos.x == g_gameState->miningBlockPos.x &&
                rayResult.blockPos.y == g_gameState->miningBlockPos.y &&
                rayResult.blockPos.z == g_gameState->miningBlockPos.z) {
                renderBlockCrack(rayResult.blockPos, g_gameState->miningProgress);
            }
        }

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, width, height, 0, -1, 1);'''

content = re.sub(old_hud_setup, new_rendering, content)
print("   [OK] Renderizado de particulas, outline y grietas anadido")

# ============================================================================
# 7. AÑADIR RENDERIZADO DEL HOTBAR
# ============================================================================
print("[INFO] Anadiendo renderizado del hotbar...")

# Buscar después del crosshair
old_crosshair = r'        // Renderizar crosshair solo si no está pausado\n        if \(!g_gameState->isPaused\) \{[^}]*\n        \}'
new_crosshair = '''        // Renderizar crosshair solo si no está pausado
        if (!g_gameState->isPaused) {
            glColor3f(1, 1, 1);
            glLineWidth(2);
            glBegin(GL_LINES);
            glVertex2f(width / 2 - 10, height / 2);
            glVertex2f(width / 2 + 10, height / 2);
            glVertex2f(width / 2, height / 2 - 10);
            glVertex2f(width / 2, height / 2 + 10);
            glEnd();

            // Renderizar hotbar
            renderHotbar(&g_gameState->inventory, width, height);
        }'''

content = re.sub(old_crosshair, new_crosshair, content)
print("   [OK] Renderizado del hotbar anadido")

# ============================================================================
# GUARDAR ARCHIVO
# ============================================================================
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Todas las caracteristicas estilo Minecraft fueron anadidas!")
print("\nCambios realizados:")
print("  - Sistema de particulas al romper bloques")
print("  - Hotbar visual en la parte inferior (9 slots)")
print("  - Outline del bloque apuntado")
print("  - Animacion de grietas al romper bloques (0-9 niveles)")
print("  - Contador de items en el hotbar")
print("  - Seleccion visual del slot activo")
print("\nAhora recompila el juego con: cmake --build build --config Release")
