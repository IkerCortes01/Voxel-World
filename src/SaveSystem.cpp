#include "SaveSystem.h"
#include <iostream>
#include <filesystem>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <array>

namespace fs = std::filesystem;

namespace VoxelWorld {
namespace SaveSystem {

// ============================================================================
// COMPRESSION SYSTEM IMPLEMENTATION
// ============================================================================

// RLE con byte de escape.
// Formato v2: [0xFF][count>=1][value] = run; [0xFF][0x00] = literal 0xFF.
// El formato v1 emitía el literal 0xFF sin escapar, corrompiendo el chunk al
// descomprimir; se conserva un decodificador legacy para leer saves v1.
class SimpleLZ4 {
public:
    static std::vector<uint8_t> compress(const uint8_t* src, size_t srcSize) {
        std::vector<uint8_t> compressed;
        compressed.reserve(srcSize / 2);

        for (size_t i = 0; i < srcSize; ) {
            uint8_t value = src[i];
            size_t runLength = 1;

            while (i + runLength < srcSize && src[i + runLength] == value && runLength < 255) {
                runLength++;
            }

            if (runLength >= 4) {
                // RLE: [0xFF] [count] [value]
                compressed.push_back(0xFF);
                compressed.push_back((uint8_t)runLength);
                compressed.push_back(value);
                i += runLength;
            } else if (value == 0xFF) {
                // Literal 0xFF: debe escaparse para no confundirse con el marcador
                for (size_t j = 0; j < runLength; j++) {
                    compressed.push_back(0xFF);
                    compressed.push_back(0x00);
                }
                i += runLength;
            } else {
                compressed.push_back(value);
                i++;
            }
        }

        return compressed;
    }

    // Devuelve vector vacío si los datos están malformados o exceden dstSize.
    static std::vector<uint8_t> decompress(const uint8_t* src, size_t srcSize, size_t dstSize) {
        std::vector<uint8_t> decompressed;
        decompressed.reserve(dstSize);

        for (size_t i = 0; i < srcSize; ) {
            if (src[i] == 0xFF) {
                if (i + 1 >= srcSize) return {};  // marcador truncado
                uint8_t count = src[i + 1];
                if (count == 0) {
                    // Escape: literal 0xFF
                    if (decompressed.size() + 1 > dstSize) return {};
                    decompressed.push_back(0xFF);
                    i += 2;
                } else {
                    if (i + 2 >= srcSize) return {};  // run truncado
                    uint8_t value = src[i + 2];
                    if (decompressed.size() + count > dstSize) return {};
                    decompressed.insert(decompressed.end(), count, value);
                    i += 3;
                }
            } else {
                if (decompressed.size() + 1 > dstSize) return {};
                decompressed.push_back(src[i]);
                i++;
            }
        }

        return decompressed;
    }

    // Decodificador del formato v1 (sin escape), solo para migrar saves viejos.
    // Acotado a dstSize para que un archivo corrupto no agote la memoria.
    static std::vector<uint8_t> decompressLegacyV1(const uint8_t* src, size_t srcSize, size_t dstSize) {
        std::vector<uint8_t> decompressed;
        decompressed.reserve(dstSize);

        for (size_t i = 0; i < srcSize; ) {
            if (src[i] == 0xFF && i + 2 < srcSize) {
                uint8_t count = src[i + 1];
                uint8_t value = src[i + 2];
                if (decompressed.size() + count > dstSize) return {};
                decompressed.insert(decompressed.end(), count, value);
                i += 3;
            } else {
                if (decompressed.size() + 1 > dstSize) return {};
                decompressed.push_back(src[i]);
                i++;
            }
        }

        return decompressed;
    }
};

CompressionSystem::CompressionSystem(Algorithm algo, int level)
    : algorithm(algo), compressionLevel(level) {}

std::vector<uint8_t> CompressionSystem::compress(const uint8_t* data, size_t size) {
    switch (algorithm) {
        case Algorithm::LZ4:
            return SimpleLZ4::compress(data, size);
        case Algorithm::NONE:
        default:
            return std::vector<uint8_t>(data, data + size);
    }
}

std::vector<uint8_t> CompressionSystem::decompress(const uint8_t* data, size_t compressedSize, size_t uncompressedSize) {
    switch (algorithm) {
        case Algorithm::LZ4:
            return SimpleLZ4::decompress(data, compressedSize, uncompressedSize);
        case Algorithm::NONE:
        default:
            return std::vector<uint8_t>(data, data + compressedSize);
    }
}

// CRC32 estándar (polinomio 0xEDB88320, tabla generada una sola vez)
uint32_t CompressionSystem::calculateCRC32(const uint8_t* data, size_t size) {
    static const std::array<uint32_t, 256> table = []() {
        std::array<uint32_t, 256> t{};
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) {
                c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            t[i] = c;
        }
        return t;
    }();

    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; i++) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return ~crc;
}

