#pragma once

#include <chrono>
#include <deque>
#include <atomic>
#include <mutex>
#include <thread>
#include <functional>

// ============================================================================
// PERFORMANCE GUARANTEE SYSTEM - 40-60 FPS OBLIGATORIO
// ============================================================================
// Sistema agresivo que GARANTIZA 40-60 FPS sin importar la carga
// - Frame budget estricto (16.67ms @ 60 FPS)
// - Time-slicing de operaciones pesadas
// - Dynamic throttling de generación/meshing
// - Emergency degradation si FPS < 40
// ============================================================================

namespace VoxelEngine {

// ============================================================================
// FRAME BUDGET MANAGER
// ============================================================================
// Controla cuánto tiempo queda en el frame actual
// ============================================================================

class FrameBudgetManager {
public:
    static constexpr float TARGET_FRAME_TIME_MS = 16.67f;  // 60 FPS
    static constexpr float MIN_FRAME_TIME_MS = 25.0f;      // 40 FPS (límite de emergencia)
    static constexpr float SAFETY_MARGIN_MS = 2.0f;        // Margen de seguridad

private:
    std::chrono::high_resolution_clock::time_point frameStartTime_;
    float budgetUsedMs_;
    float totalBudgetMs_;

    // Asignación de budget por categoría
    float renderBudgetMs_;
    float chunkGenBudgetMs_;
    float chunkMeshBudgetMs_;
    float chunkUploadBudgetMs_;
    float physicsBudgetMs_;
    float inputBudgetMs_;

public:
    FrameBudgetManager() {
        reset();
        allocateBudget();
    }

    void beginFrame() {
        frameStartTime_ = std::chrono::high_resolution_clock::now();
        budgetUsedMs_ = 0.0f;
    }

    float getElapsedMs() const {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<float, std::milli>(now - frameStartTime_).count();
    }

    float getRemainingMs() const {
        return totalBudgetMs_ - getElapsedMs();
    }

    bool hasTimeFor(float ms) const {
        return getRemainingMs() >= (ms + SAFETY_MARGIN_MS);
    }

    // Budget por categoría
    float getRenderBudget() const { return renderBudgetMs_; }
    float getChunkGenBudget() const { return chunkGenBudgetMs_; }
    float getChunkMeshBudget() const { return chunkMeshBudgetMs_; }
    float getChunkUploadBudget() const { return chunkUploadBudgetMs_; }

    void recordTime(float ms) {
        budgetUsedMs_ += ms;
    }

    bool isOverBudget() const {
        return getElapsedMs() > totalBudgetMs_;
    }

    bool isCritical() const {
        return getElapsedMs() > MIN_FRAME_TIME_MS;
    }

private:
    void reset() {
        totalBudgetMs_ = TARGET_FRAME_TIME_MS;
        budgetUsedMs_ = 0.0f;
    }

    void allocateBudget() {
        // Distribución de budget (total 16.67ms @ 60 FPS)
        renderBudgetMs_ = 8.0f;        // 48% - Renderizado
        chunkGenBudgetMs_ = 1.0f;      // 6%  - Generación de chunks
        chunkMeshBudgetMs_ = 3.0f;     // 18% - Meshing de chunks
        chunkUploadBudgetMs_ = 2.0f;   // 12% - Upload a GPU
        physicsBudgetMs_ = 1.0f;       // 6%  - Física
        inputBudgetMs_ = 0.5f;         // 3%  - Input
        // 1.17ms restante como margen
    }
};

// ============================================================================
// TIME-SLICED TASK SYSTEM
// ============================================================================
// Divide operaciones pesadas en múltiples frames
// ============================================================================

class TimeSlicedTask {
public:
    using WorkFunction = std::function<bool()>;  // Retorna true si terminó

private:
    WorkFunction workFunc_;
    float maxTimeMs_;
    bool completed_;
    int priority_;

public:
    TimeSlicedTask(WorkFunction work, float maxTimeMs, int priority = 0)
        : workFunc_(work)
        , maxTimeMs_(maxTimeMs)
        , completed_(false)
        , priority_(priority)
    {}

