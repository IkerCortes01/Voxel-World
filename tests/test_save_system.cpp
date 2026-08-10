#include <doctest/doctest.h>

// SimpleLZ4 y legacyCRC32v1 tienen enlace interno en SaveSystem.cpp, así que
// el test incluye la unidad de traducción entera en lugar de enlazarla. Por eso
// el target de tests NO enlaza SaveSystem.cpp (evita símbolos duplicados).
#include "SaveSystem.cpp"

#include <random>

using namespace VoxelWorld::SaveSystem;

static bool roundTrips(const std::vector<uint8_t>& original) {
    auto compressed = SimpleLZ4::compress(original.data(), original.size());
    auto restored = SimpleLZ4::decompress(compressed.data(), compressed.size(), original.size());
    return restored == original;
}

// ============================================================================
// Compresión RLE (formato v2)
// ============================================================================
// Regresión: en v1 el byte 0xFF se emitía crudo como literal y el descompresor
// lo leía como marcador de run, desplazando el resto del chunk. Cualquier
// BlockType cuyo byte valiera 255 destruía el chunk al recargarlo.

TEST_CASE("RLE: round-trip con literales 0xFF") {
    CHECK(roundTrips({0x01, 0xFF, 0x02}));
    CHECK(roundTrips({0xFF}));
    CHECK(roundTrips({0xFF, 0xFF}));
    CHECK(roundTrips({0xFF, 0xFF, 0xFF}));       // justo bajo el umbral de run
    CHECK(roundTrips({0xFF, 0xFF, 0xFF, 0xFF})); // justo en el umbral
}

TEST_CASE("RLE: round-trip con runs largos") {
    CHECK(roundTrips(std::vector<uint8_t>(100, 0xFF)));
    CHECK(roundTrips(std::vector<uint8_t>(1000, 0xFF)));  // supera el max de 255 por run
    CHECK(roundTrips(std::vector<uint8_t>(500, 0x00)));
    CHECK(roundTrips(std::vector<uint8_t>(255, 0x07)));
    CHECK(roundTrips(std::vector<uint8_t>(256, 0x07)));
}

TEST_CASE("RLE: round-trip con datos tipo chunk") {
    std::vector<uint8_t> chunk(65536, 0x00);
    for (int i = 1000; i < 2000; i++) chunk[i] = 0x03;
    chunk[5000] = 0xFF;
    chunk[5001] = 0xFF;
    for (int i = 9000; i < 9010; i++) chunk[i] = 0xFF;
    CHECK(roundTrips(chunk));
}

TEST_CASE("RLE: round-trip con datos aleatorios (peor caso, sin runs)") {
    std::mt19937 rng(42);
    std::vector<uint8_t> random(10000);
    for (auto& b : random) b = static_cast<uint8_t>(rng() & 0xFF);
    CHECK(roundTrips(random));
}

TEST_CASE("RLE: buffer vacio") {
    CHECK(roundTrips({}));
}

TEST_CASE("RLE: comprime de verdad los datos repetitivos") {
    std::vector<uint8_t> uniform(65536, 0x00);
    auto compressed = SimpleLZ4::compress(uniform.data(), uniform.size());
    CHECK(compressed.size() < uniform.size() / 10);
}

// ============================================================================
// Descompresión defensiva
// ============================================================================

TEST_CASE("RLE: rechaza marcador truncado") {
    std::vector<uint8_t> bad = {0x01, 0x02, 0xFF};
    CHECK(SimpleLZ4::decompress(bad.data(), bad.size(), 100).empty());
}

TEST_CASE("RLE: rechaza expansion mayor que dstSize") {
    // 100 runs de 255 bytes = 25500 bytes de salida declarados para un destino de 50
    std::vector<uint8_t> bomb;
    for (int i = 0; i < 100; i++) {
        bomb.push_back(0xFF);
        bomb.push_back(255);
        bomb.push_back(0xAA);
    }
    CHECK(SimpleLZ4::decompress(bomb.data(), bomb.size(), 50).empty());
}

TEST_CASE("RLE legacy v1: lee el formato viejo y respeta dstSize") {
    std::vector<uint8_t> v1 = {0xFF, 200, 0x07, 0x01, 0x02};  // run de 200 + 2 literales
    auto ok = SimpleLZ4::decompressLegacyV1(v1.data(), v1.size(), 202);
    REQUIRE(ok.size() == 202);
    CHECK(ok[0] == 0x07);
    CHECK(ok[199] == 0x07);
    CHECK(ok[200] == 0x01);

    CHECK(SimpleLZ4::decompressLegacyV1(v1.data(), v1.size(), 100).empty());
}

