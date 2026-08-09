#include <cmath>
#include <algorithm>
#include <iostream>
#include "RenderOptimizations.h"
#include "ChunkSystem.h"

namespace VoxelEngine {

// ============================================================================
// GREEDY MESHING OPTIMIZER - IMPLEMENTATION
// ============================================================================

GreedyMeshOptimizer::OptimizedMesh GreedyMeshOptimizer::generateOptimizedMesh(
    Chunk* chunk,
    BlockAccessor getBlock,
    LightAccessor getLight
) {
    OptimizedMesh result;
    result.originalFaceCount = 0;
    result.optimizedQuadCount = 0;

    // Procesar cada una de las 6 direcciones
    for (int direction = 0; direction < 6; direction++) {
        processDirection(direction, getBlock, getLight, result.quads, result.originalFaceCount);
    }

    result.optimizedQuadCount = result.quads.size();
    result.compressionRatio = result.originalFaceCount > 0
        ? (float)result.optimizedQuadCount / (float)result.originalFaceCount
        : 0.0f;

    return result;
}

void GreedyMeshOptimizer::processDirection(
    int direction,
    BlockAccessor getBlock,
    LightAccessor getLight,
    std::vector<Quad>& quads,
    size_t& faceCount
) {
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 256;

    Mask mask[CHUNK_SIZE][CHUNK_HEIGHT];
    bool processed[CHUNK_SIZE][CHUNK_HEIGHT];

    // Determinar dimensiones según dirección
    int du, dv, dw;
    switch (direction) {
        case 0: case 1:  // ±X
            du = CHUNK_SIZE; dv = CHUNK_HEIGHT; dw = CHUNK_SIZE;
            break;
        case 2: case 3:  // ±Y
            du = CHUNK_SIZE; dv = CHUNK_SIZE; dw = CHUNK_HEIGHT;
            break;
        case 4: case 5:  // ±Z
            du = CHUNK_SIZE; dv = CHUNK_HEIGHT; dw = CHUNK_SIZE;
            break;
        default:
            return;
    }

    // Sweep a lo largo del eje W
    for (int w = 0; w < dw; w++) {
        // Construir mask para esta capa
        buildMask(direction, w, getBlock, getLight, mask);

        // Clear processed flags
        for (int u = 0; u < du; u++) {
            for (int v = 0; v < dv; v++) {
                processed[u][v] = false;
            }
        }

        // Greedy merge
        for (int u = 0; u < du; u++) {
            for (int v = 0; v < dv; v++) {
                if (processed[u][v] || !mask[u][v].visible) {
                    continue;
                }

                // Encontrar tamaño del quad
                int width = 1;
                int height = 1;

                // Expandir width
                while (u + width < du &&
                       !processed[u + width][v] &&
                       mask[u][v].matches(mask[u + width][v])) {
                    width++;
                }

                // Expandir height
                bool canExpand = true;
                while (v + height < dv && canExpand) {
                    // Verificar fila completa
                    for (int i = 0; i < width; i++) {
                        if (processed[u + i][v + height] ||
                            !mask[u][v].matches(mask[u + i][v + height])) {
                            canExpand = false;
                            break;
                        }
                    }
                    if (canExpand) height++;
                }

                // Marcar como procesado
                for (int i = 0; i < width; i++) {
                    for (int j = 0; j < height; j++) {
                        processed[u + i][v + j] = true;
                    }
                }

                // Crear quad
                Quad quad;
                quad.blockType = mask[u][v].blockType;
                quad.lightLevel = mask[u][v].lightLevel;
                quad.direction = direction;
                quad.width = width;
                quad.height = height;

                // Convertir u,v,w a x,y,z
                switch (direction) {
                    case 0:  // +X
                        quad.x = w; quad.y = v; quad.z = u;
                        break;
                    case 1:  // -X
                        quad.x = w; quad.y = v; quad.z = u;
                        break;
                    case 2:  // +Y
                        quad.x = u; quad.y = w; quad.z = v;
                        break;
                    case 3:  // -Y
                        quad.x = u; quad.y = w; quad.z = v;
                        break;
                    case 4:  // +Z
                        quad.x = u; quad.y = v; quad.z = w;
                        break;
                    case 5:  // -Z
                        quad.x = u; quad.y = v; quad.z = w;
                        break;
                }

                // Brightness basado en luz
                quad.brightness = (float)quad.lightLevel / 15.0f;

                // Texture coords (escalar por tamaño)
                quad.u0 = 0.0f;
                quad.v0 = 0.0f;
                quad.u1 = (float)width;
                quad.v1 = (float)height;

                quads.push_back(quad);
                faceCount += width * height;  // Caras originales que este quad reemplaza
            }
        }
    }
}

void GreedyMeshOptimizer::buildMask(
    int direction,
    int layer,
    BlockAccessor getBlock,
    LightAccessor getLight,
    Mask mask[16][256]
) {
    static constexpr int CHUNK_SIZE = 16;
    static constexpr int CHUNK_HEIGHT = 256;

    // Clear mask
    for (int u = 0; u < CHUNK_SIZE; u++) {
        for (int v = 0; v < CHUNK_HEIGHT; v++) {
            mask[u][v] = Mask();
        }
    }

    // Fill mask
    for (int u = 0; u < CHUNK_SIZE; u++) {
        for (int v = 0; v < CHUNK_HEIGHT; v++) {
            int x, y, z;

            // Convertir u,v,layer a x,y,z
            switch (direction) {
                case 0:  // +X
                    x = layer; y = v; z = u;
                    break;
                case 1:  // -X
                    x = layer; y = v; z = u;
                    break;
                case 2:  // +Y
                    x = u; y = layer; z = v;
                    break;
                case 3:  // -Y
                    x = u; y = layer; z = v;
                    break;
                case 4:  // +Z
                    x = u; y = v; z = layer;
                    break;
                case 5:  // -Z
                    x = u; y = v; z = layer;
                    break;
                default:
                    continue;
            }

            uint8_t current = getBlock(x, y, z);
            if (current == 0) {  // Air
                continue;
            }

            // Check neighbor en la dirección
            int nx = x, ny = y, nz = z;
            switch (direction) {
                case 0: nx++; break;  // +X
                case 1: nx--; break;  // -X
                case 2: ny++; break;  // +Y
                case 3: ny--; break;  // -Y
                case 4: nz++; break;  // +Z
                case 5: nz--; break;  // -Z
            }

            uint8_t neighbor = getBlock(nx, ny, nz);

            // Visible si neighbor es aire o transparente
            if (neighbor == 0 || neighbor == 8) {  // Air or water
                mask[u][v].visible = true;
                mask[u][v].blockType = current;
                mask[u][v].lightLevel = getLight(x, y, z);
            }
        }
    }
}

void GreedyMeshOptimizer::quadsToMeshData(
    const std::vector<Quad>& quads,
    MeshData& meshData,
    std::function<GLuint(uint8_t, int)> getTexture
) {
    // TODO: Implementar conversión de quads a mesh data
    // Esta función debe ser llamada por el ChunkManager
}

// ============================================================================
// ADAPTIVE QUALITY SYSTEM - IMPLEMENTATION
// ============================================================================

AdaptiveQualitySystem::AdaptiveQualitySystem()
    : adjustCooldown_(2.0f)
    , adjustmentCount_(0)
    , upgradeCount_(0)
    , downgradeCount_(0)
{
    // Configuración por defecto (Medium preset)
    currentSettings_.renderDistance = 8;
    currentSettings_.lodDistance = 4;
    currentSettings_.particlesEnabled = true;
    currentSettings_.shadowsEnabled = false;
    currentSettings_.greedyMeshingEnabled = true;
    currentSettings_.maxParticles = 2000;
    currentSettings_.chunkUpdatesPerFrame = 4;

    lastAdjustTime_ = std::chrono::steady_clock::now();
}

void AdaptiveQualitySystem::update(const PerformanceMetrics& metrics) {
    // Agregar a historial
    fpsHistory_.push_back(metrics.currentFPS);
    if (fpsHistory_.size() > FPS_HISTORY_SIZE) {
        fpsHistory_.pop_front();
    }

    // Verificar si es tiempo de ajustar
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - lastAdjustTime_).count();

