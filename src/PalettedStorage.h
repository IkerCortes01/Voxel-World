#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>

// ============================================================================
// SISTEMA DE PALETAS - Optimización de Memoria para Chunks
// ============================================================================
// Inspirado en Minecraft 1.13+ y versiones modernas
// Reduce el uso de memoria de ~260KB por chunk a ~20-100KB dependiendo de la variedad de bloques
// ============================================================================

// NOTA: BlockType debe estar definido ANTES de incluir este header
// En main.cpp se define como: enum BlockType { BLOCK_AIR = 0, ... };


// ============================================================================
// PALETTED SUBCHUNK - Chunk de 16x16x16 con paleta local
// ============================================================================
class PalettedSubChunk {
private:
    static constexpr int SUBCHUNK_SIZE = 16;
    static constexpr int TOTAL_BLOCKS = SUBCHUNK_SIZE * SUBCHUNK_SIZE * SUBCHUNK_SIZE; // 4096 bloques

    // ⭐ PALETA LOCAL: Lista única de tipos de bloques presentes en este subchunk
    std::vector<BlockType> palette;

    // ⭐ ÍNDICES: Arreglo de índices a la paleta (bit-packed)
    // El tamaño de bits por índice se ajusta dinámicamente según el tamaño de la paleta
    std::vector<uint64_t> indices;

    // ⭐ Bits por índice (se calcula automáticamente según el tamaño de la paleta)
    uint8_t bitsPerIndex;

    // ⭐ Calcular cuántos bits necesitamos para representar N elementos de paleta
    static uint8_t calculateBitsPerIndex(size_t paletteSize) {
        if (paletteSize <= 1) return 0;  // 1 bloque = 0 bits (todos iguales)
        if (paletteSize <= 2) return 1;  // 2 bloques = 1 bit
        if (paletteSize <= 4) return 2;  // 4 bloques = 2 bits
        if (paletteSize <= 16) return 4; // 16 bloques = 4 bits
        if (paletteSize <= 256) return 8; // 256 bloques = 8 bits
        return 16; // 65536 bloques = 16 bits (máximo realista)
    }

    // ⭐ Calcular cuántos uint64_t necesitamos para almacenar todos los índices
    static size_t calculateIndicesSize(uint8_t bitsPerIdx) {
        if (bitsPerIdx == 0) return 0; // Paleta de 1 elemento = no necesita índices

        // Cada uint64_t almacena 64 bits
        // Necesitamos TOTAL_BLOCKS índices de bitsPerIdx bits cada uno
        size_t totalBits = (size_t)TOTAL_BLOCKS * bitsPerIdx;
        return (totalBits + 63) / 64; // Redondear hacia arriba
    }

    // ⭐ BIT-PACKING: Obtener índice de paleta para un bloque específico
    uint16_t getPackedIndex(int blockIndex) const {
        if (bitsPerIndex == 0) return 0; // Paleta de 1 elemento

        // Calcular qué uint64_t contiene este índice y el offset dentro de él
        size_t bitOffset = (size_t)blockIndex * bitsPerIndex;
        size_t arrayIndex = bitOffset / 64;
        size_t bitPosition = bitOffset % 64;

        // Crear máscara para extraer los bits
        uint64_t mask = ((uint64_t)1 << bitsPerIndex) - 1;

        // Extraer los bits
        if (bitPosition + bitsPerIndex <= 64) {
            // Caso simple: el índice cabe en un solo uint64_t
            return (uint16_t)((indices[arrayIndex] >> bitPosition) & mask);
        } else {
            // Caso complejo: el índice está dividido entre dos uint64_t
            uint8_t bitsInFirst = 64 - bitPosition;
            uint8_t bitsInSecond = bitsPerIndex - bitsInFirst;

            uint64_t lowBits = (indices[arrayIndex] >> bitPosition) & ((1ULL << bitsInFirst) - 1);
            uint64_t highBits = (indices[arrayIndex + 1] & ((1ULL << bitsInSecond) - 1));

            return (uint16_t)((highBits << bitsInFirst) | lowBits);
        }
    }