// Réplica exacta del "CRC" defectuoso de la versión 1 (tabla vacía, algoritmo
// degenerado). Solo se usa para verificar saves v1 existentes.
static uint32_t legacyCRC32v1(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        uint8_t byte = data[i];
        crc = (crc >> 8) ^ ((crc ^ byte) & 0xFF);
    }
    return ~crc;
}

// ============================================================================
// SAVE JOURNAL IMPLEMENTATION
// ============================================================================

SaveJournal::SaveJournal(const std::string& worldPath)
    : journalPath(worldPath + "/save.journal") {
    fs::create_directories(worldPath);
}

SaveJournal::~SaveJournal() {
    if (journalFile.is_open()) {
        journalFile.close();
    }
}

void SaveJournal::beginTransaction(const std::string& operation) {
    std::lock_guard<std::mutex> lock(journalMutex);

    if (!journalFile.is_open()) {
        journalFile.open(journalPath, std::ios::app);
    }

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    journalFile << "BEGIN " << timestamp << " " << operation << std::endl;
    journalFile.flush();
}

void SaveJournal::endTransaction() {
    std::lock_guard<std::mutex> lock(journalMutex);

    if (journalFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        journalFile << "END " << timestamp << std::endl;
        journalFile.flush();
    }
}

void SaveJournal::rollback() {
    std::lock_guard<std::mutex> lock(journalMutex);
    // Close and delete journal
    if (journalFile.is_open()) {
        journalFile.close();
    }
    fs::remove(journalPath);
}

bool SaveJournal::hasUnfinishedTransactions() {
    std::lock_guard<std::mutex> lock(journalMutex);
    std::ifstream check(journalPath);
    if (!check.is_open()) return false;

    std::string line;
    int begins = 0, ends = 0;
    while (std::getline(check, line)) {
        if (line.find("BEGIN") == 0) begins++;
        if (line.find("END") == 0) ends++;
    }

    return begins > ends;
}

void SaveJournal::recover() {
    if (hasUnfinishedTransactions()) {
        std::cout << "[SaveJournal] Recovering from incomplete transaction..." << std::endl;
        rollback();
    }
}

// ============================================================================
// REGION FILE IMPLEMENTATION
// ============================================================================

RegionFile::RegionFile(const std::string& path, const std::string& worldName, uint64_t seed)
    : filePath(path), isDirty(false) {

    // Initialize header
    memset(&header, 0, sizeof(RegionHeader));
    memcpy(header.magic, "VXRF", 4);
    header.version = SAVE_VERSION;
    header.timestamp = (uint32_t)time(nullptr);
    header.chunkCount = 0;
    header.flags = 0;
    strncpy(header.worldName, worldName.c_str(), sizeof(header.worldName) - 1);
    header.worldSeed = seed;

    // Initialize offset/size tables
    chunkOffsets.resize(1024, 0);
    chunkSizes.resize(1024, 0);
}

RegionFile::~RegionFile() {
    close();
}

