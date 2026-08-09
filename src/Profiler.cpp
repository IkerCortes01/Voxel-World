#include "Profiler.h"

#ifdef _WIN32
#include <windows.h>
#include <gl/GL.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#include <algorithm>
#include <numeric>
#include <cmath>

namespace Profiler {

// ============================================================================
// STATIC INSTANCE
// ============================================================================

ProfilerManager* ProfilerManager::instance_ = nullptr;

ProfilerManager* ProfilerManager::getInstance() {
    if (!instance_) {
        instance_ = new ProfilerManager();
    }
    return instance_;
}

ProfilerManager::ProfilerManager()
    : enabled_(true), visible_(false)
{
    fpsHistory_.reserve(HISTORY_SIZE);
    frameTimeHistory_.reserve(HISTORY_SIZE);
    lastUpdateTime_ = std::chrono::high_resolution_clock::now();
}

// ============================================================================
// FRAME MANAGEMENT
// ============================================================================

void ProfilerManager::beginFrame() {
    if (!enabled_) return;
    // Placeholder para future GPU queries
}

void ProfilerManager::endFrame() {
    if (!enabled_) return;
    updateHistory();
}

void ProfilerManager::updateStats(const FrameStats& stats) {
    if (!enabled_) return;

    currentStats_ = stats;

    // Smooth stats para display más estable
    smoothStats();

    // Actualizar historial
    auto now = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastUpdateTime_).count();

    if (elapsed >= updateIntervalSec_) {
        lastUpdateTime_ = now;
    }
}

void ProfilerManager::smoothStats(float alpha) {
    // Exponential moving average
    smoothedStats_.fps = smoothedStats_.fps * (1.0f - alpha) + currentStats_.fps * alpha;
    smoothedStats_.frameTimeMs = smoothedStats_.frameTimeMs * (1.0f - alpha) + currentStats_.frameTimeMs * alpha;
    smoothedStats_.cpuTimeMs = smoothedStats_.cpuTimeMs * (1.0f - alpha) + currentStats_.cpuTimeMs * alpha;

    // Integers - usar directamente
    smoothedStats_.totalChunks = currentStats_.totalChunks;
    smoothedStats_.readyChunks = currentStats_.readyChunks;
    smoothedStats_.generatingChunks = currentStats_.generatingChunks;
    smoothedStats_.meshingChunks = currentStats_.meshingChunks;
    smoothedStats_.uploadingChunks = currentStats_.uploadingChunks;

    smoothedStats_.drawCalls = currentStats_.drawCalls;
    smoothedStats_.verticesRendered = currentStats_.verticesRendered;
    smoothedStats_.trianglesRendered = currentStats_.trianglesRendered;

    // Memory - smooth
    smoothedStats_.memoryUsedMB = smoothedStats_.memoryUsedMB * (1.0f - alpha) + currentStats_.memoryUsedMB * alpha;
    smoothedStats_.chunkMemoryMB = smoothedStats_.chunkMemoryMB * (1.0f - alpha) + currentStats_.chunkMemoryMB * alpha;
    smoothedStats_.meshMemoryMB = smoothedStats_.meshMemoryMB * (1.0f - alpha) + currentStats_.meshMemoryMB * alpha;
}

void ProfilerManager::updateHistory() {
    if (!enabled_) return;

    // Agregar al historial
    if (fpsHistory_.size() >= HISTORY_SIZE) {
        fpsHistory_.erase(fpsHistory_.begin());
    }
    fpsHistory_.push_back(currentStats_.fps);

    if (frameTimeHistory_.size() >= HISTORY_SIZE) {
        frameTimeHistory_.erase(frameTimeHistory_.begin());
    }
    frameTimeHistory_.push_back(currentStats_.frameTimeMs);
}

// ============================================================================
// FUNCTION TIMING
// ============================================================================

void ProfilerManager::recordFunctionTiming(const std::string& name, float ms) {
    if (!enabled_) return;

    auto& timings = functionTimings_[name];
    timings.push_back(ms);

    // Mantener solo últimos 60 samples
    if (timings.size() > 60) {
        timings.erase(timings.begin());
    }
}

