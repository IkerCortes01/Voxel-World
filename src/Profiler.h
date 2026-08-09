#pragma once

#include <chrono>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

// ============================================================================
// IN-GAME PROFILER - Visual Performance Overlay
// ============================================================================
// Professional profiling system with visual overlay (F3 to toggle)
// Tracks FPS, frame time, chunk stats, memory, draw calls
// Zero-cost when disabled, minimal overhead when enabled
// ============================================================================

namespace Profiler {

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

struct FrameStats {
    // Timing
    float fps = 0.0f;
    float frameTimeMs = 0.0f;
    float cpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;

    // Chunks
    int totalChunks = 0;
    int readyChunks = 0;
    int generatingChunks = 0;
    int meshingChunks = 0;
    int uploadingChunks = 0;

    // Rendering
    int drawCalls = 0;
    int verticesRendered = 0;
    int trianglesRendered = 0;

    // Memory (MB)
    float memoryUsedMB = 0.0f;
    float chunkMemoryMB = 0.0f;
    float meshMemoryMB = 0.0f;
    float textureMemoryMB = 0.0f;

    // Input latency
    float inputLatencyMs = 0.0f;
};

// ============================================================================
// SCOPED TIMER - RAII timing for functions
// ============================================================================

class ScopedTimer {
private:
    std::chrono::high_resolution_clock::time_point start_;
    const char* name_;
    float* targetMs_;

public:
    ScopedTimer(const char* name, float* targetMs = nullptr)
        : name_(name), targetMs_(targetMs)
    {
        start_ = std::chrono::high_resolution_clock::now();
    }

    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        float ms = std::chrono::duration<float, std::milli>(end - start_).count();

        if (targetMs_) {
            *targetMs_ = ms;
        }

        // Log to profiler history
        recordTiming(name_, ms);
    }

    static void recordTiming(const char* name, float ms);
};

// Macro para profiling fácil
#define PROFILE_SCOPE(name) Profiler::ScopedTimer _timer_##__LINE__(name)
#define PROFILE_SCOPE_MS(name, target) Profiler::ScopedTimer _timer_##__LINE__(name, &target)

// ============================================================================
// PROFILER MANAGER
// ============================================================================

class ProfilerManager {
private:
    static ProfilerManager* instance_;

    bool enabled_ = false;
    bool visible_ = true;

    FrameStats currentStats_;
    FrameStats smoothedStats_;  // Smoothed para evitar parpadeo

    // Historial para gráficas
    static constexpr int HISTORY_SIZE = 120;  // 2 segundos @ 60 FPS
    std::vector<float> fpsHistory_;
    std::vector<float> frameTimeHistory_;

    // Timings de funciones
    std::map<std::string, std::vector<float>> functionTimings_;

    // Update timing
    std::chrono::high_resolution_clock::time_point lastUpdateTime_;
    float updateIntervalSec_ = 0.5f;  // Actualizar display cada 0.5s

    ProfilerManager();

public:
    static ProfilerManager* getInstance();

    // Control
    void toggle() { visible_ = !visible_; }
    void setEnabled(bool enabled) { enabled_ = enabled; }
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    bool isEnabled() const { return enabled_; }

    // Update stats
    void beginFrame();
    void endFrame();
    void updateStats(const FrameStats& stats);

    // Get stats
    const FrameStats& getStats() const { return smoothedStats_; }
    const std::vector<float>& getFPSHistory() const { return fpsHistory_; }
    const std::vector<float>& getFrameTimeHistory() const { return frameTimeHistory_; }

    // Function timings
    void recordFunctionTiming(const std::string& name, float ms);
    std::vector<std::pair<std::string, float>> getTopFunctions(int count = 10);

    // Render overlay
    void renderOverlay(int screenWidth, int screenHeight);

private:
    void smoothStats(float alpha = 0.1f);  // Exponential smoothing
    void updateHistory();
    void renderText(const std::string& text, int x, int y, int screenWidth, int screenHeight);
    void renderGraph(const std::vector<float>& data, int x, int y, int width, int height,
                     float minVal, float maxVal, float r, float g, float b);
};

// ============================================================================
// CONVENIENCE FUNCTIONS
// ============================================================================

inline void beginFrame() { ProfilerManager::getInstance()->beginFrame(); }
inline void endFrame() { ProfilerManager::getInstance()->endFrame(); }
inline void updateStats(const FrameStats& stats) { ProfilerManager::getInstance()->updateStats(stats); }
inline void toggle() { ProfilerManager::getInstance()->toggle(); }
inline bool isVisible() { return ProfilerManager::getInstance()->isVisible(); }

} // namespace Profiler