bool RegionFile::open() {
    std::lock_guard<std::mutex> lock(fileMutex);

    // Create directory if needed
    fs::path filepath(filePath);
    fs::create_directories(filepath.parent_path());

    // Open or create file
    if (fs::exists(filePath)) {
        file.open(filePath, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) return false;

        // Read header
        file.read((char*)&header, sizeof(RegionHeader));
        if (!file.good()) {
            std::cerr << "[RegionFile] Header truncado en " << filePath << std::endl;
            return false;
        }

        // Verify magic
        if (memcmp(header.magic, "VXRF", 4) != 0) {
            std::cerr << "[RegionFile] Invalid magic number in " << filePath << std::endl;
            return false;
        }

        if (header.version == 0 || header.version > (uint32_t)SAVE_VERSION) {
            std::cerr << "[RegionFile] Version no soportada (" << header.version
                      << ") en " << filePath << std::endl;
            return false;
        }

        // Read offset table
        for (int i = 0; i < 1024; i++) {
            file.read((char*)&chunkOffsets[i], sizeof(uint32_t));
        }

        // Read size table
        for (int i = 0; i < 1024; i++) {
            file.read((char*)&chunkSizes[i], sizeof(uint32_t));
        }

        if (!file.good()) {
            std::cerr << "[RegionFile] Tablas truncadas en " << filePath << std::endl;
            return false;
        }

    } else {
        // Create new file
        file.open(filePath, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) return false;

        // Write header
        file.write((char*)&header, sizeof(RegionHeader));

        // Write empty offset table
        for (int i = 0; i < 1024; i++) {
            uint32_t zero = 0;
            file.write((char*)&zero, sizeof(uint32_t));
        }

        // Write empty size table
        for (int i = 0; i < 1024; i++) {
            uint32_t zero = 0;
            file.write((char*)&zero, sizeof(uint32_t));
        }

        file.flush();
    }

    return true;
}

void RegionFile::close() {
    std::lock_guard<std::mutex> lock(fileMutex);
    if (file.is_open()) {
        flushLocked();
        file.close();
    }
}

bool RegionFile::saveChunk(int localX, int localZ, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(fileMutex);

    if (!file.is_open()) return false;

    int index = getChunkIndex(localX, localZ);
    uint32_t dataSize = (uint32_t)data.size();

    if (dataSize == 0 || dataSize > (uint32_t)MAX_CHUNK_SIZE) {
        std::cerr << "[RegionFile] saveChunk rechazado: tamano invalido " << dataSize << std::endl;
        return false;
    }

    // Calculate sector count needed
    uint32_t sectorCount = (dataSize + SECTOR_SIZE - 1) / SECTOR_SIZE;

    // Find free space at end of file
    file.seekp(0, std::ios::end);
    uint32_t offset = (uint32_t)file.tellp();

    // Write chunk data
    file.write((const char*)data.data(), dataSize);

    // Pad to sector boundary
    uint32_t padding = (sectorCount * SECTOR_SIZE) - dataSize;
    std::vector<uint8_t> paddingData(padding, 0);
    file.write((const char*)paddingData.data(), padding);

    if (!file.good()) {
        // Escritura fallida: NO actualizar tablas — la entrada anterior (si
        // existía) sigue apuntando a datos completos y válidos.
        std::cerr << "[RegionFile] Error de escritura en " << filePath << std::endl;
        file.clear();
        return false;
    }

    // Update tables (los datos ya están completos en disco: si crasheamos
    // antes del flush, la tabla vieja sigue apuntando al chunk anterior íntegro)
    bool wasNew = (chunkOffsets[index] == 0);
    chunkOffsets[index] = offset;
    chunkSizes[index] = dataSize;

    // Solo contar chunks nuevos, no sobreescrituras
    if (wasNew) {
        header.chunkCount++;
    }
    header.timestamp = (uint32_t)time(nullptr);

    isDirty = true;

    // ⭐ Persistir tablas inmediatamente: reduce la ventana en la que un crash
    // deja datos escritos pero no referenciados por la tabla.
    flushLocked();
    return true;
}

std::vector<uint8_t> RegionFile::loadChunk(int localX, int localZ) {
    std::lock_guard<std::mutex> lock(fileMutex);

    if (!file.is_open()) return {};

    int index = getChunkIndex(localX, localZ);
    uint32_t offset = chunkOffsets[index];
    uint32_t size = chunkSizes[index];

    if (offset == 0 || size == 0) return {};

    // ⭐ Validar contra valores corruptos del archivo: sin esto, un size
    // arbitrario provoca una allocación no acotada (hasta 4 GiB)
    if (size > (uint32_t)MAX_CHUNK_SIZE) {
        std::cerr << "[RegionFile] Chunk (" << localX << "," << localZ
                  << ") con tamano corrupto: " << size << std::endl;
        return {};
    }
    constexpr uint32_t dataStart = sizeof(RegionHeader) + 2 * 1024 * sizeof(uint32_t);
    if (offset < dataStart) {
        std::cerr << "[RegionFile] Chunk (" << localX << "," << localZ
                  << ") con offset corrupto: " << offset << std::endl;
        return {};
    }

    // Read chunk data
    std::vector<uint8_t> data(size);
    file.seekg(offset);
    file.read((char*)data.data(), size);

    if (!file.good() || (size_t)file.gcount() != size) {
        std::cerr << "[RegionFile] Lectura truncada del chunk (" << localX
                  << "," << localZ << ")" << std::endl;
        file.clear();
        return {};
    }

    return data;
}