    // ⭐ BIT-PACKING: Establecer índice de paleta para un bloque específico
    void setPackedIndex(int blockIndex, uint16_t paletteIdx) {
        if (bitsPerIndex == 0) return; // Paleta de 1 elemento = no hay índices

        // Calcular posición
        size_t bitOffset = (size_t)blockIndex * bitsPerIndex;
        size_t arrayIndex = bitOffset / 64;
        size_t bitPosition = bitOffset % 64;

        // Crear máscara
        uint64_t mask = ((uint64_t)1 << bitsPerIndex) - 1;
        uint64_t value = (uint64_t)paletteIdx & mask;

        if (bitPosition + bitsPerIndex <= 64) {
            // Caso simple: cabe en un solo uint64_t
            // Limpiar los bits existentes y establecer los nuevos
            indices[arrayIndex] = (indices[arrayIndex] & ~(mask << bitPosition)) | (value << bitPosition);
        } else {
            // Caso complejo: dividido entre dos uint64_t
            uint8_t bitsInFirst = 64 - bitPosition;
            uint8_t bitsInSecond = bitsPerIndex - bitsInFirst;

            uint64_t maskFirst = ((1ULL << bitsInFirst) - 1) << bitPosition;
            uint64_t maskSecond = (1ULL << bitsInSecond) - 1;

            indices[arrayIndex] = (indices[arrayIndex] & ~maskFirst) | ((value & ((1ULL << bitsInFirst) - 1)) << bitPosition);
            indices[arrayIndex + 1] = (indices[arrayIndex + 1] & ~maskSecond) | ((value >> bitsInFirst) & maskSecond);
        }
    }

    // ⭐ Reorganizar paleta y recomprimir índices (cuando la paleta crece)
    void repack() {
        uint8_t newBitsPerIndex = calculateBitsPerIndex(palette.size());

        if (newBitsPerIndex == bitsPerIndex) return; // No hay cambio

        // Guardar índices antiguos
        std::vector<uint16_t> oldIndices;
        oldIndices.reserve(TOTAL_BLOCKS);
        for (int i = 0; i < TOTAL_BLOCKS; i++) {
            oldIndices.push_back(getPackedIndex(i));
        }

        // Crear nuevo arreglo de índices
        bitsPerIndex = newBitsPerIndex;
        indices.clear();
        indices.resize(calculateIndicesSize(bitsPerIndex), 0);

        // Re-empacar con los nuevos bits
        for (int i = 0; i < TOTAL_BLOCKS; i++) {
            setPackedIndex(i, oldIndices[i]);
        }
    }

public:
    // ⭐ CONSTRUCTOR: Inicializa con un solo tipo de bloque (optimización común: aire)
    PalettedSubChunk(BlockType initialBlock = static_cast<BlockType>(0)) {
        palette.push_back(initialBlock);
        bitsPerIndex = 0; // 1 elemento = 0 bits
        // No necesitamos índices si todo es el mismo bloque
    }

    // ⭐ OBTENER BLOQUE: Coordenadas locales [0-15] en X, Y, Z
    BlockType getBlock(int x, int y, int z) const {
        int blockIndex = y * 256 + z * 16 + x; // YZX order para mejor cache coherence

        if (palette.size() == 1) {
            return palette[0]; // Optimización: paleta de 1 elemento
        }

        uint16_t paletteIdx = getPackedIndex(blockIndex);
        return palette[paletteIdx];
    }

