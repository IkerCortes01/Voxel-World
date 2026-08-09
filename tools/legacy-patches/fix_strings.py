#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Fix the broken strings at lines 3935-3940
# Line 3935-3936 should be one line
if len(lines) > 3939:
    # Fix first broken string (lines 3935-3936, 0-indexed so 3934-3935)
    if 'std::cout << "' in lines[3934] and 'Iniciando' in lines[3935]:
        lines[3934] = '    std::cout << "\\nIniciando calculo de iluminacion global (en hilo separado)..." << std::endl;\n'
        lines[3935] = ''  # Remove this line

    # Fix second broken string (lines 3939-3940, 0-indexed so 3938-3939)
    if 'std::cout << "La iluminación se calculará mientras juegas.' in lines[3938]:
        lines[3938] = '    std::cout << "La iluminacion se calculara mientras juegas.\\n" << std::endl;\n'
        lines[3939] = ''  # Remove this line

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)

print("Fixed broken strings!")