bool RegionFile::hasChunk(int localX, int localZ) const {
    std::lock_guard<std::mutex> lock(fileMutex);
    int index = getChunkIndex(localX, localZ);
    return chunkOffsets[index] != 0;
}

void RegionFile::flush() {
    std::lock_guard<std::mutex> lock(fileMutex);
    flushLocked();
}

void RegionFile::flushLocked() {
    if (!isDirty) return;

    // Write header
    file.seekp(0);
    file.write((char*)&header, sizeof(RegionHeader));

    // Write offset table
    for (int i = 0; i < 1024; i++) {
        file.write((char*)&chunkOffsets[i], sizeof(uint32_t));
    }

    // Write size table
    for (int i = 0; i < 1024; i++) {
        file.write((char*)&chunkSizes[i], sizeof(uint32_t));
    }

    file.flush();
    isDirty = false;
}

// ============================================================================
// CHUNK SERIALIZER IMPLEMENTATION
// ============================================================================

ChunkSerializer::ChunkSerializer() : compression(CompressionSystem::Algorithm::LZ4, 3) {}

std::vector<uint8_t> ChunkSerializer::serialize(
    int chunkX, int chunkZ,
    const void* blockData, size_t blockDataSize,
    const ChunkMetadata& metadata,
    bool useDeltaCompression)
{
    std::vector<uint8_t> result;

    // Compress block data
    std::vector<uint8_t> compressed = compression.compress((const uint8_t*)blockData, blockDataSize);

    // Build header
    ChunkSaveHeader header;
    memset(&header, 0, sizeof(ChunkSaveHeader));
    memcpy(header.magic, "CHNK", 4);
    header.version = SAVE_VERSION;
    header.compressedSize = (uint32_t)compressed.size();
    header.uncompressedSize = (uint32_t)blockDataSize;
    header.crc32 = CompressionSystem::calculateCRC32(compressed.data(), compressed.size());
    header.flags = 0x01; // Compressed flag
    header.chunkX = chunkX;
    header.chunkZ = chunkZ;
    header.timestamp = metadata.lastModified;
    header.blockCount = metadata.blockChanges;

    // Write header
    result.resize(sizeof(ChunkSaveHeader));
    memcpy(result.data(), &header, sizeof(ChunkSaveHeader));

    // Append compressed data
    result.insert(result.end(), compressed.begin(), compressed.end());

    return result;
}

bool ChunkSerializer::deserialize(
    const std::vector<uint8_t>& data,
    void* blockDataOut, size_t blockDataSize,
    ChunkMetadata& metadataOut)
{
    if (data.size() < sizeof(ChunkSaveHeader)) return false;

    // Read header
    ChunkSaveHeader header;
    memcpy(&header, data.data(), sizeof(ChunkSaveHeader));

    // Verify magic
    if (memcmp(header.magic, "CHNK", 4) != 0) return false;

    // ⭐ Validar campos del header ANTES de usarlos como longitudes:
    // un archivo corrupto/truncado no debe provocar lecturas fuera del buffer
    // ni allocaciones gigantes.
    if (header.version == 0 || header.version > (uint32_t)SAVE_VERSION) {
        std::cerr << "[ChunkSerializer] Version no soportada: " << header.version << std::endl;
        return false;
    }
    if (header.compressedSize == 0 ||
        header.compressedSize > (uint32_t)MAX_CHUNK_SIZE ||
        data.size() < sizeof(ChunkSaveHeader) + header.compressedSize) {
        std::cerr << "[ChunkSerializer] compressedSize invalido: " << header.compressedSize
                  << " (buffer: " << data.size() << ")" << std::endl;
        return false;
    }
    if (header.uncompressedSize != blockDataSize) {
        std::cerr << "[ChunkSerializer] uncompressedSize no coincide: "
                  << header.uncompressedSize << " vs " << blockDataSize << std::endl;
        return false;
    }

    const uint8_t* compressedData = data.data() + sizeof(ChunkSaveHeader);

    // Verify CRC (algoritmo según versión del archivo)
    uint32_t calculatedCRC = (header.version >= 2)
        ? CompressionSystem::calculateCRC32(compressedData, header.compressedSize)
        : legacyCRC32v1(compressedData, header.compressedSize);
    if (calculatedCRC != header.crc32) {
        std::cerr << "[ChunkSerializer] CRC mismatch!" << std::endl;
        return false;
    }

    // Decompress (decodificador según versión del archivo)
    std::vector<uint8_t> decompressed = (header.version >= 2)
        ? SimpleLZ4::decompress(compressedData, header.compressedSize, header.uncompressedSize)
        : SimpleLZ4::decompressLegacyV1(compressedData, header.compressedSize, header.uncompressedSize);

    if (decompressed.size() != blockDataSize) {
        std::cerr << "[ChunkSerializer] Size mismatch!" << std::endl;
        return false;
    }

    // Copy to output
    memcpy(blockDataOut, decompressed.data(), blockDataSize);

    // Fill metadata
    metadataOut.lastModified = header.timestamp;
    metadataOut.blockChanges = header.blockCount;

    return true;
}

