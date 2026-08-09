#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Find the location to insert
marker = "    glfwMakeContextCurrent(window);\n    glfwSwapInterval(0); // Deshabilitar VSync para permitir 120+ FPS\n\n    g_gameState = new GameState();"

if marker not in content:
    print("ERROR: Could not find insertion point")
    exit(1)

# Replace with the new code
new_code = """    glfwMakeContextCurrent(window);
    glfwSwapInterval(0); // Deshabilitar VSync para permitir 120+ FPS

    // Inicializar TextureManager (debe hacerse DESPUÉS de crear contexto OpenGL)
    std::cout << "Inicializando sistema de texturas..." << std::endl;
    g_textureManager = new TextureManager();
    g_textureManager->loadAllBlockTextures();
    std::cout << "Sistema de texturas listo!" << std::endl << std::endl;

    g_gameState = new GameState();"""

content = content.replace(marker, new_code)

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Successfully added TextureManager initialization to main()!")
print("TextureManager will be initialized after OpenGL context creation")
