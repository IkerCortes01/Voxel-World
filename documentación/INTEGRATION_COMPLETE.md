# 🚀 INTEGRACIÓN COMPLETA - TODO EN UNO

## ⏱️ TIEMPO TOTAL ESTIMADO: 2-3 horas

Este documento contiene **TODO el código necesario** para implementar las optimizaciones A, B, C, D, E.

---

# A) PROFILER + OBJECT POOL [30 min]

## ✅ ARCHIVOS YA CREADOS
- ✅ `src/Profiler.h`
- ✅ `src/Profiler.cpp`
- ✅ `src/ObjectPool.h`
- ✅ `CMakeLists.txt` YA actualizado (Profiler.cpp agregado)

## 📝 CÓDIGO A AGREGAR EN MAIN.CPP

### **1. Después de `#include "ChunkSystem.h"` (línea ~75):**
```cpp
// ============================================================================
// PROFILER SYSTEM - Performance monitoring
// ============================================================================
#include "Profiler.h"

// ============================================================================
// OBJECT POOL - Memory optimization
// ============================================================================
#include "ObjectPool.h"

// ============================================================================
// GLOBAL PERFORMANCE COUNTERS
// ============================================================================
int g_drawCalls = 0;
int g_verticesRendered = 0;
int g_trianglesRendered = 0;

void resetPerformanceCounters() {
    g_drawCalls = 0;
    g_verticesRendered = 0;
    g_trianglesRendered = 0;
}

// ============================================================================
// PARTICLE POOL GLOBAL
// ============================================================================
ObjectPool<Particle>* g_particlePool = nullptr;

void initializeParticlePool() {
    g_particlePool = new ObjectPool<Particle>(
        // Factory
        []() { return new Particle(); },
        // Reset
        [](Particle* p) {
            p->position = Vec3(0, 0, 0);
            p->velocity = Vec3(0, 0, 0);
            p->life = 0.0f;
            p->maxLife = 0.0f;
        },
        // Destroy
        nullptr,
        // Config
        1000,  // initial
        500,   // expand
        5000   // max
    );
}

void cleanupParticlePool() {
    delete g_particlePool;
    g_particlePool = nullptr;
}
```

### **2. Modificar ParticleSystem (línea ~2730):**

**Reemplazar las funciones spawn y update:**

```cpp
class ParticleSystem {
public:
    std::vector<Particle*> particles;  // ← CAMBIAR de Particle a Particle*

    // Partículas pequeñas mientras se mina (progresivas)
    void spawnMiningParticles(Vec3 blockPos, BlockType blockType) {
        float r, g, b;
        getBlockColor(blockType, r, g, b);

        int count = 2 + (rand() % 3);
        for (int i = 0; i < count; i++) {
            Particle* p = g_particlePool->acquire();  // ← USAR POOL
            if (!p) continue;
            
            p->position = blockPos + Vec3(
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f
            );

            p->velocity = Vec3(
                ((rand() % 100) - 50) / 100.0f,
                ((rand() % 80) + 20) / 100.0f,
                ((rand() % 100) - 50) / 100.0f
            );

            float colorVar = 0.8f + (rand() % 40) / 100.0f;
            p->r = r * colorVar;
            p->g = g * colorVar;
            p->b = b * colorVar;
            p->life = 0.3f + (rand() % 20) / 100.0f;
            p->maxLife = p->life;
            particles.push_back(p);
        }
    }

    void spawnBlockBreakParticles(Vec3 blockPos, BlockType blockType) {
        float r, g, b;
        getBlockColor(blockType, r, g, b);

        int count = 30 + (rand() % 16);
        for (int i = 0; i < count; i++) {
            Particle* p = g_particlePool->acquire();  // ← USAR POOL
            if (!p) continue;
            
            p->position = blockPos + Vec3(
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f,
                0.3f + (rand() % 100) / 250.0f
            );

            p->velocity = Vec3(
                ((rand() % 200) - 100) / 100.0f,
                ((rand() % 150) + 50) / 100.0f,
                ((rand() % 200) - 100) / 100.0f
            );

            float colorVar = 0.8f + (rand() % 40) / 100.0f;
            p->r = r * colorVar;
            p->g = g * colorVar;
            p->b = b * colorVar;
            p->life = 0.25f + (rand() % 25) / 100.0f;
            p->maxLife = p->life;
            particles.push_back(p);
        }
    }

    void update(float deltaTime) {
        for (auto it = particles.begin(); it != particles.end(); ) {
            Particle* p = *it;
            p->update(deltaTime);
            
            if (p->isDead()) {
                g_particlePool->release(p);  // ← DEVOLVER AL POOL
                it = particles.erase(it);
            } else {
                ++it;
            }
        }
    }

    void render(const Player& player) {
        // ... (mantener igual, solo cambiar acceso)
        for (const auto* p : particles) {  // ← Ahora es Particle*
            // ... código de render ...
        }
    }
};
```