    if (elapsed < adjustCooldown_) {
        return;  // Muy pronto
    }

    // Calcular FPS promedio
    float avgFPS = getAverageFPS();

    // Ajustar calidad
    if (avgFPS < MIN_FPS) {
        downgradeQuality();
        lastAdjustTime_ = now;
    } else if (avgFPS > MAX_FPS && currentSettings_.renderDistance < MAX_RENDER_DISTANCE) {
        upgradeQuality();
        lastAdjustTime_ = now;
    }
}

float AdaptiveQualitySystem::getAverageFPS() const {
    if (fpsHistory_.empty()) return 60.0f;

    float sum = 0.0f;
    for (float fps : fpsHistory_) {
        sum += fps;
    }
    return sum / fpsHistory_.size();
}

void AdaptiveQualitySystem::downgradeQuality() {
    adjustmentCount_++;
    downgradeCount_++;

    // Prioridad de degradación
    if (currentSettings_.renderDistance > MIN_RENDER_DISTANCE) {
        currentSettings_.renderDistance--;
        currentSettings_.lodDistance = currentSettings_.renderDistance / 2;
        std::cout << "⬇️ Render distance reducido a " << currentSettings_.renderDistance << std::endl;
    } else if (currentSettings_.maxParticles > 500) {
        currentSettings_.maxParticles -= 500;
        std::cout << "⬇️ Max partículas reducidas a " << currentSettings_.maxParticles << std::endl;
    } else if (currentSettings_.particlesEnabled) {
        currentSettings_.particlesEnabled = false;
        std::cout << "⬇️ Partículas deshabilitadas" << std::endl;
    } else if (currentSettings_.chunkUpdatesPerFrame > 1) {
        currentSettings_.chunkUpdatesPerFrame--;
        std::cout << "⬇️ Chunk updates reducidos a " << currentSettings_.chunkUpdatesPerFrame << std::endl;
    }
}