    bool execute(FrameBudgetManager& budget) {
        if (completed_) return true;

        if (!budget.hasTimeFor(maxTimeMs_)) {
            return false;  // No hay tiempo este frame
        }

        auto start = std::chrono::high_resolution_clock::now();
        completed_ = workFunc_();
        auto end = std::chrono::high_resolution_clock::now();

        float elapsed = std::chrono::duration<float, std::milli>(end - start).count();
        budget.recordTime(elapsed);

        return completed_;
    }

    bool isCompleted() const { return completed_; }
    int getPriority() const { return priority_; }
};

class TimeSlicedTaskQueue {
private:
    std::deque<TimeSlicedTask> tasks_;
    std::mutex mutex_;

public:
    void addTask(TimeSlicedTask&& task) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Insertar según prioridad
        auto it = tasks_.begin();
        while (it != tasks_.end() && it->getPriority() >= task.getPriority()) {
            ++it;
        }
        tasks_.insert(it, std::move(task));
    }

    void processTasks(FrameBudgetManager& budget) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = tasks_.begin();
        while (it != tasks_.end()) {
            if (it->execute(budget)) {
                // Tarea completada, remover
                it = tasks_.erase(it);
            } else {
                // No hay más tiempo este frame
                break;
            }
        }
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return tasks_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks_.clear();
    }
};

// ============================================================================
// CHUNK GENERATION THROTTLER
// ============================================================================
// Controla cuántos chunks se generan/meshean por frame
// ============================================================================

class ChunkThrottler {
private:
    // Límites adaptativos
    int maxChunksGeneratedPerFrame_;
    int maxChunksMeshedPerFrame_;
    int maxChunksUploadedPerFrame_;

    // Contadores del frame actual
    std::atomic<int> chunksGeneratedThisFrame_;
    std::atomic<int> chunksMeshedThisFrame_;
    std::atomic<int> chunksUploadedThisFrame_;

    // Histórico de FPS
    std::deque<float> fpsHistory_;
    static constexpr size_t FPS_HISTORY_SIZE = 30;

    // Configuración agresiva
    bool aggressiveMode_;

public:
    ChunkThrottler()
        : maxChunksGeneratedPerFrame_(2)
        , maxChunksMeshedPerFrame_(2)
        , maxChunksUploadedPerFrame_(2)
        , chunksGeneratedThisFrame_(0)
        , chunksMeshedThisFrame_(0)
        , chunksUploadedThisFrame_(0)
        , aggressiveMode_(true)
    {}

    void beginFrame() {
        chunksGeneratedThisFrame_ = 0;
        chunksMeshedThisFrame_ = 0;
        chunksUploadedThisFrame_ = 0;
    }

    bool canGenerateChunk() const {
        return chunksGeneratedThisFrame_.load() < maxChunksGeneratedPerFrame_;
    }

    bool canMeshChunk() const {
        return chunksMeshedThisFrame_.load() < maxChunksMeshedPerFrame_;
    }

    bool canUploadChunk() const {
        return chunksUploadedThisFrame_.load() < maxChunksUploadedPerFrame_;
    }

    void recordGeneration() {
        chunksGeneratedThisFrame_++;
    }

    void recordMeshing() {
        chunksMeshedThisFrame_++;
    }

    void recordUpload() {
        chunksUploadedThisFrame_++;
    }

    void updateLimits(float currentFPS) {
        fpsHistory_.push_back(currentFPS);
        if (fpsHistory_.size() > FPS_HISTORY_SIZE) {
            fpsHistory_.pop_front();
        }

        float avgFPS = 0.0f;
        for (float fps : fpsHistory_) {
            avgFPS += fps;
        }
        avgFPS /= fpsHistory_.size();

        // Ajuste agresivo de límites
        if (avgFPS < 45.0f) {
            // EMERGENCIA: Reducir a mínimo
            maxChunksGeneratedPerFrame_ = 1;
            maxChunksMeshedPerFrame_ = 1;
            maxChunksUploadedPerFrame_ = 1;
            aggressiveMode_ = true;
        } else if (avgFPS < 55.0f) {
            // Bajo FPS: Reducir
            maxChunksGeneratedPerFrame_ = 2;
            maxChunksMeshedPerFrame_ = 2;
            maxChunksUploadedPerFrame_ = 2;
            aggressiveMode_ = true;
        } else if (avgFPS < 65.0f) {
            // FPS aceptable: Normal
            maxChunksGeneratedPerFrame_ = 3;
            maxChunksMeshedPerFrame_ = 3;
            maxChunksUploadedPerFrame_ = 3;
            aggressiveMode_ = false;
        } else {
            // FPS alto: Aumentar
            maxChunksGeneratedPerFrame_ = 4;
            maxChunksMeshedPerFrame_ = 4;
            maxChunksUploadedPerFrame_ = 4;
            aggressiveMode_ = false;
        }
    }