// ============================================================================
// CRC32
// ============================================================================
// Regresión: el CRC de v1 usaba una tabla vacía y un algoritmo degenerado, así
// que una corrupción de un byte pasaba desapercibida.

TEST_CASE("CRC32: coincide con el vector de verificacion estandar") {
    const char* s = "123456789";
    CHECK(CompressionSystem::calculateCRC32(reinterpret_cast<const uint8_t*>(s), 9) == 0xCBF43926u);
}

TEST_CASE("CRC32: detecta cambios de un solo bit") {
    std::vector<uint8_t> a(1000, 0x42);
    auto b = a;
    b[500] ^= 0x01;
    CHECK(CompressionSystem::calculateCRC32(a.data(), a.size()) !=
          CompressionSystem::calculateCRC32(b.data(), b.size()));
}

// ============================================================================
// ChunkSerializer
// ============================================================================

TEST_CASE("ChunkSerializer: round-trip completo preserva datos y metadata") {
    ChunkSerializer ser;

    std::vector<uint8_t> blockData(65536, 0x00);
    for (int i = 0; i < 100; i++) blockData[i * 300] = static_cast<uint8_t>(i % 32);
    blockData[123] = 0xFF;  // el caso que corrompía chunks en v1
    blockData[124] = 0xFF;

    ChunkMetadata meta{};
    meta.lastModified = 12345;
    meta.blockChanges = 7;

    auto serialized = ser.serialize(3, -5, blockData.data(), blockData.size(), meta);
    REQUIRE(ser.validate(serialized));

    std::vector<uint8_t> out(65536, 0xCC);
    ChunkMetadata metaOut{};
    REQUIRE(ser.deserialize(serialized, out.data(), out.size(), metaOut));

    CHECK(out == blockData);
    CHECK(metaOut.lastModified == 12345);
    CHECK(metaOut.blockChanges == 7);
}

TEST_CASE("ChunkSerializer: rechaza datos corruptos sin leer fuera del buffer") {
    ChunkSerializer ser;
    std::vector<uint8_t> blockData(65536, 0x05);
    ChunkMetadata meta{};
    auto serialized = ser.serialize(0, 0, blockData.data(), blockData.size(), meta);

    std::vector<uint8_t> out(65536);
    ChunkMetadata metaOut{};

    SUBCASE("un byte alterado dispara el CRC") {
        auto corrupted = serialized;
        corrupted[sizeof(ChunkSaveHeader) + 10] ^= 0x55;
        CHECK_FALSE(ser.deserialize(corrupted, out.data(), out.size(), metaOut));
    }

    SUBCASE("compressedSize gigante se rechaza (antes: over-read de heap)") {
        auto oversized = serialized;
        reinterpret_cast<ChunkSaveHeader*>(oversized.data())->compressedSize = 0xFFFFFF00u;
        CHECK_FALSE(ser.deserialize(oversized, out.data(), out.size(), metaOut));
    }

    SUBCASE("buffer truncado se rechaza") {
        auto truncated = serialized;
        truncated.resize(sizeof(ChunkSaveHeader) + 5);
        CHECK_FALSE(ser.deserialize(truncated, out.data(), out.size(), metaOut));
    }

    SUBCASE("magic invalido se rechaza") {
        auto badMagic = serialized;
        badMagic[0] = 'X';
        CHECK_FALSE(ser.deserialize(badMagic, out.data(), out.size(), metaOut));
    }

    SUBCASE("version futura se rechaza") {
        auto futureVer = serialized;
        reinterpret_cast<ChunkSaveHeader*>(futureVer.data())->version = 99;
        CHECK_FALSE(ser.deserialize(futureVer, out.data(), out.size(), metaOut));
    }

    SUBCASE("buffer mas corto que el header se rechaza") {
        std::vector<uint8_t> tiny(4, 0);
        CHECK_FALSE(ser.deserialize(tiny, out.data(), out.size(), metaOut));
    }
}

TEST_CASE("ChunkSerializer: rechaza destino de tamano distinto al guardado") {
    ChunkSerializer ser;
    std::vector<uint8_t> blockData(1024, 0x09);
    ChunkMetadata meta{};
    auto serialized = ser.serialize(0, 0, blockData.data(), blockData.size(), meta);

    std::vector<uint8_t> wrongSize(2048);
    ChunkMetadata metaOut{};
    CHECK_FALSE(ser.deserialize(serialized, wrongSize.data(), wrongSize.size(), metaOut));
}
