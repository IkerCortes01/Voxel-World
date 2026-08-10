#include <doctest/doctest.h>

#include "PalettedStorage.h"

#include <random>

// ============================================================================
// PalettedSubChunk: almacenamiento bit-packed de 16x16x16 bloques
// ============================================================================
// Es la fuente de verdad de getBlock() en el motor, así que un fallo aquí se
// traduce directamente en terreno mal guardado o mal dibujado.

TEST_CASE("PalettedSubChunk: arranca uniforme con el bloque inicial") {
    PalettedSubChunk sub(BLOCK_AIR);

    CHECK(sub.isUniform());
    CHECK(sub.getPaletteSize() == 1);
    CHECK(sub.getUniformBlock() == BLOCK_AIR);
    CHECK(sub.getBlock(0, 0, 0) == BLOCK_AIR);
    CHECK(sub.getBlock(15, 15, 15) == BLOCK_AIR);
    CHECK(sub.getBlock(7, 3, 11) == BLOCK_AIR);
}

TEST_CASE("PalettedSubChunk: un subchunk uniforme no gasta indices") {
    PalettedSubChunk uniform(BLOCK_STONE);
    PalettedSubChunk mixed(BLOCK_STONE);
    mixed.setBlock(0, 0, 0, BLOCK_DIRT);

    // El caso uniforme es la optimización principal: 0 bits por índice
    CHECK(uniform.getMemoryUsage() < mixed.getMemoryUsage());
}

TEST_CASE("PalettedSubChunk: set/get de un bloque") {
    PalettedSubChunk sub(BLOCK_AIR);
    sub.setBlock(5, 10, 3, BLOCK_STONE);

    CHECK(sub.getBlock(5, 10, 3) == BLOCK_STONE);
    CHECK_FALSE(sub.isUniform());
    CHECK(sub.getPaletteSize() == 2);

    // Los vecinos no se contaminan
    CHECK(sub.getBlock(4, 10, 3) == BLOCK_AIR);
    CHECK(sub.getBlock(5, 9, 3) == BLOCK_AIR);
    CHECK(sub.getBlock(5, 10, 2) == BLOCK_AIR);
}

TEST_CASE("PalettedSubChunk: las esquinas no se pisan entre si") {
    PalettedSubChunk sub(BLOCK_AIR);
    sub.setBlock(0, 0, 0, BLOCK_STONE);
    sub.setBlock(15, 0, 0, BLOCK_DIRT);
    sub.setBlock(0, 15, 0, BLOCK_SAND);
    sub.setBlock(0, 0, 15, BLOCK_WOOD);
    sub.setBlock(15, 15, 15, BLOCK_GRASS);

    CHECK(sub.getBlock(0, 0, 0) == BLOCK_STONE);
    CHECK(sub.getBlock(15, 0, 0) == BLOCK_DIRT);
    CHECK(sub.getBlock(0, 15, 0) == BLOCK_SAND);
    CHECK(sub.getBlock(0, 0, 15) == BLOCK_WOOD);
    CHECK(sub.getBlock(15, 15, 15) == BLOCK_GRASS);
}

TEST_CASE("PalettedSubChunk: sobrescribir un bloque mantiene el valor nuevo") {
    PalettedSubChunk sub(BLOCK_AIR);
    sub.setBlock(8, 8, 8, BLOCK_STONE);
    sub.setBlock(8, 8, 8, BLOCK_DIRT);

    CHECK(sub.getBlock(8, 8, 8) == BLOCK_DIRT);
}