    // ⭐ ESTABLECER BLOQUE: Coordenadas locales [0-15]
    void setBlock(int x, int y, int z, BlockType blockType) {
        int blockIndex = y * 256 + z * 16 + x;

        // Buscar el bloque en la paleta
        auto it = std::find(palette.begin(), palette.end(), blockType);
        uint16_t paletteIdx;

        if (it != palette.end()) {
            // El bloque ya está en la paleta
            paletteIdx = (uint16_t)(it - palette.begin());
        } else {
            // Agregar nuevo bloque a la paleta
            paletteIdx = (uint16_t)palette.size();
            palette.push_back(blockType);

            // Si la paleta creció, puede que necesitemos más bits por índice
            repack();
        }

        // Establecer el índice
        setPackedIndex(blockIndex, paletteIdx);
    }

    // ⭐ VERIFICAR SI EL SUBCHUNK ESTÁ COMPLETAMENTE LLENO DE UN TIPO DE BLOQUE
    bool isUniform() const {
        return palette.size() == 1;
    }

    // ⭐ OBTENER EL BLOQUE UNIFORME (solo si isUniform() == true)
    BlockType getUniformBlock() const {
        return palette[0];
    }

    // ⭐ CALCULAR USO DE MEMORIA EN BYTES
    size_t getMemoryUsage() const {
        size_t paletteSize = palette.size() * sizeof(BlockType);
        size_t indicesSize = indices.size() * sizeof(uint64_t);
        return paletteSize + indicesSize + sizeof(PalettedSubChunk);
    }

    // ⭐ OBTENER TAMAÑO DE LA PALETA (para debugging/stats)
    size_t getPaletteSize() const {
        return palette.size();
    }

    // ⭐ SERIALIZACIÓN: Escribir a buffer binario
    void serialize(std::vector<uint8_t>& buffer) const {
        // Formato:
        // [1 byte: bitsPerIndex]
        // [2 bytes: palette size]
        // [N * sizeof(BlockType): palette data]
        // [M * 8 bytes: packed indices]

        buffer.push_back(bitsPerIndex);

        uint16_t paletteSize = (uint16_t)palette.size();
        buffer.push_back((uint8_t)(paletteSize & 0xFF));
        buffer.push_back((uint8_t)((paletteSize >> 8) & 0xFF));

        // Escribir paleta
        for (BlockType block : palette) {
            buffer.push_back((uint8_t)(block & 0xFF));
            buffer.push_back((uint8_t)((block >> 8) & 0xFF));
            buffer.push_back((uint8_t)((block >> 16) & 0xFF));
            buffer.push_back((uint8_t)((block >> 24) & 0xFF));
        }

        // Escribir índices
        for (uint64_t idx : indices) {
            for (int i = 0; i < 8; i++) {
                buffer.push_back((uint8_t)((idx >> (i * 8)) & 0xFF));
            }
        }
    }

    // ⭐ DESERIALIZACIÓN: Leer de buffer binario
    static PalettedSubChunk deserialize(const uint8_t* data, size_t& offset) {
        PalettedSubChunk subchunk;

        // Leer bitsPerIndex
        subchunk.bitsPerIndex = data[offset++];

        // Leer palette size
        uint16_t paletteSize = data[offset] | (data[offset + 1] << 8);
        offset += 2;

        // Leer paleta
        subchunk.palette.clear();
        subchunk.palette.reserve(paletteSize);
        for (uint16_t i = 0; i < paletteSize; i++) {
            int blockValue = data[offset] | (data[offset + 1] << 8) |
                           (data[offset + 2] << 16) | (data[offset + 3] << 24);
            BlockType block = static_cast<BlockType>(blockValue);
            subchunk.palette.push_back(block);
            offset += 4;
        }

        // Leer índices
        size_t indicesSize = calculateIndicesSize(subchunk.bitsPerIndex);
        subchunk.indices.clear();
        subchunk.indices.reserve(indicesSize);
        for (size_t i = 0; i < indicesSize; i++) {
            uint64_t idx = 0;
            for (int j = 0; j < 8; j++) {
                idx |= ((uint64_t)data[offset++]) << (j * 8);
            }
            subchunk.indices.push_back(idx);
        }

        return subchunk;
    }
};
