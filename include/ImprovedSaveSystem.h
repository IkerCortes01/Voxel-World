#pragma once

#include "SaveSystem.h"
#include <memory>
#include <unordered_map>

// ============================================================================
// IMPROVED SAVE SYSTEM - Optimizaciones sobre SaveSystem.h
// ============================================================================
// Mejoras adicionales:
// - Batch saving (múltiples chunks en una operación)
// - Smart delta compression (solo bloques modificados)
// - Memory-mapped I/O para regiones activas
// - Write-ahead logging (WAL) para crash recovery
// - Incremental saves (solo chunks dirty)
// - Priority queue (chunks críticos primero)
// ============================================================================

namespace VoxelWorld {
namespace SaveSystem {

// ============================================================================
// BATCH SAVE OPTIMIZER
// ============================================================================
// Agrupa múltiples chunks en una sola escritura para reducir syscalls
// ============================================================================

class BatchSaveOptimizer {
public:
    struct BatchConfig {
        size_t maxBatchSize;           // Máximo de chunks por batch
        size_t maxBatchSizeBytes;      // Máximo de bytes por batch
        float maxBatchDelaySeconds;    // Máximo tiempo de espera
    };

private:
    struct PendingChunk {
        int chunkX, chunkZ;
        std::vector<uint8_t> data;
        ChunkMetadata metadata;
        uint64_t priority;
        std::chrono::steady_clock::time_point queueTime;
    };

    BatchConfig config_;
    std::vector<PendingChunk> pendingChunks_;
    std::mutex batchMutex_;
    size_t currentBatchBytes_;
    std::chrono::steady_clock::time_point lastFlushTime_;

public:
    BatchSaveOptimizer(const BatchConfig& config = {});

    // Agregar chunk al batch
    void addChunk(int chunkX, int chunkZ, std::vector<uint8_t>&& data,
                  const ChunkMetadata& metadata, uint64_t priority = 0);

    // Flush batch si está listo
    bool shouldFlush() const;

    // Obtener batch actual
    std::vector<PendingChunk> flushBatch();

    // Force flush
    std::vector<PendingChunk> forceFlush();

    // Stats
    size_t getPendingCount() const;
    size_t getPendingBytes() const;
};

// ============================================================================
// SMART DELTA COMPRESSOR
// ============================================================================
// Comprime solo los bloques que cambiaron desde la última save
// ============================================================================

class SmartDeltaCompressor {
private:
    struct ChunkSnapshot {
        std::vector<uint8_t> lastSavedState;
        uint64_t lastSaveTime;
        uint32_t version;
    };

    std::unordered_map<uint64_t, ChunkSnapshot> snapshots_;
    std::mutex snapshotsMutex_;

    uint64_t packCoords(int x, int z) const {
        return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z;
    }

public:
    struct DeltaResult {
        std::vector<BlockDelta> changes;
        bool isFullSave;  // true si no hay snapshot previo
        size_t originalSize;
        size_t compressedSize;
        float compressionRatio;
    };

    // Calcular delta vs última versión guardada
    DeltaResult calculateDelta(
        int chunkX, int chunkZ,
        const uint8_t* currentData,
        size_t dataSize
    );

    // Actualizar snapshot después de save exitoso
    void updateSnapshot(
        int chunkX, int chunkZ,
        const uint8_t* data,
        size_t dataSize
    );

    // Clear snapshot (para liberar memoria)
    void clearSnapshot(int chunkX, int chunkZ);

    // Clear old snapshots
    void clearOldSnapshots(uint64_t olderThanSeconds);

    // Stats
    size_t getSnapshotCount() const;
    size_t getTotalSnapshotMemory() const;
};

// ============================================================================
// MEMORY-MAPPED REGION CACHE
// ============================================================================
// Mapea regiones activas en memoria para I/O ultrarrápido
// ============================================================================

class MemoryMappedRegionCache {
private:
    struct MappedRegion {
        void* mappedData;
        size_t mappedSize;
        std::string filePath;
        std::chrono::steady_clock::time_point lastAccess;
        bool isDirty;
        std::mutex regionMutex;

#ifdef _WIN32
        void* fileHandle;
        void* mapHandle;
#else
        int fileDescriptor;
#endif
    };

