#pragma once

#include <string>
#include <vector>
#include <map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <fstream>
#include <memory>
#include <chrono>
#include <unordered_set>
#include <functional>

// ============================================================================
// VOXELWORLD AAA SAVE SYSTEM
// ============================================================================
// Professional-grade persistence system for infinite voxel worlds
// Features:
// - Region file system (32x32 chunks per region)
// - Delta saving (only modified blocks)
// - Asynchronous multithreaded I/O
// - LZ4 compression
// - Crash-safe journaling
// - Rolling backups
// - World versioning & migration
// - Chunk metadata & UUID system
// ============================================================================

namespace VoxelWorld {
namespace SaveSystem {

// ============================================================================
// CONSTANTS & CONFIGURATION
// ============================================================================

constexpr int SAVE_VERSION = 1;                    // Current save format version
constexpr int REGION_SIZE = 32;                    // 32x32 chunks per region file
constexpr int CHUNK_HEADER_SIZE = 8;               // Chunk header: 4 bytes offset + 4 bytes size
constexpr int SECTOR_SIZE = 4096;                  // 4KB sectors (standard disk block)
constexpr int MAX_CHUNK_SIZE = 1024 * 1024;        // 1MB max chunk size
constexpr int COMPRESSION_LEVEL = 3;               // LZ4 compression level
constexpr int SAVE_THREAD_COUNT = 2;               // Number of async save threads
constexpr int BACKUP_COUNT = 3;                    // Number of rolling backups
constexpr int AUTO_SAVE_INTERVAL = 300;            // Auto-save every 5 minutes

// ============================================================================
// DATA STRUCTURES
// ============================================================================

#pragma pack(push, 1)

// Region file header (4KB)
struct RegionHeader {
    char magic[4];              // "VXRF" - VoxelWorld Region File
    uint32_t version;           // Save format version
    uint32_t timestamp;         // Last modification time
    uint32_t chunkCount;        // Number of saved chunks in this region
    uint32_t flags;             // Feature flags
    char worldName[256];        // World name
    uint64_t worldSeed;         // World generation seed
    uint8_t reserved[3816];     // Reserved for future use
};

// Chunk save header
struct ChunkSaveHeader {
    char magic[4];              // "CHNK"
    uint32_t version;           // Chunk format version
    uint32_t compressedSize;    // Compressed data size
    uint32_t uncompressedSize;  // Original data size
    uint32_t crc32;             // CRC32 checksum
    uint32_t flags;             // Chunk flags (compressed, modified, etc)
    int32_t chunkX;             // Chunk X position
    int32_t chunkZ;             // Chunk Z position
    uint64_t timestamp;         // Last modification time
    uint32_t blockCount;        // Number of modified blocks (for delta saves)
    uint8_t reserved[32];       // Reserved
};

// Block delta entry (for sparse saving)
struct BlockDelta {
    uint16_t x : 4;             // Local X (0-15)
    uint16_t y : 8;             // Local Y (0-255)
    uint16_t z : 4;             // Local Z (0-15)
    uint16_t blockType;         // Block type ID
};

// Chunk metadata
struct ChunkMetadata {
    uint64_t uuid;              // Unique chunk identifier
    uint64_t lastModified;      // Last modification timestamp
    uint32_t modificationCount; // Number of times modified
    uint32_t blockChanges;      // Total block changes
    bool isDirty;               // Needs saving
    bool isGenerated;           // Was generated (vs loaded)
    bool hasEntities;           // Contains entities
    bool hasLighting;           // Has custom lighting
};

#pragma pack(pop)

// ============================================================================
// SAVE JOURNAL (Crash Recovery)
// ============================================================================

class SaveJournal {
private:
    std::string journalPath;
    std::ofstream journalFile;
    std::mutex journalMutex;

public:
    SaveJournal(const std::string& worldPath);
    ~SaveJournal();

    void beginTransaction(const std::string& operation);
    void endTransaction();
    void rollback();
    bool hasUnfinishedTransactions();
    void recover();
};

// ============================================================================
// COMPRESSION SYSTEM
// ============================================================================

class CompressionSystem {
public:
    enum class Algorithm {
        NONE,
        LZ4,
        ZSTD
    };

private:
    Algorithm algorithm;
    int compressionLevel;

public:
    CompressionSystem(Algorithm algo = Algorithm::LZ4, int level = COMPRESSION_LEVEL);

    // Compress data
    std::vector<uint8_t> compress(const uint8_t* data, size_t size);

    // Decompress data
    std::vector<uint8_t> decompress(const uint8_t* data, size_t compressedSize, size_t uncompressedSize);

    // Calculate CRC32 checksum
    static uint32_t calculateCRC32(const uint8_t* data, size_t size);
};

// ============================================================================
// REGION FILE MANAGER
// ============================================================================

class RegionFile {
private:
    std::string filePath;
    std::fstream file;
    RegionHeader header;
    std::vector<uint32_t> chunkOffsets;     // Offset table (1024 entries)
    std::vector<uint32_t> chunkSizes;       // Size table (1024 entries)
    std::mutex fileMutex;
    bool isDirty;

    int getChunkIndex(int localX, int localZ) const {
        return (localX & 31) + (localZ & 31) * 32;
    }

public:
    RegionFile(const std::string& path, const std::string& worldName, uint64_t seed);
    ~RegionFile();

    bool open();
    void close();
    bool saveChunk(int localX, int localZ, const std::vector<uint8_t>& data);
    std::vector<uint8_t> loadChunk(int localX, int localZ);
    bool hasChunk(int localX, int localZ) const;
    void flush();

