#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <queue>
#include <unordered_map>
#include <thread>
#include <condition_variable>
#include <GL/gl.h>

// ============================================================================
// PROFESSIONAL VOXEL ENGINE ARCHITECTURE
// ============================================================================
// Thread-safe, corruption-free, high-performance chunk system
// Designed for infinite worlds with dynamic streaming
// ============================================================================

namespace VoxelEngine {

// Forward declarations
struct ChunkPosition;
class Chunk;
class ChunkMesh;
class ChunkManager;
class MeshBuilder;

// ============================================================================
// CHUNK STATE MACHINE
// ============================================================================

enum class ChunkState : uint8_t {
    UNLOADED = 0,           // Not in memory
    GENERATING,             // Worker thread generating voxel data
    GENERATED,              // Voxel data ready, needs mesh
    WAITING_FOR_NEIGHBORS,  // ⭐ CRITICAL: Waiting for neighbor chunks before meshing
    MESHING,                // Worker thread building mesh
    MESH_READY,             // Mesh built, needs GPU upload (CPU-side only)
    UPLOADING,              // Main thread uploading to GPU
    READY,                  // Fully ready for rendering
    DIRTY,                  // Needs mesh rebuild
    UNLOADING               // Being removed
};

const char* ChunkStateToString(ChunkState state);

// ============================================================================
// CHUNK POSITION (immutable, hashable)
// ============================================================================

struct ChunkPosition {
    int32_t x, y, z;

    ChunkPosition(int32_t x = 0, int32_t y = 0, int32_t z = 0) : x(x), y(y), z(z) {}

    bool operator==(const ChunkPosition& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const ChunkPosition& other) const {
        return !(*this == other);
    }

    struct Hash {
        size_t operator()(const ChunkPosition& pos) const {
            // FNV-1a hash
            size_t hash = 2166136261u;
            hash = (hash ^ static_cast<size_t>(pos.x)) * 16777619u;
            hash = (hash ^ static_cast<size_t>(pos.y)) * 16777619u;
            hash = (hash ^ static_cast<size_t>(pos.z)) * 16777619u;
            return hash;
        }
    };
};

// ============================================================================
// MESH DATA (CPU-side, built by worker threads)
// ============================================================================

struct MeshData {
    // Per-texture batches
    struct Batch {
        GLuint textureId;
        std::vector<float> vertices;    // xyz
        std::vector<float> colors;      // rgba
        std::vector<float> uvs;         // uv
        std::vector<uint32_t> indices;  // uint32_t for large meshes

        Batch() : textureId(0) {}

        bool isValid() const {
            return !vertices.empty() &&
                   vertices.size() / 3 == colors.size() / 4 &&
                   vertices.size() / 3 == uvs.size() / 2 &&
                   !indices.empty();
        }

        size_t vertexCount() const { return vertices.size() / 3; }
        size_t indexCount() const { return indices.size(); }
    };

    std::vector<Batch> batches;
    size_t totalVertices = 0;
    size_t totalIndices = 0;

    bool isEmpty() const { return batches.empty(); }

    bool isValid() const {
        for (const auto& batch : batches) {
            if (!batch.isValid()) return false;
            // Validate indices
            size_t vertCount = batch.vertexCount();
            for (uint32_t idx : batch.indices) {
                if (idx >= vertCount) return false;
            }
        }
        return true;
    }

    void clear() {
        batches.clear();
        totalVertices = 0;
        totalIndices = 0;
    }
};

// ============================================================================
// GPU MESH (main thread only, contains GPU resources)
// ============================================================================

struct GPUMesh {
    struct GPUBatch {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint colorVBO = 0;
        GLuint uvVBO = 0;
        GLuint ebo = 0;
        GLuint textureId = 0;
        uint32_t indexCount = 0;

        bool isValid() const {
            return vbo != 0 && colorVBO != 0 && uvVBO != 0 && indexCount > 0;
        }

