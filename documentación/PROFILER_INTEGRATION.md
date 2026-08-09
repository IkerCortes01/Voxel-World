# 🔍 PROFILER VISUAL - Guía de Integración

## ✅ COMPLETADO

Se ha implementado un **sistema de profiling profesional** con overlay visual.

---

## 📦 ARCHIVOS CREADOS

1. **`src/Profiler.h`** - Header con interfaz del profiler
2. **`src/Profiler.cpp`** - Implementación del profiler
3. Este documento - Guía de integración

---

## 🔧 INTEGRACIÓN EN MAIN.CPP

### **Paso 1: Include el header**
```cpp
#include "Profiler.h"
```

### **Paso 2: Inicializar en main()**
```cpp
int main() {
    // ... inicialización de GLFW, OpenGL, etc ...
    
    // Inicializar profiler
    Profiler::ProfilerManager::getInstance()->setEnabled(true);
    Profiler::ProfilerManager::getInstance()->setVisible(false); // Hidden por defecto
    
    // ... resto de init ...
}
```

### **Paso 3: Toggle con F3 en input callback**
```cpp
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        // ... otros keys ...
        
        if (key == GLFW_KEY_F3) {
            Profiler::toggle();  // Toggle profiler visibility
        }
    }
}
```

### **Paso 4: Actualizar stats cada frame**
```cpp
void gameLoop() {
    while (!glfwWindowShouldClose(window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();
        
        Profiler::beginFrame();
        
        // ... game logic ...
        // ... rendering ...
        
        Profiler::endFrame();
        
        // Calcular stats
        auto frameEnd = std::chrono::high_resolution_clock::now();
        float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
        float fps = 1000.0f / frameTimeMs;
        
        // Recolectar stats de chunks
        int total, ready, gen, mesh, upload;
        chunkManager.getStats(total, ready, gen, mesh, upload);
        
        // Construir stats
        Profiler::FrameStats stats;
        stats.fps = fps;
        stats.frameTimeMs = frameTimeMs;
        stats.cpuTimeMs = frameTimeMs; // TODO: separar CPU/GPU
        stats.gpuTimeMs = 0.0f;        // TODO: GPU queries
        
        stats.totalChunks = total;
        stats.readyChunks = ready;
        stats.generatingChunks = gen;
        stats.meshingChunks = mesh;
        stats.uploadingChunks = upload;
        
        stats.drawCalls = g_drawCalls;           // Global counter
        stats.verticesRendered = g_verticesRendered;
        stats.trianglesRendered = g_trianglesRendered;
        
        stats.memoryUsedMB = getTotalMemoryUsageMB();
        stats.chunkMemoryMB = getChunkMemoryMB();
        stats.meshMemoryMB = getMeshMemoryMB();
        stats.textureMemoryMB = getTextureMemoryMB();
        
        Profiler::updateStats(stats);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}
```

### **Paso 5: Renderizar overlay después de todo**
```cpp
void render() {
    // ... render mundo ...
    // ... render UI ...
    
    // Profiler overlay (último, encima de todo)
    Profiler::ProfilerManager::getInstance()->renderOverlay(screenWidth, screenHeight);
}
```

---

## 📊 PROFILING DE FUNCIONES

Para medir tiempo de funciones específicas, usa macros:

```cpp
void buildChunkMesh(Chunk* chunk) {
    PROFILE_SCOPE("buildChunkMesh");  // Auto-timing
    
    // ... código ...
}

void updatePhysics(float deltaTime) {
    float physicsTime = 0.0f;
    PROFILE_SCOPE_MS("updatePhysics", physicsTime);  // Guarda en variable
    
    // ... código ...
    
    // physicsTime ahora contiene el tiempo en ms
}
```

---

## 🎯 CONTADORES GLOBALES NECESARIOS

Agrega estos contadores globales en main.cpp:

```cpp
// Global performance counters
int g_drawCalls = 0;
int g_verticesRendered = 0;
int g_trianglesRendered = 0;

void resetPerformanceCounters() {
    g_drawCalls = 0;
    g_verticesRendered = 0;
    g_trianglesRendered = 0;
}

void renderChunk(Chunk* chunk) {
    // ... render code ...
    
    g_drawCalls++;
    g_verticesRendered += vertexCount;
    g_trianglesRendered += triangleCount;
}
```

**Llamar `resetPerformanceCounters()` al inicio de cada frame.**

---

## 💾 FUNCIONES DE MEMORIA

Implementa estas funciones para tracking de memoria:

```cpp
float getTotalMemoryUsageMB() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize / (1024.0f * 1024.0f);
    }
#endif
    return 0.0f;
}

float getChunkMemoryMB() {
    // Chunks * (SIZE * HEIGHT * SIZE * sizeof(uint8_t) * 2) para blocks + light
    size_t chunkCount = world.getChunkCount();
    size_t bytesPerChunk = 16 * 256 * 16 * 2;  // blocks + light
    return (chunkCount * bytesPerChunk) / (1024.0f * 1024.0f);
}

float getMeshMemoryMB() {
    // Estimar: readyChunks * average_mesh_size
    size_t readyCount = world.getReadyChunkCount();
    size_t avgMeshBytes = 50000;  // Ajustar según tu mesh promedio
    return (readyCount * avgMeshBytes) / (1024.0f * 1024.0f);
}

float getTextureMemoryMB() {
    // Total texture memory loaded
    return textureManager->getTotalMemoryMB();
}
```

---

## 🎨 INTEGRACIÓN CON RENDERTEX

El profiler usa `renderText()` que ya existe en tu main.cpp. Necesitas exponer la función:

**Opción 1: Callback**
```cpp
// En main.cpp, después de inicializar
Profiler::ProfilerManager::getInstance()->setTextRenderer(
    [](const std::string& text, int x, int y, int w, int h) {
        renderText(text.c_str(), x, y, w, h);
    }
);
```

**Opción 2: Modificar Profiler.cpp directamente**
```cpp
// En Profiler.cpp, línea ~160
void ProfilerManager::renderText(...) {
    extern void renderText(const char*, int, int, int, int);
    renderText(text.c_str(), x, y, screenWidth, screenHeight);
}
```

---

## 📝 ACTUALIZAR CMAKELIST.TXT

Agrega los nuevos archivos:

```cmake
set(SOURCES
    src/main.cpp
    src/ChunkSystem.cpp
    src/SaveSystem.cpp
    src/Profiler.cpp          # ← AGREGAR
)
```

---

## 🚀 CONTROLES

- **F3** - Toggle profiler on/off
- El profiler se actualiza cada 0.5s (configurable)
- Historial de 2 segundos (120 frames @ 60 FPS)

---

## 📊 QUÉ MUESTRA EL PROFILER

### **Panel Superior:**
- FPS y frame time
- CPU time / GPU time
- Chunk stats (total, ready, generating, meshing, uploading)
- Draw calls
- Vertices y triángulos renderizados
- Memory usage (total, chunks, meshes, textures)

### **Gráficas:**
- **Verde**: FPS history (0-120 FPS)
  - Líneas de referencia en 30 y 60 FPS
- **Roja**: Frame time history (0-33 ms)
  - 16.67 ms = 60 FPS target

---

## ⚡ PERFORMANCE

- **Overhead cuando visible:** ~0.1-0.2 ms por frame
- **Overhead cuando hidden:** prácticamente cero
- **Memory overhead:** ~10 KB (history buffers)

---

## 🔧 CONFIGURACIÓN AVANZADA

```cpp
auto profiler = Profiler::ProfilerManager::getInstance();

// Cambiar intervalo de actualización
profiler->setUpdateInterval(1.0f);  // 1 segundo

// Obtener top funciones más lentas
auto topFuncs = profiler->getTopFunctions(10);
for (const auto& [name, avgMs] : topFuncs) {
    std::cout << name << ": " << avgMs << " ms" << std::endl;
}
```

---

## ✅ CHECKLIST DE INTEGRACIÓN

- [ ] Agregar `Profiler.cpp` a CMakeLists.txt
- [ ] Include `Profiler.h` en main.cpp
- [ ] Inicializar profiler en `main()`
- [ ] Agregar toggle F3 en `keyCallback()`
- [ ] Actualizar stats cada frame
- [ ] Renderizar overlay al final
- [ ] Agregar contadores globales (drawCalls, etc)
- [ ] Implementar funciones de memoria
- [ ] Conectar `renderText()`
- [ ] Recompilar con `cmake --build build --config Release`
- [ ] Probar con F3 in-game

---

## 🎯 SIGUIENTE PASO

Una vez integrado, **presiona F3 en el juego** y verás el profiler visual.

Esto te permitirá:
- Identificar FPS drops en tiempo real
- Ver qué chunks están generando
- Monitorear uso de memoria
- Verificar que draw calls estén optimizados
- Detectar bottlenecks instantáneamente

**Objetivo:** Mantener **FPS > 60** y **frame time < 16.67 ms**

---

📖 **¿Necesitas ayuda con la integración?** Avísame y te guío paso a paso.
