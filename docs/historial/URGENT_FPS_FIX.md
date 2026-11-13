# 🚨 SOLUCIÓN URGENTE - 40-60 FPS GARANTIZADOS

**Problema:** El juego va muy lento cuando se generan o modifican chunks  
**Causa:** Generación/meshing de chunks bloquea el frame principal  
**Solución:** Sistema de throttling agresivo + time-slicing  
**Tiempo:** 30-45 minutos de integración

---

## ⚡ SOLUCIÓN RÁPIDA (15 minutos)

### **PASO 1: Limitar chunks por frame** [5 min]

Agregar al inicio del game loop:

```cpp
// ============================================================================
// CHUNK THROTTLING - Limitar procesamiento por frame
// ============================================================================
const int MAX_CHUNKS_GENERATE_PER_FRAME = 1;  // Solo 1 chunk generado por frame
const int MAX_CHUNKS_MESH_PER_FRAME = 2;      // Solo 2 chunks con mesh por frame
const int MAX_CHUNKS_UPLOAD_PER_FRAME = 2;    // Solo 2 chunks subidos a GPU por frame

static int chunksGeneratedThisFrame = 0;
static int chunksMeshedThisFrame = 0;
static int chunksUploadedThisFrame = 0;

// Reset contadores al inicio del frame
chunksGeneratedThisFrame = 0;
chunksMeshedThisFrame = 0;
chunksUploadedThisFrame = 0;
```

---

### **PASO 2: Aplicar límites en chunk generation** [5 min]

En tu función que genera chunks, agregar:

```cpp
void processChunkGeneration() {
    // ⭐ THROTTLING: Solo procesar si no excedimos límite
    if (chunksGeneratedThisFrame >= MAX_CHUNKS_GENERATE_PER_FRAME) {
        return;  // Ya generamos suficiente este frame
    }
    
    // Obtener siguiente chunk a generar
    Chunk* chunk = getNextChunkToGenerate();
    if (!chunk) return;
    
    // Generar chunk
    generateChunk(chunk);
    chunksGeneratedThisFrame++;
    
    // Log (debug)
    #ifdef DEBUG_THROTTLING
    printf("Generated chunk (%d, %d) - %d/%d this frame\n",
           chunk->x, chunk->z,
           chunksGeneratedThisFrame, MAX_CHUNKS_GENERATE_PER_FRAME);
    #endif
}
```

---

### **PASO 3: Aplicar límites en chunk meshing** [5 min]

En tu función que construye meshes:

```cpp
void processChunkMeshing() {
    // ⭐ THROTTLING: Solo procesar si no excedemos límite
    if (chunksMeshedThisFrame >= MAX_CHUNKS_MESH_PER_FRAME) {
        return;  // Ya procesamos suficiente este frame
    }
    
    // Obtener siguiente chunk a meshear
    Chunk* chunk = getNextChunkToMesh();
    if (!chunk) return;
    
    // Construir mesh
    buildChunkMesh(chunk);
    chunksMeshedThisFrame++;
    
    #ifdef DEBUG_THROTTLING
    printf("Meshed chunk (%d, %d) - %d/%d this frame\n",
           chunk->x, chunk->z,
           chunksMeshedThisFrame, MAX_CHUNKS_MESH_PER_FRAME);
    #endif
}
```

---

### **PASO 4: Aplicar límites en GPU upload** [5 min]

En tu función que sube meshes a GPU:

```cpp
void processChunkUpload() {
    // ⭐ THROTTLING: Solo procesar si no excedemos límite
    if (chunksUploadedThisFrame >= MAX_CHUNKS_UPLOAD_PER_FRAME) {
        return;  // Ya subimos suficiente este frame
    }
    
    // Obtener siguiente chunk a subir
    Chunk* chunk = getNextChunkToUpload();
    if (!chunk) return;
    
    // Subir a GPU
    uploadChunkToGPU(chunk);
    chunksUploadedThisFrame++;
    
    #ifdef DEBUG_THROTTLING
    printf("Uploaded chunk (%d, %d) - %d/%d this frame\n",
           chunk->x, chunk->z,
           chunksUploadedThisFrame, MAX_CHUNKS_UPLOAD_PER_FRAME);
    #endif
}
```