        ~GPUBatch() {
            // Destructor doesn't delete - must call cleanup() explicitly on main thread
        }
    };

    std::vector<GPUBatch> batches;

    bool isEmpty() const { return batches.empty(); }

    bool isValid() const {
        for (const auto& batch : batches) {
            if (!batch.isValid()) return false;
        }
        return !batches.empty();
    }

    // MUST be called on main thread
    void cleanup();
};

// ============================================================================
// CHUNK (thread-safe state machine)
// ============================================================================

class Chunk {
public:
    static constexpr int SIZE = 16;
    static constexpr int HEIGHT = 256;

    explicit Chunk(const ChunkPosition& pos);
    ~Chunk();

    // State management (thread-safe)
    ChunkState getState() const { return state_.load(std::memory_order_acquire); }
    void setState(ChunkState newState) { state_.store(newState, std::memory_order_release); }
    bool compareAndSetState(ChunkState expected, ChunkState desired) {
        return state_.compare_exchange_strong(expected, desired);
    }

    // Position (immutable)
    const ChunkPosition& getPosition() const { return position_; }

    // Voxel data access (thread-safe with mutex)
    uint8_t getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, uint8_t blockType);
    uint8_t getLightLevel(int x, int y, int z) const;
    void setLightLevel(int x, int y, int z, uint8_t light);

    // Neighbor access (for mesh building)
    void setNeighbor(int direction, Chunk* neighbor);
    Chunk* getNeighbor(int direction) const;
    bool hasAllNeighbors() const;

    // ⭐ CRITICAL: Check if chunk can build mesh (all neighbors must exist and be ready)
    bool canBuildMesh() const;

    // Mesh data (CPU-side, set by worker thread)
    void setMeshData(std::unique_ptr<MeshData> mesh);
    std::unique_ptr<MeshData> takeMeshData(); // Transfers ownership

    // GPU mesh (main thread only)
    GPUMesh& getGPUMesh() { return gpuMesh_; }
    const GPUMesh& getGPUMesh() const { return gpuMesh_; }

    // Dirty flag (atomic)
    bool isDirty() const { return dirty_.load(std::memory_order_acquire); }
    void markDirty() { dirty_.store(true, std::memory_order_release); }
    void clearDirty() { dirty_.store(false, std::memory_order_release); }

    // Generation/meshing timestamps (for debugging)
    uint64_t getGenerationTime() const { return generationTime_; }
    void setGenerationTime(uint64_t time) { generationTime_ = time; }

private:
    ChunkPosition position_;
    std::atomic<ChunkState> state_;
    std::atomic<bool> dirty_;

    // Voxel data (protected by mutex)
    mutable std::mutex dataMutex_;
    std::vector<uint8_t> blocks_;     // SIZE * HEIGHT * SIZE
    std::vector<uint8_t> lightLevels_; // SIZE * HEIGHT * SIZE

    // Neighbors (0=North, 1=South, 2=East, 3=West, 4=Up, 5=Down)
    std::atomic<Chunk*> neighbors_[6];

    // Mesh data (protected by mutex)
    mutable std::mutex meshMutex_;
    std::unique_ptr<MeshData> cpuMesh_;

    // GPU mesh (main thread only, no mutex needed)
    GPUMesh gpuMesh_;

    uint64_t generationTime_ = 0;

    inline int getIndex(int x, int y, int z) const {
        return x + z * SIZE + y * SIZE * SIZE;
    }
};

// ============================================================================
// THREAD-SAFE QUEUE
// ============================================================================

template<typename T>
class ThreadSafeQueue {
public:
    void push(T item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(item));
        cv_.notify_one();
    }

    bool tryPop(T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) return false;
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    bool waitPop(T& item, int timeoutMs = 100) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                          [this] { return !queue_.empty(); })) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

// ============================================================================
// MESH BUILDER (worker thread, no OpenGL)
// ============================================================================