    std::unordered_map<uint64_t, std::unique_ptr<MappedRegion>> mappedRegions_;
    std::mutex cacheMutex_;
    size_t maxCachedRegions_;
    size_t maxMemoryUsage_;
    size_t currentMemoryUsage_;

    uint64_t packRegionCoords(int regionX, int regionZ) const {
        return ((uint64_t)(uint32_t)regionX << 32) | (uint32_t)regionZ;
    }

    void evictLRU();
    void unmapRegion(MappedRegion* region);

public:
    MemoryMappedRegionCache(size_t maxRegions = 16, size_t maxMemoryMB = 256);
    ~MemoryMappedRegionCache();

    // Map region to memory
    bool mapRegion(int regionX, int regionZ, const std::string& filePath);

    // Unmap region
    void unmapRegion(int regionX, int regionZ);

    // Write chunk to mapped region
    bool writeChunk(int regionX, int regionZ, int localX, int localZ,
                   const uint8_t* data, size_t size);

    // Read chunk from mapped region
    bool readChunk(int regionX, int regionZ, int localX, int localZ,
                  uint8_t* dataOut, size_t maxSize, size_t& sizeOut);

    // Flush dirty regions
    void flushAll();

    // Stats
    struct Stats {
        size_t regionsMapped;
        size_t memoryUsedMB;
        size_t dirtyRegions;
        size_t totalReads;
        size_t totalWrites;
    };
    Stats getStats() const;
};

// ============================================================================
// WRITE-AHEAD LOG (WAL)
// ============================================================================
// Journal de operaciones para crash recovery
// ============================================================================

class WriteAheadLog {
public:
    enum class OperationType : uint8_t {
        CHUNK_WRITE,
        CHUNK_DELETE,
        REGION_CREATE,
        CHECKPOINT
    };

    struct LogEntry {
        OperationType operation;
        int chunkX, chunkZ;
        uint64_t timestamp;
        std::vector<uint8_t> data;
        uint32_t checksum;
    };

private:
    std::string walPath_;
    std::ofstream walFile_;
    std::mutex walMutex_;
    uint64_t sequenceNumber_;
    size_t entriesSinceCheckpoint_;
    size_t checkpointInterval_;

public:
    WriteAheadLog(const std::string& walPath, size_t checkpointInterval = 100);
    ~WriteAheadLog();

    // Log operation
    void logOperation(OperationType op, int chunkX, int chunkZ,
                     const uint8_t* data = nullptr, size_t dataSize = 0);

    // Checkpoint (flush WAL, create recovery point)
    void checkpoint();

    // Replay WAL (for crash recovery)
    std::vector<LogEntry> replayLog();

    // Clear WAL
    void clear();

    // Stats
    size_t getEntryCount() const;
    size_t getWALSize() const;
};

// ============================================================================
// INCREMENTAL SAVE MANAGER
// ============================================================================
// Gestiona saves incrementales (solo chunks dirty)
// ============================================================================

class IncrementalSaveManager {
private:
    WorldSaveManager* baseSaveManager_;
    SmartDeltaCompressor deltaCompressor_;
    BatchSaveOptimizer batchOptimizer_;
    WriteAheadLog wal_;

    std::unordered_set<uint64_t> dirtyChunks_;
    std::unordered_map<uint64_t, uint64_t> chunkPriorities_;
    std::mutex dirtyMutex_;

    uint64_t packCoords(int x, int z) const {
        return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z;
    }

    struct SaveStats {
        std::atomic<uint64_t> incrementalSaves;
        std::atomic<uint64_t> fullSaves;
        std::atomic<uint64_t> bytesSavedByDelta;
        std::atomic<float> averageCompressionRatio;
    };
    SaveStats stats_;

public:
    IncrementalSaveManager(WorldSaveManager* baseManager, const std::string& walPath);
    ~IncrementalSaveManager();

    // Mark chunk as dirty
    void markDirty(int chunkX, int chunkZ, uint64_t priority = 0);

    // Save dirty chunks incrementally
    size_t saveDirtyChunks(size_t maxChunks = 10);