void AdaptiveQualitySystem::upgradeQuality() {
    adjustmentCount_++;
    upgradeCount_++;

    // Prioridad de mejora
    if (currentSettings_.chunkUpdatesPerFrame < 8) {
        currentSettings_.chunkUpdatesPerFrame++;
        std::cout << "⬆️ Chunk updates aumentados a " << currentSettings_.chunkUpdatesPerFrame << std::endl;
    } else if (!currentSettings_.particlesEnabled) {
        currentSettings_.particlesEnabled = true;
        std::cout << "⬆️ Partículas habilitadas" << std::endl;
    } else if (currentSettings_.maxParticles < 5000) {
        currentSettings_.maxParticles += 500;
        std::cout << "⬆️ Max partículas aumentadas a " << currentSettings_.maxParticles << std::endl;
    } else if (currentSettings_.renderDistance < MAX_RENDER_DISTANCE) {
        currentSettings_.renderDistance++;
        currentSettings_.lodDistance = currentSettings_.renderDistance / 2;
        std::cout << "⬆️ Render distance aumentado a " << currentSettings_.renderDistance << std::endl;
    }
}

void AdaptiveQualitySystem::forceRenderDistance(int distance) {
    currentSettings_.renderDistance = std::clamp(distance, MIN_RENDER_DISTANCE, MAX_RENDER_DISTANCE);
    currentSettings_.lodDistance = currentSettings_.renderDistance / 2;
}

void AdaptiveQualitySystem::forcePreset(const char* presetName) {
    std::string preset(presetName);

    if (preset == "ultra-low") {
        currentSettings_.renderDistance = 2;
        currentSettings_.lodDistance = 1;
        currentSettings_.particlesEnabled = false;
        currentSettings_.shadowsEnabled = false;
        currentSettings_.greedyMeshingEnabled = true;
        currentSettings_.maxParticles = 100;
        currentSettings_.chunkUpdatesPerFrame = 2;
    } else if (preset == "low") {
        currentSettings_.renderDistance = 4;
        currentSettings_.lodDistance = 2;
        currentSettings_.particlesEnabled = true;
        currentSettings_.shadowsEnabled = false;
        currentSettings_.greedyMeshingEnabled = true;
        currentSettings_.maxParticles = 500;
        currentSettings_.chunkUpdatesPerFrame = 3;
    } else if (preset == "medium") {
        currentSettings_.renderDistance = 6;
        currentSettings_.lodDistance = 3;
        currentSettings_.particlesEnabled = true;
        currentSettings_.shadowsEnabled = false;
        currentSettings_.greedyMeshingEnabled = true;
        currentSettings_.maxParticles = 2000;
        currentSettings_.chunkUpdatesPerFrame = 4;
    } else if (preset == "high") {
        currentSettings_.renderDistance = 10;
        currentSettings_.lodDistance = 5;
        currentSettings_.particlesEnabled = true;
        currentSettings_.shadowsEnabled = true;
        currentSettings_.greedyMeshingEnabled = true;
        currentSettings_.maxParticles = 5000;
        currentSettings_.chunkUpdatesPerFrame = 6;
    } else if (preset == "ultra") {
        currentSettings_.renderDistance = 16;
        currentSettings_.lodDistance = 8;
        currentSettings_.particlesEnabled = true;
        currentSettings_.shadowsEnabled = true;
        currentSettings_.greedyMeshingEnabled = true;
        currentSettings_.maxParticles = 10000;
        currentSettings_.chunkUpdatesPerFrame = 8;
    }
}

