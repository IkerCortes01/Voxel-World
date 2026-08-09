#pragma once

#include <chrono>
#include <deque>

// ============================================================================
// ADAPTIVE QUALITY SYSTEM - Dynamic Performance Scaling
// ============================================================================
// Ajusta render distance, LOD, effects en tiempo real para mantener 60 FPS
// Sistema inspirado en Minecraft's performance auto-adjust
// ============================================================================

class AdaptiveQuality {
private:
    // Target FPS
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float MIN_FPS = 50.0f;
    static constexpr float MAX_FPS = 70.0f;

    // Render distance limits
    static constexpr int MIN_RENDER_DISTANCE = 2;
    static constexpr int MAX_RENDER_DISTANCE = 12;

    // FPS history for smoothing
    std::deque<float> fpsHistory_;
    static constexpr size_t HISTORY_SIZE = 60;  // 1 segundo @ 60 FPS

    // Current settings
    int renderDistance_ = 8;
    int lodDistance_ = 4;
    bool particlesEnabled_ = true;
    bool shadowsEnabled_ = true;
    int maxParticles_ = 5000;

    // Timing
    std::chrono::high_resolution_clock::time_point lastAdjustTime_;
    float adjustCooldown_ = 2.0f;  // Ajustar cada 2 segundos

    // Stats
    size_t adjustmentCount_ = 0;
    size_t upgradeCount_ = 0;
    size_t downgradeCount_ = 0;

public:
    AdaptiveQuality() {
        lastAdjustTime_ = std::chrono::high_resolution_clock::now();
    }

    // Update with current FPS
    void update(float currentFPS) {
        // Add to history
        fpsHistory_.push_back(currentFPS);
        if (fpsHistory_.size() > HISTORY_SIZE) {
            fpsHistory_.pop_front();
        }

        // Check if it's time to adjust
        auto now = std::chrono::high_resolution_clock::now();
        float elapsed = std::chrono::duration<float>(now - lastAdjustTime_).count();

        if (elapsed < adjustCooldown_) {
            return;  // Too soon
        }

        // Calculate average FPS
        float avgFPS = getAverageFPS();

        // Adjust quality
        if (avgFPS < MIN_FPS) {
            downgradeQuality();
            lastAdjustTime_ = now;
        } else if (avgFPS > MAX_FPS && renderDistance_ < MAX_RENDER_DISTANCE) {
            upgradeQuality();
            lastAdjustTime_ = now;
        }
    }

    // Getters
    int getRenderDistance() const { return renderDistance_; }
    int getLODDistance() const { return lodDistance_; }
    bool particlesEnabled() const { return particlesEnabled_; }
    bool shadowsEnabled() const { return shadowsEnabled_; }
    int getMaxParticles() const { return maxParticles_; }

    // Stats
    struct Stats {
        int renderDistance;
        int lodDistance;
        bool particles;
        bool shadows;
        int maxParticles;
        size_t adjustments;
        size_t upgrades;
        size_t downgrades;
        float averageFPS;
    };

    Stats getStats() const {
        Stats s;
        s.renderDistance = renderDistance_;
        s.lodDistance = lodDistance_;
        s.particles = particlesEnabled_;
        s.shadows = shadowsEnabled_;
        s.maxParticles = maxParticles_;
        s.adjustments = adjustmentCount_;
        s.upgrades = upgradeCount_;
        s.downgrades = downgradeCount_;
        s.averageFPS = getAverageFPS();
        return s;
    }

    // Manual override
    void setRenderDistance(int distance) {
        renderDistance_ = std::clamp(distance, MIN_RENDER_DISTANCE, MAX_RENDER_DISTANCE);
        lodDistance_ = renderDistance_ / 2;
    }

    void setParticlesEnabled(bool enabled) {
        particlesEnabled_ = enabled;
    }

private:
    float getAverageFPS() const {
        if (fpsHistory_.empty()) return 60.0f;

        float sum = 0.0f;
        for (float fps : fpsHistory_) {
            sum += fps;
        }
        return sum / fpsHistory_.size();
    }

    void downgradeQuality() {
        adjustmentCount_++;
        downgradeCount_++;

        // Prioridad de degradación
        if (renderDistance_ > MIN_RENDER_DISTANCE) {
            renderDistance_--;
            lodDistance_ = renderDistance_ / 2;
            std::cout << "⬇️ Render distance reducido a " << renderDistance_ << std::endl;
        } else if (maxParticles_ > 1000) {
            maxParticles_ -= 1000;
            std::cout << "⬇️ Max partículas reducidas a " << maxParticles_ << std::endl;
        } else if (particlesEnabled_) {
            particlesEnabled_ = false;
            std::cout << "⬇️ Partículas deshabilitadas" << std::endl;
        } else if (shadowsEnabled_) {
            shadowsEnabled_ = false;
            std::cout << "⬇️ Sombras deshabilitadas" << std::endl;
        }
    }

    void upgradeQuality() {
        adjustmentCount_++;
        upgradeCount_++;

        // Prioridad de mejora (inversa a degradación)
        if (!shadowsEnabled_) {
            shadowsEnabled_ = true;
            std::cout << "⬆️ Sombras habilitadas" << std::endl;
        } else if (!particlesEnabled_) {
            particlesEnabled_ = true;
            std::cout << "⬆️ Partículas habilitadas" << std::endl;
        } else if (maxParticles_ < 5000) {
            maxParticles_ += 1000;
            std::cout << "⬆️ Max partículas aumentadas a " << maxParticles_ << std::endl;
        } else if (renderDistance_ < MAX_RENDER_DISTANCE) {
            renderDistance_++;
            lodDistance_ = renderDistance_ / 2;
            std::cout << "⬆️ Render distance aumentado a " << renderDistance_ << std::endl;
        }
    }
};

// ============================================================================
// LOD SYSTEM - Level of Detail for chunks
// ============================================================================

enum class ChunkLOD {
    FULL,        // Full detail (cerca del jugador)
    SIMPLIFIED,  // Reduced detail (medio)
    IMPOSTOR     // Billboard/very low poly (lejos)
};

inline ChunkLOD calculateChunkLOD(float distanceToPlayer, int lodDistance) {
    if (distanceToPlayer < lodDistance * 16) {
        return ChunkLOD::FULL;
    } else if (distanceToPlayer < lodDistance * 24) {
        return ChunkLOD::SIMPLIFIED;
    } else {
        return ChunkLOD::IMPOSTOR;
    }
}

// ============================================================================
// PERFORMANCE PRESET - Predefined quality levels
// ============================================================================

struct PerformancePreset {
    const char* name;
    int renderDistance;
    int lodDistance;
    bool particles;
    bool shadows;
    int maxParticles;
    bool greedyMeshing;
    bool frustumCulling;
};

static const PerformancePreset PRESETS[] = {
    // Ultra Low (Intel HD 4000, very old GPUs)
    {"Ultra Low", 2, 1, false, false, 100, true, true},

    // Low (Intel HD 5000-6000)
    {"Low", 4, 2, true, false, 500, true, true},

    // Medium (Intel Iris, GTX 750)
    {"Medium", 6, 3, true, false, 2000, true, true},

    // High (GTX 1050, RX 560)
    {"High", 8, 4, true, true, 5000, true, true},

    // Ultra (GTX 1060+, RX 580+)
    {"Ultra", 12, 6, true, true, 10000, true, true}
};

inline const PerformancePreset& getPresetForHardware() {
    // Auto-detect hardware tier
    // Por ahora retornar Medium por defecto
    return PRESETS[2];  // Medium
}