    // Save all dirty chunks
    size_t saveAllDirty();

    // Update (call every frame)
    void update();

    // Force checkpoint
    void checkpoint();

    // Recover from crash
    bool recoverFromCrash();

    // Stats
    struct Stats {
        uint64_t dirtyChunks;
        uint64_t incrementalSaves;
        uint64_t fullSaves;
        uint64_t bytesSaved;
        float compressionRatio;
        size_t walSize;
        size_t batchPending;
    };
    Stats getStats() const;
};

// ============================================================================
// PRIORITY SAVE QUEUE
// ============================================================================
// Cola de prioridad para saves críticos
// ============================================================================

class PrioritySaveQueue {
public:
    enum class Priority : uint8_t {
        CRITICAL = 0,   // Player position, player inventory
        HIGH = 1,       // Chunks visibles, modificados recientemente
        NORMAL = 2,     // Chunks cargados
        LOW = 3,        // Chunks lejanos
        BACKGROUND = 4  // Chunks viejos, raros
    };

    struct PrioritySaveTask {
        int chunkX, chunkZ;
        std::vector<uint8_t> data;
        ChunkMetadata metadata;
        Priority priority;
        uint64_t timestamp;

        bool operator<(const PrioritySaveTask& other) const {
            // Menor priority value = mayor prioridad
            if (priority != other.priority) {
                return priority > other.priority;  // Invertido para min-heap
            }
            return timestamp > other.timestamp;  // Más viejo = más prioridad
        }
    };

private:
    std::priority_queue<PrioritySaveTask> queue_;
    std::mutex queueMutex_;
    std::condition_variable queueCV_;

public:
    // Push con prioridad
    void push(int chunkX, int chunkZ, std::vector<uint8_t>&& data,
             const ChunkMetadata& metadata, Priority priority = Priority::NORMAL);

    // Pop tarea de mayor prioridad
    bool pop(PrioritySaveTask& taskOut, int timeoutMs = 100);

    // Size
    size_t size() const;

    // Clear
    void clear();
};

// ============================================================================
// OPTIMIZED WORLD SAVE MANAGER
// ============================================================================
// Wrapper que combina todas las optimizaciones
// ============================================================================

class OptimizedWorldSaveManager {
private:
    std::unique_ptr<WorldSaveManager> baseSaveManager_;
    std::unique_ptr<IncrementalSaveManager> incrementalManager_;
    std::unique_ptr<MemoryMappedRegionCache> mmapCache_;
    std::unique_ptr<PrioritySaveQueue> priorityQueue_;

    std::vector<std::thread> saveThreads_;
    std::atomic<bool> savingActive_;

    void saveThreadWorker();

public:
    OptimizedWorldSaveManager(const std::string& worldPath,
                             const std::string& worldName,
                             uint64_t seed);
    ~OptimizedWorldSaveManager();

    // Initialize
    bool initialize(int numThreads = 4);

    // Shutdown
    void shutdown();

    // Save operations (prioritized)
    void saveChunk(int chunkX, int chunkZ,
                  const void* blockData, size_t blockDataSize,
                  const ChunkMetadata& metadata,
                  PrioritySaveQueue::Priority priority = PrioritySaveQueue::Priority::NORMAL);

    // Load operations (with mmap cache)
    bool loadChunk(int chunkX, int chunkZ,
                  void* blockDataOut, size_t blockDataSize,
                  ChunkMetadata& metadataOut);

    // Mark dirty
    void markChunkDirty(int chunkX, int chunkZ,
                       PrioritySaveQueue::Priority priority = PrioritySaveQueue::Priority::NORMAL);

    // Update (call every frame)
    void update();

    // Save all dirty
    void saveAllDirty();

    // Flush everything
    void flushAll();

    // Checkpoint
    void checkpoint();

    // Stats
    struct Stats {
        IncrementalSaveManager::Stats incremental;
        MemoryMappedRegionCache::Stats mmap;
        size_t priorityQueueSize;
        size_t activeThreads;
    };
    Stats getStats() const;
};

} // namespace SaveSystem
} // namespace VoxelWorld