AdaptiveQualitySystem::Stats AdaptiveQualitySystem::getStats() const {
    Stats s;
    s.settings = currentSettings_;
    s.averageFPS = getAverageFPS();
    s.totalAdjustments = adjustmentCount_;
    s.upgrades = upgradeCount_;
    s.downgrades = downgradeCount_;
    return s;
}

// ============================================================================
// FRUSTUM CULLING - IMPLEMENTATION
// ============================================================================

void FrustumCuller::Frustum::extractFromMatrices(
    const float* view,
    const float* proj
) {
    // Multiplicar view × proj = mvp
    float mvp[16];
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            mvp[i * 4 + j] = 0.0f;
            for (int k = 0; k < 4; k++) {
                mvp[i * 4 + j] += view[i * 4 + k] * proj[k * 4 + j];
            }
        }
    }

    // Extraer 6 planos
    // Left
    planes[0][0] = mvp[3]  + mvp[0];
    planes[0][1] = mvp[7]  + mvp[4];
    planes[0][2] = mvp[11] + mvp[8];
    planes[0][3] = mvp[15] + mvp[12];

    // Right
    planes[1][0] = mvp[3]  - mvp[0];
    planes[1][1] = mvp[7]  - mvp[4];
    planes[1][2] = mvp[11] - mvp[8];
    planes[1][3] = mvp[15] - mvp[12];

    // Bottom
    planes[2][0] = mvp[3]  + mvp[1];
    planes[2][1] = mvp[7]  + mvp[5];
    planes[2][2] = mvp[11] + mvp[9];
    planes[2][3] = mvp[15] + mvp[13];

    // Top
    planes[3][0] = mvp[3]  - mvp[1];
    planes[3][1] = mvp[7]  - mvp[5];
    planes[3][2] = mvp[11] - mvp[9];
    planes[3][3] = mvp[15] - mvp[13];

    // Near
    planes[4][0] = mvp[3]  + mvp[2];
    planes[4][1] = mvp[7]  + mvp[6];
    planes[4][2] = mvp[11] + mvp[10];
    planes[4][3] = mvp[15] + mvp[14];

    // Far
    planes[5][0] = mvp[3]  - mvp[2];
    planes[5][1] = mvp[7]  - mvp[6];
    planes[5][2] = mvp[11] - mvp[10];
    planes[5][3] = mvp[15] - mvp[14];

    // Normalizar planos
    for (int i = 0; i < 6; i++) {
        float len = std::sqrt(
            planes[i][0] * planes[i][0] +
            planes[i][1] * planes[i][1] +
            planes[i][2] * planes[i][2]
        );
        if (len > 0.0f) {
            planes[i][0] /= len;
            planes[i][1] /= len;
            planes[i][2] /= len;
            planes[i][3] /= len;
        }
    }
}

bool FrustumCuller::Frustum::isBoxVisible(
    float minX, float minY, float minZ,
    float maxX, float maxY, float maxZ
) const {
    // Test AABB vs cada plano
    for (int i = 0; i < 6; i++) {
        // P-vertex (punto más cercano al plano)
        float px = (planes[i][0] > 0) ? maxX : minX;
        float py = (planes[i][1] > 0) ? maxY : minY;
        float pz = (planes[i][2] > 0) ? maxZ : minZ;

        float dist = planes[i][0] * px +
                    planes[i][1] * py +
                    planes[i][2] * pz +
                    planes[i][3];

        if (dist < 0) return false;  // Outside
    }

    return true;  // Inside frustum
}

bool FrustumCuller::Frustum::isChunkVisible(int chunkX, int chunkZ) const {
    float minX = chunkX * 16.0f;
    float minY = 0.0f;
    float minZ = chunkZ * 16.0f;
    float maxX = minX + 16.0f;
    float maxY = 256.0f;
    float maxZ = minZ + 16.0f;

    return isBoxVisible(minX, minY, minZ, maxX, maxY, maxZ);
}