class MeshBuilder {
public:
    // Texture callback function type: (blockType, faceDirection) -> textureId
    using TextureCallback = GLuint(*)(uint8_t, int);

    // Set the texture callback (call this once during initialization)
    static void setTextureCallback(TextureCallback callback);

    // Build mesh for a chunk (runs on worker thread)
    static std::unique_ptr<MeshData> buildMesh(Chunk* chunk);

private:
    // Texture callback
    static TextureCallback textureCallback_;

    // Helper: get block from chunk or neighbor
    static uint8_t getBlockSafe(Chunk* chunk, int x, int y, int z);

    // Helper: check if face should be rendered
    static bool shouldRenderFace(uint8_t currentBlock, uint8_t neighborBlock);

    // Helper: add face to mesh
    static void addFace(MeshData::Batch& batch,
                       int direction,
                       float x, float y, float z,
                       uint8_t blockType,
                       float r, float g, float b, float a);
};

// ============================================================================
// CHUNK MANAGER (orchestrates everything)
// ============================================================================

class ChunkManager {
public:
    ChunkManager();
    ~ChunkManager();

    // Initialize/shutdown (main thread)
    void initialize(int numWorkers = 4);
    void shutdown();

    // Update (main thread, call every frame)
    void update(const float playerX, const float playerY, const float playerZ,
                int renderDistance);

    // Render (main thread only)
    void render(const float* viewProj, const float* playerPos);

    // Block access (thread-safe)
    uint8_t getBlock(int worldX, int worldY, int worldZ);
    void setBlock(int worldX, int worldY, int worldZ, uint8_t blockType);

    // Stats
    size_t getChunkCount() const;
    size_t getReadyChunkCount() const;
    void getStats(int& total, int& ready, int& generating, int& meshing, int& uploading);

private:
    // Worker thread function
    void workerThread();

    // Main thread functions
    void processUploadQueue();
    void requestChunksAroundPlayer(const ChunkPosition& playerChunk, int renderDistance);
    void unloadDistantChunks(const ChunkPosition& playerChunk, int unloadDistance);

    // ⭐ CRITICAL: Neighbor synchronization
    void updateNeighborLinks(Chunk* chunk);
    void notifyNeighborsReady(Chunk* chunk);

    // Chunk retrieval
    Chunk* getChunk(const ChunkPosition& pos);
    Chunk* getOrCreateChunk(const ChunkPosition& pos);

    // Coordinate conversion
    ChunkPosition worldToChunkPos(int worldX, int worldY, int worldZ) const;

    // Chunk storage (protected by mutex)
    mutable std::mutex chunksMutex_;
    std::unordered_map<ChunkPosition, std::unique_ptr<Chunk>, ChunkPosition::Hash> chunks_;

    // Work queues
    ThreadSafeQueue<Chunk*> generationQueue_;
    ThreadSafeQueue<Chunk*> meshingQueue_;
    ThreadSafeQueue<Chunk*> uploadQueue_;

    // Worker threads
    std::vector<std::thread> workers_;
    std::atomic<bool> shouldStop_;

    // Stats
    std::atomic<size_t> statsGenerating_;
    std::atomic<size_t> statsMeshing_;
    std::atomic<size_t> statsUploading_;
};

// ============================================================================
// GPU UPLOADER (main thread only)
// ============================================================================

class GPUUploader {
public:
    // Upload mesh data to GPU (main thread only)
    static bool uploadMesh(Chunk* chunk, MeshData* meshData);

private:
    static bool createGPUBatch(GPUMesh::GPUBatch& gpuBatch,
                              const MeshData::Batch& cpuBatch);
};

// ============================================================================
// DEBUG & VALIDATION
// ============================================================================

class ChunkDebug {
public:
    static void validateChunk(const Chunk* chunk);
    static void validateMesh(const MeshData* mesh);
    static void validateGPUMesh(const GPUMesh* mesh);
    static void enableGLDebugOutput();
};

} // namespace VoxelEngine