### **3. En main() después de crear g_soundManager (línea ~14073):**

```cpp
// ⭐ Inicializar Profiler
std::cout << "Inicializando sistema de profiling..." << std::endl;
Profiler::ProfilerManager::getInstance()->setEnabled(true);
Profiler::ProfilerManager::getInstance()->setVisible(false);  // Hidden por defecto, F3 para toggle
std::cout << "Profiler listo! (F3 para toggle)" << std::endl;

// ⭐ Inicializar Particle Pool
std::cout << "Inicializando object pool de partículas..." << std::endl;
initializeParticlePool();
std::cout << "Object pool listo! (1000 partículas pre-warm)" << std::endl;
```

### **4. En keyCallback() agregar toggle F3:**

Buscar la función `keyCallback` y agregar dentro del `if (action == GLFW_PRESS)`:

```cpp
if (key == GLFW_KEY_F3) {
    Profiler::toggle();
    std::cout << "Profiler " << (Profiler::isVisible() ? "visible" : "oculto") << std::endl;
}
```

### **5. En el game loop (buscar glfwSwapBuffers):**

**ANTES del swap buffers, agregar actualización de profiler:**

```cpp
// ⭐ PROFILER: Actualizar stats cada frame
{
    auto frameEnd = std::chrono::high_resolution_clock::now();
    float frameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
    
    Profiler::FrameStats stats;
    stats.fps = (frameTimeMs > 0) ? 1000.0f / frameTimeMs : 0.0f;
    stats.frameTimeMs = frameTimeMs;
    stats.cpuTimeMs = frameTimeMs;  // TODO: separar CPU/GPU
    stats.gpuTimeMs = 0.0f;
    
    // Chunk stats
    int total, ready, gen, mesh, upload;
    g_gameState->world.getStats(total, ready, gen, mesh, upload);
    stats.totalChunks = total;
    stats.readyChunks = ready;
    stats.generatingChunks = gen;
    stats.meshingChunks = mesh;
    stats.uploadingChunks = upload;
    
    // Rendering stats
    stats.drawCalls = g_drawCalls;
    stats.verticesRendered = g_verticesRendered;
    stats.trianglesRendered = g_trianglesRendered;
    
    // Memory (placeholder - mejorar después)
    stats.memoryUsedMB = 100.0f;  // TODO: calcular real
    stats.chunkMemoryMB = total * 0.5f;  // Estimación
    stats.meshMemoryMB = ready * 0.2f;
    stats.textureMemoryMB = 50.0f;
    
    Profiler::updateStats(stats);
    Profiler::endFrame();
}

// Reset counters para próximo frame
resetPerformanceCounters();

glfwSwapBuffers(window);
```

### **6. Renderizar profiler overlay (DESPUÉS de renderizar UI):**

Buscar donde renderizas la UI/HUD y agregar al final:

```cpp
// ⭐ PROFILER OVERLAY (último, encima de todo)
Profiler::ProfilerManager::getInstance()->renderOverlay(width, height);
```

### **7. Cleanup antes de return en main():**

```cpp
cleanupParticlePool();
```

---

# B) STATE MACHINE PATTERN [45 min]

## 📝 CREAR ARCHIVO: `src/StateMachine.h`