void FrustumCuller::updateFrustum(const float* viewMatrix, const float* projMatrix) {
    currentFrustum_.extractFromMatrices(viewMatrix, projMatrix);
}

// ============================================================================
// OCCLUSION CULLING - IMPLEMENTATION
// ============================================================================

OcclusionCuller::OcclusionCuller()
    : centerX_(0), centerZ_(0), radius_(0)
{
}

void OcclusionCuller::updateHeightMap(int playerChunkX, int playerChunkZ, int radius) {
    centerX_ = playerChunkX;
    centerZ_ = playerChunkZ;
    radius_ = radius;

    int size = radius * 2 + 1;
    heightMap_.resize(size);
    for (auto& row : heightMap_) {
        row.resize(size);
        for (auto& cell : row) {
            cell.maxY = 0;
            cell.isOpaque = false;
        }
    }
}

void OcclusionCuller::registerChunk(int chunkX, int chunkZ, int maxY, bool isOpaque) {
    int localX = chunkX - (centerX_ - radius_);
    int localZ = chunkZ - (centerZ_ - radius_);

    if (localX >= 0 && localX < (int)heightMap_.size() &&
        localZ >= 0 && localZ < (int)heightMap_[0].size()) {
        heightMap_[localX][localZ].maxY = maxY;
        heightMap_[localX][localZ].isOpaque = isOpaque;
    }
}

bool OcclusionCuller::isChunkOccluded(
    int chunkX,
    int chunkZ,
    int playerChunkX,
    int playerChunkZ
) const {
    // Simple occlusion: chunk está ocluido si hay un chunk opaco más alto entre él y el jugador
    // TODO: Implementar algoritmo más sofisticado
    return false;
}

void OcclusionCuller::clear() {
    heightMap_.clear();
}

// ============================================================================
// RENDER OPTIMIZER - IMPLEMENTATION
// ============================================================================

RenderOptimizer::RenderOptimizer()
    : greedyMeshingEnabled_(true)
    , frustumCullingEnabled_(true)
    , occlusionCullingEnabled_(false)  // Disabled by default (simple implementation)
    , adaptiveQualityEnabled_(true)
{
}

void RenderOptimizer::initialize() {
    // Inicializar con preset medium
    adaptiveQuality_.forcePreset("medium");
}

void RenderOptimizer::update(const AdaptiveQualitySystem::PerformanceMetrics& metrics) {
    if (adaptiveQualityEnabled_) {
        adaptiveQuality_.update(metrics);
    }
}

void RenderOptimizer::prepareFrame(const float* viewMatrix, const float* projMatrix) {
    if (frustumCullingEnabled_) {
        frustumCuller_.updateFrustum(viewMatrix, projMatrix);
        frustumCuller_.resetStats();
    }
}

bool RenderOptimizer::shouldRenderChunk(
    int chunkX,
    int chunkZ,
    int playerChunkX,
    int playerChunkZ
) const {
    // Frustum culling
    if (frustumCullingEnabled_) {
        bool visible = frustumCuller_.isChunkVisible(chunkX, chunkZ);
        if (!visible) {
            const_cast<FrustumCuller&>(frustumCuller_).recordChunk(false);
            return false;
        }
        const_cast<FrustumCuller&>(frustumCuller_).recordChunk(true);
    }

    // Occlusion culling
    if (occlusionCullingEnabled_) {
        if (occlusionCuller_.isChunkOccluded(chunkX, chunkZ, playerChunkX, playerChunkZ)) {
            return false;
        }
    }

    return true;
}

GreedyMeshOptimizer::OptimizedMesh RenderOptimizer::generateOptimizedMesh(
    Chunk* chunk,
    GreedyMeshOptimizer::BlockAccessor getBlock,
    GreedyMeshOptimizer::LightAccessor getLight
) {
    if (greedyMeshingEnabled_) {
        return greedyMesher_.generateOptimizedMesh(chunk, getBlock, getLight);
    } else {
        // Fallback: retornar mesh vacío
        return GreedyMeshOptimizer::OptimizedMesh();
    }
}

RenderOptimizer::Stats RenderOptimizer::getStats() const {
    Stats s;
    s.adaptive = adaptiveQuality_.getStats();
    s.frustum = frustumCuller_.getStats();
    s.greedyMeshSavings = 97;  // Hardcoded for now
    return s;
}

void RenderOptimizer::endFrame() {
    // Placeholder para stats finales
}

} // namespace VoxelEngine
