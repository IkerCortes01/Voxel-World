#!/usr/bin/env python3
# Script para añadir línea visual del raycast desde el jugador

import re

print("[INFO] Leyendo src/main.cpp...")
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# ============================================================================
# AÑADIR FUNCIÓN PARA DIBUJAR LÍNEA DE RAYCAST
# ============================================================================
print("[INFO] Anadiendo funcion para dibujar linea de raycast...")

raycast_line_function = '''
// Dibujar línea de raycast desde el jugador hasta el bloque apuntado
void renderRaycastLine(Vec3 origin, Vec3 direction, float distance, bool hit) {
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Vec3 endpoint = origin + direction * distance;

    // Color: Verde si detectó bloque, Rojo si no
    if (hit) {
        glColor4f(0.0f, 1.0f, 0.0f, 0.5f); // Verde semi-transparente
    } else {
        glColor4f(1.0f, 0.0f, 0.0f, 0.3f); // Rojo semi-transparente
    }

    glLineWidth(2.0f);
    glBegin(GL_LINES);
    glVertex3f(origin.x, origin.y, origin.z);
    glVertex3f(endpoint.x, endpoint.y, endpoint.z);
    glEnd();

    // Pequeña esfera en el origen (posición de los ojos del jugador)
    glPointSize(8.0f);
    glColor4f(1.0f, 1.0f, 0.0f, 1.0f); // Amarillo
    glBegin(GL_POINTS);
    glVertex3f(origin.x, origin.y, origin.z);
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

'''

# Buscar después de renderBlockCrack y antes de updateMining
insert_pos = content.find('// Sistema de minado progresivo (como Minecraft)')
if insert_pos != -1:
    # Retroceder al inicio de línea
    while insert_pos > 0 and content[insert_pos-1] != '\n':
        insert_pos -= 1
    content = content[:insert_pos] + raycast_line_function + content[insert_pos:]
    print("   [OK] Funcion renderRaycastLine anadida")
else:
    print("   [ERROR] No se encontro punto de insercion")

# ============================================================================
# AÑADIR LLAMADA PARA RENDERIZAR LA LÍNEA
# ============================================================================
print("[INFO] Anadiendo renderizado de linea de raycast...")

# Buscar donde se renderiza el outline del bloque y añadir la línea
render_line_code = '''
        // Renderizar línea de raycast (desde los ojos del jugador hasta el bloque)
        if (!g_gameState->isPaused) {
            Vec3 lineOrigin = g_gameState->player.getEyePosition();
            Vec3 lineDirection = g_gameState->player.getForward();
            RaycastResult lineResult = raycastBlock(g_gameState->world, lineOrigin, lineDirection, 5.0f);

            float lineDistance = lineResult.hit ? lineResult.distance : 5.0f;
            renderRaycastLine(lineOrigin, lineDirection, lineDistance, lineResult.hit);
        }

'''

# Buscar el lugar donde se renderiza el outline
pattern = r'(        if \(rayResult\.hit && !g_gameState->isPaused\) \{)'

if re.search(pattern, content):
    content = re.sub(pattern, render_line_code + r'\1', content)
    print("   [OK] Llamada a renderRaycastLine anadida")
else:
    print("   [WARNING] No se encontro el patron, intentando alternativa...")

# ============================================================================
# ACTUALIZAR DEBUG PARA MOSTRAR ORIGEN DEL RAYCAST
# ============================================================================
print("[INFO] Actualizando debug para mostrar origen del raycast...")

# Buscar donde está el debug de posición y añadir origen del raycast
debug_update = r'''(            // Posición del jugador
            sprintf\(debugText, "Pos: \(%\.1f, %\.1f, %\.1f\)",
                g_gameState->player\.position\.x,
                g_gameState->player\.position\.y,
                g_gameState->player\.position\.z\);
            renderText\(debugText, 10, 10, 12\);)'''

new_debug = r'''\1

            // Origen del raycast (posición de los ojos)
            Vec3 eyePos = g_gameState->player.getEyePosition();
            sprintf(debugText, "Ojos: (%.1f, %.1f, %.1f)",
                eyePos.x, eyePos.y, eyePos.z);
            renderText(debugText, 10, 150, 12);'''

if re.search(debug_update, content):
    content = re.sub(debug_update, new_debug, content)
    print("   [OK] Debug de origen del raycast anadido")

# Guardar
print("[INFO] Guardando cambios en src/main.cpp...")
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("\n[OK] Linea visual del raycast anadida!")
print("\nAhora veras:")
print("  - Linea VERDE desde tus ojos hasta el bloque apuntado")
print("  - Linea ROJA si no hay bloque (aire/lejos)")
print("  - Punto AMARILLO en la posicion de tus ojos (origen del raycast)")
print("  - En el debug: Posicion de tus pies Y posicion de tus ojos")
print("\nEsto demuestra que el raycast parte EXACTAMENTE desde tu posicion")
print("\nCompila: cmake --build build --config Release")
