#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the location to add lighting initialization
marker = """    std::cout << "Jugador spawneado en Y=" << g_gameState->player.position.y << std::endl;
    std::cout << "¡Mundo listo!" << std::endl;

    double lastTime = glfwGetTime();"""

if marker not in content:
    print("ERROR: Could not find initialization point")
    exit(1)

# Add lighting calculation
new_code = """    std::cout << "Jugador spawneado en Y=" << g_gameState->player.position.y << std::endl;
    std::cout << "¡Mundo listo!" << std::endl;

    // Calcular iluminación inicial en un hilo separado
    std::cout << "\nIniciando cálculo de iluminación global (en hilo separado)..." << std::endl;
    g_gameState->world.startLightingCalculation();
    std::cout << "Sistema de iluminación iniciado!" << std::endl;
    std::cout << "La iluminación se calculará mientras juegas.\n" << std::endl;

    double lastTime = glfwGetTime();"""

content = content.replace(marker, new_code)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Successfully added lighting initialization!")
print("Lighting calculation will start after world generation")
print("It will run in a separate thread")