```cpp
#pragma once

#include <memory>
#include <stack>
#include <string>

// ============================================================================
// STATE MACHINE PATTERN - Professional Game State Management
// ============================================================================

class GameState;  // Forward declaration

// ============================================================================
// BASE STATE CLASS
// ============================================================================

class State {
public:
    virtual ~State() = default;

    // Called once when entering this state
    virtual void onEnter(GameState* gameState) = 0;

    // Called every frame while this state is active
    virtual void update(GameState* gameState, float deltaTime) = 0;

    // Called once when exiting this state
    virtual void onExit(GameState* gameState) = 0;

    // Optional: called when rendering
    virtual void render(GameState* gameState) {}

    // Get state name for debugging
    virtual const char* getName() const = 0;
};

// ============================================================================
// CONCRETE STATES
// ============================================================================

class MenuState : public State {
public:
    void onEnter(GameState* gameState) override;
    void update(GameState* gameState, float deltaTime) override;
    void onExit(GameState* gameState) override;
    void render(GameState* gameState) override;
    const char* getName() const override { return "MenuState"; }
};

class PlayingState : public State {
public:
    void onEnter(GameState* gameState) override;
    void update(GameState* gameState, float deltaTime) override;
    void onExit(GameState* gameState) override;
    void render(GameState* gameState) override;
    const char* getName() const override { return "PlayingState"; }
};

class PausedState : public State {
public:
    void onEnter(GameState* gameState) override;
    void update(GameState* gameState, float deltaTime) override;
    void onExit(GameState* gameState) override;
    void render(GameState* gameState) override;
    const char* getName() const override { return "PausedState"; }
};

class LoadingState : public State {
private:
    float startTime_ = 0.0f;
    float duration_ = 2.0f;
public:
    void onEnter(GameState* gameState) override;
    void update(GameState* gameState, float deltaTime) override;
    void onExit(GameState* gameState) override;
    void render(GameState* gameState) override;
    const char* getName() const override { return "LoadingState"; }
};

class WorldSelectState : public State {
public:
    void onEnter(GameState* gameState) override;
    void update(GameState* gameState, float deltaTime) override;
    void onExit(GameState* gameState) override;
    void render(GameState* gameState) override;
    const char* getName() const override { return "WorldSelectState"; }
};

// ============================================================================
// STATE MACHINE
// ============================================================================

class StateMachine {
private:
    std::unique_ptr<State> currentState_;
    std::stack<std::unique_ptr<State>> stateStack_;  // For pause/resume
    GameState* gameState_;

public:
    StateMachine(GameState* gs) : gameState_(gs) {}

    // Transition to a new state (destroys current)
    void transitionTo(std::unique_ptr<State> newState) {
        if (currentState_) {
            currentState_->onExit(gameState_);
        }
        currentState_ = std::move(newState);
        if (currentState_) {
            std::cout << "State transition -> " << currentState_->getName() << std::endl;
            currentState_->onEnter(gameState_);
        }
    }

    // Push state (pause current, will resume later)
    void pushState(std::unique_ptr<State> newState) {
        if (currentState_) {
            stateStack_.push(std::move(currentState_));
        }
        currentState_ = std::move(newState);
        if (currentState_) {
            std::cout << "State push -> " << currentState_->getName() << std::endl;
            currentState_->onEnter(gameState_);
        }
    }

    // Pop state (resume previous)
    void popState() {
        if (currentState_) {
            currentState_->onExit(gameState_);
        }
        if (!stateStack_.empty()) {
            currentState_ = std::move(stateStack_.top());
            stateStack_.pop();
            std::cout << "State pop -> " << currentState_->getName() << std::endl;
            // Don't call onEnter - state was just paused
        } else {
            currentState_ = nullptr;
        }
    }

    // Update current state
    void update(float deltaTime) {
        if (currentState_) {
            currentState_->update(gameState_, deltaTime);
        }
    }

    // Render current state
    void render() {
        if (currentState_) {
            currentState_->render(gameState_);
        }
    }

    // Get current state
    State* getCurrentState() const { return currentState_.get(); }
    const char* getCurrentStateName() const {
        return currentState_ ? currentState_->getName() : "None";
    }
};
```

## 📝 CREAR ARCHIVO: `src/StateMachine.cpp`

```cpp
#include "StateMachine.h"
// Incluir lo necesario del GameState original

// ============================================================================
// MENU STATE
// ============================================================================

void MenuState::onEnter(GameState* gs) {
    // Setup menu buttons, cursor visible, etc.
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void MenuState::update(GameState* gs, float deltaTime) {
    // Handle menu logic
}

void MenuState::onExit(GameState* gs) {
    // Cleanup menu
}

void MenuState::render(GameState* gs) {
    // Render menu UI
}

// ============================================================================
// PLAYING STATE
// ============================================================================

void PlayingState::onEnter(GameState* gs) {
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void PlayingState::update(GameState* gs, float deltaTime) {
    // Update physics, world, player, etc.
}

void PlayingState::onExit(GameState* gs) {
    // Cleanup if needed
}

void PlayingState::render(GameState* gs) {
    // Render world, HUD, etc.
}

// ============================================================================
// PAUSED STATE
// ============================================================================

void PausedState::onEnter(GameState* gs) {
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void PausedState::update(GameState* gs, float deltaTime) {
    // Pause menu logic
}

void PausedState::onExit(GameState* gs) {
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void PausedState::render(GameState* gs) {
    // Render pause overlay
}

// ============================================================================
// LOADING STATE
// ============================================================================

void LoadingState::onEnter(GameState* gs) {
    startTime_ = glfwGetTime();
}

void LoadingState::update(GameState* gs, float deltaTime) {
    float elapsed = glfwGetTime() - startTime_;
    if (elapsed >= duration_) {
        // Transition to playing
    }
}

void LoadingState::onExit(GameState* gs) {
}

void LoadingState::render(GameState* gs) {
    // Render loading screen
}

// ============================================================================
// WORLD SELECT STATE
// ============================================================================

void WorldSelectState::onEnter(GameState* gs) {
    glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    // Scan saved worlds
}

void WorldSelectState::update(GameState* gs, float deltaTime) {
    // Handle world selection
}

void WorldSelectState::onExit(GameState* gs) {
}

void WorldSelectState::render(GameState* gs) {
    // Render world list
}
```