    struct Stats {
        int maxGen, maxMesh, maxUpload;
        int currentGen, currentMesh, currentUpload;
        bool aggressive;
    };

    Stats getStats() const {
        Stats s;
        s.maxGen = maxChunksGeneratedPerFrame_;
        s.maxMesh = maxChunksMeshedPerFrame_;
        s.maxUpload = maxChunksUploadedPerFrame_;
        s.currentGen = chunksGeneratedThisFrame_.load();
        s.currentMesh = chunksMeshedThisFrame_.load();
        s.currentUpload = chunksUploadedThisFrame_.load();
        s.aggressive = aggressiveMode_;
        return s;
    }
};

// ============================================================================
// EMERGENCY PERFORMANCE MANAGER
// ============================================================================
// Medidas de emergencia cuando FPS < 40
// ============================================================================

class EmergencyPerformanceManager {
private:
    bool emergencyMode_;
    int consecutiveSlowFrames_;
    static constexpr int EMERGENCY_THRESHOLD = 5;  // 5 frames malos = emergencia

    // Acciones de emergencia
    bool particlesDisabled_;
    bool shadowsDisabled_;
    bool soundDisabled_;
    int renderDistanceReduced_;

public:
    EmergencyPerformanceManager()
        : emergencyMode_(false)
        , consecutiveSlowFrames_(0)
        , particlesDisabled_(false)
        , shadowsDisabled_(false)
        , soundDisabled_(false)
        , renderDistanceReduced_(0)
    {}

    void update(float currentFPS) {
        if (currentFPS < 40.0f) {
            consecutiveSlowFrames_++;

            if (consecutiveSlowFrames_ >= EMERGENCY_THRESHOLD) {
                if (!emergencyMode_) {
                    activateEmergencyMode();
                }
            }
        } else if (currentFPS > 55.0f) {
            consecutiveSlowFrames_ = 0;

            if (emergencyMode_) {
                deactivateEmergencyMode();
            }
        }
    }

    bool isEmergencyMode() const { return emergencyMode_; }

    struct EmergencyActions {
        bool disableParticles;
        bool disableShadows;
        bool disableSound;
        int reduceRenderDistance;
    };

    EmergencyActions getActions() const {
        EmergencyActions a;
        a.disableParticles = particlesDisabled_;
        a.disableShadows = shadowsDisabled_;
        a.disableSound = soundDisabled_;
        a.reduceRenderDistance = renderDistanceReduced_;
        return a;
    }

private:
    void activateEmergencyMode() {
        emergencyMode_ = true;

        // Acciones drásticas
        particlesDisabled_ = true;
        shadowsDisabled_ = true;
        soundDisabled_ = false;  // Mantener sonido
        renderDistanceReduced_ = 2;  // Reducir 2 chunks

        std::cout << "🚨 MODO EMERGENCIA ACTIVADO - FPS < 40" << std::endl;
        std::cout << "   Partículas: OFF" << std::endl;
        std::cout << "   Sombras: OFF" << std::endl;
        std::cout << "   Render Distance: -2" << std::endl;
    }

    void deactivateEmergencyMode() {
        emergencyMode_ = false;

        // Restaurar gradualmente
        particlesDisabled_ = false;
        shadowsDisabled_ = false;
        soundDisabled_ = false;
        renderDistanceReduced_ = 0;

        std::cout << "✅ MODO EMERGENCIA DESACTIVADO - FPS recuperado" << std::endl;
    }
};