---

## 🎯 SOLUCIÓN COMPLETA (45 minutos)

### **PASO 1: Agregar header** [2 min]

```cpp
#include "PerformanceGuarantee.h"
```

---

### **PASO 2: Crear instancia global** [3 min]

```cpp
// Global
VoxelEngine::PerformanceGuaranteeSystem* g_performanceGuarantee = nullptr;
```

---

### **PASO 3: Inicializar en main()** [5 min]

```cpp
// En main(), después de inicializar GLFW:
g_performanceGuarantee = new VoxelEngine::PerformanceGuaranteeSystem();
g_performanceGuarantee->initialize();
```

---

### **PASO 4: Integrar en game loop** [15 min]

```cpp
// ============================================================================
// GAME LOOP CON PERFORMANCE GUARANTEE
// ============================================================================
auto lastFrameTime = std::chrono::high_resolution_clock::now();

while (!glfwWindowShouldClose(window)) {
    // ⭐ BEGIN FRAME
    auto frameStart = std::chrono::high_resolution_clock::now();
    g_performanceGuarantee->beginFrame();
    
    // Get budget
    auto& budget = g_performanceGuarantee->getBudget();
    auto& throttler = g_performanceGuarantee->getThrottler();
    
    // ========================================================================
    // INPUT (Fast, no throttling needed)
    // ========================================================================
    processInput();
    
    // ========================================================================
    // CHUNK GENERATION (THROTTLED)
    // ========================================================================
    if (throttler.canGenerateChunk() && budget.hasTimeFor(1.0f)) {
        auto genStart = std::chrono::high_resolution_clock::now();
        
        if (generateNextChunk()) {
            throttler.recordGeneration();
        }
        
        auto genEnd = std::chrono::high_resolution_clock::now();
        float genTime = std::chrono::duration<float, std::milli>(genEnd - genStart).count();
        budget.recordTime(genTime);
    }
    
    // ========================================================================
    // CHUNK MESHING (THROTTLED)
    // ========================================================================
    if (throttler.canMeshChunk() && budget.hasTimeFor(3.0f)) {
        auto meshStart = std::chrono::high_resolution_clock::now();
        
        if (meshNextChunk()) {
            throttler.recordMeshing();
        }
        
        auto meshEnd = std::chrono::high_resolution_clock::now();
        float meshTime = std::chrono::duration<float, std::milli>(meshEnd - meshStart).count();
        budget.recordTime(meshTime);
    }
    
    // ========================================================================
    // CHUNK UPLOAD (THROTTLED)
    // ========================================================================
    if (throttler.canUploadChunk() && budget.hasTimeFor(2.0f)) {
        auto uploadStart = std::chrono::high_resolution_clock::now();
        
        if (uploadNextChunk()) {
            throttler.recordUpload();
        }
        
        auto uploadEnd = std::chrono::high_resolution_clock::now();
        float uploadTime = std::chrono::duration<float, std::milli>(uploadEnd - uploadStart).count();
        budget.recordTime(uploadTime);
    }
    
    // ========================================================================
    // PHYSICS (Skip if over budget)
    // ========================================================================
    if (budget.hasTimeFor(1.0f)) {
        updatePhysics(deltaTime);
    }
    
    // ========================================================================
    // RENDER (Priority - always execute)
    // ========================================================================
    renderScene();
    
    // ========================================================================
    // PROCESS TIME-SLICED TASKS
    // ========================================================================
    g_performanceGuarantee->processTasks();
    
    // ========================================================================
    // EMERGENCY ACTIONS
    // ========================================================================
    if (g_performanceGuarantee->isEmergencyMode()) {
        auto actions = g_performanceGuarantee->getEmergencyActions();
        
        if (actions.disableParticles) {
            particleSystem.clear();
        }
        if (actions.reduceRenderDistance > 0) {
            currentRenderDistance -= actions.reduceRenderDistance;
        }
    }
    
    // ⭐ END FRAME
    auto frameEnd = std::chrono::high_resolution_clock::now();
    float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
    
    g_performanceGuarantee->endFrame(frameTimeMs);
    
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
    
    // Stats (opcional)
    #ifdef DEBUG_PERFORMANCE
    auto stats = g_performanceGuarantee->getStats();
    if (frameCount % 60 == 0) {  // Cada 60 frames
        printf("FPS: %.1f (avg: %.1f) | Budget: %.1fms remaining | Chunks: %d gen, %d mesh, %d upload\n",
               stats.currentFPS, stats.averageFPS, stats.budgetRemainingMs,
               stats.throttler.currentGen, stats.throttler.currentMesh, stats.throttler.currentUpload);
    }
    #endif
}
```