bool ChunkSerializer::validate(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(ChunkSaveHeader)) return false;

    ChunkSaveHeader header;
    memcpy(&header, data.data(), sizeof(ChunkSaveHeader));

    if (memcmp(header.magic, "CHNK", 4) != 0) return false;

    // Mismas validaciones de límites que deserialize
    if (header.version == 0 || header.version > (uint32_t)SAVE_VERSION) return false;
    if (header.compressedSize == 0 ||
        header.compressedSize > (uint32_t)MAX_CHUNK_SIZE ||
        data.size() < sizeof(ChunkSaveHeader) + header.compressedSize) {
        return false;
    }

    const uint8_t* compressedData = data.data() + sizeof(ChunkSaveHeader);
    uint32_t calculatedCRC = (header.version >= 2)
        ? CompressionSystem::calculateCRC32(compressedData, header.compressedSize)
        : legacyCRC32v1(compressedData, header.compressedSize);

    return calculatedCRC == header.crc32;
}

// ============================================================================
// SAVE QUEUE IMPLEMENTATION
// ============================================================================

void SaveQueue::push(SaveTask&& task) {
    std::lock_guard<std::mutex> lock(queueMutex);
    tasks.push(std::move(task));
    queueSize.fetch_add(1);
    queueCV.notify_one();
}

bool SaveQueue::pop(SaveTask& taskOut, int timeoutMs) {
    std::unique_lock<std::mutex> lock(queueMutex);

    if (tasks.empty()) {
        if (timeoutMs > 0) {
            queueCV.wait_for(lock, std::chrono::milliseconds(timeoutMs));
        }
    }

    if (tasks.empty()) return false;

    taskOut = std::move(tasks.front());
    tasks.pop();
    queueSize.fetch_sub(1);
    return true;
}

void SaveQueue::clear() {
    std::lock_guard<std::mutex> lock(queueMutex);
    while (!tasks.empty()) {
        tasks.pop();
    }
    queueSize.store(0);
}

// ============================================================================
// BACKUP MANAGER IMPLEMENTATION
// ============================================================================

BackupManager::BackupManager(const std::string& path, int maxBackups)
    : worldPath(path), maxBackups(maxBackups) {}