    const RegionHeader& getHeader() const { return header; }
};

// ============================================================================
// CHUNK SERIALIZER
// ============================================================================

class ChunkSerializer {
private:
    CompressionSystem compression;

public:
    ChunkSerializer();

    // Serialize chunk to binary format
    std::vector<uint8_t> serialize(
        int chunkX, int chunkZ,
        const void* blockData, size_t blockDataSize,
        const ChunkMetadata& metadata,
        bool useDeltaCompression = true
    );

    // Deserialize chunk from binary format
    bool deserialize(
        const std::vector<uint8_t>& data,
        void* blockDataOut, size_t blockDataSize,
        ChunkMetadata& metadataOut
    );

    // Validate chunk data integrity
    bool validate(const std::vector<uint8_t>& data);
};

// ============================================================================
// SAVE QUEUE (Async Save System)
// ============================================================================

struct SaveTask {
    int chunkX;
    int chunkZ;
    std::vector<uint8_t> data;
    ChunkMetadata metadata;
    uint64_t priority;          // Higher = more important

    SaveTask(int x, int z, std::vector<uint8_t>&& d, const ChunkMetadata& m)
        : chunkX(x), chunkZ(z), data(std::move(d)), metadata(m), priority(0) {}
};

class SaveQueue {
private:
    std::queue<SaveTask> tasks;
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::atomic<size_t> queueSize;

public:
    void push(SaveTask&& task);
    bool pop(SaveTask& taskOut, int timeoutMs = 100);
    size_t size() const { return queueSize.load(); }
    bool empty() const { return queueSize.load() == 0; }
    void clear();
};

// ============================================================================
// BACKUP MANAGER
// ============================================================================

class BackupManager {
private:
    std::string worldPath;
    int maxBackups;
    std::mutex backupMutex;

public:
    BackupManager(const std::string& path, int maxBackups = BACKUP_COUNT);

    void createBackup(const std::string& backupName = "");
    void restoreBackup(const std::string& backupName);
    std::vector<std::string> listBackups();
    void cleanOldBackups();
};

// ============================================================================
// WORLD SAVE MANAGER (Main System)
// ============================================================================

class WorldSaveManager {
private:
    std::string worldPath;
    std::string worldName;
    uint64_t worldSeed;
    int saveVersion;

    // Region file cache
    std::map<std::pair<int, int>, std::unique_ptr<RegionFile>> regionCache;
    std::mutex regionCacheMutex;

    // Chunk metadata
    std::map<std::pair<int, int>, ChunkMetadata> chunkMetadata;
    std::mutex metadataMutex;

    // Dirty chunks (need saving)
    std::unordered_set<uint64_t> dirtyChunks;
    std::mutex dirtyMutex;

    // Async save system
    std::vector<std::thread> saveThreads;
    SaveQueue saveQueue;
    std::atomic<bool> savingActive;

    // Systems
    ChunkSerializer serializer;
    SaveJournal journal;
    BackupManager backupManager;

    // Statistics
    std::atomic<uint64_t> totalChunksSaved;
    std::atomic<uint64_t> totalBytesSaved;
    std::atomic<uint64_t> totalChunksLoaded;

    // Auto-save
    std::chrono::steady_clock::time_point lastAutoSave;

    // Helper functions
    std::pair<int, int> getRegionCoords(int chunkX, int chunkZ);
    RegionFile* getOrCreateRegion(int regionX, int regionZ);
    void saveThreadWorker();
    uint64_t packChunkCoords(int x, int z) const {
        return ((uint64_t)(uint32_t)x << 32) | (uint32_t)z;
    }

public:
    WorldSaveManager(const std::string& path, const std::string& name, uint64_t seed);
    ~WorldSaveManager();

    // Initialize/shutdown
    bool initialize();
    void shutdown();

    // Save operations
    void saveChunkAsync(int chunkX, int chunkZ, const void* blockData, size_t blockDataSize, const ChunkMetadata& metadata);
    void saveChunkSync(int chunkX, int chunkZ, const void* blockData, size_t blockDataSize, const ChunkMetadata& metadata);
    void saveAllDirtyChunks();
    void flushAll();

    // Load operations
    bool loadChunk(int chunkX, int chunkZ, void* blockDataOut, size_t blockDataSize, ChunkMetadata& metadataOut);
    bool chunkExists(int chunkX, int chunkZ);

    // Metadata operations
    void markChunkDirty(int chunkX, int chunkZ);
    void markChunkClean(int chunkX, int chunkZ);
    bool isChunkDirty(int chunkX, int chunkZ);
    ChunkMetadata* getChunkMetadata(int chunkX, int chunkZ);

    // Auto-save
    void updateAutoSave();

    // Backup operations
    void createBackup(const std::string& name = "");
    void restoreBackup(const std::string& name);

    // Statistics
    struct Statistics {
        uint64_t chunksSaved;
        uint64_t chunksLoaded;
        uint64_t bytesSaved;
        uint64_t bytesLoaded;
        size_t dirtyChunkCount;
        size_t regionCount;
        size_t queuedSaves;
    };
    Statistics getStatistics() const;

    // World info
    const std::string& getWorldName() const { return worldName; }
    uint64_t getWorldSeed() const { return worldSeed; }
    int getSaveVersion() const { return saveVersion; }
};

} // namespace SaveSystem
} // namespace VoxelWorld
