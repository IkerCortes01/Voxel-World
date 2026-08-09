#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
AGREGAR PROFILING SIMPLE AL GAME LOOP
Para identificar el bottleneck real
"""

with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

import re

# Agregar includes necesarios
if '#include <chrono>' not in content:
    content = content.replace(
        '#include <mutex>',
        '#include <mutex>\n#include <chrono>'
    )

# Encontrar el game loop y agregar profiling
game_loop_pattern = r'(while \(!glfwWindowShouldClose\(window\)\) \{)'

profiling_code = r'''\1
        auto frame_start = std::chrono::high_resolution_clock::now();
        auto t1 = frame_start, t2 = frame_start;
        double physics_ms = 0, chunks_ms = 0, render_ms = 0;'''

content = re.sub(game_loop_pattern, profiling_code, content)

# Agregar timing después de updatePlayerPhysics
physics_timing = r'''(updatePlayerPhysics\(g_gameState->player.*?\);)'''
physics_with_timing = r'''\1
            t2 = std::chrono::high_resolution_clock::now();
            physics_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
            t1 = t2;'''

content = re.sub(physics_timing, physics_with_timing, content, flags=re.DOTALL)

# Agregar timing después de updateChunks
chunks_timing = r'''(g_gameState->world\.updateChunks\(.*?\);)'''
chunks_with_timing = r'''\1
            t2 = std::chrono::high_resolution_clock::now();
            chunks_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();
            t1 = t2;'''

content = re.sub(chunks_timing, chunks_with_timing, content)

# Agregar timing después de render
render_timing = r'''(g_gameState->world\.render\(\);)'''
render_with_timing = r'''\1
        t2 = std::chrono::high_resolution_clock::now();
        render_ms = std::chrono::duration<double, std::milli>(t2 - t1).count();'''

content = re.sub(render_timing, render_with_timing, content)

# Modificar el título para incluir profiling
title_pattern = r'sprintf\(title, "Voxel World - Sandbox Infinito \[60 FPS\] \| FPS: %d \| Pos: %.1f, %.1f, %.1f \| Seed: %d",'

new_title = r'sprintf(title, "VoxelWorld [60FPS] | FPS:%d | Phys:%.1fms Chunks:%.1fms Render:%.1fms | Pos:%.0f,%.0f,%.0f",'

content = re.sub(title_pattern, new_title, content)

# Actualizar los argumentos del sprintf
args_pattern = r'frameCount, g_gameState->player\.position\.x,\s+g_gameState->player\.position\.y, g_gameState->player\.position\.z,\s+g_gameState->world\.getSeed\(\)\);'

new_args = r'''frameCount, physics_ms, chunks_ms, render_ms,
                    g_gameState->player.position.x,
                    g_gameState->player.position.y,
                    g_gameState->player.position.z);'''

content = re.sub(args_pattern, new_args, content)

# Guardar
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("PROFILING agregado al game loop!")
print("")
print("Ahora el titulo mostrara:")
print("  - FPS")
print("  - Physics time (ms)")
print("  - Chunks time (ms)")
print("  - Render time (ms)")
print("")
print("Esto nos dira EXACTAMENTE donde esta el bottleneck.")
