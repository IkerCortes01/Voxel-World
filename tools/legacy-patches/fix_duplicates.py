#!/usr/bin/env python3

# Read the file
with open('src/main.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix 1: Replace the first destructor to include thread cleanup
old_destructor = """    ~World() {
        for (auto& pair : chunks) {
            delete pair.second;
        }
        chunks.clear();
        delete terrainGen;
    }"""

new_destructor = """    ~World() {
        // Esperar a que termine el hilo de iluminación
        if (lightingThread != nullptr) {
            if (lightingThread->joinable()) {
                lightingThread->join();
            }
            delete lightingThread;
        }

        for (auto& pair : chunks) {
            delete pair.second;
        }
        chunks.clear();
        delete terrainGen;
    }"""

content = content.replace(old_destructor, new_destructor)

# Fix 2: Remove the duplicate destructor added by the script
duplicate_destructor = """    // Destructor: esperar a que termine el hilo de iluminación
    ~World() {
        if (lightingThread != nullptr) {
            if (lightingThread->joinable()) {
                lightingThread->join();
            }
            delete lightingThread;
        }

        for (auto& pair : chunks) {
            delete pair.second;
        }
    }"""

# Find and remove it
if duplicate_destructor in content:
    content = content.replace(duplicate_destructor, "")
    print("Removed duplicate destructor")
else:
    # Try variations
    lines_to_remove = []
    lines = content.split('\n')
    i = 0
    while i < len(lines):
        if '// Destructor: esperar a que termine el hilo de iluminación' in lines[i]:
            # Found start, find end
            start = i
            brace_count = 0
            found_opening = False
            for j in range(i, min(i+20, len(lines))):
                if '{' in lines[j]:
                    found_opening = True
                    brace_count += lines[j].count('{')
                    brace_count -= lines[j].count('}')
                elif '}' in lines[j]:
                    brace_count -= lines[j].count('}')

                if found_opening and brace_count == 0:
                    # Found the end
                    lines_to_remove = list(range(start, j+1))
                    break
            break
        i += 1

    if lines_to_remove:
        for idx in reversed(lines_to_remove):
            del lines[idx]
        content = '\n'.join(lines)
        print(f"Removed duplicate destructor (lines {lines_to_remove[0]+1}-{lines_to_remove[-1]+1})")

# Write back
with open('src/main.cpp', 'w', encoding='utf-8') as f:
    f.write(content)

print("Fixed duplicate destructor!")
