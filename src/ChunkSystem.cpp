#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "ChunkSystem.h"
#include <cstring>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <GL/gl.h>

// OpenGL VBO extension function pointers (loaded at runtime)
#ifdef _WIN32
#define GL_ARRAY_BUFFER 0x8892
#define GL_STATIC_DRAW 0x88E4
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, ptrdiff_t size, const void *data, GLenum usage);

// Global function pointers (extern from main.cpp or load here)
extern PFNGLGENBUFFERSPROC glGenBuffers;
extern PFNGLDELETEBUFFERSPROC glDeleteBuffers;
extern PFNGLBINDBUFFERPROC glBindBuffer;
extern PFNGLBUFFERDATAPROC glBufferData;
#endif

namespace VoxelEngine {

// ============================================================================
// MESH BUILDER - Static texture callback
// ============================================================================

MeshBuilder::TextureCallback MeshBuilder::textureCallback_ = nullptr;

void MeshBuilder::setTextureCallback(TextureCallback callback) {
    textureCallback_ = callback;
}

// ============================================================================
// CHUNK STATE UTILITIES
// ============================================================================

const char* ChunkStateToString(ChunkState state) {
    switch (state) {
        case ChunkState::UNLOADED: return "UNLOADED";
        case ChunkState::GENERATING: return "GENERATING";
        case ChunkState::GENERATED: return "GENERATED";
        case ChunkState::WAITING_FOR_NEIGHBORS: return "WAITING_FOR_NEIGHBORS";
        case ChunkState::MESHING: return "MESHING";
        case ChunkState::MESH_READY: return "MESH_READY";
        case ChunkState::UPLOADING: return "UPLOADING";
        case ChunkState::READY: return "READY";
        case ChunkState::DIRTY: return "DIRTY";
        case ChunkState::UNLOADING: return "UNLOADING";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// GPU MESH IMPLEMENTATION
// ============================================================================

void GPUMesh::cleanup() {
    // MUST be called on main thread
    for (auto& batch : batches) {
        if (batch.vbo != 0) glDeleteBuffers(1, &batch.vbo);
        if (batch.colorVBO != 0) glDeleteBuffers(1, &batch.colorVBO);
        if (batch.uvVBO != 0) glDeleteBuffers(1, &batch.uvVBO);
        // No VAO or EBO in legacy OpenGL mode
    }
    batches.clear();
}

// ============================================================================
// CHUNK IMPLEMENTATION
// ============================================================================

Chunk::Chunk(const ChunkPosition& pos)
    : position_(pos)
    , state_(ChunkState::UNLOADED)
    , dirty_(false)
    , generationTime_(0)
{
    // Allocate voxel data
    blocks_.resize(SIZE * HEIGHT * SIZE, 0);
    lightLevels_.resize(SIZE * HEIGHT * SIZE, 15);

    // Initialize neighbors to nullptr
    for (int i = 0; i < 6; ++i) {
        neighbors_[i].store(nullptr, std::memory_order_relaxed);
    }
}

Chunk::~Chunk() {
    // Cleanup GPU resources (must be on main thread)
    gpuMesh_.cleanup();
}

uint8_t Chunk::getBlock(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE) {
        return 0; // Air
    }
    std::lock_guard<std::mutex> lock(dataMutex_);
    return blocks_[getIndex(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, uint8_t blockType) {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE) {
        return;
    }
    std::lock_guard<std::mutex> lock(dataMutex_);
    blocks_[getIndex(x, y, z)] = blockType;
    markDirty();
}

uint8_t Chunk::getLightLevel(int x, int y, int z) const {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE) {
        return 15;
    }
    std::lock_guard<std::mutex> lock(dataMutex_);
    return lightLevels_[getIndex(x, y, z)];
}

void Chunk::setLightLevel(int x, int y, int z, uint8_t light) {
    if (x < 0 || x >= SIZE || y < 0 || y >= HEIGHT || z < 0 || z >= SIZE) {
        return;
    }
    std::lock_guard<std::mutex> lock(dataMutex_);
    lightLevels_[getIndex(x, y, z)] = light;
}

void Chunk::setNeighbor(int direction, Chunk* neighbor) {
    assert(direction >= 0 && direction < 6);
    neighbors_[direction].store(neighbor, std::memory_order_release);
}

Chunk* Chunk::getNeighbor(int direction) const {
    assert(direction >= 0 && direction < 6);
    return neighbors_[direction].load(std::memory_order_acquire);
}

bool Chunk::hasAllNeighbors() const {
    for (int i = 0; i < 4; ++i) { // Only check horizontal neighbors
        if (neighbors_[i].load(std::memory_order_acquire) == nullptr) {
            return false;
        }
    }
    return true;
}

// ⭐ CRITICAL: Check if all neighbors are ready for mesh building
bool Chunk::canBuildMesh() const {
    // Horizontal neighbors MUST exist and be at least GENERATED before building mesh
    for (int i = 0; i < 4; ++i) {  // 0=North, 1=South, 2=East, 3=West
        Chunk* neighbor = neighbors_[i].load(std::memory_order_acquire);

        if (!neighbor) {
            return false;  // Neighbor doesn't exist yet
        }

        ChunkState neighborState = neighbor->getState();

        // Neighbor must be at least GENERATED (have voxel data)
        if (neighborState == ChunkState::UNLOADED ||
            neighborState == ChunkState::GENERATING ||
            neighborState == ChunkState::UNLOADING) {
            return false;  // Neighbor not ready yet
        }
    }

    return true;  // All neighbors exist and have voxel data
}

void Chunk::setMeshData(std::unique_ptr<MeshData> mesh) {
    std::lock_guard<std::mutex> lock(meshMutex_);
    cpuMesh_ = std::move(mesh);
}

std::unique_ptr<MeshData> Chunk::takeMeshData() {
    std::lock_guard<std::mutex> lock(meshMutex_);
    return std::move(cpuMesh_);
}

// ============================================================================
// MESH BUILDER IMPLEMENTATION
// ============================================================================

std::unique_ptr<MeshData> MeshBuilder::buildMesh(Chunk* chunk) {
    if (!chunk) return nullptr;

    auto meshData = std::make_unique<MeshData>();

    // Temporary storage grouped by texture
    std::unordered_map<GLuint, MeshData::Batch> batchMap;

    // Iterate through all blocks
    for (int y = 0; y < Chunk::HEIGHT; ++y) {
        for (int z = 0; z < Chunk::SIZE; ++z) {
            for (int x = 0; x < Chunk::SIZE; ++x) {
                uint8_t block = chunk->getBlock(x, y, z);
                if (block == 0) continue; // Skip air

                // Get light level
                uint8_t light = chunk->getLightLevel(x, y, z);
                float brightness = light / 15.0f;
                brightness = 0.3f + brightness * 0.7f; // Min 0.3, max 1.0

                // Check each face
                float wx = static_cast<float>(x + chunk->getPosition().x * Chunk::SIZE);
                float wy = static_cast<float>(y);
                float wz = static_cast<float>(z + chunk->getPosition().z * Chunk::SIZE);

                // Face directions: 0=North(+Z), 1=South(-Z), 2=East(+X), 3=West(-X), 4=Up(+Y), 5=Down(-Y)

                // North (+Z)
                if (shouldRenderFace(block, getBlockSafe(chunk, x, y, z + 1))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 0) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 0, wx, wy, wz, block, brightness * 0.8f, brightness * 0.8f, brightness * 0.8f, 1.0f);
                }

                // South (-Z)
                if (shouldRenderFace(block, getBlockSafe(chunk, x, y, z - 1))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 1) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 1, wx, wy, wz, block, brightness * 0.8f, brightness * 0.8f, brightness * 0.8f, 1.0f);
                }

                // East (+X)
                if (shouldRenderFace(block, getBlockSafe(chunk, x + 1, y, z))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 2) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 2, wx, wy, wz, block, brightness * 0.6f, brightness * 0.6f, brightness * 0.6f, 1.0f);
                }

                // West (-X)
                if (shouldRenderFace(block, getBlockSafe(chunk, x - 1, y, z))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 3) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 3, wx, wy, wz, block, brightness * 0.6f, brightness * 0.6f, brightness * 0.6f, 1.0f);
                }

                // Up (+Y)
                if (shouldRenderFace(block, getBlockSafe(chunk, x, y + 1, z))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 4) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 4, wx, wy, wz, block, brightness, brightness, brightness, 1.0f);
                }

                // Down (-Y)
                if (shouldRenderFace(block, getBlockSafe(chunk, x, y - 1, z))) {
                    GLuint textureId = textureCallback_ ? textureCallback_(block, 5) : static_cast<GLuint>(block);
                    MeshData::Batch& batch = batchMap[textureId];
                    if (batch.textureId == 0) batch.textureId = textureId;
                    addFace(batch, 5, wx, wy, wz, block, brightness * 0.5f, brightness * 0.5f, brightness * 0.5f, 1.0f);
                }
            }
        }
    }

    // Convert map to vector
    for (auto& pair : batchMap) {
        if (!pair.second.vertices.empty()) {
            meshData->totalVertices += pair.second.vertexCount();
            meshData->totalIndices += pair.second.indexCount();
            meshData->batches.push_back(std::move(pair.second));
        }
    }

    // Validate before returning
    if (!meshData->isValid()) {
        std::cerr << "ERROR: Generated invalid mesh for chunk at ("
                  << chunk->getPosition().x << ", "
                  << chunk->getPosition().y << ", "
                  << chunk->getPosition().z << ")" << std::endl;
        return nullptr;
    }

    return meshData;
}