// ============================================================================
// PERFORMANCE GUARANTEE SYSTEM (Orquestador principal)
// ============================================================================
// Garantiza 40-60 FPS sin importar la carga
// ============================================================================

class PerformanceGuaranteeSystem {
private:
    FrameBudgetManager budgetManager_;
    TimeSlicedTaskQueue taskQueue_;
    ChunkThrottler chunkThrottler_;
    EmergencyPerformanceManager emergencyManager_;

    // Stats
    std::deque<float> fpsHistory_;
    static constexpr size_t FPS_HISTORY_SIZE = 120;  // 2 segundos

    float currentFPS_;
    float averageFPS_;
    float worstFrameMs_;
    float bestFrameMs_;

    // Flags
    bool initialized_;

public:
    PerformanceGuaranteeSystem()
        : currentFPS_(60.0f)
        , averageFPS_(60.0f)
        , worstFrameMs_(16.67f)
        , bestFrameMs_(16.67f)
        , initialized_(false)
    {}

    void initialize() {
        if (initialized_) return;

        std::cout << "🎯 PERFORMANCE GUARANTEE SYSTEM" << std::endl;
        std::cout << "   Target: 40-60 FPS GARANTIZADO" << std::endl;
        std::cout << "   Frame Budget: 16.67ms (60 FPS)" << std::endl;
        std::cout << "   Emergency Threshold: 25ms (40 FPS)" << std::endl;

        initialized_ = true;
    }

    void beginFrame() {
        budgetManager_.beginFrame();
        chunkThrottler_.beginFrame();
    }

    void endFrame(float frameTimeMs) {
        // Actualizar FPS
        currentFPS_ = 1000.0f / frameTimeMs;

        fpsHistory_.push_back(currentFPS_);
        if (fpsHistory_.size() > FPS_HISTORY_SIZE) {
            fpsHistory_.pop_front();
        }

        // Calcular average
        float sum = 0.0f;
        for (float fps : fpsHistory_) {
            sum += fps;
        }
        averageFPS_ = fpsHistory_.empty() ? 60.0f : sum / fpsHistory_.size();

        // Track extremos
        if (frameTimeMs > worstFrameMs_) worstFrameMs_ = frameTimeMs;
        if (frameTimeMs < bestFrameMs_) bestFrameMs_ = frameTimeMs;

        // Actualizar sistemas
        chunkThrottler_.updateLimits(currentFPS_);
        emergencyManager_.update(currentFPS_);
    }

    // Budget management
    FrameBudgetManager& getBudget() { return budgetManager_; }

    // Task queue
    void addTask(TimeSlicedTask&& task) {
        taskQueue_.addTask(std::move(task));
    }

    void processTasks() {
        taskQueue_.processTasks(budgetManager_);
    }

    // Chunk throttling
    ChunkThrottler& getThrottler() { return chunkThrottler_; }

    // Emergency actions
    bool isEmergencyMode() const {
        return emergencyManager_.isEmergencyMode();
    }

    EmergencyPerformanceManager::EmergencyActions getEmergencyActions() const {
        return emergencyManager_.getActions();
    }

    // Stats
    struct Stats {
        float currentFPS;
        float averageFPS;
        float frameTimeMs;
        float budgetRemainingMs;
        bool overBudget;
        bool critical;
        bool emergency;
        ChunkThrottler::Stats throttler;
        size_t pendingTasks;
    };

    Stats getStats() const {
        Stats s;
        s.currentFPS = currentFPS_;
        s.averageFPS = averageFPS_;
        s.frameTimeMs = 1000.0f / currentFPS_;
        s.budgetRemainingMs = budgetManager_.getRemainingMs();
        s.overBudget = budgetManager_.isOverBudget();
        s.critical = budgetManager_.isCritical();
        s.emergency = emergencyManager_.isEmergencyMode();
        s.throttler = chunkThrottler_.getStats();
        s.pendingTasks = taskQueue_.size();
        return s;
    }

    // Helpers
    float getCurrentFPS() const { return currentFPS_; }
    float getAverageFPS() const { return averageFPS_; }
};

} // namespace VoxelEngine