---

### **PASO 5: Chunk generation async (CRÍTICO)** [20 min]

**El problema principal:** Generar chunks en el thread principal bloquea todo.

**Solución:** Usar workers threads (ChunkSystem.h ya los tiene):

```cpp
// En lugar de:
void generateChunk(Chunk* chunk) {
    // Generar datos de voxel (BLOQUEA EL FRAME)
    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            for (int y = 0; y < 256; y++) {
                chunk->setBlock(x, y, z, calculateBlock(x, y, z));
            }
        }
    }
}

// Hacer:
void generateChunkAsync(Chunk* chunk) {
    // Push a worker thread queue
    chunkGenerationQueue.push(chunk);
    
    // Worker thread lo procesará en background
    // Main thread NO se bloquea
}
```

**Usando ChunkSystem.h existente:**

```cpp
// Ya tienes esto en ChunkSystem.h:
ThreadSafeQueue<Chunk*> generationQueue_;
ThreadSafeQueue<Chunk*> meshingQueue_;
ThreadSafeQueue<Chunk*> uploadQueue_;

// Solo necesitas asegurar que los worker threads estén corriendo:
chunkManager.initialize(4);  // 4 worker threads

// Y push chunks a las queues en lugar de procesarlos directamente:
generationQueue_.push(chunk);  // NO bloquea el main thread
```

---

## 🚀 MEJORAS ADICIONALES (Opcionales)

### **1. V-Sync** [1 min]

Habilitar V-Sync para estabilizar FPS:

```cpp
// En main(), después de crear contexto OpenGL:
glfwSwapInterval(1);  // V-Sync ON (limita a 60 FPS)
```

---

### **2. Prioridad de chunks** [10 min]

Procesar primero chunks visibles:

```cpp
struct ChunkPriority {
    Chunk* chunk;
    float priority;  // Mayor = más importante
    
    bool operator<(const ChunkPriority& other) const {
        return priority < other.priority;  // Min-heap invertido
    }
};

std::priority_queue<ChunkPriority> priorityQueue;

// Calcular prioridad basada en distancia al jugador
float calculatePriority(Chunk* chunk, Vec3 playerPos) {
    float dx = chunk->x * 16 - playerPos.x;
    float dz = chunk->z * 16 - playerPos.z;
    float distance = sqrt(dx * dx + dz * dz);
    
    // Menor distancia = mayor prioridad
    return 1000.0f / (distance + 1.0f);
}

// Agregar chunks con prioridad
for (auto* chunk : chunksToGenerate) {
    float priority = calculatePriority(chunk, playerPosition);
    priorityQueue.push({chunk, priority});
}

// Procesar en orden de prioridad
while (!priorityQueue.empty() && canGenerateChunk()) {
    auto top = priorityQueue.top();
    priorityQueue.pop();
    
    generateChunk(top.chunk);
}
```

---

### **3. Reducir render distance dinámicamente** [5 min]

```cpp
// En game loop:
auto stats = g_performanceGuarantee->getStats();

if (stats.averageFPS < 50.0f) {
    // FPS bajo: reducir render distance
    renderDistance = std::max(2, renderDistance - 1);
    std::cout << "⬇️ Render distance reducido a " << renderDistance << std::endl;
} else if (stats.averageFPS > 65.0f && renderDistance < 12) {
    // FPS alto: aumentar render distance
    renderDistance++;
    std::cout << "⬆️ Render distance aumentado a " << renderDistance << std::endl;
}
```