void BackupManager::createBackup(const std::string& backupName) {
    std::lock_guard<std::mutex> lock(backupMutex);

    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "backup_" << timestamp;
    std::string name = backupName.empty() ? ss.str() : backupName;

    fs::path backupPath = fs::path(worldPath) / "backups" / name;
    fs::create_directories(backupPath);

    try {
        // ⭐ Copiar hijo por hijo EXCLUYENDO backups/: copiar worldPath entero
        // metía los backups anteriores dentro del nuevo (crecimiento exponencial)
        for (const auto& entry : fs::directory_iterator(worldPath)) {
            if (entry.path().filename() == "backups") continue;
            fs::copy(entry.path(), backupPath / entry.path().filename(),
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }
        std::cout << "[BackupManager] Created backup: " << name << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[BackupManager] Backup failed: " << e.what() << std::endl;
    }

    cleanOldBackups();
}

void BackupManager::cleanOldBackups() {
    fs::path backupsDir = fs::path(worldPath) / "backups";
    if (!fs::exists(backupsDir)) return;

    std::vector<fs::path> backups;
    for (const auto& entry : fs::directory_iterator(backupsDir)) {
        if (entry.is_directory()) {
            backups.push_back(entry.path());
        }
    }

    if (backups.size() <= (size_t)maxBackups) return;

    // Sort by modification time
    std::sort(backups.begin(), backups.end(), [](const fs::path& a, const fs::path& b) {
        return fs::last_write_time(a) < fs::last_write_time(b);
    });

    // Delete oldest
    size_t toDelete = backups.size() - maxBackups;
    for (size_t i = 0; i < toDelete; i++) {
        fs::remove_all(backups[i]);
        std::cout << "[BackupManager] Deleted old backup: " << backups[i].filename() << std::endl;
    }
}

std::vector<std::string> BackupManager::listBackups() {
    std::vector<std::string> result;
    fs::path backupsDir = fs::path(worldPath) / "backups";

    if (!fs::exists(backupsDir)) return result;

    for (const auto& entry : fs::directory_iterator(backupsDir)) {
        if (entry.is_directory()) {
            result.push_back(entry.path().filename().string());
        }
    }

    return result;
}

void BackupManager::restoreBackup(const std::string& backupName) {
    std::lock_guard<std::mutex> lock(backupMutex);

    fs::path backupPath = fs::path(worldPath) / "backups" / backupName;
    if (!fs::exists(backupPath)) {
        std::cerr << "[BackupManager] Backup not found: " << backupName << std::endl;
        return;
    }

    // ⭐ Restaurar en 3 pasos para no destruir el mundo si la copia falla:
    // 1) copiar el backup a un staging temporal, 2) recién entonces borrar los
    // datos actuales, 3) mover el staging a su lugar (renames, casi atómico).
    fs::path stagingPath = fs::path(worldPath) / "_restore_staging";
    try {
        // Paso 1: copiar backup a staging (si falla, el mundo queda intacto)
        fs::remove_all(stagingPath);
        fs::create_directories(stagingPath);
        for (const auto& entry : fs::directory_iterator(backupPath)) {
            fs::copy(entry.path(), stagingPath / entry.path().filename(),
                     fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        }

        // Paso 2: borrar datos actuales (excepto backups y el staging)
        for (const auto& entry : fs::directory_iterator(worldPath)) {
            const auto name = entry.path().filename();
            if (name != "backups" && name != "_restore_staging") {
                fs::remove_all(entry.path());
            }
        }

        // Paso 3: mover contenido del staging al mundo (renames rápidos)
        for (const auto& entry : fs::directory_iterator(stagingPath)) {
            fs::rename(entry.path(), fs::path(worldPath) / entry.path().filename());
        }
        fs::remove_all(stagingPath);

        std::cout << "[BackupManager] Restored backup: " << backupName << std::endl;
    } catch (const fs::filesystem_error& e) {
        std::cerr << "[BackupManager] Restore failed: " << e.what() << std::endl;
        std::error_code ec;
        fs::remove_all(stagingPath, ec);
    }
}

// ============================================================================
// WORLD SAVE MANAGER IMPLEMENTATION
// ============================================================================

WorldSaveManager::WorldSaveManager(const std::string& path, const std::string& name, uint64_t seed)
    : worldPath(path), worldName(name), worldSeed(seed), saveVersion(SAVE_VERSION),
      journal(path), backupManager(path), savingActive(false),
      totalChunksSaved(0), totalBytesSaved(0), totalChunksLoaded(0) {

    lastAutoSave = std::chrono::steady_clock::now();
}

WorldSaveManager::~WorldSaveManager() {
    shutdown();
}

bool WorldSaveManager::initialize() {
    std::cout << "[WorldSaveManager] Initializing for world: " << worldName << std::endl;

    // Create world directory
    fs::create_directories(worldPath);
    fs::create_directories(worldPath + "/regions");

    // Check for crash recovery
    journal.recover();

    // Start save threads
    savingActive.store(true);
    for (int i = 0; i < SAVE_THREAD_COUNT; i++) {
        saveThreads.emplace_back(&WorldSaveManager::saveThreadWorker, this);
    }

    std::cout << "[WorldSaveManager] Initialized with " << SAVE_THREAD_COUNT << " save threads" << std::endl;
    return true;
}

void WorldSaveManager::shutdown() {
    std::cout << "[WorldSaveManager] Shutting down..." << std::endl;

    // Save all dirty chunks
    saveAllDirtyChunks();

    // Stop save threads
    savingActive.store(false);
    for (auto& thread : saveThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Flush all regions
    flushAll();

    std::cout << "[WorldSaveManager] Shutdown complete" << std::endl;
}

std::pair<int, int> WorldSaveManager::getRegionCoords(int chunkX, int chunkZ) {
    int regionX = chunkX >> 5; // Divide by 32
    int regionZ = chunkZ >> 5;
    if (chunkX < 0 && (chunkX & 31) != 0) regionX--;
    if (chunkZ < 0 && (chunkZ & 31) != 0) regionZ--;
    return {regionX, regionZ};
}

RegionFile* WorldSaveManager::getOrCreateRegion(int regionX, int regionZ) {
    std::lock_guard<std::mutex> lock(regionCacheMutex);

    auto key = std::make_pair(regionX, regionZ);
    auto it = regionCache.find(key);

    if (it != regionCache.end()) {
        return it->second.get();
    }

    // Create new region file
    std::stringstream ss;
    ss << worldPath << "/regions/r." << regionX << "." << regionZ << ".vxr";
    std::string regionPath = ss.str();

    auto region = std::make_unique<RegionFile>(regionPath, worldName, worldSeed);
    if (!region->open()) {
        std::cerr << "[WorldSaveManager] Failed to open region: " << regionPath << std::endl;
        return nullptr;
    }

    RegionFile* ptr = region.get();
    regionCache[key] = std::move(region);
    return ptr;
}

void WorldSaveManager::saveThreadWorker() {
    while (savingActive.load()) {
        SaveTask task(0, 0, std::vector<uint8_t>(), ChunkMetadata());

        if (saveQueue.pop(task, 100)) {
            // ⭐ Una excepción aquí (bad_alloc, filesystem_error) mataría el
            // proceso entero via std::terminate: capturar y seguir.
            try {
                // Get region coordinates
                auto [regionX, regionZ] = getRegionCoords(task.chunkX, task.chunkZ);
                int localX = task.chunkX & 31;
                int localZ = task.chunkZ & 31;

                // Get or create region
                RegionFile* region = getOrCreateRegion(regionX, regionZ);
                if (!region) continue;

                // Journal transaction
                std::stringstream ss;
                ss << "SAVE_CHUNK " << task.chunkX << " " << task.chunkZ;
                journal.beginTransaction(ss.str());

                // Save chunk
                bool success = region->saveChunk(localX, localZ, task.data);

                if (success) {
                    totalChunksSaved.fetch_add(1);
                    totalBytesSaved.fetch_add(task.data.size());
                    markChunkClean(task.chunkX, task.chunkZ);
                }

                journal.endTransaction();
            } catch (const std::exception& e) {
                std::cerr << "[SaveThread] Error guardando chunk (" << task.chunkX
                          << "," << task.chunkZ << "): " << e.what() << std::endl;
            }
        }
    }
}

void WorldSaveManager::saveChunkAsync(int chunkX, int chunkZ, const void* blockData, size_t blockDataSize, const ChunkMetadata& metadata) {
    // Serialize chunk
    std::vector<uint8_t> serialized = serializer.serialize(chunkX, chunkZ, blockData, blockDataSize, metadata);

    // Queue for async save
    SaveTask task(chunkX, chunkZ, std::move(serialized), metadata);
    saveQueue.push(std::move(task));

    // Update metadata
    {
        std::lock_guard<std::mutex> lock(metadataMutex);
        chunkMetadata[{chunkX, chunkZ}] = metadata;
    }
}

void WorldSaveManager::saveChunkSync(int chunkX, int chunkZ, const void* blockData, size_t blockDataSize, const ChunkMetadata& metadata) {
    // Serialize chunk
    std::vector<uint8_t> serialized = serializer.serialize(chunkX, chunkZ, blockData, blockDataSize, metadata);

    // Get region coordinates
    auto [regionX, regionZ] = getRegionCoords(chunkX, chunkZ);
    int localX = chunkX & 31;
    int localZ = chunkZ & 31;

    // Get or create region
    RegionFile* region = getOrCreateRegion(regionX, regionZ);
    if (!region) return;

    // Save immediately
    journal.beginTransaction("SAVE_CHUNK_SYNC");
    bool success = region->saveChunk(localX, localZ, serialized);
    journal.endTransaction();

    if (success) {
        totalChunksSaved.fetch_add(1);
        totalBytesSaved.fetch_add(serialized.size());
        markChunkClean(chunkX, chunkZ);

        // Update metadata
        std::lock_guard<std::mutex> lock(metadataMutex);
        chunkMetadata[{chunkX, chunkZ}] = metadata;
    }
}

bool WorldSaveManager::loadChunk(int chunkX, int chunkZ, void* blockDataOut, size_t blockDataSize, ChunkMetadata& metadataOut) {
    // Get region coordinates
    auto [regionX, regionZ] = getRegionCoords(chunkX, chunkZ);
    int localX = chunkX & 31;
    int localZ = chunkZ & 31;

    // Get region
    RegionFile* region = getOrCreateRegion(regionX, regionZ);
    if (!region) return false;

    // Load chunk data
    std::vector<uint8_t> data = region->loadChunk(localX, localZ);
    if (data.empty()) return false;

    // Deserialize
    bool success = serializer.deserialize(data, blockDataOut, blockDataSize, metadataOut);

    if (success) {
        totalChunksLoaded.fetch_add(1);

        // Update metadata
        std::lock_guard<std::mutex> lock(metadataMutex);
        chunkMetadata[{chunkX, chunkZ}] = metadataOut;
    }

    return success;
}

bool WorldSaveManager::chunkExists(int chunkX, int chunkZ) {
    auto [regionX, regionZ] = getRegionCoords(chunkX, chunkZ);
    int localX = chunkX & 31;
    int localZ = chunkZ & 31;

    RegionFile* region = getOrCreateRegion(regionX, regionZ);
    if (!region) return false;

    return region->hasChunk(localX, localZ);
}

void WorldSaveManager::markChunkDirty(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(dirtyMutex);
    dirtyChunks.insert(packChunkCoords(chunkX, chunkZ));
}

void WorldSaveManager::markChunkClean(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(dirtyMutex);
    dirtyChunks.erase(packChunkCoords(chunkX, chunkZ));
}

bool WorldSaveManager::isChunkDirty(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(dirtyMutex);
    return dirtyChunks.count(packChunkCoords(chunkX, chunkZ)) > 0;
}

ChunkMetadata* WorldSaveManager::getChunkMetadata(int chunkX, int chunkZ) {
    std::lock_guard<std::mutex> lock(metadataMutex);
    auto it = chunkMetadata.find({chunkX, chunkZ});
    return (it != chunkMetadata.end()) ? &it->second : nullptr;
}

void WorldSaveManager::saveAllDirtyChunks() {
    std::cout << "[WorldSaveManager] Saving all dirty chunks..." << std::endl;

    // Wait for queue to empty
    while (saveQueue.size() > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[WorldSaveManager] All dirty chunks saved" << std::endl;
}

void WorldSaveManager::flushAll() {
    std::lock_guard<std::mutex> lock(regionCacheMutex);
    for (auto& pair : regionCache) {
        pair.second->flush();
    }
}

void WorldSaveManager::updateAutoSave() {
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastAutoSave).count();

    if (elapsed >= AUTO_SAVE_INTERVAL) {
        std::cout << "[WorldSaveManager] Auto-save triggered" << std::endl;
        saveAllDirtyChunks();
        lastAutoSave = now;
    }
}

void WorldSaveManager::createBackup(const std::string& name) {
    backupManager.createBackup(name);
}

void WorldSaveManager::restoreBackup(const std::string& name) {
    backupManager.restoreBackup(name);
}

WorldSaveManager::Statistics WorldSaveManager::getStatistics() const {
    Statistics stats;
    stats.chunksSaved = totalChunksSaved.load();
    stats.chunksLoaded = totalChunksLoaded.load();
    stats.bytesSaved = totalBytesSaved.load();
    stats.bytesLoaded = 0; // TODO: track
    {
        std::lock_guard<std::mutex> lock(dirtyMutex);
        stats.dirtyChunkCount = dirtyChunks.size();
    }
    {
        std::lock_guard<std::mutex> lock(regionCacheMutex);
        stats.regionCount = regionCache.size();
    }
    stats.queuedSaves = saveQueue.size();
    return stats;
}

} // namespace SaveSystem
} // namespace VoxelWorld