std::vector<std::pair<std::string, float>> ProfilerManager::getTopFunctions(int count) {
    std::vector<std::pair<std::string, float>> result;

    for (const auto& [name, timings] : functionTimings_) {
        if (timings.empty()) continue;

        // Calcular promedio
        float avg = std::accumulate(timings.begin(), timings.end(), 0.0f) / timings.size();
        result.push_back({name, avg});
    }

    // Ordenar por tiempo descendente
    std::sort(result.begin(), result.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Retornar top N
    if (result.size() > (size_t)count) {
        result.resize(count);
    }

    return result;
}

// ============================================================================
// RENDERING OVERLAY
// ============================================================================

void ProfilerManager::renderOverlay(int screenWidth, int screenHeight) {
    if (!visible_ || !enabled_) return;

    // Guardar estados OpenGL
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    // Setup ortho
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, screenHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Background semi-transparente
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0f, 0.0f, 0.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex2f(10, 10);
    glVertex2f(400, 10);
    glVertex2f(400, 400);
    glVertex2f(10, 400);
    glEnd();

    // Texto principal
    int yPos = 20;
    const int lineHeight = 14;

    // FPS y Frame Time
    char buffer[256];
    sprintf(buffer, "FPS: %.1f (%.2f ms)", smoothedStats_.fps, smoothedStats_.frameTimeMs);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight;

    sprintf(buffer, "CPU: %.2f ms | GPU: %.2f ms", smoothedStats_.cpuTimeMs, smoothedStats_.gpuTimeMs);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight + 5;

    // Chunks
    sprintf(buffer, "Chunks: %d total | %d ready", smoothedStats_.totalChunks, smoothedStats_.readyChunks);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight;

    sprintf(buffer, "  Gen: %d | Mesh: %d | Upload: %d",
            smoothedStats_.generatingChunks,
            smoothedStats_.meshingChunks,
            smoothedStats_.uploadingChunks);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight + 5;

    // Rendering
    sprintf(buffer, "Draw Calls: %d", smoothedStats_.drawCalls);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight;

    sprintf(buffer, "Vertices: %d | Tris: %d",
            smoothedStats_.verticesRendered,
            smoothedStats_.trianglesRendered);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight + 5;

    // Memory
    sprintf(buffer, "Memory: %.1f MB total", smoothedStats_.memoryUsedMB);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight;

    sprintf(buffer, "  Chunks: %.1f MB | Mesh: %.1f MB | Tex: %.1f MB",
            smoothedStats_.chunkMemoryMB,
            smoothedStats_.meshMemoryMB,
            smoothedStats_.textureMemoryMB);
    renderText(buffer, 20, yPos, screenWidth, screenHeight);
    yPos += lineHeight + 10;

    // FPS Graph
    if (!fpsHistory_.empty()) {
        renderGraph(fpsHistory_, 20, yPos, 360, 60, 0.0f, 120.0f, 0.2f, 1.0f, 0.2f);
        yPos += 70;
    }

    // Frame Time Graph
    if (!frameTimeHistory_.empty()) {
        renderGraph(frameTimeHistory_, 20, yPos, 360, 60, 0.0f, 33.0f, 1.0f, 0.2f, 0.2f);
    }

    // Restaurar estados
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

void ProfilerManager::renderText(const std::string& text, int x, int y, int screenWidth, int screenHeight) {
    // Texto simple usando GL_LINES (bitmap font estilo retro)
    // Por ahora placeholder - integrar con tu sistema de renderText existente
    glColor3f(1.0f, 1.0f, 1.0f);

    // Esto se debe reemplazar con llamada a tu renderText() de main.cpp
    // renderText(text.c_str(), x, y, screenWidth, screenHeight);
}

void ProfilerManager::renderGraph(const std::vector<float>& data, int x, int y, int width, int height,
                                   float minVal, float maxVal, float r, float g, float b)
{
    if (data.empty()) return;

    // Grid background
    glColor4f(0.1f, 0.1f, 0.1f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + width, y);
    glVertex2f(x + width, y + height);
    glVertex2f(x, y + height);
    glEnd();

    // Grid lines (30 FPS, 60 FPS)
    glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
    glBegin(GL_LINES);
    // 60 FPS line
    float y60 = y + height - (60.0f / maxVal) * height;
    glVertex2f(x, y60);
    glVertex2f(x + width, y60);
    // 30 FPS line
    float y30 = y + height - (30.0f / maxVal) * height;
    glVertex2f(x, y30);
    glVertex2f(x + width, y30);
    glEnd();

    // Graph data
    glColor3f(r, g, b);
    glBegin(GL_LINE_STRIP);

    float xStep = (float)width / (float)data.size();
    for (size_t i = 0; i < data.size(); ++i) {
        float value = std::clamp(data[i], minVal, maxVal);
        float normalized = (value - minVal) / (maxVal - minVal);
        float px = x + i * xStep;
        float py = y + height - normalized * height;
        glVertex2f(px, py);
    }

    glEnd();
}

// ============================================================================
// SCOPED TIMER
// ============================================================================

void ScopedTimer::recordTiming(const char* name, float ms) {
    ProfilerManager::getInstance()->recordFunctionTiming(name, ms);
}

} // namespace Profiler