// El repack ocurre al cruzar 1→2, 2→4, 4→16 y 16→256 entradas de paleta:
// es el punto donde todos los índices se re-empaquetan con otro ancho de bits
// y donde un error corrompería el subchunk entero de golpe.
TEST_CASE("PalettedSubChunk: los datos sobreviven al crecer la paleta (repack)") {
    PalettedSubChunk sub(BLOCK_AIR);

    // Escribir un bloque distinto en cada posición, cruzando todos los umbrales
    const BlockType types[] = {
        BLOCK_STONE, BLOCK_DIRT, BLOCK_GRASS, BLOCK_SAND, BLOCK_WOOD,
        BLOCK_LEAVES, BLOCK_WATER, BLOCK_GRAVEL, BLOCK_SNOW, BLOCK_LAVA,
        BLOCK_COAL_ORE, BLOCK_IRON_ORE, BLOCK_GOLD_ORE, BLOCK_GLASS,
        BLOCK_BRICKS, BLOCK_PLANKS, BLOCK_COBBLESTONE, BLOCK_BEDROCK,
        BLOCK_TALLGRASS, BLOCK_STICK
    };
    const int typeCount = static_cast<int>(sizeof(types) / sizeof(types[0]));

    for (int i = 0; i < typeCount; i++) {
        sub.setBlock(i, i, i, types[i]);
    }

    // Todo lo escrito antes de cada repack debe seguir intacto
    for (int i = 0; i < typeCount; i++) {
        CHECK(sub.getBlock(i, i, i) == types[i]);
    }
    CHECK(sub.getPaletteSize() == static_cast<size_t>(typeCount) + 1);  // +1 por BLOCK_AIR
}

TEST_CASE("PalettedSubChunk: subchunk lleno con patron pseudoaleatorio") {
    PalettedSubChunk sub(BLOCK_AIR);
    std::mt19937 rng(1234);

    BlockType expected[16][16][16];
    for (int y = 0; y < 16; y++) {
        for (int z = 0; z < 16; z++) {
            for (int x = 0; x < 16; x++) {
                BlockType t = static_cast<BlockType>(rng() % (BLOCK_TYPE_MAX + 1));
                expected[x][y][z] = t;
                sub.setBlock(x, y, z, t);
            }
        }
    }

    bool allMatch = true;
    for (int y = 0; y < 16 && allMatch; y++) {
        for (int z = 0; z < 16 && allMatch; z++) {
            for (int x = 0; x < 16 && allMatch; x++) {
                if (sub.getBlock(x, y, z) != expected[x][y][z]) allMatch = false;
            }
        }
    }
    CHECK(allMatch);
}

TEST_CASE("PalettedSubChunk: escribir el mismo tipo no infla la paleta") {
    PalettedSubChunk sub(BLOCK_AIR);
    for (int i = 0; i < 100; i++) {
        sub.setBlock(i % 16, (i / 16) % 16, 0, BLOCK_STONE);
    }
    CHECK(sub.getPaletteSize() == 2);  // AIR + STONE
}

TEST_CASE("PalettedSubChunk: serializar y deserializar preserva el contenido") {
    PalettedSubChunk original(BLOCK_AIR);
    original.setBlock(1, 2, 3, BLOCK_STONE);
    original.setBlock(4, 5, 6, BLOCK_DIRT);
    original.setBlock(15, 15, 15, BLOCK_LAVA);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);
    REQUIRE_FALSE(buffer.empty());

    size_t offset = 0;
    PalettedSubChunk restored = PalettedSubChunk::deserialize(buffer.data(), offset);

    CHECK(offset == buffer.size());
    CHECK(restored.getBlock(1, 2, 3) == BLOCK_STONE);
    CHECK(restored.getBlock(4, 5, 6) == BLOCK_DIRT);
    CHECK(restored.getBlock(15, 15, 15) == BLOCK_LAVA);
    CHECK(restored.getBlock(0, 0, 0) == BLOCK_AIR);
    CHECK(restored.getPaletteSize() == original.getPaletteSize());
}

TEST_CASE("PalettedSubChunk: serializar un subchunk uniforme") {
    PalettedSubChunk original(BLOCK_STONE);

    std::vector<uint8_t> buffer;
    original.serialize(buffer);

    size_t offset = 0;
    PalettedSubChunk restored = PalettedSubChunk::deserialize(buffer.data(), offset);

    CHECK(restored.isUniform());
    CHECK(restored.getUniformBlock() == BLOCK_STONE);
    CHECK(restored.getBlock(9, 9, 9) == BLOCK_STONE);
}
