#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    lines = f.readlines()

# Find the line with "// FISICA Y COLISIONES"
for i, line in enumerate(lines):
    if '// FISICA Y COLISIONES' in line:
        # Check if there's a closing brace before this
        # Go back to find the last non-empty line
        j = i - 1
        while j >= 0 and lines[j].strip() == '':
            j -= 1

        # Check if it's a closing brace
        if j >= 0 and lines[j].strip() == '}':
            # Add the class closing after this
            lines[j] = lines[j].rstrip() + ';\n'
            print(f"Added semicolon after closing brace at line {j+1}")
            break
        elif j >= 0 and not lines[j].strip().endswith('};'):
            # Need to add };
            lines.insert(i, '};\n\n')
            print(f"Inserted closing brace at line {i+1}")
            break

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.writelines(lines)

print("Fixed class World closing!")