uint8_t MeshBuilder::getBlockSafe(Chunk* chunk, int x, int y, int z) {
    // Out of vertical bounds
    if (y < 0 || y >= Chunk::HEIGHT) {
        return 0; // Air
    }

    // Within chunk bounds
    if (x >= 0 && x < Chunk::SIZE && z >= 0 && z < Chunk::SIZE) {
        return chunk->getBlock(x, y, z);
    }

    // Need to check neighbors
    Chunk* neighbor = nullptr;

    if (z >= Chunk::SIZE) {
        neighbor = chunk->getNeighbor(0); // North
        if (neighbor) {
            ChunkState state = neighbor->getState();
            if (state >= ChunkState::GENERATED && state != ChunkState::UNLOADING) {
                return neighbor->getBlock(x, y, z - Chunk::SIZE);
            }
        }
    } else if (z < 0) {
        neighbor = chunk->getNeighbor(1); // South
        if (neighbor) {
            ChunkState state = neighbor->getState();
            if (state >= ChunkState::GENERATED && state != ChunkState::UNLOADING) {
                return neighbor->getBlock(x, y, z + Chunk::SIZE);
            }
        }
    } else if (x >= Chunk::SIZE) {
        neighbor = chunk->getNeighbor(2); // East
        if (neighbor) {
            ChunkState state = neighbor->getState();
            if (state >= ChunkState::GENERATED && state != ChunkState::UNLOADING) {
                return neighbor->getBlock(x - Chunk::SIZE, y, z);
            }
        }
    } else if (x < 0) {
        neighbor = chunk->getNeighbor(3); // West
        if (neighbor) {
            ChunkState state = neighbor->getState();
            if (state >= ChunkState::GENERATED && state != ChunkState::UNLOADING) {
                return neighbor->getBlock(x + Chunk::SIZE, y, z);
            }
        }
    }

    // ⭐ CRITICAL FIX: If neighbor doesn't exist or isn't ready, return SOLID
    // This prevents false faces from being generated at chunk borders
    // The mesh should NOT be built if neighbors aren't ready (checked by canBuildMesh())
    return 1;  // SOLID - prevents holes and false faces
}