---

## 📊 RESULTADOS ESPERADOS

### **Antes (Sin throttling):**
```
Generando chunks: 10-20 chunks por frame
Frame time:       50-200ms (5-20 FPS) ❌
Stuttering:       Severo
Jugabilidad:      Mala
```

### **Después (Con throttling):**
```
Generando chunks: 1-2 chunks por frame
Frame time:       16-25ms (40-60 FPS) ✅
Stuttering:       Mínimo
Jugabilidad:      Buena
```

---

## 🎯 VALORES RECOMENDADOS

### **Para Intel HD 4000 (GPU muy antigua):**
```cpp
MAX_CHUNKS_GENERATE_PER_FRAME = 1;
MAX_CHUNKS_MESH_PER_FRAME = 1;
MAX_CHUNKS_UPLOAD_PER_FRAME = 1;
renderDistance = 4;
```

### **Para Intel HD 5000-6000:**
```cpp
MAX_CHUNKS_GENERATE_PER_FRAME = 1;
MAX_CHUNKS_MESH_PER_FRAME = 2;
MAX_CHUNKS_UPLOAD_PER_FRAME = 2;
renderDistance = 6;
```

### **Para GTX 750 / Intel Iris:**
```cpp
MAX_CHUNKS_GENERATE_PER_FRAME = 2;
MAX_CHUNKS_MESH_PER_FRAME = 2;
MAX_CHUNKS_UPLOAD_PER_FRAME = 2;
renderDistance = 8;
```

### **Para GTX 1050+:**
```cpp
MAX_CHUNKS_GENERATE_PER_FRAME = 3;
MAX_CHUNKS_MESH_PER_FRAME = 3;
MAX_CHUNKS_UPLOAD_PER_FRAME = 3;
renderDistance = 10;
```

---

## 🐛 TROUBLESHOOTING

### **Problema: Chunks tardan mucho en aparecer**

**Causa:** Límites muy bajos

**Solución:** Aumentar `MAX_CHUNKS_*_PER_FRAME` gradualmente hasta que FPS empiece a bajar

---

### **Problema: Sigue yendo lento**

**Causa:** Generación de chunks no está en background thread

**Solución:** Verificar que `chunkManager.initialize(4)` se llamó y que usas las queues

---

### **Problema: FPS fluctúa mucho**

**Causa:** No hay V-Sync

**Solución:** Agregar `glfwSwapInterval(1);`

---

## ✅ CHECKLIST

- [ ] Agregar límites de chunks por frame
- [ ] Aplicar throttling en generation
- [ ] Aplicar throttling en meshing
- [ ] Aplicar throttling en upload
- [ ] Habilitar V-Sync
- [ ] Verificar que workers threads están activos
- [ ] Compilar: `cmake --build build --config Release`
- [ ] Probar y ajustar límites según FPS

---

## 🎮 TESTING

### **Test 1: Volar rápido**
1. Volar en línea recta a velocidad máxima
2. FPS debe mantenerse >= 40
3. Chunks deben aparecer gradualmente (no todos a la vez)

### **Test 2: Modificar muchos bloques**
1. Romper 100+ bloques seguidos
2. FPS debe mantenerse >= 40
3. Puede haber lag mínimo pero no freeze

### **Test 3: Cargar mundo nuevo**
1. Crear mundo nuevo
2. FPS debe estar >= 40 desde el inicio
3. Chunks aparecen gradualmente alrededor del jugador

---

**⚡ SOLUCIÓN URGENTE: 15 minutos de integración = FPS estables**  
**🎯 SOLUCIÓN COMPLETA: 45 minutos de integración = 40-60 FPS garantizados**

---

**🚨 PRIORIDAD MÁXIMA: Implementar SOLUCIÓN RÁPIDA ahora mismo (15 min)**