**NOTA:** Esta implementación necesita acceso a las funciones existentes. Es un template - ajustar según tu código actual.

---

# C) RENDERING OPTIMIZATION [60 min]

## 📝 TÉCNICAS A IMPLEMENTAR

### **1. Batching Mejorado**

En `World::render()`, agrupar por textura:

```cpp
// Agrupar chunks por textura antes de renderizar
std::map<GLuint, std::vector<Chunk*>> batchesByTexture;

for (auto& chunk : readyChunks) {
    GLuint tex = chunk->getTextureID();
    batchesByTexture[tex].push_back(chunk);
}

// Renderizar batch completo por textura
for (const auto& [tex, chunks] : batchesByTexture) {
    glBindTexture(GL_TEXTURE_2D, tex);
    
    for (Chunk* chunk : chunks) {
        renderChunk(chunk);
        g_drawCalls++;  // ← Contador para profiler
    }
}
```

### **2. Frustum Culling Optimizado**

```cpp
// Early rejection antes de procesar chunk
bool isChunkInFrustum(const Chunk* chunk, const Frustum& frustum) {
    Vec3 chunkMin(chunk->x * 16, 0, chunk->z * 16);
    Vec3 chunkMax(chunkMin.x + 16, 256, chunkMin.z + 16);
    
    // AABB vs Frustum test - early out
    for (int i = 0; i < 6; i++) {
        if (distanceToPlane(frustum.planes[i], chunkMin, chunkMax) < 0) {
            return false;  // Fuera del frustum
        }
    }
    return true;
}
```

### **3. Occlusion Culling Simple**

```cpp
// No renderizar chunks completamente rodeados
bool isChunkOccluded(const Chunk* chunk) {
    // Check si todos los vecinos son sólidos
    bool allNeighborsSolid = true;
    for (int i = 0; i < 6; i++) {
        if (!chunk->hasNeighbor(i) || !chunk->getNeighbor(i)->isFullySolid()) {
            allNeighborsSolid = false;
            break;
        }
    }
    return allNeighborsSolid;
}
```

---

# D) EXPLORACIÓN CON GRAPHIFY [15 min]

## 🔍 COMANDOS ÚTILES

```bash
# Actualizar graph después de cambios
cd "D:\Respaldo\Voxel World"
graphify . --code-only --update
graphify cluster-only .

# Queries útiles
graphify query "¿Cómo optimizar el rendering?"
graphify query "¿Qué funciones usan más tiempo?"
graphify path "MeshBuilder" "GPUUploader"
graphify path "ChunkManager" "ParticleSystem"
graphify explain "buildChunkMesh"
graphify explain "WorldSaveManager"

# Ver relaciones
graphify query "¿Qué sistemas dependen de ChunkSystem?"
graphify query "¿Cómo fluyen los datos desde input hasta render?"
```

---

# E) TAREAS RESTANTES [90 min]

## ⏳ TAREA #5: Memory Pool para Meshes [20 min]

Crear `src/MeshPool.h`:

```cpp
template<typename T>
class MemoryPool {
private:
    std::vector<T*> blocks_;
    std::queue<T*> freeList_;
    size_t blockSize_;
    
public:
    MemoryPool(size_t initialBlocks = 10, size_t blockSize = 1000) {
        for (size_t i = 0; i < initialBlocks; ++i) {
            T* block = new T[blockSize];
            blocks_.push_back(block);
            for (size_t j = 0; j < blockSize; ++j) {
                freeList_.push(&block[j]);
            }
        }
    }
    
    T* allocate() {
        if (freeList_.empty()) expand();
        T* ptr = freeList_.front();
        freeList_.pop();
        return ptr;
    }
    
    void deallocate(T* ptr) {
        freeList_.push(ptr);
    }
};
```

## ⏳ TAREA #6: Lock-Free Queues [20 min]

Reemplazar `ThreadSafeQueue` con ring buffer:

```cpp
template<typename T>
class LockFreeQueue {
private:
    std::atomic<size_t> head_{0};
    std::atomic<size_t> tail_{0};
    std::vector<T> buffer_;
    size_t capacity_;
    
public:
    bool tryPush(const T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);
        size_t next = (tail + 1) % capacity_;
        if (next == head_.load(std::memory_order_acquire)) {
            return false;  // Full
        }
        buffer_[tail] = item;
        tail_.store(next, std::memory_order_release);
        return true;
    }
    
    bool tryPop(T& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            return false;  // Empty
        }
        item = buffer_[head];
        head_.store((head + 1) % capacity_, std::memory_order_release);
        return true;
    }
};
```

## ⏳ TAREA #7: Async Asset Loading [15 min]

```cpp
class AssetLoader {
private:
    std::thread loaderThread_;
    std::queue<std::string> loadQueue_;
    std::atomic<bool> running_{true};
    
    void loaderWorker() {
        while (running_) {
            std::string asset;
            if (getNextAsset(asset)) {
                loadAssetAsync(asset);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
    
public:
    void startLoading(const std::string& path) {
        loadQueue_.push(path);
    }
};
```

## ⏳ TAREA #8: AABB Optimization [15 min]

```cpp
// Spatial hash para broadphase
struct SpatialHash {
    std::unordered_map<int, std::vector<AABB*>> grid;
    float cellSize = 16.0f;
    
    int hash(float x, float z) {
        int ix = (int)(x / cellSize);
        int iz = (int)(z / cellSize);
        return ix * 73856093 ^ iz * 19349663;
    }
    
    void insert(AABB* aabb) {
        int h = hash(aabb->center.x, aabb->center.z);
        grid[h].push_back(aabb);
    }
    
    std::vector<AABB*> query(const AABB& box) {
        int h = hash(box.center.x, box.center.z);
        return grid[h];  // Solo objetos en misma celda
    }
};
```

## ⏳ TAREA #9: Data-Driven Config [10 min]

Crear `config/game_settings.json`:

```json
{
  "graphics": {
    "renderDistance": 8,
    "fov": 70,
    "vsync": true,
    "msaa": 4
  },
  "gameplay": {
    "walkSpeed": 4.3,
    "jumpForce": 8.0,
    "gravity": 20.0,
    "miningSpeed": 1.0
  },
  "performance": {
    "maxChunksPerFrame": 5,
    "meshBuildThreads": 4,
    "particleLimit": 5000
  }
}
```

## ⏳ TAREA #10: Profiling Macros [5 min]

Ya incluido en `Profiler.h`:

```cpp
PROFILE_SCOPE("functionName");
PROFILE_SCOPE_MS("name", timeVar);
```

## ⏳ TAREA #11: Documentation [5 min]

Usar Graphify para generar:

```bash
graphify . --wiki
# Genera wiki completa con artículos por comunidad
```

---

# 📊 CHECKLIST FINAL

## A) Integración Profiler + Pool
- [ ] CMakeLists.txt actualizado ✅ (ya hecho)
- [ ] Includes agregados en main.cpp
- [ ] Contadores globales agregados
- [ ] Particle pool inicializado
- [ ] ParticleSystem refactorizado
- [ ] F3 toggle agregado
- [ ] Stats update en game loop
- [ ] Profiler overlay renderizado
- [ ] Compilar y probar

## B) State Machine
- [ ] Crear StateMachine.h
- [ ] Crear StateMachine.cpp
- [ ] Refactorizar GameState
- [ ] Integrar en main loop
- [ ] Probar transiciones

## C) Rendering
- [ ] Batching por textura
- [ ] Frustum culling optimizado
- [ ] Occlusion culling
- [ ] Contadores de draw calls

## D) Graphify
- [ ] Actualizar graph
- [ ] Queries exploratorias
- [ ] Documentar hallazgos

## E) Tareas Restantes
- [ ] Memory pool
- [ ] Lock-free queues
- [ ] Async loading
- [ ] AABB optimization
- [ ] Config files
- [ ] Profiling macros
- [ ] Documentation

---

# 🎯 ORDEN RECOMENDADO

1. **A) Profiler + Pool** (30 min) - Mayor impacto inmediato
2. **C) Rendering** (60 min) - Gran mejora visible
3. **D) Graphify** (15 min) - Explorar resultados
4. **B) State Machine** (45 min) - Cleanup arquitectural
5. **E) Resto** (90 min) - Polish final

**TOTAL: ~4 horas para TODO completo**

---

📖 **SIGUIENTE PASO:** Comenzar con A) - copiar código de integración y compilar.