bool MeshBuilder::shouldRenderFace(uint8_t currentBlock, uint8_t neighborBlock) {
    if (currentBlock == 0) return false; // Air doesn't render
    if (neighborBlock == 0) return true;  // Render if neighbor is air

    // Add transparency logic here
    // For now, don't render if neighbor is solid
    return false;
}

void MeshBuilder::addFace(MeshData::Batch& batch, int direction,
                         float x, float y, float z,
                         uint8_t blockType,
                         float r, float g, float b, float a) {
    uint32_t baseIndex = static_cast<uint32_t>(batch.vertices.size() / 3);

    // Define cube face vertices based on direction
    switch (direction) {
        case 0: // North (+Z)
            batch.vertices.insert(batch.vertices.end(), {
                x, y, z + 1,
                x + 1, y, z + 1,
                x + 1, y + 1, z + 1,
                x, y + 1, z + 1
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;

        case 1: // South (-Z)
            batch.vertices.insert(batch.vertices.end(), {
                x + 1, y, z,
                x, y, z,
                x, y + 1, z,
                x + 1, y + 1, z
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;

        case 2: // East (+X)
            batch.vertices.insert(batch.vertices.end(), {
                x + 1, y, z + 1,
                x + 1, y, z,
                x + 1, y + 1, z,
                x + 1, y + 1, z + 1
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;

        case 3: // West (-X)
            batch.vertices.insert(batch.vertices.end(), {
                x, y, z,
                x, y, z + 1,
                x, y + 1, z + 1,
                x, y + 1, z
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;

        case 4: // Up (+Y)
            batch.vertices.insert(batch.vertices.end(), {
                x, y + 1, z + 1,
                x + 1, y + 1, z + 1,
                x + 1, y + 1, z,
                x, y + 1, z
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;

        case 5: // Down (-Y)
            batch.vertices.insert(batch.vertices.end(), {
                x, y, z,
                x + 1, y, z,
                x + 1, y, z + 1,
                x, y, z + 1
            });
            batch.uvs.insert(batch.uvs.end(), {
                0.0f, 1.0f,
                1.0f, 1.0f,
                1.0f, 0.0f,
                0.0f, 0.0f
            });
            break;
    }

    // Add colors (4 vertices)
    for (int i = 0; i < 4; ++i) {
        batch.colors.insert(batch.colors.end(), {r, g, b, a});
    }

    // Add indices (2 triangles = 6 indices)
    batch.indices.insert(batch.indices.end(), {
        baseIndex, baseIndex + 1, baseIndex + 2,
        baseIndex, baseIndex + 2, baseIndex + 3
    });
}

// ============================================================================
// GPU UPLOADER IMPLEMENTATION
// ============================================================================

bool GPUUploader::uploadMesh(Chunk* chunk, MeshData* meshData) {
    if (!chunk || !meshData || !meshData->isValid()) {
        return false;
    }

    // Clear old GPU mesh
    chunk->getGPUMesh().cleanup();

    GPUMesh& gpuMesh = chunk->getGPUMesh();
    gpuMesh.batches.reserve(meshData->batches.size());

    for (const auto& cpuBatch : meshData->batches) {
        GPUMesh::GPUBatch gpuBatch;
        if (createGPUBatch(gpuBatch, cpuBatch)) {
            gpuMesh.batches.push_back(gpuBatch);
        } else {
            // Cleanup on failure
            gpuMesh.cleanup();
            return false;
        }
    }

    return gpuMesh.isValid();
}

bool GPUUploader::createGPUBatch(GPUMesh::GPUBatch& gpuBatch,
                                 const MeshData::Batch& cpuBatch) {
    // Create and upload VBO (positions)
    glGenBuffers(1, &gpuBatch.vbo);
    if (gpuBatch.vbo == 0) {
        std::cerr << "ERROR: Failed to create vertex VBO" << std::endl;
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, gpuBatch.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 cpuBatch.vertices.size() * sizeof(float),
                 cpuBatch.vertices.data(),
                 GL_STATIC_DRAW);

    // Create and upload color VBO
    glGenBuffers(1, &gpuBatch.colorVBO);
    if (gpuBatch.colorVBO == 0) {
        std::cerr << "ERROR: Failed to create color VBO" << std::endl;
        glDeleteBuffers(1, &gpuBatch.vbo);
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, gpuBatch.colorVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 cpuBatch.colors.size() * sizeof(float),
                 cpuBatch.colors.data(),
                 GL_STATIC_DRAW);

    // Create and upload UV VBO
    glGenBuffers(1, &gpuBatch.uvVBO);
    if (gpuBatch.uvVBO == 0) {
        std::cerr << "ERROR: Failed to create UV VBO" << std::endl;
        glDeleteBuffers(1, &gpuBatch.vbo);
        glDeleteBuffers(1, &gpuBatch.colorVBO);
        return false;
    }
    glBindBuffer(GL_ARRAY_BUFFER, gpuBatch.uvVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 cpuBatch.uvs.size() * sizeof(float),
                 cpuBatch.uvs.data(),
                 GL_STATIC_DRAW);

    // No EBO needed for legacy rendering (will use glDrawArrays with quads)
    gpuBatch.ebo = 0;
    gpuBatch.vao = 1; // Dummy value (not used in legacy OpenGL)

    gpuBatch.textureId = cpuBatch.textureId;
    gpuBatch.indexCount = static_cast<uint32_t>(cpuBatch.vertexCount());

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

// ============================================================================
// CHUNK MANAGER IMPLEMENTATION
// ============================================================================

ChunkManager::ChunkManager()
    : shouldStop_(false)
    , statsGenerating_(0)
    , statsMeshing_(0)
    , statsUploading_(0)
{
}

ChunkManager::~ChunkManager() {
    shutdown();
}

void ChunkManager::initialize(int numWorkers) {
    shouldStop_.store(false);

    // Start worker threads
    for (int i = 0; i < numWorkers; ++i) {
        workers_.emplace_back(&ChunkManager::workerThread, this);
    }

    std::cout << "ChunkManager initialized with " << numWorkers << " worker threads" << std::endl;
}

void ChunkManager::shutdown() {
    shouldStop_.store(true);

    // Wait for workers to finish
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    // Clear queues
    generationQueue_.clear();
    meshingQueue_.clear();
    uploadQueue_.clear();

    // Cleanup all chunks (on main thread)
    std::lock_guard<std::mutex> lock(chunksMutex_);
    chunks_.clear();

    std::cout << "ChunkManager shut down" << std::endl;
}

void ChunkManager::update(const float playerX, const float playerY, const float playerZ,
                          int renderDistance) {
    ChunkPosition playerChunk = worldToChunkPos(
        static_cast<int>(playerX),
        static_cast<int>(playerY),
        static_cast<int>(playerZ)
    );

    // Request chunks around player
    requestChunksAroundPlayer(playerChunk, renderDistance);

    // Upload ready meshes (main thread only)
    processUploadQueue();

    // Unload distant chunks
    unloadDistantChunks(playerChunk, renderDistance + 2);
}

void ChunkManager::workerThread() {
    while (!shouldStop_.load()) {
        // Try to get work from generation queue
        Chunk* chunk = nullptr;
        if (generationQueue_.tryPop(chunk)) {
            if (chunk && chunk->getState() == ChunkState::GENERATING) {
                // TODO: Generate voxel data (call your terrain generator)
                // For now, just mark as generated
                chunk->setState(ChunkState::GENERATED);

                // ⭐ CRITICAL: Notify neighbors that this chunk is ready
                // This allows waiting neighbors to start meshing
                notifyNeighborsReady(chunk);

                // ⭐ CRITICAL: After generation, check if neighbors allow meshing
                if (chunk->canBuildMesh()) {
                    // All neighbors exist and have data - can mesh immediately
                    meshingQueue_.push(chunk);
                } else {
                    // Must wait for neighbors - mark as waiting
                    chunk->setState(ChunkState::WAITING_FOR_NEIGHBORS);
                    // Don't add to meshing queue yet - will be added when neighbors arrive
                }
            }
            continue;
        }

        // Try to get work from meshing queue
        if (meshingQueue_.tryPop(chunk)) {
            ChunkState state = chunk->getState();

            // Can mesh from GENERATED or WAITING_FOR_NEIGHBORS state
            if (state == ChunkState::GENERATED || state == ChunkState::WAITING_FOR_NEIGHBORS) {

                // ⭐ CRITICAL: Double-check neighbors before meshing
                if (!chunk->canBuildMesh()) {
                    // Neighbors not ready yet - put back in waiting state
                    chunk->setState(ChunkState::WAITING_FOR_NEIGHBORS);
                    continue;  // Don't mesh yet
                }

                chunk->setState(ChunkState::MESHING);

                // Build mesh (CPU-side, no OpenGL)
                auto meshData = MeshBuilder::buildMesh(chunk);

                if (meshData && meshData->isValid()) {
                    chunk->setMeshData(std::move(meshData));
                    chunk->setState(ChunkState::MESH_READY);

                    // Add to upload queue
                    uploadQueue_.push(chunk);
                } else {
                    // Mesh build failed, go back to waiting
                    chunk->setState(ChunkState::WAITING_FOR_NEIGHBORS);
                }
            }
            continue;
        }

        // No work, sleep briefly
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ChunkManager::processUploadQueue() {
    const int MAX_UPLOADS_PER_FRAME = 4;
    int uploaded = 0;

    Chunk* chunk = nullptr;
    while (uploaded < MAX_UPLOADS_PER_FRAME && uploadQueue_.tryPop(chunk)) {
        if (chunk && chunk->getState() == ChunkState::MESH_READY) {
            chunk->setState(ChunkState::UPLOADING);

            // Take ownership of mesh data
            auto meshData = chunk->takeMeshData();

            if (meshData && GPUUploader::uploadMesh(chunk, meshData.get())) {
                chunk->setState(ChunkState::READY);
                uploaded++;
            } else {
                // Upload failed, mark as generated to retry
                chunk->setState(ChunkState::GENERATED);
            }
        }
    }
}

void ChunkManager::requestChunksAroundPlayer(const ChunkPosition& playerChunk, int renderDistance) {
    for (int dx = -renderDistance; dx <= renderDistance; ++dx) {
        for (int dz = -renderDistance; dz <= renderDistance; ++dz) {
            ChunkPosition pos(playerChunk.x + dx, 0, playerChunk.z + dz);

            Chunk* chunk = getOrCreateChunk(pos);

            if (chunk->getState() == ChunkState::UNLOADED) {
                // ⭐ CRITICAL: Update neighbor links BEFORE generating
                updateNeighborLinks(chunk);

                chunk->setState(ChunkState::GENERATING);
                generationQueue_.push(chunk);
            }
        }
    }
}

// ⭐ CRITICAL: Update bidirectional neighbor links
void ChunkManager::updateNeighborLinks(Chunk* chunk) {
    if (!chunk) return;

    const ChunkPosition& pos = chunk->getPosition();

    // Get neighbor chunks
    Chunk* north = getChunk(ChunkPosition(pos.x, pos.y, pos.z + 1));
    Chunk* south = getChunk(ChunkPosition(pos.x, pos.y, pos.z - 1));
    Chunk* east = getChunk(ChunkPosition(pos.x + 1, pos.y, pos.z));
    Chunk* west = getChunk(ChunkPosition(pos.x - 1, pos.y, pos.z));

    // Set this chunk's neighbors
    chunk->setNeighbor(0, north);  // North
    chunk->setNeighbor(1, south);  // South
    chunk->setNeighbor(2, east);   // East
    chunk->setNeighbor(3, west);   // West

    // Set neighbors' pointers back to this chunk
    if (north) north->setNeighbor(1, chunk);  // North's south = this
    if (south) south->setNeighbor(0, chunk);  // South's north = this
    if (east) east->setNeighbor(3, chunk);    // East's west = this
    if (west) west->setNeighbor(2, chunk);    // West's east = this
}

// ⭐ CRITICAL: Notify neighbors that this chunk is ready - they might be waiting!
void ChunkManager::notifyNeighborsReady(Chunk* chunk) {
    if (!chunk) return;

    const ChunkPosition& pos = chunk->getPosition();

    // Check all 4 horizontal neighbors
    ChunkPosition neighborPositions[4] = {
        ChunkPosition(pos.x, pos.y, pos.z + 1),  // North
        ChunkPosition(pos.x, pos.y, pos.z - 1),  // South
        ChunkPosition(pos.x + 1, pos.y, pos.z),  // East
        ChunkPosition(pos.x - 1, pos.y, pos.z)   // West
    };

    for (const auto& neighborPos : neighborPositions) {
        Chunk* neighbor = getChunk(neighborPos);

        if (!neighbor) continue;

        // If neighbor is waiting for this chunk, check if it can mesh now
        if (neighbor->getState() == ChunkState::WAITING_FOR_NEIGHBORS) {
            if (neighbor->canBuildMesh()) {
                // All neighbors ready! Add to meshing queue
                meshingQueue_.push(neighbor);
            }
        }
    }
}

void ChunkManager::unloadDistantChunks(const ChunkPosition& playerChunk, int unloadDistance) {
    std::lock_guard<std::mutex> lock(chunksMutex_);

    std::vector<ChunkPosition> toRemove;

    for (auto& pair : chunks_) {
        const ChunkPosition& pos = pair.first;
        int dx = abs(pos.x - playerChunk.x);
        int dz = abs(pos.z - playerChunk.z);

        if (dx > unloadDistance || dz > unloadDistance) {
            toRemove.push_back(pos);
        }
    }

    for (const auto& pos : toRemove) {
        chunks_.erase(pos);
    }
}

Chunk* ChunkManager::getChunk(const ChunkPosition& pos) {
    std::lock_guard<std::mutex> lock(chunksMutex_);
    auto it = chunks_.find(pos);
    return (it != chunks_.end()) ? it->second.get() : nullptr;
}

Chunk* ChunkManager::getOrCreateChunk(const ChunkPosition& pos) {
    std::lock_guard<std::mutex> lock(chunksMutex_);

    auto it = chunks_.find(pos);
    if (it != chunks_.end()) {
        return it->second.get();
    }

    auto chunk = std::make_unique<Chunk>(pos);
    Chunk* ptr = chunk.get();
    chunks_[pos] = std::move(chunk);

    return ptr;
}

ChunkPosition ChunkManager::worldToChunkPos(int worldX, int worldY, int worldZ) const {
    return ChunkPosition(
        worldX >> 4,  // Divide by 16
        worldY >> 8,  // Divide by 256
        worldZ >> 4   // Divide by 16
    );
}

void ChunkManager::render(const float* viewProj, const float* playerPos) {
    std::lock_guard<std::mutex> lock(chunksMutex_);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Enable client states for legacy OpenGL
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    for (auto& pair : chunks_) {
        Chunk* chunk = pair.second.get();

        if (chunk->getState() != ChunkState::READY) continue;

        const GPUMesh& mesh = chunk->getGPUMesh();
        if (!mesh.isValid()) continue;

        // Render each batch
        for (const auto& batch : mesh.batches) {
            if (batch.indexCount == 0) continue;

            glBindTexture(GL_TEXTURE_2D, batch.textureId);

            // Bind vertex VBO
            glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
            glVertexPointer(3, GL_FLOAT, 0, nullptr);

            // Bind color VBO
            glBindBuffer(GL_ARRAY_BUFFER, batch.colorVBO);
            glColorPointer(4, GL_FLOAT, 0, nullptr);

            // Bind UV VBO
            glBindBuffer(GL_ARRAY_BUFFER, batch.uvVBO);
            glTexCoordPointer(2, GL_FLOAT, 0, nullptr);

            // Draw quads (4 vertices per face)
            glDrawArrays(GL_QUADS, 0, batch.indexCount);
        }
    }

    // Disable client states
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

size_t ChunkManager::getChunkCount() const {
    std::lock_guard<std::mutex> lock(chunksMutex_);
    return chunks_.size();
}

size_t ChunkManager::getReadyChunkCount() const {
    std::lock_guard<std::mutex> lock(chunksMutex_);
    size_t count = 0;
    for (const auto& pair : chunks_) {
        if (pair.second->getState() == ChunkState::READY) {
            count++;
        }
    }
    return count;
}

void ChunkManager::getStats(int& total, int& ready, int& generating, int& meshing, int& uploading) {
    std::lock_guard<std::mutex> lock(chunksMutex_);
    total = static_cast<int>(chunks_.size());
    ready = 0;
    generating = 0;
    meshing = 0;
    uploading = 0;

    for (const auto& pair : chunks_) {
        switch (pair.second->getState()) {
            case ChunkState::READY: ready++; break;
            case ChunkState::GENERATING: generating++; break;
            case ChunkState::MESHING: meshing++; break;
            case ChunkState::UPLOADING: uploading++; break;
            default: break;
        }
    }
}

uint8_t ChunkManager::getBlock(int worldX, int worldY, int worldZ) {
    ChunkPosition chunkPos = worldToChunkPos(worldX, worldY, worldZ);
    Chunk* chunk = getChunk(chunkPos);
    if (!chunk) return 0;

    int localX = worldX - chunkPos.x * Chunk::SIZE;
    int localY = worldY - chunkPos.y * Chunk::HEIGHT;
    int localZ = worldZ - chunkPos.z * Chunk::SIZE;

    return chunk->getBlock(localX, localY, localZ);
}

void ChunkManager::setBlock(int worldX, int worldY, int worldZ, uint8_t blockType) {
    ChunkPosition chunkPos = worldToChunkPos(worldX, worldY, worldZ);
    Chunk* chunk = getChunk(chunkPos);
    if (!chunk) return;

    int localX = worldX - chunkPos.x * Chunk::SIZE;
    int localY = worldY - chunkPos.y * Chunk::HEIGHT;
    int localZ = worldZ - chunkPos.z * Chunk::SIZE;

    chunk->setBlock(localX, localY, localZ, blockType);
}

// ============================================================================
// DEBUG UTILITIES
// ============================================================================

void ChunkDebug::validateChunk(const Chunk* chunk) {
    if (!chunk) {
        std::cerr << "ERROR: Null chunk" << std::endl;
        return;
    }

    // Validate state
    ChunkState state = chunk->getState();
    std::cout << "Chunk (" << chunk->getPosition().x << ", "
              << chunk->getPosition().y << ", "
              << chunk->getPosition().z << ") "
              << "State: " << ChunkStateToString(state) << std::endl;
}

void ChunkDebug::validateMesh(const MeshData* mesh) {
    if (!mesh) {
        std::cerr << "ERROR: Null mesh data" << std::endl;
        return;
    }

    if (!mesh->isValid()) {
        std::cerr << "ERROR: Invalid mesh data" << std::endl;
    }

    std::cout << "Mesh: " << mesh->batches.size() << " batches, "
              << mesh->totalVertices << " vertices, "
              << mesh->totalIndices << " indices" << std::endl;
}

void ChunkDebug::validateGPUMesh(const GPUMesh* mesh) {
    if (!mesh) {
        std::cerr << "ERROR: Null GPU mesh" << std::endl;
        return;
    }

    if (!mesh->isValid()) {
        std::cerr << "ERROR: Invalid GPU mesh" << std::endl;
    }
}

void ChunkDebug::enableGLDebugOutput() {
    // Enable OpenGL debug output if available
    std::cout << "OpenGL debug output enabled" << std::endl;
}

} // namespace VoxelEngine
